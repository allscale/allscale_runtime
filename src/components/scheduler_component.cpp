
#include <allscale/components/scheduler.hpp>
#include <allscale/components/monitor.hpp>
#include <allscale/monitor.hpp>
#include <allscale/resilience.hpp>

#include <hpx/util/unlock_guard.hpp>
#include <hpx/traits/executor_traits.hpp>

#include <boost/format.hpp>

#include <sstream>
#include <iterator>
#include <algorithm>
#include <stdlib.h>
#include <math.h>
#include <limits>

namespace allscale { namespace components {

        scheduler::scheduler(std::uint64_t rank)
            : rank_(rank)
            , initialized_(false)
            , stopped_(false)
            , os_thread_count(hpx::get_os_thread_count())
            , active_threads(os_thread_count)
            , current_avg_iter_time(0.0)
            , sampling_interval(10)
            , enable_factor(1.01)
            , disable_factor(1.01)
            , growing(true)
            , min_threads(1)
            , max_resource(0)
            , max_time(0)
            , max_power(0)
            , current_energy_usage(0)
            , last_actual_energy_usage(0)
            , actual_energy_usage(0)
            , current_power_usage(0)
            , last_power_usage(0)
            , power_sum(0)
            , power_count(0)
#if defined(ALLSCALE_HAVE_CPUFREQ)
            , target_freq_found(false)
#endif
            , target_resource_found(false)
            , resource_step(1)
            , multi_objectives(false)
            , time_requested(false)
            , resource_requested(false)
            , energy_requested(false)
            , time_leeway(1.0)
            , resource_leeway(1.0)
            , energy_leeway(1.0)
            , period_for_time(10)
            , period_for_resource(10)
            , period_for_power(20)
            , throttle_timer_(
                              hpx::util::bind(
                                              &scheduler::periodic_throttle,
                                              this
                                              ),
                              1000000, //1 sec
                              "scheduler::periodic_throttle",
                              true
                              )
            , frequency_timer_(
                               hpx::util::bind(
                                               &scheduler::power_periodic_frequency_scale,
                                               this
                                               ),
                               10000000, //1 sec
                               "scheduler::power_periodic_frequency_scale",
                               true
                               )
            , multi_objectives_timer_(
                               hpx::util::bind(
                                               &scheduler::multi_objectives_adjust_timed,
                                               this
                                               ),
                               10000000, //1 sec
                               "scheduler::multi_objectives_adjust",
                               true
                               )
        {
            allscale_monitor = &allscale::monitor::get();
            thread_times.resize(hpx::get_os_thread_count());
        }

        std::size_t scheduler::get_num_numa_nodes()
        {
            std::size_t numa_nodes = topo_->get_number_of_numa_nodes();
            if (numa_nodes == 0)
                numa_nodes = topo_->get_number_of_sockets();
            std::vector<hpx::threads::mask_type> node_masks(numa_nodes);

            std::size_t num_os_threads = hpx::get_os_thread_count();
            for (std::size_t num_thread = 0; num_thread != num_os_threads;
                 ++num_thread)
                {
                    std::size_t pu_num = rp_->get_pu_num(num_thread);
                    std::size_t numa_node = topo_->get_numa_node_number(pu_num);

                    auto const& mask = topo_->get_thread_affinity_mask(pu_num);

                    std::size_t mask_size = hpx::threads::mask_size(mask);
                    for (std::size_t idx = 0; idx != mask_size; ++idx)
                        {
                            if (hpx::threads::test(mask, idx))
                                {
                                    hpx::threads::set(node_masks[numa_node], idx);
                                }
                        }
                }

            // Sort out the masks which don't have any bits set
            std::size_t res = 0;

            for (auto& mask : node_masks)
                {
                    if (hpx::threads::any(mask))
                        {
                            ++res;
                        }
                }

            return res;
        }

        std::size_t scheduler::get_num_numa_cores(std::size_t domain)
        {
            std::size_t numa_nodes = topo_->get_number_of_numa_nodes();
            if (numa_nodes == 0)
                numa_nodes = topo_->get_number_of_sockets();
            std::vector<hpx::threads::mask_type> node_masks(numa_nodes);

            std::size_t res = 0;
            std::size_t num_os_threads = hpx::get_os_thread_count();
            for (std::size_t num_thread = 0; num_thread != num_os_threads;
                 num_thread++)
                {
                    std::size_t pu_num = rp_->get_pu_num(num_thread);
                    std::size_t numa_node = topo_->get_numa_node_number(pu_num);
                    if(numa_node != domain) continue;

                    auto const& mask = topo_->get_thread_affinity_mask(pu_num);

                    std::size_t mask_size = hpx::threads::mask_size(mask);
                    for (std::size_t idx = 0; idx != mask_size; ++idx)
                        {
                            if (hpx::threads::test(mask, idx))
                                {
                                    ++res;
                                }
                        }
                }
            return res;
        }

