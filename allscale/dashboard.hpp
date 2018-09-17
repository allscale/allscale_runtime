
#ifndef ALLSCALE_DASHBOARD_HPP
#define ALLSCALE_DASHBOARD_HPP

#include <cstdint>
#include <vector>
#include <string>

namespace allscale { namespace dashboard
{
    struct node_state
    {
        // the node this state is describing
        std::uint64_t rank;

        // the time this state was recorded
        std::uint64_t time;

        // the state of this node
        bool online = false;

        // is this node active according to the current scheduler
        bool active = false;

        // -- CPU load --

        // the number of cores involved on this node
        int num_cores = 1;

        // the current CPU load (snapshot)
        float cpu_load = 0.0f;

        // -- CPU core frequency --

        float max_frequency = 1.0;

        float cur_frequency = 1.0;

        // -- memory state --

        // the current memory usage
        std::uint64_t memory_load = 0;

        // the total available physical memory
        std::uint64_t total_memory = 0;

        // -- worker state --

        // number of tasks processed per second
        float task_throughput = 0.0f;

        // number of weighted tasks processed per second
        float weighted_task_throughput = 0.0f;

        // percentage of time being idle
        float idle_rate = 0.0f;

        // -- network I/O --

        // received network throughput
        std::uint64_t network_in = 0;

        // transmitted network throughput
        std::uint64_t network_out = 0;

        // -- Data distribution --

        // the data owned by this node
        std::string ownership;

        // -- multi-objective metrics --

        // the number of productive cycles spend per second
        std::uint64_t productive_cycles_per_second = 0;

        // the maximum power consumption per second
        float cur_power = 0;

        // the maximum energy consumptoin per second
        float max_power = 0;

        // cycles of progress / max available cycles \in [0..1]
        float speed = 0;

        // cycles of progress / current available cycles \in [0..1]
        float efficiency = 0;

        // current power usage / max power usage \in [0..1]
        float power = 0;

        std::string to_json() const;

        template <typename Archive>
        void serialize(Archive& ar, unsigned);
    };

    struct system_state
    {
        // the time this state was recorded
        std::uint64_t time;

        // -- multi-objective metrics --

        // cycles of progress / max available cycles on all nodes \in [0..1]
        float speed = 0;

        // cycles of progress / current available cycles on active nodes \in [0..1]
        float efficiency = 0;

        // current power usage / max power usage on all nodes \in [0..1]
        float power = 0;

        // the overall system-wide score of the objective function
        float score = 0;

        // -- the node states --

        // the state of the nodes
        std::vector<node_state> nodes;

        std::string to_json() const;
    };

    void update();
    void shutdown();
}}

#endif
