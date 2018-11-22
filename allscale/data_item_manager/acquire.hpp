
#ifndef ALLSCALE_DATA_ITEM_MANAGER_ACQUIRE_HPP
#define ALLSCALE_DATA_ITEM_MANAGER_ACQUIRE_HPP

#include <allscale/lease.hpp>
#include <allscale/api/core/data.h>
#include <allscale/data_item_requirement.hpp>
#include <allscale/data_item_manager/index_service.hpp>
#include <allscale/data_item_manager/fragment.hpp>
#include <allscale/data_item_manager/data_item_view.hpp>
#include <allscale/data_item_manager/location_info.hpp>

#include <hpx/lcos/broadcast.hpp>
#include <hpx/util/annotated_function.hpp>

#include <hpx/plugins/parcel/coalescing_message_handler_registration.hpp>

#include <hpx/include/actions.hpp>
#include <hpx/runtime/naming/id_type.hpp>
#include <hpx/util/tuple.hpp>

#include <vector>

namespace allscale { namespace data_item_manager {
    namespace detail
    {
        template <typename DataItemType>
        data_item_view<DataItemType> transfer(data_item_id id, typename DataItemType::region_type region, bool migrate)
        {
            // The data_item_view transparently extracts and inserts the parts
            // into the responsible fragments.
            return data_item_view<DataItemType>(id, std::move(region), migrate);
        }

        template <typename DataItemType>
        struct transfer_action
          : hpx::actions::make_direct_action<
                decltype(&transfer<DataItemType>),
                &transfer<DataItemType>,
                transfer_action<DataItemType>
            >::type
        {};

        template <typename DataItemType>
        void update_cache(data_item_id id, typename DataItemType::region_type missing, std::size_t new_rank)
        {
            runtime::HierarchicalOverlayNetwork::forAllLocal<index_service<DataItemType>>(
                [&](index_service<DataItemType>& is)
                {
                    is.get(id).update_cache(missing, new_rank);
                }
            );
        }

        template <typename DataItemType>
        struct update_cache_action
          : hpx::actions::make_direct_action<
                decltype(&update_cache<DataItemType>),
                &update_cache<DataItemType>,
                update_cache_action<DataItemType>
            >::type
        {};

