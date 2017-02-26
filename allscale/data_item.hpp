
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
            using future_type = hpx::future<DataItemDescription>;

            data_item()
            {
            }

            data_item( data_item const& other)
            : base_type(other),
                parent_loc(other.parent_loc),
                region_(other.region_),
                fragment_(other.fragment_)
            {
            }

            data_item( data_item && other)
            : base_type(std::move(other)),
              parent_loc(std::move(other.parent_loc)),
              region_(std::move(other.region_)),
              fragment_(std::move(other.fragment_))
            {
                //std::cout << " move operator: other id is " << other.get_id() << " my id is " << this->get_id() <<  std::endl;
//                 HPX_ASSERT(this->valid());
//                 HPX_ASSERT(!other.valid());
            }


            data_item& operator=(const data_item& other)
            {
                if (other.valid())
                {
                  this->base_type::operator=(static_cast<base_type const&>(other));
                  region_ = other.region_;
                  parent_loc = other.parent_loc;
                  fragment_ = other.fragment_;
                  HPX_ASSERT(this->valid());
                }
                return *this;
            }

            data_item& operator=(data_item&& other)
            {
                if (other.valid())
                {
                  this->base_type::operator=(std::move(static_cast<base_type&&>(other)));
                  region_ = std::move(other.region_);
                  parent_loc = std::move(other.parent_loc);
                  fragment_ = std::move(other.fragment_);
                  HPX_ASSERT(this->valid());
                  HPX_ASSERT(!other.valid());
                }
                return *this;
            }


            data_item(hpx::id_type const& id)
              : base_type(id)
            {
                HPX_ASSERT(this->valid());
            }

            data_item(hpx::id_type loc, DataItemDescription descr)
              : base_type(hpx::new_<components::data_item<DataItemDescription> >(loc))
            {
                region_ = descr.r_;
                parent_loc = loc;
                HPX_ASSERT(this->valid());
            }


            data_item(hpx::id_type loc, DataItemDescription descr, fragment_type frag)
              : base_type(hpx::new_<components::data_item<DataItemDescription> >(loc))
            {

                region_ = descr.r_;
                parent_loc = loc;
                fragment_ = frag;
                HPX_ASSERT(this->valid());
            }


            data_item(hpx::future<hpx::naming::id_type> id)
              : base_type(std::move(id))
            {
                std::cout<<"TEST" << std::endl;
            }

            data_item(hpx::shared_future<hpx::naming::id_type> id)
              : base_type(id)
            {
                std::cout<<"TEST2" << std::endl;
            }

            fragment_type fragment_;
            region_type region_;
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
