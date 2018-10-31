
#include <allscale/config.hpp>
#include <allscale/hierarchy.hpp>
#include <allscale/scheduler.hpp>
#include <allscale/monitor.hpp>
#include <allscale/optimizer.hpp>
#include <allscale/resilience.hpp>
#include <allscale/schedule_policy.hpp>
#include <allscale/components/scheduler.hpp>
#include <allscale/get_num_numa_nodes.hpp>
#include <allscale/get_num_localities.hpp>
#include <allscale/data_item_manager/index_service.hpp>
#include <allscale/data_item_manager/task_requirements.hpp>
#include <allscale/utils/printer/vectors.h>

#include <hpx/include/actions.hpp>
#include <hpx/apply.hpp>
#include <hpx/util/detail/yield_k.hpp>
#include <hpx/util/register_locks.hpp>
#include <hpx/runtime/threads/policies/local_priority_queue_scheduler.hpp>
#include <hpx/runtime/threads/detail/scheduled_thread_pool.hpp>

#include <hpx/runtime/components/component_factory_base.hpp>

HPX_REGISTER_COMPONENT_MODULE()

namespace allscale
{

    void schedule_global(runtime::HierarchyAddress addr, work_item work);
    void schedule_down_global(runtime::HierarchyAddress addr, work_item work, std::unique_ptr<data_item_manager::task_requirements_base> reqs);

    void toggle_node_broadcast(std::size_t locality_id);
    void set_policy_broadcast(std::string policy);
}

HPX_PLAIN_DIRECT_ACTION(allscale::schedule_global, schedule_global_action);
HPX_PLAIN_DIRECT_ACTION(allscale::schedule_down_global, schedule_down_global_action);
HPX_PLAIN_DIRECT_ACTION(allscale::toggle_node_broadcast, toggle_node_action);
HPX_PLAIN_DIRECT_ACTION(allscale::set_policy_broadcast, set_policy_action);

namespace allscale
{
    scheduler::mutex_type scheduler::active_mtx_;
    bool scheduler::active_ = true;
    bool scheduler::active()
    {
        std::lock_guard<mutex_type> l(active_mtx_);
        return active_;
    }

    void scheduler::toggle_active(bool toggle)
    {
        std::lock_guard<mutex_type> l(active_mtx_);
        if (toggle)
            active_ = !active_;
        else
            active_ = true;
    }

    void scheduler::partition_resources(hpx::resource::partitioner& rp)
    {
        auto const& numa_domains = rp.numa_domains();
//         rp.create_thread_pool("default");
//         std::cerr << "===============================================================================\n";
//         std::cerr << "Setting up Scheduler\n";

        std::string input_objective_str = hpx::get_config_entry("allscale.objective", "none");
//         std::cerr << "  Scheduler objective is " << input_objective_str << "\n";
        bool enable_elasticity = false;
        if ( !input_objective_str.empty() )
        {
            std::istringstream iss_leeways(input_objective_str);
            std::string objective_str;
            while (iss_leeways >> objective_str)
            {
                auto idx = objective_str.find(':');
                std::string obj = objective_str;
                // Don't scale objectives if none is given
                double leeway = 1.0;

                if (idx != std::string::npos)
                {
                    obj = objective_str.substr(0, idx);
                    leeway = std::stod( objective_str.substr(idx + 1) );
                }

                if (obj == "time")
                {
                    enable_elasticity = true;
                    break;
                }
                else if (obj == "resource")
                {
                    enable_elasticity = true;
                    break;
                }
            }
        }

        rp.set_default_pool_name("allscale-numa-0");

        std::size_t domain = 0;
        for (auto& numa: numa_domains)
        {
            std::string pool_name = "allscale-numa-" + std::to_string(domain);

            rp.create_thread_pool(pool_name,
                [domain, enable_elasticity](hpx::threads::policies::callback_notifier& notifier,
                    std::size_t num_threads, std::size_t thread_offset,
                    std::size_t pool_index, std::string const& pool_name)
                -> std::unique_ptr<hpx::threads::thread_pool_base>
                {
                    // instantiate the scheduler
                    typedef hpx::threads::policies::local_priority_queue_scheduler<
                        hpx::compat::mutex, hpx::threads::policies::lockfree_fifo>
                        local_sched_type;
                    local_sched_type::init_parameter_type init(num_threads,
                        num_threads, 1000, 1,
                        "core-local_priority_queue_scheduler");
                    std::unique_ptr<local_sched_type> sched(
                        new local_sched_type(init));

                    hpx::threads::policies::scheduler_mode mode =
                        hpx::threads::policies::scheduler_mode::delay_exit;

                    mode = hpx::threads::policies::scheduler_mode(
                        hpx::threads::policies::scheduler_mode::do_background_work
                      | mode);

                    if(enable_elasticity)
                        mode = hpx::threads::policies::scheduler_mode(
                            hpx::threads::policies::scheduler_mode::enable_elasticity
                          | mode);

                    std::unique_ptr<hpx::threads::thread_pool_base> pool(
                        new hpx::threads::detail::scheduled_thread_pool<
                                local_sched_type
                            >(std::move(sched), notifier,
                                pool_index, pool_name, mode, thread_offset));
                    return pool;
                });

            for (auto& core: numa.cores())
            {
                for (auto& pu: core.pus())
                {
                        rp.add_resource(pu, pool_name);
                }
            }

            ++domain;
        }
    }

