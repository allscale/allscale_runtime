
#ifndef ALLSCALE_COMPONENTS_DATA_ITEM_HPP
#define ALLSCALE_COMPONENTS_DATA_ITEM_HPP

#include <hpx/include/components.hpp>
#include <iostream>
namespace allscale { namespace components {
    template <typename T>
    struct data_item
        : hpx::components::managed_component_base<data_item<T> >  
    {

        data_item()
        {
        }


        data_item(hpx::id_type loc)
        {
        }


        data_item(T t)
        {
        }


        data_item(data_item const& other)
        {
        }


    };
}}


#endif
