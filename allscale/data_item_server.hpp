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
#include <hpx/include/serialization.hpp>
#include <allscale/util.hpp>


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
    using lease_type = typename allscale::lease<DataItemType>;
    using location_info_type = typename allscale::location_info<DataItemType>;
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

	std::map<hpx::id_type, fragment_info> store;
    network_type network_;
    std::map<std::size_t,hpx::id_type> servers_;
    public:

	data_item_server() {
    }

    
    data_item_reference_client_type  create_impl()
    {
        //typedef typename allscale::data_item_reference<DataItemType> data_item_reference_type;
        
        data_item_reference_client_type ret_val;
        //data_item_reference_client_type ret_val( hpx::components::new_ < data_item_reference_type> (hpx::find_here()));
        data_item_shared_data_type shared;
        auto dataItemID =  ret_val.id_;
        store.emplace(dataItemID, std::move(fragment_info(shared)));
        
        for(auto& server : network_.servers){
            if(server.get_id()  != this->get_id()){
                server.register_data_item_ref(dataItemID);
            }
        }
        return ret_val;
    }


    
	template <typename T, typename ... Ts>
    data_item_reference_client_type create_impl(const T& arg, const Ts& ... args) 
    {
        typedef typename allscale::data_item_reference<DataItemType> data_item_reference_type;
        
        data_item_reference_client_type ret_val;
        //data_item_reference_client_type ret_val( hpx::components::new_ < data_item_reference_type> (hpx::find_here()));
        /*
        allscale::utils::Archive received(arg, args...);
        auto p2 = allscale::utils::deserialize<data_item_shared_data_type>(received);
        data_item_shared_data_type shared(p2);
        
        */
        using size_type = typename data_item_shared_data_type::size_type;
        data_item_shared_data_type first= arg; 
        //hpx::util::tuple<size_type> tup = hpx::util::make_tuple(arg);
      
     // auto tup   = hpx::util::make_tuple(arg,args...);
        //size_type first = hpx::util::get<0>(tup);

//        auto second = hpx::util::get<1>(tup);
     
        data_item_shared_data_type shared(first);
        
        
        auto dataItemID =  ret_val.id_;
        store.emplace(dataItemID, std::move(fragment_info(shared)));
        
        for(auto& server : network_.servers){
            if(server.get_id()  != this->get_id()){
//                server.register_data_item_ref(dataItemID,arg, args...);
            }
        }
        return ret_val;
    }

	template <typename ... T>
    data_item_reference_client_type create(const T& ... args) 
    {
        return create_impl(args...);
    }

    /*
    template <typename T>
    class foo<T, typename std::enable_if<std::is_integral<T>::value>::type>
    { };
    
    
    static typename std::enable_if<sizeof...(Args) == 0,allscale::data_item_reference<DataItemType> >::type create()
*/
     

	template<typename ... T>
	struct create_action: hpx::actions::make_action< data_item_reference_client_type (data_item_server::*)(const T& ...),
			&data_item_server::template create<T...>, create_action<T...> > {
	};

	void register_data_item_ref_impl(hpx::id_type id) {
        data_item_shared_data_type shared;
        auto dataItemID =  id;
        store.emplace(dataItemID, std::move(fragment_info(shared)));
    }

    template<typename T, typename T2, typename ... Ts>
	void register_data_item_ref_impl(const T& arg,const T2& arg2, const Ts& ... args) {
       /* auto tup   = hpx::util::make_tuple(arg,arg2,args...);
        auto first = hpx::util::get<0>(tup);
        auto second = hpx::util::get<1>(tup);
        allscale::utils::Archive received(second);
        auto p2 = allscale::utils::deserialize<data_item_shared_data_type>(received);
        */
        /*
        data_item_shared_data_type shared(p2);
        auto dataItemID =  first;
        store.emplace(dataItemID, std::move(fragment_info(shared)));
        */
    }

    template<typename ... T>
	void register_data_item_ref(const T& ... args) {
        register_data_item_ref_impl(args...);
    } 



    template<typename ... T>
	struct register_data_item_ref_action : hpx::actions::make_action< void (data_item_server::*)(const T& ...),
			&data_item_server::template register_data_item_ref<T...>, register_data_item_ref_action<T...> > {
	};

    template<typename T>
    lease_type acquire(const T& req){
		//auto locationInfo = locate(req.ref,req.region);
				// collect data on data distribution
				//auto locationInfo = locate(request.ref,request.region);

				// get local fragment info
	            auto& info = get_info(req.ref);
				// allocate storage for requested data on local fragment
				info.fragment.resize(merge(info.fragment.getCoveredRegion(), req.region));
                /*
				// transfer data using a transfer plan
				auto success = execute(buildPlan(locationInfo,myLocality,request));

				// make sure the transfer was ok
				assert_true(success);
    */
        lease_type l(req);
        return l;
    }
	template<typename T>
	struct acquire_action : hpx::actions::make_action< lease_type (data_item_server::*)(const T&),
			&data_item_server::template acquire<T>, acquire_action<T> > {
	};



    location_info_type locate(const data_item_reference_client_type& ref, const data_item_region_type& region) {

        location_info_type res;
        
        for(auto& server : network_.servers){
            std::cout<< server.get_id() << std::endl;
             typedef typename allscale::server::data_item_server<DataItemType>::get_info_action action_type;
		        action_type()(server.get_id(),ref);
            /*if(server.get_id() != this->get_id()){
                typedef typename allscale::server::data_item_server<DataItemType>::get_info_action action_type;
		        action_type()(server.get_id(),ref);
            }*/
	      
        }
        /*

        network.broadcast([&](const DataItemServer& server) {

            if (!server.alive) return;

            auto& info = server.getInfo(ref);

            auto part = data_item_region_type::intersect(region,info.fragment.getCoveredRegion());

            if (part.empty()) return;

            res.addPart(part,server.myLocality);

        });
        */
        return res;
    }

    //std::vector<char> get_info(const data_item_reference_client_type& ref) {
    fragment_info& get_info(const data_item_reference_client_type& ref ) {
        auto pos = store.find(ref.id_);
        assert_true(pos != store.end()) << "Requested invalid data item id: " << ref.id;
        return pos->second;
          
        //std::cout<<"get_info_action called on loc: " << hpx::find_here() << std::endl;	
    }

    std::size_t  get(const data_item_reference_client_type& ref) 
    {
        //std::cout<<"get method called for id: " << ref.id_<<std::endl;
        auto pos = store.find(ref.id_);
        assert_true(pos != store.end()) << "Requested invalid data item id: " << ref.id_;
        //auto fragment  = pos->second.fragment.mask();
        auto *frag_ptr  = &(pos->second.fragment);
        std::size_t k;
        k = (std::size_t) frag_ptr;
        //std::shared_ptr<decltype(pos->second.fragment.mask())> ptr(mask);


        //std::cout<<maski[10]<<std::endl; 
        //return maski;
        return k;
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
    
    
    
    
    HPX_DEFINE_COMPONENT_ACTION(data_item_server, get);
    HPX_DEFINE_COMPONENT_ACTION(data_item_server, set_network);
    HPX_DEFINE_COMPONENT_ACTION(data_item_server, set_servers);
    HPX_DEFINE_COMPONENT_ACTION(data_item_server, print_network);
    HPX_DEFINE_COMPONENT_ACTION(data_item_server, print);
//    HPX_DEFINE_COMPONENT_ACTION(data_item_server, get_info);
//    HPX_DEFINE_COMPONENT_ACTION(data_item_server, create_zero);
//    HPX_DEFINE_COMPONENT_ACTION(data_item_server, register_data_item_ref_zero);
};

}}