    namespace {
        struct replacable_policy
        {
            enum policy_value
            {
                static_,
                dynamic,
                tuned,
                random
            };

            policy_value value_;
            std::unique_ptr<scheduling_policy> policy_;

            std::string policy()
            {
                switch (value_)
                {
                    case static_:
                        return "uniform";
                    case dynamic:
                        return "balanced";
                    case tuned:
                        return "tuned";
                    case random:
                        return "random";
                    default:
                        return "unknown";
                }
            }
        };

        replacable_policy create_policy(std::string const& policy)
        {
            if (policy == "uniform")
            {
                return {
                    replacable_policy::static_,
                    tree_scheduling_policy::create_uniform(allscale::get_num_localities())
                };
            }
            if (policy == "balanced")
            {
                return {
                    replacable_policy::dynamic,
                    tree_scheduling_policy::create_uniform(allscale::get_num_localities())
                };
            }
            if (policy == "tuned")
            {
                return {
                    replacable_policy::tuned,
                    tree_scheduling_policy::create_uniform(allscale::get_num_localities())
                };
            }
            if (policy == "random")
            {
                return {
                    replacable_policy::random,
                    std::unique_ptr<scheduling_policy>(new random_scheduling_policy(
                        runtime::HierarchyAddress::getRootOfNetworkSize(allscale::get_num_localities()))
                    )
                };
            }

            return {
                replacable_policy::static_,
                tree_scheduling_policy::create_uniform(allscale::get_num_localities())
            };
        }

        replacable_policy create_policy()
        {
            char * env = std::getenv("ALLSCALE_SCHEDULING_POLICY");
            if (env)
            {
                return create_policy(env);
            }
            return create_policy("uniform");
        }
    }

    struct scheduler_service
    {

        using mutex_type = hpx::lcos::local::spinlock;
        mutex_type mtx_;
        replacable_policy policy_;
        std::vector<bool> mask_;
        runtime::HierarchyAddress here_;
        runtime::HierarchyAddress root_;
        runtime::HierarchyAddress parent_;
        runtime::HierarchyAddress left_;
        runtime::HierarchyAddress right_;
        hpx::id_type parent_id_;
        hpx::id_type right_id_;
        bool is_root_;
        global_optimizer optimizer_;