        template <typename Requirement>
        hpx::future<void> acquire(runtime::HierarchyAddress const& addr, Requirement const& req)
        {
            using data_item_type = typename Requirement::data_item_type;
            using region_type = typename data_item_type::region_type;
            using location_info = location_info<region_type>;
            using transfer_action_type = transfer_action<data_item_type>;
            using mutex_type = typename data_item_store<data_item_type>::data_item_type::mutex_type;

            HPX_ASSERT(addr.getRank() == hpx::get_locality_id());
            HPX_ASSERT(addr.isLeaf());

            HPX_ASSERT(!req.region.empty());

            // First, check if we have missing regions for write access
            if (req.mode == access_mode::ReadWrite)
            {
                auto& entry =
                    runtime::HierarchicalOverlayNetwork::getLocalService<index_service<data_item_type>>(addr.getLayer()).get(req.ref);
//                 entry.resize_fragment(req, req.region, true);
                region_type missing;
                {
                    std::lock_guard<mutex_type> l(entry.mtx_);
                    missing = region_type::difference(req.region, entry.full_);
                }

                if(!missing.empty())
                {
                    entry.resize_fragment(req, missing, true);
                    HPX_ASSERT(req.allowance.empty());

                    hpx::future<void> update_cache =
                        hpx::lcos::broadcast<update_cache_action<data_item_type>>(
                            hpx::find_all_localities(), req.ref.id(), missing, addr.getRank());
//                     HPX_ASSERT(addr.isLeaf());
                    // Acquire ownership
                    return hpx::dataflow(hpx::launch::sync,
                        [req, addr](hpx::future<location_info> infof, hpx::future<void> cache_update) ->hpx::future<void>
                        {
                            cache_update.get();
                            auto info = infof.get();
                            if (info.regions.empty()) return hpx::make_ready_future();

                            std::vector<hpx::future<void>> transfers;

                            transfers.reserve(info.regions.size());

                            // collect the parts we need to transfer...

                            for (auto const& part: info.regions)
                            {
                                if (part.first == addr.getRank()) continue;
#if defined(HPX_DEBUG)
                                if (part.first == std::size_t(-1)) continue;
#endif

                                hpx::id_type target(
                                    hpx::naming::get_id_from_locality_id(part.first));
                                transfers.push_back(
                                    hpx::async<transfer_action_type>(target, req.ref.id(),
                                        std::move(part.second), true));
                            }
                            return hpx::when_all(transfers);
                            // Do transfer...
                        },
                        entry.acquire_ownership(req, missing), update_cache);
                }

                // All requirements should be fulfilled now.
#if defined(HPX_DEBUG)
                {
                    std::lock_guard<mutex_type> l(entry.mtx_);
                    HPX_ASSERT(allscale::api::core::isSubRegion(req.region, entry.full_));
                }
#endif
                return hpx::make_ready_future();
            }

            HPX_ASSERT(req.mode == access_mode::ReadOnly);

            return hpx::dataflow(hpx::launch::sync,
                [req, addr](hpx::future<location_info> infof) -> hpx::future<void>
                {
                    auto info = infof.get();
                    if (info.regions.empty()) return hpx::make_ready_future();

//                     auto& entry =
//                         runtime::HierarchicalOverlayNetwork::getLocalService<index_service<data_item_type>>(addr.getLayer()).get(req.ref);
//                     entry.resize_fragment(req, req.region, false);

                    std::vector<hpx::future<void>> transfers;

                    transfers.reserve(info.regions.size());

                    HPX_ASSERT(req.mode == access_mode::ReadOnly);

                    // collect the parts we need to transfer...

                    for (auto const& part: info.regions)
                    {
                        if (part.first == addr.getRank()) continue;
#if defined(HPX_DEBUG)
                        if (part.first == std::size_t(-1)) continue;
#endif

                        hpx::id_type target(
                            hpx::naming::get_id_from_locality_id(part.first));
                        transfers.push_back(
                            hpx::async<transfer_action_type>(target, req.ref.id(),
                                std::move(part.second), false));
                    }
                    return hpx::when_all(transfers);
                    // Do transfer...
                },
                data_item_manager::locate(addr, req)
            );
        }

        template <typename Requirement, typename RequirementAllocator>
        hpx::future<void> acquire(runtime::HierarchyAddress const& addr, std::vector<Requirement, RequirementAllocator> const& reqs)
        {
            std::vector<hpx::future<void>> futs;
            futs.reserve(reqs.size());
            for (auto& req: reqs)
            {
                futs.push_back(acquire(addr, req));
            }

            return hpx::when_all(futs);
        }

        template <typename Requirements, std::size_t...Is>
        hpx::future<void> acquire(runtime::HierarchyAddress const& addr, Requirements const& reqs,
            hpx::util::detail::pack_c<std::size_t, Is...>)
        {
            if (sizeof...(Is) == 0) return hpx::make_ready_future();

            std::array<hpx::future<void>, sizeof...(Is)> futs;
            int const sequencer[] = {0, (futs[Is] = detail::acquire(addr, hpx::util::get<Is>(reqs)), 0)...};
            (void)sequencer;

            return hpx::when_all(futs);
        }
    }

    template <typename Requirements>
    hpx::future<void>
    acquire(runtime::HierarchyAddress const& addr, Requirements const& reqs)
    {
        return detail::acquire(addr, reqs,
            typename hpx::util::detail::make_index_pack<
                hpx::util::tuple_size<Requirements>::type::value>::type{});
    }
}}

#endif
