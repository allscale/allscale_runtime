
#ifndef ALLSCALE_SPAWN_HPP
#define ALLSCALE_SPAWN_HPP

#include <allscale/work_item.hpp>
#include <allscale/scheduler.hpp>

namespace allscale
{

    using dependencies = std::vector<task_reference>;

    template<typename ... Refs>
    dependencies after(const Refs& ... refs) {
        return dependencies{ refs ... };
    }


    template <typename WorkItemDescription, typename ...Ts>
    treeture<typename WorkItemDescription::result_type>
    spawn(Ts&&...vs)
    {
        allscale::treeture<typename WorkItemDescription::result_type> tres(hpx::find_here());

        scheduler::schedule(
            work_item(false, WorkItemDescription(), tres, std::forward<Ts>(vs)...)
        );

        HPX_ASSERT(tres.valid());

        return tres;
    }

    template <typename WorkItemDescription, typename ...Ts>
    treeture<typename WorkItemDescription::result_type>
    spawn_after(const dependencies& deps, Ts&&...vs)
    {
        // TODO: forward those dependencies to the scheduler, such that it can handle those 
        //       for now it will wait for them here
        for(const auto& cur : deps) cur.wait();

        allscale::treeture<typename WorkItemDescription::result_type> tres(hpx::find_here());

        scheduler::schedule(
            work_item(false, WorkItemDescription(), tres, std::forward<Ts>(vs)...)
        );

        HPX_ASSERT(tres.valid());

        return tres;
    }

    template <typename WorkItemDescription, typename ...Ts>
    treeture<typename WorkItemDescription::result_type>
    spawn_first(Ts&&...vs)
    {
        allscale::treeture<typename WorkItemDescription::result_type> tres(hpx::find_here());

        scheduler::schedule(
            work_item(true, WorkItemDescription(), tres, std::forward<Ts>(vs)...)
        );

        HPX_ASSERT(tres.valid());

        return tres;
    }


    template <typename WorkItemDescription, typename ...Ts>
    treeture<typename WorkItemDescription::result_type>
    spawn_first_after(const dependencies& deps, Ts&&...vs)
    {
        // TODO: forward those dependencies to the scheduler, such that it can handle those 
        //       for now it will wait for them here
        for(const auto& cur : deps) cur.wait();

        allscale::treeture<typename WorkItemDescription::result_type> tres(hpx::find_here());

        scheduler::schedule(
            work_item(true, WorkItemDescription(), tres, std::forward<Ts>(vs)...)
        );

        HPX_ASSERT(tres.valid());

        return tres;
    }
}

#endif
