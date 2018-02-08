#include <allscale/components/monitor.hpp>
#include <allscale/monitor.hpp>

#include <math.h>
#include <limits>
#include <algorithm>
#include <string>
#include <unistd.h>

#include <hpx/compat/thread.hpp>

#include <hpx/runtime/serialization/serialize.hpp>
#include <hpx/runtime/serialization/string.hpp>
#include <hpx/runtime/serialization/vector.hpp>
#include <hpx/runtime/serialization/unordered_map.hpp>
#include <hpx/runtime/shutdown_function.hpp>

#include <hpx/lcos/gather.hpp>

#ifdef HAVE_PAPI
#include <boost/tokenizer.hpp>
#include <string.h>
#endif

char const* gather_basename1 = "allscale/monitor/gather1";

HPX_REGISTER_GATHER(profile_map, profile_gatherer);

char const* gather_basename2 = "allscale/monitor/gather2";
HPX_REGISTER_GATHER(dependency_graph, dependency_gatherer);

//HPX_REGISTER_GATHER(std::vector<allscale::work_item_stats>, profile_gatherer);

using namespace std::chrono;

namespace allscale { namespace components {


   monitor::monitor(std::uint64_t rank)
     : num_localities_(hpx::get_num_localities().get())
     , rank_(rank)
     , enable_monitor(true)
     , output_profile_table_(0)
     , output_treeture_(0)
     , output_iteration_trees_(0)
     , collect_papi_(0)
     , cutoff_level_(0)
     , done(false)
     , current_read_queue(0)
     , current_write_queue(0)
//#ifdef WI_STATS
     , total_split_time(0)
     , total_process_time(0)
     , num_split_tasks(0)
     , num_process_tasks(0)
     , min_split_task(0)
     , max_split_task(0)
     , min_process_task(0)
     , max_process_task(0)
//#endif
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


   std::uint64_t monitor::get_timestamp( void ) {
      uint64_t timestamp = duration_cast<nanoseconds>(system_clock::now().time_since_epoch()).count();
      timestamp = timestamp - execution_init;
      return timestamp;
   }


   // Additional OS-thread process profiles in here
   void monitor::process_profiles()
   {
       std::shared_ptr<allscale::profile> profile;
       std::string id, parent_id;

       std::cout << "Starting additional monitoring OS-thread\n";
 
       // Wait until there's some work to do
       std::unique_lock<std::mutex> lk(m_queue);
       while(!done)
       {
          cv.wait(lk);
          current_write_queue++;
          current_write_queue = current_write_queue%2;
          lk.unlock();


          while(!queues[current_read_queue].empty()) {

             profile = queues[current_read_queue].front();

	     id = profile->get_wid();
             parent_id = profile->get_parent_wid();

             // Save the treeture graph
             // TODO Check whether this needs an extra mutex 
//             std::shared_ptr<allscale::work_item_dependency> wd(
//                 new allscale::work_item_dependency(parent_id, id));

             // Need to lock in case there's another thread accessing the the perf stats
             std::unique_lock<mutex_type> lock(work_map_mutex);

//             w_graph.push_back(wd);

       	     auto it = w_dependencies.find(parent_id);
             if( it == w_dependencies.end() ) {
                   w_dependencies.insert(std::make_pair(parent_id,
                         std::vector<std::string>(1, id)));
             }
             else it->second.push_back(id);

//             w_names.insert(std::make_pair(parent_id, id));
             profiles.insert(std::make_pair(id, profile));

	     // Save work item time in the times vector to compute stats on-the-fly if need be for the last X work items
             work_item_times.push_back(profile->get_exclusive_time());
             lock.unlock();
             queues[current_read_queue].pop(); 
          }
          lk.lock();
          current_read_queue++;
          current_read_queue = current_read_queue%2;
       }


       // Check if there are remaining profiles in the queue
       while(!queues[current_read_queue].empty())
       {
//          std::cerr << "Processing profile " << profile_queue.front()->get_wid() << std::endl;

             profile = queues[current_read_queue].front();
             id = profile->get_wid();
             parent_id = profile->get_parent_wid();

//             std::shared_ptr<allscale::work_item_dependency> wd(
//                 new allscale::work_item_dependency(parent_id, id));

             std::unique_lock<mutex_type> lock(work_map_mutex);
//             w_graph.push_back(wd);

             auto it = w_dependencies.find(parent_id);
             if( it == w_dependencies.end() ) {
                   w_dependencies.insert(std::make_pair(parent_id,
                         std::vector<std::string>(1, id)));
             }
             else it->second.push_back(id);

//             w_names.insert(std::make_pair(parent_id, id));
             profiles.insert(std::make_pair(id, profile));
             
             // Save work item time in the times vector to compute stats on-the-fly if need be for the last X work items
             work_item_times.push_back(profile->get_exclusive_time());
             
             lock.unlock();
             

             queues[current_read_queue].pop();

       }
       lk.unlock();
   }


