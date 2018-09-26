
#ifndef ALLSCALE_DATA_ITEM_MANAGER_GET_OWNERSHIP_JSON_HPP
#define ALLSCALE_DATA_ITEM_MANAGER_GET_OWNERSHIP_JSON_HPP

#include <functional>
#include <string>

namespace allscale { namespace data_item_manager {
    void register_data_item(std::function<std::string()>&& f);
    std::string get_ownership_json();
}}

#endif
