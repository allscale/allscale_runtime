#include <allscale/work_item.hpp>
#include <allscale/components/monitor.hpp>
#include <allscale/monitor.hpp>
#include <allscale/work_item_dependency.hpp>
#include <allscale/profile.hpp>
#include <allscale/work_item_stats.hpp>
#include <allscale/util/graph_colouring.hpp>
#include <allscale/historical_data.hpp>

#include <hpx/include/lcos.hpp>

#include <unistd.h>
#include <unordered_map>
#include <iostream>
#include <fstream>
#include <sstream>
#include <mutex>
#include <memory>
#include <vector>
#include <stdlib.h>

// Performance profiles per work item ID
std::unordered_map<std::string, std::shared_ptr<allscale::profile>> profiles;

// Performance data per work item name
std::unordered_map<std::string, std::shared_ptr<allscale::work_item_stats>> work_item_stats_map;  

typedef hpx::lcos::local::spinlock mutex_type;
mutex_type work_map_mutex;

// For graph creation
std::unordered_map<std::string, std::string> w_names; // Maps work_item name and ID for nice graph node labelling
						      // Also serves as a list of nodes
std::list <std::shared_ptr<allscale::work_item_dependency>> w_graph;

// For graph creation from a specific node
std::unordered_map<std::string, std::vector <std::string>> graph;

// Measuring total execution time
std::chrono::steady_clock::time_point execution_start;
std::chrono::steady_clock::time_point execution_end;
double wall_clock;

// Historial data
std::shared_ptr<allscale::historical_data> history;

// Env. vars
int output_treeture = 0;
int output_iteration_trees = 0;
   
namespace allscale { namespace components {


   void w_exec_start_wrapper(allscale::work_item const& w)
   {
      allscale::this_work_item::id my_wid = w.id();
      std::shared_ptr<allscale::profile> p(new profile());
      std::shared_ptr<allscale::work_item_dependency> wd(
          new allscale::work_item_dependency(my_wid.parent().name(), my_wid.name()));


      std::lock_guard<mutex_type> lock(work_map_mutex);
      w_names.insert(std::make_pair(my_wid.name(), w.name()));
      profiles.insert(std::make_pair(my_wid.name(), p));
      w_graph.push_back(wd);

      auto it = graph.find(my_wid.parent().name());
      if( it == graph.end() ) {
	 graph.insert(std::make_pair(my_wid.parent().name(), 
			 std::vector<std::string>(1, my_wid.name())));
      }
      else it->second.push_back(my_wid.name());
/*
      std::cout
          << "Start work item "
          << w.name()
          << " "
          << my_wid.name()
          << " my parent "
          << my_wid.parent().name()
          << std::endl;
*/
   }

   void w_exec_finish_wrapper(allscale::work_item const& w)
   {
      allscale::this_work_item::id my_wid = w.id();
      std::shared_ptr<allscale::profile> p;
      std::shared_ptr<allscale::work_item_stats> stats;
      double time; 

      std::lock_guard<mutex_type> lock(work_map_mutex);
      p = profiles[my_wid.name()];

      p->end = std::chrono::steady_clock::now();
      
     
      // Update stats per work item name
      time = p->get_exclusive_time();
      auto it = work_item_stats_map.find(w.name());
      if( it == work_item_stats_map.end() )
      {
	 stats.reset(new allscale::work_item_stats());
	 work_item_stats_map.insert(std::make_pair(w.name(), stats)); 
      } 
      else
	stats = it->second;

      // Compute max per work item with same name
      if( stats->max < time)
	 stats->max = time;
	    
      // Compute min per work item with same name
      if( !(stats->min) || stats->min > time)
         stats->min = time;        

      // Compute total time per work item with same name 
      stats->total += time;

      // Number of work items with that same name
      stats->num_work_items++;


      // Update global stats about children in the parent node
      // Compute streamed mean and stdev
      // see http://www.johndcook.com/standard_deviation.html
      // or Donald Knuth's Art of Computer Programming, Vol 2, page 232, 3rd edition


      // Find parent
      // We assume here work item 0 is in the map
      std::string parent_ID = my_wid.parent().name();
      p = profiles[parent_ID];

      p->push(time);


/*
      std::cout
          << "Finish work item "
          << w.name()
          << " " << my_wid.name()
          << std::endl;
*/

   }



   void w_result_propagated_wrapper(allscale::work_item const& w)
   {
      allscale::this_work_item::id my_wid = w.id();
      std::shared_ptr<allscale::profile> p;

      std::lock_guard<mutex_type> lock(work_map_mutex);
      p = profiles[my_wid.name()];

      p->result_ready = std::chrono::steady_clock::now();


/*
      std::cout
          << "Result propagated work item "
          << w.name()
          << " " << my_wid.name()
          << std::endl;
*/
   }


