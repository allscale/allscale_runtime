
#include <allscale/components/scheduler.hpp>
#include <allscale/components/monitor.hpp>
#include <allscale/monitor.hpp>
#include <allscale/resilience.hpp>
// #include <allscale/util/hardware_reconf.hpp>

#include <hpx/util/scoped_unlock.hpp>
#include <hpx/traits/executor_traits.hpp>
#include <iterator>
#include <algorithm>
#include <stdlib.h>

namespace allscale { namespace components {

    scheduler::scheduler(std::uint64_t rank)
      : num_localities_(hpx::get_num_localities().get())
      , num_threads_(hpx::get_num_worker_threads())
      , rank_(rank)
      , stopped_(false)
      , os_thread_count(hpx::get_os_thread_count())
      , active_threads(os_thread_count)
      , depth_cap(1.5 * (std::log(os_thread_count)/std::log(2) + 0.5))
      , current_avg_iter_time(0.0)
      , sampling_interval(10)
      , time_leeway(1.0)
      , min_threads(4)
	  , current_energy_usage(0)
      , last_actual_energy_usage(0)
      , actual_energy_usage(0)
//       , count_(0)
      , timer_(
            hpx::util::bind(
                &scheduler::collect_counters,
                this
            ),
            700,
            "scheduler::collect_counters",
            true
        )
      , throttle_timer_(
           hpx::util::bind(
               &scheduler::periodic_throttle,
               this
           ),
           100000, //0.1 sec
           "scheduler::periodic_throttle",
           true
        )
    {
        allscale_monitor = &allscale::monitor::get();
        thread_times.resize(hpx::get_os_thread_count());
    }

    void scheduler::init()
    {
        rp_ = &hpx::resource::get_partitioner();
        topo_ = &hpx::threads::get_topology();

// FIXME
//         auto const& numa_domains_ = rp_->numa_domains();
//         for (std::size_t i = 0; i < numa_domains_.size(); ++i)
//         {
//             rp_->create_thread_pool("allscale/numa/" + std::to_string(i));
//         }

        input_objective = hpx::get_config_entry("allscale.objective", "");

        if (!input_objective.empty())
        {
            if ( std::find(scheduler::objectives.begin(), scheduler::objectives.end(), input_objective) == scheduler::objectives.end() ) {
                std::ostringstream all_keys;
                copy(scheduler::objectives.begin(), scheduler::objectives.end(), std::ostream_iterator<std::string>(all_keys, ","));
                std::string keys_str = all_keys.str();
                keys_str.pop_back();
                HPX_THROW_EXCEPTION(hpx::bad_request, "scheduler::init",
                        boost::str(boost::format("Wrong objective: %s, Valid values: [%s]") % input_objective % keys_str));
            }
            else
                std::cerr << "The requested objective is " << input_objective << std::endl;
        }

        // setup performance counter to use to decide on split/process
        static const char * queue_counter_name = "/threadqueue{locality#%d/total}/length";
//         static const char * idle_counter_name = "/threads{locality#%d/total}/idle-rate";
//         static const char * allscale_app_counter_name = "/allscale{locality#*/total}/examples/app_time";
        //static const char * threads_time_total = "/threads{locality#*/total}/time/overall";

        const std::uint32_t prefix = hpx::get_locality_id();

        queue_length_counter_ = hpx::performance_counters::get_counter(
            boost::str(boost::format(queue_counter_name) % prefix));

        hpx::performance_counters::stubs::performance_counter::start(
            hpx::launch::sync, queue_length_counter_);

//         idle_rate_counter_ = hpx::performance_counters::get_counter(
//             boost::str(boost::format(idle_counter_name) % prefix));

//         hpx::performance_counters::stubs::performance_counter::start(hpx::launch::sync, idle_rate_counter_);

        collect_counters();

        numa_domains = hpx::compute::host::numa_domains();
        auto num_numa_domains = numa_domains.size();
        executors.reserve(num_numa_domains);
        for (hpx::compute::host::target const & domain : numa_domains) {
            auto num_pus = domain.num_pus();
            //Create executor per numa domain
            executors.emplace_back(num_pus.first, num_pus.second);
            std::cerr << "Numa num_pus.first: " << num_pus.first << ", num_pus.second: " << num_pus.second << ". Total numa domains: " << numa_domains.size() << std::endl;
        }
//         timer_.start();

        if (!input_objective.empty() ) {
            thread_scheduler = dynamic_cast<hpx::threads::policies::throttling_scheduler<>*>(hpx::resource::get_thread_pool(0).get_scheduler());
            if (thread_scheduler != nullptr) {
               std::cout << "We have a thread manager holding the throttling_scheduler" << std::endl;
            } else {
               HPX_THROW_EXCEPTION(hpx::bad_request, "scheduler::init",
            "thread_scheduler is null. Make sure you select throttling scheduler via --hpx:queuing=throttling");
            }

            if (input_objective == "energy")
            {
                using hardware_reconf = allscale::components::util::hardware_reconf;
                cpu_freqs = hardware_reconf::get_frequencies(0);
                auto min_max_freqs = std::minmax_element(cpu_freqs.begin(), cpu_freqs.end());
                unsigned long min_freq = *min_max_freqs.first;
                unsigned long max_freq = *min_max_freqs.second;

                std::string governor = "userspace";
                policy.governor = const_cast<char*>(governor.c_str());
                policy.min = min_freq;
                policy.max = max_freq;

                topo = hardware_reconf::read_hw_topology();
                for (int cpu_id = 0; cpu_id < topo.num_logical_cores; cpu_id += topo.num_hw_threads)
                    int res = hardware_reconf::set_freq_policy(cpu_id, policy);
            }

// 	    throttle_timer_.start();
        }

        std::cerr
            << "Scheduler with rank " << rank_ << " created!\n";
    }