/*
    HPX_REGISTER_ACTION_DECLARATION(                                          \
    	allscale::server::data_item_server<type>::create_zero_action,           \
        BOOST_PP_CAT(__data_item_server_create_zero_action_, type));            \
    HPX_REGISTER_ACTION_DECLARATION(                                          \
    	allscale::server::data_item_server<type>::register_data_item_ref_zero_action,           \
        BOOST_PP_CAT(__data_item_server_register_data_item_ref_zero_action_, type));            \
 
*/


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
    HPX_REGISTER_ACTION_DECLARATION(                                          \
    	allscale::server::data_item_server<type>::get_action,           \
        BOOST_PP_CAT(__data_item_server_get_action_, type));            \
    namespace allscale{                                                           \
        template<>                                                                \
        struct data_item_server_name<type>                                               \
        {                                                                         \
            static const char* name(){return BOOST_PP_STRINGIZE(BOOST_PP_CAT(allscale/data_item_manager/data_item_server_,type));}           \
        };                                                                        \
    }                                                                             \





/*
 *
 * HPX_REGISTER_ACTION(            \
        allscale::server::data_item_server<type>::create_zero_action,           \
        BOOST_PP_CAT(__data_item_create_zero_action_, type));            \
    HPX_REGISTER_ACTION(            \
        allscale::server::data_item_server<type>::register_data_item_ref_zero_action,           \
        BOOST_PP_CAT(__data_item_register_data_item_ref_zero_action_, type));            \
 */ 
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
        BOOST_PP_CAT(__data_item_set_servers_action_, type));            \
    HPX_REGISTER_ACTION(            \
        allscale::server::data_item_server<type>::get_action,           \
        BOOST_PP_CAT(__data_item_get_action_, type));            \
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
        /*
        using parent_type = typename allscale::server::data_item_server<DataItemType>;
        hpx::future<std::shared_ptr<parent_type> > f = hpx::get_ptr<parent_type>(this->get_id());
        std::shared_ptr<parent_type> ptr = f.get();
        auto result = (ptr.get())->get(ref);
        return result;   */
        HPX_ASSERT(this->get_id());
        typedef typename allscale::server::data_item_server<DataItemType>::get_action action_type;
        action_type()(this->get_id(),ref);
        
    }

};

}
























#endif