   void monitor::update_work_item_stats(work_item const& w, std::shared_ptr<allscale::profile> p)
   {
      double time;
      std::shared_ptr<allscale::work_item_stats> stats;
      allscale::this_work_item::id my_wid = w.id();

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
      if(collect_papi_) {
         // Record PAPI counters for this work item
         hpx::performance_counters::counter_value papi_value;
         std::size_t tid = hpx::get_worker_thread_num();
//      std::multimap<std::uint32_t, hpx::performance_counters::performance_counter>::const_iterator it1, it2;
         std::multimap<std::uint32_t, hpx::id_type>::const_iterator it1, it2;
         it1 = counters.lower_bound(tid);
         it2 = counters.upper_bound(tid);

         std::uint32_t counter_num = 0;

         while(it1 != it2)
         {
             if(p->papi_counters_start[counter_num] != 0) {
	          papi_value = hpx::performance_counters::stubs::performance_counter::get_value(
			hpx::launch::sync, it1->second);

                  p->papi_counters_stop[counter_num] = papi_value.get_value<long long>();
             }
             ++it1; counter_num++;
         }
      }
#endif

#if 0
      // REMOVED DUE TO OVERHEAD ISSUES
      // Now this is computed on-demand
    
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
      // TODO What happens is parent is not registered
      std::string parent_ID = my_wid.parent().name();
      std::shared_ptr<allscale::profile> parent;
      std::unordered_map<std::string, std::shared_ptr<allscale::profile>>::const_iterator it_profiles = profiles.find(parent_ID);

      if( it_profiles == profiles.end() ) {
         // Parent is not registered
         // this child is not included in its parent statistics then
	 // TODO maybe think how to do it in the future
      }
      else {
         parent = it_profiles->second;
         parent->push(time);
      }
#endif

      // Save work item time in the times vector to compute stats on-the-fly if need be for the last X work items
      std::lock_guard<mutex_type> lock(work_items_vector);
      work_item_times.push_back(time);

   }


