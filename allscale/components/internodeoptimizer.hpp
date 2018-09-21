#ifndef ALLSCALE_COMPONENTS_INTERNODEOPTIMIZER_HPP
#include <numeric>
#include <vector>
#include <set>
#include <map>
#include <tuple>
#include <cassert>

#define INO_EXPLORATION_PHASE_STEPS 3u
#define INO_EXPLORATION_SOFT_LIMIT 10u
#define INO_RANDOM_EVERY 3u
#define INO_DEFAULT_FORGET_AFTER 10000u

#define INO_DEBUG_PARETO

#ifdef INO_RANDOM_EVERY
#if INO_RANDOM_EVERY < 1
#error INO_RANDOM_EVERY must be a positive number (if its equal to 2, every 3rd balance() will explore)
#endif
#endif

namespace allscale
{
namespace components
{

struct ino_knobs_t
{
    ino_knobs_t(unsigned int nodes)
        : u_nodes(nodes)
    {
    }

    unsigned int u_nodes;
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
    pareto_entry_t(unsigned int at_tick,
                 ino_knobs_t knobs,
                 std::vector<double> measure_load,
                 std::vector<double> measure_time,
                 std::vector<double> measure_energy)
        : u_at_tick(at_tick),
          c_knobs(knobs),
          v_load(measure_load),
          v_time(measure_time),
          v_energy(measure_energy)
    {
        // VV: Perhaps take into account the mean of the vectors
        // std::aggregate(<vector>.begin(), <vector>.end(), 0.0) / nodes
    }

    double average_load() const
    {
        return std::accumulate(v_load.begin(), v_load.end(), 0.0) / c_knobs.u_nodes;
    }

    double average_time() const
    {
        return std::accumulate(v_time.begin(), v_time.end(), 0.0) / c_knobs.u_nodes;
    }

    double average_energy() const
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

    unsigned int u_at_tick;
    ino_knobs_t c_knobs;
    std::vector<double> v_load, v_time, v_energy;
};

typedef bool (*ParetoEntryComparator)(const pareto_entry_t &lhs, const pareto_entry_t &rhs);

struct exploration_t
{
    exploration_t(unsigned int nodes)
        : u_nodes(nodes), b_explored(false)
    {
    }

    ino_knobs_t get_next()
    {
        assert(b_explored == false && "Generator has already been depleted");
        b_explored = true;
        return ino_knobs_t(u_nodes);
    }
    unsigned int u_nodes;
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
    node_load_t(unsigned int node, double load) : node(node), load(load){

                                                              };

    unsigned int node;
    double load;
};

struct internode_optimizer_t
{
    internode_optimizer_t(unsigned int nodes,
                        double target, double leeway,
                        unsigned int reset_history_every = INO_DEFAULT_FORGET_AFTER);

    /*VV: Accepts current state of computational network plus the decision which led
          to this state. If last_knobs is nullptr and INO has made a choice before
          it's going to assume that its that choice (c_last_choice) which lead to
          the current state.
          
          It returns a set of configuration knobs. If possible it keeps a log of
          the effects of last_knobs
          */
    ino_knobs_t balance(const std::vector<double> &measure_load,
                        const std::vector<double> &measure_time,
                        const std::vector<double> &measure_energy,
                        const ino_knobs_t *previous_decision = nullptr);

    template <typename work_item>
    std::map<unsigned int, node_config_t<work_item>> decide_schedule(
        const std::map<unsigned int, std::vector<work_item>> &node_map,
        const std::map<unsigned int, double> &node_loads,
        const ino_knobs_t &ino_knobs) const
    {
        auto new_schedule = std::map<unsigned int, node_config_t<work_item>>();

        // VV: The average load per task belonging to node <unsigned int>
        std::map<unsigned int, double> work_item_load;

        auto expected_node_load = std::vector<node_load_t>();

        for (auto node = node_loads.begin(); node != node_loads.end(); ++node)
        {
            double task_avg_load = node->second / node_map.at(node->first).size();
            // VV: Make sure that the task avg load is not zero.
            task_avg_load = std::max(0.001, task_avg_load);
            work_item_load[node->first] = task_avg_load;
        }
        unsigned int nodes = std::min((long unsigned int)ino_knobs.u_nodes,
                                      node_loads.size());

        for (auto i = 0; i < nodes; ++i)
        {
            expected_node_load.push_back(node_load_t(i, 0.0));
            new_schedule[i] = node_config_t<work_item>();
        }

        if (ino_knobs.u_nodes > 1 && node_loads.size() > 1)
        {
            // VV: Assign task to least loaded node. (Nodes could be oversubscribed)
            for (const auto work_items : node_map)
            {
                const double cost = work_item_load[work_items.first];

                for (const auto wi : work_items.second)
                {
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

                    expected_node_load[0].load += cost;
                }
            }
        }
        else
        {
            double total_cost = 0.0;
            // VV: Special case for single-node configurations
            for (const auto work_items : node_map)
            {
                const double cost = work_item_load[work_items.first];
                for (const auto wi : work_items.second)
                {
                    total_cost += cost;
                    new_schedule[0].v_work_items.push_back(wi);
                }
            }
            expected_node_load[0].load = total_cost;
        }
        std::cout << "Target: " << ino_knobs.u_nodes
                  << " total nodes " << node_loads.size()
                  << std::endl;
        for (auto node_load : expected_node_load)
        {
            std::cout << "Node " << node_load.node
                      << " load " << node_load.load << std::endl;
        }

        return new_schedule;
    }

    void record_point(const ino_knobs_t &knobs,
                      const std::vector<double> &measure_load,
                      const std::vector<double> &measure_time,
                      const std::vector<double> &measure_energy);

    /*VV: Utilities*/
  protected:
    void try_forget();

    ino_knobs_t get_next_explore();
    ino_knobs_t get_best();

    void explore_configurations();

    ino_knobs_t c_last_choice;

    unsigned int u_nodes, u_min_nodes, u_max_nodes;
    unsigned int u_choices, u_history_interval, u_history_tick;
    double d_target, d_leeway;

    // VV: <number_of_nodes, pareto_entry>

    cmap_history_t v_past;

    // VV: Exploration logistics
    std::vector<exploration_t> v_explore_logistics;
    std::set<ino_knobs_t, ino_knobs_cmp_t> s_forgotten, s_explore;
};

} // namespace components
} // namespace allscale

#endif // ALLSCALE_COMPONENTS_INTERNODEOPTIMIZER_HPP