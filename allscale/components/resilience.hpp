
#ifndef ALLSCALE_COMPONENTS_MONITOR_HPP
#define ALLSCALE_COMPONENTS_MONITOR_HPP

#include <allscale/work_item.hpp>

#include <hpx/hpx.hpp>
#include <hpx/include/actions.hpp>
#include <hpx/include/lcos.hpp>
#include <hpx/include/components.hpp>
#include <hpx/include/serialization.hpp>
#include <boost/dynamic_bitset.hpp>

//#include <boost/asio.hpp>

//using boost::asio::ip::udp;

namespace allscale { namespace components {

       struct HPX_COMPONENT_EXPORT resilience
	 : hpx::components::component_base<resilience>
       {
           resilience()
           {
               HPX_ASSERT(false);
           }

           std::unique_ptr<hpx::threads::executors::io_pool_executor> scheduler;

           std::atomic_bool resilience_component_running;

           //UDP port
           //udp::socket * sock;
           //const int UDP_PORT = 4444;
           // START failure detection here (Kiril)
           enum state {TRUST, SUSPECT};
           std::condition_variable cv;
           std::mutex cv_m;
           bool recovery_done; // protected via cv and cv_m
           state my_state;
           std::chrono::high_resolution_clock::time_point start_time,trust_lease;
           std::size_t heartbeat_counter;
           const std::size_t miu = 1000;
           const std::size_t delta = 2000;
           boost::dynamic_bitset<> rank_running_;
           bool rank_running(uint64_t rank);
           void failure_detection_loop_async ();
           void failure_detection_loop ();
           void check_with_delay(std::size_t actual_epoch);
           void send_heartbeat(std::size_t counter);
           HPX_DEFINE_COMPONENT_DIRECT_ACTION(resilience,send_heartbeat);
           std::string get_ip_address();
           //HPX_DEFINE_COMPONENT_DIRECT_ACTION(resilience,get_ip_address);
           // END failure detection here

           bool resilience_disabled;
           uint64_t rank_, num_localities;
           hpx::id_type guard_;
           uint64_t guard_rank_;
           hpx::id_type protectee_;
           uint64_t protectee_rank_;
           hpx::id_type protectees_protectee_;
           uint64_t protectees_protectee_rank_;
           std::mutex backup_mutex_;
           std::map<this_work_item::id,work_item> local_backups_;
           std::mutex result_mutex_;
           std::map<this_work_item::id,work_item> remote_backups_;
           void init();
           resilience(std::uint64_t rank);
           void protectee_crashed();
           int get_cp_granularity();

           void set_guard(hpx::id_type guard);
           HPX_DEFINE_COMPONENT_DIRECT_ACTION(resilience,set_guard);
           std::pair<hpx::id_type,uint64_t> get_protectee();
           HPX_DEFINE_COMPONENT_DIRECT_ACTION(resilience,get_protectee);
           std::map<this_work_item::id,work_item> get_local_backups();
           HPX_DEFINE_COMPONENT_DIRECT_ACTION(resilience,get_local_backups);
           void remote_backup(work_item wi);
           HPX_DEFINE_COMPONENT_DIRECT_ACTION(resilience,remote_backup);
           void remote_unbackup(work_item wi);
           HPX_DEFINE_COMPONENT_DIRECT_ACTION(resilience,remote_unbackup);
           void shutdown();
           HPX_DEFINE_COMPONENT_ACTION(resilience,shutdown);

           // Wrapping functions to signals from the runtime
           static void global_w_exec_start_wrapper(work_item const& w);
           void w_exec_start_wrapper(work_item const& w);
           static void global_w_exec_finish_wrapper(work_item const& w);
           void w_exec_finish_wrapper(work_item const& w);
           //void do_send(udp::endpoint receiver);
           //void do_receive(udp::endpoint sender);
       };
}}
HPX_REGISTER_ACTION_DECLARATION(allscale::components::resilience::remote_backup_action, remote_backup_action)
HPX_REGISTER_ACTION_DECLARATION(allscale::components::resilience::send_heartbeat_action, send_heartbeat_action)
HPX_REGISTER_ACTION_DECLARATION(allscale::components::resilience::remote_unbackup_action, remote_unbackup_action)
HPX_REGISTER_ACTION_DECLARATION(allscale::components::resilience::set_guard_action, set_guard_action)
HPX_REGISTER_ACTION_DECLARATION(allscale::components::resilience::get_protectee_action, get_protectee_action)
HPX_REGISTER_ACTION_DECLARATION(allscale::components::resilience::get_local_backups_action, get_local_backups_action)
HPX_REGISTER_ACTION_DECLARATION(allscale::components::resilience::shutdown_action, allscale_resilience_shutdown_action)
//HPX_REGISTER_ACTION_DECLARATION(allscale::components::resilience::get_ip_address_action, get_ip_address_action)
#endif
