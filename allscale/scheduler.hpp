
#ifndef ALLSCALE_SCHEDULER_HPP
#define ALLSCALE_SCHEDULER_HPP

#include <hpx/config.hpp>
#include <allscale/work_item.hpp>

#include <memory>

namespace allscale
{
    namespace components
    {
        struct scheduler;
    }

    struct scheduler
    {
        scheduler() { HPX_ASSERT(false); }
        scheduler(std::size_t rank);

        static void schedule(work_item work);
        static components::scheduler* run(std::size_t rank);
        static void stop();
        static components::scheduler* get_ptr();

    private:
        static std::size_t rank_;
        static components::scheduler & get();


        std::shared_ptr<components::scheduler> component_;
    };
}

#endif
