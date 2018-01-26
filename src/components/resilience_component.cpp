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
        hpx::cout << "DEBUG (" << rank_ << "):" << output << std::flush;
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
	    if (my_heartbeat > protectee_heartbeat + 5) {
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

    std::map<std::string,work_item> resilience::get_local_backups() {
        return local_backups_;
    }

    void resilience::init() {
        char *env = std::getenv("ALLSCALE_RESILIENCE");
        env_resilience_disabled = (env && env[0] == '0');

        num_localities = hpx::get_num_localities().get();
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

        allscale::monitor::connect(allscale::monitor::work_item_execution_started, allscale::resilience::global_w_exec_start_wrapper);
        allscale::monitor::connect(allscale::monitor::work_item_result_propagated, resilience::global_w_exec_finish_wrapper);
        hpx::get_num_localities().get();

        std::uint64_t right_id = (rank_ + 1) % num_localities;
        std::uint64_t left_id = (rank_ == 0)?(num_localities-1):(rank_-1);
        std::uint64_t left_left_id = (left_id == 0)?(num_localities-1):(left_id-1);
        guard_ = hpx::find_from_basename("allscale/resilience", right_id).get();
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

    void resilience::global_w_exec_finish_wrapper(work_item const& w)
    {
        allscale::resilience::get().w_exec_finish_wrapper(w);
    }

    // equiv. to taskAcquired in prototype
    void resilience::w_exec_start_wrapper(work_item const& w) {
        if (get_running_ranks() < 2 || env_resilience_disabled) 
            return;

        if (w.id().depth() != get_cp_granularity()) return;

        //@ToDo: do I really need to block (via get) here?
        if (get_running_ranks() > 1) {
            hpx::async<remote_backup_action>(guard_, w).get();
	    std::unique_lock<mutex_type> lock(backup_mutex_);
	    //thread_safe_printer("local_backups_["+w.id().name()+"]\n");
	    local_backups_[w.id().name()] = w;
        }


        thread_safe_printer("Done backing up : "+w.id().name()+"\n");
    }

    void resilience::w_exec_finish_wrapper(work_item const& w) {
        if (get_running_ranks() < 2 || env_resilience_disabled) 
            return;


        if (w.id().depth() != get_cp_granularity()) return;

        //@ToDo: do I really need to block (via get) here?
        if (get_running_ranks() > 1)  {
            hpx::async<remote_unbackup_action>(guard_, w.id().name()).get();
            std::unique_lock<mutex_type> lock(backup_mutex_);
	    thread_safe_printer("local_backups_["+w.id().name()+"].erase()\n");
	    local_backups_.erase(w.id().name());
        }

    }

    std::size_t resilience::get_running_ranks() {
        int ranks;
        std::unique_lock<mutex_type> lock(running_ranks_mutex_);
        ranks = rank_running_.count();
	//thread_safe_printer("# Ranks: " + std::to_string(ranks));
        return ranks;
    }

    void resilience::protectee_crashed() {

        thread_safe_printer("set bitrank of "+std::to_string(protectee_rank_)+" to false\n");
        //{
            //std::unique_lock<mutex_type> lock2(running_ranks_mutex_);
	    thread_safe_printer("in lock region of set bitrank\n");
            rank_running_[protectee_rank_] = false;
	    //lock2.unlock();
        //}
        thread_safe_printer("after set bitrank of "+std::to_string(protectee_rank_)+" to false\n");

        std::unique_lock<mutex_type> lock(remote_backup_mutex_);
        thread_safe_printer("after lock before rescheduling");

        for (auto c : remote_backups_) {
            work_item restored = c.second;
            thread_safe_printer("Will reschedule task "+restored.id().name()+"\n");
            allscale::scheduler::schedule(std::move(restored));
        }
	thread_safe_printer("After rescheduling\n");
        remote_backups_.clear();
	if (get_running_ranks() > 1)
		remote_backups_ = hpx::async<get_local_backups_action>(protectee_).get();
	lock.unlock();

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
    }

	int resilience::get_cp_granularity() {
		return 3;
	}

    void resilience::remote_backup(work_item w) {
        std::unique_lock<mutex_type> lock(remote_backup_mutex_);
        thread_safe_printer("remote_backups_["+w.id().name()+"]\n");
        remote_backups_[w.id().name()] = w;
    }

    void resilience::remote_unbackup(std::string name) {
        std::unique_lock<mutex_type> lock(remote_backup_mutex_);
        auto b = remote_backups_.find(name);
        if (b == remote_backups_.end())
        {
                std::cerr << "ERROR: Backup not found that should be there!\n";
                return;
        }
        // We safe the backed up work item so that dtor of the underlying
        // treeture isn't triggered when we erase from the map while the lock
        // is being held.
        else {
		thread_safe_printer("remote_backups_["+name+"].erase()\n");
                remote_backups_.erase(b);
        }
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
HPX_REGISTER_ACTION(allscale::components::resilience::remote_backup_action, remote_backup_action);
HPX_REGISTER_ACTION(allscale::components::resilience::remote_unbackup_action, remote_unbackup_action);
HPX_REGISTER_ACTION(allscale::components::resilience::set_guard_action, set_guard_action);
HPX_REGISTER_ACTION(allscale::components::resilience::get_protectee_action, get_protectee_action);
HPX_REGISTER_ACTION(allscale::components::resilience::get_local_backups_action, get_local_backups_action);
HPX_REGISTER_ACTION(allscale::components::resilience::shutdown_action, allscale_resilience_shutdown_action);