   void monitor::w_exec_split_start_wrapper(work_item const& w)
   {

      allscale::this_work_item::id& my_wid = const_cast<allscale::this_work_item::id&>(const_cast<work_item&>(w).id());
      std::shared_ptr<allscale::profile> p;
      bool notify_consumer = false;

//std::cerr << hpx::get_worker_thread_num() << " Start split wrapper " << my_wid.name() << "\n";

      if(( p = my_wid.get_profile()) == nullptr) {
         // Profile does not exist yet, we create it
         p = std::make_shared<profile>(my_wid.name(), w.name(), my_wid.parent().name());
         my_wid.set_profile(p);
      }      
      else {
         // Profile exists, finish wrapper executed before start wrapper
         p->end = std::chrono::steady_clock::now();

         // Put the profile in the queue for the OS-thread to process it
         { 
            std::lock_guard<std::mutex> lk(m_queue);

            queues[current_write_queue].push(p);
            if(queues[current_write_queue].size() >= MIN_QUEUE_ELEMS)
		notify_consumer = true;
         }
         if(notify_consumer) cv.notify_one();
      }

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


   void monitor::global_w_exec_split_start_wrapper(work_item const& w)
   {
        allscale::monitor::get().w_exec_split_start_wrapper(w);
   }



   void monitor::w_exec_process_start_wrapper(work_item const& w)
   {

      allscale::this_work_item::id& my_wid = const_cast<allscale::this_work_item::id&>(const_cast<work_item&>(w).id());
      std::shared_ptr<allscale::profile> p;
      bool notify_consumer = false;

//std::cerr << "Start process wrapper " << my_wid.name() << "\n";

      if(( p = my_wid.get_profile()) == nullptr) {
         // Profile does not exist yet, we create it
         p = std::make_shared<profile>(my_wid.name(), w.name(), my_wid.parent().name());
         my_wid.set_profile(p);
      }
      else {
         // Profile exists, finish wrapper executed before start wrapper
         p->end = std::chrono::steady_clock::now();

         // Put the profile in the queue for the OS-thread to process it
         {
            std::lock_guard<std::mutex> lk(m_queue);

            queues[current_write_queue].push(p);
            if(queues[current_write_queue].size() >= MIN_QUEUE_ELEMS)
                notify_consumer = true;
         }
         if(notify_consumer) cv.notify_one();
      }
   }


   void monitor::global_w_exec_process_start_wrapper(work_item const& w)
   {
        allscale::monitor::get().w_exec_process_start_wrapper(w);
   }


   void monitor::w_exec_split_finish_wrapper(work_item const& w)
   {

      allscale::this_work_item::id& my_wid = const_cast<allscale::this_work_item::id&>(const_cast<work_item&>(w).id());
//      allscale::this_work_item::id my_wid = w.id();
      std::shared_ptr<allscale::profile> p;
//      std::shared_ptr<allscale::work_item_stats> stats;
      bool notify_consumer = false;


      if(( p = my_wid.get_profile()) == nullptr) {
         // Profile does not exist yet, finish wrapper executed before than create
         p = std::make_shared<profile>(my_wid.name(), w.name(), my_wid.parent().name());
         my_wid.set_profile(p);
      }
      else {
         p->end = std::chrono::steady_clock::now();

         // Put the profile in the queue for the OS-thread to process it 
         {
           std::lock_guard<std::mutex> lk(m_queue);

           queues[current_write_queue].push(p);
           if(queues[current_write_queue].size() >= MIN_QUEUE_ELEMS)
		notify_consumer = true;
           num_split_tasks++; 
           double task_time = p->get_exclusive_time();
           total_split_time += task_time;
           if(!min_split_task || min_split_task > task_time)
               min_split_task = task_time;

           if(max_split_task <= task_time)
               max_split_task = task_time;
         }
         if(notify_consumer) cv.notify_one();
     }


/*
      std::cout
          << "Finish work item "
          << w.name()
          << " " << my_wid.name()
          << std::endl;
*/

   }


   void monitor::global_w_exec_split_finish_wrapper(work_item const& w)
   {
        allscale::monitor::get().w_exec_split_finish_wrapper(w);
   }


   void monitor::w_exec_process_finish_wrapper(work_item const& w)
   {

      allscale::this_work_item::id& my_wid = const_cast<allscale::this_work_item::id&>(const_cast<work_item&>(w).id());
//      allscale::this_work_item::id my_wid = w.id();
      std::shared_ptr<allscale::profile> p;
//      std::shared_ptr<allscale::work_item_stats> stats;
      bool notify_consumer = false;


      if(( p = my_wid.get_profile()) == nullptr) {
         // Profile does not exist yet, finish wrapper executed before than create
         p = std::make_shared<profile>(my_wid.name(), w.name(), my_wid.parent().name());
         my_wid.set_profile(p);
      }
      else {
         p->end = std::chrono::steady_clock::now();

         // Put the profile in the queue for the OS-thread to process it 
         {
           std::lock_guard<std::mutex> lk(m_queue);

           queues[current_write_queue].push(p);
           if(queues[current_write_queue].size() >= MIN_QUEUE_ELEMS)
                notify_consumer = true;
           num_process_tasks++;
           double task_time = p->get_exclusive_time();
           total_process_time += task_time;
           if(!min_process_task || min_process_task > task_time)
               min_process_task = task_time;

           if(max_process_task <= task_time)
               max_process_task = task_time;

         }
         if(notify_consumer) cv.notify_one();
     }
 
   }
   

   void monitor::global_w_exec_process_finish_wrapper(work_item const& w)
   {
        allscale::monitor::get().w_exec_process_finish_wrapper(w);
   }


   void monitor::w_result_propagated_wrapper(allscale::work_item const& w)
   {
/*
      allscale::this_work_item::id my_wid = w.id();
      std::shared_ptr<allscale::profile> p;

      std::lock_guard<mutex_type> lock(work_map_mutex);
      std::unordered_map<std::string, std::shared_ptr<allscale::profile>>::const_iterator it_profiles = profiles.find(my_wid.name());

      if( it_profiles == profiles.end() ) {
         // Work item not created yet
//         p = std::make_shared<profile>();
//         profiles.insert(std::make_pair(my_wid.name(), p));
      }
      else {
         p = it_profiles->second;
         p->result_ready = std::chrono::steady_clock::now();
      }

      std::cout
          << "Result propagated work item "
          << w.name()
          << " " << my_wid.name()
          << std::endl;
*/
   }


   void monitor::global_w_result_propagated_wrapper(work_item const& w)
   {
        allscale::monitor::get().w_result_propagated_wrapper(w);
   }


   // Signal for new iteration
   void monitor::w_app_iteration(allscale::work_item const& w)
   {
      std::lock_guard<mutex_type> lock(history_mutex);

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
        allscale::monitor::get().w_app_iteration(w);
   }


// TEST
/*   std::uint64_t monitor::get_rank_remote(hpx::id_type locality)
   {
      get_rank_action act;
      hpx::future<std::uint64_t> f = hpx::async(act, locality);

      return f.get();
   }
*/

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


   double monitor::get_exclusive_time_remote(hpx::id_type locality, std::string w_id)
   {
      get_exclusive_time_action act;
      hpx::future<double> f = hpx::async(act, locality, w_id);

      return f.get();
   }


   // Returns the inclusive time for a work item with ID w_id
   double monitor::get_inclusive_time(std::string w_id)
   {
      std::shared_ptr<allscale::profile> p;

      std::lock_guard<mutex_type> lock(work_map_mutex);

      std::unordered_map<std::string, std::shared_ptr<allscale::profile>>::const_iterator it = profiles.find(w_id);
      std::chrono::steady_clock::time_point start_time, max_time;

      if( it == profiles.end() ) return 0.0;

      start_time = max_time = (it->second)->start;

      // Inclusive time computed as the difference between work item start time and maximum children finish time
      auto it_g = w_dependencies.find(w_id);

      if(it_g == w_dependencies.end()) return 0.0;

      for(auto it2 = (it_g->second).begin(); it2 != (it_g->second).end(); ++it2)
      {
          auto it_profiles = profiles.find(*it2);
            if(it_profiles != profiles.end()) {
                p = it_profiles->second;
                max_time = (p->end > max_time) ? p->end : max_time;
            }
      }

      std::chrono::duration<double> time_elapsed = std::chrono::duration_cast<std::chrono::duration<double>>(max_time - start_time);
      return time_elapsed.count();

   }


   double monitor::get_inclusive_time_remote(hpx::id_type locality, std::string w_id)
   {
      get_inclusive_time_action act;
      hpx::future<double> f = hpx::async(act, locality, w_id);

      return f.get();
   }


   // Returns the average exclusive time for a work item with name w_name
   double monitor::get_average_exclusive_time(std::string w_name)
   {
      double total = 0.0;
      long long num_wi = 0;
      std::lock_guard<mutex_type> lock(work_map_mutex);
//      std::shared_ptr<allscale::profile> p;

      for(auto it = profiles.begin(); it != profiles.end(); ++it)
         if((it->second)->get_wname() == w_name) {
                total += (it->second)->get_exclusive_time();
                num_wi++;
         }

/*
      for(auto it = w_names.begin(); it != w_names.end(); ++it)
         if(it->second == w_name) {
            auto it_profiles = profiles.find(it->first);
            if(it_profiles != profiles.end()) {
                p = it_profiles->second;
                total += p->get_exclusive_time();
                num_wi++;
            }
         }
*/
      if(num_wi > 0) return total/num_wi;
      else return 0.0;
   }


   double monitor::get_average_exclusive_time_remote(hpx::id_type locality, std::string w_name)
   {
      get_average_exclusive_time_action act;
      hpx::future<double> f = hpx::async(act, locality, w_name);

      return f.get();
   }


   // Returns the minimum exclusive time for a work item with name w_name
   double monitor::get_minimum_exclusive_time(std::string w_name)
   {
      std::lock_guard<mutex_type> lock(work_map_mutex);
      double min = std::numeric_limits<double>::max();
      long long num_wi = 0;
//      std::shared_ptr<allscale::profile> p;

      for(auto it = profiles.begin(); it != profiles.end(); ++it)
         if((it->second)->get_wname() == w_name) {
                double time = (it->second)->get_exclusive_time();
                min = ((time < min) ? time : min );
                num_wi++;
         }

/*
      for(auto it = w_names.begin(); it != w_names.end(); ++it)
         if(it->second == w_name) {
            auto it_profiles = profiles.find(it->first);
            if(it_profiles != profiles.end()) {
                p = it_profiles->second;
                double time = p->get_exclusive_time();
                min = ((time < min) ? time : min );
                num_wi++;
            }
         }
*/
      if(num_wi > 0) return min;
      else return 0.0;
   }


   double monitor::get_minimum_exclusive_time_remote(hpx::id_type locality, std::string w_name)
   {
      get_minimum_exclusive_time_action act;
      hpx::future<double> f = hpx::async(act, locality, w_name);

      return f.get();
   }


   // Returns the maximum exclusive time for a work item with name w_name
   double monitor::get_maximum_exclusive_time(std::string w_name)
   {
      std::lock_guard<mutex_type> lock(work_map_mutex);
      double max = 0.0;
//      std::shared_ptr<allscale::profile> p;

      for(auto it = profiles.begin(); it != profiles.end(); ++it)
         if((it->second)->get_wname() == w_name) {
                double time = (it->second)->get_exclusive_time();
                max = ((time > max) ? time : max );
         }

/*
      for(auto it = w_names.begin(); it != w_names.end(); ++it)
         if(it->second == w_name) {
            auto it_profiles = profiles.find(it->first);
            if(it_profiles != profiles.end()) {
                p = it_profiles->second;
                double time = p->get_exclusive_time();
                max = ((time > max) ? time : max );
            }
         }
*/
      return max;
   }



   double monitor::get_maximum_exclusive_time_remote(hpx::id_type locality, std::string w_name)
   {
      get_maximum_exclusive_time_action act;
      hpx::future<double> f = hpx::async(act, locality, w_name);

      return f.get();
   }

   // Returns the mean exclusive time for all children
   double monitor::get_children_mean_time(std::string w_id)
   {
      double total = 0.0;
      long long num_children = 0;
      std::shared_ptr<allscale::profile> p;

      std::lock_guard<mutex_type> lock(work_map_mutex);
      auto it = w_dependencies.find(w_id);

      if(it == w_dependencies.end()) return 0.0;

      num_children = (it->second).size();

      for(auto it2 = (it->second).begin(); it2 != (it->second).end(); ++it2)
      {
          auto it_profiles = profiles.find(*it2);
            if(it_profiles != profiles.end()) {
                p = it_profiles->second;
                total += p->get_exclusive_time();
            }
      }

      if( num_children > 0 )
         return total/num_children;
      else
         return 0.0;
   }


   double monitor::get_children_mean_time_remote(hpx::id_type locality, std::string w_id)
   {
      get_children_mean_time_action act;
      hpx::future<double> f = hpx::async(act, locality, w_id);

      return f.get();
   }


   // Returns the exclusive time standard deviation for all children
   double monitor::get_children_SD_time(std::string w_id)
   {
      std::vector<double> times;
      double total, mean, stdev, accum = 0.0;
      std::shared_ptr<allscale::profile> p;

      std::lock_guard<mutex_type> lock(work_map_mutex);
      auto it = w_dependencies.find(w_id);

      if(it == w_dependencies.end()) return 0.0;

      for(auto it2 = (it->second).begin(); it2 != (it->second).end(); ++it2)
      {
          auto it_profiles = profiles.find(*it2);
            if(it_profiles != profiles.end()) {
                p = it_profiles->second;
                times.push_back(p->get_exclusive_time());
            }
      }

     total = std::accumulate(std::begin(times), std::end(times), 0.0);
     mean =  total / times.size();

     std::for_each (std::begin(times), std::end(times), [&](const double d) {
           accum += (d - mean) * (d - mean);
      });

     stdev = sqrt(accum / (times.size()-1));
     return stdev;
   }


   double monitor::get_children_SD_time_remote(hpx::id_type locality, std::string w_id)
   {
      get_children_SD_time_action act;
      hpx::future<double> f = hpx::async(act, locality, w_id);

      return f.get();
   }


   double monitor::get_avg_work_item_times(std::uint32_t num_work_items)
   {
      std::lock_guard<mutex_type> lock(work_items_vector);
      double avg = 0.0;
      std::uint32_t j = num_work_items;

      for(std::vector<double>::reverse_iterator i = work_item_times.rbegin();
                   i != work_item_times.rend(); ++i)
      {
          if(!j) break;
          avg += *i; j--;

      }

      if(num_work_items <= work_item_times.size())
         return avg/(double)num_work_items;
      else
         return avg/(double)work_item_times.size();
   }


   double monitor::get_SD_work_item_times(std::uint32_t num_work_items)
   {
      double std = 0.0;

      //First get the mean
      double mean = get_avg_work_item_times(num_work_items);

      // Now compute the standard deviation
      std::uint32_t j = num_work_items;

      std::lock_guard<mutex_type> lock(work_items_vector);
      double acc = 0.0;

      for(std::vector<double>::reverse_iterator i = work_item_times.rbegin();
                   i != work_item_times.rend(); ++i)
      {
          if(!j) break;
          acc += pow(*i - mean, 2); j--;

      }

      if(num_work_items <= work_item_times.size())
         return sqrt(acc/(double)num_work_items);
      else
         return sqrt(acc/(double)work_item_times.size());
   }



#ifdef HAVE_PAPI
   // Returns PAPI counters for a work item with ID w_id
   long long *monitor::get_papi_counters(std::string w_id)
   {
      std::shared_ptr<allscale::profile> p;

      std::lock_guard<mutex_type> lock(work_map_mutex);
      std::unordered_map<std::string, std::shared_ptr<allscale::profile>>::const_iterator it = profiles.find(w_id);

      if( it == profiles.end() )
         return NULL;
      else
         return (it->second)->get_counters();
   }
#endif


#if 0
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
      std::lock_guard<mutex_type> lock(history_mutex);

      if(i < 0 || i >= history->iteration_time.size()) return 0.0;
      else return history->iteration_time[i];
   }