   // Signal for new iteration
   void w_app_iteration(allscale::work_item const& w)
   {
      if(history->current_iteration < 0) {
//	 history = std::make_shared<allscale::historical_data>();
	 history->last_iteration_start = std::chrono::steady_clock::now();
	 history->add_tree_root(w.id().name());
	 history->current_iteration++;
      }
      else
         history->new_iteration(w.id().name());

   }


   // Returns the exclusive time for a work item with ID w_id
   double get_exclusive_time(std::string w_id)
   {
      std::lock_guard<mutex_type> lock(work_map_mutex);
      std::unordered_map<std::string, std::shared_ptr<allscale::profile>>::const_iterator it = profiles.find(w_id);

      if( it == profiles.end() )
         return 0.0;
      else
         return (it->second)->get_exclusive_time();
   }


   // Returns the inclusive time for a work item with ID w_id
   double get_inclusive_time(std::string w_id)
   {
      std::lock_guard<mutex_type> lock(work_map_mutex);
      std::unordered_map<std::string, std::shared_ptr<allscale::profile>>::const_iterator it = profiles.find(w_id);

      if( it == profiles.end() )
         return 0.0;
      else
         return (it->second)->get_inclusive_time();
   }

   // Returns the average exclusive time for a work item with name w_name
   double get_average_exclusive_time(std::string w_name)
   {
      std::lock_guard<mutex_type> lock(work_map_mutex);
      std::unordered_map<std::string, std::shared_ptr<allscale::work_item_stats>>::const_iterator it = 
	  work_item_stats_map.find(w_name);

      if( it == work_item_stats_map.end() )
         return 0.0;
      else
         return (it->second)->get_average();
   }

   // Returns the minimum exclusive time for a work item with name w_name
   double get_minimum_exclusive_time(std::string w_name)
   {
      std::lock_guard<mutex_type> lock(work_map_mutex);
      std::unordered_map<std::string, std::shared_ptr<allscale::work_item_stats>>::const_iterator it = 
          work_item_stats_map.find(w_name);

      if( it == work_item_stats_map.end() )
         return 0.0;
      else
         return (it->second)->get_min();

   }


   // Returns the maximum exclusive time for a work item with name w_name
   double get_maximum_exclusive_time(std::string w_name)
   {
      std::lock_guard<mutex_type> lock(work_map_mutex);
      std::unordered_map<std::string, std::shared_ptr<allscale::work_item_stats>>::const_iterator it = 
          work_item_stats_map.find(w_name);

      if( it == work_item_stats_map.end() )
         return 0.0;
      else
         return (it->second)->get_max();

   }


   // Returns the mean exclusive time for all children
   double get_children_mean_time(std::string w_id)
   {
      std::lock_guard<mutex_type> lock(work_map_mutex);
      std::unordered_map<std::string, std::shared_ptr<allscale::profile>>::const_iterator it = profiles.find(w_id);

      if( it == profiles.end() )
         return 0.0;
      else
         return (it->second)->Mean();
   }


   // Returns the exclusive time standard deviation for all children
   double get_children_SD_time(std::string w_id)
   {
      std::lock_guard<mutex_type> lock(work_map_mutex);
      std::unordered_map<std::string, std::shared_ptr<allscale::profile>>::const_iterator it = profiles.find(w_id);

      if( it == profiles.end() )
         return 0.0;
      else
         return (it->second)->StandardDeviation();
   }

   // Returns PAPI counters for a work item with ID w_id
   long long *get_papi_counters(std::string w_id)
   {
#ifdef HAVE_PAPI
      std::shared_ptr<allscale::profile> p;

      std::lock_guard<mutex_type> lock(work_map_mutex);
      std::unordered_map<std::string, std::shared_ptr<allscale::profile>> const_iterator it = profiles.find(w_id);

      if( it == profiles.end() )
         return NULL;
      else
         return (it->second)->get_counters();
#else
      return NULL;
#endif
   }


   // Returns the PAPI counter with name c_name for the work item 
   // with ID w_id
   double get_papi_counter(std::string w_id, std::string c_name)
   {
#ifdef HAVE_PAPI

#else
      return 0.0;
#endif
   }


   // Translates a work ID changing it prefix with the last iteration root 
   // Returns the same label for work items of the first iteration
   std::string match_previous_treeture(std::string const& w_ID)
   {
      std::string match = w_ID;
      int last_iteration = history->current_iteration - 1;

      if(last_iteration >= 0) { 
         std::string previous_root = history->iteration_roots[last_iteration];

         match.replace(0, previous_root.size(), previous_root);
      }

      return match;
   }


