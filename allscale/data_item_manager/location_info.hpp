
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

        std::unordered_map<std::size_t, Region> regions;

        template <typename Archive>
        void serialize(Archive& ar, unsigned)
        {
            ar & regions;
        }
    };
}}

#endif