   double monitor::get_iteration_time_remote(hpx::id_type locality, int i)
   {
      get_iteration_time_action act;
      hpx::future<double> f = hpx::async(act, locality, i);

      return f.get();
   }


   double monitor::get_last_iteration_time()
   {
      std::lock_guard<mutex_type> lock(history_mutex);
 
      if(!(history->iteration_time.size())) return 0.0;
      else return history->iteration_time.back();
   }


   double monitor::get_last_iteration_time_remote(hpx::id_type locality)
   {
      get_last_iteration_time_action act;
      hpx::future<double> f = hpx::async(act, locality);

      return f.get();
   }


   long monitor::get_number_of_iterations()
   {
      std::lock_guard<mutex_type> lock(history_mutex);

      return history->current_iteration;
   }


   long monitor::get_number_of_iterations_remote(hpx::id_type locality)
   {
      get_number_of_iterations_action act;
      hpx::future<long> f = hpx::async(act, locality);

      return f.get();
   }

   double monitor::get_avg_time_last_iterations(std::uint32_t num_iters)
   {
      double avg_time = 0.0;
      std::uint32_t j = num_iters;
      std::lock_guard<mutex_type> lock(history_mutex);

      for(std::vector<double>::reverse_iterator i = history->iteration_time.rbegin();
		   i != history->iteration_time.rend(); ++i)
      {
	  if(!j) break;
	  avg_time += *i; j--;

      }

      if(num_iters <= history->iteration_time.size())
          return avg_time/(double)num_iters;
      else if(history->iteration_time.empty())
          return avg_time;
      else
          return avg_time/(double)history->iteration_time.size();
   }

