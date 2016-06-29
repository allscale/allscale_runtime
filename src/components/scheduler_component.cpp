
#include <allscale/components/scheduler.hpp>

#include <hpx/util/scoped_unlock.hpp>

namespace allscale { namespace components {
    scheduler::scheduler(std::uint64_t rank)
      : num_localities_(hpx::get_num_localities().get())
      , rank_(rank)
      , schedule_rank_(0)
      , stopped_(false)
      , count_(0)
      , timer_(
            hpx::util::bind(
                &scheduler::steal_work,
                this
            ),
            100,
            "scheduler::steal_work",
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
                    {
                        std::unique_lock<mutex_type> l(work_queue_mtx_);
                        work_queue_.push_back(std::move(work));
                        //std::cout << "rank(" << rank_ << "): enqueue ... " << work_queue_.size() << " (" << count_ << ")\n";
                    }
                    work_queue_cv_.notify_all();
                    return;
                }
            default:
                HPX_ASSERT(false);
        }
        HPX_ASSERT(schedule_id);
        HPX_ASSERT(work.valid());
        hpx::apply<enqueue_action>(schedule_id, work);
    }

    std::vector<work_item> scheduler::dequeue()
    {
        std::unique_lock<mutex_type> l(work_queue_mtx_);
        std::vector<work_item> res;
        if(work_queue_.empty()) return res;

        std::size_t n = work_queue_.size() - 1;
        res.reserve(n);

        while(res.size() < n)
        {
            if(work_queue_.empty()) break;
            res.push_back(std::move(work_queue_.back()));
            work_queue_.pop_back();
        }
        return res;
    }

    bool scheduler::steal_work()
    {
        {
            std::unique_lock<mutex_type> l(work_queue_mtx_);
            if(!work_queue_.empty())
                return true;
        }
        auto completion_handler =
            [this](hpx::future<std::vector<work_item>> f)
            {
                std::vector<work_item> items = f.get();
                if(!items.empty())
                {
//                     std::cout << "rank(" << rank_ << ") stole work\n";
                    {
                        std::unique_lock<mutex_type> l(work_queue_mtx_);
                        for(auto && work : items)
                        {
                            work_queue_.push_back(std::move(work));
                        }
                    }
                    work_queue_cv_.notify_all();
                }
            };
        if(left_)
            hpx::async<dequeue_action>(left_).then(completion_handler);
        if(right_)
            hpx::async<dequeue_action>(right_).then(completion_handler);

        return true;
    }

    void scheduler::run()
    {
        while (true)
        {
            work_item work;
            {
                std::unique_lock<mutex_type> l(work_queue_mtx_);
//                 std::cout << "rank(" << rank_ << "): running ... " << work_queue_.size() << " (" << count_ << ")\n";
                while(work_queue_.empty() && !stopped_)
                {
                    work_queue_cv_.wait(l);
//                     std::cout << "rank(" << rank_ << "): got notified ... " << work_queue_.size() << " (" << count_ << ")\n";
                }
                if(stopped_ && work_queue_.empty())
                {
                    std::cout << "rank(" << rank_ << "): scheduler stopped ...\n";
                    return;
                }
                HPX_ASSERT(!work_queue_.empty());
                work = std::move(work_queue_.front());
                work_queue_.pop_front();
            }
            HPX_ASSERT(work.valid());
            hpx::apply(&work_item::execute, std::move(work));
            ++count_;
        }
        work_queue_cv_.notify_all();
    }

    void scheduler::stop()
    {
        timer_.stop();
        if(stopped_)
            return;

        stopped_ = true;
        work_queue_cv_.notify_all();
        std::cout << "rank(" << rank_ << "): scheduled " << count_ << "\n";

        hpx::future<void> stop_left;
        if(left_)
            hpx::future<void> stop_left = hpx::async<stop_action>(left_);
        hpx::future<void> stop_right;
        if(right_)
            hpx::future<void> stop_right = hpx::async<stop_action>(right_);

        {
            std::unique_lock<mutex_type> l(work_queue_mtx_);
            while(!work_queue_.empty())
            {
                std::cout << "rank(" << rank_ << "): waiting to become empty " << work_queue_.size() << "\n";
                work_queue_cv_.wait(l);
            }
            std::cout << "rank(" << rank_ << "): done. " << count_ << "\n";
        }
        if(stop_left.valid())
            stop_left.get();
        if(stop_right.valid())
            stop_right.get();
        left_ = hpx::id_type();
        right_ = hpx::id_type();
    }
}}
