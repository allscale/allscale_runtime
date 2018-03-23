
#ifndef ALLSCALE_COMPONENTS_SCHEDULER_NETWORK_HPP
#define ALLSCALE_COMPONENTS_SCHEDULER_NETWORK_HPP

#include <allscale/work_item.hpp>
#include <allscale/this_work_item.hpp>
#include <allscale/util/readers_writers_mutex.hpp>

#include <hpx/lcos/local/spinlock.hpp>
#include <hpx/runtime/naming/id_type.hpp>

#include <unordered_map>

namespace allscale { namespace components {
    struct scheduler_network
    {
        void schedule(std::size_t rank, work_item work, this_work_item::id,
            std::vector<hpx::util::function<void()>> register_owned = std::vector<hpx::util::function<void()>>());
        void stop();

    private:
        typedef allscale::util::readers_writers_mutex mutex_type;
        mutex_type mtx_;
        std::unordered_map<std::size_t, hpx::id_type> schedulers_;
    };
}}

#endif
