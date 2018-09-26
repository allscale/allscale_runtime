
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
}

HPX_PLAIN_DIRECT_ACTION(allscale::schedule_global, schedule_global_action);
HPX_PLAIN_DIRECT_ACTION(allscale::schedule_down_global, schedule_down_global_action);

namespace allscale
{
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

//         std::cerr << "  Scheduling is using " << numa_domains.size() << " NUMA Domains\n";
        rp.set_default_pool_name("allscale-numa-0");

        std::size_t domain = 0;
        bool skip = true;
        for (auto& numa: numa_domains)
        {
            std::string pool_name = "allscale-numa-" + std::to_string(domain);
//             std::cerr << "  Creating \"" << pool_name << "\" thread pool:\n";

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

                    if (domain == 0)
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
//                     if (skip && core.pus().size() > 1 && resilience::enabled())
//                     {
//                         std::cerr << "    Using PU " << pu.id() << " for performing backups\n";
//                         rp.add_resource(pu, "resilience");
//                         skip = false;
//                     }
//                     else
//                     {
//                         std::cerr << "    Adding PU " << pu.id() << '\n';
                        rp.add_resource(pu, pool_name);
//                     }
                }
            }

            ++domain;
        }

//         std::cerr << "===============================================================================\n";
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
        };

        replacable_policy create_policy()
        {
            char * env = std::getenv("ALLSCALE_SCHEDULING_POLICY");
            if (env)
            {
                if (env == std::string("uniform"))
                {
                    return {
                        replacable_policy::static_,
                        tree_scheduling_policy::create_uniform(
                            allscale::get_num_numa_nodes(), allscale::get_num_localities())
                    };
                }
                if (env == std::string("dynamic"))
                {
                    return {
                        replacable_policy::dynamic,
                        tree_scheduling_policy::create_uniform(
                            allscale::get_num_numa_nodes(), allscale::get_num_localities())
                    };
                }
                if (env == std::string("tuned"))
                {
                    return {
                        replacable_policy::tuned,
                        tree_scheduling_policy::create_uniform(
                            allscale::get_num_numa_nodes(), allscale::get_num_localities())
                    };
                }
                if (env == std::string("random"))
                {
                    return {
                        replacable_policy::random,
                        std::unique_ptr<scheduling_policy>(new random_scheduling_policy())
                    };
                }
            }

            return {
                replacable_policy::static_,
                tree_scheduling_policy::create_uniform(
                    allscale::get_num_numa_nodes(), allscale::get_num_localities())
            };
        }
    }

    struct scheduler_service
    {

        using mutex_type = hpx::lcos::local::spinlock;
        mutex_type mtx_;
        replacable_policy policy_;
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
          , here_(std::move(other.here_))
          , root_(std::move(other.root_))
          , parent_(std::move(other.parent_))
          , left_(std::move(other.left_))
          , right_(std::move(other.right_))
          , parent_id_(std::move(other.parent_id_))
          , right_id_(std::move(other.right_id_))
          , is_root_(other.is_root_)
          , optimizer_(std::move(other.optimizer_))
        {}

        scheduler_service(runtime::HierarchyAddress here)
          : here_(here)
          , policy_(create_policy())
          , root_(runtime::HierarchyAddress::getRootOfNetworkSize(
                allscale::get_num_numa_nodes(), allscale::get_num_localities()
                ))
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
        }

        void update_policy(std::vector<optimizer_state> const& state, std::vector<bool> mask)
        {
            if (!(policy_.value_ == replacable_policy::dynamic || policy_.value_ == replacable_policy::tuned)) return;

            std::lock_guard<mutex_type> l(mtx_);
            policy_.policy_ = tree_scheduling_policy::create_rebalanced(*policy_.policy_, state, mask);
        }

        void schedule(work_item work)
        {
            auto reqs = work.get_task_requirements();

            if (policy_.value_ == replacable_policy::dynamic &&
                work.id().is_root() && work.id().id > 0 && (work.id().id % 10 == 0))
            {
                optimizer_.balance(false);
            }

            if (policy_.value_ == replacable_policy::tuned &&
                work.id().is_root() && work.id().id > 0 && (work.id().id % 10 == 0))
            {
                optimizer_.balance(true);
            }

            if (!work.enqueue_remote())
            {
                reqs->get_missing_regions(here_);
//                 HPX_ASSERT(reqs->check_write_requirements(here_));
                reqs->add_allowance(here_);
                scheduler::get().schedule_local(
                    std::move(work), std::move(reqs), here_, work.id().depth());
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
                    is_involved = policy_.policy_->is_involved(here_, path);
                }
                if (is_involved
                    // test that this virtual node has control over all required data
                    && reqs->check_write_requirements(here_))
                {
//                 std::cout << here_ << ' ' << work.name() << "." << work.id() << ": shortcut " << '\n';
                    schedule_down(std::move(work), std::move(reqs));
                    return;
                }
            }

            // if we are not the root, we need to propagate to our parent...
            if (!is_root_)
            {
                if (!parent_id_)
                {
                    runtime::HierarchicalOverlayNetwork::getLocalService<scheduler_service>(parent_).
                        schedule(std::move(work));
                }
                else
                {
                    hpx::apply<schedule_global_action>(parent_id_, parent_, std::move(work));
                }
                return;
            }

            reqs->get_missing_regions(here_);
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
                    runtime::HierarchicalOverlayNetwork::getLocalService<scheduler_service>(parent_).
                        schedule(std::move(work));
                }
                else
                {
                    hpx::apply<schedule_global_action>(parent_id_, parent_, std::move(work));
                }
                return;
            }

            if (here_.isLeaf())
            {
                d = schedule_decision::stay;
            }

            HPX_ASSERT(d != schedule_decision::done);

            // if it should stay, process it here
            if (d == schedule_decision::stay)
            {
//                 std::cout << here_ << ' ' << work.name() << "." << id << ": local: " << '\n';
//                 reqs->show();
                // add granted allowances
                reqs->add_allowance(here_);
//                 HPX_ASSERT(reqs->check_write_requirements(here_));

                scheduler::get().schedule_local(
                    std::move(work), std::move(reqs), here_, id.depth());
                return;
            }

            bool target_left = (d == schedule_decision::left) || (right_ == left_);

            if (target_left || !allscale::resilience::rank_running(right_.getRank()))
            {
//                 std::cout << here_ << ' ' << work.name() << "." << id << ": left: " << '\n';
//                 reqs->show();
                reqs->add_allowance_left(here_);
                runtime::HierarchicalOverlayNetwork::getLocalService<scheduler_service>(left_).
                    schedule_down(std::move(work), std::move(reqs));
            }
            else
            {
//                 std::cout << here_ << ' ' << work.name() << "." << id << ": right: " << '\n';
//                 reqs->show();
                reqs->add_allowance_right(here_);

                if (!right_id_ && allscale::resilience::rank_running(right_.getRank()))
                {
                    runtime::HierarchicalOverlayNetwork::getLocalService<scheduler_service>(right_).
                        schedule_down(std::move(work), std::move(reqs));
                }
                else
		{
		    allscale::resilience::global_wi_dispatched(work, right_.getRank());
                    hpx::apply<schedule_down_global_action>(right_id_, right_, std::move(work), std::move(reqs));
                }
            }
            return;
        }
    };

    void scheduler::schedule(work_item&& work)
    {
        // Calculate our local root address ...
        static auto rank = get().rank_;
        static auto local_layers = runtime::HierarchyAddress::getLayersOn(
            rank, 0, allscale::get_num_numa_nodes(),  allscale::get_num_localities());
        static auto addr = runtime::HierarchyAddress(rank, 0, local_layers - 1);
        static auto &local_scheduler_service =
            runtime::HierarchicalOverlayNetwork::getLocalService<scheduler_service>(addr);

        allscale::monitor::signal(allscale::monitor::work_item_enqueued, work);

        local_scheduler_service.schedule(std::move(work));
//         get().enqueue(work);
    }

    void scheduler::update_policy(std::vector<optimizer_state> const& state, std::vector<bool> mask)
    {
        runtime::HierarchicalOverlayNetwork::forAllLocal<scheduler_service>(
            [&](scheduler_service& sched)
            {
                sched.update_policy(state, mask);
            }
        );
    }

    void schedule_global(runtime::HierarchyAddress addr, work_item work)
    {
        runtime::HierarchicalOverlayNetwork::getLocalService<scheduler_service>(addr).
            schedule(std::move(work));
    }

    void schedule_down_global(runtime::HierarchyAddress addr, work_item work, std::unique_ptr<data_item_manager::task_requirements_base> reqs)
    {
        runtime::HierarchicalOverlayNetwork::getLocalService<scheduler_service>(addr).
            schedule_down(std::move(work), std::move(reqs));
    }

    std::size_t scheduler::rank()
    {
        return get().rank_;
    }

    components::scheduler* scheduler::run(std::size_t rank)
    {
        runtime::HierarchyAddress::numaCutOff = std::ceil(std::log2(allscale::get_num_numa_nodes()));
        runtime::HierarchicalOverlayNetwork hierarchy;

        hierarchy.installService<scheduler_service>();

        data_item_manager::init_index_services(hierarchy);

        return get_ptr();
    }

    void scheduler::stop()
    {
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
