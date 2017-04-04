#ifndef ALLSCALE_COMPONENTS_FRAGMENT_HPP
#define ALLSCALE_COMPONENTS_FRAGMENT_HPP

#include <hpx/include/components.hpp>
#include <iostream>

namespace allscale { namespace components {
    template <typename Region, typename T>
    struct fragment
        : hpx::components::managed_component_base<fragment<Region,T> >
    {};
}}
