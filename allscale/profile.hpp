#ifndef ALLSCALE_FRAGMENT_HPP
#define ALLSCALE_FRAGMENT_HPP


#include <chrono>

namespace allscale
{
    struct profile 
    {
        std::chrono::steady_clock::time_point start;
        std::chrono::steady_clock::time_point end;
        std::chrono::steady_clock::time_point result_ready;
#if HAVE_PAPI
        long long papi_counters_start[8];
        long long papi_counters_stop[8];
#endif

        profile():start(std::chrono::steady_clock::now())
        {
        }


        double get_exclusive_time() {

           std::chrono::duration<double> time_elapsed = std::chrono::duration_cast<std::chrono::duration<double>>(end - start);
           return time_elapsed.count();

        }


        double get_inclusive_time() {

           std::chrono::duration<double> time_elapsed = std::chrono::duration_cast<std::chrono::duration<double>>(result_ready - start);
           return time_elapsed.count();

        }

    };



}

#endif
