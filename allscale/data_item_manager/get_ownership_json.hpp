
#ifndef ALLSCALE_DATA_ITEM_MANAGER_GET_OWNERSHIP_JSON_HPP
#define ALLSCALE_DATA_ITEM_MANAGER_GET_OWNERSHIP_JSON_HPP

#include <hpx/config.hpp>

#include <functional>
#include <string>

namespace allscale { namespace data_item_manager {
    HPX_EXPORT void register_data_item(std::function<std::string()>&& f);
    HPX_EXPORT std::string get_ownership_json();
}}

#endif
