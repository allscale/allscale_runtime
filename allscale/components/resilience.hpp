
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

           bool resilience_disabled;
           uint64_t rank_, num_localities;
           hpx::id_type guard_;
           hpx::id_type protectee_;
           hpx::id_type protectees_protectee_;
           std::mutex backup_mutex_;
           std::map<this_work_item::id,work_item> local_backups_;
           std::mutex result_mutex_;
           std::map<this_work_item::id,work_item> remote_backups_;
           void init();
           resilience(std::uint64_t rank);
           void protectee_crashed();
           int get_cp_granularity();
           void set_guard(hpx::id_type guard);
           HPX_DEFINE_COMPONENT_ACTION(resilience,set_guard);
           hpx::id_type get_protectee();
           HPX_DEFINE_COMPONENT_ACTION(resilience,get_protectee);
           std::map<this_work_item::id,work_item> get_local_backups();
           HPX_DEFINE_COMPONENT_ACTION(resilience,get_local_backups);
           void remote_backup(work_item wi);
           HPX_DEFINE_COMPONENT_ACTION(resilience,remote_backup);
           void remote_unbackup(work_item wi);
           HPX_DEFINE_COMPONENT_ACTION(resilience,remote_unbackup);

           // Wrapping functions to signals from the runtime
           static void global_w_exec_start_wrapper(work_item const& w);
           void w_exec_start_wrapper(work_item const& w);
           static void global_w_exec_finish_wrapper(work_item const& w);
           void w_exec_finish_wrapper(work_item const& w);
       };
}}
HPX_REGISTER_ACTION_DECLARATION(allscale::components::resilience::remote_backup_action, remote_backup_action)
HPX_REGISTER_ACTION_DECLARATION(allscale::components::resilience::remote_unbackup_action, remote_unbackup_action)
HPX_REGISTER_ACTION_DECLARATION(allscale::components::resilience::set_guard_action, set_guard_action)
HPX_REGISTER_ACTION_DECLARATION(allscale::components::resilience::get_protectee_action, get_protectee_action)
HPX_REGISTER_ACTION_DECLARATION(allscale::components::resilience::get_local_backups_action, get_local_backups_action)
#endif