        scheduler_service(scheduler_service&& other)
          : policy_(std::move(other.policy_))
          , mask_(std::move(other.mask_))
          , here_(std::move(other.here_))
          , root_(std::move(other.root_))
          , parent_(std::move(other.parent_))
          , left_(std::move(other.left_))
          , right_(std::move(other.right_))
          , parent_id_(std::move(other.parent_id_))
          , right_id_(std::move(other.right_id_))
          , is_root_(other.is_root_)
          , optimizer_(std::move(other.optimizer_))
        {
            HPX_ASSERT(false);
        }

        scheduler_service(runtime::HierarchyAddress here)
          : policy_(create_policy())
          , mask_(allscale::get_num_localities(), true)
          , here_(here)
          , root_(
                runtime::HierarchyAddress::getRootOfNetworkSize(allscale::get_num_localities()))
          , parent_(here_.getParent())
          , is_root_(here_ == root_)
        {
            if (parent_.getRank() != scheduler::rank())
            {
                parent_id_ = hpx::naming::get_id_from_locality_id(
                    parent_.getRank());
            }


            if (!here_.isLeaf())
            {
                left_ = here_.getLeftChild();
                right_ = here_.getRightChild();
                if (right_.getRank() >= allscale::get_num_localities())
                {
                    right_ = left_;
                }

                HPX_ASSERT(left_.getRank() == scheduler::rank());

                if (right_.getRank() != scheduler::rank())
                {
                    right_id_ = hpx::naming::get_id_from_locality_id(
                        right_.getRank());
                }
            }

            if (is_root_) run();
        }

        std::string policy()
        {
            return policy_.policy();
        }

        void toggle_node(std::size_t locality_id)
        {
            std::lock_guard<mutex_type> l(mtx_);
            mask_[locality_id] = !mask_[locality_id];

            if (policy_.value_ == replacable_policy::random)
            {
                policy_.value_ = replacable_policy::static_;
                policy_.policy_ = tree_scheduling_policy::create_uniform(mask_);
            }
            else
            {
                policy_.policy_ = tree_scheduling_policy::create_uniform(mask_);
            }
        }

        void set_policy(std::string policy)
        {
            auto pol = create_policy(policy);
            {
                std::lock_guard<mutex_type> l(mtx_);
                mask_ = std::vector<bool>(mask_.size(), true);

                policy_ = std::move(pol);
            }
        }

        void update_policy(task_times const& times, std::vector<bool> const& mask)
        {
            if (!(policy_.value_ == replacable_policy::dynamic || policy_.value_ == replacable_policy::tuned)) return;

            auto new_policy = tree_scheduling_policy::create_rebalanced(*policy_.policy_, times, mask, is_root_);

            {
                std::lock_guard<mutex_type> l(mtx_);

                mask_ = mask;
                policy_.policy_ = std::move(new_policy);
            }
        }

        bool balance()
        {
            HPX_ASSERT(is_root_);

            if (policy_.value_ == replacable_policy::dynamic)
            {
                optimizer_.balance(false);
            }

            if (policy_.value_ == replacable_policy::tuned)
            {
                optimizer_.balance(true);
            }

            return true;
        }

        void run()
        {
        }

        void stop()
        {
        }

        void schedule(work_item work)
        {
            if (is_root_ && work.id().is_root() && work.id().id % 10 == 0)
            {
                balance();
            }

            auto reqs = work.get_task_requirements();
            if (!work.enqueue_remote())
            {
                reqs->get_missing_regions(here_);
                schedule_local(std::move(work), std::move(reqs));
                return;
            }

            // check whether current node is allowed to make autonomous scheduling decisions
            auto path = work.id().path;
            if (!is_root_)
            {
                bool is_involved = false;
                {
                    // test that this virtual node is allowed to interfere with the scheduling
                    // of this task
                    std::lock_guard<mutex_type> l(mtx_);
                    is_involved = (path.isRoot()) ? is_root_ : policy_.policy_->is_involved(here_, path.getParentPath());
//                     is_involved = policy_.policy_->is_involved(here_, path);
                }
                if (is_involved
                    // test that this virtual node has control over all required data
                    && reqs->check_write_requirements(here_))
                {
//                     std::cout << here_ << ' ' << work.name() << "." << work.id() << ": shortcut " << '\n';
                    schedule_down(std::move(work), std::move(reqs));
                    return;
                }
            }

            bool unallocated = reqs->get_missing_regions(here_);
            // If there are unallocated allowances still to process, do so
            if (unallocated)
            {
//                 std::cout << "unallocated...\n";
                schedule_down(std::move(work), std::move(reqs));
                return;
            }

            // if we are not the root, we need to propagate to our parent...
            if (!is_root_)
            {
                if (!parent_id_)
                {
                    runtime::HierarchicalOverlayNetwork::getLocalService<scheduler_service>(parent_.getLayer()).
                        schedule(std::move(work));
                }
                else
                {
                    hpx::apply<schedule_global_action>(parent_id_, parent_, std::move(work));
                }
                return;
            }

            schedule_down(std::move(work), std::move(reqs));
            return;
        }