   void print_node(std::ofstream& myfile, std::string node, double total_tree_time)
   {
      std::string label;
      double excl_elapsed, incl_elapsed;

      auto profile_it = profiles.find(node);

      excl_elapsed = (profile_it->second)->get_exclusive_time();
      incl_elapsed = (profile_it->second)->get_inclusive_time();

      std::stringstream stream;
      stream << std::scientific << std::setprecision(3) << excl_elapsed;
      label += std::string("\\n Texcl = ") + stream.str();
      // Clear the string
      stream.str(std::string());
      stream << std::scientific << std::setprecision(3) << incl_elapsed;
      label += std::string("\\n Tincl = ") + stream.str();

      unsigned color = allscale::components::util::intensity_to_rgb(excl_elapsed, total_tree_time);
      auto it = w_names.find(profile_it->first);

      myfile << "  \"" << profile_it->first << " " << it->second
            << label <<"\"     [ fillcolor=\"#" << std::hex << color << "\" ]" << std::endl;

      // Traverse children recursively
      for( auto children = graph[node].begin(); children != graph[node].end(); ++children )
     	  print_node(myfile, *children, total_tree_time);

      return; 

   }

   void print_edges(std::ofstream& myfile, std::string node)
   {
      double excl_elapsed, incl_elapsed;

      auto it = graph.find(node);
      // Tree leaf
      if( it == graph.end() ) return;

      for(auto child = (it->second).begin(); child != (it->second).end(); ++child) {
          std::string label1, label2;

          auto it = w_names.find(node);
          if(it != w_names.end()) label1 = node + ' ' + it->second;
          else label1 = node;

          auto it2 = profiles.find(node);
          if(it2 != profiles.end()) {
                excl_elapsed = (it2->second)->get_exclusive_time();
                incl_elapsed = (it2->second)->get_inclusive_time();

                std::stringstream stream;
                stream << std::scientific << std::setprecision(3) << excl_elapsed;
                label1 += std::string("\\n Texcl = ") + stream.str();
                // Clear the string
                stream.str(std::string());
                stream << std::scientific << std::setprecision(3) << incl_elapsed;
                label1 += std::string("\\n Tincl = ") + stream.str();

          }

          it = w_names.find(*child);
          if(it != w_names.end()) label2 = (*child) + ' ' + it->second;
          else label2 = (*child);

          it2 = profiles.find(*child);
          if(it2 != profiles.end()) {
                excl_elapsed = (it2->second)->get_exclusive_time();
                incl_elapsed = (it2->second)->get_inclusive_time();

                std::stringstream stream;
                stream << std::scientific << std::setprecision(3) << excl_elapsed;
                label2 += std::string("\\n Texcl = ") + stream.str();
                // Clear the string
                stream.str(std::string());
                stream << std::scientific << std::setprecision(3) << incl_elapsed;
                label2 += std::string("\\n Tincl = ") + stream.str();

          }

          myfile << "  \"" << label1 << "\" -> \"" << label2 << "\"" << std::endl;

          print_edges(myfile, *child);
      }
   }


   void print_treeture(std::string filename, std::string root, double total_tree_time)
   {
      double excl_elapsed, incl_elapsed;
      std::ofstream myfile;
      
      myfile.open(filename.c_str());

      // Init colour palette for graph colouring
      allscale::components::util::init_gradient_HSV_color();

      // Print the graph
      myfile << "digraph {\n";

      // Print graph attributes
      myfile << "node [\n"
             << "fillcolor=white,\n"
             << "fontsize=11,\n"
             << "shape=box,\n"
             << "style=filled ];\n\n";

      // First print node attributes (colour)
      print_node(myfile, root, total_tree_time);


      // Print Edges
      print_edges(myfile, root);
/*
      for(auto w_dep = w_graph.begin(); w_dep != w_graph.end(); ++w_dep) {
          std::string label1, label2;

          auto it = w_names.find((*w_dep)->parent);
          if(it != w_names.end()) label1 = (*w_dep)->parent + ' ' + it->second;
          else label1 = (*w_dep)->parent;

          auto it2 = profiles.find((*w_dep)->parent);
          if(it2 != profiles.end()) {
                excl_elapsed = (it2->second)->get_exclusive_time();
                incl_elapsed = (it2->second)->get_inclusive_time();

                std::stringstream stream;
                stream << std::scientific << std::setprecision(3) << excl_elapsed;
                label1 += std::string("\\n Texcl = ") + stream.str();
                // Clear the string
                stream.str(std::string());
                stream << std::scientific << std::setprecision(3) << incl_elapsed;
                label1 += std::string("\\n Tincl = ") + stream.str();

          }

          it = w_names.find((*w_dep)->child);
          if(it != w_names.end()) label2 = (*w_dep)->child + ' ' + it->second;
          else label2 = (*w_dep)->child;

          it2 = profiles.find((*w_dep)->child);
          if(it2 != profiles.end()) {
                excl_elapsed = (it2->second)->get_exclusive_time();
                incl_elapsed = (it2->second)->get_inclusive_time();

                std::stringstream stream;
                stream << std::scientific << std::setprecision(3) << excl_elapsed;
                label2 += std::string("\\n Texcl = ") + stream.str();
                // Clear the string
                stream.str(std::string());
                stream << std::scientific << std::setprecision(3) << incl_elapsed;
                label2 += std::string("\\n Tincl = ") + stream.str();

          }

          myfile << "  \"" << label1 << "\" -> \"" << label2 << "\"" << std::endl;
      }
*/

      myfile << "}\n";


      myfile.close();
   }

