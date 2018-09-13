
#include <allscale/get_num_numa_nodes.hpp>
#include <hpx/runtime/resource/partitioner_fwd.hpp>
#include <hpx/runtime/resource/detail/partitioner.hpp>

#include <cstdint>

namespace allscale {
    std::size_t get_num_numa_nodes()
    {
        // FIXME: make resilient...
        static thread_local std::size_t num(0);

        if (num == std::size_t(0))
        {
            num = hpx::resource::get_partitioner().numa_domains().size();
        }

        return num;
    }
}
