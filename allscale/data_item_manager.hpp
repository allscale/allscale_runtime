#ifndef ALLSCALE_DATA_ITEM_MANAGER
#define ALLSCALE_DATA_ITEM_MANAGER

#include <allscale/data_item_manager_impl.hpp>
#include <allscale/data_item_reference.hpp>
#include <allscale/data_item_server.hpp>
#include <allscale/data_item_server_network.hpp>
#include <allscale/data_item_requirement.hpp>
#include <allscale/lease.hpp>
#include "allscale/utils/serializer.h"

#include <hpx/include/components.hpp>
#include <hpx/util/detail/yield_k.hpp>

#include <type_traits>

namespace allscale{

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
		static void release(const allscale::lease<DataItemType>& lease) {
            return data_item_manager_impl<DataItemType>::release(lease);
        }

        template<typename DataItemType>
		static void destroy(const data_item_reference<DataItemType>& ref) {
            data_item_manager_impl<DataItemType>::destroy(ref);
        }

//         template<typename DataItemType>
//         static hpx::id_type get_server()
//         {
//             auto data_item_server_name = allscale::data_item_server_name<DataItemType>::name();
//             auto res =  hpx::find_all_from_basename(data_item_server_name, hpx::find_all_localities().size());
//             hpx::id_type result;
//             for(auto& fut : res )
//             {
//                 typedef typename allscale::server::data_item_server<DataItemType>::print_action action_type;
//                 result = fut.get();
//                 if( hpx::naming::get_locality_id_from_id(result) == hpx::naming::get_locality_id_from_id(hpx::find_here()))
//                 {
//                     return result;
//                 }
//             }
//
//             return result;
//         }
//
//         template<typename DataItemType>
//         static allscale::data_item_server_network<DataItemType>
//         create_server_network()
//         {
//             typedef typename allscale::server::data_item_server<DataItemType> data_item_server_type;
//             //CREATE DATA ITEM SERVER INSTANCES ON LOCALITIES
//             allscale::data_item_server_network<DataItemType> sn;
//             std::vector < hpx::id_type > localities = hpx::find_all_localities();
//             std::vector<allscale::data_item_server<DataItemType>> result;
//             for (auto& loc : localities)
//             {
//                 allscale::data_item_server<DataItemType> server(
//                     hpx::components::new_ < data_item_server_type > (loc).get());
//                 result.push_back(server);
//             }
//             sn.servers = result;
//             for( auto& server : result)
//             {
//                 server.set_network(sn);
//             }
//             return sn;
//         }
    };
}//end namespace allscale
#endif
