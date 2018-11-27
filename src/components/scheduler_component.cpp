
#include <allscale/components/monitor.hpp>
#include <allscale/components/scheduler.hpp>
#include <allscale/monitor.hpp>
#include <allscale/resilience.hpp>
#include <allscale/components/localoptimizer.hpp>

#include <hpx/traits/executor_traits.hpp>
#include <hpx/util/unlock_guard.hpp>

#include <boost/format.hpp>

#include <algorithm>
#include <iterator>
#include <limits>
#include <math.h>
#include <sstream>
#include <stdlib.h>
#include <stdexcept>
#include <chrono>
#include <typeinfo>

//#define DEBUG_ 1
//#define DEBUG_INIT_ 1 // define to generate output during scheduler initialization
// #define DEBUG_MULTIOBJECTIVE_ 1
//#define DEBUG_THREADTHROTTLING_ 1
//#define DEBUG_THREADSTATUS_ 1
//#define DEBUG_FREQSCALING_ 1
//#define MEASURE_MANUAL 1 // define to generate output consumed by the regression test
// #define MEASURE_ 1
// only meant to be defined if one needs to measure the efficacy
// of the scheduler
#undef DEBUG_
#define ALLSCALE_USE_CORE_OFFLINING 1

namespace allscale {
namespace components {

scheduler::scheduler(std::uint64_t rank)
    : rank_(rank), initialized_(false), stopped_(false),
      os_thread_count(hpx::get_os_thread_count()),
      active_threads(os_thread_count),
      enable_factor(1.01),
      disable_factor(1.01),
      growing(true),
      min_threads(1),
      max_resource(0),
      max_time(0),
      max_power(0),
      current_power_usage(0),
      last_power_usage(0),
      power_sum(0),
      power_count(0),
      sampling_interval(3),
      current_avg_iter_time(0.0),
      multi_objectives(false),
      time_requested(false),
      resource_requested(false),
      energy_requested(false),
      time_weight(0.0),
      resource_weight(0.0),
      energy_weight(0.0),
      period_for_time(10),
      period_for_resource(10),
      period_for_power(20),
      nr_tasks_scheduled(0)

#if defined(MEASURE_MANUAL_)
      ,
      param_osthreads_(1),param_locked_frequency_idx_(0),fixed_frequency_(0)

#endif
#if defined(MEASURE_)
      ,
      meas_active_threads_sum(0), meas_active_threads_count(0),
      meas_active_threads_max(0), meas_active_threads_min(0),
      meas_power_min(0), meas_power_max(0),
      min_iter_time(0), max_iter_time(0)
#endif
      ,
      nr_opt_steps(0),
      last_objective_score(-1.0),
      uselopt(false)
  {
  allscale_monitor = &allscale::monitor::get();
  thread_times.resize(os_thread_count);

#ifdef DEBUG_
  std::cout << "DEBUG_ is defined" << std::endl << std::flush;
#endif
#ifdef DEBUG_INIT_
  std::cout << "DEBUG_INIT_ is defined" << std::endl << std::flush;
#endif
#ifdef DEBUG_KOSTAS
  std::cout << "DEBUG_KOSTAS is defined" << std::endl << std::flush;
#endif
#ifdef ALLSCALE_HAVE_CPUFREQ
  std::cout << "ALLSCALE_HAVE_CPUFREQ is defined" << std::endl << std::flush;
#else
  std::cout << "ALLSCALE_HAVE_CPUFREQ is not defined. No real power measurements or CPU frequency scaling" << std::endl << std::flush;
#endif
}

/**
 *
 * scheduler::get_num_numa_nodes()
 *
 * Returns number of NUMA nodes in the local domain (node/server) as an integer
 *
*/
std::size_t scheduler::get_num_numa_nodes() {
  std::size_t numa_nodes = topo_->get_number_of_numa_nodes();
  if (numa_nodes == 0)
    numa_nodes = topo_->get_number_of_sockets();
  std::vector<hpx::threads::mask_type> node_masks(numa_nodes);

  std::size_t num_os_threads = os_thread_count;
  for (std::size_t num_thread = 0; num_thread != num_os_threads; ++num_thread) {
    std::size_t pu_num = rp_->get_pu_num(num_thread);
    std::size_t numa_node = topo_->get_numa_node_number(pu_num);

    auto const &mask = topo_->get_thread_affinity_mask(pu_num);

    std::size_t mask_size = hpx::threads::mask_size(mask);
    for (std::size_t idx = 0; idx != mask_size; ++idx) {
      if (hpx::threads::test(mask, idx)) {
        hpx::threads::set(node_masks[numa_node], idx);
      }
    }
  }

  // Sort out the masks which don't have any bits set
  std::size_t res = 0;

  for (auto &mask : node_masks) {
    if (hpx::threads::any(mask)) {
      ++res;
    }
  }

  return res;
}

/**
 *
 * scheduler::get_num_numa_cores()
 *
 * Returns number of cores in the specified domain as an integer.
 * The desired domain is specified as an integer argument. It is the
 * responsibility of the caller to specify a domain ID within bounds.
 * If the domain is off bounds, an exception is thrown.
 *
*/
std::size_t scheduler::get_num_numa_cores(std::size_t domain) {
  std::size_t numa_nodes = topo_->get_number_of_numa_nodes();
  if (numa_nodes == 0)
    numa_nodes = topo_->get_number_of_sockets();
  std::vector<hpx::threads::mask_type> node_masks(numa_nodes);

  std::size_t res = 0;
  std::size_t num_os_threads = os_thread_count;
  for (std::size_t num_thread = 0; num_thread != num_os_threads; num_thread++) {
    std::size_t pu_num = rp_->get_pu_num(num_thread);
    std::size_t numa_node = topo_->get_numa_node_number(pu_num);
    if (numa_node != domain)
      continue;

    auto const &mask = topo_->get_thread_affinity_mask(pu_num);

    std::size_t mask_size = hpx::threads::mask_size(mask);
    for (std::size_t idx = 0; idx != mask_size; ++idx) {
      if (hpx::threads::test(mask, idx)) {
        ++res;
      }
    }
  }
  return res;
}

/*****
 *
 * scheduler::init()
 *
 * Initializes scheduler data structures and state, including local optimizer
 * per user parameteres specified.
 * Called only once after local scheduler first instantiation.
 *
*/
void scheduler::init() {
  std::size_t num_localities = allscale::get_num_localities();

  std::unique_lock<mutex_type> l(resize_mtx_);
  hpx::util::ignore_while_checking<std::unique_lock<mutex_type>> il(&l);

  if (initialized_)
    return;

#ifdef MEASURE_
  last_measure_power = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now()).time_since_epoch().count();
  last_measure_threads = last_measure_power;
#endif

