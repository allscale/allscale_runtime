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
        region_type region_;

        data_item_view() = default;

        data_item_view(hpx::naming::gid_type id, region_type region)
          : id_(std::move(id))
          , region_(std::move(region))
        {
        }

        template <typename Archive>
        void load(Archive& ar, unsigned)
        {
            ar & id_;
//             hpx::util::high_resolution_timer timer;
            auto & item = data_item_store<DataItem>::lookup(id_);
            HPX_ASSERT(item.fragment);
            allscale::utils::ArchiveReader reader(ar);

            item.fragment->insert(reader);
//             using mutex_type = typename data_item_store<DataItem>::data_item_type::mutex_type;
//             {
//                 std::unique_lock<mutex_type> l(item.mtx);
//                 item.insert_time += timer.elapsed();
//             }
        }

        template <typename Archive>
        void save(Archive& ar, unsigned) const
        {
            ar & id_;
//             hpx::util::high_resolution_timer timer;
            allscale::utils::ArchiveWriter writer(ar);
            auto& item = data_item_store<DataItem>::lookup(id_);
            HPX_ASSERT(item.fragment);
            item.fragment->extract(writer, region_);

//             auto & item = data_item_store<DataItem>::lookup(id_);
//             using mutex_type = typename data_item_store<DataItem>::data_item_type::mutex_type;
//             {
//                 std::unique_lock<mutex_type> l(item.mtx);
//                 item.extract_time += timer.elapsed();
//             }
        }

        HPX_SERIALIZATION_SPLIT_MEMBER()
    };
}}

#endif
