
#include <allscale/no_split.hpp>
#include <allscale/no_serialization.hpp>
#include <allscale/do_serialization.hpp>
#include <allscale/treeture.hpp>
#include <allscale/spawn.hpp>
#include <allscale/scheduler.hpp>
#include <allscale/work_item_description.hpp>

#include <hpx/include/unordered_map.hpp>
#include <hpx/include/performance_counters.hpp>

#include <unistd.h>


typedef std::int64_t int_type;
ALLSCALE_REGISTER_TREETURE_TYPE(int_type);

// fibonacci serial reference
std::int64_t fib_ser(std::int64_t n)
{
    if(n <= 2) return 1;
    return fib_ser(n-1) + fib_ser(n-2);
}

////////////////////////////////////////////////////////////////////////////////
// Fibonacci Work Item:
//  - Result: std::int64_t
//  - Closure: [std::int64_t]
//  - SplitVariant: spawns fib(n-1) and fib(n-2) recursively as tasks
//  - ProcessVariant: computes fib(n) recursively

struct add_name
{
    static const char *name()
    {
        return "add";
    }
};

struct add_variant;
using add_work =
    allscale::work_item_description<
        std::int64_t,
        add_name,
        allscale::do_serialization,
        allscale::no_split<std::int64_t>,
        add_variant
    >;

struct add_variant
{
    static constexpr bool valid = true;
    using result_type = std::int64_t;

    // The split variant requires nothing ...
//     template <typename Closure>
//     static std::tuple<> requires(Closure const& closure)
//     {
//         return std::tuple<>{};
//     }

    // Just perform the addition, no extra tasks are spawned
    template <typename Closure>
    static allscale::treeture<std::int64_t> execute(Closure const& closure)
    {
//         std::cout << allscale::this_work_item::get_id().name() << " add ... \n";
        return
            allscale::treeture<std::int64_t>{
                hpx::util::get<0>(closure) + hpx::util::get<1>(closure)
            };
    }
};

typedef std::tuple<std::int64_t> closure_type;

struct split_variant;
struct process_variant;

struct fib_name
{
    static const char *name()
    {
        return "fib";
    }
};

using fibonacci_work =
    allscale::work_item_description<
        std::int64_t,
        fib_name,
        allscale::no_serialization,
        split_variant,
        process_variant
    >;

struct split_variant
{
    static constexpr bool valid = true;
    using result_type = std::int64_t;

    // The split variant requires nothing ...
//     template <typename Closure>
//     static std::tuple<> requires(Closure const& closure)
//     {
//         return std::tuple<>{};
//     }

    // It spawns two new tasks, fib(n-1) and fib(n-2)
    template <typename Closure>
    static allscale::treeture<std::int64_t> execute(Closure const& closure)
    {
        if(hpx::util::get<0>(closure) <= 2)
            return allscale::treeture<std::int64_t>{1};

        return allscale::spawn<add_work>(
            allscale::spawn<fibonacci_work>(
                hpx::util::get<0>(closure) - 1
            ),
            allscale::spawn<fibonacci_work>(
                hpx::util::get<0>(closure) - 2
            )
        );
    }
};

struct process_variant
{
    static constexpr bool valid = true;
    using result_type = std::int64_t;

    static const char* name()
    {
        return "fib_process_variant";
    }

    // The split variant requires nothing ...
//     template <typename Closure>
//     static std::tuple<> requires(Closure const& closure)
//     {
//         return std::tuple<>{};
//     }

    // Execute fibonacci serially
    template <typename Closure>
    static allscale::treeture<std::int64_t> execute(Closure const& closure)
    {
        return
            allscale::treeture<std::int64_t>{
                fib_ser(hpx::util::get<0>(closure))
            };
    }
};

#include <hpx/hpx_init.hpp>
#include <hpx/util/high_resolution_timer.hpp>
#include <iostream>
#include <thread>

#include <boost/atomic.hpp>

#include <allscale/components/monitor.hpp>


std::int64_t fib_elapsed = 0;


std::int64_t fib_performance_data(bool reset)
{
    return hpx::util::get_and_reset_value(fib_elapsed, reset);
}


void register_counter_type()
{

    std::cout << "Registering allscale counter" << std::endl;

    // Call the HPX API function to register the counter type.
    hpx::performance_counters::install_counter_type(
        "/allscale/examples/fib_time",
        // counter type name
        &fib_performance_data,
        // function providing counter data
        "returns time spent on calculation of the fib"
        // description text
    );
}



bool get_startup(hpx::startup_function_type& startup_func, bool& pre_startup)
{
    // return our startup-function if performance counters are required
    startup_func = register_counter_type;   // function to run during startup
    pre_startup = true;       // run 'startup' as pre-startup function
    return true;
}




std::int64_t fib_runner(std::int64_t n)
{
   allscale::treeture<std::int64_t> fib = allscale::spawn<true, fibonacci_work>(n);
   std::int64_t res = fib.get_result();

   return res;
}


int hpx_main(int argc, char **argv)
{

    allscale::components::monitor_component_init();

    // start allscale scheduler ...
    allscale::scheduler::run(hpx::get_locality_id());

    if(hpx::get_locality_id() == 0)
    {
        std::int64_t n = argc >= 2 ? std::stoi(std::string(argv[1])) : 10;
        std::int64_t iters = argc >= 3 ? std::stoi(std::string(argv[2])) : 1;

        for(int i=0; i<iters; i++) {
	   std::cout << "Starting fib(" << n << "), " << "Iter: " << i << "\n";
           hpx::util::high_resolution_timer t;
           std::int64_t res = fib_runner(n);
           fib_elapsed = t.elapsed_microseconds();
           std::cout << "fib(" << n << ") = " << res << " taking " << fib_elapsed << " microseconds. Iter: " << i << "\n";
           usleep(2000000);
        }
        allscale::scheduler::stop();
    }

    return hpx::finalize();
}

int main(int argc, char **argv)
{
    hpx::register_pre_startup_function(&register_counter_type);

    return hpx::init(argc, argv);
}