  rp_ = &hpx::resource::get_partitioner();
  topo_ = &hpx::threads::get_topology();

  std::size_t num_cores = os_thread_count;

  depth_cut_off_ =
    std::ceil(
        std::log2(
            (num_cores + 2) * allscale::get_num_localities()
        )
    );

  // Reading user provided options in terms of desired optimization objectives
  std::string input_objective_str =
      hpx::get_config_entry("allscale.objective", "");

  /* Read optimization policy selected by the user. If not specified,
     allscale policy is the default */
    std::string input_optpolicy_str =
      hpx::get_config_entry("allscale.policy", "none");
      if ( input_optpolicy_str == "none" ){
        char *c_optpolicy = std::getenv("ALLSCALE_LOCAL_OPTIMIZER");
        if ( c_optpolicy) 
          input_optpolicy_str = std::string(c_optpolicy);
      }


    uselopt=false;
#ifdef DEBUG_MULTIOBJECTIVE_
    std::cout << "[Local Optimizer|INFO] Optimization Policy Active = " << input_optpolicy_str << std::endl;
#endif
  if (input_optpolicy_str=="allscale")
		lopt_.setPolicy(allscale);
  else 	if (input_optpolicy_str=="random")
      lopt_.setPolicy(random);
  else if (input_optpolicy_str=="manual")
      lopt_.setPolicy(manual);
 	else if ( input_optpolicy_str != "none" ) {
		HPX_THROW_EXCEPTION(hpx::bad_request, "scheduler::init", 
							"unknown allscale.policy");
	}


#ifdef MEASURE_MANUAL_
  std::string input_osthreads_str =
      hpx::get_config_entry("allscale.osthreads", "");

  std::string input_frequencyidx_str =
      hpx::get_config_entry("allscale.frequencyidx", "");

  int temp_idx=-1;
  bool manual_input_provided = false;
  try{
    temp_idx = std::stoi(input_frequencyidx_str);
    manual_input_provided=true;
  }catch(std::invalid_argument e1){
    HPX_THROW_EXCEPTION(hpx::bad_request, "scheduler::init",
                            "error in reading fixed frequency index");
  }catch(std::out_of_range e2){
    HPX_THROW_EXCEPTION(hpx::bad_request, "scheduler::init",
                            "error in reading fixed frequency index");
  }
#endif

  if (input_objective_str.empty() ){
    char *c_opt_objective = std::getenv("ALLSCALE_LOCAL_OBJECTIVE");
    if ( c_opt_objective )
      input_objective_str = std::string(c_opt_objective);
  }

  if (!input_objective_str.empty()) {
    uselopt=true;
    std::istringstream iss_leeways(input_objective_str);
    std::string objective_str;
    while (iss_leeways >> objective_str) {
      auto idx = objective_str.find(':');

      std::string obj = objective_str;
#ifdef DEBUG_INIT_
      std::cout << "Scheduling Objective provided: " << obj << "\n";
#endif
      // VV: Don't scale objectives if none is given
      double opt_weight = 1.0;

      if (idx != std::string::npos) {
#ifdef DEBUG_INIT_
        std::cout << "Found an optimization weight, triggering " 
                     "multi-objectives policies\n" << std::flush;
#endif

        multi_objectives = true;
        obj = objective_str.substr(0, idx);
        opt_weight = std::stod(objective_str.substr(idx + 1));
      }

      if (obj == "time") {
          time_requested = true;
          time_weight = opt_weight;
#ifdef DEBUG_INIT_
          std::cout << "Set time weight to " << time_weight << "\n" << std::flush;
#endif
      } else if (obj == "resource") {
        resource_requested = true;
        resource_weight = opt_weight;
#ifdef DEBUG_INIT_
        std::cout << "Set resource weight to " << resource_weight << "\n"
                  << std::flush;
#endif

      } else if (obj == "energy") {
        energy_requested = true;
        energy_weight = opt_weight;
#ifdef DEBUG_INIT_
        std::cout << "Set energy weight to " << energy_weight << "\n"
                  << std::flush;
#endif
      } else {
        std::cout << "TRIED PARSING \"" << obj << "\"" << std::endl;
        HPX_THROW_EXCEPTION(
            hpx::bad_request, "scheduler::init",
            boost::str(
                boost::format("Wrong objective: Valid values: [time, energy, resource]")));
      }

      if (time_weight > 2 || resource_weight > 2 || energy_weight > 2
          || time_weight < -2 || resource_weight < -2 || energy_weight < -2) {
        HPX_THROW_EXCEPTION(hpx::bad_request, "scheduler::init",
                            "Objective weights should be within [-2, 2]");
      }
    }
  }

  /* Initializing convenience vector masks */
  auto const &numa_domains = rp_->numa_domains();
  executors_.reserve(numa_domains.size());
  thread_pools_.reserve(numa_domains.size());
  numa_start = runtime::HierarchyAddress::getLayersOn(0, num_localities);
  numa_depth = std::floor(std::log2(numa_domains.size()));
//   std::cout << "got numa depth: " << numa_depth << '\n';

  for (std::size_t domain = 0; domain < numa_domains.size(); ++domain) {
    std::string pool_name;
    pool_name = "allscale-numa-" + std::to_string(domain);

    thread_pools_.push_back(&hpx::resource::get_thread_pool(pool_name));
    //                     std::cerr << "Attached to " << pool_name << " (" <<
    //                     thread_pools_.back() << '\n';
    initial_masks_.push_back(thread_pools_.back()->get_used_processing_units());
    suspending_masks_.push_back(
        thread_pools_.back()->get_used_processing_units());
    hpx::threads::reset((suspending_masks_.back()));
    resuming_masks_.push_back(
        thread_pools_.back()->get_used_processing_units());
    hpx::threads::reset((resuming_masks_.back()));
    executors_.emplace_back(pool_name);
  }

  if (multi_objectives) {

    #ifdef DEBUG_INIT_
    std::cout << "\n****************************************************\n" << std::flush;
    std::cout << "Policy selected: multi-objective set with time=" << time_weight
              << ", energy=" << energy_weight 
              << ", resource=" << resource_weight
              << "\n"
              << std::flush;
    std::cout << "Objectives Flags Set: \n" <<
              "\tTime: " << time_requested <<
              "\tResources: " << resource_requested <<
              "\tEnergy: " << energy_requested <<
              "\tMulti-objective: " << multi_objectives <<
              "\n" << std::flush;
    std::cout << "****************************************************\n" << std::flush;
    #endif
  }

  if (energy_requested)
    initialize_cpu_frequencies();

