#include <allscale/components/resilience.hpp>
#include <allscale/resilience.hpp>
#include <allscale/monitor.hpp>
#include <allscale/scheduler.hpp>
#include <allscale/work_item.hpp>

#include <chrono>

using std::chrono::milliseconds;

namespace allscale { namespace components {

    resilience::resilience(std::uint64_t rank) : rank_(rank) {
    }

    void resilience::set_guard(hpx::id_type guard) {
        guard_ = guard;
    }

    void resilience::send_heartbeat(std::size_t counter) {
        heartbeat_counter = counter;
    }

    void resilience::failure_detection_loop_async() {
        if (resilience_disabled)
            return;
        HPX_ASSERT(this->get_id());
        typedef resilience::failure_detection_loop_action action_type;
        hpx::apply<action_type>(this->get_id());
    }

    // Run detection forever ...
    void resilience::failure_detection_loop () {
        std::size_t actual_epoch = 0;
        while (resilience_component_running) {
//            hpx::this_thread::sleep_for(milliseconds(miu-delta));
//            auto t_now =  std::chrono::high_resolution_clock::now();
//            actual_epoch = std::chrono::duration_cast<std::chrono::milliseconds>(t_now-start_time).count()/1000;
//            // asynchronously send heartbeat m_i
//            // at sigma_i = i * miu
//            hpx::id_type protectee = get_protectee();
//            hpx::apply<send_heartbeat_action>(protectee, actual_epoch);
//            // At t_i - sigma_i + delta, check if I
//            // have received a message m_j with j>=i from a peer
//
            hpx::this_thread::sleep_for(milliseconds(delta));
//            if (heartbeat_counter < actual_epoch) {
//                auto end_time = std::chrono::high_resolution_clock::now();
//                if (end_time >= trust_lease)
//                    my_state = SUSPECT;
//            }
//            else { // j >= i
//                // a message has been received for this epoch
//                // so TRUST until t_(i+1) = (i+1) * miu + delta
//                trust_lease = start_time + milliseconds((heartbeat_counter + 1) * miu + delta);
//                my_state = TRUST;
//            }
//            if (my_state == TRUST)
//                std::cout << "TRUST\n";
//            else
//                std::cout << "SUSPECT\n";
        }
        loop_done = true;
    }


    hpx::id_type resilience::get_protectee() {
        return protectee_;
    }
    std::map<this_work_item::id,work_item> resilience::get_local_backups() {
        return local_backups_;
    }

    void resilience::init() {
        start_time = std::chrono::high_resolution_clock::now();
        num_localities = hpx::get_num_localities().get();
        resilience_component_running = true;
        loop_done = false;

        if (num_localities < 2) {
            resilience_disabled = true;
            std::cout << "Resilience disabled for single locality!\n";
            return;
        }
        else
            resilience_disabled = false;

        allscale::monitor::connect(allscale::monitor::work_item_execution_started, resilience::global_w_exec_start_wrapper);
        allscale::monitor::connect(allscale::monitor::work_item_execution_finished, resilience::global_w_exec_finish_wrapper);
        hpx::get_num_localities().get();

        std::uint64_t right_id = (rank_ + 1) % num_localities;
        std::uint64_t left_id = (rank_ == 0)?(num_localities-1):(rank_-1);
        std::uint64_t left_left_id = (left_id == 0)?(num_localities-1):(left_id-1);
        guard_ = hpx::find_from_basename("allscale/resilience", right_id).get();
        protectee_ = hpx::find_from_basename("allscale/resilience", left_id).get();
        protectees_protectee_ = hpx::find_from_basename("allscale/resilience", left_left_id).get();
    }

    void resilience::global_w_exec_start_wrapper(work_item const& w)
    {
        (allscale::resilience::get_ptr().get())->w_exec_start_wrapper(w);
    }

    void resilience::global_w_exec_finish_wrapper(work_item const& w)
    {
        (allscale::resilience::get_ptr().get())->w_exec_finish_wrapper(w);
    }

    // equiv. to taskAcquired in prototype
    void resilience::w_exec_start_wrapper(work_item const& w) {
        if (resilience_disabled) return;

        if (w.id().depth() != get_cp_granularity()) return;

        //@ToDo: do I really need to block (via get) here?
        hpx::async<remote_backup_action>(guard_, w).get();
        local_backups_[w.id()] = w;
    }

    void resilience::w_exec_finish_wrapper(work_item const& w) {
        if (resilience_disabled) return;

        if (w.id().depth() != get_cp_granularity()) return;

        //@ToDo: do I really need to block (via get) here?
        hpx::async<remote_unbackup_action>(guard_, w).get();
        local_backups_.erase(w.id());

    }

    void resilience::protectee_crashed() {
        for (auto c : remote_backups_) {
            work_item restored = c.second;
            allscale::scheduler::schedule(std::move(restored));
        }

        // restore guard / protectee connections
        protectee_ = protectees_protectee_;
        hpx::async<set_guard_action>(protectee_, hpx::find_here()).get();
        protectees_protectee_ = hpx::async<get_protectee_action>(protectee_).get();
        remote_backups_.clear();
        remote_backups_ = hpx::async<get_local_backups_action>(protectee_).get();
    }

	int resilience::get_cp_granularity() {
		return 2;
	}

    void resilience::remote_backup(work_item w) {
        std::cout << "will write remote checkpoint ...\n";
        std::unique_lock<std::mutex> lock(backup_mutex_);
        remote_backups_[w.id()] = w;
    }

    void resilience::remote_unbackup(work_item w) {

        std::unique_lock<std::mutex> lock(backup_mutex_);
        auto b = remote_backups_.find(w.id());
        if (b == remote_backups_.end())
            std::cerr << "ERROR: Backup not found that should be there!\n";
        remote_backups_.erase(b);
    }

    void resilience::shutdown() {
        if (resilience_component_running) {
            resilience_component_running = false;
            while (!loop_done) {
                hpx::this_thread::sleep_for(milliseconds(500));
            }
            hpx::apply<shutdown_action>(guard_);
        }
    }

} // end namespace components
} // end namespace allscale

HPX_REGISTER_ACTION(allscale::components::resilience::remote_backup_action, remote_backup_action);
HPX_REGISTER_ACTION(allscale::components::resilience::remote_unbackup_action, remote_unbackup_action);
HPX_REGISTER_ACTION(allscale::components::resilience::set_guard_action, set_guard_action);
HPX_REGISTER_ACTION(allscale::components::resilience::get_protectee_action, get_protectee_action);
HPX_REGISTER_ACTION(allscale::components::resilience::get_local_backups_action, get_local_backups_action);
HPX_REGISTER_ACTION(allscale::components::resilience::send_heartbeat_action, send_heartbeat_action);
HPX_REGISTER_ACTION(allscale::components::resilience::failure_detection_loop_action, failure_detection_loop_action);
HPX_REGISTER_ACTION(allscale::components::resilience::shutdown_action, shutdown_action);
