
#include <allscale/data_item_manager/get_ownership_json.hpp>

#include <vector>

namespace allscale { namespace data_item_manager {
    namespace {
        std::vector<std::function<std::string()>>& registry()
        {
            static std::vector<std::function<std::string()>> registry_;
            return registry_;
        }
    }

    void register_data_item(std::function<std::string()>&& f)
    {
        registry().push_back(f);
    }

    std::string get_ownership_json()
    {
        std::string res;
        res += '[';
        for(auto& f: registry())
        {
            res += f() + ',';
        }

        if (res.size() == 1)
        {
            res += ']';
        }
        else
        {
            res.back() = ']';
        }
        return res;
    }
}}