   double monitor::get_avg_time_last_iterations_remote(hpx::id_type locality, std::uint32_t num_iters)
   {
      get_avg_time_last_iterations_action act;
      hpx::future<double> f = hpx::async(act, locality, num_iters);

      return f.get();
   }


   // Translates a work ID changing it prefix with the last iteration root
   // Returns the same label for work items of the first iteration
   std::string monitor::match_previous_treeture(std::string const& w_ID)
   {
      std::string match = w_ID;
      std::lock_guard<mutex_type> lock(history_mutex);

      int last_iteration = history->current_iteration - 1;

      if(last_iteration >= 0) {
         std::string previous_root = history->iteration_roots[last_iteration];

         match.replace(0, previous_root.size(), previous_root);
      }

      return match;
   }


   void monitor::print_node(std::ofstream& myfile, std::string node, double total_tree_time, 
				profile_map& global_stats, dependency_graph& g)
   {
      std::string label;
      double excl_elapsed, incl_elapsed;

      auto profile_it = global_stats.find(node);
      if(profile_it == global_stats.end()) return;


      excl_elapsed = (profile_it->second).get_exclusive_time();
      incl_elapsed = (profile_it->second).get_inclusive_time();

      std::stringstream stream;
      stream << std::scientific << std::setprecision(3) << excl_elapsed;
      label += std::string("\\n Texcl = ") + stream.str();
      // Clear the string
      stream.str(std::string());
      stream << std::scientific << std::setprecision(3) << incl_elapsed;
      label += std::string("\\n Tincl = ") + stream.str();

      unsigned color = allscale::components::util::intensity_to_rgb(excl_elapsed, total_tree_time);

      myfile << "  \"" << profile_it->first << " " << (profile_it->second).get_name()
            << label <<"\"     [ fillcolor=\"#" << std::hex << color << "\" ]" << std::endl;


      // Traverse children recursively
      auto dep_it = g.find(node);
      if(dep_it != g.end()) 
         for( auto children = (dep_it->second).begin(); children != (dep_it->second).end(); ++children )
     	     print_node(myfile, *children, total_tree_time, global_stats, g);

      return;

   }