        void scheduler::init()
        {
            std::size_t num_localities = allscale::get_num_localities();

            std::unique_lock<mutex_type> l(resize_mtx_);
            hpx::util::ignore_while_checking<std::unique_lock<mutex_type>> il(&l);
            if (initialized_) return;

            rp_ = &hpx::resource::get_partitioner();
            topo_ = &hpx::threads::get_topology();

            mconfig_.num_localities = num_localities;
            mconfig_.thread_depths.resize(get_num_numa_nodes());
            // Setting the default thread depths for each NUMA domain.
            for (std::size_t i = 0; i < mconfig_.thread_depths.size(); i++)
                {
                    std::size_t num_cores = get_num_numa_cores(i);
                    // We round up here...
                    if (num_cores == 1)
                        mconfig_.thread_depths[i] = 1;
                    else
                        mconfig_.thread_depths[i] = std::lround(std::pow(num_cores, 1.5));
                }

            std::string input_objective_str = hpx::get_config_entry("allscale.objective", "");
            if ( !input_objective_str.empty() )
                {
                    std::istringstream iss_leeways(input_objective_str);
                    std::string objective_str;
                    while (iss_leeways >> objective_str)
                        {
                            auto idx = objective_str.find(':');
                            std::string obj = objective_str;
#ifdef DEBUG_
                            std::cout << "Scheduling Objective provided: " << obj << "\n" ;
#endif
                            // Don't scale objectives if none is given
                            double leeway = 1.0;

                            if (idx != std::string::npos)
                                {
#ifdef DEBUG_
                                    std::cout << "Found a leeway, triggering multi-objectives policies\n" <<std::flush ;
#endif

                                    multi_objectives = true;
                                    obj = objective_str.substr(0, idx);
                                    leeway = std::stod( objective_str.substr(idx + 1) );
                                }

                            if (obj == "time")
                                {
                                    if (idx == std::string::npos)
                                        {
                                            time_requested = true;
                                        }
                                    time_leeway = leeway;
#ifdef DEBUG_
                                    std::cout << " Setting time policy\n" ;
#endif

                                }
                            else if (obj == "resource")
                                {
                                    if (idx == std::string::npos)
                                        {
                                            resource_requested = true;
                                        }
                                    resource_leeway = leeway;
#ifdef DEBUG_
                                    std::cout << " Setting resource policy\n" ;
#endif

                                }
                            else if (obj == "energy")
                                {
                                    if (idx == std::string::npos)
                                        {
#ifdef DEBUG_
                                            std::cout << " Setting energy policy\n" ;
#endif

                                            energy_requested = true;
                                        }
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
                                    HPX_THROW_EXCEPTION(hpx::bad_request, "scheduler::init", "leeways should be within ]0, 1]");
                                }

                            //                objectives_with_leeways.push_back(std::make_pair(obj, leeway));
                        }
                }

            std::string input_resource_step_str = hpx::get_config_entry("allscale.resource_step", "");
            if ( !input_resource_step_str.empty() )
                {

                    resource_step = std::stoul(input_resource_step_str);
#ifdef DEBUG_
                    std::cout << "Resource step provided : " << resource_step << "\n" ;
#endif

                    if ( resource_step ==0 || resource_step >= os_thread_count )
                        {
                            HPX_THROW_EXCEPTION(hpx::bad_request, "scheduler::init", "resource step should be within ]0, total nb threads[");
                        }
                }


            auto const& numa_domains = rp_->numa_domains();
            executors_.reserve(numa_domains.size());
            thread_pools_.reserve(numa_domains.size());

            for (std::size_t domain = 0; domain < numa_domains.size(); ++domain)
                {
                    std::string pool_name;
                    pool_name = "allscale-numa-" + std::to_string(domain);

                    thread_pools_.push_back( &hpx::resource::get_thread_pool(pool_name));
//                     std::cerr << "Attached to " << pool_name << " (" << thread_pools_.back() << '\n';
                    initial_masks_.push_back(thread_pools_.back()->get_used_processing_units());
                    suspending_masks_.push_back(thread_pools_.back()->get_used_processing_units());
                    hpx::threads::reset((suspending_masks_.back()));
                    resuming_masks_.push_back(thread_pools_.back()->get_used_processing_units());
                    hpx::threads::reset((resuming_masks_.back()));
                    executors_.emplace_back(pool_name);
                }
#if defined(ALLSCALE_HAVE_CPUFREQ)
            if (multi_objectives)
                {
                    //reallocating objectives_status vector of vectors
                    objectives_status.resize(3);
                    for (int i = 0 ; i < 3 ; i++)
                        {
                            objectives_status[i].resize(3);
                        }
                    std::cout << "multi-objectives policy set with time=" << time_leeway
                              << ", resource=" << resource_leeway
                              << ", energy=" << energy_leeway << "\n" << std::flush;
                }
#endif
            // if energy is to be taken into account, need to prep for it
            if ( energy_requested )
                {
#if defined(ALLSCALE_HAVE_CPUFREQ)
                    using hardware_reconf = allscale::components::util::hardware_reconf;
                    cpu_freqs = hardware_reconf::get_frequencies(0);
                    freq_step = 8; //cpu_freqs.size() / 2;
                    freq_times.resize(cpu_freqs.size());

#ifdef DEBUG_
                    std::cout << "Frequencies: " ;
                    for (auto& ind : cpu_freqs )
                        {
                            std::cout << ind << " ";
                        }
                    std::cout << "\n" << std::flush;
#endif

                    auto min_max_freqs = std::minmax_element(cpu_freqs.begin(), cpu_freqs.end());
                    min_freq = *min_max_freqs.first;
                    max_freq = *min_max_freqs.second;

#ifdef DEBUG_
                    std::cout << "Min freq:  " << min_freq << ", Max freq: " << max_freq << "\n" << std::flush;
#endif
                    //TODO: verify that nbpus == all pus of the system, not just the online ones
                    size_t nbpus = topo_->get_number_of_pus();
#ifdef DEBUG_
                    std::cout << "nbpus known to topo_:  " << nbpus << "\n" << std::flush;
#endif

                    hardware_reconf::make_cpus_online(0, nbpus);
                    hardware_reconf::topo_init();
                    // We have to set CPU governors to userpace in order to change frequencies later
                    std::string governor = "ondemand";

                    policy.governor = const_cast<char*>(governor.c_str());

                    topo = hardware_reconf::read_hw_topology();
                    // first reinitialize to a normal setup
                    for (int cpu_id = 0; cpu_id < topo.num_logical_cores; cpu_id ++)
                        {
                            int res = hardware_reconf::set_freq_policy(cpu_id, policy);
#ifdef DEBUG_
                            std::cout << "cpu_id "<< cpu_id << " back to on-demand. ret=  " << res << "\n" << std::flush;
#endif
                        }


                    governor = "userspace";
                    policy.governor = const_cast<char*>(governor.c_str());
                    policy.min = min_freq;
                    policy.max = max_freq;


                    for (int cpu_id = 0; cpu_id < topo.num_logical_cores; cpu_id += topo.num_hw_threads)
                        {
                            int res = hardware_reconf::set_freq_policy(cpu_id, policy);
                            if (res)
                                {
                                    HPX_THROW_EXCEPTION(hpx::bad_request, "scheduler::init",
                                                        "Requesting energy objective without being able to set cpu frequency");

                                    return;
                                }
#ifdef DEBUG_
                            std::cout << "cpu_id "<< cpu_id << " initial freq policy setting. ret=  " << res << "\n" << std::flush;
#endif
                        }

                    // Set frequency of all threads to max when we start

                    {
                        // set freq to all PUs used by allscale
                        for (std::size_t i = 0; i != thread_pools_.size(); ++i)
                            {
                                std::size_t thread_count = thread_pools_[i]->get_os_thread_count();
                                for (std::size_t j = 0  ; j < thread_count ; j++)
                                    {
                                        std::size_t pu_num = rp_->get_pu_num(j + thread_pools_[i]->get_thread_offset());

                                        if (!cpufreq_cpu_exists(pu_num))
                                            {
                                                int res = hardware_reconf::set_frequency(pu_num, 1 , cpu_freqs[0]);
#ifdef DEBUG_
                                                std::cout << "Setting cpu " << pu_num <<" to freq  "<< cpu_freqs[0] << ", (ret= " << res << ")\n" << std::flush;
#endif
                                            }

                                    }

                            }

                    }


                    std::this_thread::sleep_for(std::chrono::microseconds(2));

                    // Make sure frequency change happened before continuing
                    std::cout << "topo.num_logical_cores: " << topo.num_logical_cores << "topo.num_hw_threads" << topo.num_hw_threads<< "\n" << std::flush;
                    {
                        // check status of Pus frequency
                        for (std::size_t i = 0; i != thread_pools_.size(); ++i)
                            {
                                unsigned long hardware_freq = 0;
                                std::size_t thread_count = thread_pools_[i]->get_os_thread_count();
                                for (std::size_t j =  0 ;  j < thread_count ; j++)
                                    {
                                        std::size_t pu_num = rp_->get_pu_num(j + thread_pools_[i]->get_thread_offset());

                                        if (!cpufreq_cpu_exists(pu_num))
                                            {
                                                do
                                                    {
                                                        hardware_freq = hardware_reconf::get_hardware_freq(pu_num);
#ifdef DEBUG_
                                                        std::cout << "current freq on cpu "<< pu_num << " is " << hardware_freq << " (target freq is " << cpu_freqs[0] << " )\n" << std::flush;

#endif

                                                    } while (hardware_freq != cpu_freqs[0]);

                                            }

                                    }

                            }

                    }

                    // offline unused cpus
                    for (int cpu_id = 0; cpu_id < topo.num_logical_cores; cpu_id += topo.num_hw_threads)
                        {
                            bool found_it = false;
                            for ( std::size_t i = 0; i != thread_pools_.size(); i++)
                                {
                                    if (hpx::threads::test(initial_masks_[i], cpu_id))
                                        {
                                            std::cout << " cpu_id "<< cpu_id << " found\n" << std::flush;
                                            found_it = true;

                                        }
                                }

                            if (!found_it)
                                {
#ifdef DEBUG_
                                    std::cout << " setting cpu_id "<< cpu_id << " offline \n" << std::flush;
#endif

                                    hardware_reconf::make_cpus_offline(cpu_id, cpu_id + topo.num_hw_threads);
                                }
                        }



                    //frequency_timer_.start();
#else
                    // should we really abort or should we reset energy to 1 ?
                    HPX_THROW_EXCEPTION(hpx::bad_request, "scheduler::init",
                                        "Requesting energy objective without having compiled with cpufreq");
#endif

                }


            //             std::cerr  << "Scheduler with rank " << rank_ << " created!\n";

            initialized_ = true;
            //             std::cerr
            //                 << "Scheduler with rank " << rank_ << " created!\n";
        }

