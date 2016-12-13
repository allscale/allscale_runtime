
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
        static void schedule(work_item work);
        static components::scheduler* run(std::size_t rank);
        static void stop();

    private:
        static std::size_t rank_;
        static std::shared_ptr<components::scheduler> & get_ptr();
        static components::scheduler & get();

        scheduler(std::size_t rank);

        std::shared_ptr<components::scheduler> component_;
    };
}

#endif