    void scheduler::enqueue(work_item work, this_work_item::id const& id)
    {
        if (id)
            this_work_item::set_id(id);

        std::uint64_t schedule_rank = work.id().rank();
        if(!allscale::resilience::rank_running(schedule_rank))
        {
            schedule_rank = rank_;
        }

        HPX_ASSERT(work.valid());

        HPX_ASSERT(schedule_rank != std::uint64_t(-1));

#ifdef DEBUG_
        std::cout << "Will schedule task " << work.id().name() << " on rank " << right_rank_ << std::endl;
#endif

        // schedule locally
        if (schedule_rank == rank_)
        {
            if (work.is_first())
            {
                std::size_t current_id = work.id().last();
                const char* wi_name = work.name();
                {
                    std::unique_lock<mutex_type> lk(spawn_throttle_mtx_);
                    auto it = spawn_throttle_.find(wi_name);
                    if (it == spawn_throttle_.end())
                    {
                        auto em_res = spawn_throttle_.emplace(wi_name,
                            treeture_buffer(4));//num_threads_));
                        it = em_res.first;
                    }
                    it->second.add(std::move(lk), work.get_treeture());
                }
                allscale::monitor::signal(allscale::monitor::work_item_first, work);

                if (current_id % 100)
                {
                    periodic_throttle();
                }
            }
            else
            {
            }
            allscale::monitor::signal(allscale::monitor::work_item_enqueued, work);

            auto execute = [this](work_item work, this_work_item::id const& id = this_work_item::id())
            {
                if (id)
                    this_work_item::set_id(id);
                if (do_split(work))
                {
                    work.split();
                }
                else
                {
                    work.process();
                }
            };

            std::size_t numa_domain = work.id().numa_domain();
            std::size_t pu_num = rp_->get_pu_num(hpx::get_worker_thread_num());
            std::size_t my_numa_domain = topo_->get_numa_node_number(pu_num);
            if (numa_domain == my_numa_domain)
            {
                execute(std::move(work));
            }
            // Dispatch to another numa domain
            else
            {
                HPX_ASSERT(numa_domain < executors.size());
                hpx::parallel::execution::post(
                    executors[numa_domain], execute, std::move(work), id);
            }

            return;
        }

        network_.schedule(schedule_rank, std::move(work), this_work_item::get_id());
    }

