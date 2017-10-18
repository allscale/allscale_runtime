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
class data_item_server
  : public hpx::components::component_base<data_item_server<DataItemType> > {

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
            HPX_ASSERT(pos != readLeases.end());
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

    typedef hpx::lcos::local::spinlock mutex_type;

    mutable mutex_type mtx_;
	std::map<hpx::id_type, fragment_info> store;
    network_type network_;
    std::map<std::size_t,hpx::id_type> servers_;
    public:

	data_item_server()
    {
    }

    lease_type acquire(const allscale::data_item_requirement<DataItemType>& req)
    {
        std::unique_lock<mutex_type> l(mtx_);
        // Find our requested Data Item.
        auto store_it = store.find(req.ref.id());
        if (store_it == store.end())
        {
            // If it is not there already, we "first touch it" with the data stored
            // in the data item reference.
            auto store_pair = store.insert(
                std::make_pair(req.ref.id(), fragment_info(req.ref.shared_data())));
            HPX_ASSERT(store_pair.second);
            store_it = store_pair.first;
        }
//         auto locationInfo = locate(req.ref,req.region);
//         // collect data on data distribution
//         auto locationInfo = locate(request.ref,request.region);

        // get local fragment info
        auto& info = store_it->second;
        // allocate storage for requested data on local fragment
        info.fragment.resize(merge(info.fragment.getCoveredRegion(), req.region));

//         // transfer data using a transfer plan
//         auto success = execute(buildPlan(locationInfo,myLocality,request));
//
//         // make sure the transfer was ok
//         assert_true(success);

        switch (req.mode)
        {
            case access_mode::ReadOnly:
                {
                    info.addReadLease(req.region);
                    break;
                }
            case access_mode::ReadWrite:
                {
                    HPX_ASSERT(!allscale::api::core::isSubRegion(req.region, info.writeLocked));
                    // FIXME: add debug check that all other fragments don't have
                    // any kinds of locks on this fragment...
                    info.writeLocked = data_item_region_type::merge(
                        info.writeLocked, req.region);
                    break;
                }
            default:
                HPX_ASSERT(false);
        }

        return lease_type(req);
    }

    void release(const lease<DataItemType>& lease) {
        // get information about fragment
        std::unique_lock<mutex_type> l(mtx_);
        // Find our requested Data Item.
        auto store_it = store.find(lease.ref.id());
        HPX_ASSERT(store_it != store.end());
        // get local fragment info
        auto& info = store_it->second;

        // update lock states
        switch(lease.mode) {
            case access_mode::ReadOnly:
            {
                // check that this region is actually protected
                HPX_ASSERT(allscale::api::core::isSubRegion(info.readLocked,lease.region));

                // remove read lease
                info.removeReadLease(lease.region);

                break;
            }
            case access_mode::ReadWrite: {
                // check that this region is actually protected
                HPX_ASSERT(allscale::api::core::isSubRegion(lease.region, info.writeLocked));

                // lock data for write
                info.writeLocked = data_item_region_type::difference(info.writeLocked, lease.region);

                break;
            }
            default:
                HPX_ASSERT(false);
        }
    }

    void destroy(const data_item_reference<DataItemType>& ref) {
        std::unique_lock<mutex_type> l(mtx_);
        // check that the reference is valid
        auto store_it = store.find(ref.id());
        HPX_ASSERT(store_it != store.end());

        // FIXME: replace with broadcast...
        store.erase(ref.id());
        // remove data from all nodes
//         network.broadcast([&ref](data_item_server& server){
//             if (!server.alive) return;
//
//             // make sure now access on fragment is still granted
//             assert_true(server.getInfo(ref).readLocked.empty())
//                 << "Still read access on location " << server.myLocality << " for region " << server.getInfo(ref).readLocked;
//             assert_true(server.getInfo(ref).writeLocked.empty())
//                 << "Still write access on location " << server.myLocality << " for region " << server.getInfo(ref).writeLocked;
//
//             server.store.erase(ref.id);
//         });

    }

    location_info_type locate(const data_item_reference_client_type& ref, const data_item_region_type& region)
    {

        location_info_type res;
//
//         for(auto& server : network_.servers){
//             std::cout<< server.get_id() << std::endl;
//              typedef typename allscale::server::data_item_server<DataItemType>::get_info_action action_type;
// 		        action_type()(server.get_id(),ref);
//             /*if(server.get_id() != this->get_id()){
//                 typedef typename allscale::server::data_item_server<DataItemType>::get_info_action action_type;
// 		        action_type()(server.get_id(),ref);
//             }*/
//
//         }
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

    data_item_fragment_type& get(const data_item_reference_client_type& ref)
    {
        std::unique_lock<mutex_type> l(mtx_);
        //std::cout<<"get method called for id: " << ref.id()<<std::endl;
        auto pos = store.find(ref.id());
        HPX_ASSERT(pos != store.end());
        return pos->second.fragment;
    }


    void print()
    {
        std::unique_lock<mutex_type> l(mtx_);
        std::cout<<"print method for server: " << this->get_id() << " " << "store.size():" << store.size()<<std::endl;
        /*
        for(auto& el : store){
            std::cout<<el.first.get_id()<<std::endl;
        }*/
    }

    void set_network(const network_type& network){

//         network_ = network;
    }

    void set_servers(){
        // servers_[hpx::naming::get_locality_id_from_id(this->get_id())] = this->get_id();
        // std::cout << servers_[hpx::naming::get_locality_id_from_id(this->get_id())] << std::endl;
//         auto data_item_server_name = allscale::data_item_server_name<DataItemType>::name();
//         auto res =  hpx::find_all_from_basename(data_item_server_name, hpx::find_all_localities().size());
//         for(auto& fut : res ){
//             auto id = fut.get();
//             //servers_[hpx::naming::get_locality_id_from_id(fut.get())] = fut.get();
//         }
    }
    void print_network(){
//         for(auto& server : network_.servers){
//             std::cout<<server.get_id()<<std::endl;
//         }
    }

//     HPX_DEFINE_COMPONENT_ACTION(data_item_server, get);
//     HPX_DEFINE_COMPONENT_ACTION(data_item_server, set_network);
//     HPX_DEFINE_COMPONENT_ACTION(data_item_server, set_servers);
//     HPX_DEFINE_COMPONENT_ACTION(data_item_server, print_network);
//     HPX_DEFINE_COMPONENT_ACTION(data_item_server, print);
//    HPX_DEFINE_COMPONENT_ACTION(data_item_server, get_info);
//    HPX_DEFINE_COMPONENT_ACTION(data_item_server, create_zero);
};

}}