   void monitor::print_edges(std::ofstream& myfile, std::string node, 
				profile_map& global_stats, dependency_graph& g)
   {
      double excl_elapsed, incl_elapsed;
      std::string label1, label2;

      auto it = global_stats.find(node);
      // Tree leaf
      if( it == global_stats.end() ) return;

      label1 = node + ' ' + (it->second).get_name();

      excl_elapsed = (it->second).get_exclusive_time();
      incl_elapsed = (it->second).get_inclusive_time();

      std::stringstream stream;
      stream << std::scientific << std::setprecision(3) << excl_elapsed;
      label1 += std::string("\\n Texcl = ") + stream.str();
      // Clear the string
      stream.str(std::string());
      stream << std::scientific << std::setprecision(3) << incl_elapsed;
      label1 += std::string("\\n Tincl = ") + stream.str();

      auto dep_it = g.find(node);
      if(dep_it != g.end())
         for(auto child = (dep_it->second).begin(); child != (dep_it->second).end(); ++child) {

             auto it2 = global_stats.find(*child);
             if(it2 == global_stats.end()) continue;

             label2 = (*child) + ' ' + (it->second).get_name();

             excl_elapsed = (it2->second).get_exclusive_time();
             incl_elapsed = (it2->second).get_inclusive_time();

             std::stringstream stream;
             stream << std::scientific << std::setprecision(3) << excl_elapsed;
             label2 += std::string("\\n Texcl = ") + stream.str();
             // Clear the string
             stream.str(std::string());
             stream << std::scientific << std::setprecision(3) << incl_elapsed;
             label2 += std::string("\\n Tincl = ") + stream.str();

             myfile << "  \"" << label1 << "\" -> \"" << label2 << "\"" << std::endl;

             print_edges(myfile, *child, global_stats, g);
         }
   }


   void monitor::print_treeture(std::string filename, std::string root, double total_tree_time, 
					profile_map& global_stats, dependency_graph& g)
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
      print_node(myfile, root, total_tree_time, global_stats, g);


      // Print Edges
      print_edges(myfile, root, global_stats, g);

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

//	 print_treeture(filename, *it1, *it2);
	 iter_number++;
      }
   }


   void monitor::monitor_component_output(profile_map &global_stats) {
//      std::lock_guard<mutex_type> lock(work_map_mutex);


      std::cerr << "\nWall-clock time: " << wall_clock << std::endl;
      std::cerr << "\nWork Item		Exclusive time  |  % Total  |  Inclusive time  |  % Total   |   Mean (child.)  |   SD (child.)  "
                << "\n------------------------------------------------------------------------------------------------------------------\n";


      // Sort work items by id
//      std::sort(global_stats.begin(), global_stats.end());

      std::vector<std::string> w_ids;
      w_ids.reserve(global_stats.size());

      // sort work_item names
      for(auto& it : global_stats) {
         w_ids.push_back(it.first);
      }

      std::sort(w_ids.begin(), w_ids.end());


      // iterate over the profiles
      for(auto s : w_ids) {

        if(s == "0") continue;   //skip main profile, think whether no need to save it

        profile_map::iterator it = global_stats.find(s);
        if(it == global_stats.end()) continue;

        std::cerr.precision(5);
        std::cerr << std::scientific << "   " << s + ' ' + (it->second).get_name() << "\t\t " << (it->second).get_exclusive_time() << "\t\t";
            std::cerr.precision(2);
            std::cerr << std::fixed << ((it->second).get_exclusive_time()/wall_clock) * 100;
            std::cerr.precision(5);
            std::cerr << std::scientific << "\t" << (it->second).get_inclusive_time() << "\t\t";
            std::cerr.precision(2);
            std::cerr << std::fixed << ((it->second).get_inclusive_time()/wall_clock) * 100;
            std::cerr.precision(5);
            std::cerr << std::fixed << (it->second).get_children_mean() << "       " << (it->second).get_children_SD() << std::endl;
//            std::cerr << std::fixed << p->Mean() << "       " << p->StandardDeviation() << std::endl;
      }
    }