    bool scheduler::do_split(work_item const& w)
    {
        // FIXME: make the cut off runtime configurable...
        if (!w.id().splittable()) return false;

        if (!w.can_split()) return false;

        return true;

        // FIXME: this doesn't really work efficiently as of now. revisit later...
        // the above works fine for now.
        //FIXME: think about if locking
        //counters_mtx_ could lead to a potential dead_lock situatione
        //when more than one enque action is active (this can be the case due
        //to hpx apply), and the thread holding the lock is suspended and the
        //others start burning up cpu time by spinning to get the lock
//         std::unique_lock<mutex_type> l(counters_mtx_);
//         // Do we have enough tasks in the system?
//         if (queue_length_ < num_threads_ * 10 )
//         {
// //        	std::cout<<"not enough tasks and total_idlerate: " << total_idle_rate_ << " queue length " << total_length_ << std::endl;
//         	//TODO: Think of some smart way to solve this, as of now, having new split workitems spawned
//         	// in system that does not have many tasks with idle_Rate>=x and x being to low can lead to endless loops:
//         	// as new items get spawned, they are too fine  granular, leading to them being not further processed,but
//         	// due to short queue length and too low idle rate requirement for NOT splitting anymore, splitting keeps going on
//             return idle_rate_ >= 10.0;
//         }
// //    	std::cout<<"enough tasks and total_idlerate: " << total_idle_rate_ << " queue length " << total_length_ << std::endl;
//
//         return idle_rate_ < 10.0;
    }

    bool scheduler::collect_counters()
    {
//         hpx::performance_counters::counter_value idle_value;
        hpx::performance_counters::counter_value length_value;
//         idle_value = hpx::performance_counters::stubs::performance_counter::get_value(
//                 hpx::launch::sync, idle_rate_counter_);
        length_value = hpx::performance_counters::stubs::performance_counter::get_value(
                hpx::launch::sync, queue_length_counter_);

//         double idle_rate = idle_value.get_value<double>() * 0.01;
        std::size_t queue_length = length_value.get_value<std::size_t>();

        {
            std::unique_lock<mutex_type> l(counters_mtx_);

//             idle_rate_ = idle_rate;
            queue_length_ = queue_length;
        }

        return true;
    }


    bool scheduler::periodic_throttle()
    {
        if ( num_threads_ > 1 && !input_objective.empty())
        {
            std::unique_lock<mutex_type> l(resize_mtx_);
            if ( current_avg_iter_time == 0.0 || allscale_monitor->get_number_of_iterations() < sampling_interval)
            {
                {
                    hpx::util::unlock_guard<std::unique_lock<mutex_type>> ul(l);
//                    current_avg_iter_time = allscale_monitor->get_avg_work_item_times(sampling_interval);
                    current_avg_iter_time = allscale_monitor->get_avg_time_last_iterations(sampling_interval);
                }
                return true;
            } else if ( current_avg_iter_time > 0 )
            {
                last_avg_iter_time = current_avg_iter_time;

                {
                    hpx::util::unlock_guard<std::unique_lock<mutex_type>> ul(l);
//                    current_avg_iter_time = allscale_monitor->get_avg_work_item_times(sampling_interval);
                    current_avg_iter_time = allscale_monitor->get_avg_time_last_iterations(sampling_interval);
                }

                boost::dynamic_bitset<> const & blocked_os_threads_ =
                    thread_scheduler->get_disabled_os_threads();
                active_threads = os_thread_count - blocked_os_threads_.count();

                unsigned thread_use_count = thread_times[active_threads - 1].second;
                double thread_exe_time = current_avg_iter_time + thread_times[active_threads - 1].first;
                thread_times[active_threads - 1] = std::make_pair(thread_exe_time, thread_use_count + 1);

                std::size_t suspend_cap = 1; //active_threads < SMALL_SYSTEM  ? SMALL_SUSPEND_CAP : LARGE_SUSPEND_CAP;
                std::size_t resume_cap = 1;  //active_threads < SMALL_SYSTEM  ? LARGE_RESUME_CAP : SMALL_RESUME_CAP;

                double time_threshold = current_avg_iter_time; 
                bool disable_flag = last_avg_iter_time >= time_threshold;
                bool enable_flag = last_avg_iter_time * time_leeway < time_threshold;
        

                if (input_objective == scheduler::objectives.at(objective_IDs::TIME_RESOURCE))
                {
                    // If we have a sublinear speedup then prefer resources over time and throttle
                    time_threshold = current_avg_iter_time * (active_threads / (active_threads - suspend_cap)) * 1.2;
                    disable_flag = last_avg_iter_time <= time_threshold;
                    enable_flag = last_avg_iter_time > time_threshold;
                    min_threads = 1;
                }


                if ( active_threads > min_threads && disable_flag )
                {
                    depth_cap = (1.5 * (std::log(active_threads)/std::log(2) + 0.5));
                    {
                        hpx::util::unlock_guard<std::unique_lock<mutex_type> > ul(l);
                        thread_scheduler->disable_more(suspend_cap);
                    }
                    std::cout << "Sent disable signal. Active threads: " << active_threads - suspend_cap << std::endl;
                }
                else if ( blocked_os_threads_.any() && enable_flag )
                {
                    depth_cap = (1.5 * (std::log(active_threads)/std::log(2) + 0.5));
                    if (time_leeway < 1.01)
                        time_leeway *= 1.0005;
                    {
                        hpx::util::unlock_guard<std::unique_lock<mutex_type> > ul(l);
                        thread_scheduler->enable_more(resume_cap);
                    }
                    std::cout << "Sent enable signal. Active threads: " << active_threads + resume_cap << std::endl;
                //    std::cout << "LEEWAY: " << time_leeway << std::endl;
                }
            }

        }

        return true;
    }


