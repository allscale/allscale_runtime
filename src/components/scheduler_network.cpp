
#include <allscale/components/scheduler_network.hpp>
#include <allscale/components/scheduler.hpp>

#include <hpx/util/bind.hpp>

#include <vector>

namespace allscale { namespace components {
    void scheduler_network::schedule(std::size_t rank, work_item work, this_work_item::id const& id)
    {
        hpx::id_type scheduler_id;

        typedef scheduler::enqueue_action enqueue_action;

        {
            std::unique_lock<mutex_type> l(mtx_);
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
                hpx::find_from_basename("allscale/scheduler", rank).then(
                    hpx::util::bind(
                        hpx::util::one_shot(
                        [this, rank](hpx::future<hpx::id_type> fut, work_item work, this_work_item::id const& id)
                        {
                            hpx::id_type scheduler_id = fut.get();
                            hpx::apply<enqueue_action>(scheduler_id, std::move(work), id);
                            {
                                std::unique_lock<mutex_type> l(mtx_);
                                // We need to search again in case of concurrent
                                // lookups.
                                auto it = schedulers_.find(rank);
                                if (it == schedulers_.end())
                                {
                                    schedulers_.insert(std::make_pair(rank, scheduler_id));
                                }
                            }
                        }), hpx::util::placeholders::_1, std::move(work), id));
                return;
            }
            scheduler_id = it->second;
        }

        HPX_ASSERT(scheduler_id);
        hpx::apply<enqueue_action>(scheduler_id, std::move(work), id);
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

        typedef scheduler::stop_action stop_action;

        for (auto& p: schedulers_)
        {
            stop_futures.push_back(hpx::async<stop_action>(p.second));
        }

        // Check for exceptions...
        hpx::when_all(stop_futures).get();
    }
}}
