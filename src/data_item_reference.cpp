#include <allscale/data_item_reference.hpp>

namespace allscale {
    namespace detail {
        std::uint32_t create_data_item_id()
        {
            static hpx::util::atomic_count id(0);
            std::uint32_t this_id = static_cast<std::uint32_t>(++id - 1);
            std::cout << "creating DIR " << this_id << "\n";
            return this_id;
        }
    }
}
