
#ifndef ALLSCALE_DATA_ITEM_HPP
#define ALLSCALE_DATA_ITEM_HPP

#include <allscale/components/data_item.hpp>
#include <allscale/traits/is_data_item.hpp>
//#include <allscale/data_item_description.hpp>
#include <allscale/data_item_base.hpp>
#include <hpx/include/future.hpp>
#include <hpx/include/lcos.hpp>

#include <iostream>
#include <memory>
#include <utility>

namespace allscale
{
    template <typename DataItemDescription>
    struct data_item
    : hpx::components::client_base<data_item<DataItemDescription>, components::data_item<DataItemDescription> > , data_item_base
    {
        public:
            using region_type = typename DataItemDescription::region_type;
            using fragment_type = typename DataItemDescription::fragment_type;
            using collection_facade = typename DataItemDescription::collection_facade;


            using base_type = hpx::components::client_base<data_item<DataItemDescription>, components::data_item<DataItemDescription> >;


            data_item()
            {
            }

            data_item(hpx::id_type loc)
              : base_type(hpx::new_<components::data_item<DataItemDescription> >(loc))
            {
                std::cout<<"Creating data item on locality: " << loc << std::endl;
                parent_loc = loc;
                HPX_ASSERT(this->valid());
            }

            data_item(hpx::future<hpx::naming::id_type> id)
              : base_type(std::move(id))
            {}

            data_item(hpx::shared_future<hpx::naming::id_type> id)
              : base_type(id)
            {}


/*
            data_item(const data_item &obj)
                :
            {
                std::cout << "Copy constructor allocating ptr." << std::endl;
            }
*/
            /*
            data_item(data_item&& d) noexcept : parent_loc(std::move(d.parent_loc))

            {

            }
            */

            hpx::id_type parent_loc;
    };

    namespace traits
    {
        template <typename T>
        struct is_data_item< ::allscale::data_item<T>>
          : std::true_type
        {};
    }
}




#define ALLSCALE_REGISTER_DATA_ITEM_TYPE_(DataItemDescription, NAME)            \
    using NAME =                                                                \
        ::hpx::components::managed_component<                                   \
            ::allscale::components::data_item< DataItemDescription >            \
        >;                                                                      \
    HPX_REGISTER_COMPONENT(NAME)                                                \
/**/

#define ALLSCALE_REGISTER_DATA_ITEM_TYPE(DataItemDescription)                   \
    ALLSCALE_REGISTER_DATA_ITEM_TYPE_(                                           \
        DataItemDescription,                                                                      \
        BOOST_PP_CAT(BOOST_PP_CAT(data_item, DataItemDescription), _component_type)                \
    )                                                                           \
/**/



#endif
