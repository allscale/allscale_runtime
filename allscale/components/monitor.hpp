
#ifndef ALLSCALE_COMPONENTS_MONITOR_HPP
#define ALLSCALE_COMPONENTS_MONITOR_HPP

#include <unistd.h>
#include <unordered_map>
#include <iostream>
#include <fstream>
#include <sstream>
#include <thread>
#include <mutex>
#include <memory>
#include <vector>
#include <queue>
#include <stdlib.h>
#include <string>
#include <condition_variable>

#include <allscale/work_item_dependency.hpp>
#include <allscale/work_item.hpp>
#include <allscale/profile.hpp>
#include <allscale/work_item_stats.hpp>
#include <allscale/util/graph_colouring.hpp>
#include <allscale/historical_data.hpp>


#include <hpx/include/components.hpp>

#include <hpx/include/lcos.hpp>
#include <hpx/util/interval_timer.hpp>

#ifdef HAVE_PAPI
#include <hpx/include/performance_counters.hpp>

#define MAX_PAPI_COUNTERS 4
#endif

#define MIN_QUEUE_ELEMS 500

typedef std::unordered_map<std::string, allscale::work_item_stats> profile_map;
typedef std::unordered_map<std::string, std::vector<std::string>> dependency_graph;

namespace allscale { namespace components {

