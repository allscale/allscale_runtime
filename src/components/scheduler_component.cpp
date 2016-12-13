
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
    {
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

        static const char * queue_counter_name = "/threadqueue{locality#%d/worker-thread#%d}/length";
        static const char * idle_counter_name = "/threads{locality#%d/worker-thread#%d}/idle-rate";

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

        collect_counters();

        timer_.start();
        std::cout
            << "Scheduler with rank "
            << rank_ << " created (" << left_ << " " << right_ << ")!\n";
    }

    void scheduler::enqueue(work_item work)
    {
        std::uint64_t schedule_rank = schedule_rank_.fetch_add(1) % 3;

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

                    if (do_split(work))
                    {
                        hpx::apply(&work_item::split, std::move(work));
                    }
                    else
                    {
                        hpx::apply(&work_item::process, std::move(work));
                    }

                    return;
                }
            default:
                HPX_ASSERT(false);
        }
        HPX_ASSERT(schedule_id);
        HPX_ASSERT(work.valid());
        hpx::apply<enqueue_action>(schedule_id, work);
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

//             std::cout << "Collecting[" << num_thread << "] " << idle_rates_[num_thread] << " " << queue_length_[num_thread] << "\n";
        }

        total_idle_rate_ = total_idle_rate_ / num_threads;
        total_length_ = total_length_ / num_threads;

        return true;
    }

    void scheduler::stop()
    {
        timer_.stop();
        if(stopped_)
            return;

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
