#ifndef ALLSCALE_DATA_ITEM_MANAGER_INDEX_SERVICE_HPP
#define ALLSCALE_DATA_ITEM_MANAGER_INDEX_SERVICE_HPP

#include <allscale/hierarchy.hpp>
#include <allscale/api/core/data.h>
#include <allscale/data_item_requirement.hpp>
#include <allscale/data_item_manager/location_info.hpp>
#include <allscale/data_item_manager/fragment.hpp>
#include <allscale/scheduler.hpp>
#include <allscale/util/readers_writers_mutex.hpp>

#include <array>
#include <functional>
#include <unordered_map>

namespace allscale { namespace data_item_manager {
    void register_index_service(std::function<void(runtime::HierarchicalOverlayNetwork &)> f);
    void init_index_services(runtime::HierarchicalOverlayNetwork& hierarchy);

    template <typename DataItem>
    struct index_service;

    template <typename Requirement>
    hpx::future<
        location_info<typename Requirement::region_type>
    > locate(runtime::HierarchyAddress const& addr, Requirement const& req);

    template <typename Requirement>
    struct locate_action
      : hpx::actions::make_direct_action<
            decltype(&locate<Requirement>),
            &locate<Requirement>,
            locate_action<Requirement>>::type
    {};

    template <typename DataItemReference, typename Region>
    hpx::future<location_info<Region>>
    acquire_ownership_for(runtime::HierarchyAddress const& addr,
            runtime::HierarchyAddress const& child,
            DataItemReference const& ref, Region missing);

    template <
        typename Requirement,
        typename DataItemReference = typename Requirement::ref_type,
        typename Region = typename Requirement::region_type
    >
    struct acquire_ownership_for_action
      : hpx::actions::make_direct_action<
            decltype(&acquire_ownership_for<DataItemReference, Region>),
            &acquire_ownership_for<DataItemReference, Region>,
            acquire_ownership_for_action<Requirement, DataItemReference, Region>>::type
    {};

    template <typename DataItemReference, typename Region>
    hpx::future<location_info<Region>>
    collect_child_ownerships(runtime::HierarchyAddress const& addr,
            DataItemReference const& ref, Region region);

    template <
        typename Requirement,
        typename DataItemReference = typename Requirement::ref_type,
        typename Region = typename Requirement::region_type
    >
    struct collect_child_ownerships_action
      : hpx::actions::make_direct_action<
            decltype(&collect_child_ownerships<DataItemReference, Region>),
            &collect_child_ownerships<DataItemReference, Region>,
            collect_child_ownerships_action<Requirement, DataItemReference, Region>>::type
    {};

    template <typename DataItem>
    struct index_entry
    {
        using data_item_type = DataItem;
        using mutex_type = allscale::util::readers_writers_mutex;
        using region_type = typename DataItem::region_type;
        using fragment_type = typename DataItem::fragment_type;
        using shared_data_type = typename DataItem::shared_data_type;

        explicit index_entry(index_service<DataItem>* service)
          : service_(service)
        {}

        index_entry(index_entry const& other) : service_(other.service_)
        {
            HPX_ASSERT(other.service_);
            HPX_ASSERT(other.full_.empty());
        }

        region_type get_managed_unallocated(region_type const& region) const
        {
            if (service_->here_.isLeaf()) return {};

            region_type allocated = region_type::merge(left_, right_);
            region_type unallocated = region_type::difference(full_, allocated);
            return region_type::intersect(region, unallocated);
        }

        region_type get_missing_region(region_type const& region) const
        {
            if (region.empty()) return {};
            std::lock_guard<mutex_type> l(mtx_);

            region_type managed_unallocated = get_managed_unallocated(region);
            if (!managed_unallocated.empty()) return managed_unallocated;

            return region_type::difference(region, full_);
        }

        bool check_write_requirement(region_type const& region) const
        {
            return get_missing_region(region).empty();
        }

