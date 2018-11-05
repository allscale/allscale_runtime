
#include <allscale/data_item_manager/data_item_store.hpp>
#include <unordered_map>

namespace allscale { namespace data_item_manager {

    namespace detail
    {
        static std::unordered_map<data_item_id, void*>& get_map()
        {
            thread_local static std::unordered_map<data_item_id, void*> map;
            return map;
        }
        void tls_cache(data_item_id const& id, void *dm)
        {
            get_map()[id] = dm;
        }
        void *tls_cache(data_item_id const& id)
        {
            auto& map = get_map();
            auto it = map.find(id);
            if (it == map.end()) return nullptr;
            return it->second;
        }
    }
}}