        void scheduler::enqueue(work_item work, this_work_item::id id)
        {
            this_work_item::id parent_id;
            bool sync = true;
            if (id)
                {
                    this_work_item::set_id(id);
                    parent_id = std::move(id);
                    sync = false;
                }
            else
                {
                    std::unique_lock<mutex_type> l(resize_mtx_);

                    work.set_this_id(mconfig_);
                    l.unlock();
                    parent_id = this_work_item::get_id();
                }


            std::uint64_t schedule_rank = work.id().rank();
            if(!allscale::resilience::rank_running(schedule_rank))
                {
                    schedule_rank = rank_;
                }

            HPX_ASSERT(work.valid());

            HPX_ASSERT(schedule_rank != std::uint64_t(-1));

            //#ifdef DEBUG_
                // std::cout << "Will schedule task " << work.id().name() << " on rank " << schedule_rank << std::endl;
            //#endif

            // schedule locally
            if (schedule_rank == rank_)
                {
                    if (work.is_first())
                        {
                            std::size_t current_id = work.id().last();
                            //#ifdef DEBUG_
                                // std::cout << "current_id: "<< current_id << " (could be " << work.id().name()  << " ), on rank : " << rank_ << "\n" << std::flush;
                            //#endif

                            allscale::monitor::signal(allscale::monitor::work_item_first, work);
                            //this is the place where we take policy specific actions
                            if (multi_objectives)
                                {
                                    multi_objectives_adjust(current_id);
                                }
                            else if ( time_requested || resource_requested)
                                {
                                    if ((current_id >= period_for_time ) && (current_id % period_for_time == 0))
                                        {
                                            periodic_throttle();
                                        }
                                }
                            else if (energy_requested)
                                {
                                    if ((current_id >= period_for_power ) && (current_id % period_for_power == 0))
                                        {
                                            power_periodic_frequency_scale();
                                        }
                                }
                        }

                    allscale::monitor::signal(allscale::monitor::work_item_enqueued, work);

                    // FIXME: modulus shouldn't be needed here
                    std::size_t numa_domain = work.id().numa_domain();
                    HPX_ASSERT(numa_domain < executors_.size());

                    if (do_split(work))
                        {
                            hpx::dataflow(hpx::launch::sync,
                                 [this_id = std::move(parent_id), work, this](hpx::future<std::size_t>&& f) mutable
                                 {
                                     std::size_t expected_rank = f.get();
                                     if(expected_rank == std::size_t(-1))
                                         {
                                            std::cerr << "split for " << work.name() << ": write requirements reside on different localities!\n";
                                            std::abort();
                                         }
                                     else if(expected_rank != std::size_t(-2))
                                         {
                                             HPX_ASSERT(expected_rank != rank_);
//                                              std::cout << "Dispatching " << work.name() << " to " << expected_rank << '\n';
                                             work.update_rank(expected_rank);
                                             network_.schedule(expected_rank, std::move(work), std::move(this_id));
                                         }
                                 },
                                 work.split(sync, rank_)
                            );
                        }
                    else
                        {
                            hpx::dataflow(hpx::launch::sync,
                                 [this_id = std::move(parent_id), work, numa_domain, this](hpx::future<std::size_t>&& f) mutable
                                 {
                                     // the process variant might fail if we try to acquire
                                     // data item read/write on multiple localities
                                     // we need to split then.
                                     std::size_t expected_rank = f.get();
                                     if(expected_rank == std::size_t(-1))
                                         {
                                             // We should move on and split...
                                             HPX_ASSERT(work.can_split());
                                             work.split(true, rank_);
                                         }
                                     else if(expected_rank != std::size_t(-2))
                                         {
                                             HPX_ASSERT(expected_rank != rank_);
                                             work.update_rank(expected_rank);
                                             network_.schedule(expected_rank, std::move(work), std::move(this_id));
                                         }
                                 },
                                 work.process(executors_[numa_domain], rank_)
                            );
                        }

                    return;
                }
            //task not meant to be local: move task to remote nodes
            work.mark_child_requirements(schedule_rank);

            allscale::resilience::global_wi_dispatched(work, schedule_rank);
            network_.schedule(schedule_rank, std::move(work), parent_id);
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


        unsigned int  scheduler::suspend_threads()
        {
            std::unique_lock<mutex_type> l(resize_mtx_);
#ifdef DEBUG_
            std::cout << "trying to suspend a thread\n" << std::flush;
#endif
            //find out which pool has the most threads
            hpx::threads::mask_type active_mask;
            std::size_t active_threads_ = 0;
            std::size_t domain_active_threads = 0;
            std::size_t pool_idx = 0;
            {
                // Select thread pool with the highest number of activated threads
                for (std::size_t i = 0; i != thread_pools_.size(); ++i)
                    {
                        //get the current used PUs as HPX knows
                        hpx::threads::mask_type curr_mask = thread_pools_[i]->get_used_processing_units();
#ifdef DEBUG_
                        std::cout << "Thread pool " << i << " has supposedly for active PUs: ";
                        for (std::size_t j = 0 ; j <  thread_pools_[i]->get_os_thread_count() ; j++)
                            {
                                std::size_t pu_num = rp_->get_pu_num(j + thread_pools_[i]->get_thread_offset());
                                if (hpx::threads::test(curr_mask, pu_num))
                                    std::cout << pu_num << " " ;
                            }
                        std::cout << "\n" << std::flush;
#endif

                        // remove from curr_mask any suspending thread still pending
                        for (std::size_t j = 0 ; j <  thread_pools_[i]->get_os_thread_count() ; j++)
                            {
                                std::size_t pu_num = rp_->get_pu_num(j + thread_pools_[i]->get_thread_offset());

                                // use this opportunity to update state of suspending threads
                                if  ( (hpx::threads::test(suspending_masks_[i], pu_num)) && !(hpx::threads::test(curr_mask, pu_num)) )
                                    {
#ifdef DEBUG_
                                        std::cout << " PU: " << pu_num << " suspension has finally happened\n" << std::flush;
#endif
                                        hpx::threads::unset(suspending_masks_[i], pu_num);
                                    }
                                // use also this opportunity to update resuming threads mask
                                if( (hpx::threads::test(resuming_masks_[i], pu_num)) && (hpx::threads::test(curr_mask, pu_num)) )
                                    {
#ifdef DEBUG_
                                        std::cout << " PU: " << pu_num << " resuming has finally happened\n" << std::flush;
#endif
                                        hpx::threads::unset(resuming_masks_[i], pu_num);
                                    }
                                // remove suspending threads from active list if necessary
                                if ((hpx::threads::test(curr_mask, pu_num)) && (hpx::threads::test(suspending_masks_[i], pu_num)) )
                                    {
#ifdef DEBUG_
                                        std::cout << " PU: " << pu_num << " suspension pending\n" << std::flush;
#endif
                                        hpx::threads::unset(curr_mask, pu_num);
                                    }

                            }
                        // don't mind the resuming threads here:
                        // if they appear in used_processing_unit, assume it has resumed

                        //count real active threads
                        std::size_t curr_active_pus = hpx::threads::count(curr_mask);
                        active_threads_ += curr_active_pus;
#ifdef DEBUG_
                        std::cout << "Thread pool " << i << " has actually " << curr_active_pus << " active PUs: ";
                        for (std::size_t j = 0 ; j <  thread_pools_[i]->get_os_thread_count() ; j++)
                            {
                                std::size_t pu_num = rp_->get_pu_num(j + thread_pools_[i]->get_thread_offset());
                                if (hpx::threads::test(curr_mask, pu_num))
                                    std::cout << pu_num << " " ;
                            }
                        std::cout << "\n" << std::flush;
#endif

                        // select this pool if new maximum found
                        if (curr_active_pus > domain_active_threads)
                            {
#ifdef DEBUG_
                                std::cout << "curr_active_pus: " << curr_active_pus << " domain_active_threads: "<< domain_active_threads << ", selecting pool " << i << " for next suspend\n";
#endif

                                domain_active_threads = curr_active_pus;
                                active_mask = curr_mask;
                                pool_idx = i;
                            }
                    }
            }
#ifdef DEBUG_
            std::cout << "total active PUs: " << active_threads_ << "\n";
#endif
            active_threads = active_threads_;
            growing = false;
            //check if we already suspended every possible thread
            if (domain_active_threads == min_threads)
                {
                    return 0;
                }

            // what threads are blocked
            auto blocked_os_threads = active_mask ^ initial_masks_[pool_idx];


            // fill a vector of PUs to suspend
            std::vector<std::size_t> suspend_threads;
            std::size_t thread_count = thread_pools_[pool_idx]->get_os_thread_count();
            suspend_threads.reserve(thread_count);
            suspend_threads.clear();
            for (std::size_t i = thread_count - 1; i >= min_threads ; i--)
                {
                    std::size_t pu_num = rp_->get_pu_num(i + thread_pools_[pool_idx]->get_thread_offset());
#ifdef DEBUG_
                    std::cout << "testing pu_num: " << pu_num << "\n";
#endif

                    if (hpx::threads::test(active_mask, pu_num))
                        {
                            suspend_threads.push_back(i);
                            hpx::threads::set(suspending_masks_[pool_idx], pu_num);
                            if (suspend_threads.size() == resource_step)
                                {
#ifdef DEBUG_
                                    std::cout << "reached the cap of nb thread to suspend (" << scheduler::resource_step << ")\n";
#endif
                                    break;
                                }
                        }
                }
            {
                hpx::util::unlock_guard<std::unique_lock<mutex_type> > ul(l);
                for(auto& pu: suspend_threads)
                    {
                        //hpx::util::unlock_guard<std::unique_lock<mutex_type> > ul(l);
#ifdef DEBUG_
                        std::cout << "suspend thread on pu: " << rp_->get_pu_num(pu + thread_pools_[pool_idx]->get_thread_offset()) << "\n" << std::flush;
#endif

                        thread_pools_[pool_idx]->suspend_processing_unit(pu).get();
                    }
            }
            // Setting the default thread depths of the NUMA domain.
            {
                std::size_t num_cores = get_num_numa_cores(pool_idx) - suspend_threads.size();
                // We round up here...
                mconfig_.thread_depths[pool_idx] = std::lround(std::pow(num_cores, 1.5));
            }
            active_threads = active_threads - suspend_threads.size();
#ifdef DEBUG_
            std::cout << "Sent disable signal. Active threads: " << active_threads  << std::endl;
#endif
            return suspend_threads.size();

        }

        unsigned int scheduler::resume_threads()
        {
            std::unique_lock<mutex_type> l(resize_mtx_);
#ifdef DEBUG_
            std::cout << "Trying to awake a thread\n" << std::flush;
#endif
            //hpx::threads::any(blocked_os_threads)

            hpx::threads::mask_type blocked_mask;
            std::size_t active_threads_ = 0;
            std::size_t domain_blocked_threads = 0; //std::numeric_limits<std::size_t>::max();
            std::size_t pool_idx = 0;

            growing = true;
            // Select thread pool with the largest number of blocked threads
            {
                for (std::size_t i = 0; i != thread_pools_.size(); ++i)
                    {
                        hpx::threads::mask_type curr_mask = thread_pools_[i]->get_used_processing_units();

#ifdef DEBUG_
                        std::cout << "Thread pool " << i << " has supposedly for active PUs: ";
                        for (std::size_t j = 0 ; j <  thread_pools_[i]->get_os_thread_count() ; j++)
                            {
                                std::size_t pu_num = rp_->get_pu_num(j + thread_pools_[i]->get_thread_offset());
                                if (hpx::threads::test(curr_mask, pu_num))
                                    std::cout << pu_num << " " ;
                            }
                        std::cout << "\n" << std::flush;
#endif


                        for (std::size_t j = 0 ; j <  thread_pools_[i]->get_os_thread_count() ; j++)
                            {
                                std::size_t pu_num = rp_->get_pu_num(j + thread_pools_[i]->get_thread_offset());
                                // use also this opportunity to update resuming threads mask
                                if( (hpx::threads::test(resuming_masks_[i], pu_num)) && (hpx::threads::test(curr_mask, pu_num)) )
                                    {
#ifdef DEBUG_
                                        std::cout << " PU: " << pu_num << " resuming has finally happened\n" << std::flush;
#endif
                                        hpx::threads::unset(resuming_masks_[i], pu_num);
                                    }
                                // use this opportunity to update state of suspending threads
                                if  ( (hpx::threads::test(suspending_masks_[i], pu_num)) && !(hpx::threads::test(curr_mask, pu_num)) )
                                    {
#ifdef DEBUG_
                                        std::cout << " PU: " << pu_num << " suspension has finally happened\n" << std::flush;
#endif
                                        hpx::threads::unset(suspending_masks_[i], pu_num);
                                    }


                                // if already resuming this thread, let's consider it active
                                if( (hpx::threads::test(resuming_masks_[i], pu_num)) && !(hpx::threads::test(curr_mask, pu_num)) )
                                    {
#ifdef DEBUG_
                                        std::cout << " PU: " << pu_num << " already resuming, consider active\n" << std::flush;
#endif
                                        hpx::threads::set(curr_mask, pu_num);
                                    }
                            }

                        std::size_t curr_active_pus = hpx::threads::count(curr_mask);
                        active_threads_ += curr_active_pus;
#ifdef DEBUG_
                        std::cout << "Thread pool " << i << " has actually " << curr_active_pus << " active PUs: ";
                        for (std::size_t j = 0 ; j <  thread_pools_[i]->get_os_thread_count() ; j++)
                            {
                                std::size_t pu_num = rp_->get_pu_num(j + thread_pools_[i]->get_thread_offset());
                                if (hpx::threads::test(curr_mask, pu_num))
                                    std::cout << pu_num << " " ;
                            }
                        std::cout << "\n" << std::flush;
#endif

                        auto blocked_os_threads = curr_mask ^ initial_masks_[i];
                        std::size_t nb_blocked_threads = hpx::threads::count(blocked_os_threads);
                        if (nb_blocked_threads > domain_blocked_threads)
                            {
#ifdef DEBUG_
                                std::cout << "nb_blocked_threads: " << nb_blocked_threads << " domain_blocked_threads: "<< domain_blocked_threads << ", selecting pool " << i << " for next resume\n";
#endif

                                domain_blocked_threads = nb_blocked_threads;
                                blocked_mask = blocked_os_threads;
                                pool_idx = i;
                            }
                    }
            }
#ifdef DEBUG_
            std::cout << "total active PUs: " << active_threads_ << "\n";
#endif
            active_threads = active_threads_;
            //if no thread is suspended, nothing to do
            if (domain_blocked_threads == 0)
                {
#ifdef DEBUG_
                    std::cout << "Every thread is awake\n" << std::flush;
#endif

                    return 0;
                }


            std::vector<std::size_t> resume_threads;
            std::size_t thread_count = thread_pools_[pool_idx]->get_os_thread_count();
            resume_threads.reserve(thread_count);
            resume_threads.clear();
            for (std::size_t i = 0; i < thread_count; ++i)
                {
                    std::size_t pu_num = rp_->get_pu_num(i + thread_pools_[pool_idx]->get_thread_offset());
#ifdef DEBUG_
                    std::cout << "testing pu " << pu_num << " for resume\n" << std::flush;
#endif

                    if (hpx::threads::test(blocked_mask, pu_num))
                        {
#ifdef DEBUG_
                            std::cout << "selecting pu " << pu_num << " for resume\n" << std::flush;
#endif
                            hpx::threads::set(resuming_masks_[pool_idx], pu_num);
                            resume_threads.push_back(i);
                            if (resume_threads.size() == resource_step)
                                break;
                        }
                }
            {
                hpx::util::unlock_guard<std::unique_lock<mutex_type> > ul(l);
                for(auto& pu: resume_threads)
                    {
                        std::cout << "And now : Resuming PU " << rp_->get_pu_num(pu + thread_pools_[pool_idx]->get_thread_offset()) << " !\n" << std::flush;
                        //hpx::util::unlock_guard<std::unique_lock<mutex_type> > ul(l);
                        thread_pools_[pool_idx]->resume_processing_unit(pu).get();
                    }
            }
            // Setting the default thread depths of the NUMA domain.
            {
                std::size_t num_cores = get_num_numa_cores(pool_idx) - resume_threads.size();
                // We round up here...
                mconfig_.thread_depths[pool_idx] = std::lround(std::pow(num_cores, 1.5));
            }
            active_threads = active_threads + resume_threads.size();
#ifdef DEBUG_
            std::cout << "Sent enable signal. Active threads: " << active_threads  << std::endl;
#endif
            return resume_threads.size();
        }

        //called periodically to possibly suspend or resume threads
        bool scheduler::periodic_throttle()
        {
            //will have some work to do only if we started with more than one thread, and if scheduling policy allows it

            if ( os_thread_count > 1 && ( time_requested || resource_requested ) )
                {
                    //early stage of the execution don't provide correct intelligence on sampling, so we skip them
                    std::unique_lock<mutex_type> l(resize_mtx_);
#ifdef DEBUG_
                    std::cout << "Entering periodic_throttle(), os_thread_count: "<< os_thread_count << ", time_requested: " << time_requested << ", resource_requested: " << resource_requested <<  "\n";
#endif

                    if ( current_avg_iter_time == 0.0 || allscale_monitor->get_number_of_iterations() < sampling_interval)
                        {
#ifdef DEBUG_
                            std::cout << "current_avg_iter_time: " << current_avg_iter_time << ", allscale_monitor->get_number_of_iterations(): " << allscale_monitor->get_number_of_iterations() << ", sampling_interval: " << sampling_interval << "\n";
#endif
                            if(allscale_monitor->get_number_of_iterations() == 0)
                                {
#ifdef DEBUG_
                                    std::cout << "number of iteration ==0, let's pass this round \n";
#endif
                                    return true;
                                }


                            {

                                current_avg_iter_time = allscale_monitor->get_avg_time_last_iterations(sampling_interval);
                                if (std::isnan(current_avg_iter_time))
                                    {
#ifdef DEBUG_
                                        std::cout << "monitoring returns NaN, setting current average time to 0\n ";
#endif
                                        current_avg_iter_time = 0.0;
                                    }

                                //                    current_avg_iter_time = allscale_monitor->get_last_iteration_time();
                                //                    current_avg_iter_time = allscale_monitor->get_avg_work_item_times(sampling_interval);
#ifdef DEBUG_
                                std::cout << "Now current_avg_iter_time= " << current_avg_iter_time << ", return true\n";
#endif

                            }
                            return true;
                        } //From now, we should have enough information
                    else if ( current_avg_iter_time > 0 )
                        {
                            //hpx::util::unlock_guard<std::unique_lock<mutex_type>> ul(l);
                            //current_avg_iter_time = allscale_monitor->get_last_iteration_time();

                            // get the average iteration time from the monitoring
                            last_avg_iter_time = current_avg_iter_time;
                            current_avg_iter_time = allscale_monitor->get_avg_time_last_iterations(sampling_interval);
#ifdef DEBUG_
                            std::cout << "last_avg_iter_time: " << last_avg_iter_time << ", current_avg_iter_time: " << current_avg_iter_time << "\n";
#endif


                            double time_threshold = current_avg_iter_time;

                            //check if we need to suspend or resume some threads

                            bool disable_flag = (growing && (last_avg_iter_time * disable_factor) < time_threshold)
                                || ((!growing) &&  last_avg_iter_time > (time_threshold * disable_factor));
                            bool enable_flag = ((!growing) && (last_avg_iter_time * enable_factor) < time_threshold)
                                || (growing &&  last_avg_iter_time > (time_threshold * enable_factor));

#ifdef DEBUG_
                            std::cout << "disable flag: " << disable_flag << ", enable flag: " << enable_flag << ", was growing: " << growing  << "\n";
#endif


                            if ( resource_requested )
                                {
#ifdef DEBUG_
                                    std::cout << "resource policy specifics \n";
#endif

                                    // If we have a sublinear speedup then prefer resources over time and throttle
                                    time_threshold = current_avg_iter_time * (active_threads - resource_step ) / active_threads;
                                    disable_flag = last_avg_iter_time > time_threshold;
                                    enable_flag = last_avg_iter_time < time_threshold;
                                    min_threads = 1;
                                }
                            // #ifdef DEBUG_
                            //                      std::cout << "domain_active_threads: " << domain_active_threads << ", min_threads: " << min_threads << "\n";
                            // #endif

                            if (disable_flag )
                                {
                                    hpx::util::unlock_guard<std::unique_lock<mutex_type>> ul(l);
                                    suspend_threads();
                                }
                            else if (enable_flag )
                                {
                                    hpx::util::unlock_guard<std::unique_lock<mutex_type>> ul(l);
                                    resume_threads();
                                }
                        }
                }

            return true;
        }


        bool scheduler::periodic_frequency_scale()
        {

#if defined(ALLSCALE_HAVE_CPUFREQ)
            if (energy_requested)
                {
                    std::unique_lock<mutex_type> l(resize_mtx_);
                    if ( !target_freq_found )
                        {
                            if ( current_energy_usage == 0 )
                                {
                                    current_energy_usage = hardware_reconf::read_system_energy();
                                    return true;
                                }
                            else if ( current_energy_usage > 0 )
                                {
                                    //read_system_energy() gives total number of joule spent since system powering

                                    last_energy_usage = current_energy_usage;
                                    current_energy_usage = hardware_reconf::read_system_energy();
                                    //need to compute energy usage as difference between now and last time
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
                                    if ( target_freq != min_freq )
                                        {
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
                }
            else
                {
                    if ( current_energy_usage == 0 )
                        {
                            current_energy_usage = hardware_reconf::read_system_energy();
                            return true;
                        }
                    else if ( current_energy_usage > 0 )
                        {
                            return true;
                        }
                }
#endif

            return true;
        }


        bool scheduler::power_periodic_frequency_scale()
        {

#if defined(ALLSCALE_HAVE_CPUFREQ)
            if (energy_requested)
                {
                    std::unique_lock<mutex_type> l(resize_mtx_);

                    //update current power usage
                    last_power_usage++;
                    current_power_usage = hardware_reconf::read_system_power();
                    power_sum += current_power_usage;
                    power_count++;
                    last_avg_iter_time = current_avg_iter_time;
                    {
                        //      hpx::util::unlock_guard<std::unique_lock<mutex_type>> ul(l);
                        //                    current_avg_iter_time = allscale_monitor->get_avg_time_last_iterations(sampling_interval);
                        current_avg_iter_time = allscale_monitor->get_last_iteration_time();
                    }

                    unsigned int freq_idx = -1;
                    unsigned long current_freq_hw = hardware_reconf::get_hardware_freq(0);
                    //unsigned long current_freq_kernel = hardware_reconf::get_kernel_freq(0);

                    // std::cout << "power: " << current_power_usage << ", time: " << current_avg_iter_time << ", current_freq_hw: " << current_freq_hw << "\n" << std::flush;
                    // Get freq index in cpu_freq
                    std::vector<unsigned long>::iterator it = std::find(cpu_freqs.begin(), cpu_freqs.end(), current_freq_hw);

                    if ( it != cpu_freqs.end() )
                        {
                            freq_idx = it - cpu_freqs.begin();
                            //  std::cout << "iterator " << freq_idx << "\n" << std::flush;
                        }
                    else
                        {
                            // If you run it without sudo, get_hardware_freq will fail and end up here as well!
                            HPX_THROW_EXCEPTION(hpx::bad_request, "scheduler::power_periodic_frequency_scale",
                                                boost::str(boost::format("Cannot find frequency: %s in the list of frequencies. Something must be wrong!") % current_freq_hw));
                        }

                    freq_times[freq_idx] = std::make_pair(current_power_usage, current_avg_iter_time);

                    //to avoid not updating power/freq values, lets rolls them out once in a while
                    if (last_power_usage >=20)
                        {
                            //   std::cout << "resetting power logs\n" << std::flush;
                            if ( freq_idx + freq_step < cpu_freqs.size() )
                                {
                                    freq_times[freq_idx + freq_step] = std::make_pair(0, 0);
                                }
                            if ( freq_idx  > freq_step )
                                {
                                    freq_times[freq_idx - freq_step] = std::make_pair(0, 0);
                                }
                            last_power_usage = 0;
                        }

                    //unsigned long target_freq = current_freq_hw;
                    // If we have not finished until the minimum frequnecy then continue
                    if ( freq_idx + freq_step < cpu_freqs.size() )
                        {
                            //      std::cout << "current power: " << freq_times[freq_idx].first << " power for lower freq: " << freq_times[freq_idx + freq_step].first << "\n" << std::flush;
                            if (freq_times[freq_idx].first > freq_times[freq_idx + freq_step].first)
                                {
                                    //      std::cout << " lowering frequency\n" << std::flush;
                                    hardware_reconf::set_next_frequency(freq_step, true);
                                    last_power_usage = 0;
                                    return true;
                                }

                            // hpx::util::unlock_guard<std::unique_lock<mutex_type> > ul(l);
                            // //Start decreasing frequencies by 2
                            // hardware_reconf::set_next_frequency(freq_step, true);
                            // target_freq = hardware_reconf::get_hardware_freq(0);
                            // std::cout << "Decrease frequency. " << "current_power_usage: "
                            //        << current_power_usage << ", current_freq_hw: "
                            //        << current_freq_hw << ", target_freq: "
                            //        << target_freq << ", current_avg_iter_time: "
                            //        << current_avg_iter_time << std::endl;
                        }
                    if (freq_idx   > freq_step )
                        {
                            // std::cout << "current power: " << freq_times[freq_idx].first << " power for higher freq: " << freq_times[freq_idx - freq_step].first << "\n" << std::flush;
                            if (freq_times[freq_idx].first > freq_times[freq_idx - freq_step].first)
                                {
                                    //      std::cout << " increasing frequency\n" << std::flush;
                                    hardware_reconf::set_next_frequency(freq_step, false);
                                    last_power_usage = 0;
                                    return true;
                                }
                        }
                }
            else
                {
                    current_power_usage = hardware_reconf::read_system_power();
                    power_sum += current_power_usage;
                    power_count++;
                    //              std::cout << "Current power: " << current_power_usage << "\n" << std::flush;
                }
#endif

            return true;
        }

        //multi-objectives policy called on task enqueued
        bool scheduler::multi_objectives_adjust(std::size_t current_id)
        {
#if defined(ALLSCALE_HAVE_CPUFREQ)
            //we enter this function for each enqueued task, check if we need to adjust objectives
            if ((current_id < period_for_time ) ||
                (current_id % period_for_time != 0))
                {
                    return true;
                }


            std::unique_lock<mutex_type> l(resize_mtx_);
#ifdef DEBUG_
            std::cout << "Entering multi_objectives_adjust(), num_threads_: "<< active_threads << ", time_requested\
: " << time_requested << ", resource_requested: " << resource_requested <<  "\n";
#endif

            // first section: all freq to max, resource to max, find the best time by throttling more and more
            if (!target_resource_found)
                {
                    if ( current_avg_iter_time == 0.0 || allscale_monitor->get_number_of_iterations() < sampling_interval)
                        {
#ifdef DEBUG_
                            std::cout << "current_avg_iter_time: " << current_avg_iter_time << ", allscale_monitor->get_number_of_iterations(): " << allscale_monitor->get_number_of_iterations() << ", sampling_interval: " << sampling_interval << "\n";
#endif
                            if (allscale_monitor->get_number_of_iterations() < sampling_interval)
                                {
#ifdef DEBUG_
                                    std::cout << "number of iteration < sampling_interval, let's pass this round \n";
#endif
                                    return true;
                                }


                            {
                                //    hpx::util::unlock_guard<std::unique_lock<mutex_type>> ul(l);
                                current_avg_iter_time = allscale_monitor->get_avg_time_last_iterations(sampling_interval);
                                if (std::isnan(current_avg_iter_time))
                                    {
#ifdef DEBUG_
                                        std::cout << "current_avg_iter_time get nan from allscale_monitor->get_avg_time_last_iterations()\n ";
#endif
                                        current_avg_iter_time = 0.0;
                                    }

                                //                    current_avg_iter_time = allscale_monitor->get_last_iteration_time();
                                //                    current_avg_iter_time = allscale_monitor->get_avg_work_item_times(sampling_interval);
#ifdef DEBUG_
                                std::cout << "Now current_avg_iter_time= " << current_avg_iter_time << "\n" << std::flush;
#endif

                                //measure max power now
#if defined(ALLSCALE_HAVE_CPUFREQ)
                                if (energy_requested)
                                    {
                                        max_power = hardware_reconf::read_system_power();
#ifdef DEBUG_
                                        std::cout << "measuring max power= " << max_power << "\n" << std::flush;
#endif
                                    }
#endif
                            }
                            return true;
                        } //From now, we should have enough information


                    // get the average iteration time from the monitoring
                    last_avg_iter_time = current_avg_iter_time;
                    current_avg_iter_time = allscale_monitor->get_avg_time_last_iterations(sampling_interval);
#ifdef DEBUG_
                    std::cout << "Looking for best time: last_avg_iter_time= " << last_avg_iter_time << ", current_avg_iter_time= " << current_avg_iter_time << "\n" << std::flush;
#endif
                    // if time is improving, continue to throttle down
                    if ((current_avg_iter_time < last_avg_iter_time) || (active_threads == os_thread_count))
                        {
#ifdef DEBUG_
                            std::cout << "Looking for best time: suspending thread \n" << std::flush;
#endif
                            hpx::util::unlock_guard<std::unique_lock<mutex_type> > ul(l);
                            suspend_threads();
                            return true;
                        }
                    else // the time is degrading, let's get back to previous state and claim we found optimal number of threads
                        {
                            unsigned int res;
                            {
                                hpx::util::unlock_guard<std::unique_lock<mutex_type> > ul(l);
                                res = resume_threads();
                            }
                            target_resource_found = true;
                            max_time = last_avg_iter_time;
                            max_resource = active_threads + res;
#ifdef DEBUG_
                            std::cout << "Looking for best time: found it ! setting max_resource to  " << max_resource << ", and max_time to " << max_time << "\n" << std::flush;
#endif
                            return true;
                        }


                }
            else //(target_resource_found) : now we know what number of resource gives best time
                {
                    unsigned long current_freq_hw = 0;
                    //check the current time from monitoring
                    current_avg_iter_time = allscale_monitor->get_avg_time_last_iterations(sampling_interval);

                    //new time < target time, we still have room in resource or energy budget
                    if (current_avg_iter_time <= (max_time / time_leeway))
                        {
#ifdef DEBUG_
                            std::cout << "Still have some time margin\n" << std::flush;
#endif

                            //resource are not yet at obj(r)
                            if(active_threads > (resource_leeway * os_thread_count))
                                {
                                    //decrease resources by suspending threads
#ifdef DEBUG_
                                    std::cout << "suspending threads\n" << std::flush;
#endif
                                    hpx::util::unlock_guard<std::unique_lock<mutex_type> > ul(l);
                                    suspend_threads();
                                    return true;

                                }
                            else if ((current_id >= period_for_power )    &&
                                     (current_id % period_for_power == 0) &&
                                     target_resource_found                &&
                                     energy_requested) //resource == obj(r), what about energy?
                                {
#if defined(ALLSCALE_HAVE_CPUFREQ)
                                    current_freq_hw = hardware_reconf::get_hardware_freq(0);
                                    current_power_usage = hardware_reconf::read_system_power();
                                    if(current_power_usage > (energy_leeway * max_power)) //e> obj(e)
                                        {
                                            if(current_freq_hw != min_freq) // freq != min_freq
                                                {
                                                    //lower frequency
                                                    std::vector<unsigned long>::iterator it = std::find(cpu_freqs.begin(), cpu_freqs.end(), current_freq_hw);
                                                    unsigned int freq_idx = it - cpu_freqs.begin();
                                                    if ( freq_idx + freq_step < cpu_freqs.size() )
                                                        {
#ifdef DEBUG_
                                                            std::cout << "Lowering frequency\n" << std::flush;
#endif

                                                            //      std::cout << "current power: " << freq_times[freq_idx].first << " power for lower freq: " << freq_times[freq_idx + freq_step].first << "\n" << std::flush;

                                                            hardware_reconf::set_next_frequency(freq_step, true);
                                                            return true;
                                                        }
                                                }
                                        }
                                    else if(current_freq_hw == max_freq) //below energy goal, are we already at max freq ? (in which case, this is due to resources)
                                        {
#ifdef DEBUG_
                                            std::cout << "Frequency already at maximum: resuming threads\n" << std::flush;
#endif

                                            //increase resource
                                            hpx::util::unlock_guard<std::unique_lock<mutex_type> > ul(l);
                                            resume_threads();
                                            return true;
                                        }
                                    else
                                        {
                                            //increase freq
                                            std::vector<unsigned long>::iterator it = std::find(cpu_freqs.begin(), cpu_freqs.end(), current_freq_hw);
                                            unsigned int freq_idx = it - cpu_freqs.begin();
                                            if (freq_idx   > freq_step )
                                                {
#ifdef DEBUG_
                                                    std::cout << "Increasing frequency\n" << std::flush;
#endif

                                                    // std::cout << "current power: " << freq_times[freq_idx].first << " power for higher freq: " << freq_times[freq_idx - freq_step].first << "\n" << std::flush;

                                                    //      std::cout << " increasing frequency\n" << std::flush;
                                                    hardware_reconf::set_next_frequency(freq_step, false);
                                                    return true;

                                                }
                                        }
#endif
                                }
                        }
                    else  //new time > target time, i.e. we are off target for time, need to increase speed
                        {
#ifdef DEBUG_
                            std::cout << "Not going fast enough\n" << std::flush;
#endif

                            if ((active_threads != os_thread_count ) && (active_threads > max_resource * resource_leeway)) // current resources != max && != target
                                {
#ifdef DEBUG_
                                    std::cout << "Resuming threads\n" << std::flush;
#endif
                                    hpx::util::unlock_guard<std::unique_lock<mutex_type> > ul(l);
                                    resume_threads();
                                    return true;
                                    //increase nb resources
                                }
#if defined(ALLSCALE_HAVE_CPUFREQ)
                            else if (current_freq_hw = hardware_reconf::get_hardware_freq(0) != max_freq) //current freq != max
                              {
#ifdef DEBUG_
                                    std::cout << "Increasing frequency\n" << std::flush;
#endif

                                    //increase freq
                                    std::vector<unsigned long>::iterator it = std::find(cpu_freqs.begin(), cpu_freqs.end(), current_freq_hw);
                                    unsigned int freq_idx = it - cpu_freqs.begin();
                                    if (freq_idx   > freq_step )
                                        {
                                            // std::cout << "current power: " << freq_times[freq_idx].first << " power for higher freq: " << freq_times[freq_idx - freq_step].first << "\n" << std::flush;

                                            //      std::cout << " increasing frequency\n" << std::flush;
                                            hardware_reconf::set_next_frequency(freq_step, false);
                                            return true;

                                        }
                                }
#endif
                            else  //current_resource == target; freq is already max, add resource if possible
                                {
#ifdef DEBUG_
                                    std::cout << "Resuming threads as a last resort\n" << std::flush;
#endif

                                    //increase nb_resource
                                    hpx::util::unlock_guard<std::unique_lock<mutex_type> > ul(l);
                                    resume_threads();
                                    return true;
                                }
                        }
                }
#endif
            return true;
        }

        //multi-objectives policy called on timer
        bool scheduler::multi_objectives_adjust_timed()
        {
            // adjust multi objectives on timer: pass a fake current_id that matches all modulos
            return multi_objectives_adjust(period_for_time*period_for_resource*period_for_power);
        }

        void scheduler::stop()
        {
            //         timer_.stop();

            //         if ( time_requested || resource_requested )
            //             throttle_timer_.stop();

            if ( energy_requested )
                frequency_timer_.stop();

            if(stopped_)
                return;

            //Resume all sleeping threads
            if ( ( time_requested || resource_requested ) && !energy_requested)
                {
                    std::size_t pool_idx = 0;
                    for (auto &pool: thread_pools_)
                        {
                            std::size_t thread_count = pool->get_os_thread_count();
                            for (std::size_t i = 0; i < thread_count; ++i)
                                {
                                    pool->resume_processing_unit(i);
                                }
                            ++pool_idx;
                        }
                }

            if ( energy_requested )
                {
#if defined(ALLSCALE_HAVE_CPUFREQ)

                    for (int cpu_id = 0; cpu_id < topo.num_logical_cores; cpu_id += topo.num_hw_threads)
                        {
                            bool found_it = false;
                            for ( std::size_t i = 0; i != thread_pools_.size(); i++)
                                {
                                    if (hpx::threads::test(initial_masks_[i], cpu_id))
                                        found_it = true;
                                }

                            if (!found_it)
                                {
#ifdef DEBUG_
                                    std::cout << " setting cpu_id "<< cpu_id << " back online \n" << std::flush;
#endif

                                    hardware_reconf::make_cpus_online(cpu_id, cpu_id + topo.num_hw_threads);
                                }
                        }

                    std::string governor = "ondemand";
                    policy.governor = const_cast<char*>(governor.c_str());
                    std::cout << "Set CPU governors back to " << governor << std::endl;
                    for (int cpu_id = 0; cpu_id < topo.num_logical_cores; cpu_id += topo.num_hw_threads)
                        int res = hardware_reconf::set_freq_policy(cpu_id, policy);
#endif
                }

            if (power_count)
                {
                    std::cerr << "Power average usage: " << (power_sum / power_count) << "\n" << std::flush;
                }
            stopped_ = true;
            //         work_queue_cv_.notify_all();
            //         std::cout << "rank(" << rank_ << "): scheduled " << count_ << "\n";

            network_.stop();
        }

    }
}
