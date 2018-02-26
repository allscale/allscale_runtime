#ifndef ALLSCALE_DATA_ITEM_VIEW_HPP
#define ALLSCALE_DATA_ITEM_VIEW_HPP

#include "allscale/utils/serializer.h"

#include <memory>

namespace allscale { namespace data_item_manager {
    template <typename DataItem>
    struct data_item_store;

    template <typename DataItem>
    struct data_item_view
    {
        using fragment_type = typename DataItem::fragment_type;
        using region_type = typename DataItem::region_type;

        hpx::naming::gid_type id_;
        fragment_type* fragment_;
        region_type region_;

        data_item_view()
          : fragment_(nullptr)
        {}

        data_item_view(hpx::naming::gid_type id, fragment_type& fragment, region_type region)
          : id_(std::move(id))
          , fragment_(&fragment)
          , region_(std::move(region))
        {
        }

        template <typename Archive>
        void load(Archive& ar, unsigned)
        {
            ar & id_;
            auto & item = data_item_store<DataItem>::lookup(id_);
            using mutex_type = typename data_item_store<DataItem>::data_item_type::mutex_type;
            std::unique_lock<mutex_type> l(item.mtx);
            HPX_ASSERT(item.fragment);
            fragment_ = item.fragment.get();
            allscale::utils::ArchiveReader reader(ar);

            fragment_->insert(reader);
        }

        template <typename Archive>
        void save(Archive& ar, unsigned) const
        {
            ar & id_;
            allscale::utils::ArchiveWriter writer(ar);
            fragment_->extract(writer, region_);
        }

        HPX_SERIALIZATION_SPLIT_MEMBER()
    };
}}

#endif