        template <typename Requirement>
        void resize_fragment(Requirement const& req, region_type const& region, bool exclusive)
        {
            // resize exclusive...
            auto& item = data_item_store<data_item_type>::lookup(req.ref);
            {
                std::unique_lock<mutex_type> ll(item.mtx);
                auto& frag = fragment(req.ref, item, ll);
                // reserve region...
                if (!allscale::api::core::isSubRegion(region, frag.getCoveredRegion()))
                {
                    // grow reserved region...
                    region_type reserved = region_type::merge(frag.getCoveredRegion(), region);
                    // resize fragment...
                    frag.resize(reserved);
                    // update ownership
                    if (exclusive)
                        item.exclusive = region_type::merge(
                            item.exclusive,
                            region_type::intersect(frag.getCoveredRegion(), region));
                }
            }
        }

        void add_full(region_type const& region)
        {
            if (region.empty()) return;

            std::lock_guard<mutex_type> l(mtx_);
            full_ = region_type::merge(full_, region);
        }

        region_type add_left(region_type const& region, region_type const& required)
        {
            std::lock_guard<mutex_type> l(mtx_);
//             std::cout << "    left: " << region << ' ' << full_ << ' ' << left_ << ' ' << ' ' << right_ << '\n';

            // extend the local ownership
            full_ = region_type::merge(full_, region);

            // Get the missing region from the left...
            region_type missing = region_type::difference(required, left_);

            // If nothing is missing, return empty...
            if (missing.empty()) return {};

            // Cut down to what this process is allowed...
            missing = region_type::difference(
                region_type::intersect(missing, full_), right_);

            // add missing to left
            left_ = region_type::merge(left_, missing);

            return missing;
        }

        region_type add_right(region_type const& region, region_type const& required)
        {
            std::lock_guard<mutex_type> l(mtx_);
//             std::cout << "    right: " << region << ' ' << full_ << ' ' << left_ << ' ' << ' ' << right_ << '\n';

            // extend the local ownership
            full_ = region_type::merge(full_, region);

            // Get the missing region from the right...
            region_type missing = region_type::difference(required, right_);

            // If nothing is missing, return empty...
            if (missing.empty()) return {};

            // Cut down to what this process is allowed...
            missing = region_type::difference(
                region_type::intersect(missing, full_), left_);

            // add missing to right
            right_ = region_type::merge(right_, missing);

            return missing;
        }

        template <typename Requirement>
        hpx::future<location_info<region_type>>
        acquire_ownership(Requirement const& req, region_type const& missing)
        {
            {
                std::lock_guard<mutex_type> l(mtx_);
                full_ = region_type::merge(full_, missing);
                left_ = region_type::difference(left_, missing);
                right_ = region_type::difference(right_, missing);
            }
            add_full(missing);

            // If we are at the root, just return an empty loc info...
            if (service_->is_root_)
            {
                location_info<region_type> info;
                return hpx::make_ready_future(std::move(info));
            }
#if defined(HPX_DEBUG)
            HPX_ASSERT(!missing.empty());
            auto cont =
                [this, &req, missing](hpx::future<location_info<region_type>> infof)
                {
                    HPX_ASSERT(service_->here_.isLeaf());
                    // We should now cover all missing data.
                    location_info<region_type> info = infof.get();
                    {
                        region_type covered;
                        for (auto const& parts : info.regions)
                        {
                            covered = region_type::merge(covered, parts.second);
                        }
                        HPX_ASSERT(covered == missing);
                    }
                    return info;
                };


            if (service_->parent_id_)
            {
                return hpx::dataflow(hpx::launch::sync, std::move(cont),
                    hpx::async<acquire_ownership_for_action<Requirement>>(
                        service_->parent_id_,
                        service_->parent_, service_->here_, req.ref, missing));
            }
            else
            {
                return hpx::dataflow(hpx::launch::sync,
                    [cont = std::move(cont)](hpx::future<location_info<region_type>> infof)
                    {
                        return cont(std::move(infof));
                    },
                    data_item_manager::acquire_ownership_for(
                        service_->parent_, service_->here_, req.ref, missing));
            }
#else
            if (service_->parent_id_)
            {
                return hpx::async<acquire_ownership_for_action<Requirement>>(
                        service_->parent_id_,
                        service_->parent_, service_->here_, req.ref, std::move(missing));
            }
            else
            {
                return data_item_manager::acquire_ownership_for(
                        service_->parent_, service_->here_, req.ref, std::move(missing));
            }
#endif
        }

