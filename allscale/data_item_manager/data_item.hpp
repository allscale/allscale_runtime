
#ifndef ALLSCALE_DATA_ITEM_MANAGER_DATA_ITEM_HPP
#define ALLSCALE_DATA_ITEM_MANAGER_DATA_ITEM_HPP

#include <allscale/data_item_manager/get_ownership_json.hpp>
#include <allscale/data_item_manager/location_info.hpp>
#include <allscale/util/readers_writers_mutex.hpp>
#include <hpx/lcos/local/spinlock.hpp>
#include <hpx/runtime/naming/id_type.hpp>
#include "allscale/utils/printer/join.h"

#include <memory>
#include <unordered_map>
#include <sstream>

namespace allscale { namespace data_item_manager {
    namespace detail
    {
        template <typename Region>
        std::string to_json(hpx::naming::gid_type const& id, Region const& region, ...)
        {
            return std::string();
        }

        template <typename Region>
        std::string to_json(hpx::naming::gid_type const& id, Region const& region, decltype(Region::Dimension)*)
        {
            if (region.empty()) return std::string();

            std::stringstream out;
            out << "{\"id\" : " << id.get_lsb() << ",";
            out << "\"type\" : \"" << Region::Dimensions << "D-Grid\",";
            out << "\"region\" : [";

            out << allscale::utils::join(",",region.getBoxes(),[](std::ostream& out, const auto& cur){
                out << "{";
                out << "\"from\" : " << cur.getMin() << ",";
                out << "\"to\" : " << cur.getMax();
                out << "}";
            });

            out << "]}";

            return out.str();
        }
    }

    // The data_item represents a fragment of the globally distributed data item
    // It knows about ownership of its own local region, and the ones of it's
    // parents and children for faster lookup. The lookup is organized in a
    // binary tree fashion.
    // For faster lookup times, frequently accessed fragments are cached.
    template <typename DataItemType>
    struct data_item
    {
        using data_item_type = DataItemType;
        using mutex_type = allscale::util::readers_writers_mutex;
        using region_type = typename DataItemType::region_type;
        using fragment_type = typename DataItemType::fragment_type;
        using shared_data_type = typename DataItemType::shared_data_type;

        data_item()
        {
            // Registering callback for ownership reporting to dashboard.
            register_data_item(
                [this]() -> std::string
                {
                    std::lock_guard<mutex_type> l(mtx);
                    return detail::to_json(id, exclusive, nullptr);
                });
        }

        data_item(data_item const&) = delete;
        data_item(data_item&&) = delete;
        data_item& operator=(data_item const&) = delete;
        data_item& operator=(data_item&&) = delete;

        // The mutex which protects this data item from concurrent accesses
        mutex_type mtx;

        hpx::naming::gid_type id;

        std::unique_ptr<fragment_type> fragment;
        std::unique_ptr<shared_data_type> shared_data;

        region_type exclusive;
    };
}}

#endif
