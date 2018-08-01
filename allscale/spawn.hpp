
#ifndef ALLSCALE_SPAWN_HPP
#define ALLSCALE_SPAWN_HPP

#include <allscale/detail/futurize_if.hpp>
#include <allscale/spawn_fwd.hpp>
#include <allscale/work_item.hpp>
#include <allscale/scheduler.hpp>
#include <allscale/task_id.hpp>

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

    namespace detail
    {
        template <typename WorkItemDescription, typename ...Ts>
        treeture<typename WorkItemDescription::result_type>
        spawn(const runtime::dependencies& deps, task_id id, Ts&&...vs)
        {
            typedef typename WorkItemDescription::result_type result_type;

            using work_item_type =
                detail::work_item_impl<
                    WorkItemDescription,
                        hpx::util::tuple<
                            typename hpx::util::decay<
                            decltype(detail::futurize_if(std::forward<Ts>(vs)))>::type...
                        >
                    >;

            auto wi = std::make_shared<work_item_type>(id, deps.dep_,
                        hpx::util::make_tuple(detail::futurize_if(std::forward<Ts>(vs))...));

            allscale::treeture<result_type> tres = wi->get_treeture();

            scheduler::schedule(work_item(std::move(wi)));

            return tres;
        }
    }

    template <typename WorkItemDescription, typename ...Ts>
    treeture<typename WorkItemDescription::result_type>
    spawn_with_dependencies(const runtime::dependencies& deps, Ts&&...vs)
    {
        return detail::spawn<WorkItemDescription>(
            deps, task_id::create_child(), std::forward<Ts>(vs)...);
    }

    template <typename WorkItemDescription, typename ...Ts>
    treeture<typename WorkItemDescription::result_type>
    spawn(Ts&&...vs)
    {
        return detail::spawn<WorkItemDescription>(
            runtime::dependencies(hpx::future<void>()), task_id::create_child(),
            std::forward<Ts>(vs)...);
    }

    template <typename WorkItemDescription, typename ...Ts>
    treeture<typename WorkItemDescription::result_type>
    spawn_first_with_dependencies(const runtime::dependencies& deps, Ts&&...vs)
    {
        return detail::spawn<WorkItemDescription>(
            deps, task_id::create_root(), std::forward<Ts>(vs)...);
    }

    template <typename WorkItemDescription, typename ...Ts>
    treeture<typename WorkItemDescription::result_type>
    spawn_first(Ts&&...vs)
    {
        return detail::spawn<WorkItemDescription>(runtime::dependencies(hpx::future<void>()),
            task_id::create_root(), std::forward<Ts>(vs)...);
    }
}

#endif
