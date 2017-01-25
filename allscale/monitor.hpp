
#ifndef ALLSCALE_MONITOR_HPP
#define ALLSCALE_MONITOR_HPP

#include <allscale/this_work_item.hpp>

#include <functional>

namespace allscale {

    struct work_item;

    struct monitor
    {
        enum event {
            work_item_enqueued = 0,
            work_item_dequeued = 1,
            work_item_execution_started = 2,
            work_item_execution_finished = 3,
            work_item_result_propagated = 4,
            work_item_first = 4,
            last_ = 5
        };

        typedef
            std::function<void(work_item const&)>
            event_function;

        static void connect(event, event_function f);
        static void signal(event, work_item const& w);
    };
};

#endif
