#include <allscale/components/monitor.hpp>
#include <allscale/monitor.hpp>

 
namespace allscale { namespace components {


   monitor::monitor(std::uint64_t rank)
     : rank_(rank)
     , output_profile_table_(0)
     , output_treeture_(0)
     , output_iteration_trees_(0)
#ifdef REALTIME_VIZ
     , num_active_tasks_(0)
     , total_tasks_(0)
     , realtime_viz(0)
     , sample_id_(0)
     , timer_(
          hpx::util::bind(
              &monitor::sample_task_stats,
              this
          ),
          10000,
          "monitor:sample_task_stats",
          true
       )
#endif
    {

    }

#ifdef REALTIME_VIZ
   bool monitor::sample_task_stats()
   {
      hpx::performance_counters::counter_value idle_value;
      hpx::performance_counters::counter_value rss_value;

      idle_value = hpx::performance_counters::stubs::performance_counter::get_value(
                hpx::launch::sync, idle_rate_counter_);
      rss_value = hpx::performance_counters::stubs::performance_counter::get_value(
                hpx::launch::sync, resident_memory_counter_);


     std::unique_lock<std::mutex> lock(counter_mutex_);

     idle_rate_ = idle_value.get_value<double>() * 0.01;
     resident_memory_ = rss_value.get_value<double>() * 1e-6; 

     data_file << sample_id_++ << "\t" << num_active_tasks_ << "\t" 
               << get_avg_task_duration() << "\t" << idle_rate_ << "\t" << resident_memory_ << std::endl;
     
//     std::cout << "Total number of tasks: " << total_tasks_ << " Number of active tasks: " << num_active_tasks_ 
//	       << "Average time per task: " << get_avg_task_duration() <<  "IDLE RATE: " << idle_rate_ << std::endl; 
     return true;
   }

 
   double monitor::get_avg_task_duration()
   {
     if(!total_tasks_) return 0.0;
     else return total_task_duration_/(double)total_tasks_;
   }

#endif


   void monitor::w_exec_start_wrapper(work_item const& w)
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

#ifdef REALTIME_VIZ
      if(realtime_viz) {
         std::unique_lock<std::mutex> lock2(counter_mutex_);
         num_active_tasks_++;
         lock2.unlock();
      }
#endif

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


   void monitor::global_w_exec_start_wrapper(work_item const& w)
   {
	(allscale::monitor::get_ptr().get())->w_exec_start_wrapper(w);
   }