  #ifdef MEASURE_MANUAL_
  if (manual_input_provided && input_objective_str.empty())
      fix_allcores_frequencies(temp_idx);
  #endif

  initialized_ = true;
#ifdef DEBUG_INIT_
  std::cout << "[INFO] Scheduler on rank " << rank_
            << " initialization: DONE \n"
            << std::flush;
#endif

  /* initialization of local multi-objective optimizer object*/
  if (uselopt){
    auto t_now = std::chrono::system_clock::now();
    auto t_now_ms = std::chrono::time_point_cast<std::chrono::milliseconds>(t_now);
    auto t_value = t_now_ms.time_since_epoch();
    long t_duration_now = t_value.count();
    last_optimization_timestamp_ = t_duration_now;
    last_objective_measurement_timestamp_= t_duration_now;

    lopt_.setmaxthreads(os_thread_count);

    #if defined(ALLSCALE_HAVE_CPUFREQ)
    using hardware_reconf = allscale::components::util::hardware_reconf;
    auto  freqs = hardware_reconf::get_frequencies(0);

    auto freq_temp = lopt_.setfrequencies(freqs);
    if (freq_temp.empty()){
      HPX_THROW_EXCEPTION(hpx::bad_request, "scheduler::init",
      "error in initializing the local optimizer, allowed frequency values are empty");
    }
    // VV: Set to max number of threads and max frequency
    lopt_.reset(os_thread_count, freqs.size()-1);
    #else
    // VV: Max number of threads, and an arbitrary frequency index
    lopt_.reset(os_thread_count,0);
    auto freq_temp = lopt_.setfrequencies({0});
    #endif
    
    // VV: Set objectives after setting all constraints to
    //     trigger the initialization of nmd
    lopt_.setobjectives(time_weight, energy_weight, resource_weight);
    #ifdef DEBUG_
    lopt_.printobjectives();
    #endif
  }
}

/**
 *
 * scheduler::initialize_cpu_frequencies()
 *
 * Sets frequencies of all CPUs to maximum value, ensuring full performance
 * Used e.g. during initialization to make sure CPUs are set to full frequency
 * potential.
 *
*/
#if defined(ALLSCALE_HAVE_CPUFREQ)
void scheduler::initialize_cpu_frequencies() 
{
  using hardware_reconf = allscale::components::util::hardware_reconf;
  cpu_freqs = hardware_reconf::get_frequencies(0);
  
  #if defined(MEASURE_) && defined(DEBUG_INIT)
  unsigned long temp_transition_latency=hardware_reconf::get_cpu_transition_latency(1);
  if (temp_transition_latency==0)
    std::cout << "[INFO] Transition Latency Unavailable" <<
      "\n" << std::flush;
  else
    std::cout << "[INFO] Core-1 Frequency Transition Latency = " <<
      hardware_reconf::get_cpu_transition_latency(2)/1000 <<
      " milliseconds\n" << std::flush;

  #endif

  #ifdef DEBUG_INIT_
  std::cout << "[INFO] Governors available on the system: " <<
      "\n" << std::flush;
  std::vector<std::string> temp_governors = hardware_reconf::get_governors(0);
  for (std::vector<std::string>::const_iterator i = temp_governors.begin(); i != temp_governors.end(); ++i)
    std::cout << "[INFO]\t" << *i << "\n" << std::flush;
  std::cout << "\n" << std::flush;

  std::cout << "Server Processor Available Frequencies (size = " << cpu_freqs.size() << ")";
  for (auto &ind : cpu_freqs) {
    std::cout << ind << " ";
  }
  std::cout << "\n" << std::flush;
  #endif

  auto min_max_freqs = std::minmax_element(cpu_freqs.begin(), cpu_freqs.end());
  min_freq = *min_max_freqs.first;
  max_freq = *min_max_freqs.second;
  // TODO: verify that nbpus == all pus of the system, not just the online
  // ones
  size_t nbpus = topo_->get_number_of_pus();

  #ifdef DEBUG_INIT_
  std::cout << "Min freq:  " << min_freq << ", Max freq: " << max_freq << "\n"
            << std::flush;
  std::cout << "nbpus known to topo_:  " << nbpus << "\n" << std::flush;
  #endif

  hardware_reconf::make_cpus_online(0, nbpus);
  hardware_reconf::topo_init();
  // We have to set CPU governors to userpace in order to change frequencies
  // later
  std::string governor = "ondemand";

  policy.governor = const_cast<char *>(governor.c_str());

  topo = hardware_reconf::read_hw_topology();
  // first reinitialize to a normal setup
  for (unsigned int cpu_id = 0; cpu_id < topo.num_logical_cores; cpu_id++){
    hardware_reconf::set_freq_policy(cpu_id, policy);
    #ifdef DEBUG_INIT_
    std::cout << "cpu_id " << cpu_id << " back to on-demand. ret=  " 
              << res << std::endl;
    #endif
  }

  governor = "userspace";
  policy.governor = const_cast<char *>(governor.c_str());
  policy.min = min_freq;
  policy.max = max_freq;

  for (unsigned int cpu_id = 0; 
       cpu_id < topo.num_logical_cores;
       cpu_id += topo.num_hw_threads) 
  {
    int res = hardware_reconf::set_freq_policy(cpu_id, policy);
    if (res) {
      HPX_THROW_EXCEPTION(hpx::bad_request, "scheduler::init",
                          "Requesting energy objective without being able to "
                          "set cpu frequency");

      return;
    }
  #ifdef DEBUG_INIT_
    std::cout << "cpu_id " << cpu_id
              << " initial freq policy setting. ret=  " << res << "\n"
              << std::flush;
  #endif
  }
  // Set frequency of all threads to max when we start

  // set freq to all PUs used by allscale
  for (std::size_t i = 0; i != thread_pools_.size(); ++i) {
    std::size_t thread_count = thread_pools_[i]->get_os_thread_count();
    for (std::size_t j = 0; j < thread_count; j++) {
      std::size_t pu_num =
          rp_->get_pu_num(j + thread_pools_[i]->get_thread_offset());


      if (!cpufreq_cpu_exists(pu_num)) {
        hardware_reconf::set_frequency(pu_num, 1, cpu_freqs[0]);
        #ifdef DEBUG_INIT_
        std::cout << "Setting cpu " << pu_num << " to freq  " << cpu_freqs[0]
                  << ", (ret= " << res << ")\n"
                  << std::flush;
        #endif
      }
    }
  }

  std::this_thread::sleep_for(std::chrono::microseconds(2));

  // Make sure frequency change happened before continuing
  std::cout << "topo.num_logical_cores: " << topo.num_logical_cores
            << " topo.num_hw_threads" << topo.num_hw_threads << "\n"
            << std::flush;
      // check status of Pus frequency
    
  for (std::size_t i = 0; i != thread_pools_.size(); ++i) {
    unsigned long hardware_freq = 0;
    std::size_t thread_count = thread_pools_[i]->get_os_thread_count();
    for (std::size_t j = 0; j < thread_count; j++) {
      std::size_t pu_num =
          rp_->get_pu_num(j + thread_pools_[i]->get_thread_offset());

      if (!cpufreq_cpu_exists(pu_num)) {
        do {
          hardware_freq = hardware_reconf::get_hardware_freq(pu_num);
        #ifdef DEBUG_INIT_
          std::cout << "current freq on cpu " << pu_num << " is "
                    << hardware_freq << " (target freq is " << cpu_freqs[0]
                    << " )\n"
                    << std::flush;
        #endif

        } while (hardware_freq != cpu_freqs[0]);
      }
    }
  }

  #ifdef ALLSCALE_USE_CORE_OFFLINING
  // offline unused cpus
  for (unsigned int cpu_id = 0; cpu_id < topo.num_logical_cores;
       cpu_id += topo.num_hw_threads) {
    bool found_it = false;
    for (std::size_t i = 0; i != thread_pools_.size(); i++) {
      if (hpx::threads::test(initial_masks_[i], cpu_id)) {
        //std::cout << " cpu_id " << cpu_id << " found\n" << std::flush;
        found_it = true;
      }
    }

    if (!found_it) {
      #ifdef DEBUG_INIT_
      std::cout << " setting cpu_id " << cpu_id << " offline \n" << std::flush;
      #endif

      hardware_reconf::make_cpus_offline(cpu_id, cpu_id + topo.num_hw_threads);
    }
  }
  #endif
}
#else
void scheduler::initialize_cpu_frequencies() 
{
    cpu_freqs.clear();
    // VV: Bogus frequency
    cpu_freqs.push_back(1000*1024);
}
#endif

/**
 *
 * scheduler::enqueue
 *
 * Function entry point for scheduling work items for execution.
 *
*/
void scheduler::optimize_locally(work_item const& work)
{
    if (work.id().depth() == 0) {
        allscale::monitor::signal(allscale::monitor::work_item_first, work);
#ifdef DEBUG_
        // find out which pool has the most threads

        /* Count Active threads for validation*/
        hpx::threads::mask_type active_mask;
        std::size_t domain_active_threads = 0;
        std::size_t pool_idx = 0;
        int total_threads_counted=0;
        // Select thread pool with the highest number of activated threads
        for (std::size_t i = 0; i != thread_pools_.size(); ++i) {
            // get the current used PUs as HPX knows
            hpx::threads::mask_type curr_mask =
                thread_pools_[i]->get_used_processing_units();
            for (std::size_t j = 0; j < thread_pools_[i]->get_os_thread_count();j++) {
                std::size_t pu_num =
                    rp_->get_pu_num(j + thread_pools_[i]->get_thread_offset());
                if (hpx::threads::test(curr_mask, pu_num))
                    total_threads_counted++;
            }
        }
        std::cout << "Active OS Threads = " <<  total_threads_counted << std::endl;

#endif

        if (uselopt && !lopt_.isConverged()) {
            last_power_usage++;
            allscale::components::monitor *monitor_c = &allscale::monitor::get();
            current_power_usage = monitor_c->get_current_power();
            power_sum += current_power_usage;

            auto t_now = std::chrono::system_clock::now();
            auto t_now_ms = std::chrono::time_point_cast<std::chrono::milliseconds>(t_now);
            auto t_value = t_now_ms.time_since_epoch();
            long t_duration_now = t_value.count();

            long elapsedTimeMs = t_duration_now - last_objective_measurement_timestamp_;

            auto dt_power = t_duration_now - last_measure_power;
            last_measure_power = t_duration_now;
            update_power_consumption(power_sum/last_power_usage, dt_power);

            if (elapsedTimeMs > objective_measurement_period_ms){
                last_objective_measurement_timestamp_= t_duration_now;

                // iteration time
                current_avg_iter_time =
                    allscale_monitor->get_avg_time_last_iterations(sampling_interval);
                if (std::isnan(current_avg_iter_time)) {
#ifdef DEBUG_
                    std::cout
                        << "monitoring returns NaN, setting current average time to 0\n ";
#endif
                    current_avg_iter_time = 0.0;
                }
                double last_objectives[] = {current_avg_iter_time,power_sum/(last_power_usage*monitor_c->get_max_power()),
                        active_threads};
                lopt_.measureObjective(current_avg_iter_time,power_sum/(last_power_usage*monitor_c->get_max_power()),
                        active_threads);
                last_power_usage=0;
                power_sum=0;

                last_objective_score = lopt_.evaluate_score(last_objectives);

                auto power_dt = t_duration_now - last_measure_power;
                update_power_consumption(power_sum/last_power_usage, power_dt);
                last_measure_power = t_duration_now;
            }

            elapsedTimeMs = t_duration_now - last_optimization_timestamp_;

            if (elapsedTimeMs > optimization_period_ms || nr_opt_steps == 0){
                last_optimization_timestamp_= t_duration_now;
                nr_opt_steps++;
                actuation act_temp = lopt_.step(active_threads);
#ifdef DEBUG_MULTIOBJECTIVE_
                lopt_.printverbosesteps(act_temp);
#endif
                auto dt_threads = t_duration_now - last_measure_threads;
                update_active_osthreads(active_threads, dt_threads);
                last_measure_threads = t_duration_now;
                if (act_temp.threads < active_threads){
                    suspend_threads(active_threads-act_temp.threads);
                }
                else if (act_temp.threads > active_threads){
                    resume_threads(act_temp.threads - active_threads);
                }
                fix_allcores_frequencies(act_temp.frequency_idx);
                lopt_.setCurrentFrequencyIdx(act_temp.frequency_idx);
                lopt_.setCurrentThreads(active_threads);

#ifdef DEBUG_MULTIOBJECTIVE_
                    std::cout << "[SCHEDULER|INFO]: Active Threads = " << active_threads << " out of " << lopt_.getmaxthreads() 
                    << " , target threads = " << act_temp.threads << std::endl;
#endif
            }
        } 
    #ifdef MEASURE_
        else {
          auto timestamp_now = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now()).time_since_epoch().count();
          auto dt = timestamp_now - last_measure_power;
          if ( dt >= 1000 ) {
            allscale::components::monitor *monitor_c = &allscale::monitor::get();
            auto cur_power = monitor_c->get_current_power();

            update_power_consumption(cur_power, dt);
            last_measure_power = timestamp_now;
          }
        }
    #endif
  }
}


