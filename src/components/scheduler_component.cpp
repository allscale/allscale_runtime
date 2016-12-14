
#include <allscale/components/scheduler.hpp>
#include <allscale/monitor.hpp>

#include <hpx/util/scoped_unlock.hpp>


namespace allscale { namespace components {
    scheduler::scheduler(std::uint64_t rank)
      : num_localities_(hpx::get_num_localities().get())
      , rank_(rank)
      , schedule_rank_(0)
      , stopped_(false)
//       , count_(0)
      , timer_(
            hpx::util::bind(
                &scheduler::collect_counters,
                this
            ),
            1000,
            "scheduler::collect_counters",
            true
        )
      , throttle_timer_(
           hpx::util::bind(
               &scheduler::periodic_throttle,
               this
           ),
           10000, //0.1 sec 
           "scheduler::periodic_throttle",
           true
        )
    {
	blocked_os_threads_.resize(hpx::get_os_thread_count());
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

        // setup performance counter to use to decide on split/process
        static const char * queue_counter_name = "/threadqueue{locality#%d/worker-thread#%d}/length";
        static const char * idle_counter_name = "/threads{locality#%d/worker-thread#%d}/idle-rate";
        //static const char * threads_time_total = "/threads{locality#*/total}/time/overall";
	static const char * allscale_app_counter_name = "/allscale{locality#*/total}/examples/fib_time";

        std::size_t num_threads = hpx::get_num_worker_threads();
        idle_rates_counters_.reserve(num_threads);
        idle_rates_.reserve(num_threads);
        queue_length_counters_.reserve(num_threads);
        queue_length_.reserve(num_threads);

        for (std::size_t num_thread = 0; num_thread != num_threads; ++num_thread)
        {
            const std::uint32_t prefix = hpx::get_locality_id();
            const std::size_t worker_tid = hpx::get_worker_thread_num();

            hpx::id_type queue_counter_id = hpx::performance_counters::get_counter(
                boost::str(boost::format(queue_counter_name) % prefix % worker_tid));

            hpx::performance_counters::stubs::performance_counter::start(hpx::launch::sync, queue_counter_id);
            queue_length_counters_.push_back(queue_counter_id);

            hpx::id_type idle_counter_id = hpx::performance_counters::get_counter(
                boost::str(boost::format(idle_counter_name) % prefix % worker_tid));

            hpx::performance_counters::stubs::performance_counter::start(hpx::launch::sync, idle_counter_id);
            idle_rates_counters_.push_back(idle_counter_id);
        }

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

    void scheduler::enqueue(work_item work, bool remote)
    {

        std::uint64_t schedule_rank = 0;
        if (!remote)
            schedule_rank = schedule_rank_.fetch_add(1) % 3;

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
                    monitor::signal(monitor::work_item_enqueued, work);

                    std::size_t current_numa_domain = ++current_ % executors.size();

                    if (do_split(work))
                    {
                        hpx::apply(executors.at(current_numa_domain), &work_item::split, std::move(work));
                    }
                    else
                    {
                        hpx::apply(executors.at(current_numa_domain), &work_item::process, std::move(work));
                    }

                    return;
                }
            default:
                HPX_ASSERT(false);
        }
        HPX_ASSERT(schedule_id);
        HPX_ASSERT(work.valid());
        hpx::apply<enqueue_action>(schedule_id, work, true);
    }

    bool scheduler::do_split(work_item const& w)
    {
        std::unique_lock<mutex_type> l(counters_mtx_);
        hpx::util::ignore_while_checking<std::unique_lock<mutex_type>> il(&l);
        std::size_t num_threads = hpx::get_num_worker_threads();
        // Do we have enough tasks in the system?
        if (total_length_ < num_threads * 10)
        {
//             std::cout << total_length_ << " " << total_idle_rate_ << "\n";
            return total_idle_rate_ >= 10.0;
        }

        return total_idle_rate_ < 10.0;
    }

