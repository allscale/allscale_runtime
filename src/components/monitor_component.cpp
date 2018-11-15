#include <allscale/components/monitor.hpp>
#include <allscale/monitor.hpp>
#include <allscale/dashboard.hpp>
#include <allscale/get_num_numa_nodes.hpp>
#include <allscale/power.hpp>

#include <math.h>
#include <limits>
#include <algorithm>
#include <string>
#include <unistd.h>
#include <arpa/inet.h>
#include <thread>

#include <boost/format.hpp>

#include <hpx/compat/thread.hpp>

#include <hpx/runtime/serialization/serialize.hpp>
#include <hpx/runtime/serialization/string.hpp>
#include <hpx/runtime/serialization/vector.hpp>
#include <hpx/runtime/serialization/unordered_map.hpp>
#include <hpx/runtime/shutdown_function.hpp>
#include <hpx/runtime/get_num_localities.hpp>
#include <hpx/runtime/find_here.hpp>

#include <hpx/lcos/gather.hpp>

#ifdef ALLSCALE_HAVE_CPUFREQ
#define POWER_MEASUREMENT_PERIOD_MS 100
#include <allscale/util/hardware_reconf.hpp>
#endif

#ifdef HAVE_PAPI
#include <boost/tokenizer.hpp>
#include <string.h>
#endif

#ifdef HAVE_EXTRAE
#include "extrae.h"
#endif

#define SAMPLE_INTERVAL 1000

char const* gather_basename1 = "allscale/monitor/gather1";
HPX_REGISTER_GATHER(profile_map, profile_gatherer);

char const* gather_basename2 = "allscale/monitor/gather2";
HPX_REGISTER_GATHER(dependency_graph, dependency_gatherer);

char const* gather_basename3 = "allscale/monitor/gather3";
HPX_REGISTER_GATHER(std::vector<double>, heatmap_gatherer);

char const* gather_basename4 = "allscale/monitor/gather4";

using namespace std::chrono;

namespace allscale { namespace components {


   monitor::monitor(std::uint64_t rank)
     : last_task_times_sample_(std::chrono::high_resolution_clock::now())
     , idle_rate_idx_(0)
     , rank_(rank)
     , num_localities_(0)
     , enable_monitor(true)
     , output_profile_table_(0)
     , output_treeture_(0)
     , output_iteration_trees_(0)
     , collect_papi_(0)
     , cutoff_level_(0)
     , print_throughput_hm_(0)
     , print_idle_hm_(0)
     , done(false)
     , current_read_queue(0)
     , current_write_queue(0)
     , sampling_interval_ms(SAMPLE_INTERVAL)
     , finished_tasks(0)
     , task_throughput(0)
     , cpu_load_(0.0)
     , total_memory_(0)
     , num_cpus_(0)
     , total_split_time(0)
     , total_process_time(0)
     , num_split_tasks(0)
     , num_process_tasks(0)
     , min_split_task(0)
     , max_split_task(0)
     , min_process_task(0)
     , max_process_task(0)
     , weighted_sum(0.0)
     , weighted_throughput(0.0)
     , bytes_sent_(0)
     , bytes_recv_(0)
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
        std::fill(idle_rates_.begin(), idle_rates_.end(), 1.0);
    }


   void monitor::add_task_time(task_id::task_path const& path, task_times::time_t const& time)
   {
       if (!enable_monitor) return;

       std::lock_guard<mutex_type> l(task_times_mtx_);
//        if (hpx::get_locality_id() == 0)
//        {
//            std::cout << path.getPath() << " " << time.count() << "\n";
//        }
       task_times_.add(path, time);
       process_time_ += time;
   }

   task_times monitor::get_task_times()
   {
       if (!enable_monitor) return task_times{};

       std::lock_guard<mutex_type> l(task_times_mtx_);
       auto now = std::chrono::high_resolution_clock::now();

       // normalize to one second
       auto interval = std::chrono::duration_cast<std::chrono::nanoseconds>(now - last_task_times_sample_);
       auto res = (task_times_ - last_task_times_) / interval.count();

       last_task_times_sample_ = now;
       last_task_times_ = task_times_;
       return res;
   }

//    task_times::time_t monitor::get_process_time()
//    {
//        std::lock_guard<mutex_type> l(task_times_mtx_);
//        return process_time_;
//    }

   double monitor::get_idle_rate()
   {
       std::lock_guard<mutex_type> l(task_times_mtx_);
       auto now = std::chrono::high_resolution_clock::now();

       auto process_time = process_time_;

       auto d1 = std::chrono::duration_cast<task_times::time_t>(process_time - process_time_buffer_.oldest_data());
       auto d2 = std::chrono::duration_cast<task_times::time_t>(now - process_time_buffer_.oldest_time());


       double cur_idle_rate = 1. - static_cast<double>(d1.count())/(static_cast<double>(d2.count()) * num_cpus_);

       idle_rates_[idle_rate_idx_] = cur_idle_rate;
       idle_rate_idx_ = (idle_rate_idx_ + 1) % idle_rate_history_count;

       // aggregate process time...
       process_time_buffer_.push(process_time, now);

       // calucalate the mean idle rate over all as the result
       return std::accumulate(idle_rates_.begin(), idle_rates_.end(), 0.0) / idle_rate_history_count;
   }

#ifdef REALTIME_VIZ
   bool monitor::sample_task_stats()
   {
      hpx::performance_counters::counter_value idle_value;
      hpx::performance_counters::counter_value rss_value;

      rss_value = hpx::performance_counters::stubs::performance_counter::get_value(
                hpx::launch::sync, resident_memory_counter_);


     std::unique_lock<std::mutex> lock(counter_mutex_);

     resident_memory_ = rss_value.get_value<double>() * 1e-6;

     data_file << sample_id_++ << "\t" << num_active_tasks_ << "\t"
               << get_avg_task_duration() << "\t" << get_idle_rate() << "\t" << resident_memory_ << std::endl;

//     std::cout << "Total number of tasks: " << total_tasks_ << " Number of active tasks: " << num_active_tasks_
//	       << "Average time per task: " << get_avg_task_duration() <<  "IDLE RATE: " << idle_rate_ << std::endl;
     return true;
   }
#endif