   void print_trees_per_iteration() 
   {
      std::string filename;

      // We don't print the last iteration cause it's a fake one created from doing new_iteration() in monitor_finalize() 
      // to have some timing for the real last iteration. Notice this time for the real last 
      // iteration includes all the execution until the end of the program though
      std::vector<std::string>::const_iterator it1 = history->iteration_roots.begin();
      std::vector<double>::const_iterator it2 = history->iteration_time.begin();
      int iter_number = 0;
      for(; it1 != history->iteration_roots.end() && it2 != history->iteration_time.end(); ++it1, ++it2) {
         
         filename = "treeture.ite.";
         filename += std::to_string(iter_number);
         filename += ".dot";

	 print_treeture(filename, *it1, *it2);
	 iter_number++;
      }
   }

   // Create task graph
   // TODO make it shorter and more structured
   void create_work_item_graph() {
//      std::lock_guard<mutex_type> lock(work_map_mutex);
      double excl_elapsed, incl_elapsed;
      std::ofstream myfile;
      myfile.open("treeture.dot");

      // Init colour palette for graph colouring
      allscale::components::util::init_gradient_HSV_color();

      // Print the graph
      myfile << "digraph {\n";

      // Print graph attributes
      myfile << "node [\n"
             << "fillcolor=white,\n"
             << "fontsize=11,\n"
             << "shape=box,\n"
             << "style=filled ];\n\n";

      // First print node attributes (colour)
      for(auto nodes = profiles.begin(); nodes != profiles.end(); ++nodes) {
         std::string label;
	 excl_elapsed = (nodes->second)->get_exclusive_time();
         incl_elapsed = (nodes->second)->get_inclusive_time();

         std::stringstream stream;
         stream << std::scientific << std::setprecision(3) << excl_elapsed;
         label += std::string("\\n Texcl = ") + stream.str();
         // Clear the string
         stream.str(std::string());
         stream << std::scientific << std::setprecision(3) << incl_elapsed;
         label += std::string("\\n Tincl = ") + stream.str();

         unsigned color = allscale::components::util::intensity_to_rgb(excl_elapsed, wall_clock);
         auto it = w_names.find(nodes->first);

         myfile << "  \"" << nodes->first << " " << it->second
                << label <<"\"     [ fillcolor=\"#" << std::hex << color << "\" ]" << std::endl;
      }

      // Print Edges
      for(auto w_dep = w_graph.begin(); w_dep != w_graph.end(); ++w_dep) {
          std::string label1, label2;

          auto it = w_names.find((*w_dep)->parent);
          if(it != w_names.end()) label1 = (*w_dep)->parent + ' ' + it->second;
          else label1 = (*w_dep)->parent;

          auto it2 = profiles.find((*w_dep)->parent);
          if(it2 != profiles.end()) {
          	excl_elapsed = (it2->second)->get_exclusive_time();
          	incl_elapsed = (it2->second)->get_inclusive_time();

		std::stringstream stream;
		stream << std::scientific << std::setprecision(3) << excl_elapsed;
		label1 += std::string("\\n Texcl = ") + stream.str();
                // Clear the string
		stream.str(std::string());
                stream << std::scientific << std::setprecision(3) << incl_elapsed;
                label1 += std::string("\\n Tincl = ") + stream.str();

          }

          it = w_names.find((*w_dep)->child);
          if(it != w_names.end()) label2 = (*w_dep)->child + ' ' + it->second;
          else label2 = (*w_dep)->child;

          it2 = profiles.find((*w_dep)->child);
          if(it2 != profiles.end()) {
                excl_elapsed = (it2->second)->get_exclusive_time();
                incl_elapsed = (it2->second)->get_inclusive_time();

                std::stringstream stream;
                stream << std::scientific << std::setprecision(3) << excl_elapsed;
                label2 += std::string("\\n Texcl = ") + stream.str();
                // Clear the string
                stream.str(std::string());
                stream << std::scientific << std::setprecision(3) << incl_elapsed;
                label2 += std::string("\\n Tincl = ") + stream.str();

          }

          myfile << "  \"" << label1 << "\" -> \"" << label2 << "\"" << std::endl;
      }

      myfile << "}\n";
      w_graph.clear();
      myfile.close();
   }


