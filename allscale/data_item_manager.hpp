#ifndef ALLSCALE_DATA_ITEM_MANAGER
#define ALLSCALE_DATA_ITEM_MANAGER

#include <allscale/data_item_reference.hpp>
#include <allscale/data_item_server.hpp>
#include <allscale/data_item_server_network.hpp>
#include <allscale/data_item_requirement.hpp>
#include <allscale/lease.hpp>
namespace allscale{
    struct data_item_manager{
        public:
        template <typename DataItemType, typename ... Args>
        static data_item_reference<DataItemType> create(const Args& ... args){
            return getServer<DataItemType>().create(args...);
        }
 		template<typename DataItemType>
		static lease<DataItemType> acquire(const data_item_requirement<DataItemType>& requirement) {
			return getServer<DataItemType>().acquire(requirement);
		}

      
        private:
        template<typename DataItemType>
        static data_item_server<DataItemType>& getServer(){
            return data_item_server_network<DataItemType>::getLocalDataItemServer();
        }

    };
}//end namespace allscale
#endif
