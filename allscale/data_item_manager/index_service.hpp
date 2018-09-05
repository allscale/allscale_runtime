#ifndef ALLSCALE_DATA_ITEM_MANAGER_INDEX_SERVICE_HPP
#define ALLSCALE_DATA_ITEM_MANAGER_INDEX_SERVICE_HPP

#include <allscale/hierarchy.hpp>
#include <allscale/api/core/data.h>
#include <allscale/data_item_reference.hpp>
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
            if (service_->here_.isLeaf())
            {
                return {};
            }

            if (region.empty())
            {
                return {};
            }

            std::lock_guard<mutex_type> l(mtx_);
            region_type allocated = region_type::merge(left_, right_);
            region_type unallocated = region_type::difference(full_, allocated);
            return region_type::intersect(region, unallocated);
        }

        region_type get_missing_region(region_type const& region) const
        {
            if (region.empty()) return {};

            std::lock_guard<mutex_type> l(mtx_);
            return region_type::difference(region, full_);
        }

        region_type get_missing_region_left(region_type const& region)
        {
            if (region.empty()) return {};

            std::lock_guard<mutex_type> l(mtx_);
            auto res = region_type::difference(region, left_);

            left_ = region_type::merge(left_, res);

            return res;
        }

        region_type get_missing_region_right(region_type const& region)
        {
            if (region.empty()) return {};

            std::lock_guard<mutex_type> l(mtx_);
            auto res = region_type::difference(region, right_);

            right_ = region_type::merge(right_, res);

            return res;
        }

        template <typename Requirement>
        void resize_fragment(Requirement const& req, region_type const& region, bool exclusive = true)
        {
            // resize exclusive...
            auto& item = data_item_store<data_item_type>::lookup(req.ref);
            {
                std::unique_lock<mutex_type> ll(item.mtx);
                // reserve region...
                if (!allscale::api::core::isSubRegion(region, item.reserved))
                {
                    // grow reserved region...
                    item.reserved = region_type::merge(item.reserved, region);

                    // resize fragment...
                    auto& frag = fragment(req.ref, item, ll);
                    frag.resize(item.reserved);
                }
                // update ownership
                if (exclusive)
                    item.exclusive = region;
            }
        }

        template <typename Requirement>
        void add_full(Requirement const& req, region_type const& region)
        {
            if (region.empty()) return;

            region_type full;
            {
                std::lock_guard<mutex_type> l(mtx_);
                full_ = region_type::merge(full_, region);
                full = full_;
            }
            resize_fragment(req, full);
        }

        template <typename Requirement>
        region_type add_left(Requirement const& req, region_type const& full, region_type const& required)
        {
            if(full.empty() && required.empty()) return {};

            region_type missing;
            region_type region;
            {
                std::lock_guard<mutex_type> l(mtx_);

                full_ = region_type::merge(full_, region);
                region = full_;

                // Get the missing region from the left...
                missing = region_type::difference(required, left_);

                // If nothing is missing, return empty...
                if (missing.empty()) return {};

                // Cut down to what this process is allowed...
                missing = region_type::difference(
                    region_type::intersect(missing, full_), right_);

                // add missing to left
                left_ = region_type::merge(left_, missing);
            }
            resize_fragment(req, region);

            return missing;
        }

        template <typename Requirement>
        region_type add_right(Requirement const& req, region_type const& full, region_type const& required)
        {
            if(full.empty() && required.empty()) return {};

            region_type missing;
            region_type region;
            {
                std::lock_guard<mutex_type> l(mtx_);

                full_ = region_type::merge(full_, region);
                region = full_;

                // Get the missing region from the right...
                missing = region_type::difference(required, right_);

                // If nothing is missing, return empty...
                if (missing.empty()) return {};

                // Cut down to what this process is allowed...
                missing = region_type::difference(
                    region_type::intersect(missing, full_), left_);

                // add missing to left
                right_ = region_type::merge(right_, missing);
            }
            resize_fragment(req, region);

            return missing;
        }

        void mark_region(region_type const& region)
        {
            if (region.empty()) return;

            std::lock_guard<mutex_type> l(mtx_);
            full_ = region_type::merge(full_, region);
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

            // Second look in our cache to speed up lookups
            for (auto const& cached: location_cache_.regions)
            {
                auto part = region_type::intersect(remaining, cached.second);
                // We got a hit!
                if (!part.empty())
                {
                    // Insert location information...
                    info.add_part(cached.first, part);

                    // Subtract what we got from what we requested
                    remaining = region_type::difference(remaining, part);

                    // If the remainder is empty, we got everything covered...
                    if (remaining.empty())
                    {
                        HPX_ASSERT(!info.regions.empty());
                        return hpx::make_ready_future(info);
                    }
                }
            }

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
                            location_cache_.add_part(part.first, part.second);
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

        index_service(index_service const& other)
          : here_(other.here_),
            parent_(other.parent_),
            left_(other.left_),
            right_(other.right_),
            parent_id_(other.parent_id_),
            right_id_(other.right_id_),
            is_root_(other.is_root_)
        {
            HPX_ASSERT(other.entries_.empty());
        }

        index_entry<DataItem>& get(hpx::naming::gid_type const& id)
        {
            std::unique_lock<mutex_type> l(mtx_);
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