        template <typename DataItemReference>
        hpx::future<location_info<region_type>>
        acquire_ownership_for(runtime::HierarchyAddress const& child,
            DataItemReference const& ref, region_type missing)
        {
            using data_item_type = typename DataItemReference::data_item_type;
            using requirement_type = allscale::data_item_requirement<data_item_type>;
            HPX_ASSERT(!service_->here_.isLeaf());

#if defined(HPX_DEBUG)
            region_type missing_ref = missing;
//             // Make sure local management is consistent (for the requested part)
//             {
//                 std::unique_lock<mutex_type> l(mtx_);
//                 if (!(region_type::intersect(full_, missing) ==
//                     region_type::merge(
//                         region_type::intersect(left_, missing),
//                         region_type::intersect(right_, missing)
//                     )))
//                 {
//                     std::cerr
//                         << __FILE__ << ':' << __LINE__ << ":\n"
//                         << "Node:      " << service_->here_ << '\n'
//                         << "Regions:   " << missing << '\n'
//                         << "Available: " << full_ << '\n'
//                         << "Left:      " << left_ << '\n'
//                         << "Right:     " << right_ << '\n'
//                         << "Merged:    " << region_type::merge(left_, right_) << '\n'
//                         << " -- intersected -- \n"
//                         << "Available: " << region_type::intersect(full_, missing) << '\n'
//                         << "Left:      " << region_type::intersect(left_, missing) << '\n'
//                         << "Right:     " << region_type::intersect(right_, missing) << '\n'
//                         ;
//                     std::abort();
//                 }
//             }
#endif

            // Make sure the given child is really a child of this node
            HPX_ASSERT(child == service_->left_ || child == service_->right_);

            bool collect_left = (child == service_->right_);

            // Extract the missing regions from our left or right child
            region_type part;
            {
                std::unique_lock<mutex_type> l(mtx_);

//                 HPX_ASSERT(full_ == region_type::merge(left_, right_));

                if (collect_left)
                {
                    HPX_ASSERT(region_type::intersect(right_, missing).empty());
                    part = region_type::intersect(missing, left_);
                    right_ = region_type::merge(right_, missing);
                }
                else
                {
                    HPX_ASSERT(region_type::intersect(left_, missing).empty());
                    part = region_type::intersect(missing, right_);
                    left_ = region_type::merge(left_, missing);
                }

                if (!part.empty())
                {
//                     HPX_ASSERT(allscale::api::core::isSubRegion(part, full_));
                    missing = region_type::difference(missing, part);

                    // Remove remaining missing from our ownership list.
                    left_ = region_type::difference(left_, part);
                    right_ = region_type::difference(right_, part);

                    // Add the new ownerhsip info
                    // collect left means, we came from a right child, as such,
                    // the missing part is now owned by the right child and vice versa
                    if (collect_left)
                    {
                        right_ = region_type::merge(right_, part);
                    }
                    else
                    {
                        left_ = region_type::merge(left_, part);
                    }
                }

                // if there are still parts missing, we need to remove it from
                // our ownership record, and update the left/right part
                if (!missing.empty())
                {
                    full_ = region_type::difference(full_, missing);
                }
            }

            std::vector<hpx::future<location_info<region_type>>> remote_infos;
            remote_infos.reserve(2);

            // If we haven't covered everything, we need to descend to our parent
            if (!missing.empty())
            {
                HPX_ASSERT(!service_->is_root_);
                // If we are at the root, just return an empty loc info, this means
                // the part can be safely allocated at the destination
                if (service_->is_root_)
                {
                    location_info<region_type> info;
#if defined(HPX_DEBUG)
                    info.add_part(std::size_t(-1), missing);
#endif

                    return hpx::make_ready_future(std::move(info));
                }
                if (service_->parent_id_)
                {
                    remote_infos.push_back(
                        hpx::async<acquire_ownership_for_action<requirement_type>>(
                            service_->parent_id_, service_->parent_, service_->here_,
                            ref, std::move(missing)));
                }
                else
                {
                    remote_infos.push_back(
                        data_item_manager::acquire_ownership_for(
                            service_->parent_, service_->here_,
                            ref, std::move(missing)));
                }
            }

            // If the part is not empty, we need to collect the information from
            // our child.
            if (!part.empty())
            {
                auto id = collect_left ? hpx::invalid_id : service_->right_id_;
                auto child = collect_left ? service_->left_ : service_->right_;

                if (id)
                {
                    remote_infos.push_back(
                        hpx::async<collect_child_ownerships_action<requirement_type>>(
                            id, child, ref, std::move(part)));
                }
                else
                {
                    remote_infos.push_back(
                        data_item_manager::collect_child_ownerships(
                            child, ref, std::move(part)));
                }
            }

            return hpx::dataflow(hpx::launch::sync,
#if defined(HPX_DEBUG)
                [missing_ref](auto remote_infos)
#else
                [](auto remote_infos)
#endif
                {
                    // Merge infos...
                    location_info<region_type> info;
#if defined(HPX_DEBUG)
                    region_type covered;
#endif

                    for (auto& fut: remote_infos)
                    {
                        auto remote_info = fut.get();
                        for (auto& part: remote_info.regions)
                        {
                            info.add_part(part.first, part.second);
#if defined(HPX_DEBUG)
                            covered = region_type::merge(covered, part.second);
#endif
                        }
                    }
#if defined(HPX_DEBUG)
                    HPX_ASSERT(covered == missing_ref);
#endif
                    return info;
                },
                remote_infos);
        }

