
#include <allscale/components/scheduler_network.hpp>
#include <allscale/scheduler.hpp>

#include <vector>

namespace allscale { namespace components {
    void scheduler_network::schedule(std::size_t rank, work_item work, this_work_item::id id)
    {
        hpx::id_type scheduler_id;

        typedef allscale::scheduler::schedule_action enqueue_action;

        {
            boost::shared_lock<mutex_type> l(mtx_);
            auto it = schedulers_.find(rank);
            // If we couldn't find it in the map, we resolve the name.
            // This is happening asynchronously. Once the name was resolved,
            // the work item gets enqueued and the id is being put in the
            // network map.
            //
            // If we are able to locate the rank in our map, we just go ahead and
            // schedule it.

            // ToDo: make sure this call transitions to "start work item" -> This is
            // NOT covered currently by the resilience protocol
            if (it == schedulers_.end())
            {
                l.unlock();
                scheduler_id = hpx::naming::get_id_from_locality_id(rank);
                {
                    std::unique_lock<mutex_type> l(mtx_);
                    schedulers_[rank] = scheduler_id;
                }
            }
            else
            {
                scheduler_id = it->second;
            }
        }

        HPX_ASSERT(scheduler_id);
        hpx::apply<enqueue_action>(scheduler_id, std::move(work), std::move(id));
    }

    void scheduler_network::stop()
    {
        std::unordered_map<std::size_t, hpx::id_type> schedulers;
        {
            std::unique_lock<mutex_type> l(mtx_);
            std::swap(schedulers_, schedulers);
        }

        std::vector<hpx::future<void>> stop_futures;
        stop_futures.reserve(schedulers.size());

        typedef allscale::scheduler::stop_action stop_action;

        for (auto& p: schedulers_)
        {
            stop_futures.push_back(hpx::async<stop_action>(p.second));
        }

        // Check for exceptions...
        hpx::when_all(stop_futures).get();
    }
}}
