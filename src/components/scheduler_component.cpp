
#include <allscale/components/scheduler.hpp>
#include <allscale/components/monitor.hpp>
#include <allscale/monitor.hpp>
#include <allscale/resilience.hpp>

#include <hpx/util/scoped_unlock.hpp>
#include <hpx/traits/executor_traits.hpp>

#include <sstream>
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
      , enable_factor(1.0)
      , disable_factor(1.0)
      , min_threads(4)
	  , current_energy_usage(0)
      , last_actual_energy_usage(0)
      , actual_energy_usage(0)
      #if defined(ALLSCALE_HAVE_CPUFREQ)
      , target_freq_found(false)
      #endif
      , time_requested(false)
      , resource_requested(false)
      , energy_requested(false)
      , time_leeway(1.0)
      , resource_leeway(1.0)
      , energy_leeway(1.0)
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
      , frequency_timer_(
           hpx::util::bind(
               &scheduler::periodic_frequency_scale,
               this
           ),
           10000000, //0.1 sec
           "scheduler::periodic_frequency_scale",
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

        std::string input_objective_str = hpx::get_config_entry("allscale.objective", "");

        if ( !input_objective_str.empty() )
        {
            std::istringstream iss_leeways(input_objective_str);
            std::string objective_str;
            while (iss_leeways >> objective_str)
            {
                auto idx = objective_str.find(':');
                std::string obj = objective_str;
                // Don't scale objectives if none is given
                double leeway = 1.0;

                if (idx != std::string::npos)
                {
                    obj = objective_str.substr(0, idx);
                    leeway = std::stod( objective_str.substr(idx + 1) );
                }

                if (obj == "time")
                {
                    time_requested = true;
                    time_leeway = leeway;
                }
                else if (obj == "resource")
                {
                    resource_requested = true;
                    resource_leeway = leeway;
                }
                else if (obj == "energy")
                {
                    energy_requested = true;
                    energy_leeway = leeway;
                }
                else
                {
                    std::ostringstream all_keys;
                    copy(scheduler::objectives.begin(), scheduler::objectives.end(), std::ostream_iterator<std::string>(all_keys, ","));
                    std::string keys_str = all_keys.str();
                    keys_str.pop_back();
                    HPX_THROW_EXCEPTION(hpx::bad_request, "scheduler::init",
                            boost::str(boost::format("Wrong objective: %s, Valid values: [%s]") % obj % keys_str));
                }

                if ( time_leeway > 1 || resource_leeway > 1 || energy_leeway > 1 )
                {
                    HPX_THROW_EXCEPTION(hpx::bad_request, "scheduler::init", "leeways should be within [0, 1]");
                }

//                objectives_with_leeways.push_back(std::make_pair(obj, leeway));
            }

            if ( resource_requested && energy_requested )
                HPX_THROW_EXCEPTION(hpx::bad_request, "scheduler::init",
                            boost::str(boost::format("Sorry not supported yet. Check back soon!")));
        }


        // setup performance counter to use to decide on split/process
        static const char * queue_counter_name = "/threadqueue{locality#%d/total}/length";

        const std::uint32_t prefix = hpx::get_locality_id();

        queue_length_counter_ = hpx::performance_counters::get_counter(
            boost::str(boost::format(queue_counter_name) % prefix));

        hpx::performance_counters::stubs::performance_counter::start(
            hpx::launch::sync, queue_length_counter_);


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

        if ( time_requested || resource_requested || energy_requested ) {
            // TODO for frequency scaling we don't actually need throttling policy.
            thread_scheduler = dynamic_cast<hpx::threads::policies::throttling_scheduler<>*>(hpx::resource::get_thread_pool(0).get_scheduler());
            if (thread_scheduler != nullptr) {
               std::cout << "We have a thread manager holding the throttling_scheduler" << std::endl;
            } else {
               HPX_THROW_EXCEPTION(hpx::bad_request, "scheduler::init",
                    "thread_scheduler is null. Make sure you select throttling scheduler via --hpx:queuing=throttling");
            }

//            if ( time_requested || resource_requested )
//                throttle_timer_.start();

            if ( energy_requested )
            {
#if defined(ALLSCALE_HAVE_CPUFREQ)
                using hardware_reconf = allscale::components::util::hardware_reconf;
                cpu_freqs = hardware_reconf::get_frequencies(0);
                freq_step = 8; //cpu_freqs.size() / 2;
                freq_times.resize(cpu_freqs.size());

                auto min_max_freqs = std::minmax_element(cpu_freqs.begin(), cpu_freqs.end());
                min_freq = *min_max_freqs.first;
                max_freq = *min_max_freqs.second;

                // We have to set CPU governors to userpace in order to change frequencies later
                std::string governor = "userspace";
                policy.governor = const_cast<char*>(governor.c_str());
                policy.min = min_freq;
                policy.max = max_freq;

                topo = hardware_reconf::read_hw_topology();
                for (int cpu_id = 0; cpu_id < topo.num_logical_cores; cpu_id += topo.num_hw_threads)
                    int res = hardware_reconf::set_freq_policy(cpu_id, policy);

                // Set frequency of all threads to max when we start
                int res = hardware_reconf::set_frequency(0, os_thread_count, cpu_freqs[0]);

                std::this_thread::sleep_for(std::chrono::microseconds(2));

                // Make sure frequency change happened before continuing
                for (int cpu_id = 0; cpu_id < topo.num_logical_cores; cpu_id += topo.num_hw_threads)
                {
                    unsigned long hardware_freq = hardware_reconf::get_hardware_freq(cpu_id);
                    HPX_ASSERT(hardware_freq == cpu_freqs[0]);
                }


                frequency_timer_.start();
#else
                HPX_THROW_EXCEPTION(hpx::bad_request, "scheduler::init",
                            "Requesting energy objective without having compiled with cpufreq");
#endif
            }

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
        std::cout << "Will schedule task " << work.id().name() << " on rank " << schedule_rank << std::endl;
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
                            treeture_buffer(6));//num_threads_));
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
        // Check if the work item is splittable first
        if (w.can_split())
        {
            // Check if we reached the required depth
            // FIXME: make the cut off runtime configurable...
            if (w.id().splittable())
            {
                // FIXME: add more elaborate splitting criterions
                return true;
            }
            return false;
        }
        // return false if it isn't
        return false;

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
        if ( num_threads_ > 1 && ( time_requested || resource_requested ) )
        {
            std::unique_lock<mutex_type> l(resize_mtx_);
            if ( current_avg_iter_time == 0.0 || allscale_monitor->get_number_of_iterations() < sampling_interval)
            {
                {
                    hpx::util::unlock_guard<std::unique_lock<mutex_type>> ul(l);
                    current_avg_iter_time = allscale_monitor->get_avg_time_last_iterations(sampling_interval);
//                    current_avg_iter_time = allscale_monitor->get_last_iteration_time();
//                    current_avg_iter_time = allscale_monitor->get_avg_work_item_times(sampling_interval);
                }
                return true;
            } else if ( current_avg_iter_time > 0 )
            {
                last_avg_iter_time = current_avg_iter_time;

                {
                    hpx::util::unlock_guard<std::unique_lock<mutex_type>> ul(l);
//                    current_avg_iter_time = allscale_monitor->get_last_iteration_time();
                    current_avg_iter_time = allscale_monitor->get_avg_time_last_iterations(sampling_interval);
//                    current_avg_iter_time = allscale_monitor->get_avg_work_item_times(sampling_interval);
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
                bool disable_flag = last_avg_iter_time > time_threshold;
                bool enable_flag = last_avg_iter_time * enable_factor < time_threshold;


                if ( resource_requested )
                {
                    // If we have a sublinear speedup then prefer resources over time and throttle
                    time_threshold = current_avg_iter_time * (active_threads - suspend_cap ) / active_threads;
                    disable_flag = last_avg_iter_time > time_threshold;
                    enable_flag = last_avg_iter_time < time_threshold;
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
                    if ( active_threads < topo_->get_number_of_pus() / topo_->get_number_of_cores() + min_threads &&  enable_factor < 1.01 )
                        enable_factor *= 1.0005;


                    {
                        hpx::util::unlock_guard<std::unique_lock<mutex_type> > ul(l);
                        thread_scheduler->enable_more(resume_cap);
                    }
                    std::cout << "Sent enable signal. Active threads: " << active_threads + resume_cap << std::endl;
                }
            }
        }

        return true;
    }


    bool scheduler::periodic_frequency_scale()
    {
#if defined(ALLSCALE_HAVE_CPUFREQ)
        std::unique_lock<mutex_type> l(resize_mtx_);
        if ( !target_freq_found )
        {
            if ( current_energy_usage == 0 )
            {
                current_energy_usage = hardware_reconf::read_system_energy();
                return true;
            } else if ( current_energy_usage > 0 )
            {
                last_energy_usage = current_energy_usage;
                current_energy_usage = hardware_reconf::read_system_energy();
                last_actual_energy_usage = actual_energy_usage;
                actual_energy_usage = current_energy_usage - last_energy_usage;

                last_avg_iter_time = current_avg_iter_time;
                {
                    hpx::util::unlock_guard<std::unique_lock<mutex_type>> ul(l);
//                    current_avg_iter_time = allscale_monitor->get_avg_time_last_iterations(sampling_interval);
                    current_avg_iter_time = allscale_monitor->get_last_iteration_time();
                }

                unsigned int freq_idx = -1;
                unsigned long current_freq_hw = hardware_reconf::get_hardware_freq(0);
                //unsigned long current_freq_kernel = hardware_reconf::get_kernel_freq(0);
                //std::cout << "current_freq_hw: " << current_freq_hw << ", current_freq_kernel: " << current_freq_kernel << std::endl;

                // Get freq index in cpu_freq
                std::vector<unsigned long>::iterator it = std::find(cpu_freqs.begin(), cpu_freqs.end(), current_freq_hw);
                if ( it != cpu_freqs.end() )
                    freq_idx = it - cpu_freqs.begin();
                else
                {
                    // If you run it without sudo, get_hardware_freq will fail and end up here as well!
                    HPX_THROW_EXCEPTION(hpx::bad_request, "scheduler::periodic_frequency_scale",
                        boost::str(boost::format("Cannot find frequency: %s in the list of frequencies. Something must be wrong!") % current_freq_hw));
                }

                freq_times[freq_idx] = std::make_pair(actual_energy_usage, current_avg_iter_time);

                unsigned long target_freq = current_freq_hw;
                // If we have not finished until the minimum frequnecy then continue
                if ( target_freq != min_freq ) {
                    hpx::util::unlock_guard<std::unique_lock<mutex_type> > ul(l);
                    //Start decreasing frequencies by 2
                    hardware_reconf::set_next_frequency(freq_step, true);
                    target_freq = hardware_reconf::get_hardware_freq(0);
                    std::cout << "Decrease frequency. " << "actual_energy_usage: "
                         << actual_energy_usage << ", current_freq_hw: "
                         << current_freq_hw << ", target_freq: "
                         << target_freq << ", current_avg_iter_time: "
                         << current_avg_iter_time << std::endl;
                }
                else
                {   // We should have measurement with all frequencies with step 2
                    // End of freq_times contains minimum frequency
                    unsigned long long min_energy = freq_times.back().first;
                    unsigned int min_energy_idx = freq_times.size() - 1;

                    double min_exec_time = freq_times[0].second;
                    unsigned int min_exec_time_idx = 0;

                    for (int i = 0; i < freq_times.size(); i++)
                    {
                        // If we have frequencies with zero energy usage
                        // it means we haven't measured them, so skip them
                        if ( freq_times[i].first == 0 )
                            continue;

                        if ( min_energy > freq_times[i].first )
                        {
                            min_energy = freq_times[i].first;
                            min_energy_idx = i;
                        }

                        if ( min_exec_time > freq_times[i].second )
                        {
                            min_exec_time = freq_times[i].second;
                            min_exec_time_idx = i;
                        }
                    }

                    // We will save minimum energy and execution time
                    // and use them for comparision using leeways
                    unsigned long long optimal_energy = min_energy;
                    double optimal_exec_time = min_exec_time;

                    if ( time_requested && energy_requested )
                    {
                        for (int i = 0; i < freq_times.size(); i++)
                        {
                            // the frequencies that have not been used will have default value of zero
                            if ( freq_times[i].first == 0 )
                                continue;

                            if ( ( time_leeway < 1 && energy_leeway == 1.0 ) && freq_times[i].second  - min_exec_time <  min_exec_time * time_leeway )
                            {
                                if ( optimal_energy > freq_times[i].first )
                                {
                                    min_energy_idx = i;
                                    min_exec_time_idx = i;
                                    optimal_energy = freq_times[i].first;
                                }
                            }
                            else if ( ( energy_leeway < 1 && time_leeway == 1.0 ) && freq_times[i].first  - min_energy <  min_energy * energy_leeway  )
                            {

                                if ( optimal_exec_time > freq_times[i].second )
                                {
                                    min_energy_idx = i;
                                    min_exec_time_idx = i;
                                    optimal_exec_time = freq_times[i].second;
                                }
                            }
                            else if ( time_leeway == 1.0 && energy_leeway == 1.0 )
                            {
                                if ( optimal_energy > freq_times[i].first && optimal_exec_time > freq_times[i].second )
                                {
                                    optimal_energy = freq_times[i].first;
                                    min_energy_idx = i;
                                    min_exec_time_idx = i;
                                    optimal_exec_time = freq_times[i].second;
                                }
                            }
                        }
                    }

                    hardware_reconf::set_frequencies_bulk(os_thread_count, cpu_freqs[min_energy_idx]);
                    target_freq_found = true;
                    std::cout << "Min energy: " << freq_times[min_energy_idx].first << " with freq: "
                              << cpu_freqs[min_energy_idx] << ", Min time with freq: "
                              << cpu_freqs[min_exec_time_idx] << std::endl;
                }
            }
        }
#endif

        return true;
    }



    void scheduler::stop()
    {
//         timer_.stop();

//        if ( time_requested || resource_requested )
//            throttle_timer_.stop();

        if ( energy_requested )
            frequency_timer_.stop();

        if(stopped_)
            return;

        //Resume all sleeping threads
        if ( ( time_requested || resource_requested ) && !energy_requested)
        {
       	    thread_scheduler->enable_more(os_thread_count);

            double resource_usage = 0;
            for (int i = 0; i < thread_times.size(); i++)
            {
                resource_usage += thread_times[i].first * (i + 1);
            }

            std::cout << "Resource usage: " << resource_usage << std::endl;
        }

        if ( energy_requested )
        {
#if defined(ALLSCALE_HAVE_CPUFREQ)
            std::string governor = "ondemand";
            policy.governor = const_cast<char*>(governor.c_str());

            for (int cpu_id = 0; cpu_id < topo.num_logical_cores; cpu_id += topo.num_hw_threads)
                int res = hardware_reconf::set_freq_policy(cpu_id, policy);

            std::cout << "Set CPU governors back to " << governor << std::endl;
#endif
        }

        stopped_ = true;
//         work_queue_cv_.notify_all();
//         std::cout << "rank(" << rank_ << "): scheduled " << count_ << "\n";

        network_.stop();
    }

}}
