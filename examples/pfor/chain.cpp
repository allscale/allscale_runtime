
#include <allscale/no_split.hpp>
#include <allscale/no_serialization.hpp>
#include <allscale/do_serialization.hpp>
#include <allscale/treeture.hpp>
#include <allscale/spawn.hpp>
#include <allscale/scheduler.hpp>
#include <allscale/work_item_description.hpp>

#include <allscale/runtime.hpp>

#include <hpx/include/unordered_map.hpp>
#include <hpx/include/performance_counters.hpp>

#include <unistd.h>

#include "pfor.hpp"


static const int MAX_SIZE = 128 * 1024 * 1024;

static int dataA[MAX_SIZE];
static int dataB[MAX_SIZE];


struct simple_stencil_body {
    void operator()(std::int64_t i, const hpx::util::tuple<std::int64_t,std::int64_t>& params) const {

        // limit maximum range
        if (i >= MAX_SIZE) return;

        // extract parameters
        auto t = hpx::util::get<0>(params);
        auto n = hpx::util::get<1>(params);

        // figure out in which way to move the data
        int* A = (t%2) ? dataA : dataB;
        int* B = (t%2) ? dataB : dataA;

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

    // initialize the data array
    for(int i=0; i<MAX_SIZE; i++) {
        dataA[i] = 0;
    }

    // start allscale scheduler ...
    allscale::scheduler::run(hpx::get_locality_id());

    if(hpx::get_locality_id() == 0)
    {
        std::int64_t n = argc >= 2 ? std::stoi(std::string(argv[1])) : 1000000;
        std::int64_t steps = argc >= 3 ? std::stoi(std::string(argv[2])) : 1000;
        std::int64_t iters = argc >= 4 ? std::stoi(std::string(argv[2])) : 1;

        for(int i=0; i<iters; i++) {
            std::cout << "Starting " << steps << "x pfor(0.." << n << "), " << "Iter: " << i << "\n";
            hpx::util::high_resolution_timer t;
            for(int t=0; t<steps; t++) {
                pfor<simple_stencil_body>(0,n,t,n);
            }
            auto elapsed = t.elapsed_microseconds();
            std::cout << "pfor(0.." << n << ") taking " << elapsed << " microseconds. Iter: " << i << "\n";
        }
        allscale::scheduler::stop();
    }

    return hpx::finalize();
}

int main(int argc, char **argv)
{
    return hpx::init(argc, argv);
}