        template <typename DataItemReference>
        hpx::future<location_info<region_type>> collect_child_ownerships(
            DataItemReference ref, region_type missing)
        {
            using data_item_type = typename DataItemReference::data_item_type;
            using requirement_type = allscale::data_item_requirement<data_item_type>;
            std::array<region_type, 2> remote_regions;
#if defined(HPX_DEBUG)
            region_type missing_ref = missing;
#endif
            {
                std::unique_lock<mutex_type> l(mtx_);

//                 HPX_ASSERT(allscale::api::core::isSubRegion(missing, full_));
                // If we are at a leaf, we stop the recursion and return the
                // location info.
                if (service_->here_.isLeaf())
                {
                    location_info<region_type> info;

                    HPX_ASSERT(!missing.empty());
                    HPX_ASSERT(region_type::intersect(missing, full_) == missing);
                    HPX_ASSERT(region_type::difference(missing, full_).empty());

                    full_ = region_type::difference(full_, missing);
                    HPX_ASSERT(left_.empty());
                    HPX_ASSERT(right_.empty());
                    l.unlock();
                    std::size_t locality_id = service_->here_.getRank();
                    {
                        auto& item = data_item_store<DataItem>::lookup(ref.id());
                        std::unique_lock<mutex_type> ll(item.mtx);

                        // Remove exclusive ownership
                        item.exclusive = region_type::difference(item.exclusive, missing);

                        // If the fragment has not been allocated,
                        // we only had splits there so far, this means,
                        // we don't need to transfer data
                        auto& frag = fragment(ref, item, ll);
                        if (allscale::api::core::isSubRegion(missing, frag.getCoveredRegion()))
                        {
                            info.add_part(service_->here_.getRank(), std::move(missing));
                        }
#if defined(HPX_DEBUG)
                        else
                        {

                            info.add_part(std::size_t(-1), std::move(missing));
                        }
#endif
                    }


                    return hpx::make_ready_future(std::move(info));
                }

//                 HPX_ASSERT(full_ == region_type::merge(left_, right_));
                // Check left child
                {
                    auto part = region_type::intersect(missing, left_);
                    if (!part.empty())
                    {
                        left_ = region_type::difference(left_, part);
                        full_ = region_type::difference(full_, part);
                        missing = region_type::difference(missing, part);
                        HPX_ASSERT(!allscale::api::core::isSubRegion(part, right_));
                        remote_regions[0] = std::move(part);
                    }
                    else
                    {
                        remote_regions[0] = region_type();
                    }
                }
                // Check right child
                if (!missing.empty())
                {
                    auto part = region_type::intersect(missing, right_);
                    if (!part.empty())
                    {
                        right_ = region_type::difference(right_, part);
                        full_ = region_type::difference(full_, part);
#if defined(HPX_DEBUG)
                        missing = region_type::difference(missing, part);
#endif
                        HPX_ASSERT(!allscale::api::core::isSubRegion(part, left_));
                        remote_regions[1] = std::move(part);
                    }
                    else
                    {
                        remote_regions[1] = region_type();
                    }
                }
                else
                {
                    remote_regions[1] = region_type();
                }
            }
            HPX_ASSERT(missing.empty());

            std::vector<hpx::future<location_info<region_type>>> remote_infos;
            remote_infos.reserve(2);

            // First, recurse to right child for better overlap.
            if (!remote_regions[1].empty())
            {
                if (service_->right_id_)
                {
                    remote_infos.push_back(
                        hpx::async<collect_child_ownerships_action<requirement_type>>(
                            service_->right_id_, service_->right_, ref, std::move(remote_regions[1])));
                }
                else
                {
                    remote_infos.push_back(
                        data_item_manager::collect_child_ownerships(
                            service_->right_, ref, std::move(remote_regions[1])));
                }
            }
            if (!remote_regions[0].empty())
            {
                remote_infos.push_back(
                    data_item_manager::collect_child_ownerships(
                        service_->left_, ref, std::move(remote_regions[0])));
            }

            return hpx::dataflow(hpx::launch::sync,
#if defined(HPX_DEBUG)
                [missing_ref](auto remote_infos)
#else
                [](auto remote_infos)
#endif
                {
                    // Merge infos...
                    location_info<region_type> info;
#if defined(HPX_DEBUG)
                    region_type covered;
#endif

                    for (auto& fut: remote_infos)
                    {
                        auto remote_info = fut.get();
                        for (auto& part: remote_info.regions)
                        {
                            info.add_part(part.first, part.second);
#if defined(HPX_DEBUG)
                            covered = region_type::merge(covered, part.second);
#endif
                        }
                    }
#if defined(HPX_DEBUG)
                    HPX_ASSERT(covered == missing_ref);
#endif
                    return info;
                },
                remote_infos);
        }

