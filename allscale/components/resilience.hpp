
#ifndef ALLSCALE_COMPONENTS_MONITOR_HPP
#define ALLSCALE_COMPONENTS_MONITOR_HPP

#include <allscale/work_item.hpp>

#include <hpx/hpx.hpp>
#include <hpx/include/actions.hpp>
#include <hpx/include/lcos.hpp>
#include <hpx/include/components.hpp>
#include <hpx/include/serialization.hpp>



namespace allscale { namespace components {

       struct HPX_COMPONENT_EXPORT resilience 
	 : hpx::components::component_base<resilience>
       {
           resilience()
           {
               HPX_ASSERT(false);
           }

           uint64_t rank_, num_localities;
           hpx::id_type guard;
           work_item backup_;
           void init();
           resilience(std::uint64_t rank);
           int get_cp_granularity();
           void backup(work_item wi);
           HPX_DEFINE_COMPONENT_ACTION(resilience,backup);
           
           // Wrapping functions to signals from the runtime
           static void global_w_exec_start_wrapper(work_item const& w);
           void w_exec_start_wrapper(work_item const& w);
       };
}}
HPX_REGISTER_ACTION_DECLARATION(allscale::components::resilience::backup_action, backup_action)
#endif
