
#include <allscale/components/scheduler.hpp>
#include <allscale/components/monitor.hpp>
#include <allscale/monitor.hpp>
// #include <allscale/util/hardware_reconf.hpp>

#include <hpx/util/scoped_unlock.hpp>
#include <iterator>
#include <algorithm>
#include <stdlib.h>

namespace allscale { namespace components {

    scheduler::scheduler(std::uint64_t rank)
      : num_localities_(hpx::get_num_localities().get())
      , num_threads_(hpx::get_num_worker_threads())
      , rank_(rank)
      , schedule_rank_(0)
      , stopped_(false)
      , os_thread_count(hpx::get_os_thread_count())
      , active_threads(os_thread_count)
      , depth_cap(1.5 * (std::log(os_thread_count)/std::log(2) + 0.5))
      , current_avg_iter_time(0.0)
      , sampling_interval(10)
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
    }

    void scheduler::init()
    {
        // Find neighbors...
        std::uint64_t left_id =
            rank_ == 0 ? num_localities_ - 1 : rank_ - 1;
        std::uint64_t right_id =
            rank_ == num_localities_ - 1 ? 0 : rank_ + 1;


        hpx::future<hpx::id_type> right_future =
            hpx::find_from_basename("allscale/scheduler", right_id);
        if(left_id != right_id)
        {
            hpx::future<hpx::id_type> left_future =
                hpx::find_from_basename("allscale/scheduler", left_id);
            left_ = left_future.get();
        }
        if(num_localities_ > 1)
            right_ = right_future.get();

        input_objective = hpx::get_config_entry("allscale.objective", scheduler::objectives.at(objective_IDs::TIME)  );

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

        if (input_objective == scheduler::objectives.at(objective_IDs::TIME_RESOURCE) ) {
//             allscale_app_counter_id = hpx::performance_counters::get_counter(allscale_app_counter_name);
//             hpx::performance_counters::stubs::performance_counter::start(hpx::launch::sync, allscale_app_counter_id);

        thread_scheduler = dynamic_cast<hpx::threads::policies::throttling_scheduler<>*>(hpx::resource::get_thread_pool(0).get_scheduler());
            if (thread_scheduler != nullptr) {
               std::cout << "We have a thread manager holding the throttling_scheduler" << std::endl;
            } else {
               HPX_THROW_EXCEPTION(hpx::bad_request, "scheduler::init",
			"thread_scheduler is null. Make sure you select throttling scheduler via --hpx:queuing=throttling");
            }

// 	    throttle_timer_.start();
        }

        std::cerr
            << "Scheduler with rank "
            << rank_ << " created (" << left_ << " " << right_ << ")!\n";
    }

    void scheduler::enqueue(work_item work, this_work_item::id const& id)
    {
        std::uint64_t schedule_rank = 0;
        //std::cout<<"remote is " << remote << std::endl;
        if (work.enqueue_remote())
        {
//            std::cout<<"schedulign work item on loc " << hpx::get_locality_id()<<std::endl;

            schedule_rank = schedule_rank_.fetch_add(1) % 3;
        }

        if (id)
            this_work_item::set_id(id);

        hpx::id_type schedule_id;
        //work.requires();
        switch (schedule_rank)
        {
            case 1:
                if(right_)
                {
                    schedule_id = right_;
                    break;
                }
            case 2:
                if(left_)
                {
                    schedule_id = left_;
                    break;
                }
            case 0:
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
//                     std::size_t current_numa_domain = ++current_ % executors.size();
                    if (do_split(work))
                    {
                        work.split();
                    }
                    else
                    {
                        work.process();
//                         std::size_t current_numa_domain = ++current_ % executors.size();
//                         hpx::apply(executors.at(current_numa_domain), &work_item::process, std::move(work));
                    }

                    return;
                }
            default:
                HPX_ASSERT(false);
        }
        HPX_ASSERT(schedule_id);
        HPX_ASSERT(work.valid());
    	//std::cout<< "schedule_id for distributed enque is : " <<  schedule_id << std::endl;

        hpx::apply<enqueue_action>(schedule_id, work, this_work_item::get_id());
    }

    bool scheduler::do_split(work_item const& w)
    {
//         return (w.id().depth() <= 1.5 * hpx::get_os_thread_count());
        // FIXME: make the cut off runtime configurable...
        if (w.id().depth() > depth_cap) return false;

    	//std::cout<< " wcansplit: " << w.can_split()<<std::endl;
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
        if ( num_threads_ > 1 && input_objective == scheduler::objectives.at(objective_IDs::TIME_RESOURCE) )
        {
            std::unique_lock<mutex_type> l(resize_mtx_);
            if ( current_avg_iter_time == 0.0 || allscale_monitor->get_number_of_iterations() < sampling_interval)
            {
//                std::cout << "Avg last iterations:  " << allscale_monitor->get_avg_time_last_iterations(10) << " Number of iterations: " << allscale_monitor->get_number_of_iterations() << std::endl;
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

                std::size_t suspend_cap = 1; //active_threads < SMALL_SYSTEM  ? SMALL_SUSPEND_CAP : LARGE_SUSPEND_CAP;
                std::size_t resume_cap = 1;  //active_threads < SMALL_SYSTEM  ? LARGE_RESUME_CAP : SMALL_RESUME_CAP;

                if ( active_threads > 4 && last_avg_iter_time >= current_avg_iter_time )
                {
                    depth_cap = (1.5 * (std::log(active_threads)/std::log(2) + 0.5));
                    {
                        hpx::util::unlock_guard<std::unique_lock<mutex_type> > ul(l);
                        thread_scheduler->disable_more(suspend_cap);
                    }
                    std::cout << "Sent disable signal. Active threads: " << active_threads - suspend_cap << std::endl;
                }
                else if ( blocked_os_threads_.any() && last_avg_iter_time < current_avg_iter_time )
                {
                    depth_cap = (1.5 * (std::log(active_threads)/std::log(2) + 0.5));
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



    void scheduler::stop()
    {
//         timer_.stop();

//         if (input_objective == scheduler::objectives.at(objective_IDs::TIME_RESOURCE))
//             throttle_timer_.stop();

        if(stopped_)
            return;

        //Resume all sleeping threads
        if (input_objective == scheduler::objectives.at(objective_IDs::TIME_RESOURCE) )
	    thread_scheduler->enable_more(os_thread_count);

        stopped_ = true;
//         work_queue_cv_.notify_all();
//         std::cout << "rank(" << rank_ << "): scheduled " << count_ << "\n";

        hpx::future<void> stop_left;
        if(left_)
            hpx::future<void> stop_left = hpx::async<stop_action>(left_);
        hpx::future<void> stop_right;
        if(right_)
            hpx::future<void> stop_right = hpx::async<stop_action>(right_);

//         {
//             std::unique_lock<mutex_type> l(work_queue_mtx_);
//             while(!work_queue_.empty())
//             {
//                 std::cout << "rank(" << rank_ << "): waiting to become empty " << work_queue_.size() << "\n";
//                 work_queue_cv_.wait(l);
//             }
//             std::cout << "rank(" << rank_ << "): done. " << count_ << "\n";
//         }
        if(stop_left.valid())
            stop_left.get();
        if(stop_right.valid())
            stop_right.get();
        left_ = hpx::id_type();
        right_ = hpx::id_type();
    }

}}
