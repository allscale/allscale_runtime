
#include <allscale/config.hpp>
#include <allscale/hierarchy.hpp>
#include <allscale/scheduler.hpp>
#include <allscale/monitor.hpp>
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
        std::unique_ptr<scheduling_policy> create_policy()
        {
            // FIXME: add random policy...
            return tree_scheduling_policy::create_uniform(
                allscale::get_num_numa_nodes(), allscale::get_num_localities());
        }
    }

    struct scheduler_service
    {
        std::unique_ptr<scheduling_policy> policy_;
        runtime::HierarchyAddress here_;
        runtime::HierarchyAddress root_;
        runtime::HierarchyAddress parent_;
        runtime::HierarchyAddress left_;
        runtime::HierarchyAddress right_;
        hpx::id_type parent_id_;
        hpx::id_type right_id_;
        bool is_root_;
        std::size_t depth_;

        scheduler_service(runtime::HierarchyAddress here)
          : here_(here)
          , policy_(create_policy())
          , root_(runtime::HierarchyAddress::getRootOfNetworkSize(
                allscale::get_num_numa_nodes(), allscale::get_num_localities()
                ))
          , parent_(here_.getParent())
          , is_root_(here_ == root_)
          , depth_(root_.getLayer() - here_.getLayer())
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

        void schedule(work_item work)
        {
            auto reqs = work.get_task_requirements();

            if (!work.enqueue_remote())
            {
                HPX_ASSERT(reqs->check_write_requirements(here_));
                scheduler::get().schedule_local(
                    std::move(work), std::move(reqs), here_, work.id().depth());
                return;
            }

            // check whether current node is allowed to make autonomous scheduling decisions
            auto path = work.id().path;
            if (!is_root_
                    // test that this virtual node is allowed to interfere with the scheduling
                    // of this task
                    && policy_->is_involved(here_, path)
                    // test that this virtual node has control over all required data
                    && reqs->check_write_requirements(here_))
            {
                schedule_down(std::move(work), std::move(reqs));
                return;
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

            return schedule_down(std::move(work), std::move(reqs));
        }

        void schedule_down(work_item work, std::unique_ptr<data_item_manager::task_requirements_base> reqs)
        {
            HPX_ASSERT(here_.getRank() == hpx::get_locality_id());
            HPX_ASSERT(policy_->is_involved(here_, work.id().path));

            reqs->mark_write_requirements(here_);

            HPX_ASSERT(reqs->check_write_requirements(here_));

            auto id = work.id();
            // TODO: check whether left or right node covers all write requirements

            // ask the scheduling policy what to do with this task
            // on leaf level, schedule locally
            auto d = here_.isLeaf() ? schedule_decision::stay : policy_->decide(here_, id.path);
            HPX_ASSERT(d != schedule_decision::done);

            // if it should stay, process it here
            if (d == schedule_decision::stay)
            {
//                 TODO
//                 // add granted allowances
//                 diis.addAllowanceLocal(allowance);
//                 HPX_ASSERT(reqs->check_write_requirements(here_));

                scheduler::get().schedule_local(
                    std::move(work), std::move(reqs), here_, id.depth());
                return;
            }

            bool target_left = (d == schedule_decision::left);

            // FIXME: add DIM stuff...
            if (target_left)
            {
                reqs->get_missing_regions_left(here_);
                runtime::HierarchicalOverlayNetwork::getLocalService<scheduler_service>(left_).
                    schedule_down(std::move(work), std::move(reqs));
            }
            else
            {
                reqs->get_missing_regions_right(here_);
                if (!right_id_)
                {
                    runtime::HierarchicalOverlayNetwork::getLocalService<scheduler_service>(right_).
                        schedule_down(std::move(work), std::move(reqs));
                }
                else
                {
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

        local_scheduler_service.schedule(std::move(work));
//         get().enqueue(work);
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
