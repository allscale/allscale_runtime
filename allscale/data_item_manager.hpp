#ifndef ALLSCALE_DATA_ITEM_MANAGER
#define ALLSCALE_DATA_ITEM_MANAGER

#include <allscale/data_item_reference.hpp>
#include <allscale/data_item_server.hpp>
#include <allscale/data_item_server_network.hpp>
#include <allscale/data_item_requirement.hpp>
#include <allscale/lease.hpp>
#include "allscale/utils/serializer.h"
#include <type_traits>

namespace allscale{
    struct data_item_manager{
        
        
        public:
       
        template <typename DataItemType, typename ... Args>
        static typename std::enable_if<sizeof...(Args) == 0,allscale::data_item_reference<DataItemType> >::type create()
        { 

            if(hpx::get_locality_id() == 0){
                static auto sn = allscale::data_item_manager::create_server_network<DataItemType>();
            } 
            using data_item_reference_type = allscale::data_item_reference<DataItemType>;
            hpx::id_type server_id = allscale::data_item_manager::get_server<DataItemType>();

            typedef typename allscale::server::data_item_server<DataItemType>::template create_action<> action_type;
            auto res = action_type()(server_id);
            return res;    
        }
        

            
        template <typename DataItemType, typename ... Args>
        static allscale::data_item_reference<DataItemType> create(const Args& ... args)
        {

            if(hpx::get_locality_id() == 0){
                static auto sn = allscale::data_item_manager::create_server_network<DataItemType>();
            } 
            using data_item_reference_type = allscale::data_item_reference<DataItemType>;
            auto archive = allscale::utils::serialize(args...);
            using buffer_type = std::vector<char>;
            buffer_type buffer;
            buffer = archive.getBuffer();
            hpx::id_type server_id = allscale::data_item_manager::get_server<DataItemType>();
            typedef typename  allscale::server::data_item_server<DataItemType>::template create_action<buffer_type> action_type;
            auto res = action_type()(server_id, buffer);
            return res;    
        }
        
        template<typename DataItemType>
		 static typename DataItemType::facade_type get(const allscale::data_item_reference<DataItemType>& ref) {
		 // static std::size_t get(const allscale::data_item_reference<DataItemType>& ref) {

            using facade_type = typename DataItemType::facade_type;
            hpx::id_type server_id = allscale::data_item_manager::get_server<DataItemType>();
            typedef typename allscale::server::data_item_server<DataItemType>::get_action action_type;
            std::size_t res  = action_type()(server_id,ref);
            
	        using data_item_fragment_type = typename DataItemType::fragment_type;
            data_item_fragment_type* frag_ptr;
            frag_ptr = (data_item_fragment_type*) res;
            // DataItemType::facade_type* ptr;
            // ptr = (DataItemType::facade_type*) raw_ptr_as_sizet;
            return frag_ptr->mask();
		}

        template<typename DataItemType>
		static allscale::lease<DataItemType> acquire(const allscale::data_item_requirement<DataItemType>& requirement) {
            hpx::id_type server_id = allscale::data_item_manager::get_server<DataItemType>();
            typedef typename  allscale::server::data_item_server<DataItemType>::template acquire_action<allscale::data_item_requirement<DataItemType>> action_type;
		    return action_type()(server_id, requirement);
            //return get_server<DataItemType>().acquire(requirement);
		}


/*
 		template<typename DataItemType>
		static allscale::lease<DataItemType>  acquire(const allscale::data_item_requirement<DataItemType>& requirement) {
            allscale::lease<DataItemType> lease;
            return lease;
		}
	
*/


        template<typename DataItemType>
		static void release(const allscale::lease<DataItemType>& lease) {
		    std::cout<<"Releasing DataItem"<<std::endl;
        }
        template<typename DataItemType>
		static void destroy(const data_item_reference<DataItemType>& ref) {
		    std::cout<<"Destroying DataItem"<< std::endl;
        }

        /*```
        template<typename DataItemType>
        static data_item_server<DataItemType>& getLocalDataItemServer(){
            typedef typename allscale::server::data_item_server<DataItemType> data_item_server_type;
            static allscale::data_item_server<DataItemType> server(
                            hpx::components::new_ < data_item_server_type > (hpx::find_here()));

            return server;
        }

        */

        template<typename DataItemType>
        static hpx::id_type get_server(){
           /* 
            auto data_item_server_name = allscale::data_item_server_name<DataItemType>::name();
            auto res =  hpx::find_from_basename(data_item_server_name, hpx::naming::get_locality_id_from_id(hpx::find_here()));
            static hpx::id_type local_server = res.get();
      */
                
            auto data_item_server_name = allscale::data_item_server_name<DataItemType>::name();
            auto res =  hpx::find_all_from_basename(data_item_server_name, hpx::find_all_localities().size());
            hpx::id_type result;
            for(auto& fut : res ){
                typedef typename allscale::server::data_item_server<DataItemType>::print_action action_type;
               result = fut.get();
               if( hpx::naming::get_locality_id_from_id(result) == hpx::naming::get_locality_id_from_id(hpx::find_here()))
               {
                   return result;
               }
            }
            
            return result;
            //return getLocalDataItemServer<DataItemType>();
        }

        template<typename DataItemType>
        static allscale::data_item_server_network<DataItemType>  create_server_network(){
            typedef typename allscale::server::data_item_server<DataItemType> data_item_server_type;
            //CREATE DATA ITEM SERVER INSTANCES ON LOCALITIES
            allscale::data_item_server_network<DataItemType> sn;
            std::vector < hpx::id_type > localities = hpx::find_all_localities();
            std::vector<allscale::data_item_server<DataItemType>> result;
            for (auto& loc : localities) {
                allscale::data_item_server<DataItemType> server(
                        hpx::components::new_ < data_item_server_type > (loc).get());
                result.push_back(server);
            }
           sn.servers = result;
           for( auto& server : result){
            server.set_network(sn);
           }
           return sn;
        }



       



    };
}//end namespace allscale
#endif