   double monitor::get_avg_task_duration()
   {
     if(!total_tasks_) return 0.0;
     else return total_task_duration_/(double)total_tasks_;
   }


   std::uint64_t monitor::get_timestamp( void ) {
      uint64_t timestamp = duration_cast<nanoseconds>(system_clock::now().time_since_epoch()).count();
      timestamp = timestamp - execution_init;
      return timestamp;
   }


/*
   void monitor::change_sampling_interval(long long new_interval)
   {
      std::unique_lock<std::mutex> lock(sampling_mutex);
      sampling_interval_ms = new_interval;
   }
*/

   double monitor::get_throughput()
   {
      std::unique_lock<std::mutex> lock(sampling_mutex);
      return task_throughput;
   }


   double monitor::get_throughput_remote(hpx::id_type locality)
   {
      get_throughput_action act;
      hpx::future<double> f = hpx::async(act, locality);

      return f.get();
   }


   double monitor::get_weighted_throughput()
   {
      std::unique_lock<std::mutex> lock(sampling_mutex);
      return weighted_throughput;
   }




   double monitor::get_idle_rate_remote(hpx::id_type locality)
   {
      get_idle_rate_action act;
      hpx::future<double> f = hpx::async(act, locality);

      return f.get();
   }


   unsigned long long monitor::get_node_total_memory()
   {
      return total_memory_;
   }


   int monitor::get_num_cpus()
   {
     return num_cpus_;
   }


   float monitor::get_cpu_load()
   {
      std::unique_lock<std::mutex> lock(sampling_mutex);
      return cpu_load_;
   }

   std::uint64_t monitor::get_consumed_memory()
   {
      std::unique_lock<std::mutex> lock(sampling_mutex);
      return resident_memory_;
   }


   std::uint64_t monitor::get_current_freq(int cpuid)
   {
      static const char * file_name = "/sys/devices/system/cpu/cpu%d/cpufreq/scaling_cur_freq";
      std::uint64_t freq_in_KHz = 0;

      std::ifstream file(boost::str(boost::format(file_name) % cpuid));

      if(!file) {
	 std::cerr << "Cannot read frequency from /sys/devices for CPU " << cpuid << std::endl;
	 return 0;
      }

      file >> freq_in_KHz;

      file.close();
      return freq_in_KHz;
   }


   std::uint64_t monitor::get_min_freq(int cpuid)
   {
      std::map<int, std::vector<std::uint64_t>>::iterator it;

      it = cpu_frequencies.find(cpuid);
      if(it != cpu_frequencies.end())
      {
          auto back = (it->second).back();
          auto front = (it->second).front();
          return std::min(back, front);
      }
      else
	 return 0;
   }


   std::uint64_t monitor::get_max_freq(int cpuid)
   {
      std::map<int, std::vector<std::uint64_t>>::iterator it;

      it = cpu_frequencies.find(cpuid);
      if(it != cpu_frequencies.end())
      {
          auto back = (it->second).back();
          auto front = (it->second).front();
          return std::max(back, front);
      }
      else
         return 0;
   }

   void monitor::set_cur_freq(std::uint64_t freq)
   {
      static const char * file_name = "/sys/devices/system/cpu/cpu%d/cpufreq/scaling_cur_freq";

      std::string freq_s = std::to_string(freq);

      auto format_str = boost::format(file_name);

      for (std::size_t cpuid = 0; cpuid != num_cpus_; ++cpuid)
      {
          std::ofstream file(boost::str(format_str % cpuid));
          if (file)
          {
              file.write(freq_s.data(), freq_s.size());
              file.flush();
          }
      }
   }


   std::vector<std::uint64_t> monitor::get_available_freqs(int cpuid)
   {
      std::map<int, std::vector<std::uint64_t>>::iterator it;

      it = cpu_frequencies.find(cpuid);
      if(it != cpu_frequencies.end())
         return it->second;
      else
         return std::vector<std::uint64_t>();
   }


   float monitor::get_current_power()
   {
#ifdef ALLSCALE_HAVE_CPUFREQ
      /*VV: Read potentially multiple measurements of power within the span of 
            POWER_MEASUREMENT_PERIOD_MS milliseconds. Each time this function
            is invoked it returns the running average of power.*/
      static mutex_type power_mtx;
      static unsigned long long times_read_power=0;
      static unsigned long long power_sum = 0ull;
      static long timestamp_reset_power = 0;

      int64_t t_now, dt;
      float ret;

      std::lock_guard<mutex_type> lock(power_mtx);
      
      t_now = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now()).time_since_epoch().count();
 
