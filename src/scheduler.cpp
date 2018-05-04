
#include <allscale/config.hpp>
#include <allscale/scheduler.hpp>
#include <allscale/monitor.hpp>
#include <allscale/resilience.hpp>
#include <allscale/components/scheduler.hpp>

#include <hpx/util/detail/yield_k.hpp>
#include <hpx/util/register_locks.hpp>
#include <hpx/runtime/threads/policies/local_priority_queue_scheduler.hpp>
#include <hpx/runtime/threads/detail/scheduled_thread_pool.hpp>

#include <hpx/runtime/components/component_factory_base.hpp>

HPX_REGISTER_COMPONENT_MODULE()

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

    void scheduler::schedule(work_item work)
    {
        get().enqueue(work, this_work_item::id());
    }

    void scheduler::schedule(work_item work, this_work_item::id id)
    {
        get().enqueue(work, std::move(id));
    }

    components::scheduler* scheduler::run(std::size_t rank)
    {
        static this_work_item::id main_id(0);
        this_work_item::set_id(main_id);
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