   void monitor_component_output() {
//      std::lock_guard<mutex_type> lock(work_map_mutex);


      std::cerr << "\nWall-clock time: " << wall_clock << std::endl;
      std::cerr << "\nWork Item		Exclusive time  |  % Total  |  Inclusive time  |  % Total   |   Mean (child.)  |   SD (child.)  "
                << "\n------------------------------------------------------------------------------------------------------------------\n";

      std::vector<std::string> w_id;
      // sort work_item names
      for(auto it : profiles) {
         std::string name = it.first;
         w_id.push_back(name);
         }

      std::sort(w_id.begin(), w_id.end());
      // iterate over the profiles
      for(std::string i : w_id) {
          std::shared_ptr<profile> p = profiles[i];
        if (p) {
            double excl_elapsed = p->get_exclusive_time();
            double perc_excl_elapsed = (excl_elapsed/wall_clock) * 100;
            double incl_elapsed = p->get_inclusive_time();
            double perc_incl_elapsed = (incl_elapsed/wall_clock) * 100;

            std::cerr.precision(5);
            std::cerr << std::scientific << "   " << i + ' ' + w_names[i] << "\t\t " << excl_elapsed << "\t\t";
            std::cerr.precision(2);
            std::cerr << std::fixed << perc_excl_elapsed;
            std::cerr.precision(5);
            std::cerr << std::scientific << "\t" << incl_elapsed << "\t\t";
            std::cerr.precision(2);
            std::cerr << std::fixed << perc_incl_elapsed << "\t";
            std::cerr.precision(5);
            std::cerr << std::fixed << p->Mean() << "       " << p->StandardDeviation() << std::endl;
        }
      }
      w_id.clear();
    }



   void monitor_component_finalize() {

      execution_end = std::chrono::steady_clock::now();

      std::chrono::duration<double> total_time_elapsed =
          std::chrono::duration_cast<std::chrono::duration<double>>(execution_end - execution_start);
      wall_clock = total_time_elapsed.count();

      // Finish time for "main" work item
//      std::lock_guard<mutex_type> lock(work_map_mutex);
      std::string mystring("0");
      std::shared_ptr<allscale::profile> p = profiles[mystring];
      p->end = execution_end;
      p->result_ready = execution_end;


      // Dump profile reports and graphs
      monitor_component_output();

      if(output_treeture)
         create_work_item_graph();

      if(output_iteration_trees) {
         history->new_iteration(std::string("foo"));
         print_trees_per_iteration();
      }
   }


   void monitor_component_init() {

      allscale::monitor::connect(allscale::monitor::work_item_execution_started, w_exec_start_wrapper);
      allscale::monitor::connect(allscale::monitor::work_item_execution_finished, w_exec_finish_wrapper);
      allscale::monitor::connect(allscale::monitor::work_item_result_propagated, w_result_propagated_wrapper);
      allscale::monitor::connect(allscale::monitor::work_item_first, w_app_iteration);

      const int result = std::atexit(monitor_component_finalize);
      if(result != 0) std::cerr << "Registration of monitor_finalize function failed!" << std::endl;

      // Check environment variables
      if(const char* env_p = std::getenv("PRINT_TREE_ITERATIONS"))
	 output_iteration_trees = atoi(env_p);
     
      if(const char* env_p = std::getenv("PRINT_TREETURE"))
         output_treeture = atoi(env_p);

      execution_start = std::chrono::steady_clock::now();

      // Create the profile for the "Main"
      std::shared_ptr<allscale::profile> p(new profile());
      std::string mystring("0");
      profiles.insert(std::make_pair(mystring, p));
      w_names.insert(std::make_pair(mystring, std::string("Main")));

      // Insert the root "0" in the graph
      graph.insert(std::make_pair(std::string("0"), std::vector<std::string>()));

      // Init historical data
      history = std::make_shared<allscale::historical_data>();
   }




}}
