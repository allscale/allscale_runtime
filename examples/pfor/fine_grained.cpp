#include <allscale/no_split.hpp>
#include <allscale/no_serialization.hpp>
#include <allscale/do_serialization.hpp>
#include <allscale/treeture.hpp>
#include <allscale/spawn.hpp>
#include <allscale/scheduler.hpp>
#include <allscale/components/monitor.hpp>
#include <allscale/work_item_description.hpp>

//#include <allscale/runtime.hpp>

#include <hpx/include/unordered_map.hpp>
#include <hpx/include/performance_counters.hpp>
#include <hpx/hpx_init.hpp>

#include <atomic>
#include <unistd.h>

#include "pfor.hpp"


static const int DEFAULT_SIZE = 128 * 1024 * 1024;

static std::unique_ptr<int[]> dataA;
static std::unique_ptr<int[]> dataB;

std::atomic<std::int64_t> app_elapsed(0);

std::int64_t app_performance_data(bool reset)
{
    return hpx::util::get_and_reset_value(app_elapsed, reset);
}

void register_counter_type()
{

    std::cout << "Registering allscale counter" << std::endl;

    // Call the HPX API function to register the counter type.
    hpx::performance_counters::install_counter_type(
        "/allscale/examples/app_time",
        // counter type name
        &app_performance_data,
        // function providing counter data
        "returns time spent on calculation of the one iter of the application"
        // description text
    );
}

struct init_body {
    void operator()(std::int64_t i, const hpx::util::tuple<std::int64_t>& params) const {
        // extract parameters
        auto n = hpx::util::get<0>(params);
        HPX_ASSERT(i < n);

        // figure out in which way to move the data
        int* A = dataA.get();
        int* B = dataB.get();

        A[i] = 0;
        B[i] = 0;
    }
};

struct simple_stencil_body {
    void operator()(std::int64_t i, const hpx::util::tuple<std::int64_t,std::int64_t>& params) const {
        // extract parameters
        auto t = hpx::util::get<0>(params);
        auto n = hpx::util::get<1>(params);

        HPX_ASSERT(i < n);

        // figure out in which way to move the data
        int* A = (t%2) ? dataA.get() : dataB.get();
        int* B = (t%2) ? dataB.get() : dataA.get();

        // check current state
        if ((i > 0 && A[i-1] != A[i]) || (i < n-1 && A[i] != A[i+1])) {
                std::cout << "Error in synchronization!\n";
                std::cout << "  for i=" << i << "\n";
                std::cout << "  A[i-1]=" << A[i-1] << "\n";
                std::cout << "  A[ i ]=" << A[ i ] << "\n";
                std::cout << "  A[i+1]=" << A[i+1] << "\n";
                exit(42);
        }
        // update B
        B[i] = A[i] + 1;
    }
};


int hpx_main(int argc, char **argv)
{
    // include monitoring support
//     allscale::components::monitor_component_init();
    // start allscale scheduler ...
    allscale::scheduler::run(hpx::get_locality_id());
    allscale::monitor::run(hpx::get_locality_id());

    std::int64_t n = argc >= 2 ? std::stoi(std::string(argv[1])) : DEFAULT_SIZE;
    std::int64_t steps = argc >= 3 ? std::stoi(std::string(argv[2])) : 1000;
    std::int64_t iters = argc >= 4 ? std::stoi(std::string(argv[3])) : 1;

    // initialize the data array
    dataA.reset(new int[n]);
    dataB.reset(new int[n]);

    double mean = 0.0;

    if(hpx::get_locality_id() == 0)
    {
        for(int i=0; i<iters; i++) {
            std::cout << "Starting " << steps << "x pfor(0.." << n << "), " << "Iter: " << i << "\n";
            hpx::util::high_resolution_timer t;

            {
                pfor_loop_handle last;
                for(int t=0; t<steps; t++) {
                    last = pfor_neighbor_sync<simple_stencil_body>(last,0,n,t,n);
                }
            }
            // auto elapsed = t.elapsed_microseconds();
	        app_elapsed = t.elapsed_microseconds();
            mean += app_elapsed/steps;
            std::cout << "pfor(0.." << n << ") taking " << app_elapsed/steps << " microseconds. Iter: " << i << "\n";
        }
        allscale::scheduler::stop();
        allscale::monitor::stop();

        std::cout << "Mean time: " << mean/iters << std::endl;
    }

    return hpx::finalize();
}


int main(int argc, char **argv)
{
//     std::cout << "Before register counter" << std::endl;
    hpx::register_pre_startup_function(&register_counter_type);
    allscale::scheduler::setup_resources(argc, argv);
    return hpx::init();
}

