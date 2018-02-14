
#ifndef ALLSCALE_COMPONENTS_MONITOR_HPP
#define ALLSCALE_COMPONENTS_MONITOR_HPP

#include <allscale/work_item.hpp>

#include <hpx/hpx.hpp>
#include <hpx/traits/action_schedule_thread.hpp>
#include <hpx/include/actions.hpp>
#include <hpx/include/lcos.hpp>
#include <hpx/include/components.hpp>
#include <hpx/include/serialization.hpp>
#include <boost/dynamic_bitset.hpp>

#include <boost/asio.hpp>

using boost::asio::ip::udp;

namespace allscale { namespace components {

       struct HPX_COMPONENT_EXPORT resilience
	 : hpx::components::component_base<resilience>
       {
           resilience()
           {
               HPX_ASSERT(false);
           }

           typedef hpx::lcos::local::spinlock mutex_type;

           //std::unique_ptr<hpx::threads::executors::io_pool_executor> scheduler;


           bool env_resilience_disabled = false;
           // START failure detection here (Kiril)
           udp::endpoint * my_receiver_endpoint, *guard_receiver_endpoint;
           enum state {TRUST, SUSPECT, RECOVERING};
           std::condition_variable cv;
           std::mutex cv_m;
           bool recovery_done; // protected via cv and cv_m
           std::atomic<state> my_state;
           std::atomic_bool keep_running;
           std::chrono::high_resolution_clock::time_point start_time,trust_lease;
           std::atomic<std::size_t> protectee_heartbeat;
           std::size_t my_heartbeat;
           const std::size_t miu = 1000;
           const std::size_t delta = 1000;
           boost::dynamic_bitset<> rank_running_;
           std::size_t get_running_ranks();
           bool rank_running(uint64_t rank);
           void failure_detection_loop_async ();
           void send_heartbeat_loop ();
           void thread_safe_printer(std::string output);
           // END failure detection here

           uint64_t rank_, num_localities;
           hpx::id_type guard_;
           uint64_t guard_rank_;
           hpx::id_type protectee_;
           static std::size_t protectee_rank_;
           hpx::id_type protectees_protectee_;
           uint64_t protectees_protectee_rank_;
           mutable mutex_type backup_mutex_;
           mutable mutex_type delegated_items_mutex_;
           mutable mutex_type remote_backup_mutex_;
           mutable mutex_type running_ranks_mutex_;
           std::map<std::string, work_item> local_backups_;
           mutable mutex_type result_mutex_;
           std::map<std::string, work_item> remote_backups_;
           std::map<std::string, work_item> delegated_items_;
           void init();
           resilience(std::uint64_t rank);
           void protectee_crashed();
           int get_cp_granularity();

           void set_guard(hpx::id_type guard, uint64_t guard_rank);
           HPX_DEFINE_COMPONENT_DIRECT_ACTION(resilience,set_guard);
           void send_heartbeat(uint64_t heartbeat);
           HPX_DEFINE_COMPONENT_DIRECT_ACTION(resilience,send_heartbeat);
           std::pair<hpx::id_type,uint64_t> get_protectee();
           HPX_DEFINE_COMPONENT_DIRECT_ACTION(resilience,get_protectee);
           std::map<std::string,work_item> get_local_backups();
           HPX_DEFINE_COMPONENT_DIRECT_ACTION(resilience,get_local_backups);
           void remote_backup(work_item wi);
           HPX_DEFINE_COMPONENT_ACTION(resilience,remote_backup);
           void remote_unbackup(std::string name);
           HPX_DEFINE_COMPONENT_DIRECT_ACTION(resilience,remote_unbackup);
           void shutdown(std::size_t token);
           HPX_DEFINE_COMPONENT_ACTION(resilience,shutdown);

           // Wrapping functions to signals from the runtime
           static void global_w_exec_start_wrapper(work_item const& w);
           void w_exec_start_wrapper(work_item const& w);
           static void global_w_exec_finish_wrapper(work_item const& w);
           void w_exec_finish_wrapper(work_item const& w);
           static void global_w_exec_dispatched_wrapper(work_item const& w);
           void w_exec_dispatched_wrapper(work_item const& w);
           mutable mutex_type access_scheduler_mtx_;
       };
}}

namespace hpx { namespace traits {
    template <>
    struct action_schedule_thread<allscale::components::resilience::remote_backup_action>
    {
        static void call(naming::address_type lva, naming::component_type comp_type,
            threads::thread_init_data& data, threads::thread_state_enum initial)
        {
            data.func(hpx::threads::wait_signaled);
        }
    };
}}

HPX_REGISTER_ACTION_DECLARATION(allscale::components::resilience::send_heartbeat_action, send_heartbeat_action)
HPX_REGISTER_ACTION_DECLARATION(allscale::components::resilience::remote_backup_action, remote_backup_action)
HPX_REGISTER_ACTION_DECLARATION(allscale::components::resilience::remote_unbackup_action, remote_unbackup_action)
HPX_REGISTER_ACTION_DECLARATION(allscale::components::resilience::set_guard_action, set_guard_action)
HPX_REGISTER_ACTION_DECLARATION(allscale::components::resilience::get_protectee_action, get_protectee_action)
HPX_REGISTER_ACTION_DECLARATION(allscale::components::resilience::get_local_backups_action, get_local_backups_action)
HPX_REGISTER_ACTION_DECLARATION(allscale::components::resilience::shutdown_action, allscale_resilience_shutdown_action)
#endif