void scheduler::update_max_threads(std::size_t max_threads)
{
  std::cout << "Will try to set max threads to " << max_threads <<std::endl;
  if (uselopt)
    lopt_.setmaxthreads(max_threads);
  else if (active_threads > max_threads )
    suspend_threads(active_threads - max_threads);
  else if ( active_threads < max_threads )
    resume_threads(max_threads - active_threads);
}

void scheduler::set_local_optimizer_weights(double time_weight, 
                                         double energy_weight,
                                         double resource_weight)
{
    lopt_.setobjectives(time_weight, energy_weight, resource_weight);
}

void scheduler::get_local_optimizer_weights(double *time_weight,
                                           double *energy_weight,
                                           double *resource_weight)
{
    lopt_.getobjectives(time_weight, energy_weight, resource_weight);
}

std::pair<work_item, std::unique_ptr<data_item_manager::task_requirements_base>> scheduler::schedule_local(work_item work,
        std::unique_ptr<data_item_manager::task_requirements_base>&& reqs,
        runtime::HierarchyAddress const& addr)
{
    optimize_locally(work);

    nr_tasks_scheduled++;

    // FIXME!!!!
    std::size_t numa_node = work.id().numa_node(numa_depth, numa_start);

    if (do_split(work, numa_node))
    {
        if (!work.can_split()) {
            std::cerr << "split for " << work.name() << "(" << work.id()
                << ") requested but can't split further\n";
            std::abort();
        }
        reqs->add_allowance(addr);

        bool async = work.id().depth() < 2;//addr.getParent().getNumaNode() != numa_node;

        hpx::future<void> acquired = reqs->acquire_split(addr);

        typename hpx::traits::detail::shared_state_ptr_for<
            hpx::future<void>>::type const &state =
            hpx::traits::future_access<hpx::future<void>>::get_shared_state(
                    acquired);
        auto f_split = [async, state, this, numa_node, work = std::move(work), reqs = std::move(reqs)]() mutable
            {
                auto &exec = executors_[numa_node];
                if (async)
                {
                    hpx::parallel::execution::post(
                        exec, hpx::util::annotated_function(
                            hpx::util::deferred_call([work = std::move(work), reqs = std::move(reqs), &exec]() mutable
                            {
                                work.split(exec, std::move(reqs));

                            }),
                            "allscale::work_item::split"));
                }
                else
                {
                    work.split(exec, std::move(reqs));
                }
            };

        state->set_on_completed(std::move(f_split));
        return std::make_pair(work_item(), std::unique_ptr<data_item_manager::task_requirements_base>());
    }
    else
    {
        if (!addr.isLeaf()) return std::make_pair(std::move(work), std::move(reqs));

        auto begin = std::chrono::high_resolution_clock::now();

        reqs->add_allowance(addr);


//         {
//             static hpx::lcos::local::spinlock mtx;
//             mtx.lock();
//             std::cout << work.name() << " " << work.id() << " " << addr << " "
//                 << runtime::HierarchyAddress::getLayersOn(0, num)
//                 << " " << numa_node << " " << depth_cut_off_ << '\n';
//             mtx.unlock();
//         }


        hpx::future<void> acquired = reqs->acquire_process(addr);
        typename hpx::traits::detail::shared_state_ptr_for<
            hpx::future<void>>::type const &state =
            hpx::traits::future_access<hpx::future<void>>::get_shared_state(
                    acquired);

        auto f =
            [state, work = std::move(work), reqs = std::move(reqs), begin](executor_type& exec) mutable
            {
                work.process(exec, begin, std::move(reqs));
            };

        state->set_on_completed(
        [f = std::move(f), this, numa_node]() mutable
        {
            auto &exec = executors_[numa_node];
            hpx::parallel::execution::post(
                exec, hpx::util::annotated_function(
                    hpx::util::deferred_call(std::move(f), std::ref(exec)),
                    "allscale::work_item::process"));
        });

        return std::make_pair(work_item(), std::unique_ptr<data_item_manager::task_requirements_base>());
    }
}

