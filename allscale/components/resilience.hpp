
#ifndef ALLSCALE_COMPONENTS_MONITOR_HPP
#define ALLSCALE_COMPONENTS_MONITOR_HPP

#include <unistd.h>
#include <unordered_map>
#include <iostream>
#include <fstream>
#include <sstream>
#include <mutex>
#include <memory>
#include <vector>
#include <stdlib.h>
#include <string>

#include <allscale/work_item_dependency.hpp>
#include <allscale/work_item.hpp>
#include <allscale/profile.hpp>
#include <allscale/work_item_stats.hpp>
#include <allscale/util/graph_colouring.hpp>
#include <allscale/historical_data.hpp>

#include <hpx/include/components.hpp>

#include <hpx/include/lcos.hpp>

namespace allscale { namespace components {

       struct HPX_COMPONENT_EXPORT resilience 
	 : hpx::components::component_base<resilience>
       {
           resilience()
           {
               HPX_ASSERT(false);
           }

           uint64_t rank_;
           void init();
           resilience(std::uint64_t rank);
           
           // Wrapping functions to signals from the runtime
           static void global_w_exec_start_wrapper(work_item const& w);
           void w_exec_start_wrapper(work_item const& w);
       };
}}

#endif
