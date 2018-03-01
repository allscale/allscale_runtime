#ifndef ALLSCALE_DATA_ITEM_MANAGER
#define ALLSCALE_DATA_ITEM_MANAGER

#include <allscale/data_item_manager/acquire.hpp>
#include <allscale/data_item_manager/data_item_store.hpp>
#include <allscale/data_item_reference.hpp>
#include <allscale/data_item_requirement.hpp>
#include <allscale/lease.hpp>
#include "allscale/utils/serializer.h"

#include <hpx/include/components.hpp>
#include <hpx/util/detail/yield_k.hpp>

#include <type_traits>

namespace allscale { namespace data_item_manager {
        template <typename DataItemType, typename...Args>
        allscale::data_item_reference<DataItemType>
        create(Args&&...args)
        {
            typedef typename DataItemType::shared_data_type shared_data_type;

            auto ref = data_item_reference<DataItemType>(
                    hpx::local_new<allscale::detail::id_holder>(
                        [](hpx::naming::gid_type const& id)
                        {
//                             data_item_manager_impl<DataItemType>::get_ptr()->destroy(id);
                        }
                    )
                );
            auto &item = data_item_store<DataItemType>::lookup(ref);
            item.shared_data.reset(new shared_data_type(std::forward<Args>(args)...));
            return ref;
        }

        template<typename DataItemType>
        typename DataItemType::facade_type
        get(const allscale::data_item_reference<DataItemType>& ref)
        {
            if (ref.fragment == nullptr)
            {
                auto &item = data_item_store<DataItemType>::lookup(ref);
                HPX_ASSERT(item.fragment);
                ref.fragment = item.fragment.get();
            }

            return ref.fragment->mask();
		}

        template <typename T>
        void release(T&&)
        {
        }

//         template<typename DataItemType>
// 		void release(const allscale::lease<DataItemType>& lease)
//         {
// //             if (lease.mode == access_mode::Invalid)
// //                 return;
// //             data_item_manager_impl<DataItemType>::release(lease);
//         }
//
//         template<typename DataItemType>
// 		void release(const std::vector<allscale::lease<DataItemType>>& lease)
//         {
// //             for(auto const& l: lease)
// //                 if (l.mode == access_mode::Invalid) return;
// //
// //             data_item_manager_impl<DataItemType>::release(lease);
//         }

        template<typename DataItemType>
		void destroy(const data_item_reference<DataItemType>& ref)
        {
//             data_item_manager_impl<DataItemType>::destroy(ref);
        }
}}//end namespace allscale

#define REGISTER_DATAITEMSERVER_DECLARATION(type)                               \
    HPX_REGISTER_ACTION_DECLARATION(                                            \
        allscale::data_item_manager::detail::transfer_action<type>,             \
        HPX_PP_CAT(transfer_action_, type))                                     \
/**/

#define REGISTER_DATAITEMSERVER(type)                                           \
    HPX_REGISTER_ACTION(                                                        \
        allscale::data_item_manager::detail::transfer_action<type>,             \
        HPX_PP_CAT(transfer_action_, type))                                     \
/**/

#endif
