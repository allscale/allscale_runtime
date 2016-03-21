
#include <allscale/components/scheduler.hpp>

#include <hpx/util/scoped_unlock.hpp>

namespace allscale { namespace components {
    scheduler::scheduler(std::uint64_t rank)
      : rank_(rank)
      , stopped_(false)
      , count_(0)
    {
    }

    void scheduler::init()
    {
        // Find neighbors...
        std::uint64_t num_localities = hpx::get_num_localities().get();
        std::uint64_t left_id = rank_ == 0 ? num_localities - 1 : rank_ - 1;
        std::uint64_t right_id = rank_ == num_localities - 1 ? 0 : rank_ + 1;

        hpx::future<hpx::id_type> right_future =
            hpx::find_from_basename("allscale/scheduler", right_id);
        if(left_id != right_id)
        {
            hpx::future<hpx::id_type> left_future =
                hpx::find_from_basename("allscale/scheduler", left_id);
            left_ = left_future.get();
        }
        if(num_localities > 1)
            right_ = right_future.get();

        std::cout << "Scheduler with rank " << rank_ << " created (" << left_id << " " << right_id << ")!\n";
    }

    void scheduler::enqueue(work_item work)
    {
        {
            boost::unique_lock<mutex_type> l(work_queue_mtx_);
            if(right_ && work_queue_.size() > 100)
            {
                l.unlock();
                hpx::apply<enqueue_action>(right_, work);
                return;
            }
            work_queue_.push_back(std::move(work));
        }
        work_queue_cv_.notify_all();
    }

    std::vector<work_item> scheduler::dequeue()
    {
        boost::unique_lock<mutex_type> l(work_queue_mtx_);
        std::vector<work_item> res;
        if(work_queue_.empty()) return res;

        std::size_t n = work_queue_.size() - 1;
        res.reserve(n);

        while(res.size() < n)
        {
            if(work_queue_.empty()) break;
            res.push_back(std::move(work_queue_.front()));
            work_queue_.pop_front();
        }
        return res;
    }

    void scheduler::steal_work()
    {
        auto completion_handler =
            [this](hpx::future<std::vector<work_item>> f)
            {
                std::vector<work_item> items = f.get();
                if(!items.empty())
                {
                    std::cout << "rank(" << rank_ << ") stole work\n";
                    {
                        boost::unique_lock<mutex_type> l(work_queue_mtx_);
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
    }

    void scheduler::run()
    {
        while (true)
        {
            work_item work;
            {
                boost::unique_lock<mutex_type> l(work_queue_mtx_);
//                 std::cout << "rank(" << rank_ << "): running ... " << work_queue_.size() << "\n";
                while(work_queue_.empty())
                {
                    {
                        hpx::util::scoped_unlock<boost::unique_lock<mutex_type>> ul(l);
//                         std::cout << "rank(" << rank_ << "): stealing ... " << count_ << "\n";
                        steal_work();
//                         std::cout << "rank(" << rank_ << "): stealing (scheduled)... " << count_ << "\n";
                    }
                    hpx::lcos::local::cv_status status =
                        work_queue_cv_.wait_for(
                            l, boost::chrono::milliseconds(1)
                        );
                    if(status == hpx::lcos::local::cv_status::timeout)
                    {
                        hpx::util::scoped_unlock<boost::unique_lock<mutex_type>> ul(l);
                        steal_work();
                    }
//                     std::cout << "rank(" << rank_ << "): got ... " << work_queue_.size() << " done\n";
                }
                work = std::move(work_queue_.front());

                if(stopped_ && work_queue_.empty())
                    break;

                work_queue_.pop_front();
            }
            if(work.valid())
            {
                work.execute();
                ++count_;
            }
            if(count_ % 100 == 0)
                std::cout << "rank(" << rank_ << "): scheduled " << count_ << "\n";

        }
        work_queue_cv_.notify_all();
    }

    void scheduler::stop()
    {
        stopped_ = true;
        std::cout << "rank(" << rank_ << "): scheduled " << count_ << "\n";
        std::uint64_t num_localities = hpx::get_num_localities().get();
        if(left_ && rank_ > 0 )
            hpx::async<stop_action>(left_).get();
        if(right_ && rank_ < num_localities - 1)
            hpx::async<stop_action>(right_).get();
        {
            boost::unique_lock<mutex_type> l(work_queue_mtx_);
            while(!work_queue_.empty())
            {
                work_queue_cv_.wait(l);
            }
        }
        left_ = hpx::id_type();
        right_ = hpx::id_type();
    }
}}
