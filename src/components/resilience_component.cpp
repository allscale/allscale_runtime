#include <allscale/components/resilience.hpp>
#include <allscale/resilience.hpp>
#include <allscale/monitor.hpp>

namespace allscale { namespace components {

    resilience::resilience(std::uint64_t rank) : rank_(rank) {
        num_localities = hpx::get_num_localities().get();
    }

    void resilience::init() {
        allscale::monitor::connect(allscale::monitor::work_item_execution_started, resilience::global_w_exec_start_wrapper);
        allscale::monitor::connect(allscale::monitor::work_item_execution_finished, resilience::global_w_exec_finish_wrapper);
        hpx::get_num_localities().get();
        std::uint64_t right_id = (rank_ + 1) % num_localities;
        guard = hpx::find_from_basename("allscale/resilience", right_id).get();
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
        // check granularity
        if (w.id().depth() != get_cp_granularity()) return;

        //@ToDo: do I really need to block (via get) here?
        std::cout << "Will checkpoint task " << w.id().depth() << "\n";
        // remote backup
        hpx::async<remote_backup_action>(guard, w).get();
        // local backup
        local_backups_[w.id()] = w;
    }

    void resilience::w_exec_finish_wrapper(work_item const& w) {
        if (w.id().depth() != get_cp_granularity()) return;

        std::cout << "Will uncheckpoint task " << w.id().depth() << "\n";

        //@ToDo: do I really need to block (via get) here?
        hpx::async<remote_unbackup_action>(guard,w).get();
        local_backups_.erase(local_backups_.find(w.id()),local_backups_.end());

        // remove local and remote backups

    }

	int resilience::get_cp_granularity() {
		return 5;
	}

    void resilience::remote_backup(work_item w) {
        remote_backups_[w.id()] = w;
    }

    void resilience::remote_unbackup(work_item w) {
        remote_backups_.erase(remote_backups_.find(w.id()),remote_backups_.end());
    }


} // end namespace components
} // end namespace allscale

HPX_REGISTER_ACTION(allscale::components::resilience::remote_backup_action, remote_backup_action);
HPX_REGISTER_ACTION(allscale::components::resilience::remote_unbackup_action, remote_unbackup_action);