/**
 *
 * scheduler::do_split
 *
*/
bool scheduler::do_split(work_item const &w, std::size_t numa_node) {
  // Check if the work item is splittable first
  if (w.can_split()) {
    // Check if we reached the required depth
    // FIXME: make the cut off runtime configurable...
    // FIXME:!!!!!!!
    if (w.id().depth() < depth_cut_off_)
    {
//         std::cout << "
      // FIXME: add more elaborate splitting criterions
      return true;
    }

    return false;
  }
  // return false if it isn't
  return false;
}


/**
 *
 * scheduler::suspend_threads(std::size_t)
 *
 * Suspends specified number (as an integer) of runtime/OS threads assigned to
 * the runtime. Note that this may not necessarily translate to hardware
 * threads, i.e. the behaviour in terms of the latter is architecture specific.
 *
*/

unsigned int scheduler::suspend_threads(std::size_t suspendthreads) {
  std::unique_lock<mutex_type> l(resize_mtx_);
#ifdef DEBUG_THREADTHROTTLING_
  std::cout << "trying to suspend a thread\n" << std::flush;
#endif
  // find out which pool has the most threads
  hpx::threads::mask_type active_mask;
  std::size_t active_threads_ = 0;
  std::size_t domain_active_threads = 0;
  std::size_t pool_idx = 0;
  {
    // Select thread pool with the highest number of activated threads
    for (std::size_t i = 0; i != thread_pools_.size(); ++i) {
      // get the current used PUs as HPX knows
      hpx::threads::mask_type curr_mask =
          thread_pools_[i]->get_used_processing_units();
#ifdef DEBUG_THREADTHROTTLING_
      std::cout << "Thread pool " << i << " has supposedly for active PUs: ";
      for (std::size_t j = 0; j < thread_pools_[i]->get_os_thread_count();
           j++) {
        std::size_t pu_num =
            rp_->get_pu_num(j + thread_pools_[i]->get_thread_offset());
        if (hpx::threads::test(curr_mask, pu_num))
          std::cout << pu_num << " ";
      }
      std::cout << "\n" << std::flush;
#endif

      // remove from curr_mask any suspending thread still pending
      for (std::size_t j = 0; j < thread_pools_[i]->get_os_thread_count();
           j++) {
        std::size_t pu_num =
            rp_->get_pu_num(j + thread_pools_[i]->get_thread_offset());

        // use this opportunity to update state of suspending threads
        if ((hpx::threads::test(suspending_masks_[i], pu_num)) &&
            !(hpx::threads::test(curr_mask, pu_num))) {
#ifdef DEBUG_THREADTHROTTLING_
          std::cout << " PU: " << pu_num << " suspension has finally happened\n"
                    << std::flush;
#endif
          hpx::threads::unset(suspending_masks_[i], pu_num);
        }
        // use also this opportunity to update resuming threads mask
        if ((hpx::threads::test(resuming_masks_[i], pu_num)) &&
            (hpx::threads::test(curr_mask, pu_num))) {
#ifdef DEBUG_THREADTHROTTLING_
          std::cout << " PU: " << pu_num << " resuming has finally happened\n"
                    << std::flush;
#endif
          hpx::threads::unset(resuming_masks_[i], pu_num);
        }
        // remove suspending threads from active list if necessary
        if ((hpx::threads::test(curr_mask, pu_num)) &&
            (hpx::threads::test(suspending_masks_[i], pu_num))) {
#ifdef DEBUG_THREADTHROTTLING_
          std::cout << " PU: " << pu_num << " suspension pending\n"
                    << std::flush;
#endif
          hpx::threads::unset(curr_mask, pu_num);
        }
      }

      // count real active threads
      std::size_t curr_active_pus = hpx::threads::count(curr_mask);
      active_threads_ += curr_active_pus;
#ifdef DEBUG_THREADTHROTTLING_
      std::cout << "Thread pool " << i << " has actually " << curr_active_pus
                << " active PUs: ";
      for (std::size_t j = 0; j < thread_pools_[i]->get_os_thread_count();
           j++) {
        std::size_t pu_num =
            rp_->get_pu_num(j + thread_pools_[i]->get_thread_offset());
        if (hpx::threads::test(curr_mask, pu_num))
          std::cout << pu_num << " ";
      }
      std::cout << "\n\n" << std::flush;
#endif

      // select this pool if new maximum found
      if (curr_active_pus > domain_active_threads) {
#ifdef DEBUG_THREADTHROTTLING_
        std::cout << "curr_active_pus: " << curr_active_pus
                  << " domain_active_threads: " << domain_active_threads
                  << ", selecting pool " << i << " for next suspend\n";
#endif

        domain_active_threads = curr_active_pus;
        active_mask = curr_mask;
        pool_idx = i;
      }
    }
  }
#ifdef DEBUG_THREADTHROTTLING_
  std::cout << "total active PUs: " << active_threads_ << "\n";
#endif

  active_threads = active_threads_;

  growing = false;
  // check if we already suspended every possible thread
  if (domain_active_threads == min_threads) {
    return 0;
  }

  // what threads are blocked
//   auto blocked_os_threads = active_mask ^ initial_masks_[pool_idx];

  // fill a vector of PUs to suspend
  std::vector<std::size_t> suspend_threads;
  std::size_t thread_count = thread_pools_[pool_idx]->get_os_thread_count();
  suspend_threads.reserve(thread_count);
  suspend_threads.clear();
  for (std::size_t i = thread_count - 1; i >= min_threads; i--) {
    std::size_t pu_num =
        rp_->get_pu_num(i + thread_pools_[pool_idx]->get_thread_offset());
#ifdef DEBUG_THREADTHROTTLING_
    std::cout << "testing pu_num: " << pu_num << "\n";
#endif

    if (hpx::threads::test(active_mask, pu_num)) {
      suspend_threads.push_back(i);
      hpx::threads::set(suspending_masks_[pool_idx], pu_num);
      if (suspend_threads.size() == suspendthreads) {
#ifdef DEBUG_THREADTHROTTLING_
        std::cout << "reached the cap of #threads to suspend ("
                  << suspendthreads << ")\n";
#endif
        break;
      }
    }
  }
  {
    hpx::util::unlock_guard<std::unique_lock<mutex_type>> ul(l);
    for (auto &pu : suspend_threads) {
      // hpx::util::unlock_guard<std::unique_lock<mutex_type> > ul(l);
#ifdef DEBUG_THREADTHROTTLING_
      std::cout << "suspending thread on pu: "
                << rp_->get_pu_num(pu +
                                   thread_pools_[pool_idx]->get_thread_offset())
                << "\n"
                << std::flush;
#endif

      thread_pools_[pool_idx]->suspend_processing_unit(pu).get();
    }
  }

  // Setting the default thread depths of the NUMA domain
  {
      std::size_t num_cores = hpx::get_os_thread_count() - suspend_threads.size();
      depth_cut_off_ =
        std::ceil(
            std::log2(
                (num_cores + 2) * allscale::get_num_localities()
            )
        );
  }

  active_threads = active_threads - suspend_threads.size();

#ifdef DEBUG_THREADSTATUS_
  std::cout << "[SCHEDULER|INFO] Thread Suspension - Newly Active Threads: " << active_threads
            << std::endl;
#endif
  return suspend_threads.size();
}

