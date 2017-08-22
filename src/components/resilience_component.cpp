#include <allscale/components/resilience.hpp>
#include <allscale/resilience.hpp>
#include <allscale/monitor.hpp>

namespace allscale { namespace components {

    resilience::resilience(std::uint64_t rank) : rank_(rank) {
    }

    void resilience::init() {
        allscale::monitor::connect(allscale::monitor::work_item_execution_started, resilience::global_w_exec_start_wrapper);
    }
    void resilience::global_w_exec_start_wrapper(work_item const& w)
    {
        (allscale::resilience::get_ptr().get())->w_exec_start_wrapper(w);
    }
    void resilience::w_exec_start_wrapper(work_item const& w) {
        std::cout << "Starting component\n";
    }


} // end namespace components
} // end namespace allscale