        template <typename Requirement>
        hpx::future<location_info<region_type>> locate(Requirement const& req)
        {
            location_info<region_type> info;
//             HPX_ASSERT(req.mode == access_mode::ReadOnly);

            HPX_ASSERT(!req.region.empty());

            std::unique_lock<mutex_type> l(mtx_);
            hpx::util::ignore_all_while_checking il;

            region_type remaining = req.region;

            // First, check if we are the leaf node and own this region
            if (service_->here_.isLeaf())
            {
                auto part = region_type::intersect(remaining, full_);
                if (!part.empty())
                {
                    remaining = region_type::difference(remaining, part);
                    info.add_part(service_->here_.getRank(), part);
                    // If the remainder is empty, we got everything covered...
                    if (remaining.empty())
                    {
                        HPX_ASSERT(!info.regions.empty());
                        return hpx::make_ready_future(info);
                    }
                }
            }

//             // Second look in our cache to speed up lookups
//             for (auto const& cached: location_cache_.regions)
//             {
//                 auto part = region_type::intersect(remaining, cached.second);
//                 // We got a hit!
//                 if (!part.empty())
//                 {
//                     // Insert location information...
//                     info.add_part(cached.first, part);
//
//                     // Subtract what we got from what we requested
//                     remaining = region_type::difference(remaining, part);
//
//                     // If the remainder is empty, we got everything covered...
//                     if (remaining.empty())
//                     {
//                         HPX_ASSERT(!info.regions.empty());
//                         return hpx::make_ready_future(info);
//                     }
//                 }
//             }

            // Storage for our remote infos:
            // 0: parent
            // 1: left
            // 2: right
            std::array<region_type, 3> remote_regions;
            std::vector<hpx::future<location_info<region_type>>> remote_infos;
            remote_infos.reserve(3);

            if (!service_->here_.isLeaf())
            {
                // Start with right...
                {
                    auto part = region_type::intersect(remaining, right_);
                    if (!part.empty())
                    {
                        // reduce remaining
                        remaining = region_type::difference(remaining, part);
                        // add it to our lists to check further...
                        remote_regions[2] = std::move(part);
                    }
                    else
                    {
                        remote_regions[2] = region_type();
                    }
                }
                // Go on with left...
                if (!remaining.empty())
                {
                    auto part = region_type::intersect(remaining, left_);
                    if (!part.empty())
                    {
                        // reduce remaining
                        remaining = region_type::difference(remaining, part);
                        // add it to our lists to check further...
                        remote_regions[1] = std::move(part);
                    }
                    else
                    {
                        remote_regions[1] = region_type();
                    }
                }
            }
            else
            {
                remote_regions[1] = region_type();
                remote_regions[2] = region_type();
            }
            // Now escalate to the parent if necessary
            // If we are the root, or have nothing left, we are done...
            if (!service_->is_root_ && !remaining.empty())
            {
                remote_regions[0] = remaining;
            }
            else
            {
                remote_regions[0] = region_type();
            }

            l.unlock();

            if (!remote_regions[2].empty())
            {
                Requirement new_req(req.ref, remote_regions[2], req.mode);
                if (service_->right_id_)
                {
                    remote_infos.push_back(hpx::async<locate_action<Requirement>>(service_->right_id_,
                        service_->right_, std::move(new_req)));
                }
                else
                {
                    remote_infos.push_back(data_item_manager::locate(
                        service_->right_, std::move(new_req)));
                }
            }

            if (!remote_regions[1].empty())
            {
                Requirement new_req(req.ref, remote_regions[1], req.mode);
                HPX_ASSERT(service_->left_.getRank() == hpx::get_locality_id());
                remote_infos.push_back(data_item_manager::locate(
                    service_->left_, std::move(new_req)));
            }

            if (!remote_regions[0].empty())
            {
                Requirement new_req(req.ref, remote_regions[0], req.mode);
                if (service_->parent_id_)
                {
                    remote_infos.push_back(hpx::async<locate_action<Requirement>>(service_->parent_id_,
                        service_->parent_, std::move(new_req)));
                }
                else
                {
                    remote_infos.push_back(data_item_manager::locate(
                        service_->parent_, std::move(new_req)));
                }
            }

            return hpx::dataflow(hpx::launch::sync,
                [this, info = std::move(info)](auto remote_infos) mutable
                {
                    std::lock_guard<mutex_type> l(mtx_);
                    for (auto& fut : remote_infos)
                    {
                        auto remote_info = fut.get();
                        for (auto& part : remote_info.regions)
                        {
//                             location_cache_.add_part(part.first, part.second);
                            info.add_part(part.first, std::move(part.second));
                        }
                    }

                    return info;
                },
                remote_infos);

        }

