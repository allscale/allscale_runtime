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
            return
                data_item_reference<DataItemType>(
                    hpx::local_new<detail::id_holder>(), std::forward<Args>(args)...
                );
        }

        template<typename DataItemType>
        static typename DataItemType::facade_type
        get(const allscale::data_item_reference<DataItemType>& ref)
        {
            return data_item_manager_impl<DataItemType>::get(ref);
		}

        template<typename DataItemType>
		static allscale::lease<DataItemType>
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
		static void destroy(const data_item_reference<DataItemType>& ref)
        {
            data_item_manager_impl<DataItemType>::destroy(ref);
        }
    };
}//end namespace allscale
#endif
