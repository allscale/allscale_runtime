#ifndef ALLSCALE_DATA_ITEM_VIEW_HPP
#define ALLSCALE_DATA_ITEM_VIEW_HPP

#include "allscale/utils/serializer.h"

#include <allscale/data_item_manager/fragment.hpp>
#include <hpx/util/annotated_function.hpp>

#include <memory>

namespace allscale { namespace data_item_manager {
    template <typename DataItem>
    struct data_item_store;

    template <typename DataItem>
    struct data_item_view
    {
        using fragment_type = typename DataItem::fragment_type;
        using region_type = typename DataItem::region_type;
        using mutex_type = typename data_item_store<DataItem>::data_item_type::mutex_type;

        hpx::naming::gid_type id_;
        region_type region_;
        bool migrate_;

        data_item_view() = default;

        data_item_view(hpx::naming::gid_type id, region_type region, bool migrate)
          : id_(std::move(id))
          , region_(std::move(region))
          , migrate_(migrate)
        {
#if defined(HPX_DEBUG)
            auto& item = data_item_store<DataItem>::lookup(id_);
            {
                std::unique_lock<mutex_type> ll(item.mtx);
                HPX_ASSERT(item.fragment);

                auto& frag = *item.fragment;

                HPX_ASSERT(allscale::api::core::isSubRegion(region_, frag.getCoveredRegion()));
            }
#endif
        }

        template <typename Archive>
        void load(Archive& ar, unsigned)
        {
            hpx::util::annotate_function("allscale::data_item_manager::data_item_view::load");
            ar & id_;
            auto & item = data_item_store<DataItem>::lookup(id_);
            allscale::utils::ArchiveReader reader(ar);
            {
                std::unique_lock<mutex_type> ll(item.mtx);
                HPX_ASSERT(item.fragment);

                auto& frag = *item.fragment;

                frag.insert(reader);
            }
        }

        template <typename Archive>
        void save(Archive& ar, unsigned) const
        {
            hpx::util::annotate_function("allscale::data_item_manager::data_item_view::save");
            ar & id_;
            allscale::utils::ArchiveWriter writer(ar);
            auto& item = data_item_store<DataItem>::lookup(id_);
            {
                std::unique_lock<mutex_type> ll(item.mtx);
                HPX_ASSERT(item.fragment);

                auto& frag = *item.fragment;
                HPX_ASSERT(allscale::api::core::isSubRegion(region_, frag.getCoveredRegion()));

                frag.extract(writer, region_);

//                 if (migrate_)
//                 {
//                     // Remove exclusive ownership
//                     region_type reserved = region_type::difference(frag.getCoveredRegion(), region_);
//                     frag.resize(reserved);
//                 }
            }
        }

        HPX_SERIALIZATION_SPLIT_MEMBER()
    };
}}

#endif
