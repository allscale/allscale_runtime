
#ifndef ALLSCALE_DATA_ITEM_MANAGER_LOCATION_INFO_HPP
#define ALLSCALE_DATA_ITEM_MANAGER_LOCATION_INFO_HPP

#include <hpx/runtime/serialization/unordered_map.hpp>

#include <unordered_map>

namespace allscale { namespace data_item_manager {
    template <typename Region>
    struct location_info
    {
        location_info() = default;

        location_info(location_info const& other) = default;

        location_info(location_info&& other) noexcept
          : regions(std::move(other.regions))
        {
        }

        void add_part(std::size_t rank, Region&& region)
        {
            HPX_ASSERT(!region.empty());
            auto& part = regions[rank];
            part = Region::merge(part, std::move(region));
        }

        void add_part(std::size_t rank, Region const& region)
        {
            HPX_ASSERT(!region.empty());
            auto& part = regions[rank];
            part = Region::merge(part, region);
        }

        void update(std::size_t rank, Region const& region)
        {
            Region updated;
            for (auto& part: regions)
            {
                Region reg = Region::intersect(part.second, region);
                if (!reg.empty())
                {
                    part.second = Region::difference(part.second, region);
                    updated = Region::merge(updated, reg);
                }
            }
            if (!updated.empty())
            {
                add_part(rank, std::move(updated));
            }
        }

        std::unordered_map<std::size_t, Region> regions;

        template <typename Archive>
        void serialize(Archive& ar, unsigned)
        {
            ar & regions;
        }
    };
}}

#endif
