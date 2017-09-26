#ifndef ALLSCALE_HISTORICAL_DATA_HPP
#define ALLSCALE_HISTORICAL_DATA_HPP


#include <chrono>
#include <hpx/include/lcos.hpp>

namespace allscale
{
    struct historical_data 
    {
        std::chrono::steady_clock::time_point last_iteration_start;
        int current_iteration;
        std::vector<double> iteration_time;       // Time for each iteration
	std::vector<std::string> iteration_roots; // Tree root per iteration

	typedef hpx::lcos::local::spinlock mutex_type;
        mutex_type history_mutex;

        historical_data()
        {
           current_iteration = -1;
//           last_iteration_start = std::chrono::steady_clock::now();
        }

        void add_tree_root(std::string node_label) { iteration_roots.push_back(node_label); }

        void new_iteration(std::string node_label)
        {
           std::chrono::steady_clock::time_point t = std::chrono::steady_clock::now();
        
           std::chrono::duration<double> time_elapsed = std::chrono::duration_cast<std::chrono::duration<double>>(t - last_iteration_start);

	   std::lock_guard<mutex_type> lock(history_mutex);
           iteration_time.push_back(time_elapsed.count());
	   iteration_roots.push_back(node_label);
	   last_iteration_start = t;
           current_iteration++;
        }

        

    };



}

#endif
