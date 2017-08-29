#ifndef ALLSCALE_DATA_ITEM_SERVER_NETWORK
#define ALLSCALE_DATA_ITEM_SERVER_NETWORK

#include <allscale/locality.h>
namespace allscale{

    template<typename DataItemType>
    struct data_item_server;

    template<typename DataItemType>
    struct data_item_server_network {
        
        
	    using locality_type = simulator::locality_type;
        using server_type = data_item_server<DataItemType>;
        std::vector<server_type> servers;


		public:

            data_item_server_network() {
                for(locality_type i=0; i<simulator::getNumLocations(); ++i) {
                    servers.emplace_back(i, *this);
                }
            }


			static server_type& getLocalDataItemServer() {
				static data_item_server_network instance = data_item_server_network();
				return instance.servers[simulator::getLocality()];
			}

			template<typename Op>
			void broadcast(const Op& op) {
				for(auto& server : servers) {
					op(server);
				}
			}

			// a function to address a remote server directly
			template<typename Op>
			void call(std::size_t trg, const Op& op) {
				op(servers[trg]);
			}
    };

} //end namespace allscale




#endif
