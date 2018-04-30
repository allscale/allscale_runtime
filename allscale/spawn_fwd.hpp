
#ifndef ALLSCALE_SPAWN_FWD_HPP
#define ALLSCALE_SPAWN_FWD_HPP

#include <allscale/treeture_fwd.hpp>

namespace allscale
{
    // forward declaration for dependencies class
    namespace runtime {
        // A class aggregating task dependencies
        struct dependencies;
    }

    template <typename WorkItemDescription, typename ...Ts>
    treeture<typename WorkItemDescription::result_type>
    spawn_with_dependencies(const runtime::dependencies& deps, Ts&&...vs);

    template <typename WorkItemDescription, typename ...Ts>
    treeture<typename WorkItemDescription::result_type>
    spawn(Ts&&...vs);

    template <typename WorkItemDescription, typename ...Ts>
    treeture<typename WorkItemDescription::result_type>
    spawn_first_with_dependencies(const runtime::dependencies& deps, Ts&&...vs);

    template <typename WorkItemDescription, typename ...Ts>
    treeture<typename WorkItemDescription::result_type>
    spawn_first(Ts&&...vs);
}

#endif
