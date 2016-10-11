
#ifndef ALLSCALE_COMPONENTS_DATA_ITEM_HPP
#define ALLSCALE_COMPONENTS_DATA_ITEM_HPP

#include <hpx/include/components.hpp>

namespace allscale { namespace components {
    template <typename T>
    struct data_item
        : hpx::components::managed_component_base<data_item<T> >  
    {

        data_item()
        {}

        data_item(T t)
        {
        }

    };
}}


#endif
