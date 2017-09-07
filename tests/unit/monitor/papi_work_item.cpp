/*
 * A simple example to test PAPI counters
 */
#include <allscale/no_split.hpp>
#include <allscale/no_serialization.hpp>
#include <allscale/treeture.hpp>
#include <allscale/spawn.hpp>
#include <allscale/work_item.hpp>
#include <allscale/scheduler.hpp>
#include <allscale/components/monitor.hpp>
#include <allscale/monitor.hpp>
#include <allscale/work_item_description.hpp>

#include <hpx/hpx_init.hpp>

#include <stdlib.h>
#include <iostream>

typedef std::int64_t int_type;
ALLSCALE_REGISTER_TREETURE_TYPE(int_type);

struct simple_variant;

struct simple_name
{
    static const char *name()
    {
        return "simple";
    }
};

using simple_work =
    allscale::work_item_description<
        std::int64_t,
        simple_name,
        allscale::no_serialization,
        allscale::no_split<std::int64_t>,
        simple_variant
    >;


struct simple_variant
{
    static constexpr bool valid = true;
    using result_type = std::int64_t;


    // Execute serially
    template <typename Closure>
    static allscale::treeture<std::int64_t> execute(Closure const& closure)
    {
        return
            allscale::treeture<std::int64_t>{hpx::util::get<0>(closure)};
    }
};


std::int64_t simple_runner(std::int64_t k)
{
   allscale::treeture<std::int64_t> number = allscale::spawn_first<simple_work>(k);
   std::int64_t res = number.get_result();

   return res;
}



int hpx_main( int argc, char *argv[] )
{
       long long *papi_counters;
       std::shared_ptr<allscale::components::monitor> allscale_monitor;

       setenv("MONITOR_PAPI", "PAPI_TOT_INS,PAPI_TOT_CYC", 0);

#ifdef HAVE_PAPI
       // start allscale monitoring
       allscale::monitor::run(hpx::get_locality_id());

       // start allscale scheduler ...
       allscale::scheduler::run(hpx::get_locality_id());

       simple_runner(7);

       allscale_monitor = allscale::monitor::get_ptr();
       papi_counters = allscale_monitor->get_papi_counters("0.0");
       if(papi_counters[0] <= 0 || papi_counters[1] <= 0)
          std::cout << "FAILED: wrong PAPI counter values" << std::endl;
       else
          std::cout << "PASSED: " << papi_counters[0] << " instructions and " << papi_counters[1] <<
                " for work item 0.0" << std::endl;

       free(papi_counters);
       allscale::scheduler::stop();
       allscale::monitor::stop();
#else
       std::cout << "FAILED: runtime compiled without PAPI support" << std::endl;
#endif
       return hpx::finalize();
}


int main(int argc, char **argv)
{
    allscale::scheduler::setup_resources(argc, argv);
    return hpx::init();
}