        mutable mutex_type mtx_;

        region_type full_;
        region_type right_;
        region_type left_;

        // A simple location info data structure will serve as a cache to
        // accelerate lookups.
        location_info<region_type> location_cache_;

        // The service this entry belongs to...
        index_service<DataItem> *service_;
    };

    template <typename DataItem>
    struct index_service
    {
        using mutex_type = allscale::util::readers_writers_mutex;

        runtime::HierarchyAddress here_;
        runtime::HierarchyAddress parent_;
        runtime::HierarchyAddress left_;
        runtime::HierarchyAddress right_;
        hpx::id_type parent_id_;
        hpx::id_type right_id_;

        bool is_root_;

        mutex_type mtx_;
        std::unordered_map<hpx::naming::gid_type, index_entry<DataItem>> entries_;

        index_service(runtime::HierarchyAddress here)
          : here_(here)
          , parent_(here_.getParent())
          , is_root_(here_ == runtime::HierarchyAddress::getRootOfNetworkSize(
                allscale::get_num_numa_nodes(), allscale::get_num_localities()
                ))
        {
            if (parent_.getRank() != scheduler::rank())
            {
                parent_id_ = hpx::naming::get_id_from_locality_id(
                    parent_.getRank());
            }


            if (!here_.isLeaf())
            {
                left_ = here_.getLeftChild();
                right_ = here_.getRightChild();
                if (right_.getRank() >= allscale::get_num_localities())
                {
                    right_ = left_;
                }

                HPX_ASSERT(left_.getRank() == scheduler::rank());

                if (right_.getRank() != scheduler::rank())
                {
                    right_id_ = hpx::naming::get_id_from_locality_id(
                        right_.getRank());
                }
            }
        }

