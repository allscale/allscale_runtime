
#ifndef ALLSCALE_SPAWN_HPP
#define ALLSCALE_SPAWN_HPP

#include <allscale/work_item.hpp>
#include <allscale/scheduler.hpp>

namespace allscale
{
    template <typename WorkItemDescription, typename ...Ts>
    treeture<typename WorkItemDescription::result_type>
    spawn(Ts&&...vs)
    {
        HPX_ASSERT(this_work_item::get().valid());
        typedef typename WorkItemDescription::result_type result_type;

        allscale::treeture<result_type> parent(this_work_item::get().get_treeture<result_type>());
        allscale::treeture<result_type> tres(hpx::find_here(), parent.get_id());

        work_item wi(false, WorkItemDescription(), tres, std::forward<Ts>(vs)...);

        std::size_t idx = wi.id().last();
        parent.set_child(idx, tres.get_id());

        scheduler::schedule(std::move(wi));

        HPX_ASSERT(tres.valid());

        return tres;
    }

    template <typename WorkItemDescription, typename ...Ts>
    treeture<typename WorkItemDescription::result_type>
    spawn_first(Ts&&...vs)
    {
        HPX_ASSERT(!this_work_item::get().valid());
        allscale::treeture<typename WorkItemDescription::result_type> tres(hpx::find_here());

        scheduler::schedule(
            work_item(true, WorkItemDescription(), tres, std::forward<Ts>(vs)...)
        );

        HPX_ASSERT(tres.valid());

        return tres;
    }
}

#endif