    bool scheduler::periodic_frequency_scale()
    {
        {
            std::unique_lock<mutex_type> l(resize_mtx_);
            if ( current_energy_usage == 0 )
            {
                current_energy_usage = hardware_reconf::read_system_energy();
                cpu_to_freq_scale = 0;

                return true;
            } else if ( current_energy_usage > 0 )
            {
                last_energy_usage = current_energy_usage;
                current_energy_usage = hardware_reconf::read_system_energy();
                last_actual_energy_usage = actual_energy_usage;
                actual_energy_usage = current_energy_usage - last_energy_usage;

                if ( last_actual_energy_usage >= actual_energy_usage )
                {
                    {
                        hpx::util::unlock_guard<std::unique_lock<mutex_type> > ul(l);
                        int res = hardware_reconf::set_frequency(cpu_to_freq_scale, cpu_to_freq_scale + 1, cpu_freqs[0]);
                        std::cout << "Increase frequency. res: " << res << ", last_actual_energy_usage: "
                            << last_actual_energy_usage << ", actual_energy_usage: " << actual_energy_usage << ", cpu_to_freq_scale: " << cpu_to_freq_scale << std::endl;
                    }
                    cpu_to_freq_scale++;
                }
                else if ( last_actual_energy_usage < actual_energy_usage )
                {
                    {
                        cpu_to_freq_scale++;
                        hpx::util::unlock_guard<std::unique_lock<mutex_type> > ul(l);
                        int res = hardware_reconf::set_frequency(cpu_to_freq_scale, cpu_to_freq_scale + 1, cpu_freqs[3]);
                        std::cout << "Decrease frequency. res: " << res << ", last_actual_energy_usage: "
                             << last_actual_energy_usage << ", actual_energy_usage: " << actual_energy_usage << ", cpu_to_freq_scale: " << cpu_to_freq_scale << std::endl;
                    }
                }
            }
        }

        return true;
    }
 


    void scheduler::stop()
    {
//         timer_.stop();

//         if (input_objective == scheduler::objectives.at(objective_IDs::TIME_RESOURCE))
//             throttle_timer_.stop();

//        for (int i = 0; i < thread_times.size(); i++)
//        {
//            if (thread_times[i].first > 0)
//                std::cout << "times[" << i << "] = " << thread_times[i].first << ", " << thread_times[i].second << std::endl;
//        }


        if(stopped_)
            return;

        //Resume all sleeping threads
        if (!input_objective.empty())
       	    thread_scheduler->enable_more(os_thread_count);

        stopped_ = true;
//         work_queue_cv_.notify_all();
//         std::cout << "rank(" << rank_ << "): scheduled " << count_ << "\n";

        network_.stop();
    }

}}
