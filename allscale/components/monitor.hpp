
#ifndef ALLSCALE_COMPONENTS_MONITOR_HPP
#define ALLSCALE_COMPONENTS_MONITOR_HPP


namespace allscale { namespace components {

        void monitor_component_init();

        // Historical Data Introspection
        double get_iteration_time(int i);
        double get_last_iteration_time();
	long get_number_of_iterations();
}}

#endif
