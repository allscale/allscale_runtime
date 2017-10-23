#ifndef ALLSCALE_DATA_ITEM_SERVER_NETWORK
#define ALLSCALE_DATA_ITEM_SERVER_NETWORK


// #include <hpx/runtime/serialization/serialize.hpp>
// #include <hpx/runtime/serialization/vector.hpp>
// #include <hpx/runtime/serialization/input_archive.hpp>
// #include <hpx/runtime/serialization/output_archive.hpp>


// #include <allscale/data_item_server.hpp>

#include <hpx/runtime/basename_registration.hpp>

namespace allscale { namespace components {

    struct data_item_network {
    public:
        data_item_network(const char* base)
          : base_(base)
        {}

        template <typename Callback>
        void apply(std::size_t rank, Callback&&cb)
        {
            hpx::id_type id;
            {
                std::unique_lock<mutex_type> l(mtx_);
                auto it = servers_.find(rank);
                // If we couldn't find it in the map, we resolve the name.
                // This is happening asynchronously. Once the name was resolved,
                // the callback is apply and the id is being put in the network map.
                //
                // If we are able to locate the rank in our map, we just go ahead and
                // apply the callback.
                if (it == servers_.end())
                {
                    l.unlock();
                    hpx::find_from_basename(base_, rank).then(
                        hpx::util::bind(
                            hpx::util::one_shot(
                            [this, rank](hpx::future<hpx::id_type> fut,
                                typename hpx::util::decay<Callback>::type cb)
                            {
                                hpx::id_type id = fut.get();
                                cb(id);
                                {
                                    std::unique_lock<mutex_type> l(mtx_);
                                    // We need to search again in case of concurrent
                                    // lookups.
                                    auto it = servers_.find(rank);
                                    if (it == servers_.end())
                                    {
                                        servers_.insert(std::make_pair(rank, id));
                                    }
                                }
                            }), hpx::util::placeholders::_1, std::forward<Callback>(cb)));
                    return;
                }
                id = it->second;
            }

            HPX_ASSERT(id);

            cb(id);
        }

    private:
        const char* base_;

        typedef hpx::lcos::local::spinlock mutex_type;
        mutex_type mtx_;
        std::unordered_map<std::size_t, hpx::id_type> servers_;
    };
}}

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
