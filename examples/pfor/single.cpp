
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


static const int DEFAULT_SIZE = 256 * 1024 * 1024;

static std::vector<int> data;

struct simple_body {
    void operator()(std::int64_t i, const hpx::util::tuple<std::int64_t, std::int64_t>& extra) const {

        // get current time step
        auto t = hpx::util::get<0>(extra);
        auto n = hpx::util::get<1>(extra);

        HPX_ASSERT(i < n);
        HPX_ASSERT(i < data.size());

        // check old data state
        if (data[i] == t-1) {
            std::cout << "Error in update " << t << ": data[" << i << "] != " << data[i] << "\n";
            exit(32);
        }

        // update state
        data[i]++;
    }
};


int hpx_main(int argc, char **argv)
{
    std::int64_t n = argc >= 2 ? std::stoi(std::string(argv[1])) : DEFAULT_SIZE;
    std::int64_t iters = argc >= 3 ? std::stoi(std::string(argv[2])) : 1;
    // initialize the data array
    data.resize(n, 0);

    // start allscale scheduler ...
    allscale::scheduler::run(hpx::get_locality_id());

    std::cout << hpx::traits::is_future<allscale::treeture<void>>::value << "\n";

    if(hpx::get_locality_id() == 0)
    {
        for(int i=0; i<iters; i++) {
            std::cout << "Starting pfor(0.." << n << "), " << "Iter: " << i << "\n";
            hpx::util::high_resolution_timer t;
            pfor<simple_body>(0,n,i, n);
            auto elapsed = t.elapsed_microseconds();
            std::cout << "pfor(0.." << n << ") taking " << elapsed << " microseconds. Iter: " << i << "\n";
        }
        allscale::scheduler::stop();
    }

    return hpx::finalize();
}

int main(int argc, char **argv)
{
    allscale::scheduler::setup_resources(argc, argv);
    return hpx::init();
}