    bool scheduler::collect_counters()
    {
        std::size_t num_threads = hpx::get_num_worker_threads();

        std::unique_lock<mutex_type> l(counters_mtx_);
        hpx::util::ignore_while_checking<std::unique_lock<mutex_type>> il(&l);

        total_idle_rate_ = 0.0;
        total_length_ = 0;

        for (std::size_t num_thread = 0; num_thread != num_threads; ++num_thread)
        {
            auto idle_value = hpx::performance_counters::stubs::performance_counter::get_value(
                    hpx::launch::sync, idle_rates_counters_[num_thread]);
            auto length_value = hpx::performance_counters::stubs::performance_counter::get_value(
                    hpx::launch::sync, queue_length_counters_[num_thread]);

            idle_rates_[num_thread] = idle_value.get_value<double>() * 0.01;
            queue_length_[num_thread] = length_value.get_value<std::size_t>();

            total_idle_rate_ += idle_rates_[num_thread];
            total_length_ += queue_length_[num_thread];

           //  std::cout << "Collecting[" << num_thread << "] " << idle_rates_[num_thread] << " " << queue_length_[num_thread] << "\n";
        }

        total_idle_rate_ = total_idle_rate_ / num_threads;
        total_length_ = total_length_ / num_threads;
        
        return true;
    }


    bool scheduler::periodic_throttle()
    {
        const std::size_t num_threads = hpx::get_os_thread_count();
        const std::size_t worker_tid = hpx::get_worker_thread_num();


        if ( num_threads > 1 ) {

		auto allscale_app_counter = hpx::performance_counters::stubs::performance_counter::get_value(
						hpx::launch::sync, allscale_app_counter_id);

		{
		        std::unique_lock<mutex_type> l(resize_mtx_);

			allscale_app_time = allscale_app_counter.get_value<std::int64_t>();

			std::int64_t neighbour = std::rand() %  num_threads ;  //(worker_tid < num_threads - 1) ? worker_tid + 1 : worker_tid - 1;

//			if ( allscale_app_time > last_thread_time )
//  			   std::cout << "wtid = " << worker_tid << ", allscale_app_time = " 
//				<< allscale_app_time << ", last_thread_time = " << last_thread_time << ", neighbour = "<< neighbour << std::endl;

			if ( allscale_app_time > 0 ) 
			  if ( last_thread_time ==0 || allscale_app_time < last_thread_time ) {
			     if ( neighbour != worker_tid ) {
//			           std::cout << "Suspend attempt: tid = " << neighbour << ", allscale_app_time = " 
//			 		<< allscale_app_time << ", last_tt = " << last_thread_time << ", wtid = " << worker_tid << std::endl;
//			 	  hpx::util::unlock_guard<std::unique_lock<mutex_type> > ul(l);
                                  suspend(neighbour);
			     }
			  } else if ( blocked_os_threads_.any() && allscale_app_time > last_thread_time ) {
//	                       std::cout << "Resume attempt: tid = " << neighbour << ", allscale_app_time = " 
//			 		<< allscale_app_time << ", last_tt = " << last_thread_time << ", wtid = " << worker_tid << std::endl;
//         	              hpx::util::unlock_guard<std::unique_lock<mutex_type> > ul(l);
                    	      resume_one();
	                  } 

		}

		{
//	      	    std::unique_lock<mutex_type> l(resize_mtx_); 
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
        }
    }

    void scheduler::resume(std::size_t shepherd)
    {
        std::lock_guard<mutex_type> l(throttle_mtx_);

        if (shepherd >= blocked_os_threads_.size()) {
            HPX_THROW_EXCEPTION(hpx::bad_parameter, "scheduler::resume",
                "invalid thread number");
        }

        blocked_os_threads_[shepherd] = false;   // re-activate shepherd
    }

    
    void scheduler::resume_one()
    {
       std::lock_guard<mutex_type> l(throttle_mtx_);
       if (blocked_os_threads_.any())
          for (int i=0; i<blocked_os_threads_.size(); i++) 
	      if (blocked_os_threads_[i]) {
                 blocked_os_threads_[i] = false;
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
