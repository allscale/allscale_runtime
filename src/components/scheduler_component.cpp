
#include <allscale/components/scheduler.hpp>
#include <allscale/monitor.hpp>

#include <hpx/util/scoped_unlock.hpp>
#include <iterator>
#include <stdlib.h>    

namespace allscale { namespace components {

    scheduler::scheduler(std::uint64_t rank)
      : num_localities_(hpx::get_num_localities().get())
      , num_threads_(hpx::get_num_worker_threads())
      , rank_(rank)
      , schedule_rank_(0)
      , stopped_(false)
      , os_thread_count(hpx::get_os_thread_count())
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
	    blocked_os_threads_.resize(os_thread_count);
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

        input_objective = hpx::get_config_entry("allscale.objective", scheduler::objectives.at(Objective_Ids::TIME)  );

        if ( std::find(scheduler::objectives.begin(), scheduler::objectives.end(), input_objective) == scheduler::objectives.end() ) { 
	    std::ostringstream all_keys;
	    copy(scheduler::objectives.begin(), scheduler::objectives.end(), std::ostream_iterator<std::string>(all_keys, ","));
	    std::string keys_str = all_keys.str();
            keys_str.pop_back();
            HPX_THROW_EXCEPTION(hpx::bad_request, "scheduler::init", 
				boost::str(boost::format("Wrong objective: %s, Valid values: [%s]") % input_objective % keys_str));
  	} else
	    std::cout << "The requested objective is " << input_objective << std::endl;


        // setup performance counter to use to decide on split/process
        static const char * queue_counter_name = "/threadqueue{locality#%d/total}/length";
        static const char * idle_counter_name = "/threads{locality#%d/total}/idle-rate";
        static const char * allscale_app_counter_name = "/allscale{locality#*/total}/examples/fib_time";
        //static const char * threads_time_total = "/threads{locality#*/total}/time/overall";

        const std::uint32_t prefix = hpx::get_locality_id();

        queue_length_counter_ = hpx::performance_counters::get_counter(
            boost::str(boost::format(queue_counter_name) % prefix));

        hpx::performance_counters::stubs::performance_counter::start(
            hpx::launch::sync, queue_length_counter_);

        idle_rate_counter_ = hpx::performance_counters::get_counter(
            boost::str(boost::format(idle_counter_name) % prefix));

        hpx::performance_counters::stubs::performance_counter::start(hpx::launch::sync, idle_rate_counter_);

        allscale_app_counter_id = hpx::performance_counters::get_counter(allscale_app_counter_name);
        hpx::performance_counters::stubs::performance_counter::start(hpx::launch::sync, allscale_app_counter_id);

        collect_counters();

        numa_domains = hpx::compute::host::numa_domains();
        auto num_numa_domains = numa_domains.size();
        executors.reserve(num_numa_domains);
        for (hpx::compute::host::target const & domain : numa_domains) {
            auto num_pus = domain.num_pus();
            //Create executor per numa domain
            executors.emplace_back(num_pus.first, num_pus.second);
            std::cout << "Numa num_pus.first: " << num_pus.first << ", num_pus.second: " << num_pus.second << ". Total numa domains: " << numa_domains.size() << std::endl;
        }
        timer_.start();

        throttle_timer_.start();
        std::cout
            << "Scheduler with rank "
            << rank_ << " created (" << left_ << " " << right_ << ")!\n";
    }

