#ifndef ALLSCALE_DATA_ITEM_MANAGER
#define ALLSCALE_DATA_ITEM_MANAGER

#include <allscale/config.hpp>
#include <allscale/data_item_manager/acquire.hpp>
#include <allscale/data_item_manager/data_item_store.hpp>
#include <allscale/data_item_manager/fragment.hpp>
#include <allscale/data_item_manager/shared_data.hpp>
#include <allscale/data_item_reference.hpp>
#include <allscale/data_item_requirement.hpp>
#include <allscale/lease.hpp>
#include "allscale/utils/serializer.h"

#include <hpx/runtime/components/new.hpp>
#include <hpx/util/detail/yield_k.hpp>

#include <type_traits>

namespace allscale { namespace data_item_manager {
        template <typename DataItem, typename...Args>
        allscale::data_item_reference<DataItem>
        create(Args&&...args)
        {
            typedef typename DataItem::shared_data_type shared_data_type;

            auto ref = data_item_reference<DataItem>(
                    hpx::local_new<allscale::detail::id_holder>(
                        [](hpx::naming::gid_type const& id)
                        {
//                             data_item_manager_impl<DataItem>::get_ptr()->destroy(id);
                        }
                    )
                );
            auto &item = data_item_store<DataItem>::lookup(ref);
            item.shared_data.reset(new shared_data_type(std::forward<Args>(args)...));
            return ref;
        }

        template<typename DataItem>
        typename DataItem::facade_type
        get(const allscale::data_item_reference<DataItem>& ref)
        {
            return fragment(ref).mask();
		}

        template <typename T>
        void release(T&&)
        {
        }

//         template<typename DataItem>
// 		void release(const allscale::lease<DataItem>& lease)
//         {
// //             if (lease.mode == access_mode::Invalid)
// //                 return;
// //             data_item_manager_impl<DataItem>::release(lease);
//         }
//
//         template<typename DataItem>
// 		void release(const std::vector<allscale::lease<DataItem>>& lease)
//         {
// //             for(auto const& l: lease)
// //                 if (l.mode == access_mode::Invalid) return;
// //
// //             data_item_manager_impl<DataItem>::release(lease);
//         }

        template<typename DataItem>
		void destroy(const data_item_reference<DataItem>& ref)
        {
//             data_item_manager_impl<DataItem>::destroy(ref);
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
