#ifndef ALLSCALE_DATA_ITEM_MANAGER
#define ALLSCALE_DATA_ITEM_MANAGER

#include <allscale/data_item_manager_impl.hpp>
#include <allscale/data_item_reference.hpp>
#include <allscale/components/data_item.hpp>
#include <allscale/data_item_requirement.hpp>
#include <allscale/lease.hpp>
#include "allscale/utils/serializer.h"

#include <hpx/include/components.hpp>
#include <hpx/util/detail/yield_k.hpp>

#include <type_traits>

namespace allscale
{
    struct data_item_manager
    {
        template <typename DataItemType, typename...Args>
        static allscale::data_item_reference<DataItemType>
        create(Args&&...args)
        {
            auto dm = data_item_manager_impl<DataItemType>::get_ptr();
            return
                data_item_reference<DataItemType>(
                    hpx::local_new<detail::id_holder>(
                        [](hpx::naming::gid_type const& id)
                        {
                            data_item_manager_impl<DataItemType>::get_ptr()->destroy(id);
                        }
                    ), std::forward<Args>(args)...
                );
        }

        template<typename DataItemType>
        static typename DataItemType::facade_type
        get(const allscale::data_item_reference<DataItemType>& ref)
        {
            return data_item_manager_impl<DataItemType>::get(ref);
		}

        template<typename DataItemType>
		static hpx::future<allscale::lease<DataItemType>>
        acquire(const allscale::data_item_requirement<DataItemType>& requirement)
        {
            return data_item_manager_impl<DataItemType>::acquire(requirement);
		}

        template<typename DataItemType>
		static void release(const allscale::lease<DataItemType>& lease)
        {
            return data_item_manager_impl<DataItemType>::release(lease);
        }

        template<typename DataItemType>
		static std::vector<hpx::future<allscale::lease<DataItemType>>>
        acquire(std::vector<allscale::data_item_requirement<DataItemType>> const & requirement)
        {
            return data_item_manager_impl<DataItemType>::acquire(requirement);
		}

        template<typename DataItemType>
		static void release(const std::vector<allscale::lease<DataItemType>>& lease)
        {
            return data_item_manager_impl<DataItemType>::release(lease);
        }

        template<typename DataItemType>
		static void destroy(const data_item_reference<DataItemType>& ref)
        {
            data_item_manager_impl<DataItemType>::destroy(ref);
        }
    };
}//end namespace allscale

#define REGISTER_DATAITEMSERVER_DECLARATION(type)                               \
    namespace allscale{                                                         \
        template<>                                                              \
        struct data_item_server_name<type>                                      \
        {                                                                       \
            static const char* name()                                           \
            {                                                                   \
                return BOOST_PP_STRINGIZE(                                      \
                    BOOST_PP_CAT(allscale/data_item_, type));                   \
            }                                                                   \
        };                                                                      \
    }                                                                           \

#define REGISTER_DATAITEMSERVER(type)                                           \
    typedef ::hpx::components::component<                                       \
        allscale::components::data_item<type>                                   \
    > BOOST_PP_CAT(__data_item_server_, type);                                  \
    HPX_REGISTER_COMPONENT(BOOST_PP_CAT(__data_item_server_, type))             \
    template struct allscale::data_item_registry<type>;                         \
    template struct allscale::data_item_manager_impl<type>;                     \
/**/
#endif
