
#include <allscale/scheduler.hpp>
#include <allscale/monitor.hpp>
#include <allscale/components/scheduler.hpp>

#include <hpx/include/components.hpp>
#include <hpx/util/detail/yield_k.hpp>
#include <hpx/util/register_locks.hpp>
#include <hpx/runtime/threads/policies/local_priority_queue_scheduler.hpp>
#include <hpx/runtime/threads/detail/scheduled_thread_pool.hpp>

HPX_REGISTER_COMPONENT_MODULE()

typedef hpx::components::component<allscale::components::scheduler> scheduler_component;
HPX_REGISTER_COMPONENT(scheduler_component)

namespace allscale
{
    std::size_t scheduler::rank_ = std::size_t(-1);

    void scheduler::partition_resources(hpx::resource::partitioner& rp)
    {
        auto const& numa_domains = rp.numa_domains();
//         rp.create_thread_pool("default");
        std::cout << "===============================================================================\n";
        std::cout << "Setting up Scheduler\n";
        std::cout << "  Scheduling is using " << numa_domains.size() << " NUMA Domains\n";

        std::size_t domain = 0;
        for (auto& numa: numa_domains)
        {
            std::string pool_name;
            if (domain == 0)
                pool_name = "default";
            else
                pool_name = "allscale/numa/" + std::to_string(domain);
            std::cout << "  Creating \"" << pool_name << "\" thread pool:\n";

            rp.create_thread_pool(pool_name,
                [domain](hpx::threads::policies::callback_notifier& notifier,
                    std::size_t num_threads, std::size_t thread_offset,
                    std::size_t pool_index, std::string const& pool_name)
                -> std::unique_ptr<hpx::threads::detail::thread_pool_base>
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

                    hpx::threads::policies::scheduler_mode mode;
                    if (domain == 0)
                        mode = hpx::threads::policies::scheduler_mode(
                            hpx::threads::policies::scheduler_mode::do_background_work |
                            hpx::threads::policies::scheduler_mode::delay_exit);
                    else
                        mode = hpx::threads::policies::scheduler_mode(
                            hpx::threads::policies::scheduler_mode::delay_exit);

                    std::unique_ptr<hpx::threads::detail::thread_pool_base> pool(
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
                    std::cout << "    Adding PU " << pu.id() << '\n';
                    rp.add_resource(pu, pool_name);
                }
            }

            ++domain;
        }

        std::cout << "===============================================================================\n";
    }

    void scheduler::schedule(work_item work)
    {
//    	std::cout<<"schedulign work item on loc " << hpx::get_locality_id()<<std::endl;
        get().enqueue(work, this_work_item::id());
    }

    components::scheduler* scheduler::run(std::size_t rank)
    {
        static this_work_item::id main_id(0);
        this_work_item::set_id(main_id);
        rank_ = rank;
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
        static scheduler s(rank_);
        components::scheduler* res = s.component_.get();
        for (std::size_t k = 0; !res; ++k)
        {
            hpx::util::detail::yield_k(k, "scheduler::get_ptr");
            res = s.component_.get();
        }
        return res;
    }

    scheduler::scheduler(std::size_t rank)
    {
        std::unique_lock<mutex_type> l(mtx_);
        hpx::util::ignore_while_checking<std::unique_lock<mutex_type>> il(&l);

        hpx::id_type gid =
            hpx::local_new<components::scheduler>(rank).get();

        hpx::register_with_basename("allscale/scheduler", gid, rank).get();

        component_ = hpx::get_ptr<components::scheduler>(gid).get();
        component_->init();
    }
}
