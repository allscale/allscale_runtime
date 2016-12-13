#include <allscale/work_item.hpp>
#include <allscale/components/monitor.hpp>
#include <allscale/monitor.hpp>
#include <allscale/work_item_dependency.hpp>
#include <allscale/profile.hpp>
#include <allscale/util/graph_colouring.hpp>

#include <unistd.h>
#include <unordered_map>
#include <iostream>
#include <fstream>
#include <sstream>
#include <mutex>

// Performance profiles per work item
std::unordered_map<std::string, allscale::profile *> profiles;
std::mutex work_map_mutex;

// For graph creation
std::unordered_map<std::string, std::string> w_names; // Maps work_item name and ID for nice graph node labelling
std::list < allscale::work_item_dependency *> w_graph;

// Measuring total execution time
std::chrono::steady_clock::time_point execution_start;
std::chrono::steady_clock::time_point execution_end;
double wall_clock;

 
namespace allscale { namespace components {

   void w_exec_start_wrapper(allscale::work_item const& w)
   {
      allscale::profile *p;
      allscale::work_item_dependency *wd;
      allscale::this_work_item::id my_wid = w.id();
   
      p = new profile();
      wd = new allscale::work_item_dependency(my_wid.parent().name(), my_wid.name());

      std::lock_guard<std::mutex> lock(work_map_mutex);
      w_names.insert(std::make_pair(my_wid.name(), w.name()));
      profiles.insert(std::make_pair(my_wid.name(), p));
      w_graph.push_back(wd);
 
/*      std::cout
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
      allscale::profile *p;

      p = profiles[my_wid.name()];

      p->end = std::chrono::steady_clock::now(); 

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
      allscale::profile *p;

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

   // Get exclusive time
   double w_get_exclusive_time(std::string wid)
   {
   }


   // Get inclusive time
   double w_get_inclusive_time(std::string wid)
   {
   }


   // Get papi counters
   double * w_get_papi_counters(std::string wid)
   {

   }

   // Get papi counter 
 
   void process_profiles() {

   }

   // Create task graph
   // TODO make it shorter and more structured
   void create_work_item_graph() {
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
	  delete (*w_dep);
      }

      myfile << "}\n";
      w_graph.clear();
      myfile.close();
   }

 
   void monitor_component_output() {


      std::cout << "\nWall-clock time: " << wall_clock << std::endl;
      std::cout << "\nWork Item		Exclusive time  |  % Total  |  Inclusive time  |  % Total"		
                << "\n----------------------------------------------------------------------------------------\n";

      std::vector<std::string> w_id;
      // sort work_item names 
      for(auto it : profiles) {
         std::string name = it.first;
         w_id.push_back(name);
         }

      std::sort(w_id.begin(), w_id.end());
      // iterate over the profiles 
      for(std::string i : w_id) {
        profile * p = profiles[i];
        if (p) {
            double excl_elapsed = p->get_exclusive_time();
            double perc_excl_elapsed = (excl_elapsed/wall_clock) * 100; 
            double incl_elapsed = p->get_inclusive_time();
            double perc_incl_elapsed = (incl_elapsed/wall_clock) * 100;

            std::cout.precision(5);
            std::cout << std::scientific << "   " << i + ' ' + w_names[i] << "\t\t " << excl_elapsed << "\t\t";
            std::cout.precision(2); 
            std::cout << std::fixed << perc_excl_elapsed;
            std::cout.precision(5);
            std::cout << std::scientific << "\t" << incl_elapsed << "\t\t";
            std::cout.precision(2);
            std::cout << std::fixed << perc_incl_elapsed << std::endl;
        }
      }
      w_id.clear();
    }


 
   void monitor_component_finalize() {

      execution_end = std::chrono::steady_clock::now();

      std::chrono::duration<double> total_time_elapsed =
          std::chrono::duration_cast<std::chrono::duration<double>>(execution_end - execution_start);
      wall_clock = total_time_elapsed.count();

      monitor_component_output();
      create_work_item_graph();

   }


   void monitor_component_init() {

      allscale::monitor::event_function start_WI = w_exec_start_wrapper;
      allscale::monitor::event_function stop_WI = w_exec_finish_wrapper;

      allscale::monitor::connect(allscale::monitor::work_item_execution_started, w_exec_start_wrapper);
      allscale::monitor::connect(allscale::monitor::work_item_execution_finished, w_exec_finish_wrapper);
      allscale::monitor::connect(allscale::monitor::work_item_result_propagated, w_result_propagated_wrapper);

      const int result = std::atexit(monitor_component_finalize);
      if(result != 0) std::cerr << "Registration of monitor_finalize function failed!" << std::endl;

      execution_start = std::chrono::steady_clock::now();

   }


}}
