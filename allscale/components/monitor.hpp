
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


#ifdef REALTIME_VIZ
#include <hpx/util/interval_timer.hpp>
//#include <iostream>
//#include <fstream>
#endif
#ifdef HAVE_PAPI
#include <hpx/include/performance_counters.hpp>

#define MAX_PAPI_COUNTERS 4
#endif

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

           std::uint64_t get_rank() { return rank_; }
//         HPX_DEFINE_COMPONENT_ACTION(monitor, get_rank);
           std::uint64_t get_rank_remote(hpx::id_type locality);

           // Historical Data Introspection
           double get_iteration_time(int i);
           double get_last_iteration_time();
	   long get_number_of_iterations();
	   double get_avg_time_last_iterations(std::uint32_t num_iters);

	   // Performance Introspection
           double get_exclusive_time(std::string w_id);
//	   HPX_DEFINE_COMPONENT_ACTION(monitor, get_exclusive_time);

           double get_inclusive_time(std::string w_id);
//           HPX_DEFINE_COMPONENT_ACTION(monitor, get_inclusive_time);

           double get_average_exclusive_time(std::string w_name);
//           HPX_DEFINE_COMPONENT_ACTION(monitor, get_average_exclusive_time);

           double get_minimum_exclusive_time(std::string w_name);
//           HPX_DEFINE_COMPONENT_ACTION(monitor, get_minimum_exclusive_time);

           double get_maximum_exclusive_time(std::string w_name);
//           HPX_DEFINE_COMPONENT_ACTION(monitor, get_maximum_exclusive_time);

           double get_children_mean_time(std::string w_id);
//           HPX_DEFINE_COMPONENT_ACTION(monitor, get_children_mean_time);

           double get_children_SD_time(std::string w_id);
//           HPX_DEFINE_COMPONENT_ACTION(monitor, get_children_mean_SD_time);

	   // Average execution time for the last num_work_items
	   double get_avg_work_item_times(std::uint32_t num_work_items);
	
           // Standard deviation in the execution time for the last num_work_items
	   double get_SD_work_item_times(std::uint32_t num_work_items);

           // Inter-node introspection (synchronous wrappers for remote actions)
           double get_exclusive_time_remote(hpx::id_type locality, std::string w_id);
           double get_inclusive_time_remote(hpx::id_type locality, std::string w_id);
           double get_average_exclusive_time_remote(hpx::id_type locality, std::string w_name);
           double get_minimum_exclusive_time_remote(hpx::id_type locality, std::string w_name);
           double get_maximum_exclusive_time_remote(hpx::id_type locality, std::string w_name);
           double get_children_mean_time_remote(hpx::id_type locality, std::string w_id);
           double get_children_SD_time_remote(hpx::id_type locality, std::string w_id);

#ifdef HAVE_PAPI
	   // PAPI counters
	   long long *get_papi_counters(std::string w_id);
#endif

           // Wrapping functions to signals from the runtime
           static void global_w_exec_start_wrapper(work_item const& w);
           void w_exec_start_wrapper(work_item const& w);

           static void global_w_exec_finish_wrapper(work_item const& w);
           void w_exec_finish_wrapper(work_item const& w);

           static void global_w_result_propagated_wrapper(work_item const& w);
           void w_result_propagated_wrapper(work_item const& w);

           static void global_w_app_iteration(allscale::work_item const& w);
           void w_app_iteration(allscale::work_item const& w);

           static void global_finalize();
           void monitor_component_finalize();

           private:

             // MONITOR MANAGEMENT 
             // Measuring total execution time
	     std::chrono::steady_clock::time_point execution_start;
	     std::chrono::steady_clock::time_point execution_end;
	     double wall_clock;
             std::uint64_t rank_;

             typedef hpx::lcos::local::spinlock mutex_type;
             mutex_type work_map_mutex;

             // Performance profiles per work item ID
	     std::unordered_map<std::string, std::shared_ptr<allscale::profile>> profiles;

	     // Performance data per work item name
	     std::unordered_map<std::string, std::shared_ptr<allscale::work_item_stats>> work_item_stats_map;

             // Work item times in "chronological" order to compute statistics on the X last work items
             std::vector<double> work_item_times;
	     mutex_type work_items_vector;

	     // For graph creation
	     std::unordered_map<std::string, std::string> w_names; // Maps work_item name and ID for nice graph node labelling
                                                                   // Also serves as a list of nodes
             std::list <std::shared_ptr<allscale::work_item_dependency>> w_graph;

             // For graph creation from a specific node
             std::unordered_map<std::string, std::vector <std::string>> graph;

             // Update work item stats. Can be called from the start or the finish wrapper
             void update_work_item_stats(work_item const& w, std::shared_ptr<allscale::profile> p);



#ifdef REALTIME_VIZ
             // REALTIME VIZ
	     std::mutex counter_mutex_;
	     std::uint64_t num_active_tasks_;
	     std::uint64_t total_tasks_;
	     double total_task_duration_;

             hpx::id_type idle_rate_counter_;
             double idle_rate_;
             hpx::id_type resident_memory_counter_;
             double resident_memory_;

	     int realtime_viz;
             hpx::util::interval_timer timer_;

	     std::ofstream data_file;
	     unsigned long long int sample_id_;

             bool sample_task_stats();
             double get_avg_task_duration();
#endif

             // HISTORICAL DATA
	     std::shared_ptr<allscale::historical_data> history;
             std::string match_previous_treeture(std::string const& w_ID);

             // PRINTING TREES
             void print_node(std::ofstream& myfile, std::string node, double total_tree_time);
             void print_edges(std::ofstream& myfile, std::string node);
             void print_treeture(std::string filename, std::string root, double total_tree_time);
             void print_trees_per_iteration();
             void create_work_item_graph();
             void monitor_component_output();
              

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
       };

}}

#endif
