
#ifndef ALLSCALE_RESILIENCE_HPP
#define ALLSCALE_RESILIENCE_HPP

#include <allscale/this_work_item.hpp>
#include <allscale/work_item.hpp>

#include <functional>
#include <memory>

namespace allscale {
    namespace components {
        struct resilience;
    }
    struct resilience
    {
        static HPX_EXPORT components::resilience* run(std::size_t rank);
        static HPX_EXPORT void stop();
        static components::resilience *get_ptr();
        static components::resilience &get();
        static std::pair<hpx::shared_future<hpx::id_type>, uint64_t> get_protectee();
        static void handle_my_crash(int signum);
        static void global_wi_dispatched(work_item const& w, size_t schedule_rank);
        static bool rank_running(uint64_t rank);
    private:
        resilience(std::uint64_t rank);
        std::shared_ptr<components::resilience> component_;
    };
}

#endif
