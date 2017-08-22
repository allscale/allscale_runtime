#include <allscale/components/resilience.hpp>
#include <allscale/resilience.hpp>
#include <allscale/monitor.hpp>

namespace allscale { namespace components {

    resilience::resilience(std::uint64_t rank) : rank_(rank) {
        num_localities = hpx::get_num_localities().get();
    }

    void resilience::init() {
        allscale::monitor::connect(allscale::monitor::work_item_execution_started, resilience::global_w_exec_start_wrapper);
        hpx::get_num_localities().get();
        std::uint64_t right_id = (rank_ + 1) % num_localities;
        guard = hpx::find_from_basename("allscale/resilience", right_id).get();
    }

    void resilience::global_w_exec_start_wrapper(work_item const& w)
    {
        (allscale::resilience::get_ptr().get())->w_exec_start_wrapper(w);
    }

    // equiv. to taskAcquired in prototype
    void resilience::w_exec_start_wrapper(work_item const& w) {
        std::cout << "Task acquired\n";
        // check granularity
        if (w.id().depth() != get_cp_granularity()) return;

        //@ToDo: check if I should block here?
        hpx::async<backup_action>(guard, w).get();
    }

	int resilience::get_cp_granularity() {
		return 5;
	}

    void resilience::backup(work_item wi) {
        backup_ = wi;
    }


} // end namespace components
} // end namespace allscale

HPX_REGISTER_ACTION(allscale::components::resilience::backup_action, backup_action);
