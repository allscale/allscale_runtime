#ifndef ALLSCALE_COMPONENTS_DATA_ITEM_MANAGER_SERVER_HPP
#define ALLSCALE_COMPONENTS_DATA_ITEM_MANAGER_SERVER_HPP

#include <hpx/include/components.hpp>
#include <hpx/hpx.hpp>
#include <memory>
#include <allscale/data_item_base.hpp>

namespace allscale { namespace components {
    struct data_item_manager_server
      : hpx::components::managed_component_base<data_item_manager_server >
    {
        typedef hpx::components::managed_component_base<data_item_manager_server> server_type;

        template <typename T>
        hpx::future<hpx::naming::id_type>   create_data_item_async( T  arg)
        {
            using data_item_type = allscale::data_item<T>;
            hpx::lcos::local::promise<hpx::naming::id_type>  promise_;

            std::shared_ptr<data_item_base> ptr (new data_item_type(hpx::find_here()));
            local_data_items.push_back(ptr);
            data_item_type k = *(std::static_pointer_cast<data_item_type>(ptr));
            promise_.set_value(k.get_id());
            return promise_.get_future();
        }

        template <typename T>
        struct create_data_item_async_action
          : hpx::actions::make_action<
                hpx::future<hpx::naming::id_type>(data_item_manager_server::*)(T),
                &data_item_manager_server::template create_data_item_async<T>,
                create_data_item_async_action<T>
            >
        {};


        std::vector<std::shared_ptr<data_item_base> > local_data_items;
    };
}}




#endif
