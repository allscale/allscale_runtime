
#ifndef ALLSCALE_COMPONENTS_SCHEDULER_HPP
#define ALLSCALE_COMPONENTS_SCHEDULER_HPP

#include <allscale/work_item.hpp>
#include <allscale/components/treeture_buffer.hpp>
#include <allscale/components/scheduler_network.hpp>
#include <allscale/components/localoptimizer.hpp>
#if defined(ALLSCALE_HAVE_CPUFREQ)
#include <allscale/util/hardware_reconf.hpp>
#endif

#include <hpx/include/components.hpp>
// #include <hpx/include/local_lcos.hpp>
#include <hpx/util/interval_timer.hpp>

#include <hpx/compute/host.hpp>
#include <hpx/compute/host/target.hpp>
#include <hpx/runtime/threads/executors/pool_executor.hpp>
//#include <hpx/runtime/threads/policies/throttling_scheduler.hpp>
#include <hpx/runtime/threads/threadmanager.hpp>

#include <atomic>
#include <memory>
#include <deque>
#include <vector>
#include <unordered_map>
#include <chrono>

//#define MEASURE_MANUAL_ 1
#define MEASURE_ 1


namespace allscale { namespace components {

    struct scheduler
    {
        typedef hpx::lcos::local::spinlock mutex_type;
#if defined(ALLSCALE_HAVE_CPUFREQ)
        using hardware_reconf = allscale::components::util::hardware_reconf;
#endif

        scheduler()
        {
            HPX_ASSERT(false);
        }

        scheduler(std::uint64_t rank);
        void init();

//         void enqueue(work_item work);
//         void enqueue_local(work_item work, bool force_split, bool sync);

        void schedule_local(work_item work, std::size_t numa_node,
            std::size_t local_depth);


        void stop();

        std::uint64_t rank_;

    private:
        std::size_t get_num_numa_nodes();
        std::size_t get_num_numa_cores(std::size_t domain);
        void initialize_cpu_frequencies();

        hpx::resource::detail::partitioner *rp_;
        const hpx::threads::topology *topo_;
        bool initialized_;
        std::atomic<bool> stopped_;

        scheduler_network network_;

        mutex_type spawn_throttle_mtx_;
        std::unordered_map<const char*, treeture_buffer> spawn_throttle_;

        bool do_split(work_item const& work, std::size_t local_depth, std::size_t numa_node);

        bool collect_counters();
        //try to suspend resource_step threads, return number of threads which received a new suspend order;
        // REM unsigned int suspend_threads();
        unsigned int suspend_threads(std::size_t);
        //try to resume resource_step threads, return number of threads which received a new resume order;
        // REM         unsigned int resume_threads();
        unsigned int resume_threads(std::size_t);

#ifdef MEASURE_
        // convenience methods to update measured metrics of interest
        void update_active_osthreads(std::size_t);
        void update_power_consumption(std::size_t);
#endif

        void fix_allcores_frequencies(int index);

        // REM hpx::util::interval_timer throttle_timer_;
        // REMhpx::util::interval_timer frequency_timer_;
        //REM hpx::util::interval_timer multi_objectives_timer_;

        /* captures absolute timestamp of the last time the optimizer
           has been invoked */
        //std::chrono::time_point<std::chrono::high_resolution_clock>
        //    last_optimization_timestamp_;
        long last_optimization_timestamp_;

        /* periodicity in milliseconds to invoke the optimizer */
        const long optimization_period_ms = 5;

        /* captures absolute timestamp of the last time optimization
           objective value have been measured (sampled) */
        //std::chrono::time_point<std::chrono::high_resolution_clock>
        //    last_objective_measurement_timestamp_;
        long last_objective_measurement_timestamp_;

        /* periodicity in milliseconds to invoke objective sampling */
        const long objective_measurement_period_ms = 1;

        //extra masks to better handle suspending/resuming threads
        std::vector<hpx::threads::thread_pool_base*> thread_pools_;
        std::vector<hpx::threads::mask_type> initial_masks_;
        std::vector<hpx::threads::mask_type> suspending_masks_;
        std::vector<hpx::threads::mask_type> resuming_masks_;
        std::vector<executor_type> executors_;
        std::atomic<std::size_t> current_;

        // This is the depth where we don't want to split anymore...
        std::vector<std::size_t> depth_cut_off_;

        // resources tracking
        std::size_t os_thread_count; // initial (max) resources
        std::size_t active_threads;  // current resources

        double enable_factor;
        double disable_factor;
        bool   growing;
        unsigned int min_threads;
        // Indices show number of threads, which hold pair of
        // execution times and number of times that particular thread used
        // due to suspend and resume
        std::vector<std::pair<double, unsigned int>> thread_times;

        unsigned long min_freq;
        unsigned long max_freq;
        unsigned long max_resource;
        unsigned long max_time;
        unsigned long max_power;
        unsigned long long current_power_usage;
        unsigned long long last_power_usage;
        unsigned long long power_sum;
        unsigned long long power_count;
#if defined(ALLSCALE_HAVE_CPUFREQ)
        cpufreq_policy policy;
        hardware_reconf::hw_topology topo;
        std::vector<unsigned long> cpu_freqs;
        // Indices correspond to the freq id in cpu_freqs, and
        // each pair holds energy usage and execution time
        std::vector<std::pair<unsigned long long, double>> freq_times;
        std::vector<std::vector<std::pair<unsigned long long, double>>> objectives_status;
        unsigned int freq_step;
        bool target_freq_found;
#endif
        unsigned int resource_step;
        bool target_resource_found;

        mutable mutex_type throttle_mtx_;
        mutable mutex_type resize_mtx_;

        std::uint16_t sampling_interval;
        double last_avg_iter_time;
        double current_avg_iter_time;
        monitor *allscale_monitor;

        const std::vector<std::string> objectives = {
            "time",
            "resource",
            "energy",
        };

        bool multi_objectives;
        bool time_requested;
        bool resource_requested;
        bool energy_requested;

        double time_leeway;
        double resource_leeway;
        double energy_leeway;
        unsigned int period_for_time;
        unsigned int period_for_resource;
        unsigned int period_for_power;

        /* cumulative number of locally scheduled application tasks (work items) */
        unsigned long long nr_tasks_scheduled;

#ifdef MEASURE_MANUAL_
        int param_osthreads_;
        int param_locked_frequency_idx_;
        unsigned long fixed_frequency_; // fixed frequency the runtime is ran at
#endif

#ifdef MEASURE_
        /* Resources measured metrics (OS threads) */
        unsigned long long meas_active_threads_sum;  // cumulative sum of active OS threads measured
        unsigned long long meas_active_threads_count; // number of times active OS threads have been sampled
        unsigned long long meas_active_threads_max; // maximum number of active OS threads throughout execution
        unsigned long long meas_active_threads_min; // minimum number of active OS threads throughout execution

        /* Power/Energy measured metrics */
        unsigned long long meas_power_sum; // cumulative sum of power consumption measured (in Watts)
        unsigned long long meas_power_count; // number of times power consumption has been sampled
        int meas_power_min; // minimum system power measurement throughout execution in Watts
        int meas_power_max; // maximum system power measurement throughout execution in Watts

        /* Execution time measured metrics */
        double min_iter_time; // minimum iteration time measured throughout execution
        double max_iter_time; // maximum iteration time measured throughout execution
#endif
        /* local node optimizer object */
        localoptimizer lopt_;

        long int nr_opt_steps;

        /* flag to enable local optimizer, if objectives have been specified,
           set to false otherwise.
         */
         bool uselopt;

    };
}}

#endif