        void schedule_down(work_item work, std::unique_ptr<data_item_manager::task_requirements_base> reqs)
        {
            HPX_ASSERT(here_.getRank() == hpx::get_locality_id());
//             HPX_ASSERT(policy_.policy_->is_involved(here_, work.id().path));

            auto id = work.id();
            // TODO: check whether left and right node covers all...

            // ask the scheduling policy what to do with this task
            // on leaf level, schedule locally
            bool is_involved = false;
            schedule_decision d = schedule_decision::done;
            {
                std::lock_guard<mutex_type> l(mtx_);
                is_involved = policy_.policy_->is_involved(here_, id.path);
                if (is_involved)
                {
                    d = policy_.policy_->decide(here_, id.path);
                }
            }

            // if this is not involved, send task to parent
            if (!is_involved)
            {
                HPX_ASSERT(!is_root_);
                if (!parent_id_)
                {
                    runtime::HierarchicalOverlayNetwork::getLocalService<scheduler_service>(parent_.getLayer()).
                        schedule(std::move(work));
                }
                else
                {
                    hpx::apply<schedule_global_action>(parent_id_, parent_, std::move(work));
                }
                return;
            }

            HPX_ASSERT(d != schedule_decision::done);

            // if it should stay, process it here
            if (d == schedule_decision::stay || here_.isLeaf())
            {
                schedule_local(std::move(work), std::move(reqs));
                return;
            }

            bool target_left = (d == schedule_decision::left) || (right_ == left_);

	    // don't try to schedule on a failed locality
 	    if (!target_left && !allscale::resilience::rank_running(right_.getRank())) {
                 schedule_local(std::move(work), std::move(reqs));
 	    }
            else if (target_left)
            {
//                 std::cout << here_ << ' ' << work.name() << "." << id << ": left: " << '\n';
//                 reqs->show();
                reqs->add_allowance_left(here_);
                runtime::HierarchicalOverlayNetwork::getLocalService<scheduler_service>(left_.getLayer()).
                    schedule_down(std::move(work), std::move(reqs));
            }
            else
            {
//                 std::cout << here_ << ' ' << work.name() << "." << id << ": right: " << '\n';
//                 reqs->show();
                reqs->add_allowance_right(here_);

                HPX_ASSERT(right_id_);

                allscale::resilience::global_wi_dispatched(work, right_.getRank());
                hpx::apply<schedule_down_global_action>(right_id_, right_, std::move(work), std::move(reqs));
            }
            return;
        }

        void schedule_local(work_item work,
            std::unique_ptr<data_item_manager::task_requirements_base>&& reqs)
        {
//                 HPX_ASSERT(reqs->check_write_requirements(here_));

//             std::cout << here_ << ' ' << work.name() << "." << work.id() << ": local: " << '\n';
//             reqs->show();
            auto res = scheduler::get().schedule_local(
                std::move(work), std::move(reqs), here_);
            if (!res.first.valid())
            {
                HPX_ASSERT(res.second == nullptr);
                return;
            }

            HPX_ASSERT(!here_.isLeaf());
//             std::cout << here_ << ' ' << work.name() << "." << id << ": left: " << '\n';
//             reqs->show();
            res.second->add_allowance_left(here_);
            HPX_ASSERT(left_.getLayer() == here_.getLayer()-1);
            runtime::HierarchicalOverlayNetwork::getLocalService<scheduler_service>(left_.getLayer()).
                schedule_local(std::move(res.first), std::move(res.second));
        }
    };

