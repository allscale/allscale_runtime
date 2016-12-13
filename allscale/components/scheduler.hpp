
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

        void enqueue(work_item work, bool remote);
        HPX_DEFINE_COMPONENT_ACTION(scheduler, enqueue);

        void stop();
        HPX_DEFINE_COMPONENT_ACTION(scheduler, stop);

    private:
        std::uint64_t num_localities_;
        std::uint64_t rank_;
        boost::atomic<std::uint64_t> schedule_rank_;
        boost::atomic<bool> stopped_;
        hpx::id_type left_;
        hpx::id_type right_;

        bool do_split(work_item const& work);

        bool collect_counters();

        hpx::util::interval_timer timer_;

        mutex_type counters_mtx_;
        std::vector<hpx::id_type> idle_rates_counters_;
        std::vector<double> idle_rates_;
        double total_idle_rate_;

        std::vector<hpx::id_type> queue_length_counters_;
        std::vector<std::size_t> queue_length_;
        std::size_t total_length_;
    };
}}

#endif