/*
    HPX_REGISTER_ACTION_DECLARATION(                                          \
    	allscale::server::data_item_server<type>::create_zero_action,           \
        BOOST_PP_CAT(__data_item_server_create_zero_action_, type));            \
*/


#define REGISTER_DATAITEMSERVER_DECLARATION(type)                       \
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
 */
#define REGISTER_DATAITEMSERVER(type)                                   \
    typedef ::hpx::components::component<                                     \
    	allscale::server::data_item_server<type>                         \
    > BOOST_PP_CAT(__data_item_server_, type);                            \
    HPX_REGISTER_COMPONENT(BOOST_PP_CAT(__data_item_server_, type))       \


namespace allscale {
//    template<typename DataItemType>
// class data_item_server: public hpx::components::client_base<
// 		data_item_server<DataItemType>,
// 		allscale::server::data_item_server<DataItemType> > {
// 	typedef hpx::components::client_base<data_item_server<DataItemType>,
// 			allscale::server::data_item_server<DataItemType> > base_type;
//
// 	typedef typename allscale::server::data_item_server<
// 			DataItemType>::data_item_type data_item_type;
//     typedef typename allscale::data_item_reference<DataItemType> data_item_reference_client_type;
//
//     //using network_type = int;
//     using network_type = typename allscale::data_item_server_network<DataItemType>;
//
// public:
//
// 	data_item_server() {
// 	}
//
// 	data_item_server(hpx::future<hpx::id_type> && gid) :
// 			base_type(std::move(gid)) {
//         //        std::cout<<"called on loc" << this->get_id() << std::endl;
//         auto data_item_server_name = allscale::data_item_server_name<DataItemType>::name();
//         //data_item_server_name += hpx::naming::get_locality_id_from_id(this->get_id());
//         // std::cout<< hpx::register_with_basename(data_item_server_name,
//         // this->get_id(),hpx::naming::get_locality_id_from_id(this->get_id())).get()<<std::endl;
//
//     }
//
//     data_item_server(hpx::id_type && gid) :
// 			base_type(std::move(gid)) {
//         //        std::cout<<"called on loc" << this->get_id() << std::endl;
//         auto data_item_server_name = allscale::data_item_server_name<DataItemType>::name();
//         //data_item_server_name += hpx::naming::get_locality_id_from_id(this->get_id());
//         hpx::register_with_basename(data_item_server_name,this->get_id(),hpx::naming::get_locality_id_from_id(this->get_id())).get();
//
//     }
// 	template<typename ... T>
// 	data_item_reference_client_type create(const T& ... args) {
// 		HPX_ASSERT(this->get_id());
//
// 		typedef typename  allscale::server::data_item_server<DataItemType>::template create_action<T...> action_type;
// 		return action_type()(this->get_id(), args...);
// 	}
//
//     void print() {
// 		HPX_ASSERT(this->get_id());
// 		typedef typename allscale::server::data_item_server<DataItemType>::print_action action_type;
// 		action_type()(this->get_id());
// 	}
//
//     void print_network() {
// 		HPX_ASSERT(this->get_id());
// 		typedef typename allscale::server::data_item_server<DataItemType>::print_network_action action_type;
// 		action_type()(this->get_id());
// 	}
//     void set_network(const network_type& network){
//         HPX_ASSERT(this->get_id());
//         typedef typename allscale::server::data_item_server<DataItemType>::set_network_action action_type;
//         action_type()(this->get_id(),network);
//     }
//
//     void set_servers(){
//         HPX_ASSERT(this->get_id());
//         typedef typename allscale::server::data_item_server<DataItemType>::set_servers_action action_type;
//         action_type()(this->get_id());
//     }
//     typename DataItemType::facade_type get(const data_item_reference_client_type& ref){
//         /*
//         using parent_type = typename allscale::server::data_item_server<DataItemType>;
//         hpx::future<std::shared_ptr<parent_type> > f = hpx::get_ptr<parent_type>(this->get_id());
//         std::shared_ptr<parent_type> ptr = f.get();
//         auto result = (ptr.get())->get(ref);
//         return result;   */
//         HPX_ASSERT(this->get_id());
//         typedef typename allscale::server::data_item_server<DataItemType>::get_action action_type;
//         action_type()(this->get_id(),ref);
//
//     }
//
// };

}

#endif
