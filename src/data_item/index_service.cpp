#include <allscale/data_item_manager/index_service.hpp>

#include <vector>

namespace allscale { namespace data_item_manager {
    namespace {
        std::vector<std::function<void(runtime::HierarchicalOverlayNetwork &)>>& service_init_functions()
        {
            static std::vector<std::function<void(runtime::HierarchicalOverlayNetwork &)>> fs;

            return fs;
        }
    }

    void register_index_service(std::function<void(runtime::HierarchicalOverlayNetwork &)> f)
    {
        service_init_functions().push_back(f);
    }

    void init_index_services(runtime::HierarchicalOverlayNetwork& hierarchy)
    {
        for(auto const& f : service_init_functions())
        {
            f(hierarchy);
        }
    }
}}