   void monitor::w_exec_finish_wrapper(work_item const& w)
   {
      allscale::this_work_item::id my_wid = w.id();
      std::shared_ptr<allscale::profile> p;
      std::shared_ptr<allscale::work_item_stats> stats;
      double time; 

      std::lock_guard<mutex_type> lock(work_map_mutex);
      p = profiles[my_wid.name()];

      p->end = std::chrono::steady_clock::now();

#ifdef REALTIME_VIZ
      if(realtime_viz) {
         // Global task stats
         std::unique_lock<std::mutex> lock2(counter_mutex_);
         total_tasks_++; num_active_tasks_--;
         total_task_duration_ += p->get_exclusive_time();
         lock2.unlock();
      }
#endif

#ifdef HAVE_PAPI
      // Record PAPI counters for this work item

#endif

     
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


   void monitor::global_w_exec_finish_wrapper(work_item const& w)
   {
        (allscale::monitor::get_ptr().get())->w_exec_finish_wrapper(w);
   }


   void monitor::w_result_propagated_wrapper(allscale::work_item const& w)
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


   void monitor::global_w_result_propagated_wrapper(work_item const& w)
   {
        (allscale::monitor::get_ptr().get())->w_result_propagated_wrapper(w);
   }


   // Signal for new iteration
   void monitor::w_app_iteration(allscale::work_item const& w)
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


   void monitor::global_w_app_iteration(work_item const& w)
   {
        (allscale::monitor::get_ptr().get())->w_app_iteration(w);
   }


   // Returns the exclusive time for a work item with ID w_id
   double monitor::get_exclusive_time(std::string w_id)
   {
      std::lock_guard<mutex_type> lock(work_map_mutex);
      std::unordered_map<std::string, std::shared_ptr<allscale::profile>>::const_iterator it = profiles.find(w_id);

      if( it == profiles.end() )
         return 0.0;
      else
         return (it->second)->get_exclusive_time();
   }


   // Returns the inclusive time for a work item with ID w_id
   double monitor::get_inclusive_time(std::string w_id)
   {
      std::lock_guard<mutex_type> lock(work_map_mutex);
      std::unordered_map<std::string, std::shared_ptr<allscale::profile>>::const_iterator it = profiles.find(w_id);

      if( it == profiles.end() )
         return 0.0;
      else
         return (it->second)->get_inclusive_time();
   }

   // Returns the average exclusive time for a work item with name w_name
   double monitor::get_average_exclusive_time(std::string w_name)
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
   double monitor::get_minimum_exclusive_time(std::string w_name)
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
   double monitor::get_maximum_exclusive_time(std::string w_name)
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
   double monitor::get_children_mean_time(std::string w_id)
   {
      std::lock_guard<mutex_type> lock(work_map_mutex);
      std::unordered_map<std::string, std::shared_ptr<allscale::profile>>::const_iterator it = profiles.find(w_id);

      if( it == profiles.end() )
         return 0.0;
      else
         return (it->second)->Mean();
   }


   // Returns the exclusive time standard deviation for all children
   double monitor::get_children_SD_time(std::string w_id)
   {
      std::lock_guard<mutex_type> lock(work_map_mutex);
      std::unordered_map<std::string, std::shared_ptr<allscale::profile>>::const_iterator it = profiles.find(w_id);

      if( it == profiles.end() )
         return 0.0;
      else
         return (it->second)->StandardDeviation();
   }

#if 0
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

#endif

   double monitor::get_iteration_time(int i)
   {
      if(i < 0 || i >= history->iteration_time.size()) return 0.0;
      else return history->iteration_time[i];
   }


   double monitor::get_last_iteration_time()
   {
      if(!(history->iteration_time.size())) return 0.0;
      else return history->iteration_time.back();
   }

   long monitor::get_number_of_iterations()
   {
      return history->current_iteration;
   }


   // Translates a work ID changing it prefix with the last iteration root 
   // Returns the same label for work items of the first iteration
   std::string monitor::match_previous_treeture(std::string const& w_ID)
   {
      std::string match = w_ID;
      int last_iteration = history->current_iteration - 1;

      if(last_iteration >= 0) { 
         std::string previous_root = history->iteration_roots[last_iteration];

         match.replace(0, previous_root.size(), previous_root);
      }

      return match;
   }


   void monitor::print_node(std::ofstream& myfile, std::string node, double total_tree_time)
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

   void monitor::print_edges(std::ofstream& myfile, std::string node)
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


   void monitor::print_treeture(std::string filename, std::string root, double total_tree_time)
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

   void monitor::print_trees_per_iteration() 
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
   void monitor::create_work_item_graph() {
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


   void monitor::monitor_component_output() {
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



   void monitor::stop() {

      execution_end = std::chrono::steady_clock::now();

      std::chrono::duration<double> total_time_elapsed =
          std::chrono::duration_cast<std::chrono::duration<double>>(execution_end - execution_start);
      wall_clock = total_time_elapsed.count();

#ifdef REALTIME_VIZ
      timer_.stop();
      data_file.close();
#endif

      // Finish time for "main" work item
//      std::lock_guard<mutex_type> lock(work_map_mutex);
      std::string mystring("0");
      std::shared_ptr<allscale::profile> p = profiles[mystring];
      p->end = execution_end;
      p->result_ready = execution_end;


      // Dump profile reports and graphs
      if(output_profile_table_)
	 monitor_component_output();

      if(output_treeture_)
         create_work_item_graph();

      if(output_iteration_trees_) {
         history->new_iteration(std::string("foo"));
         print_trees_per_iteration();
      }
   }

   void monitor::global_finalize() {
//      (allscale::monitor::get_ptr().get())->monitor_component_finalize();
   }

   void monitor::init() {

      allscale::monitor::connect(allscale::monitor::work_item_execution_started, monitor::global_w_exec_start_wrapper);
      allscale::monitor::connect(allscale::monitor::work_item_execution_finished, monitor::global_w_exec_finish_wrapper);
      allscale::monitor::connect(allscale::monitor::work_item_result_propagated, monitor::global_w_result_propagated_wrapper);
      allscale::monitor::connect(allscale::monitor::work_item_first, monitor::global_w_app_iteration);

//      const int result = std::atexit(global_finalize);
//      if(result != 0) std::cerr << "Registration of monitor_finalize function failed!" << std::endl;

      // Check environment variables
      if(const char* env_p = std::getenv("PRINT_PERFORMANCE_TABLE"))
         if(atoi(env_p) == 1) output_profile_table_ = 1;

      if(const char* env_p = std::getenv("PRINT_TREE_ITERATIONS"))
         if(atoi(env_p) == 1) output_iteration_trees_ = 1;

      if(const char* env_p = std::getenv("PRINT_TREETURE"))
         if(atoi(env_p) == 1) output_treeture_ = 1;

      if(const char* env_p = std::getenv("REALTIME_VIZ"))
         if(atoi(env_p) == 1) {
#ifdef REALTIME_VIZ
               realtime_viz = 1;

	       data_file.open ("realtime_data.txt", std::ofstream::out | std::ofstream::trunc); 	

               // setup performance counter
               static const char * idle_counter_name = "/threads{locality#%d/total}/idle-rate";
               static const char * vm_counter_name = "/runtime{locality#%d/total}/memory/resident";

               const std::uint32_t prefix = hpx::get_locality_id();

               idle_rate_counter_ = hpx::performance_counters::get_counter(
                    boost::str(boost::format(idle_counter_name) % prefix));

               resident_memory_counter_ = hpx::performance_counters::get_counter(
                    boost::str(boost::format(vm_counter_name) % prefix));

               hpx::performance_counters::stubs::performance_counter::start(hpx::launch::sync, idle_rate_counter_);
               hpx::performance_counters::stubs::performance_counter::start(hpx::launch::sync, resident_memory_counter_);

               sample_task_stats();

               timer_.start();

#else
                std::cerr << "Var. REALTIME_VIZ defined but code compiled without REALTIME_VIZ support!" << std::endl;
#endif
         }


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
