#ifndef ALLSCALE_DATA_ITEM_SERVER
#define ALLSCALE_DATA_ITEM_SERVER

#include <allscale/data_item_server_network.hpp>
#include <allscale/locality.h>
#include <allscale/data_item_requirement.hpp>
#include <allscale/data_item_reference.hpp>
#include <allscale/lease.hpp>
#include <allscale/location_info.hpp>
#include <map>


#include <memory>
#include "allscale/api/core/data.h"
#include "allscale/utils/assert.h"
#include "allscale/utils/serializer.h"
#include <hpx/include/components.hpp>
#include <hpx/include/actions.hpp>
#include <hpx/util/register_locks.hpp>

#include <allscale/util.hpp>


/*
namespace{
    template<typename DataItemType>
    struct data_item_server;
}*/



namespace allscale{
    template<typename DataItemType>
    struct data_item_server_name;

namespace server {

template<typename DataItemType>
class data_item_server: public hpx::components::locking_hook<
		hpx::components::component_base<data_item_server<DataItemType> > > {

public:
	using data_item_type = DataItemType;
	using data_item_shared_data_type = typename DataItemType::shared_data_type;
	using data_item_fragment_type = typename DataItemType::fragment_type;
	using data_item_region_type = typename DataItemType::region_type;
	using data_item_reference_client_type = typename allscale::data_item_reference<DataItemType>;
    using network_type = typename allscale::data_item_server_network<DataItemType>;
    // using network_type = data_item_server_network<DataItemType>;
    //using network_type = int;
	struct fragment_info {
		// the managed fragment
		data_item_fragment_type fragment;

		// the regions currently locked through read leases
		data_item_region_type readLocked;

		// the list of all granted read leases
		std::vector<data_item_region_type> readLeases;

		// the regions currently locked through write access
		data_item_region_type writeLocked;

		fragment_info(const data_item_shared_data_type& shared) :
				fragment(shared), readLocked(), writeLocked() {
		}

		fragment_info(const fragment_info&) = delete;
		fragment_info(fragment_info&&) = default;

		void addReadLease(const data_item_region_type& region) {

			// add to lease set
			readLeases.push_back(region);

			// merge into read locked region
			readLocked = data_item_region_type::merge(readLocked, region);
		}

		void removeReadLease(const data_item_region_type& region) {

			// remove lease from lease set
			auto pos = std::find(readLeases.begin(), readLeases.end(), region);
			assert_true(pos != readLeases.end())
					<< "Attempting to delete non-existing read-lease: "
					<< region << " in " << readLeases;
			readLeases.erase(pos);

			// update readLocked status
			data_item_region_type locked = data_item_region_type();
			for (const auto& cur : readLeases) {
				locked = data_item_region_type::merge(locked, cur);
			}

			// exchange read locked region
			std::swap(readLocked, locked);
		}

	};

	std::map<data_item_reference_client_type, fragment_info> store;
    network_type network_;
    std::map<std::size_t,hpx::id_type> servers_;
    public:

	data_item_server() {
    }

	template<typename ... T>
	data_item_reference_client_type create(const T& ... args) {
        
        typedef typename allscale::data_item_reference<DataItemType> data_item_reference_type;
        data_item_reference_client_type ret_val( hpx::components::new_ < data_item_reference_type> (hpx::find_here()));
        allscale::utils::Archive received(args...);
        auto p2 = allscale::utils::deserialize<data_item_shared_data_type>(received);
        data_item_shared_data_type shared(p2);
        auto dataItemID =  ret_val.get_id();
        store.emplace(ret_val, std::move(fragment_info(shared)));
        
        for(auto& server : network_.servers){
            std::cout<<"blub"<<std::endl;
            server.register_data_item_ref(dataItemID,args...);
        }
        /*
        auto s = store.size();
        std::cout<< "Store has size " << s << hpx::naming::get_locality_id_from_id(dataItemID)<<std::endl; 
        auto mask = store[dataItemID].fragment.mask();
        // create shared data
        // prepare shared data to be distributed
        auto sharedStateArchive = allscale::utils::serialize(shared);
        // inform other servers
        network.broadcast(
        [&sharedStateArchive,dataItemID](data_item_server& server) {
        // retrieve shared data
        auto sharedData = allscale::utils::deserialize<data_item_shared_data_type>(sharedStateArchive);
        // create new local fragment
        server.store.emplaYce(dataItemID, std::move(fragment_info(sharedData)));
        });
        */
        return ret_val;
    }

	template<typename ... T>
	struct create_action: hpx::actions::make_action< data_item_reference_client_type (data_item_server::*)(const T& ...),
			&data_item_server::template create<T...>, create_action<T...> > {
	};
   
    template<typename ... T>
	void register_data_item_ref(const T& ... args) {
        auto tup   = hpx::util::make_tuple(args...);
        auto first = hpx::util::get<0>(tup);
        auto second = hpx::util::get<1>(tup);
        std::cout<<"register dataitem ref called for "  << first << std::endl;
    //    std::cout<<"register dataitem ref called on " << this->get_id() << " " << second.get_id() << std::endl;
        /*
        typedef typename manual_tests::example::server::DataItemReference<DataItemType> data_item_reference_type;
        allscale::utils::Archive received(args...);
        auto p2 = allscale::utils::deserialize<data_item_shared_data_type>(received);
        data_item_shared_data_type shared(p2);
        auto dataItemID =  ret_val.get_id();
        store.emplace(ret_val, std::move(fragment_info(shared)));
        */
    } 


	template<typename ... T>
	struct register_data_item_ref_action : hpx::actions::make_action< void (data_item_server::*)(const T& ...),
			&data_item_server::template register_data_item_ref<T...>, register_data_item_ref_action<T...> > {
	};
   

    typename DataItemType::facade_type  get(const data_item_reference_client_type& ref) 
    {
        std::cout<<"get method called for id: " << ref.get_id()<<std::endl;
        
        
        auto pos = store.find(ref);
        assert_true(pos != store.end()) << "Requested invalid data item id: " << ref.get_id();
        auto maski = pos->second.fragment.mask();
        //std::cout<<maski[10]<<std::endl; 
        return maski;
    
    }


    void print()
    {
        std::cout<<"print method for server: " << this->get_id() << " " << "store.size():" << store.size()<<std::endl;
        /*
        for(auto& el : store){
            std::cout<<el.first.get_id()<<std::endl; 
        }*/
    }
    
    void set_network(const network_type& network){

        network_ = network;
    }

    void set_servers(){
        // servers_[hpx::naming::get_locality_id_from_id(this->get_id())] = this->get_id();
        // std::cout << servers_[hpx::naming::get_locality_id_from_id(this->get_id())] << std::endl;
        auto data_item_server_name = allscale::data_item_server_name<DataItemType>::name();
        auto res =  hpx::find_all_from_basename(data_item_server_name, hpx::find_all_localities().size());
        for(auto& fut : res ){
            auto id = fut.get();
            //servers_[hpx::naming::get_locality_id_from_id(fut.get())] = fut.get();    
        }
    }
    void print_network(){
        for(auto& server : network_.servers){
            std::cout<<server.get_id()<<std::endl;
        }
    }
    
    
    
    
    HPX_DEFINE_COMPONENT_ACTION(data_item_server, set_network);
    HPX_DEFINE_COMPONENT_ACTION(data_item_server, set_servers);
    HPX_DEFINE_COMPONENT_ACTION(data_item_server, print_network);
    HPX_DEFINE_COMPONENT_ACTION(data_item_server, print);
};

}}
#define REGISTER_DATAITEMSERVER_DECLARATION(type)                       \
    HPX_REGISTER_ACTION_DECLARATION(                                          \
    	allscale::server::data_item_server<type>::print_action,           \
        BOOST_PP_CAT(__data_item_server_print_action_, type));            \
    HPX_REGISTER_ACTION_DECLARATION(                                          \
    	allscale::server::data_item_server<type>::set_network_action,           \
        BOOST_PP_CAT(__data_item_server_set_network_action_, type));            \
    HPX_REGISTER_ACTION_DECLARATION(                                          \
    	allscale::server::data_item_server<type>::print_network_action,           \
        BOOST_PP_CAT(__data_item_server_print_network_action_, type));            \
    HPX_REGISTER_ACTION_DECLARATION(                                          \
    	allscale::server::data_item_server<type>::set_servers_action,           \
        BOOST_PP_CAT(__data_item_server_set_servers_action_, type));            \
    namespace allscale{                                                           \
        template<>                                                                \
        struct data_item_server_name<type>                                               \
        {                                                                         \
            static const char* name(){return BOOST_PP_STRINGIZE(BOOST_PP_CAT(allscale/data_item_manager/data_item_server_,type));}           \
        };                                                                        \
    }                                                                             \