      dt = t_now - timestamp_reset_power;
      times_read_power ++;

      power_sum += util::hardware_reconf::read_system_power();

      ret = power_sum / (float)(times_read_power);

      if ( dt >= POWER_MEASUREMENT_PERIOD_MS ) {
            times_read_power = 0;
            power_sum = 0ull;
            timestamp_reset_power = t_now;
      }

      return ret;
#else
      return allscale::power::estimate_power(get_current_freq(0)) * num_cpus_;
#endif
   }


   float monitor::get_max_power()
   {
#if defined(ALLSCALE_HAVE_CPUFREQ)
      // VV: report 125.0 Watt ( this should be dynamically configured/discovered )
      return 1250.0;
#elif defined(POWER_ESTIMATE)
      return allscale::power::estimate_power(get_max_freq(0)) * num_cpus_;
#else
      return 0.0;
#endif
   }


   std::uint64_t monitor::get_network_out()
   {
      std::unique_lock<std::mutex> lock(sampling_mutex);
      return bytes_sent_;
   }


   std::uint64_t monitor::get_network_in()
   {
      std::unique_lock<std::mutex> lock(sampling_mutex);
      return bytes_recv_;
   }



   // Reading cpu info from system
   void monitor::get_cpus_info()
   {
      static const char * avail_freq_filename = "/sys/devices/system/cpu/cpu%d/cpufreq/scaling_available_frequencies";
      static const char * min_freq_filename = "/sys/devices/system/cpu/cpu%d/cpufreq/scaling_min_freq";
      static const char * max_freq_filename = "/sys/devices/system/cpu/cpu%d/cpufreq/scaling_max_freq";

      std::uint64_t freq_in_KHz = 0;
      std::vector<std::uint64_t> freqs;

      num_cpus_ = hpx::get_os_thread_count();

      for(int i = 0; i < num_cpus_; i++)
      {

          std::ifstream avail_freq_file(boost::str(boost::format(avail_freq_filename) % i));


          if(!avail_freq_file)
          {     // File not available, get min and max via the other two files

//            std::cerr << "Warning: Cannot read avail frequencies from /sys/devices/system/cpu for CPU "
//                      << i
//                      << ". Looking into scaling_min_freq and scaling_max_freq files\n";

              std::ifstream min_freq_file(boost::str(boost::format(min_freq_filename) % i));

              if(!min_freq_file) {
                  std::cerr << "Warning: Cannot read min frequency from /sys/devices/system/cpu for CPU " << i << std::endl;
                  freq_in_KHz = 0;
              }
              else min_freq_file >> freq_in_KHz;

              freqs.push_back(freq_in_KHz);

              std::ifstream max_freq_file(boost::str(boost::format(max_freq_filename) % i));

              if(!max_freq_file) {
                  std::cerr << "Warning: Cannot read max frequency from /sys/devices/system/cpu for CPU " << i << std::endl;
                  freq_in_KHz = 0;
              }
              else max_freq_file >> freq_in_KHz;

              freqs.push_back(freq_in_KHz);

              cpu_frequencies.insert( std::pair<int, std::vector<std::uint64_t>>(i, freqs) );
              freqs.clear();
          }
          else {      // Read the values from the file and put them in the vector of freqs
              while(avail_freq_file >> freq_in_KHz)
                  freqs.push_back(freq_in_KHz);

              cpu_frequencies.insert( std::pair<int, std::vector<std::uint64_t>>(i, freqs) );
              freqs.clear();
          }
      }
   }


   // Additional sampler thread
   bool monitor::sample_node()
   {
       hpx::performance_counters::counter_value idle_value;
       hpx::performance_counters::counter_value rss_value;
       hpx::performance_counters::counter_value network_in_mpi_value;
       hpx::performance_counters::counter_value network_out_mpi_value;
       hpx::performance_counters::counter_value network_in_tcp_value;
       hpx::performance_counters::counter_value network_out_tcp_value;


       std::uint64_t memory_value = 0, network_in_mpi = 0, network_out_mpi = 0;
       std::uint64_t network_in_tcp = 0, network_out_tcp = 0;
       std::uint64_t user_time, nice_time, system_time, idle_time;
       std::string foo_word;


       // Counters
       if(memory_counter_registered_)
          rss_value = hpx::performance_counters::stubs::performance_counter::get_value(
                   hpx::launch::sync, resident_memory_counter_);


       if(network_mpi_counters_registered_) {
          network_out_mpi_value = hpx::performance_counters::stubs::performance_counter::get_value(
                   hpx::launch::sync, nsend_mpi_counter_, true);

          network_in_mpi_value = hpx::performance_counters::stubs::performance_counter::get_value(
                   hpx::launch::sync, nrecv_mpi_counter_, true);
       }


       if(network_tcp_counters_registered_) {
          network_out_tcp_value = hpx::performance_counters::stubs::performance_counter::get_value(
                   hpx::launch::sync, nsend_tcp_counter_, true);

          network_in_tcp_value = hpx::performance_counters::stubs::performance_counter::get_value(
                   hpx::launch::sync, nrecv_tcp_counter_, true);
       }


       if(memory_counter_registered_) memory_value = rss_value.get_value<std::uint64_t>();
       if(network_mpi_counters_registered_) {
	  network_in_mpi = network_in_mpi_value.get_value<std::uint64_t>();
          network_out_mpi = network_out_mpi_value.get_value<std::uint64_t>();
       }
       if(network_tcp_counters_registered_) {
          network_in_tcp = network_in_tcp_value.get_value<std::uint64_t>();
          network_out_tcp = network_out_tcp_value.get_value<std::uint64_t>();
       }


       // CPU load
       if(pstat.is_open()) {
           pstat.seekg(0, std::ios::beg);
           pstat >> foo_word >> user_time >> nice_time >> system_time >> idle_time;
       }

       // Power
#ifdef CRAY_COUNTERS
       allscale::power::read_pm_counters();
#elif POWER_ESTIMATE
       allscale::power::estimate_power(get_current_freq(0));
#endif

       // Compute statistics
       std::unique_lock<std::mutex> lock(sampling_mutex);

       task_throughput = (double)finished_tasks/((double)sampling_interval_ms/1000);
       weighted_throughput = weighted_sum/((double)sampling_interval_ms/1000);
       resident_memory_ = memory_value;
       bytes_sent_ = network_out_mpi + network_out_tcp;
       bytes_recv_ = network_in_mpi + network_out_tcp;

       std::uint64_t used_time = (user_time - last_user_time) + (nice_time - last_nice_time) + (system_time - last_system_time);
       cpu_load_ = ((float)used_time / (float)(used_time + (idle_time - last_idle_time)));
       last_user_time = user_time; last_nice_time = nice_time; last_system_time = system_time; last_idle_time = idle_time;

/*       std::cerr << "NODE " << rank_ << " THROUGHPUT " << task_throughput << " tasks/s "
                 << "IDLE RATE " << get_idle_rate()
                 << "RESIDENT MEMORY " << resident_memory_
                 << "CPU LOAD " << cpu_load_ << std::endl;
*/

       if(print_throughput_hm_) throughput_history.push_back(task_throughput);

       finished_tasks = 0;
       weighted_sum = 0.0;

       lock.unlock();
       // FIXME
//        sendUpdate();

       return true;
   }


   // Additional OS-thread process profiles in here
   void monitor::process_profiles()
   {
       std::shared_ptr<allscale::profile> profile;
       std::string id, parent_id;

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

       	     auto it = wi_dependencies.find(parent_id);
             if( it == wi_dependencies.end() ) {
                   wi_dependencies.insert(std::make_pair(parent_id,
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

             // Need to lock in case there's another thread accessing the the perf stats
             std::unique_lock<mutex_type> lock(work_map_mutex);
//             w_graph.push_back(wd);

             auto it = wi_dependencies.find(parent_id);
             if( it == wi_dependencies.end() ) {
                   wi_dependencies.insert(std::make_pair(parent_id,
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
      auto my_wid = w.id();


      {
         std::unique_lock<std::mutex> lock2(counter_mutex_);
         total_task_duration_ += p->get_exclusive_time();
      }



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
#ifdef HAVE_EXTRAE
      Extrae_event(6000019, 1);
#endif
      task_id & my_wid = const_cast<task_id&>(w.id());
      std::shared_ptr<allscale::profile>& p = my_wid.profile;
      bool notify_consumer = false;


      if(p == nullptr) {
         // Profile does not exist yet, we create it
         p = std::make_shared<profile>(to_string(my_wid), w.name(), to_string(my_wid.parent()));
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

         // Update number of finished tasks for throughput calculation
         std::unique_lock<std::mutex> lock(sampling_mutex);
         finished_tasks++;
         lock.unlock();
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
#ifdef HAVE_EXTRAE
      Extrae_event(6000019, 0);
#endif

   }


   void monitor::global_w_exec_split_start_wrapper(work_item const& w)
   {
        allscale::monitor::get().w_exec_split_start_wrapper(w);
   }



   void monitor::w_exec_process_start_wrapper(work_item const& w)
   {

      task_id & my_wid = const_cast<task_id&>(w.id());
      std::shared_ptr<allscale::profile>& p = my_wid.profile;
      bool notify_consumer = false;
#ifdef HAVE_EXTRAE
      Extrae_event(6000019, 1);
#endif

      if(p  == nullptr) {
         // Profile does not exist yet, we create it
         p = std::make_shared<profile>(to_string(my_wid), w.name(), to_string(my_wid.parent()));
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

         // Update number of finished tasks for throughput calculation
         std::unique_lock<std::mutex> lock(sampling_mutex);
         finished_tasks++;

         // Update data for weighted throughput
         weighted_sum += 1/pow(2, my_wid.depth());
         lock.unlock();
      }

#ifdef HAVE_EXTRAE
      Extrae_event(6000019, 0);
#endif

   }


   void monitor::global_w_exec_process_start_wrapper(work_item const& w)
   {
        allscale::monitor::get().w_exec_process_start_wrapper(w);
   }


   void monitor::w_exec_split_finish_wrapper(work_item const& w)
   {
#ifdef HAVE_EXTRAE
      Extrae_event(6000019, 2);
#endif
      task_id & my_wid = const_cast<task_id&>(w.id());
      std::shared_ptr<allscale::profile>& p = my_wid.profile;
      bool notify_consumer = false;


      if(p == nullptr) {
         // Profile does not exist yet, finish wrapper executed before than create
         p = std::make_shared<profile>(to_string(my_wid), w.name(), to_string(my_wid.parent()));
      }
      else {
         p->end = std::chrono::steady_clock::now();

         // Put the profile in the queue for the OS-thread to process it
         {
           std::lock_guard<std::mutex> lk(m_queue);

           queues[current_write_queue].push(p);
           if(queues[current_write_queue].size() >= MIN_QUEUE_ELEMS)
		notify_consumer = true;
/*
           num_split_tasks++;
           double task_time = p->get_exclusive_time();
           total_split_time += task_time;
           if(!min_split_task || min_split_task > task_time)
               min_split_task = task_time;

           if(max_split_task <= task_time)
               max_split_task = task_time;
*/
         }
         if(notify_consumer) cv.notify_one();

         // Update number of finished tasks for throughput calculation
	 std::unique_lock<std::mutex> lock(sampling_mutex);
	 finished_tasks++;
         lock.unlock();
     }

/*
      std::cout
          << "Finish work item "
          << w.name()
          << " " << my_wid.name()
          << std::endl;
*/

#ifdef HAVE_EXTRAE
      Extrae_event(6000019, 0);
#endif

   }


   void monitor::global_w_exec_split_finish_wrapper(work_item const& w)
   {
        allscale::monitor::get().w_exec_split_finish_wrapper(w);
   }


   void monitor::w_exec_process_finish_wrapper(work_item const& w)
   {
#ifdef HAVE_EXTRAE
      Extrae_event(6000019, 2);
#endif

      task_id & my_wid = const_cast<task_id&>(w.id());
      std::shared_ptr<allscale::profile>& p = my_wid.profile;
      bool notify_consumer = false;


      if(p == nullptr) {
         // Profile does not exist yet, finish wrapper executed before than create
         p = std::make_shared<profile>(to_string(my_wid), w.name(), to_string(my_wid.parent()));
      }
      else {
         p->end = std::chrono::steady_clock::now();

         // Put the profile in the queue for the OS-thread to process it
         {
           std::lock_guard<std::mutex> lk(m_queue);

           queues[current_write_queue].push(p);
           if(queues[current_write_queue].size() >= MIN_QUEUE_ELEMS)
                notify_consumer = true;
/*
           num_process_tasks++;
           double task_time = p->get_exclusive_time();
           total_process_time += task_time;
           if(!min_process_task || min_process_task > task_time)
               min_process_task = task_time;

           if(max_process_task <= task_time)
               max_process_task = task_time;
*/
         }
         if(notify_consumer) cv.notify_one();

         // Update number of finished tasks for throughput calculation
         std::unique_lock<std::mutex> lock(sampling_mutex);
         finished_tasks++;

         // Update data for weighted throughput
         weighted_sum += 1/pow(2, my_wid.depth());
	 lock.unlock();
     }
#ifdef HAVE_EXTRAE
      Extrae_event(6000019, 0);
#endif
   }


   void monitor::global_w_exec_process_finish_wrapper(work_item const& w)
   {
        allscale::monitor::get().w_exec_process_finish_wrapper(w);
   }


   void monitor::w_result_propagated_wrapper(allscale::work_item const& w)
   {
      auto my_wid = w.id();
      std::shared_ptr<allscale::profile> p;

      std::lock_guard<mutex_type> lock(work_map_mutex);
      std::unordered_map<std::string, std::shared_ptr<allscale::profile>>::const_iterator it_profiles = profiles.find(to_string(my_wid));

      if( it_profiles == profiles.end() ) {
         // Work item not created yet
//         p = std::make_shared<profile>();
//         profiles.insert(std::make_pair(my_wid.name(), p));
      }
      else {
         p = it_profiles->second;
         p->result_ready = std::chrono::steady_clock::now();
      }
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
	 history->add_tree_root(to_string(w.id()));
	 history->current_iteration++;
      }
      else
         history->new_iteration(to_string(w.id()));

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
      auto it_g = wi_dependencies.find(w_id);

      if(it_g == wi_dependencies.end()) return 0.0;

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
      auto it = wi_dependencies.find(w_id);

      if(it == wi_dependencies.end()) return 0.0;

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
      auto it = wi_dependencies.find(w_id);

      if(it == wi_dependencies.end()) return 0.0;

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
//       double std = 0.0;

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


  double monitor::get_avg_idle_rate()
  {
      auto now = std::chrono::steady_clock::now();
      std::chrono::duration<double> time_elapsed =
          std::chrono::duration_cast<std::chrono::duration<double>>(now - execution_start);

      return get_avg_task_duration() / time_elapsed.count();


/*     hpx::performance_counters::counter_value idle_avg_value;

     idle_avg_value = hpx::performance_counters::stubs::performance_counter::get_value(
                          hpx::launch::sync, idle_rate_avg_counter_);


     return idle_avg_value.get_value<double>() * 0.01;
      return 0.0;
*/
  }

  double monitor::get_avg_idle_rate_remote(hpx::id_type locality)
  {
/*      get_avg_idle_rate_action act;
      hpx::future<double> f = hpx::async(act, locality);

      return f.get();
*/
      return 0.0;
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

   double monitor::get_iteration_time(std::size_t i)
   {
      std::lock_guard<mutex_type> lock(history_mutex);

      if(i >= history->iteration_time.size()) return 0.0;
      else return history->iteration_time[i];
   }


   double monitor::get_last_iteration_time()
   {
      std::lock_guard<mutex_type> lock(history_mutex);

      if(!(history->iteration_time.size())) return 0.0;
      else return history->iteration_time.back();
   }

   long monitor::get_number_of_iterations()
   {
      std::lock_guard<mutex_type> lock(history_mutex);

      return history->current_iteration;
   }

   double monitor::get_avg_time_last_iterations(std::uint32_t num_iters)
   {
      double avg_time = 0.0;
      std::uint32_t j = 0;
      std::lock_guard<mutex_type> lock(history_mutex);

      for (auto i = history->iteration_time.rbegin();
            i != history->iteration_time.rend() && j < num_iters;
            ++i, ++j)
         avg_time += *i;

      return avg_time / (j + 1);
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
//       double excl_elapsed, incl_elapsed;
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


   void monitor::print_heatmap(const char *file_name, std::vector<std::vector<double>> &buffer)
   {
       std::uint64_t max_sample = 0;
       std::ofstream heatmap;

       heatmap.open(file_name);

       // Get the highest number of samples collected among localitites
       for(const auto &row: buffer)
            if(row.size() >= max_sample) max_sample = row.size();

       // Iterate over the vectors and print the samples
       std::uint64_t locality = 0;

       for(const auto &row: buffer) {
           for(std::uint64_t i = 0; i < max_sample; i++) {

               if( i < row.size() ) heatmap << locality
                                            << "   "
                                            << i
                                            << "   "
                                            << row[i] << "\n";
               else heatmap << locality << "   " << i << "   0.0\n";

           }

           heatmap << "\n"; locality++;
       }

       heatmap.close();
   }


   void monitor::stop() {

      std::vector<hpx::future<void>> stop_futures;
//       std::size_t const os_threads = hpx::get_os_thread_count();
//       std::size_t self = hpx::get_worker_thread_num();


      if(!enable_monitor) return;

      execution_end = std::chrono::steady_clock::now();

      std::chrono::duration<double> total_time_elapsed =
          std::chrono::duration_cast<std::chrono::duration<double>>(execution_end - execution_start);
      wall_clock = total_time_elapsed.count();

      std::cerr << "Stopping monitor with rank " << rank_ << std::endl;

#ifdef REALTIME_VIZ
      timer_.stop();
      data_file.close();
#endif

      if(rank_ == 0)
      {
        dashboard::shutdown();
        std::vector<hpx::future<void>> stop_futures;
        typedef allscale::components::monitor::stop_action stop_action;

	std::vector< std::size_t> ids;
        for(std::size_t i = 1; i < hpx::get_num_localities().get(); i++)
        {
           hpx::future<hpx::id_type> locality_future =
               hpx::find_from_basename("allscale/monitor", i);

	   stop_futures.push_back(hpx::async<stop_action>(locality_future.get()));
        }
      }

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
         // FIXME:
//          shutdown_dashboard_conn();
      }

      cv.notify_one();
      worker_thread.join();

      metric_sampler_->stop();

      // Stop performance counters
      if(memory_counter_registered_)
         hpx::performance_counters::stubs::performance_counter::stop(hpx::launch::sync, resident_memory_counter_);

      if(network_mpi_counters_registered_) {
         hpx::performance_counters::stubs::performance_counter::stop(hpx::launch::sync, nsend_mpi_counter_);
         hpx::performance_counters::stubs::performance_counter::stop(hpx::launch::sync, nrecv_mpi_counter_);
      }

      if(network_tcp_counters_registered_) {
         hpx::performance_counters::stubs::performance_counter::stop(hpx::launch::sync, nsend_tcp_counter_);
         hpx::performance_counters::stubs::performance_counter::stop(hpx::launch::sync, nrecv_tcp_counter_);
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
//         double e_time, i_time, children_mean_time, children_sd;
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
                hpx::lcos::gather_here(gather_basename2, hpx::make_ready_future(wi_dependencies));

           std::vector<dependency_graph> rcv_buffer = f.get();


           for ( auto it = rcv_buffer.begin(); it != rcv_buffer.end(); ++it )
           {
                global_graph.insert((*it).begin(), (*it).end());
           }

        }
        else hpx::lcos::gather_there(gather_basename2, hpx::make_ready_future(wi_dependencies)).wait();

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


      if(print_idle_hm_)
      {
         if(rank_ == 0) {
            hpx::future<std::vector<std::vector<double>>> f =
                hpx::lcos::gather_here(gather_basename3, hpx::make_ready_future(idle_rate_history));

            std::vector<std::vector<double>> rcv_buffer = f.get();

            print_heatmap("idle_rates_hm.dat", rcv_buffer);
         }
         else hpx::lcos::gather_there(gather_basename3, hpx::make_ready_future(idle_rate_history)).wait();
      }

      if(print_throughput_hm_)
      {
         if(rank_ == 0) {
            hpx::future<std::vector<std::vector<double>>> f =
                hpx::lcos::gather_here(gather_basename4, hpx::make_ready_future(throughput_history));

            std::vector<std::vector<double>> rcv_buffer = f.get();

            print_heatmap("throughput_hm.dat", rcv_buffer);
         }
         else hpx::lcos::gather_there(gather_basename4, hpx::make_ready_future(throughput_history)).wait();
      }


      // Wait for all the other localitites
      if(rank_ == 0)
	  hpx::when_all(stop_futures).get();
   }


   void monitor::global_finalize() {
//      (allscale::monitor::get_ptr().get())->monitor_component_finalize();
//     allscale::monitor::get().stop();
   }

   void monitor::init() {
    std::unique_lock<mutex_type> l(init_mutex);
    if (initialized) return;
    hpx::util::ignore_while_checking<std::unique_lock<mutex_type>> il(&l);

//      bool enable_signals = true;
      if(const char* env_p = std::getenv("ALLSCALE_MONITOR"))
      {
          if(atoi(env_p) == 0)
              enable_monitor = false;
      }

      if(!enable_monitor)
      {
         std::cout << "Monitor component disabled!\n";
         initialized = true;
         return;
      }
      num_localities_ = allscale::get_num_localities();


      allscale::monitor::connect(allscale::monitor::work_item_split_execution_started, monitor::global_w_exec_split_start_wrapper);
      allscale::monitor::connect(allscale::monitor::work_item_process_execution_started, monitor::global_w_exec_process_start_wrapper);
      allscale::monitor::connect(allscale::monitor::work_item_split_execution_finished, monitor::global_w_exec_split_finish_wrapper);
      allscale::monitor::connect(allscale::monitor::work_item_process_execution_finished, monitor::global_w_exec_process_finish_wrapper);
//          allscale::monitor::connect(allscale::monitor::work_item_result_propagated, monitor::global_w_result_propagated_wrapper);
      allscale::monitor::connect(allscale::monitor::work_item_first, monitor::global_w_app_iteration);

      // Registering shutdown function
//          hpx::register_shutdown_function(global_finalize);


/*
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
*/
      // Check environment variables
/*      if(const char* env_p = std::getenv("MONITOR_CUTOFF")) {
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

      if(const char* env_p = std::getenv("PRINT_THROUGHPUT_HM"))
         if(atoi(env_p) == 1) print_throughput_hm_ = 1;

      if(const char* env_p = std::getenv("PRINT_IDLE_HM"))
         if(atoi(env_p) == 1) print_idle_hm_ = 1;


      if(const char* env_p = std::getenv("SAMPLING_INTERVAL"))
         sampling_interval_ms = atoll(env_p);


      if(const char* env_p = std::getenv("REALTIME_VIZ"))
         if(atoi(env_p) == 1) {
#ifdef REALTIME_VIZ
               realtime_viz = 1;

	       data_file.open ("realtime_data.txt", std::ofstream::out | std::ofstream::trunc);

               // setup performance counter
               static const char * vm_counter_name = "/runtime{locality#%d/total}/memory/resident";

               const std::uint32_t prefix = hpx::get_locality_id();

               resident_memory_counter_ = hpx::performance_counters::get_counter(
                    boost::str(boost::format(vm_counter_name) % prefix));

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
        (void)env_p;
        std::cerr << "Var. MONITOR_PAPI defined but code compiled without PAPI support!" << std::endl;
#endif
     }




      execution_start = std::chrono::steady_clock::now();
      execution_init = duration_cast<nanoseconds>(system_clock::now().time_since_epoch()).count();


      // Read system info

      // CPUs available and frequencies
      get_cpus_info();

      // Total memory
      FILE *proc_f = fopen("/proc/meminfo", "r");
      fscanf(proc_f, "%*s %llu %*s\n", &total_memory_); total_memory_ *= 1000;
      fclose(proc_f);

      // Read CPU load
      pstat.open("/proc/stat", std::ios::in);
      if(pstat.is_open()) {
	std::string cpu;

        pstat >> cpu >> last_user_time >> last_nice_time >> last_system_time >> last_idle_time;
      }
      else std::cerr << "Unable to open /proc/stat!\n";


      // Create HPX counters
      const std::uint32_t prefix = hpx::get_locality_id();

      // Memory counter
      static const char * memory_counter_name = "/runtime{locality#%d/total}/memory/resident";

      try {
         std::cerr << "Registering counter " << boost::str(boost::format(memory_counter_name) % prefix) << std::endl;
         resident_memory_counter_ = hpx::performance_counters::get_counter(
                       boost::str(boost::format(memory_counter_name) % prefix));

         hpx::performance_counters::stubs::performance_counter::start(hpx::launch::sync, resident_memory_counter_);
         memory_counter_registered_ = 1;
      }
      catch(const std::exception& e) {
	 std::cerr << "Failed to register counter " << boost::str(boost::format(memory_counter_name) % prefix) << std::endl;
         memory_counter_registered_ = 0;
      }

      // Network counters
      static const char * network_sent_mpi_counter_name = "/data{locality#%d/total}/count/mpi/sent";
      static const char * network_recv_mpi_counter_name = "/data{locality#%d/total}/count/mpi/received";
      static const char * network_sent_tcp_counter_name = "/data{locality#%d/total}/count/tcp/sent";
      static const char * network_recv_tcp_counter_name = "/data{locality#%d/total}/count/tcp/received";


      try {
         std::cerr << "Registering counter " << boost::str(boost::format(network_sent_mpi_counter_name) % prefix) << std::endl;
         nsend_mpi_counter_ = hpx::performance_counters::get_counter(
                       boost::str(boost::format(network_sent_mpi_counter_name) % prefix));

         hpx::performance_counters::stubs::performance_counter::start(hpx::launch::sync, nsend_mpi_counter_);
         network_mpi_counters_registered_ = 1;
      }
      catch(...) {
         std::cerr << "Failed to register counter " << boost::str(boost::format(network_sent_mpi_counter_name) % prefix) << std::endl;
         network_mpi_counters_registered_ = 0;
      }

      try {
         std::cerr << "Registering counter " << boost::str(boost::format(network_recv_mpi_counter_name) % prefix) << std::endl;
         nrecv_mpi_counter_ = hpx::performance_counters::get_counter(
                       boost::str(boost::format(network_recv_mpi_counter_name) % prefix));

         hpx::performance_counters::stubs::performance_counter::start(hpx::launch::sync, nrecv_mpi_counter_);
         network_mpi_counters_registered_ &= 1;
      }
      catch(...) {
         std::cerr << "Failed to register counter " << boost::str(boost::format(network_recv_mpi_counter_name) % prefix) << std::endl;
         network_mpi_counters_registered_ &= 0;
      }


      try {
         std::cerr << "Registering counter " << boost::str(boost::format(network_sent_tcp_counter_name) % prefix) << std::endl;
         nsend_tcp_counter_ = hpx::performance_counters::get_counter(
                       boost::str(boost::format(network_sent_tcp_counter_name) % prefix));

         hpx::performance_counters::stubs::performance_counter::start(hpx::launch::sync, nsend_tcp_counter_);
         network_tcp_counters_registered_ = 1;
      }
      catch(...) {
         std::cerr << "Failed to register counter " << boost::str(boost::format(network_sent_tcp_counter_name) % prefix) << std::endl;
         network_tcp_counters_registered_ = 0;
      }

      try {
         std::cerr << "Registering counter " << boost::str(boost::format(network_recv_tcp_counter_name) % prefix) << std::endl;
         nrecv_tcp_counter_ = hpx::performance_counters::get_counter(
                       boost::str(boost::format(network_recv_tcp_counter_name) % prefix));

         hpx::performance_counters::stubs::performance_counter::start(hpx::launch::sync, nrecv_tcp_counter_);
         network_tcp_counters_registered_ &= 1;
      }
      catch(...) {
         std::cerr << "Failed to register counter " << boost::str(boost::format(network_recv_tcp_counter_name) % prefix) << std::endl;
         network_tcp_counters_registered_ &= 0;
      }


      // Create specialised OS-thread to proces profiles and sampler timer
      worker_thread = std::thread(&allscale::components::monitor::process_profiles, this);

      // Start sampling timer
      if(sampling_interval_ms > 0) {

	 metric_sampler_ = std::make_unique<hpx::util::interval_timer>(
					hpx::util::bind(&monitor::sample_node, this),
					sampling_interval_ms*1000,
					"node_sampler",
					false);
//         metric_sampler_.change_interval(sampling_interval_ms*1000);
         metric_sampler_->start();
      }


      // Create the profile for the "Main"
      std::shared_ptr<allscale::profile> p(new profile("0", "Main", " "));
      std::string mystring("0");

      profiles.insert(std::make_pair(mystring, p));
//      w_names.insert(std::make_pair(mystring, std::string("Main")));

      // Insert the root "0" in the graph
      wi_dependencies.insert(std::make_pair(std::string("0"), std::vector<std::string>()));

      // Init historical data
      history = std::make_shared<allscale::historical_data>();

      hpx::register_with_basename("allscale/monitor", this->get_id(), rank_).get();


      initialized = true;

      std::cerr
          << "Monitor component with rank "
          << rank_ << " created!\n";

      if (rank_ == 0)
      {
          dashboard::update();
          dashboard::get_commands();
      }

   }




}}

//HPX_REGISTER_ACTION(allscale::components::monitor::get_my_rank_action, get_my_rank_action);
//HPX_REGISTER_ACTION(allscale::components::monitor::stop_action, stop_action);
HPX_REGISTER_ACTION(allscale::components::monitor::get_exclusive_time_action, get_exclusive_time_action);
HPX_REGISTER_ACTION(allscale::components::monitor::get_inclusive_time_action, get_inclusive_time_action);
HPX_REGISTER_ACTION(allscale::components::monitor::get_average_exclusive_time_action, get_average_exclusive_time_action);
HPX_REGISTER_ACTION(allscale::components::monitor::get_minimum_exclusive_time_action, get_minimum_exclusive_time_action);
HPX_REGISTER_ACTION(allscale::components::monitor::get_maximum_exclusive_time_action, get_maximum_exclusive_time_action);
HPX_REGISTER_ACTION(allscale::components::monitor::get_children_mean_time_action, get_children_mean_time_action);
HPX_REGISTER_ACTION(allscale::components::monitor::get_children_SD_time_action, get_children_SD_time_action);
HPX_REGISTER_ACTION(allscale::components::monitor::get_idle_rate_action, get_idle_rate_action);
//HPX_REGISTER_ACTION(allscale::components::monitor::get_avg_idle_rate_action, get_avg_idle_rate_action);
HPX_REGISTER_ACTION(allscale::components::monitor::get_throughput_action, get_throughput_action);
