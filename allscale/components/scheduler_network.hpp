
#ifndef ALLSCALE_COMPONENTS_SCHEDULER_NETWORK_HPP
#define ALLSCALE_COMPONENTS_SCHEDULER_NETWORK_HPP

#include <allscale/work_item.hpp>
#include <allscale/this_work_item.hpp>

#include <hpx/lcos/local/spinlock.hpp>
#include <hpx/runtime/naming/id_type.hpp>

#include <unordered_map>

namespace allscale { namespace components {
    struct scheduler_network
    {
        void schedule(std::size_t rank, work_item work, this_work_item::id const&);
        void stop();

    private:
        typedef hpx::lcos::local::spinlock mutex_type;
        mutex_type mtx_;
        std::unordered_map<std::size_t, hpx::id_type> schedulers_;
    };
}}

#endif