#define REGISTER_DATAITEMSERVER(type)                                   \
    HPX_REGISTER_ACTION(            \
        allscale::server::data_item_server<type>::print_action,           \
        BOOST_PP_CAT(__data_item_server_print_action_, type));            \
    HPX_REGISTER_ACTION(            \
        allscale::server::data_item_server<type>::set_network_action,           \
        BOOST_PP_CAT(__data_item_server_set_network_action_, type));            \
    HPX_REGISTER_ACTION(            \
        allscale::server::data_item_server<type>::print_network_action,           \
        BOOST_PP_CAT(__data_item_server_print_network_action_, type));            \
    HPX_REGISTER_ACTION(            \
        allscale::server::data_item_server<type>::set_servers_action,           \
        BOOST_PP_CAT(__data_item_set_servers_network_action_, type));            \
    typedef ::hpx::components::component<                                     \
    	allscale::server::data_item_server<type>                         \
    > BOOST_PP_CAT(__data_item_server_, type);                            \
    HPX_REGISTER_COMPONENT(BOOST_PP_CAT(__data_item_server_, type))       \


namespace allscale {
   template<typename DataItemType>
class data_item_server: public hpx::components::client_base<
		data_item_server<DataItemType>,
		allscale::server::data_item_server<DataItemType> > {
	typedef hpx::components::client_base<data_item_server<DataItemType>,
			allscale::server::data_item_server<DataItemType> > base_type;

	typedef typename allscale::server::data_item_server<
			DataItemType>::data_item_type data_item_type;
    typedef typename allscale::data_item_reference<DataItemType> data_item_reference_client_type;

    //using network_type = int;
    using network_type = typename allscale::data_item_server_network<DataItemType>;

public:

	data_item_server() {
	}

	data_item_server(hpx::future<hpx::id_type> && gid) :
			base_type(std::move(gid)) {
        //        std::cout<<"called on loc" << this->get_id() << std::endl;
        auto data_item_server_name = allscale::data_item_server_name<DataItemType>::name();
        //data_item_server_name += hpx::naming::get_locality_id_from_id(this->get_id());
        // std::cout<< hpx::register_with_basename(data_item_server_name,
        // this->get_id(),hpx::naming::get_locality_id_from_id(this->get_id())).get()<<std::endl;

    }

    data_item_server(hpx::id_type && gid) :
			base_type(std::move(gid)) {
        //        std::cout<<"called on loc" << this->get_id() << std::endl;
        auto data_item_server_name = allscale::data_item_server_name<DataItemType>::name();
        //data_item_server_name += hpx::naming::get_locality_id_from_id(this->get_id());
        hpx::register_with_basename(data_item_server_name,this->get_id(),hpx::naming::get_locality_id_from_id(this->get_id())).get();

    }
	template<typename ... T>
	data_item_reference_client_type create(const T& ... args) {
		HPX_ASSERT(this->get_id());

		typedef typename  allscale::server::data_item_server<DataItemType>::template create_action<T...> action_type;
		return action_type()(this->get_id(), args...);
	}

    template<typename ...T>
    void register_data_item_ref(const T& ... args){
        HPX_ASSERT(this->get_id());
        typedef typename allscale::server::data_item_server<DataItemType>::template register_data_item_ref_action<T...> action_type;
        action_type()(this->get_id(),args...);
    }

    void print() {
		HPX_ASSERT(this->get_id());
		typedef typename allscale::server::data_item_server<DataItemType>::print_action action_type;
		action_type()(this->get_id());
	}

    void print_network() {
		HPX_ASSERT(this->get_id());
		typedef typename allscale::server::data_item_server<DataItemType>::print_network_action action_type;
		action_type()(this->get_id());
	}
    void set_network(const network_type& network){
        HPX_ASSERT(this->get_id());
        typedef typename allscale::server::data_item_server<DataItemType>::set_network_action action_type;
        action_type()(this->get_id(),network);
    }

    void set_servers(){
        HPX_ASSERT(this->get_id());
        typedef typename allscale::server::data_item_server<DataItemType>::set_servers_action action_type;
        action_type()(this->get_id());
    }
    typename DataItemType::facade_type get(const data_item_reference_client_type& ref){
        using parent_type = typename allscale::server::data_item_server<DataItemType>;
        hpx::future<std::shared_ptr<parent_type> > f = hpx::get_ptr<parent_type>(this->get_id());
        std::shared_ptr<parent_type> ptr = f.get();
        auto result = (ptr.get())->get(ref);
        return result;   
        /*HPX_ASSERT(this->get_id());
        typedef typename allscale::server::data_item_server<DataItemType>::get_action action_type;
        action_type()(this->get_id(),ref);
        */
    }

};

}
























#endif
