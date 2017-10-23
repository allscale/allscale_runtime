#ifndef ALLSCALE_DATA_ITEM_SERVER
#define ALLSCALE_DATA_ITEM_SERVER

#include <allscale/components/data_item_network.hpp>
#include <allscale/locality.h>
#include <allscale/data_item_requirement.hpp>
#include <allscale/data_item_reference.hpp>
#include <allscale/lease.hpp>
#include <allscale/location_info.hpp>
#include <allscale/transfer_plan.hpp>
#include <allscale/simple_transfer_plan_generator.hpp>
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

namespace components {

template<typename DataItemType>
class data_item
  : public hpx::components::component_base<data_item<DataItemType> >
{

public:
    using data_item_type = DataItemType;
    using data_item_shared_data_type = typename DataItemType::shared_data_type;
    using data_item_fragment_type = typename DataItemType::fragment_type;
    using data_item_region_type = typename DataItemType::region_type;
    using data_item_reference_client_type = typename allscale::data_item_reference<DataItemType>;
    using network_type = typename allscale::data_item_network<DataItemType>;
    using lease_type = typename allscale::lease<DataItemType>;
    using location_info_type = typename allscale::location_info<DataItemType>;
    using locality_type = hpx::id_type;
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

        fragment_info(){}

        fragment_info(const data_item_shared_data_type& shared) :
                fragment(shared), readLocked(), writeLocked() {
        }

        fragment_info(const fragment_info&) = default;
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

        template <typename Archive>
        void serialize(Archive & ar, unsigned){
        /*    ar & fragment;
            ar & readLocked;
            ar & readLeases;
            ar & writeLocked;
        */
        }
    };

    typedef hpx::lcos::local::spinlock mutex_type;

    mutable mutex_type mtx_;
    hpx::lcos::local::detail::condition_variable write_cv;
	std::map<hpx::id_type, fragment_info> store;
    network_type network_;
    std::map<std::size_t,hpx::id_type> servers_;
    public:

	data_item()
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
                    // Block if another task has a write lease...
                    // This should ideally not happen...
                    auto to_test = data_item_region_type::difference(info.writeLocked, req.region);
                    while (!(to_test.empty() || to_test == info.writeLocked))
                    {
                        // TODO: add monitoring event!
                        write_cv.wait(l);
                        to_test = data_item_region_type::difference(info.writeLocked, req.region);
                    }


                    // FIXME: add debug check that all other fragments don't have
                    // any kinds of locks on this fragment...
                    info.writeLocked = data_item_region_type::merge(
                        info.writeLocked, req.region);

//                     write_cv.notify_all(std::move(l));

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
                HPX_ASSERT(allscale::api::core::isSubRegion(lease.region,info.readLocked));

                // remove read lease
                info.removeReadLease(lease.region);

                break;
            }
            case access_mode::ReadWrite: {
                // check that this region is actually protected

                HPX_ASSERT(allscale::api::core::isSubRegion(lease.region, info.writeLocked));

                // lock data for write
                info.writeLocked = data_item_region_type::difference(info.writeLocked, lease.region);

                write_cv.notify_all(std::move(l));

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


     /*
	 * A utility computing a transfer plan based on a given data distribution information,
	 * a target location, a region to be targeted, and a desired access mode.
	 *
	 * @param info a summary of the data distribution state covering the provided region
	 * @param targetLocation the location where the given region should be moved to
	 * @param region the region to be moved
	 * @param mode the intended access mode of the targeted region after the transfer
	 */
	template<typename TransferPlanGenerator = simple_transfer_plan_generator>
	transfer_plan<DataItemType> build_plan(const location_info<DataItemType>& info, locality_type targetLocation, const data_item_requirement<DataItemType>& requirement) {
		// use the transfer plan generator policy as requested
		return TransferPlanGenerator()(info,targetLocation,requirement);
	}

    location_info_type locate(const data_item_reference_client_type& ref, const data_item_region_type& region) {
        location_info_type result;
//         for(const auto& server : network_.servers){
//             if(server.get_id() != this->get_id()){
//                 typedef typename allscale::server::data_item_server<DataItemType>::get_info_action action_type;
//                 auto res = action_type()(server.get_id(),ref);
//                 auto part = data_item_region_type::intersect(region,res);
//                 if(!part.empty())
//                 {
//                     result.addPart(part,server.get_id());
//                 }
//             }
//         }

        return result;
    }

    data_item_fragment_type& get(const data_item_reference_client_type& ref)
    {
        std::unique_lock<mutex_type> l(mtx_);
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
        network_ = network;
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
        allscale::components::data_item<type>                         \
    > BOOST_PP_CAT(__data_item_server_, type);                            \
    HPX_REGISTER_COMPONENT(BOOST_PP_CAT(__data_item_server_, type))       \

#endif
