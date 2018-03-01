#include <allscale/components/resilience.hpp>
#include <allscale/resilience.hpp>
#include <allscale/monitor.hpp>
#include <allscale/scheduler.hpp>
#include <allscale/work_item.hpp>

#include <hpx/include/iostreams.hpp>
#include <hpx/include/thread_executors.hpp>
#include <hpx/util/detail/yield_k.hpp>
#include <hpx/util/asio_util.hpp>
#include <chrono>
#include <thread>

//#include <boost/asio.hpp>

using std::chrono::milliseconds;

namespace allscale { namespace components {

    std::size_t resilience::protectee_rank_ = 0;

    resilience::resilience(std::uint64_t rank)
        : rank_(rank)
    {
    }

    void resilience::thread_safe_printer(std::string output) {
#ifdef DEBUG_
        std::cout << "DEBUG (" << rank_ << "):" << output << std::flush;
#endif //DEBUG_
    }

    void resilience::send_heartbeat(std::size_t counter) {
        protectee_heartbeat = counter;
    }

    void resilience::set_guard(hpx::id_type guard, uint64_t guard_rank) {
        guard_ = guard;
        guard_.make_unmanaged();
        guard_rank_ = guard_rank;
    }

    void resilience::failure_detection_loop_async() {
        if (get_running_ranks() < 2 || env_resilience_disabled) {
            return;
        }

        hpx::apply(&resilience::send_heartbeat_loop, this);
    }

    void resilience::send_heartbeat_loop() {
        while (keep_running) {
            std::this_thread::sleep_for(milliseconds(miu));
            auto t_now =  std::chrono::high_resolution_clock::now();
            my_heartbeat = std::chrono::duration_cast<std::chrono::milliseconds>(t_now-start_time).count()/1000;
            hpx::apply<send_heartbeat_action>(guard_, my_heartbeat);
            thread_safe_printer("my counter = " + std::to_string(my_heartbeat) + " protectee COUNTER = " + std::to_string(protectee_heartbeat) + "\n");
            if (my_heartbeat > protectee_heartbeat + 10) {
                thread_safe_printer("DETECTED FAILURE!!!\n");
                // here comes recovery
                my_state = SUSPECT;
                protectee_crashed();
                thread_safe_printer("Recovery done\n");
            }

        }
    }

    bool resilience::rank_running(uint64_t rank) {
        bool rank_running;
        {
            std::unique_lock<mutex_type> lock(running_ranks_mutex_);
            rank_running = rank_running_[rank];
        }
        return rank_running;
    }

    std::pair<hpx::id_type,uint64_t> resilience::get_protectee() {
        return std::make_pair(protectee_, protectee_rank_);
    }

    void resilience::init() {
        char *env = std::getenv("ALLSCALE_RESILIENCE");
        env_resilience_disabled = (env && env[0] == '0');

        num_localities = hpx::get_num_localities().get();
        localities = hpx::find_all_localities();
        {
            std::unique_lock<mutex_type> lock(running_ranks_mutex_);
            rank_running_.resize(num_localities, true);
        }

        if (get_running_ranks() < 2 || env_resilience_disabled) {
            thread_safe_printer("Resilience disabled for single locality or env. variable ALLSCALE_RESILIENCE=0 set\n");
            return;
        }

        my_state = TRUST;
        my_heartbeat = 0;
        protectee_heartbeat = 0;
        keep_running = true;
        recovery_done = false;
        start_time = std::chrono::high_resolution_clock::now();
        hpx::get_num_localities().get();

        thread_safe_printer("before logical ring\n");
        std::size_t right_id = (rank_ + 1) % num_localities;
        std::size_t left_id = (rank_ == 0)?(num_localities-1):(rank_-1);
        std::size_t left_left_id = (left_id == 0)?(num_localities-1):(left_id-1);
        thread_safe_printer("left:"+std::to_string(left_id)+"right:"+std::to_string(right_id)+"left left:"+std::to_string(left_left_id));
        guard_ = hpx::find_from_basename("allscale/resilience", right_id).get();
        thread_safe_printer("got guard_");
        guard_rank_ = right_id;
        protectee_ = hpx::find_from_basename("allscale/resilience", left_id).get();
        protectee_.make_unmanaged();
        protectee_rank_ = left_id;
        thread_safe_printer("PROTECTEE RANK:"+std::to_string(protectee_rank_));
        protectees_protectee_ = hpx::find_from_basename("allscale/resilience", left_left_id).get();
        protectees_protectee_.make_unmanaged();
        protectees_protectee_rank_ = left_left_id;
        thread_safe_printer("Resilience component started. Protecting "+std::to_string(protectee_rank_)+"\n");

        auto & service = hpx::get_thread_pool("io_pool")->get_io_service();

        failure_detection_loop_async();
    }

