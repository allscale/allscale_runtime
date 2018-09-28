#ifndef ALLSCALE_COMPONENTS_INTERNODEOPTIMIZER_HPP
#include <numeric>
#include <vector>
#include <set>
#include <map>
#include <tuple>
#include <cassert>
#include <random>
#include <iterator>
#include <algorithm>


#define INO_EXPLORATION_PHASE_STEPS 3u
#define INO_EXPLORATION_SOFT_LIMIT 10u
#define INO_RANDOM_EVERY 3u
#define INO_DEFAULT_FORGET_AFTER 10000u

#define INO_DEBUG_PARETO
#define INO_DEBUG_DECIDE_SCHEDULE

#if defined(INO_DEBUG_PARETO) || defined(INO_DEBUG_DECIDE_SCHEDULE)
#include <iostream>
#endif

#ifdef INO_RANDOM_EVERY
#if INO_RANDOM_EVERY < 1
#error INO_RANDOM_EVERY must be a positive number (if its equal to 2, every 3rd balance() will explore)
#endif
#endif

// #define CAN_SHUFFLE

namespace allscale
{
namespace components
{

struct ino_knobs_t
{
    ino_knobs_t(std::size_t nodes)
        : u_nodes(nodes)
    {
    }

    std::size_t u_nodes;
};

struct ino_knobs_cmp_t
{
    bool operator()(const ino_knobs_t &lhs, const ino_knobs_t &rhs)
    {
        return lhs.u_nodes < rhs.u_nodes;
    }
};

struct pareto_entry_t
{
    pareto_entry_t(std::size_t at_tick,
                   ino_knobs_t knobs,
                   std::vector<float> measure_load,
                   std::vector<float> measure_time,
                   std::vector<float> measure_energy)
        : u_at_tick(at_tick),
          c_knobs(knobs),
          v_load(measure_load),
          v_time(measure_time),
          v_energy(measure_energy)
    {
        // VV: Perhaps take into account the mean of the vectors
        // std::aggregate(<vector>.begin(), <vector>.end(), 0.0) / nodes
    }

    float average_load() const
    {
        return std::accumulate(v_load.begin(), v_load.end(), 0.0) / c_knobs.u_nodes;
    }

    float average_time() const
    {
        return std::accumulate(v_time.begin(), v_time.end(), 0.0) / c_knobs.u_nodes;
    }

    float average_energy() const
    {
        return std::accumulate(v_energy.begin(), v_energy.end(), 0.0) / c_knobs.u_nodes;
    }

    static bool CompareParetoLoad(const pareto_entry_t &lhs, const pareto_entry_t &rhs)
    {
        // VV: Especially for load we're aiming for LARGER values
        return lhs.average_load() > rhs.average_load();
    }

    static bool CompareParetoTime(const pareto_entry_t &lhs, const pareto_entry_t &rhs)
    {
        return lhs.average_time() < rhs.average_time();
    }

    static bool CompareParetoEnergy(const pareto_entry_t &lhs, const pareto_entry_t &rhs)
    {
        return lhs.average_energy() < rhs.average_energy();
    }

    bool operator<(const pareto_entry_t &rhs) const
    {
        auto u_nodes = c_knobs.u_nodes;

        if (u_nodes < rhs.c_knobs.u_nodes)
            return true;
        else if (u_nodes > rhs.c_knobs.u_nodes)
            return false;

        bool all_lt = true;
        bool any_gt = false;
        for (auto i = 0u; i < u_nodes; ++i)
        {
            all_lt &= v_load[i] < rhs.v_load[i];
            any_gt |= v_load[i] > rhs.v_load[i];
        }

        if (all_lt)
            return true;
        if (any_gt)
            return false;

        all_lt = true;
        any_gt = false;
        for (auto i = 0u; i < u_nodes; ++i)
        {
            all_lt &= v_time[i] < rhs.v_time[i];
            any_gt |= v_time[i] > rhs.v_time[i];
        }

        if (all_lt)
            return true;
        if (any_gt)
            return false;

        all_lt = true;
        any_gt = false;
        for (auto i = 0u; i < v_load.size(); ++i)
        {
            all_lt &= v_energy[i] < rhs.v_energy[i];
            any_gt |= v_energy[i] > rhs.v_energy[i];
        }

        if (all_lt)
            return true;
        if (any_gt)
            return false;

        if (u_at_tick < rhs.u_at_tick)
            return true;

        return false;
    }

