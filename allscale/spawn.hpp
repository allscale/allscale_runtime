
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
        allscale::treeture<typename WorkItemDescription::result_type> tres(hpx::find_here());

        scheduler::schedule(
            work_item(WorkItemDescription(), tres, std::forward<Ts>(vs)...)
        );

        HPX_ASSERT(tres.valid());

        return tres;
    }
}

#endif