/**
 *
 * scheduler::resume_threads(std::size_t)
 *
 * Resumes specified number (as an integer) of runtime/OS threads assigned to
 * the runtime. Note that this may not necessarily translate to hardware
 * threads, i.e. the behaviour in terms of the latter is architecture specific.
 *
*/
unsigned int scheduler::resume_threads(std::size_t resumethreads) {
  std::unique_lock<mutex_type> l(resize_mtx_);
#ifdef DEBUG_THREADTHROTTLING_
  std::cout << "Trying to resume a thread\n" << std::flush;
#endif

  hpx::threads::mask_type blocked_mask;
  std::size_t active_threads_ = 0;
  std::size_t domain_blocked_threads = 0; // std::numeric_limits<std::size_t>::max();
  std::size_t pool_idx = 0;

  growing = true;
  // Select thread pool with the largest number of blocked threads
  {
    for (std::size_t i = 0; i != thread_pools_.size(); ++i) {
      hpx::threads::mask_type curr_mask =
          thread_pools_[i]->get_used_processing_units();

#ifdef DEBUG_THREADTHROTTLING_
      std::cout << "Thread pool " << i << " has supposedly for active PUs: ";
      for (std::size_t j = 0; j < thread_pools_[i]->get_os_thread_count();
           j++) {
        std::size_t pu_num =
            rp_->get_pu_num(j + thread_pools_[i]->get_thread_offset());
        if (hpx::threads::test(curr_mask, pu_num))
          std::cout << pu_num << " ";
      }
      std::cout << "\n" << std::flush;
#endif

      for (std::size_t j = 0; j < thread_pools_[i]->get_os_thread_count();
           j++) {
        std::size_t pu_num =
            rp_->get_pu_num(j + thread_pools_[i]->get_thread_offset());
        // use also this opportunity to update resuming threads mask
        if ((hpx::threads::test(resuming_masks_[i], pu_num)) &&
            (hpx::threads::test(curr_mask, pu_num))) {
#ifdef DEBUG_THREADTHROTTLING_
          std::cout << " PU: " << pu_num << " resuming has finally happened\n"
                    << std::flush;
#endif
          hpx::threads::unset(resuming_masks_[i], pu_num);
        }
        // use this opportunity to update state of suspending threads
        if ((hpx::threads::test(suspending_masks_[i], pu_num)) &&
            !(hpx::threads::test(curr_mask, pu_num))) {
#ifdef DEBUG_THREADTHROTTLING_
          std::cout << " PU: " << pu_num << " suspension has finally happened\n"
                    << std::flush;
#endif
          hpx::threads::unset(suspending_masks_[i], pu_num);
        }

        // if already resuming this thread, let's consider it active
        if ((hpx::threads::test(resuming_masks_[i], pu_num)) &&
            !(hpx::threads::test(curr_mask, pu_num))) {
#ifdef DEBUG_THREADTHROTTLING_
          std::cout << " PU: " << pu_num
                    << " already resuming, consider active\n"
                    << std::flush;
#endif
          hpx::threads::set(curr_mask, pu_num);
        }
      }

      std::size_t curr_active_pus = hpx::threads::count(curr_mask);
      active_threads_ += curr_active_pus;
#ifdef DEBUG_THREADTHROTTLING_
      std::cout << "Thread pool " << i << " has actually " << curr_active_pus
                << " active PUs: ";
      for (std::size_t j = 0; j < thread_pools_[i]->get_os_thread_count();
           j++) {
        std::size_t pu_num =
            rp_->get_pu_num(j + thread_pools_[i]->get_thread_offset());
        if (hpx::threads::test(curr_mask, pu_num))
          std::cout << pu_num << " ";
      }
      std::cout << "\n" << std::flush;
#endif

      auto blocked_os_threads = curr_mask ^ initial_masks_[i];
      std::size_t nb_blocked_threads = hpx::threads::count(blocked_os_threads);
      if (nb_blocked_threads > domain_blocked_threads) {
#ifdef DEBUG_THREADTHROTTLING_
        std::cout << "nb_blocked_threads: " << nb_blocked_threads
                  << " domain_blocked_threads: " << domain_blocked_threads
                  << ", selecting pool " << i << " for next resume\n";
#endif

        domain_blocked_threads = nb_blocked_threads;
        blocked_mask = blocked_os_threads;
        pool_idx = i;
      }
    }
  }
#ifdef DEBUG_THREADTHROTTLING_
  std::cout << "total active PUs: " << active_threads_ << "\n";
#endif

  active_threads = active_threads_;
  // if no thread is suspended, nothing to do
  if (domain_blocked_threads == 0) {
#ifdef DEBUG_THREADTHROTTLING_
    std::cout << "Every thread is awake\n" << std::flush;
#endif

    return 0;
  }

  std::vector<std::size_t> resume_threads;
  std::size_t thread_count = thread_pools_[pool_idx]->get_os_thread_count();
  resume_threads.reserve(thread_count);
  resume_threads.clear();
  for (std::size_t i = 0; i < thread_count; ++i) {
    std::size_t pu_num =
        rp_->get_pu_num(i + thread_pools_[pool_idx]->get_thread_offset());
#ifdef DEBUG_THREADTHROTTLING_
    std::cout << "testing pu " << pu_num << " for resume\n" << std::flush;
#endif

    if (hpx::threads::test(blocked_mask, pu_num)) {
#ifdef DEBUG_THREADTHROTTLING_
      std::cout << "selecting pu " << pu_num << " for resume\n" << std::flush;
#endif
      hpx::threads::set(resuming_masks_[pool_idx], pu_num);
      resume_threads.push_back(i);
      if (resume_threads.size() == resumethreads)
        break;
    }
  }
  {
    hpx::util::unlock_guard<std::unique_lock<mutex_type>> ul(l);
    for (auto &pu : resume_threads) {
#ifdef DEBUG_THREADTHROTTLING_
      std::cout << "And now : Resuming PU "
                << rp_->get_pu_num(pu +
                                   thread_pools_[pool_idx]->get_thread_offset())
                << " !\n"
                << std::flush;
#endif
      // hpx::util::unlock_guard<std::unique_lock<mutex_type> > ul(l);
      thread_pools_[pool_idx]->resume_processing_unit(pu).get();
    }
  }
  // Setting the default thread depths of the NUMA domain
  {
      std::size_t num_cores = hpx::get_os_thread_count() + resume_threads.size();
      depth_cut_off_ =
        std::ceil(
            std::log2(
                (num_cores + 2) * allscale::get_num_localities()
            )
        );
  }
  active_threads = active_threads + resume_threads.size();
#ifdef DEBUG_THREADSTATUS_
  std::cout << "[SCHEDULER|INFO]: Thread Resume - Newly Active Threads: " << active_threads
            << std::endl;
#endif
  return resume_threads.size();
}