    void resilience::reschedule_dispatched_to_dead(size_t dead_rank, size_t token) {
        thread_safe_printer("Calling reschedule_dispatched_to_dead...\n");
        std::multimap<size_t, work_item > delegated_items_copy;
            thread_safe_printer("before first lock\n");
            {
                std::unique_lock<mutex_type> lock(delegated_items_mutex_);
                std::swap(delegated_items_, delegated_items_copy);
            }
        thread_safe_printer("after first lock\n");
        {
            std::unique_lock<mutex_type> lock(running_ranks_mutex_);
            thread_safe_printer("rank["+std::to_string(dead_rank)+"]=false");
            rank_running_[dead_rank] = false;
            auto protectee_locality = get_locality_from_id(protectee_);
            auto it = std::find(localities.begin(), localities.end(), protectee_locality);
            localities.erase(it);
            num_localities--;
        }
        auto range = delegated_items_copy.equal_range(dead_rank);
        for (auto it = range.first; it != range.second; it++) {
            auto w = it->second;
            auto id = w.id();
            thread_safe_printer("Reschedule delegated item:"+id.name());
            allscale::scheduler::schedule(std::move(w), id);
        }
        delegated_items_copy.erase(dead_rank);
        {
            std::unique_lock<mutex_type> lock(delegated_items_mutex_);
            std::swap(delegated_items_, delegated_items_copy);
        }

        token++;
        if (get_running_ranks() > token) {
            thread_safe_printer("Will call dispatched at rank"+std::to_string(dead_rank));
            hpx::apply<reschedule_dispatched_to_dead_action>(guard_, dead_rank, token);
        }
        thread_safe_printer("End reshedule_dispatched_to_dead\n");
    }

    void resilience::work_item_dispatched(work_item const& w, size_t schedule_rank) {
        auto p = std::make_pair(schedule_rank,w);
        {
            std::unique_lock<mutex_type> lock(delegated_items_mutex_);
            delegated_items_.insert(p);
        }
        std::cout << "WOO -> schedule_rank = " << schedule_rank << "!\n";
        auto treeture = w.get_treeture();
        
        // get signal from treeture when finished
        // and continuation 
        treeture.get_future().then(
                        hpx::util::bind(
                            hpx::util::one_shot(
                            [schedule_rank, w, this]()
                            {
                                std::unique_lock<mutex_type> lock(this->delegated_items_mutex_);
                                thread_safe_printer("Holding a lock Before erase item"+w.id().name());
// this deletes all tasks of delegated locality -- potentially wrong
                                auto it = this->delegated_items_.find(schedule_rank);
                                if (it != delegated_items_.end()) this->delegated_items_.erase(it);
                                lock.unlock();
                                thread_safe_printer("Releasing a lock Before erase item"+w.id().name());
                            })));
    }

    std::size_t resilience::get_running_ranks() {
        int ranks;
        std::unique_lock<mutex_type> lock(running_ranks_mutex_);
        ranks = rank_running_.count();
        //thread_safe_printer("# Ranks: " + std::to_string(ranks));
        return ranks;
    }

    void resilience::protectee_crashed() {

        thread_safe_printer("After rescheduling\n");
        //remote_backups_.clear();
        //if (get_running_ranks() > 1)
            //delegated_items_.clear();
            //remote_backups_ = hpx::async<get_local_backups_action>(protectee_).get();
        //lock.unlock();

        size_t dead_protectee_rank = protectee_rank_;
        thread_safe_printer("Done rescheduling ...\n");
        // restore guard / protectee connections
        hpx::util::high_resolution_timer t;
        protectee_ = protectees_protectee_;
        protectee_rank_ = protectees_protectee_rank_;
        hpx::async<set_guard_action>(protectee_, this->get_id(), rank_).get();
        std::pair<hpx::id_type,uint64_t> p = hpx::async<get_protectee_action>(protectee_).get();
        protectees_protectee_ = p.first;
        protectees_protectee_.make_unmanaged();
        protectees_protectee_rank_ = p.second;
        thread_safe_printer("Finish recovery\n");

        // start locally to propagate fixing scheduler and rescheduling dispatched tasks
        // all through the ring ...
        hpx::apply(&resilience::reschedule_dispatched_to_dead, this, dead_protectee_rank, 0);
        //hpx::async<reschedule_dispatched_to_dead_action>(hpx::find_here(), dead_protectee_rank, 0).get();
        //hpx::apply<reschedule_dispatched_to_dead_action>(guard_, dead_protectee_rank, 1);
    

    }

    int resilience::get_cp_granularity() {
        return 3;
    }

    void resilience::shutdown(std::size_t token) {
        if (!(get_running_ranks() < 2 || env_resilience_disabled)) {
            keep_running = false;
            // dpn't forget to end the circle
            if (get_running_ranks() > token) {
                hpx::async<shutdown_action>(guard_, ++token).get();
            }
        }
    }

} // end namespace components
} // end namespace allscale

HPX_REGISTER_ACTION(allscale::components::resilience::send_heartbeat_action, send_heartbeat_action);
HPX_REGISTER_ACTION(allscale::components::resilience::set_guard_action, set_guard_action);
HPX_REGISTER_ACTION(allscale::components::resilience::get_protectee_action, get_protectee_action);
HPX_REGISTER_ACTION(allscale::components::resilience::shutdown_action, allscale_resilience_shutdown_action);
HPX_REGISTER_ACTION(allscale::components::resilience::reschedule_dispatched_to_dead_action, reschedule_dispatched_to_dead_action);