#ifdef HAVE_PAPI
   void monitor::monitor_papi_output() {
//      std::lock_guard<mutex_type> lock(work_map_mutex);
      long long *counter_values;

      std::size_t num_counters = papi_counter_names.size();
      if(!num_counters) return;

      std::cerr << "\n\nPAPI counters data" << std::endl;
      std::cerr << "\nWork Item         ";

      for(int i = 0; i < num_counters; i++)
	 std::cerr << papi_counter_names[i] << "\t\t";

      std::cerr << std::endl;


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

            counter_values = p->get_counters();

            std::cerr << "   " << i + ' ' + w_names[i];
            for(int i = 0; i < num_counters; i++)
		std::cerr << "\t\t" << counter_values[i];

            std::cerr << std::endl; free(counter_values);
        }
      }
      w_id.clear();
    }
#endif



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
//      std::shared_ptr<allscale::profile> p = profiles[mystring];
//      p->end = execution_end;
//      p->result_ready = execution_end;

      // Finish additional OS-thread
      { 
         std::lock_guard<std::mutex> lk(m_queue);
         done = true;
      }
      if(enable_monitor) {
	cv.notify_one();
        worker_thread.join();
 
     }
/*
      {
         std::lock_guard<std::mutex> lk(m_queue);
         std::cerr << "Num split tasks " << num_split_tasks << "   Avg split time " << total_split_time/num_split_tasks 
		   << " Min split time " << min_split_task << " Max split task " << max_split_task << std::endl;

         std::cerr << "Num process tasks " << num_process_tasks << "   Avg process time " << total_process_time/num_process_tasks 
                   << " Min process time " << min_process_task << " Max process task " << max_process_task << std::endl;


      }
*/
#ifdef HAVE_PAPI
/*      for(std::vector<hpx::performance_counters::performance_counter>::iterator it = counters.begin(); it != counters.end(); ++it)
      {
	 hpx::performance_counters::counter_info info = (*it).get_info().get();
         long long value = (*it).get_value<long long>().get();
         std::cout << "Counter: " << info.fullname_ << " Value: " << value << std::endl;

      }
*/
#endif


      if(output_profile_table_ || output_treeture_ || output_iteration_trees_) {

        // Compute work item stats from profiles map
        double e_time, i_time, children_mean_time, children_sd;
        profile_map my_local_stats, global_stats;
        dependency_graph global_graph;

        for ( auto it = profiles.begin(); it != profiles.end(); ++it ) {

	   	allscale::work_item_stats stats(it->first, (it->second)->get_wname(), get_exclusive_time(it->first), 
                    get_inclusive_time(it->first), get_children_mean_time(it->first), 
	            get_children_SD_time(it->first));

		my_local_stats.insert(std::make_pair(it->first, stats));
        }

        // Collect info from all localities
        if(rank_ == 0) {
           hpx::future<std::vector<profile_map>> f =
		hpx::lcos::gather_here(gather_basename1, hpx::make_ready_future(my_local_stats));

           std::vector<profile_map> rcv_buffer = f.get();


           for ( auto it = rcv_buffer.begin(); it != rcv_buffer.end(); ++it ) 
           {
		global_stats.insert((*it).begin(), (*it).end());
           }
        }
        else hpx::lcos::gather_there(gather_basename1, hpx::make_ready_future(my_local_stats)).wait(); 


        // Collect work item dependencies from all localities
        if(rank_ == 0) {
           hpx::future<std::vector<dependency_graph>> f =
                hpx::lcos::gather_here(gather_basename2, hpx::make_ready_future(w_dependencies));

           std::vector<dependency_graph> rcv_buffer = f.get();


           for ( auto it = rcv_buffer.begin(); it != rcv_buffer.end(); ++it )
           {
                global_graph.insert((*it).begin(), (*it).end());
           }

        }
        else hpx::lcos::gather_there(gather_basename2, hpx::make_ready_future(w_dependencies)).wait();

        if(rank_ == 0 && output_profile_table_) {
            monitor_component_output(global_stats);
#ifdef HAVE_PAPI
            monitor_papi_output();
#endif
        }

        if(rank_ == 0 && output_treeture_)
            print_treeture("treeture.dot", "0.0", wall_clock, global_stats, global_graph);

        if(rank_ == 0 && output_iteration_trees_) {
            history->new_iteration(std::string("foo"));
            print_trees_per_iteration();
        }
      }
   }

   void monitor::global_finalize() {
//      (allscale::monitor::get_ptr().get())->monitor_component_finalize();
   }


   void monitor::init() {

      if(const char* env_p = std::getenv("ALLSCALE_MONITOR"))
      {
          if(atoi(env_p) == 0)
              enable_monitor = false;
      }

      if(enable_monitor)
      {
          allscale::monitor::connect(allscale::monitor::work_item_split_execution_started, monitor::global_w_exec_split_start_wrapper);
          allscale::monitor::connect(allscale::monitor::work_item_process_execution_started, monitor::global_w_exec_process_start_wrapper);
          allscale::monitor::connect(allscale::monitor::work_item_split_execution_finished, monitor::global_w_exec_split_finish_wrapper);
          allscale::monitor::connect(allscale::monitor::work_item_process_execution_finished, monitor::global_w_exec_process_finish_wrapper);
//          allscale::monitor::connect(allscale::monitor::work_item_result_propagated, monitor::global_w_result_propagated_wrapper);
          allscale::monitor::connect(allscale::monitor::work_item_first, monitor::global_w_app_iteration);
      }

//      const int result = std::atexit(global_finalize);
//      if(result != 0) std::cerr << "Registration of monitor_finalize function failed!" << std::endl;


      std::uint64_t left_id =
         rank_ == 0 ? num_localities_ - 1 : rank_ - 1;
      std::uint64_t right_id =
         rank_ == num_localities_ - 1 ? 0 : rank_ + 1;

 
      hpx::future<hpx::id_type> right_future =
          hpx::find_from_basename("allscale/monitor", right_id);
                                                                  
      if(left_id != right_id)
      {
           hpx::future<hpx::id_type> left_future =
               hpx::find_from_basename("allscale/monitor", left_id);
                                                                                                                
           left_ = left_future.get();
      }
                                                                                                                        
      if(num_localities_ > 1)
          right_ = right_future.get();
 

      // Check environment variables
/*
      if(const char* env_p = std::getenv("MONITOR_CUTOFF")) {
         char *p;
         cutoff_level_ = strtol(env_p, &p, 10);
         if(*p) {
           std::cerr << "ERROR: Invalid cutoff value" << std::endl;
           cutoff_level_ = 0;
         }
      }
*/
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


     if(const char* env_p = std::getenv("MONITOR_PAPI")) {
#ifdef HAVE_PAPI
        collect_papi_ = 1;
 	std::string counter_names(env_p);
        static const char *counter_set_name = "/papi{locality#%d/worker-thread#%d}/%s";
        const std::uint32_t prefix = hpx::get_locality_id();
        std::size_t const os_threads = hpx::get_os_thread_count();


        typedef boost::tokenizer<boost::char_separator<char>> tokenizer;

        boost::char_separator<char> sep(",");
        tokenizer tok(counter_names, sep);
        int num_tokens=0;
        for (const auto& t : tok) {
             if(num_tokens >= MAX_PAPI_COUNTERS) {
                std::cerr << "Max number of PAPI counters allowed is " << MAX_PAPI_COUNTERS << std::endl;
                break;
             }

             papi_counter_names.push_back(t);

	     for (std::size_t os_thread = 0; os_thread < os_threads; ++os_thread)
             {
                std::cerr << "Registering counter " << boost::str(boost::format(counter_set_name) % prefix % os_thread % t) << std::endl;
//		hpx::performance_counters::performance_counter counter(boost::str(boost::format(counter_set_name) % prefix % os_thread % t));
	        hpx::id_type counter = hpx::performance_counters::get_counter(boost::str(boost::format(counter_set_name) % prefix % os_thread % t));
                hpx::performance_counters::stubs::performance_counter::start(hpx::launch::sync, counter);
		counters.insert(std::make_pair(os_thread, counter));
             }
//             counter_values[num_tokens] = counter.get_value<long long>().get();
//             counters.push_back(counter);

	     num_tokens++;
        }
#else
        std::cerr << "Var. MONITOR_PAPI defined but code compiled without PAPI support!" << std::endl;
#endif
     }




      execution_start = std::chrono::steady_clock::now();
      execution_init = duration_cast<nanoseconds>(system_clock::now().time_since_epoch()).count();


      // Create specialised OS-thread to proces profiles
      if(enable_monitor) worker_thread = std::thread(&allscale::components::monitor::process_profiles, this);
 
      // Create the profile for the "Main"
      std::shared_ptr<allscale::profile> p(new profile("0", "Main", " "));
      std::string mystring("0");

      profiles.insert(std::make_pair(mystring, p));
//      w_names.insert(std::make_pair(mystring, std::string("Main")));

      // Insert the root "0" in the graph
      w_dependencies.insert(std::make_pair(std::string("0"), std::vector<std::string>()));

      // Init historical data
      history = std::make_shared<allscale::historical_data>();


      std::cerr
         << "Monitor component with rank "
         << rank_ << " created!\n";

   }




}}