    void scheduler::schedule(work_item&& work)
    {
        // Calculate our local root address ...
        auto &local_scheduler_service =
            runtime::HierarchicalOverlayNetwork::getLocalService<scheduler_service>();

        allscale::monitor::signal(allscale::monitor::work_item_enqueued, work);

        local_scheduler_service.schedule(std::move(work));
//         get().enqueue(work);
    }

    void toggle_node_broadcast(std::size_t locality_id)
    {
        runtime::HierarchicalOverlayNetwork::forAllLocal<scheduler_service>(
            [&](scheduler_service& sched)
            {
                sched.toggle_node(locality_id);
            }
        );

        if (hpx::get_locality_id() == locality_id)
        {
            scheduler::toggle_active();
        }
    }

    hpx::future<void> scheduler::toggle_node(std::size_t locality_id)
    {
        return hpx::lcos::broadcast<toggle_node_action>(hpx::find_all_localities(),
            locality_id);
    }

    void set_policy_broadcast(std::string policy)
    {
        runtime::HierarchicalOverlayNetwork::forAllLocal<scheduler_service>(
            [&](scheduler_service& sched)
            {
                sched.set_policy(policy);
            }
        );
        // This sets the state to active again...
        scheduler::toggle_active(false);
    }

    hpx::future<void> scheduler::set_policy(std::string policy)
    {
        return hpx::lcos::broadcast<set_policy_action>(hpx::find_all_localities(),
            policy);
    }

    std::string scheduler::policy()
    {
        return runtime::HierarchicalOverlayNetwork::getLocalService<scheduler_service>().
            policy();
    }

    void scheduler::update_policy(task_times const& times, std::vector<bool> mask)
    {
        runtime::HierarchicalOverlayNetwork::forAllLocal<scheduler_service>(
            [&](scheduler_service& sched)
            {
                sched.update_policy(times, mask);
            }
        );
    }

    void schedule_global(runtime::HierarchyAddress addr, work_item work)
    {
        runtime::HierarchicalOverlayNetwork::getLocalService<scheduler_service>(addr.getLayer()).
            schedule(std::move(work));
    }

    void schedule_down_global(runtime::HierarchyAddress addr, work_item work, std::unique_ptr<data_item_manager::task_requirements_base> reqs)
    {
        runtime::HierarchicalOverlayNetwork::getLocalService<scheduler_service>(addr.getLayer()).
            schedule_down(std::move(work), std::move(reqs));
    }

    std::size_t scheduler::rank()
    {
        return get().rank_;
    }

    components::scheduler* scheduler::run(std::size_t rank)
    {
        runtime::HierarchicalOverlayNetwork hierarchy;

        hierarchy.installService<scheduler_service>();

        data_item_manager::init_index_services(hierarchy);

        return get_ptr();
    }

    void scheduler::stop()
    {
        auto addr = runtime::HierarchyAddress::getRootOfNetworkSize(allscale::get_num_localities());
        runtime::HierarchicalOverlayNetwork::getLocalService<scheduler_service>(addr.getLayer()).stop();
        get().stop();
//         get_ptr().reset();
    }

    components::scheduler &scheduler::get()
    {
        HPX_ASSERT(get_ptr());
        return *get_ptr();
    }

    components::scheduler* scheduler::get_ptr()
    {
        static scheduler s(hpx::get_locality_id());
        s.component_->init();
        return s.component_.get();
    }

    scheduler::scheduler(std::size_t rank)
    {
        component_.reset(new components::scheduler(rank));
    }
}
