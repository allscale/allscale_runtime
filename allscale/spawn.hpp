
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
        typedef typename WorkItemDescription::result_type result_type;
        HPX_ASSERT(this_work_item::get_id().get_treeture());

        allscale::treeture<void> parent(this_work_item::get_id().get_treeture());
        allscale::treeture<result_type> tres(parent_arg(), parent);

        work_item wi(false, WorkItemDescription(), tres, std::forward<Ts>(vs)...);

        std::size_t idx = wi.id().last();
        parent.set_child(idx, tres);

        scheduler::schedule(std::move(wi));

        HPX_ASSERT(tres.valid());

        return tres;
    }

    template <typename WorkItemDescription, typename ...Ts>
    treeture<typename WorkItemDescription::result_type>
    spawn_first(Ts&&...vs)
    {
        typedef typename WorkItemDescription::result_type result_type;
        HPX_ASSERT(!this_work_item::get_id().get_treeture());
        allscale::treeture<void> null_parent;
        allscale::treeture<result_type> tres(parent_arg(), null_parent);

        scheduler::schedule(
            work_item(true, WorkItemDescription(), tres, std::forward<Ts>(vs)...)
        );

        HPX_ASSERT(tres.valid());

        return tres;
    }
}

#endif