    std::size_t u_at_tick;
    ino_knobs_t c_knobs;
    std::vector<float> v_load, v_time, v_energy;
};

typedef bool (*ParetoEntryComparator)(const pareto_entry_t &lhs, const pareto_entry_t &rhs);

struct exploration_t
{
    exploration_t(std::size_t nodes)
        : u_nodes(nodes), b_explored(false)
    {
    }

    ino_knobs_t get_next()
    {
        assert(b_explored == false && "Generator has already been depleted");
        b_explored = true;
        return ino_knobs_t(u_nodes);
    }
    std::size_t u_nodes;
    bool b_explored;
};

typedef std::map<ino_knobs_t, pareto_entry_t, ino_knobs_cmp_t> cmap_history_t;
typedef cmap_history_t::const_iterator citer_history_t;

template <typename work_item>
struct node_config_t
{
    node_config_t() : c_knobs(0){

                      };
    std::vector<work_item> v_work_items;
    ino_knobs_t c_knobs;
};

struct node_load_t
{
    node_load_t(std::size_t node, float load) : node(node), load(load){

                                                              };

    std::size_t node;
    float load;
};

struct internode_optimizer_t
{   
    internode_optimizer_t():
        u_nodes(0), d_target(-1.), d_leeway(-1), u_history_interval(0),
        c_last_choice({0})
    {

    }

    internode_optimizer_t(std::size_t nodes,
                          float target, float leeway,
                          std::size_t reset_history_every = INO_DEFAULT_FORGET_AFTER);

    /*VV: Accepts current state of computational network plus the decision which led
          to this state. If last_knobs is nullptr and INO has made a choice before
          it's going to assume that it's that choice (c_last_choice) which lead to
          the current state.
          
          It returns a set of configuration knobs. If possible it keeps a log of
          the effects of last_knobs
          */
    ino_knobs_t get_node_configuration(const std::vector<float> &measure_load,
                                       const std::vector<float> &measure_time,
                                       const std::vector<float> &measure_energy,
                                       const ino_knobs_t *previous_decision = nullptr);

