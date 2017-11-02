#ifndef ALLSCALE_DATA_ITEM_SERVER
#define ALLSCALE_DATA_ITEM_SERVER

#include <allscale/components/data_item_network.hpp>
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

#include <hpx/lcos/local/packaged_task.hpp>

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
    using shared_data_type = typename DataItemType::shared_data_type;
    using fragment_type = typename DataItemType::fragment_type;
    using region_type = typename DataItemType::region_type;
//     using network_type = typename allscale::data_item_network<DataItemType>;
    using lease_type = typename allscale::lease<DataItemType>;

    using requirement_type = allscale::data_item_requirement<DataItemType>;
    using location_info = allscale::location_info<DataItemType>;
    using data_item_view = allscale::data_item_view<DataItemType>;

    struct fragment_info {
        // the managed fragment
        fragment_type fragment;

        // the regions currently locked through read leases
        region_type readLocked;

        // the list of all granted read leases
        std::vector<region_type> readLeases;

        // the regions currently locked through write access
        region_type writeLocked;

        fragment_info(){}

        fragment_info(const shared_data_type& shared) :
                fragment(shared), readLocked(), writeLocked() {
        }

        fragment_info(const fragment_info&) = default;
        fragment_info(fragment_info&&) = default;

        void addReadLease(const region_type& region) {

            // add to lease set
            readLeases.push_back(region);

            // merge into read locked region
            readLocked = region_type::merge(readLocked, region);
        }

        void removeReadLease(const region_type& region) {

			// remove lease from lease set
			auto pos = std::find(readLeases.begin(), readLeases.end(), region);
            HPX_ASSERT(pos != readLeases.end());
            readLeases.erase(pos);

            // update readLocked status
            region_type locked = region_type();
            for (const auto& cur : readLeases) {
                locked = region_type::merge(locked, cur);
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

    std::size_t rank_;

    mutable mutex_type mtx_;
    hpx::lcos::local::detail::condition_variable write_cv;
    typedef	std::map<hpx::id_type, fragment_info> store_type;

	store_type store;
    data_item_network network;

    typedef std::map<hpx::id_type, location_info> location_store_type;
    location_store_type location_store;

    public:

	data_item(std::size_t rank)
      : rank_(rank)
      , network(data_item_server_name<DataItemType>::name())
    {
    }

    hpx::future<lease_type> acquire_impl(const requirement_type& req, typename store_type::iterator store_it, std::unique_lock<mutex_type> l)
    {
        // get local fragment info
        auto& info = store_it->second;

        if (req.mode == access_mode::ReadWrite)
        {
            // Block if another task has a write lease...
            // This should ideally not happen...
            auto to_test = region_type::difference(info.writeLocked, req.region);
            while (!(to_test.empty() || to_test == info.writeLocked))
            {
                // TODO: add monitoring event!
                write_cv.wait(l);
                to_test = region_type::difference(info.writeLocked, req.region);
            }


            // FIXME: add debug check that all other fragments don't have
            // any kinds of locks on this fragment...
            info.writeLocked = region_type::merge(
                info.writeLocked, req.region);

//                     write_cv.notify_all(std::move(l));
            return hpx::make_ready_future(lease_type(req));
        }

        HPX_ASSERT(req.mode == access_mode::ReadOnly);
        // We no longer need to hold onto the lock ... the rest is asynchronous
        // and will lock once required...
        l.unlock();

        return locate(req).then(
            [this, store_it, req](hpx::future<location_info> future_info)
            {
                auto loc_info = future_info.get();
                // Now that we got our location information, we need to transfer the stuff

                auto const& parts = loc_info.parts();
                std::vector<hpx::future<void>> transferred;
                transferred.reserve(parts.size());
                // collect data on data distribution,
                // FIXME: add different strategies?
                // FIXME: Combine parts belonging to same locality?
                for(auto const& p: parts)
                {
                    if (p.rank != rank_)
                    {
                        auto overlap = region_type::intersect(req.region, p.region);
                        HPX_ASSERT(!overlap.empty());
//                         std::cout << "need to transfer: " << overlap << " from " << p.rank << " to " << rank_ << '\n';
                        hpx::lcos::local::packaged_task<hpx::future<void>(hpx::id_type const& id)> task(
                            [overlap, req](hpx::id_type id)
                            {
                                return hpx::async<transfer_action>(id, req.ref.id(), overlap);
                            });
                        transferred.push_back(task.get_future());
                        network.apply(p.rank, std::move(task));
                    }
                }

                return hpx::when_all(transferred).then(
                    [this, store_it, req](hpx::future<void> f)
                    {
                        // Check for errors...
                        f.get();
                        {
                            std::unique_lock<mutex_type> l(mtx_);
                            auto& info = store_it->second;
                            info.addReadLease(req.region);

                            return lease_type(req);
                        }
                    });
            }
        );
    }

    hpx::future<lease_type> acquire(const requirement_type& req)
    {
        std::unique_lock<mutex_type> l(mtx_);
        // Find our requested Data Item.
        auto store_it = store.find(req.ref.id());

        // get local fragment info
        bool needs_first_touch = false;
        if (store_it == store.end())
        {
            // If it is not there already, we "first touch it" with the data stored
            // in the data item reference.
            auto store_pair = store.insert(
                std::make_pair(req.ref.id(), fragment_info(req.ref.shared_data())));
            HPX_ASSERT(store_pair.second);
            store_it = store_pair.first;

            // The first access needs to be ReadWrite...
            HPX_ASSERT(req.mode == access_mode::ReadWrite);

            needs_first_touch = true;

        }
        auto& info = store_it->second;

        if(!needs_first_touch && req.mode == access_mode::ReadWrite)
        {
            if (region_type::intersect(info.fragment.getCoveredRegion(), req.region).empty())
                needs_first_touch = true;
        }

        // allocate storage for requested data on local fragment
        info.fragment.resize(region_type::merge(info.fragment.getCoveredRegion(), req.region));

        if (needs_first_touch)
        {
            // We store the region information locally for caching purposes
            location_store[req.ref.id()].add_part(req.region, rank_);

            l.unlock();
            hpx::lcos::local::packaged_task<hpx::future<lease_type>(hpx::id_type const& id)> task(
                // This stores our "first touch" acquired region to our manager
                [this, req, store_it](hpx::id_type const& id)
                {
                    return hpx::async<first_touch_action>(id, req.ref.id(), req.region, rank_).then(
                        [this, req, store_it](hpx::future<void> f)
                        {
                            f.get();
                            std::unique_lock<mutex_type> l(mtx_);
                            return acquire_impl(req, store_it, std::move(l));
                        }
                    );
                }
            );
            hpx::future<lease_type> ret = task.get_future();
            {
                // TODO: implement binary tree lookup
                network.apply(0, std::move(task));
            }

            return ret;
        }

        return acquire_impl(req, store_it, std::move(l));
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
                info.writeLocked = region_type::difference(info.writeLocked, lease.region);

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

    void first_touch(hpx::id_type const& id, region_type const& region, std::size_t rank)
    {
        std::unique_lock<mutex_type> l(mtx_);

        location_store[id].add_part(region, rank);
    }
    HPX_DEFINE_COMPONENT_DIRECT_ACTION(data_item, first_touch);

    location_info get_location_info_impl(hpx::id_type const& id, region_type const& region, region_type* remainder)
    {
        std::unique_lock<mutex_type> l(mtx_);
        auto locate_it = location_store.find(id);
        HPX_ASSERT(locate_it != location_store.end());

        location_info res;

        for(auto const& p: locate_it->second.parts())
        {
            auto part = region_type::intersect(p.region, region);
            if (!part.empty())
            {
                if (remainder)
                {
                    *remainder = region_type::difference(*remainder, part);
                }
                res.add_part(std::move(part), p.rank);
            }
        }
        return res;
    }
    location_info get_location_info(hpx::id_type const& id, region_type const& region)
    {
        return get_location_info_impl(id, region, nullptr);
    }
    HPX_DEFINE_COMPONENT_DIRECT_ACTION(data_item, get_location_info);

    data_item_view transfer(hpx::id_type const& id, region_type const& region)
    {
        std::unique_lock<mutex_type> l(mtx_);

        auto store_it = store.find(id);
        HPX_ASSERT(store_it != store.end());

        return data_item_view(id, store_it->second.fragment, region);
    }
    HPX_DEFINE_COMPONENT_DIRECT_ACTION(data_item, transfer);

    hpx::future<location_info> locate(const requirement_type& req)
    {
        hpx::id_type req_id = req.ref.id();
        region_type req_region = std::move(req.region);
        region_type remainder = req_region;
        location_info cached = get_location_info_impl(req_id, req_region, &remainder);

        if(remainder.empty())
        {
            return hpx::make_ready_future(cached);
        }

        req_region = remainder;

        // TODO: binary tree lookup...
        hpx::lcos::local::packaged_task<hpx::future<location_info>(hpx::id_type const& id)> task(
            [req_id, req_region, cached](hpx::id_type id) mutable
            {
                return hpx::async<get_location_info_action>(id, req_id, req_region).then(
                    [cached](hpx::future<location_info> f) mutable
                    {
                        for(auto& part: f.get().parts())
                        {
                            cached.add_part(std::move(part.region), part.rank);
                        }
                        return cached;
                    }
                );
            });
        hpx::future<location_info> f = task.get_future();
        network.apply(0, std::move(task));

        return f;
    }

    fragment_type& get(const data_item_reference<DataItemType>& ref)
    {
        std::unique_lock<mutex_type> l(mtx_);
        auto pos = store.find(ref.id());
        HPX_ASSERT(pos != store.end());
        return pos->second.fragment;
    }
};

}}

#endif