#ifdef ALLSCALE_HAVE_CPUFREQ
void scheduler::fix_allcores_frequencies(int frequency_idx){

  using hardware_reconf = allscale::components::util::hardware_reconf;
  cpu_freqs = hardware_reconf::get_frequencies(0);

  auto min_max_freqs = std::minmax_element(cpu_freqs.begin(), cpu_freqs.end());
  min_freq = *min_max_freqs.first;
  max_freq = *min_max_freqs.second;

  // fix index to be within system limits, if need be
  if (frequency_idx<0)
    frequency_idx=0;
  const int max_freq_idx = cpu_freqs.size()-1;
  if (frequency_idx > max_freq_idx)
    frequency_idx = max_freq_idx;

  // TODO: verify that nbpus == all pus of the system, not just the online
  // ones

  size_t nbpus = topo_->get_number_of_pus();
  #ifdef DEBUG_FREQSCALING_
  std::cout << "nbpus known to topo_:  " << nbpus << "\n" << std::flush;
  #endif

  hardware_reconf::make_cpus_online(0, nbpus);
  hardware_reconf::topo_init();

  // We have to set CPU governors to userpace in order to change frequencies
  // later
  std::string governor = "userspace";
  policy.governor = const_cast<char *>(governor.c_str());
  policy.min = min_freq;
  policy.max = max_freq;

  for (unsigned int cpu_id = 0; cpu_id < topo.num_logical_cores;
       cpu_id += topo.num_hw_threads) {
    int res = hardware_reconf::set_freq_policy(cpu_id, policy);
    if (res) {
      HPX_THROW_EXCEPTION(hpx::bad_request, "scheduler::init",
                          "Requesting energy objective without being able to "
                          "set cpu frequency");
      return;
    }
  #ifdef DEBUG_FREQSCALING_
    std::cout << "cpu_id " << cpu_id
              << " initial freq policy setting. ret=  " << res << "\n"
              << std::flush;
  #endif
  }

  // set freq of all cores used to min
  for (std::size_t i = 0; i != thread_pools_.size(); ++i) {
    std::size_t thread_count = thread_pools_[i]->get_os_thread_count();
    for (std::size_t j = 0; j < thread_count; j++) {
      std::size_t pu_num =
          rp_->get_pu_num(j + thread_pools_[i]->get_thread_offset());

      if (!cpufreq_cpu_exists(pu_num)) {
        //int res = hardware_reconf::set_frequency(pu_num, 1, cpu_freqs[cpu_freqs[.size()-1]]);
        int res = hardware_reconf::set_frequency(pu_num, 1, cpu_freqs[frequency_idx]);
        (void)res;
        #if defined(MEASURE_MANUAL_)
        fixed_frequency_ = cpu_freqs[frequency_idx];
        #endif
        #ifdef DEBUG_FREQSCALING_
        //std::cout << "Setting cpu " << pu_num << " to freq  " << cpu_freqs[cpu_freqs.size()-1]
        std::cout << "Setting cpu " << pu_num << " to freq  " << cpu_freqs[frequency_idx]
                  << ", (ret= " << res << ")\n"
                  << std::flush;
        #endif
      }
    }
  }



  // check status of Pus frequency
  for (std::size_t i = 0; i != thread_pools_.size(); ++i) {
    unsigned long hardware_freq = 0;
    std::size_t thread_count = thread_pools_[i]->get_os_thread_count();
    for (std::size_t j = 0; j < thread_count; j++) {
      std::size_t pu_num =
          rp_->get_pu_num(j + thread_pools_[i]->get_thread_offset());

      if (!cpufreq_cpu_exists(pu_num)) {
        do {
          hardware_freq = hardware_reconf::get_hardware_freq(pu_num);
          #ifdef DEBUG_FREQSCALING_
          std::cout << "current freq on cpu " << pu_num << " is "
                    //<< hardware_freq << " (target freq is " << cpu_freqs[cpu_freqs.size()-1]
                    << hardware_freq << " (target freq is " << cpu_freqs[frequency_idx]
                    << " )\n"

                    << std::flush;
            #endif
        //} while (hardware_freq != cpu_freqs[cpu_freqs.size()-1]);
        } while (hardware_freq != cpu_freqs[frequency_idx]);
      }
    }
  }
  
}
#else
void scheduler::fix_allcores_frequencies(int frequency_idx)
{
    // VV: This is a stub
}
#endif

