
#ifndef ALLSCALE_MONITOR_HPP
#define ALLSCALE_MONITOR_HPP

#include <allscale/this_work_item.hpp>

#include <functional>
#include <memory>

namespace allscale {

    namespace components
    {
	struct monitor;
    }

    struct work_item;

    struct monitor
    {
        enum event {
            work_item_enqueued = 0,
            work_item_dequeued = 1,
            work_item_split_execution_started = 2,
            work_item_process_execution_started = 3,
            work_item_split_execution_finished = 4,
            work_item_process_execution_finished = 5,
            work_item_result_propagated = 6,
            work_item_first = 7,
            last_ = 8
        };

        typedef
            std::function<void(work_item const&)>
            event_function;

        static HPX_EXPORT components::monitor* run(std::size_t rank);
        static void connect(event e, event_function f);
        static void disconnect(event e, event_function f);
        static HPX_EXPORT void signal(event e, work_item const& w);
        static components::monitor & get();
        static components::monitor *get_ptr();
        static HPX_EXPORT void stop();

    private:
//        static std::shared_ptr<components::monitor> & get_ptr();
//        static components::monitor & get();

        monitor(std::size_t rank);

        std::shared_ptr<components::monitor> component_;

    };
}

#endif