//HPX_REGISTER_ACTION(allscale::components::monitor::get_my_rank_action, get_my_rank_action);
HPX_REGISTER_ACTION(allscale::components::monitor::get_exclusive_time_action, get_exclusive_time_action);
HPX_REGISTER_ACTION(allscale::components::monitor::get_inclusive_time_action, get_inclusive_time_action);
HPX_REGISTER_ACTION(allscale::components::monitor::get_average_exclusive_time_action, get_average_exclusive_time_action);
HPX_REGISTER_ACTION(allscale::components::monitor::get_minimum_exclusive_time_action, get_minimum_exclusive_time_action);
HPX_REGISTER_ACTION(allscale::components::monitor::get_maximum_exclusive_time_action, get_maximum_exclusive_time_action);
HPX_REGISTER_ACTION(allscale::components::monitor::get_children_mean_time_action, get_children_mean_time_action);
HPX_REGISTER_ACTION(allscale::components::monitor::get_children_SD_time_action, get_children_SD_time_action);
HPX_REGISTER_ACTION(allscale::components::monitor::get_iteration_time_action, get_iteration_time_action);
HPX_REGISTER_ACTION(allscale::components::monitor::get_last_iteration_time_action, get_last_iteration_time_action);
HPX_REGISTER_ACTION(allscale::components::monitor::get_number_of_iterations_action, get_number_of_iterations_action);
HPX_REGISTER_ACTION(allscale::components::monitor::get_avg_time_last_iterations_action, get_avg_time_last_iterations_action);

