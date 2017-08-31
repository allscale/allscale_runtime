#ifndef ALLSCALE_DATA_ITEM_SERVER_NETWORK
#define ALLSCALE_DATA_ITEM_SERVER_NETWORK



#include <allscale/locality.h>
#include <allscale/data_item_server.hpp>
namespace allscale{


    template<typename DataItemType>
    struct data_item_server_network {
        
        
	    using locality_type = simulator::locality_type;
        /*
        using server_type = typename allscale::server::data_item_server<DataItemType>;
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

            */
    };

    
}

/*
namespace allscale{

    template<typename DataItemType>
    struct data_item_server;
    
    namespace server{
////////////////////////////////////////////////////////////////////////////////
// SERVERNETWORK  SERVER CLASS and CLIENT CLASS  AND MACRO DEFINITIONS
template<typename DataItemType>
class data_item_server_network: public hpx::components::locking_hook<
        hpx::components::component_base<data_item_server_network<DataItemType> > > {

    public:

    using data_item_type = DataItemType;
    using server_type = allscale::data_item_server<DataItemType>;

    std::vector<server_type> servers_;

    data_item_server_network(){}
    
    void set_servers(const std::vector<server_type>& existing_servers){
        std::cout<<"set_servers called on loc: " << hpx::get_locality_id()<<std::endl;
        for(auto& el : existing_servers){
            servers_.push_back(el);
        }
    }

    std::vector<server_type> get_servers(){
        std::cout<<"get_servers called on loc: " << hpx::get_locality_id()<<std::endl;
        return servers_;
    }

    void print_servers(){
        std::cout<<"print_servers action called \n";
        for(auto& el: servers_)
        {
            el.print();
            //std::cout<<el.get_id()<< " " << hpx::naming::get_locality_id_from_id(el.get_id())<<std::endl;
            
        }
    }

    HPX_DEFINE_COMPONENT_ACTION(data_item_server_network, print_servers);
    HPX_DEFINE_COMPONENT_ACTION(data_item_server_network, set_servers);
    HPX_DEFINE_COMPONENT_ACTION(data_item_server_network, get_servers);
};

}}

#define REGISTER_DATAITEMSERVERNETWORK_DECLARATION(type)                       \
    HPX_REGISTER_ACTION_DECLARATION(                                          \
        allscale::server::detail::data_item_server_network<type>::print_servers_action,           \
        BOOST_PP_CAT(__data_item_server_network_print_servers_action_, type));            \
    HPX_REGISTER_ACTION_DECLARATION(                                          \
        allscale::server::data_item_server_network<type>::set_servers_action,           \
        BOOST_PP_CAT(__data_item_server_networks_set_servers_action_, type));            \
    HPX_REGISTER_ACTION_DECLARATION(                                          \
        allscale::server::data_item_server_network<type>::get_servers_action,           \
        BOOST_PP_CAT(__data_item_server_networks_get_servers_action_, type));            \


#define REGISTER_DATAITEMSERVERNETWORK(type)                                   \
    HPX_REGISTER_ACTION(                                                      \
        allscale::server::data_item_server_network<type>::print_servers_action,           \
        BOOST_PP_CAT(__data_item_server_network_print_servers_action_, type));            \
    HPX_REGISTER_ACTION(                                                      \
        allscale::server::data_item_server_network<type>::set_servers_action,           \
        BOOST_PP_CAT(__data_item_server_network_set_servers_action_, type));            \
    HPX_REGISTER_ACTION(                                                      \
        allscale::server::data_item_server_network<type>::get_servers_action,           \
        BOOST_PP_CAT(__data_item_server_network_get_servers_action_, type));            \
    typedef ::hpx::components::component<                                     \
        allscale::server::data_item_server_network<type>                         \
    > BOOST_PP_CAT(__data_item_server_network_, type);                            \
    HPX_REGISTER_COMPONENT(BOOST_PP_CAT(__data_item_server_network_, type))       \

namespace allscale {

template<typename DataItemType>
class data_item_server_network: public hpx::components::client_base<
        data_item_server_network<DataItemType>,
        server::data_item_server_network<DataItemType> > {
    
    typedef hpx::components::client_base<data_item_server_network<DataItemType>,
            server::data_item_server_network<DataItemType> > base_type;

    typedef typename server::data_item_server_network<
            DataItemType>::data_item_type data_item_type;

    using server_type = data_item_server<DataItemType>;
    
    public:

    data_item_server_network() {
    }

    data_item_server_network(hpx::future<hpx::id_type> && gid) :
            base_type(std::move(gid)) {
    }

    void print_servers() {
        HPX_ASSERT(this->get_id());
        typedef typename server::data_item_server_network<DataItemType>::print_servers_action action_type;
        action_type()(this->get_id());
    }
    void set_servers(const std::vector<server_type>& servers) {
        HPX_ASSERT(this->get_id());
        typedef typename server::data_item_server_network<DataItemType>::set_servers_action action_type;
        action_type()(this->get_id(),servers);
    }
    std::vector<server_type> get_servers(){
        HPX_ASSERT(this->get_id());
        typedef typename server::data_item_server_network<DataItemType>::get_servers_action action_type;
        return action_type()(this->get_id());
    }
};

}


*/






#endif
