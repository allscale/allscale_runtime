
#include <allscale/data_item_manager/get_ownership_json.hpp>
#include <hpx/lcos/local/spinlock.hpp>

#include <vector>
#include <mutex>

namespace allscale { namespace data_item_manager {
    namespace {
        std::vector<std::function<std::string()>>& registry()
        {
            static std::vector<std::function<std::string()>> registry_;
            return registry_;
        }
        using mutex_type = hpx::lcos::local::spinlock;

        mutex_type& registry_mtx()
        {
            static mutex_type mtx;
            return mtx;
        }
    }

    void register_data_item(std::function<std::string()>&& f)
    {
        std::lock_guard<mutex_type> l(registry_mtx());
        registry().push_back(f);
    }

    std::string get_ownership_json()
    {
        std::string res;
        res += '[';
        std::vector<std::function<std::string()>> registry_;
        {
            std::lock_guard<mutex_type> l(registry_mtx());
            registry_ = registry();
        }
        for(auto& f: registry_)
        {
            std::string json = f();
            if (json.empty()) continue;
            res += json + ',';
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