    void scheduler::enqueue(work_item work, this_work_item::id const& id)
    {
        std::uint64_t schedule_rank = 0;
        //std::cout<<"remote is " << remote << std::endl;
        if (!id)
            schedule_rank = schedule_rank_.fetch_add(1) % 3;
        else
            this_work_item::set_id(id);

        hpx::id_type schedule_id;

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
                        monitor::signal(monitor::work_item_first, work);
                    monitor::signal(monitor::work_item_enqueued, work);
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
    	//std::cout<< " wcansplit: " << w.can_split()<<std::endl;
        if (!w.can_split()) return false;
        //FIXME: think about if locking
        //counters_mtx_ could lead to a potential dead_lock situatione
        //when more than one enque action is active (this can be the case due
        //to hpx apply), and the thread holding the lock is suspended and the
        //others start burning up cpu time by spinning to get the lock
        std::unique_lock<mutex_type> l(counters_mtx_);
        // Do we have enough tasks in the system?
        if (queue_length_ < num_threads_ * 10 )
        {
//        	std::cout<<"not enough tasks and total_idlerate: " << total_idle_rate_ << " queue length " << total_length_ << std::endl;
        	//TODO: Think of some smart way to solve this, as of now, having new split workitems spawned
        	// in system that does not have many tasks with idle_Rate>=x and x being to low can lead to endless loops:
        	// as new items get spawned, they are too fine  granular, leading to them being not further processed,but
        	// due to short queue length and too low idle rate requirement for NOT splitting anymore, splitting keeps going on
            return idle_rate_ >= 10.0;
        }
//    	std::cout<<"enough tasks and total_idlerate: " << total_idle_rate_ << " queue length " << total_length_ << std::endl;

        return idle_rate_ < 10.0;
    }

    bool scheduler::collect_counters()
    {
        hpx::performance_counters::counter_value idle_value;
        hpx::performance_counters::counter_value length_value;
        idle_value = hpx::performance_counters::stubs::performance_counter::get_value(
                hpx::launch::sync, idle_rate_counter_);
        length_value = hpx::performance_counters::stubs::performance_counter::get_value(
                hpx::launch::sync, queue_length_counter_);

        std::unique_lock<mutex_type> l(counters_mtx_);

        idle_rate_ = idle_value.get_value<double>() * 0.01;
        queue_length_ = length_value.get_value<std::size_t>();

	    hpx::util::unlock_guard<std::unique_lock<mutex_type> > ul(l);
        return true;
    }


    bool scheduler::periodic_throttle()
    {
        if ( num_threads_ > 1 && input_objective == scheduler::objectives.at(Objective_Ids::TIME_RESOURCE) ) {
		const std::size_t worker_tid = hpx::get_worker_thread_num();

		auto allscale_app_counter = hpx::performance_counters::stubs::performance_counter::get_value(
						hpx::launch::sync, allscale_app_counter_id);

		{
		        std::unique_lock<mutex_type> l(resize_mtx_);
			allscale_app_time = allscale_app_counter.get_value<std::int64_t>();  
 			std::size_t active_threads = os_thread_count - blocked_os_threads_.count();

			const std::size_t MIN_THREADS = 10;
			if (active_threads < MIN_THREADS) return true; 			//Don't go below 10

			const std::size_t SMALL_SYSTEM = 32;                      	//TOOD: make it configurable
        	        const std::size_t SMALL_SUSPEND_CAP = 1;                        //TODO make it configurable
	                const std::size_t LARGE_SUSPEND_CAP = active_threads * 0.20;    //TODO make it configurable

                        std::size_t suspend_cap = active_threads < SMALL_SYSTEM  ? SMALL_SUSPEND_CAP : LARGE_SUSPEND_CAP;
                        std::size_t suspend_cnt = 0;

			if ( allscale_app_time > 0 )
			  if ( last_thread_time ==0 || allscale_app_time < last_thread_time ) {


			     for (int ti=0; ti<os_thread_count; ti++) {

				  if (blocked_os_threads_[ti] || ti == worker_tid) 
				      continue;

			          std::cout << "Suspend attempt: tid = " << ti << ", allscale_app_time = " << allscale_app_time;
				  std::cout << ", suspend_cap: " << suspend_cap;
				  std::cout << ", last_tt = " << last_thread_time << ", wtid = " << worker_tid << ", active_threads: " << active_threads << std::endl;
				  
				  {
					hpx::util::unlock_guard<std::unique_lock<mutex_type> > ul(l);
					suspend(ti);
				  }

				  suspend_cnt++;
				  active_threads = os_thread_count - blocked_os_threads_.count();
//				  std::cout << "Suspend finished: " << ti << std::endl;
                                    
				  if (suspend_cnt == suspend_cap) 
				     break;
			     }

			  } else if ( blocked_os_threads_.any() && allscale_app_time > last_thread_time ) {
			      resumed_one = false;
         	              hpx::util::unlock_guard<std::unique_lock<mutex_type> > ul(l);
                    	      resume_one();
	                  }

		}

		{
	      	    std::unique_lock<mutex_type> l(resize_mtx_);
		    last_thread_time = allscale_app_time;
		}
        }

        return true;
    }



