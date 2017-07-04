
#include <allscale/scheduler.hpp>
#include <allscale/monitor.hpp>
#include <allscale/components/scheduler.hpp>

#include <hpx/include/components.hpp>
#include <hpx/util/detail/yield_k.hpp>

HPX_REGISTER_COMPONENT_MODULE()

typedef hpx::components::component<allscale::components::scheduler> scheduler_component;
HPX_REGISTER_COMPONENT(scheduler_component)

namespace allscale
{
    std::size_t scheduler::rank_ = std::size_t(-1);

    void scheduler::schedule(work_item work)
    {
    	std::cout<<"schedulign work item on loc " << hpx::get_locality_id()<<std::endl;
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
            hpx::util::detail::yield_k(k, "get component...");
            res = s.component_.get();
        }
        return res;
    }

    scheduler::scheduler(std::size_t rank)
    {
        hpx::id_type gid =
            hpx::local_new<components::scheduler>(rank).get();

        hpx::register_with_basename("allscale/scheduler", gid, rank).get();

        component_ = hpx::get_ptr<components::scheduler>(gid).get();
        component_->init();
    }
}