#ifdef MEASURE_
void scheduler::update_active_osthreads(std::size_t threads, int64_t delta_time) {

  if (meas_active_threads_max==0 || meas_active_threads_max < threads)
    meas_active_threads_max=threads;

  if (meas_active_threads_min==0 || meas_active_threads_min > threads)
    meas_active_threads_min=threads;

  meas_active_threads_count += delta_time;
  meas_active_threads_sum += threads * delta_time;

  std::cout <<"REGISTERING THREADS " << threads << " for " << delta_time << 
  " current average " << (meas_active_threads_sum/meas_active_threads_count) << std::endl;
}

void scheduler::update_power_consumption(std::size_t power_sample, int64_t delta_time)
{
  if ( power_sample > 10000)
    return;
  
  if (meas_power_max==0 || meas_power_max < power_sample)
    meas_power_max=power_sample;

  if (meas_power_min==0 || meas_power_min > power_sample)
    meas_power_min=power_sample;


  meas_power_count += delta_time;
  meas_power_sum += power_sample * delta_time;

  std::cout << "Reporting Threads:" << active_threads << " Power:" << power_sample << " for Dt:" << delta_time << std::endl;
}
#endif

void scheduler::stop() {
  //         timer_.stop();

  //         if ( time_requested || resource_requested )
  //             throttle_timer_.stop();

  //if (energy_requested)
  //  frequency_timer_.stop();

  if (stopped_)
    return;

  // Resume all sleeping threads
  if ((time_requested || resource_requested) && !energy_requested) {
    std::size_t pool_idx = 0;
    for (auto &pool : thread_pools_) {
      std::size_t thread_count = pool->get_os_thread_count();
      for (std::size_t i = 0; i < thread_count; ++i) {
        pool->resume_processing_unit(i);
      }
      ++pool_idx;
    }
  }
  stopped_ = true;

  /* Output all measured metrics */
#ifdef DEBUG_MULTIOBJECTIVE_
#ifdef MEASURE_
  auto timestamp_now = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now()).time_since_epoch().count();
  auto dt_threads = timestamp_now - last_measure_threads;
  auto dt_power = timestamp_now - last_measure_power;

  last_measure_power = timestamp_now;
  last_measure_threads = timestamp_now;

  update_active_osthreads(active_threads, dt_threads);
    allscale::components::monitor *monitor_c = &allscale::monitor::get();

  auto measurement = monitor_c->get_current_power();
  if ( measurement <= 10000 ) {
    update_power_consumption(measurement, dt_power);
  }
  
  if ( meas_active_threads_count == 0 )
    meas_active_threads_count = 1;
  if ( meas_power_count == 0 )
    meas_power_count = 1;
  
  std::cout << "\n****************************************************\n" << std::flush;
  std::cout << "Measured Metrics of Application Execution:\n"
            << "\tTotal number of tasks scheduled locally (#taskslocal) = "
            << nr_tasks_scheduled << std::endl

            << "\tAverage number of OS threads allocated to application (#osthreadsavg) = "
            << meas_active_threads_sum/meas_active_threads_count << std::endl

            << "\tMinimum number of OS threads allocated to application (#osthreadsmin) = "
            << meas_active_threads_min << std::endl

            << "\tMaximum number of OS threads allocated to application (#osthreadsmax) = "
            << meas_active_threads_max << std::endl

            << "\tAverage power consumption (in Watts) by application execution (#avgpower) = "
            << meas_power_sum/meas_power_count << std::endl

            << "\tMaximum power consumption (in Watts) by application execution (#maxpower) = "
            << meas_power_max << std::endl

            << "\tMinimum power consumption (in Watts) by application execution (#minpower) = "
            << meas_power_min << std::endl
#ifdef MEASURE_MANUAL_
            << "\tFixed Processor Frequency (in GHz) during application execution (#freq) = "
            << fixed_frequency_ << std::endl
#endif

            << "\n" << std::flush;
  std::cout << "****************************************************\n" << std::flush;

#endif
#endif

}

} // components
} // allscale
