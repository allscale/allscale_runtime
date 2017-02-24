
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

#include "pfor.h"


static const int MAX_SIZE = 256 * 1024 * 1024;

static int data[MAX_SIZE];


struct simple_body {
    void operator()(std::int64_t i) const {
        if (i<MAX_SIZE) data[i]++;
    }
};


int hpx_main(int argc, char **argv)
{

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
                pfor<simple_body>(0,n);
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

