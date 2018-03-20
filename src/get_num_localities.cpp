
#include <allscale/get_num_localities.hpp>
#include <hpx/lcos/future.hpp>
#include <hpx/runtime/get_num_localities.hpp>

#include <cstdint>

namespace allscale {
    std::size_t get_num_localities()
    {
        // FIXME: make resilient...
        static thread_local std::size_t num(0);

        if (num == std::size_t(0))
        {
            num = hpx::get_num_localities().get();
        }

        return num;
    }
}