    template <typename work_item>
    static std::map<std::size_t, node_config_t<work_item>> decide_schedule(
        const std::map<std::size_t, std::vector<work_item>> &node_map,
        const std::map<std::size_t, float> &node_loads,
        const std::map<std::size_t, float> &node_times,
        const ino_knobs_t &ino_knobs,
        bool distribute_randomly = false,
        std::size_t balance_top_N=1,
        std::size_t balance_bottom_K=1)
    {
        /* VV: 4 scenarios
           a) This is the first time that we're scheduling tasks to nodes
           b) We are increasing the number of nodes
           c) We are decreasing the number of nodes
           d) We are keeping the number of nodes the same
           
           Details:

           a) First time scheduling tasks to nodes
              Assign all tasks to node 0 before running this method, then this is
              a sub-case of b)
           b) Increase the number of nodes, and 
           c) Decrease the number of nodes
              1. Add all tasks in a vector, and randomly distribute them `evenly` among
                 new_nodes. 
              2. The resulting node loads will not be 1.0 (less than 1.0 if increasing
                 # of nodes, greater than 1.0 if decreasing # of nodes)
           d) Keep same number of nodes
              1. Find top N nodes whose expected time of completion is largest
              2. Find K nodes whose expected time is smallest
              3. If avg(N) - avg(K) < 2*stddev(ALL), don't do anything
              4. Scale all times of tasks belonging to N nodes such that
                 new_time = (time(N) / avg(time(K))) * old_time
              5. Add all tasks in a vector, and randomly distribute them `evenly` among those 
                 N + K nodes
              6. Resulting load is greater than 1.0 for all N + K nodes
        */

        auto new_schedule = std::map<std::size_t, node_config_t<work_item>>();
        std::size_t previous_number_of_nodes = (std::size_t)node_loads.size();

        // VV: The average load per task belonging to node <std::size_t>
        std::map<std::size_t, float> work_item_load;

        std::size_t total_tasks = 0u;

        for (auto node = node_loads.begin(); node != node_loads.end(); ++node)
        {
            auto node_tasks = node_map.at(node->first).size();
            total_tasks += (std::size_t)node_tasks;

            float task_avg_load = node->second / node_tasks;
            // VV: Make sure that the task avg load is not zero.
            task_avg_load = std::max(0.001f, task_avg_load);
            work_item_load[node->first] = task_avg_load;
        }

        // std::size_t nodes = std::min(ino_knobs.u_nodes, previous_number_of_nodes);
        std::size_t nodes = ino_knobs.u_nodes;

        // VV: Tasks to be moved to a (possibly) new node
        std::vector<std::pair<float, work_item>> all_tasks;

        // VV: This vector contains all eligible nodes to receive new tasks,
        //     and their respective load
        auto expected_node_load = std::vector<node_load_t>();

        if (previous_number_of_nodes != nodes 
            || distribute_randomly 
            || (node_times.size() < balance_top_N + balance_bottom_K))
        {
            //VV: see points b) and c) above, need to redistribute all tasks to all new_nodes

            // VV: <cost, work_item>
            all_tasks.reserve(total_tasks);

            // VV: 1. Generate random vector of tasks (and their associated cost)
            for (const auto work_items : node_map)
            {
                const float cost = work_item_load[work_items.first];

                for (const auto wi : work_items.second)
                    all_tasks.push_back(std::make_pair(cost, wi));
            }

            #ifdef CAN_SHUFFLE
            if ( nodes > 1)
            {
                std::random_device rd;
                std::mt19937 gen(rd());
                std::shuffle(all_tasks.begin(), all_tasks.end(), gen);
            }
            #endif
            
            // for (auto cost_wi : all_tasks) {
            //     std::cerr << "Shuffled " << cost_wi.first
            //               << " -- " << cost_wi.second << std::endl;
            // }

            // VV: All nodes are eligible to receive tasks, they start with a load of 0.0
            for (auto i = 0; i < nodes; ++i)
            {
                expected_node_load.push_back(node_load_t(i, 0.0));
                new_schedule[i] = node_config_t<work_item>();
            }
        }
        else
        {
            // VV: See point d) above
            float avg = 0.;
            std::vector<std::pair<std::size_t, float> > sorted;

            for (auto nt: node_times) {
                avg += nt.second;
                sorted.push_back(nt);
            }

            avg /= node_times.size();

            float stddev_sq = 0.0;

            for (auto nt: node_times ) {
                float t = nt.second - avg;
                stddev_sq += t*t;
            }

            stddev_sq = stddev_sq/(node_times.size() - 1);

            // VV: This is equivalent to checking for stddev * 2
            auto threshold = stddev_sq * 1.41421356237;

            auto compare = [](const std::pair<std::size_t, float> &nt1, 
                             const std::pair<std::size_t, float> &nt2) -> bool 
            {
                // VV: descending order
                return nt1.second > nt2.second;
            };

            std::sort(sorted.begin(), sorted.end(), compare);

            float avg_n = 0.0;
            auto it = sorted.begin();
            for (std::size_t i=0u; i<balance_top_N; ++i, ++it)
            {
                avg_n += it->second;
                // std::cerr << "top_N " << it->first
                //           << " time " << it->second << std::endl;
            }

            avg_n /= balance_top_N;

            float avg_k = 0.0;
            
            auto rit = sorted.rbegin();
            for (std::size_t i=0u; i<balance_bottom_K; ++i, ++rit)
            {   
                avg_k += rit->second;

                // std::cerr << "bottomK " << rit->first 
                //           << " time " << rit->second << std::endl;
            }
            avg_k /= balance_bottom_K;

            #ifdef INO_DEBUG_DECIDE_SCHEDULE
            std::cerr << "avg_n - avg_k = " << 
                        avg_n << " - " << avg_k << " = " << avg_n - avg_k 
                        <<  " comp with " << threshold << std::endl;
            #endif

            // VV: Check if there's enough difference in time to warrant 
            //     redistribution of tasks among N, K node sets
            auto diff = avg_n - avg_k;
            diff *= diff;

            if ( diff >= threshold && threshold > 0.01f )
            {   
                std::size_t total = (std::size_t) sorted.size();

                it = sorted.begin();
                for(std::size_t i=0u;
                    it != sorted.end(); ++it, ++i )
                {
                    if ( i < balance_top_N || i >= total-balance_bottom_K )
                    {
                        expected_node_load.push_back(node_load_t(it->first, 0.0));

                        float cost = work_item_load[it->first];

                        if ( i < balance_top_N )
                        {
                            cost *= node_times.at(it->first) / avg_k;
                        }

                        for (const auto wi : node_map.at(it->first))
                            all_tasks.push_back(std::make_pair(cost, wi));
                    }
                    else
                    {
                        for (const auto wi : node_map.at(it->first))
                            new_schedule[it->first].v_work_items.push_back(wi); 
                    }
                }

                std::random_device rd;
                std::mt19937 gen(rd());
                std::shuffle(all_tasks.begin(), all_tasks.end(), gen);
            }
        }

        // VV: Partition `all_tasks` to `expected_node_load` nodes "evenly"
        if (expected_node_load.size() > 1)
        {
            // VV: Assign task to least loaded node. (Nodes could be oversubscribed)
            for (const auto cost_wi : all_tasks)
            {
                // cost_wi = <float, wi> (cost and work_item)

                if (expected_node_load[0].load > expected_node_load[1].load)
                {
                    auto previous_lowest = expected_node_load[0];
                    expected_node_load.erase(expected_node_load.begin());

                    bool updated = false;
                    for (auto it = expected_node_load.begin();
                         it != expected_node_load.end();
                         ++it)
                    {
                        if (it->load > previous_lowest.load)
                        {
                            updated = true;
                            expected_node_load.insert(it, previous_lowest);
                            break;
                        }
                    }

                    if (updated == false)
                    {
                        expected_node_load.push_back(previous_lowest);
                    }
                }

                expected_node_load[0].load += cost_wi.first;
                new_schedule[expected_node_load[0].node].v_work_items.push_back(cost_wi.second);
            }
        }
        else if (expected_node_load.size() == 1)
        {
            float total_cost = 0.0;
            // VV: Special case for single-node configurations
            node_load_t &target_node = expected_node_load.at(0);

            for (const auto cost_wi : all_tasks)
            {
                total_cost += cost_wi.first;
                new_schedule[target_node.node].v_work_items.push_back(cost_wi.second);
            }
            target_node.load = total_cost;
        }

        for (auto it = new_schedule.begin(); it != new_schedule.end();)
            if ( it->second.v_work_items.size() == 0)
                it = new_schedule.erase(it);
            else
                ++ it;
        
        for (auto it = expected_node_load.begin(); it != expected_node_load.end();)
            if (it->load <= 0.0 )
                it = expected_node_load.erase(it);
            else
                ++ it;

        #ifdef INO_DEBUG_DECIDE_SCHEDULE
        if (expected_node_load.size())
        {
            std::cerr << "Target: " << ino_knobs.u_nodes
                      << " total nodes " << node_loads.size()
                      << std::endl;
            for (auto node_load : expected_node_load)
            {
                std::cerr << "Node " << node_load.node
                          << " load " << node_load.load << std::endl;
            }

            for (auto sched : new_schedule)
            {
                std::cerr << "Schedule for node " << sched.first << ": ";
                for (auto wi : sched.second.v_work_items)
                {
                    std::cerr << wi << " ";
                }
                std::cerr << std::endl;
            }
        }
        #endif

        return new_schedule;
    }

    void record_point(const ino_knobs_t &knobs,
                      const std::vector<float> &measure_load,
                      const std::vector<float> &measure_time,
                      const std::vector<float> &measure_energy);

    /*VV: Utilities*/
  protected:
    void try_forget();

    ino_knobs_t get_next_explore();
    ino_knobs_t get_best();

    void explore_configurations();

    ino_knobs_t c_last_choice;

    std::size_t u_nodes, u_min_nodes, u_max_nodes;
    std::size_t u_choices, u_history_interval, u_history_tick;
    float d_target, d_leeway;

    // VV: <number_of_nodes, pareto_entry>

    cmap_history_t v_past;

    // VV: Exploration logistics
    std::vector<exploration_t> v_explore_logistics;
    std::set<ino_knobs_t, ino_knobs_cmp_t> s_forgotten, s_explore;
};

} // namespace components
} // namespace allscale

#endif // ALLSCALE_COMPONENTS_INTERNODEOPTIMIZER_HPP