    void scheduler::stop()
    {
        timer_.stop();
        throttle_timer_.stop();
        if(stopped_)
            return;

        //Resume all sleeping threads
	resume_all();

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


// Methods for thread throttling
    void scheduler::suspend(std::size_t shepherd)
    {

        if (blocked_os_threads_.size() - blocked_os_threads_.count() < 2 ) {
            std::cout << "Not enough running threads to suspend. size: " << blocked_os_threads_.size() << ", count: " << blocked_os_threads_.count() << std::endl;
	    return;
        }


        // If the current thread is not the requested one, re-schedule a new
        // PX thread in order to retry.
        std::size_t thread_num = hpx::get_worker_thread_num();
        if (thread_num != shepherd) {
            register_suspend_thread(shepherd);
            return;
        }

        std::lock_guard<mutex_type> l(throttle_mtx_);

        if (shepherd >= blocked_os_threads_.size()) {
            HPX_THROW_EXCEPTION(hpx::bad_parameter, "scheduler::suspend",
                "invalid thread number");
        }

        bool is_suspended = blocked_os_threads_[shepherd];
        if (!is_suspended) {
            blocked_os_threads_[shepherd] = true;
            register_thread(shepherd);
//	    std::cout << "Suspend. Registered thread: " << shepherd << std::endl;
        }
    }

    void scheduler::resume(std::size_t shepherd)
    {
        std::lock_guard<mutex_type> l(throttle_mtx_);

        if (shepherd == -1)
	   return;

        if (shepherd >= blocked_os_threads_.size()) {
            HPX_THROW_EXCEPTION(hpx::bad_parameter, "scheduler::resume",
                "invalid thread number");
        }

        blocked_os_threads_[shepherd] = false;   // re-activate shepherd
    }


    void scheduler::resume_one()
    {
       std::lock_guard<mutex_type> l(throttle_mtx_);
       if (!resumed_one && blocked_os_threads_.any())
          for (int i=0; i<blocked_os_threads_.size(); i++)
	      if (blocked_os_threads_[i]) {
                 blocked_os_threads_[i] = false;
		 resumed_one = true;
		 std::cout << "Resumed worker_id: " << i << std::endl;
                 return;
              }
    }

    void scheduler::resume_all()
    {
        std::lock_guard<mutex_type> l(throttle_mtx_);

        if (blocked_os_threads_.any())
           for (int i=0; i<blocked_os_threads_.size(); i++)
               if (blocked_os_threads_[i])
                  blocked_os_threads_[i] = false;
    }


    // do the requested throttling
    void scheduler::throttle_controller(std::size_t shepherd)
    {
        std::unique_lock<mutex_type> l(throttle_mtx_);
        if (!blocked_os_threads_[shepherd])
            return;     // nothing more to do

        {
            // put this shepherd thread to sleep for 100ms
            hpx::util::unlock_guard<std::unique_lock<mutex_type> > ul(l);

            boost::this_thread::sleep_for(boost::chrono::milliseconds(100));
        }

        // if this thread still needs to be suspended, re-schedule this routine
        // which will give the thread manager some cycles to tend to the high
        // priority tasks which might have arrived
        if (blocked_os_threads_[shepherd])
            register_thread(shepherd);
    }

    // schedule a high priority task on the given shepherd thread
    void scheduler::register_thread(std::size_t shepherd)
    {
        std::string description("throttle controller for shepherd thread (" +
            std::to_string(shepherd) + ")");

        hpx::applier::register_thread(
            hpx::util::bind(&scheduler::throttle_controller, this, shepherd),
            description.c_str(),
            hpx::threads::pending, true,
            hpx::threads::thread_priority_critical,
            shepherd);
    }


       // schedule a high priority task on the given shepherd thread to suspend
    void scheduler::register_suspend_thread(std::size_t shepherd)
    {
        std::string description("suspend shepherd thread (" +
            std::to_string(shepherd) + ")");

        hpx::applier::register_thread(
            hpx::util::bind(&scheduler::suspend, this, shepherd),
            description.c_str(),
            hpx::threads::pending, true,
            hpx::threads::thread_priority_critical,
            shepherd);
    }


}}