       struct HPX_COMPONENT_EXPORT monitor
	 : hpx::components::component_base<monitor>
       {

           monitor()
           {
               HPX_ASSERT(false);
           }

	   monitor(std::uint64_t rank);
           void init();
           void stop();
           HPX_DEFINE_COMPONENT_ACTION(monitor, stop);

           std::uint64_t get_my_rank() { return rank_; }
//         HPX_DEFINE_COMPONENT_ACTION(monitor, get_rank);
//           std::uint64_t get_rank_remote(hpx::id_type locality);

//           hpx::id_type get_left_neighbour() { return left_; }
//           hpx::id_type get_right_neighbour() { return right_; }


           /////////////////////////////////////////////////////////////////////////////////////
           ///                       Performance Data Introspection
           ////////////////////////////////////////////////////////////////////////////////////

           /// \brief This function returns the exclusive execution time of a work item.
           ///        Exclusive time is the time spent only in the work item itself.
           ///
           /// \param w_id         [in] work item id ("1.0.1" for example)
           ///
           /// \returns            work item exclusive execution time
           double get_exclusive_time(std::string w_id);


           /// \brief This function returns the inclusive execution time of a work item.
           ///        Inclusive time is the time spent in the work item and its children. In other
           ///        words, the time from the beginning of the work item until it has the result ready.
           ///
           /// \param w_id         [in] work item id ("1.0.1" for example)
           ///
           /// \returns            work item inclusive execution time
           double get_inclusive_time(std::string w_id);

           double get_inclusive_time_old(std::string w_id);

           /// \brief This function returns the average exclusive execution time
           ///        of a set of work items with the same name.
           ///
           /// \param w_name         [in] work item name ("fib" for example)
           ///
           /// \returns            work item average exclusive execution time
           double get_average_exclusive_time(std::string w_name);

           /// \brief This function returns the minimum exclusive execution time
           ///        of a set of work items with the same name.
           ///
           /// \param w_name         [in] work item name ("fib" for example)
           ///
           /// \returns            work item minimum exclusive execution time
           double get_minimum_exclusive_time(std::string w_name);


           /// \brief This function returns the maximum exclusive execution time
           ///        of a set of work items with the same name.
           ///
           /// \param w_name         [in] work item name ("fib" for example)
           ///
           /// \returns            work item average mximum execution time
           double get_maximum_exclusive_time(std::string w_name);


           /// \brief This function returns the mean exclusive execution time for
           ///        all the children of a work item.
           ///
           /// \param w_id         [in] work item id ("1.0.1" for example)
           ///
           /// \returns            children mean exclusive execution time
           double get_children_mean_time(std::string w_id);


           /// \brief This function returns the standard deviation of
           ///        the exclusive execution time for all the children
           ///        of a work item.
           ///
           /// \param w_id         [in] work item id ("1.0.1" for example)
           ///
           /// \returns            standard deviation for children's exclusive execution time
           double get_children_SD_time(std::string w_id);


           /// \brief This function returns the average exclusive execution time for
           ///        the last num_work_items executed.
           ///
           /// \param num_work_items     [in] number of work items executed
           ///
           /// \returns                  average execution time for the last X work items executed
           double get_avg_work_item_times(std::uint32_t num_work_items);

           /// \brief This function returns the standard deviation in the execution
           ///        time for the last num_work_items executed.
           ///
           /// \param num_work_items     [in] number of work items executed
           ///
           /// \returns                  standard deviation in exec time for the last X work items executed
           double get_SD_work_item_times(std::uint32_t num_work_items);


           // Inter-node introspection (synchronous wrappers for remote actions)
//           HPX_DEFINE_COMPONENT_ACTION(monitor, get_my_rank);
           HPX_DEFINE_COMPONENT_ACTION(monitor, get_exclusive_time);
           HPX_DEFINE_COMPONENT_ACTION(monitor, get_inclusive_time);
           HPX_DEFINE_COMPONENT_ACTION(monitor, get_average_exclusive_time);
           HPX_DEFINE_COMPONENT_ACTION(monitor, get_minimum_exclusive_time);
           HPX_DEFINE_COMPONENT_ACTION(monitor, get_maximum_exclusive_time);
           HPX_DEFINE_COMPONENT_ACTION(monitor, get_children_mean_time);
           HPX_DEFINE_COMPONENT_ACTION(monitor, get_children_SD_time);


           // Remote calls for the Introspection API
//           std::uint64_t get_remote_rank(hpx::id_type locality);
           double get_exclusive_time_remote(hpx::id_type locality, std::string w_id);
           double get_inclusive_time_remote(hpx::id_type locality, std::string w_id);
           double get_average_exclusive_time_remote(hpx::id_type locality, std::string w_name);
           double get_minimum_exclusive_time_remote(hpx::id_type locality, std::string w_name);
           double get_maximum_exclusive_time_remote(hpx::id_type locality, std::string w_name);
           double get_children_mean_time_remote(hpx::id_type locality, std::string w_id);
           double get_children_SD_time_remote(hpx::id_type locality, std::string w_id);


#ifdef HAVE_PAPI
           // PAPI counters
           /// \brief This function returns an array with the PAPI counter values measured
           ///        for a work item. The array must be deallocated explicitly after its use.
           ///
           /// \param w_id         [in] work item id ("1.0.1" for example)
           ///
           /// \returns            array of counter values, ordered as they were especified
           ///                     in the var MONITOR_PAPI
           long long *get_papi_counters(std::string w_id);
#endif


           /////////////////////////////////////////////////////////////////////////////////////
           ///                       Historical Data Introspection
           ////////////////////////////////////////////////////////////////////////////////////

           /// \brief This function returns the execution time for the i-th iteration.
           ///
           /// \param i         [in] iteration number
           ///
           /// \returns         iteration time
           double get_iteration_time(int i);


           /// \brief This function returns the execution time for the last iteration
           ///        executed.
           ///
           /// \returns         last iteration time
           double get_last_iteration_time();


           /// \brief This function returns the number of iterations executed.
           ///
           /// \returns         number of iterations executed
           long get_number_of_iterations();


           /// \brief This function returns the average execution time for the
           ///        last num_iters iterations.
           ///
           /// \param num_iters     [in] number of iterations
           ///
           /// \returns             average iteration time
           double get_avg_time_last_iterations(std::uint32_t num_iters);

           uint64_t get_timestamp( void );


           /// \brief This function returns the idle rate
           ///        for the last measuring interval in the current locality.
           ///
           /// \returns             idle rate
           double get_idle_rate();


           /// \brief This function returns the average idle rate
           ///        of the current locality.
           ///
           /// \returns             average idle rate
           double get_avg_idle_rate();

           HPX_DEFINE_COMPONENT_ACTION(monitor, get_idle_rate);
//           HPX_DEFINE_COMPONENT_ACTION(monitor, get_avg_idle_rate);

           double get_idle_rate_remote(hpx::id_type locality);
           double get_avg_idle_rate_remote(hpx::id_type locality);


           // Wrapping functions to signals from the runtime
           static void global_w_exec_split_start_wrapper(work_item const& w);
           void w_exec_split_start_wrapper(work_item const& w);

           static void global_w_exec_process_start_wrapper(work_item const& w);
           void w_exec_process_start_wrapper(work_item const& w);

           static void global_w_exec_split_finish_wrapper(work_item const& w);
           void w_exec_split_finish_wrapper(work_item const& w);

           static void global_w_exec_process_finish_wrapper(work_item const& w);
           void w_exec_process_finish_wrapper(work_item const& w);

           static void global_w_enqueued_wrapper(work_item const& w);
           void w_enqueued_wrapper(work_item const& w);

           static void global_w_dequeued_wrapper(work_item const& w);
           void w_dequeued_wrapper(work_item const& w);

           static void global_w_result_propagated_wrapper(work_item const& w);
           void w_result_propagated_wrapper(work_item const& w);

           static void global_w_app_iteration(allscale::work_item const& w);
           void w_app_iteration(allscale::work_item const& w);

           static void global_finalize();
           void monitor_component_finalize();


           // Functions related to the sampled metrics per locality

           /// \brief This function changes the sampling frequency
           ///
           /// \param new_interval     [in] new interval time in milliseconds
//           void change_sampling_interval(long long new_interval);

           /// \brief This function returns the task throughput in task/ms
	   /// \returns		WorkItem throughput for the locality
           double get_throughput();

           HPX_DEFINE_COMPONENT_ACTION(monitor, get_throughput);
           double get_throughput_remote(hpx::id_type locality);

           /// \brief This function returns the weighted task throughput in task/ms
           //           /// \returns         Weighted task throughput for the locality
           double get_weighted_throughput();

           /// \brief This function returns the memory consumed by the locality
           //           /// \returns         Memory in Kb consumed by the locality
           std::uint64_t get_consumed_memory();

           /// \brief This function returns the bytes sent via network by the locality
           //           /// \returns   Data sent in bytes by the locality
           std::uint64_t get_network_out();

           /// \brief This function returns the bytes received via network by the locality
           //           /// \returns   Data received in bytes by the locality
           std::uint64_t get_network_in();

           /// \brief This function returns the current frequency for the CPU cpuid
           //            /// \returns  Current frequency of cpuid 
	   std::uint64_t get_current_freq(int cpuid);

   
           /// \brief This function returns the min frequency for the CPU cpuid
           //            /// \returns  Min frequency of cpuid 
           std::uint64_t get_min_freq(int cpuid);


           /// \brief This function returns the max frequency for the CPU cpuid
           //            /// \returns  Max frequency of cpuid 
           std::uint64_t get_max_freq(int cpuid);


           /// \brief This function returns the available frequencies for the CPU cpuid
           //            /// \returns  Available frequencies for cpuid 
	   std::vector<std::uint64_t> get_available_freqs(int cpuid);


           // Functions related to system/node metrics

           /// \brief This function returns the total memory of a node
           //           /// \returns         Total node memory
           unsigned long long get_node_total_memory();


           /// \brief This function returns the number of cpus of a node
           //           //           /// \returns     Total number of cpus
           int get_num_cpus();


           /// \brief This function returns the cpu load of the node
           //           //           /// \returns      Cpu load
           float get_cpu_load();


           private:

             // MONITOR MANAGEMENT
             // Measuring total execution time
	     std::chrono::steady_clock::time_point execution_start;
	     std::chrono::steady_clock::time_point execution_end;
	     double wall_clock;
             std::uint64_t rank_, execution_init;

             std::uint64_t num_localities_;
             bool enable_monitor;

             // System parameters
             unsigned long long total_memory_;

             int num_cpus_;
             std::map<int, std::vector<std::uint64_t>> cpu_frequencies;
             void get_cpus_info();


             // Monitor neighbours
//             hpx::id_type left_;
//             hpx::id_type right_;

             typedef hpx::lcos::local::spinlock mutex_type;
             mutex_type work_map_mutex;

             // Performance profiles per work item ID
	     std::unordered_map<std::string, std::shared_ptr<allscale::profile>> profiles;

	     // Performance data per work item name
//	     std::unordered_map<std::string, std::shared_ptr<allscale::work_item_stats>> work_item_stats_map;

             // Work item times in "chronological" order to compute statistics on the X last work items
             std::vector<double> work_item_times;
	     mutex_type work_items_vector;

	     // For graph creation
//	     std::unordered_map<std::string, std::string> w_names; // Maps work_item name and ID for nice graph node labelling
                                                                   // Also serves as a list of nodes
//             std::list <std::shared_ptr<allscale::work_item_dependency>> w_graph;

             // For graph creation from a specific node
             std::unordered_map<std::string, std::vector <std::string>> wi_dependencies;

             // Update work item stats. Can be called from the start or the finish wrapper
             void update_work_item_stats(work_item const& w, std::shared_ptr<allscale::profile> p);

             // Queue of profiles to be processed by the specialised OS-thread
             std::thread worker_thread;
             std::queue<std::shared_ptr<allscale::profile>> queues[2];
             int current_read_queue, current_write_queue;
	     std::mutex m_queue;
             std::condition_variable cv;
             bool ready;
             bool done;
             void process_profiles();

	     // WorkItem stats, not used right now
             double total_split_time, total_process_time;
             long long num_split_tasks, num_process_tasks;
             double min_split_task, max_split_task;
	     double min_process_task, max_process_task;


	     // SAMPLED METRICS
	     std::mutex sampling_mutex;
             std::unique_ptr<hpx::util::interval_timer> metric_sampler_;
	     long long finished_tasks;
             long long sampling_interval_ms;
             double task_throughput;
             std::vector<double> throughput_history;


             // weighted task throughput
             double weighted_sum;
             double weighted_throughput;

	     bool sample_node();

             // Idle rate
             hpx::id_type idle_rate_counter_;
             int rate_counter_registered_;
             double idle_rate_;
	     std::vector<double> idle_rate_history;

	     // Memory consumed
             hpx::id_type resident_memory_counter_;
             int memory_counter_registered_;
             std::uint64_t resident_memory_;

             // Bytes sent/recv  (network)
             hpx::id_type nsend_mpi_counter_;
             hpx::id_type nsend_tcp_counter_;
             hpx::id_type nrecv_mpi_counter_;
             hpx::id_type nrecv_tcp_counter_;
             int network_mpi_counters_registered_;
             int network_tcp_counters_registered_;

             std::uint64_t bytes_sent_;
             std::uint64_t bytes_recv_;

             // CPU load
             std::ifstream pstat;
             std::uint64_t last_user_time, last_nice_time, last_system_time;
             std::uint64_t last_idle_time;
             float cpu_load_;


             void print_heatmap(const char *file_name, std::vector<std::vector<double>> &buffer);

//             hpx::id_type idle_rate_avg_counter_;
//             double idle_rate_avg_;

#ifdef REALTIME_VIZ
             // REALTIME VIZ
	     std::mutex counter_mutex_;
	     std::uint64_t num_active_tasks_;
	     std::uint64_t total_tasks_;
	     double total_task_duration_;

             hpx::id_type idle_rate_counter_;
             double idle_rate_;

	     int realtime_viz;
             hpx::util::interval_timer timer_;

	     std::ofstream data_file;
	     unsigned long long int sample_id_;

             bool sample_task_stats();
             double get_avg_task_duration();
#endif

             // HISTORICAL DATA
             mutex_type history_mutex;
	     std::shared_ptr<allscale::historical_data> history;
             std::string match_previous_treeture(std::string const& w_ID);

             // PRINTING TREES
             void print_node(std::ofstream& myfile, std::string node, double total_tree_time,
                                profile_map& global_stats, dependency_graph& g);
             void print_edges(std::ofstream& myfile, std::string node, profile_map& global_stats, dependency_graph& g);
             void print_treeture(std::string filename, std::string root, double total_tree_time,
                                profile_map& global_stats, dependency_graph& g);
             void print_trees_per_iteration();
             void monitor_component_output(std::unordered_map<std::string, allscale::work_item_stats>& s);


#ifdef HAVE_PAPI
             // PAPI counters per thread
//             std::multimap<std::uint32_t, hpx::performance_counters::performance_counter> counters;
             std::multimap<std::uint32_t, hpx::id_type> counters;
             std::vector<std::string> papi_counter_names;
	     void monitor_papi_output();
#endif
             // ENV. VARS
	     int output_profile_table_;
	     int output_treeture_;
	     int output_iteration_trees_;
             int collect_papi_;
             long cutoff_level_;
	     int print_throughput_hm_;
             int print_idle_hm_;
       };

}}
//HPX_REGISTER_ACTION_DECLARATION(allscale::components::monitor::get_my_rank_action, get_my_rank_action);
//HPX_REGISTER_ACTION_DECLARATION(allscale::components::monitor::stop_action, stop_action);
HPX_REGISTER_ACTION_DECLARATION(allscale::components::monitor::get_exclusive_time_action, get_exclusive_time_action);
HPX_REGISTER_ACTION_DECLARATION(allscale::components::monitor::get_inclusive_time_action, get_inclusive_time_action);
HPX_REGISTER_ACTION_DECLARATION(allscale::components::monitor::get_average_exclusive_time_action, get_average_exclusive_time_action);
HPX_REGISTER_ACTION_DECLARATION(allscale::components::monitor::get_minimum_exclusive_time_action, get_minimum_exclusive_time_action);
HPX_REGISTER_ACTION_DECLARATION(allscale::components::monitor::get_maximum_exclusive_time_action, get_maximum_exclusive_time_action);
HPX_REGISTER_ACTION_DECLARATION(allscale::components::monitor::get_children_mean_time_action, get_children_mean_time_action);
HPX_REGISTER_ACTION_DECLARATION(allscale::components::monitor::get_children_SD_time_action, get_children_SD_time_action);
HPX_REGISTER_ACTION_DECLARATION(allscale::components::monitor::get_idle_rate_action, get_idle_rate_action);
//HPX_REGISTER_ACTION_DECLARATION(allscale::components::monitor::get_avg_idle_rate_action, get_avg_idle_rate_action);
HPX_REGISTER_ACTION_DECLARATION(allscale::components::monitor::get_throughput_action, get_throughput_action);
#endif
