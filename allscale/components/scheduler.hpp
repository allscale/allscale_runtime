
#ifndef ALLSCALE_COMPONENTS_SCHEDULER_HPP
#define ALLSCALE_COMPONENTS_SCHEDULER_HPP

#include <allscale/work_item.hpp>

#include <hpx/include/components.hpp>
#include <hpx/util/interval_timer.hpp>

#include <deque>
#include <vector>

namespace allscale { namespace components {
    struct scheduler
      : hpx::components::component_base<scheduler>
    {
        typedef hpx::lcos::local::spinlock mutex_type;

        scheduler()
        {
            HPX_ASSERT(false);
        }

        scheduler(std::uint64_t rank);
        void init();

        void enqueue(work_item work);
        HPX_DEFINE_COMPONENT_DIRECT_ACTION(scheduler, enqueue);
        std::vector<work_item> dequeue();
        HPX_DEFINE_COMPONENT_DIRECT_ACTION(scheduler, dequeue);

        bool steal_work();
        void run();
        void stop();
        HPX_DEFINE_COMPONENT_DIRECT_ACTION(scheduler, stop);

    private:
        std::uint64_t num_localities_;
        std::uint64_t rank_;
        boost::atomic<std::uint64_t> schedule_rank_;
        boost::atomic<bool> stopped_;
        hpx::id_type left_;
        hpx::id_type right_;
        boost::atomic<std::size_t> count_;

        mutex_type work_queue_mtx_;
        hpx::lcos::local::condition_variable_any work_queue_cv_;
        std::deque<work_item> work_queue_;

        hpx::util::interval_timer timer_;
    };
}}

#endif
