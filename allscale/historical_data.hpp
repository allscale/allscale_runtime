#ifndef ALLSCALE_HISTORICAL_DATA_HPP
#define ALLSCALE_HISTORICAL_DATA_HPP


#include <chrono>

namespace allscale
{
    struct historical_data 
    {
        std::chrono::steady_clock::time_point last_iteration_start;
        int current_iteration;
        std::vector<double> iteration_time;       // Time for each iteration
	std::vector<std::string> iteration_roots; // Tree root per iteration

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

           iteration_time.push_back(time_elapsed.count());
	   iteration_roots.push_back(node_label);
	   last_iteration_start = t;
           current_iteration++;
        }

        

    };



}

#endif