        index_service(index_service&& other)
          : here_(std::move(other.here_)),
            parent_(std::move(other.parent_)),
            left_(std::move(other.left_)),
            right_(std::move(other.right_)),
            parent_id_(std::move(other.parent_id_)),
            right_id_(std::move(other.right_id_)),
            is_root_(other.is_root_)
        {
            HPX_ASSERT(other.entries_.empty());
        }

        index_entry<DataItem>& get(hpx::naming::gid_type const& id)
        {
            std::lock_guard<mutex_type> l(mtx_);
            auto it = entries_.find(id);
            if (it != entries_.end())
            {
                HPX_ASSERT(it->second.service_ == this);
                return it->second;
            }

            auto res = entries_.insert(std::make_pair(id, index_entry<DataItem>(this)));
            HPX_ASSERT(res.second);
            return res.first->second;
        }

        index_entry<DataItem>& get(data_item_reference<DataItem> const& ref)
        {
            return get(ref.id());
        }
    };

    template <typename Requirement>
    hpx::future<
        location_info<typename Requirement::region_type>
    > locate(runtime::HierarchyAddress const& addr, Requirement const& req)
    {
        using data_item_type = typename Requirement::data_item_type;

        return runtime::HierarchicalOverlayNetwork::getLocalService<
            index_service<data_item_type>>(addr).get(req.ref).locate(req);

    }

    template <typename DataItemReference, typename Region>
    hpx::future<location_info<Region>>
    acquire_ownership_for(runtime::HierarchyAddress const& addr,
            runtime::HierarchyAddress const& child,
            DataItemReference const& ref, Region missing)
    {
        using data_item_type = typename DataItemReference::data_item_type;
        return runtime::HierarchicalOverlayNetwork::getLocalService<
            index_service<data_item_type>>(addr).get(ref).acquire_ownership_for(
                child, ref, std::move(missing));
    }

    template <typename DataItemReference, typename Region>
    hpx::future<location_info<Region>>
    collect_child_ownerships(runtime::HierarchyAddress const& addr,
            DataItemReference const& ref, Region region)
    {
        using data_item_type = typename DataItemReference::data_item_type;
        return runtime::HierarchicalOverlayNetwork::getLocalService<
            index_service<data_item_type>>(addr).get(ref).collect_child_ownerships(ref, std::move(region));
    }

    template <typename DataItem>
    struct auto_registration
    {
        auto_registration()
        {
            register_index_service(
                [](runtime::HierarchicalOverlayNetwork &hierarchy){
                    hierarchy.installService<index_service<DataItem>>();
                });
        }

        virtual ~auto_registration()
        {
        }
    };

}}

#endif
