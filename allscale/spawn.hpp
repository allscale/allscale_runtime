
#ifndef ALLSCALE_SPAWN_HPP
#define ALLSCALE_SPAWN_HPP

#include <allscale/spawn_fwd.hpp>
#include <allscale/work_item.hpp>
#include <allscale/scheduler.hpp>

namespace allscale
{
    // forward declaration for dependencies class
    namespace runtime {
        // A class aggregating task dependencies
        struct dependencies
        {
            dependencies() =default;
            explicit dependencies(hpx::future<void>&& dep) : dep_(std::move(dep)) {}
            hpx::shared_future<void> dep_;
        };
    }

    template <typename WorkItemDescription, typename ...Ts>
    treeture<typename WorkItemDescription::result_type>
    spawn_with_dependencies(const runtime::dependencies& deps, Ts&&...vs)
    {
        typedef typename WorkItemDescription::result_type result_type;

        allscale::treeture<void> parent(this_work_item::get_id().get_treeture());
        if (!parent)
            parent = treeture<void>{};
        allscale::treeture<result_type> tres(parent_arg(), parent);

        work_item wi(false, WorkItemDescription(), deps.dep_, tres, std::forward<Ts>(vs)...);

        scheduler::schedule(std::move(wi));

        HPX_ASSERT(tres.valid());

        return tres;
    }

    template <typename WorkItemDescription, typename ...Ts>
    treeture<typename WorkItemDescription::result_type>
    spawn(Ts&&...vs)
    {
        return spawn_with_dependencies<WorkItemDescription>(
            runtime::dependencies(hpx::future<void>()), std::forward<Ts>(vs)...);
    }

    template <typename WorkItemDescription, typename ...Ts>
    treeture<typename WorkItemDescription::result_type>
    spawn_first_with_dependencies(const runtime::dependencies& deps, Ts&&...vs)
    {
        typedef typename WorkItemDescription::result_type result_type;
        allscale::treeture<void> null_parent;
        allscale::treeture<result_type> tres(parent_arg(), null_parent);

        scheduler::schedule(
            work_item(true, WorkItemDescription(), deps.dep_, tres, std::forward<Ts>(vs)...)
        );

        HPX_ASSERT(tres.valid());

        return tres;
    }

    template <typename WorkItemDescription, typename ...Ts>
    treeture<typename WorkItemDescription::result_type>
    spawn_first(Ts&&...vs)
    {
        return spawn_first_with_dependencies<WorkItemDescription>(
            runtime::dependencies(hpx::future<void>()), std::forward<Ts>(vs)...);
    }
}

#endif
