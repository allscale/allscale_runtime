#ifndef ALLSCALE_COMPONENTS_INTERNODEOPTIMIZER_HPP
#include <numeric>
#include <vector>
#include <set>
#include <map>
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
typedef struct PARETO_ENTRY pareto_entry_t;
typedef struct INO_KNOBS ino_knobs_t;
typedef struct INO_KNOBS_CMP ino_knobs_cmp_t;
typedef struct EXPLORATION exploration_t;
typedef struct INTERNODE_OPTIMIZER internode_optimizer_t;
typedef struct NODE_CONFIGURATION node_config_t;

typedef bool(*ParetoEntryComparator)  (const pareto_entry_t &lhs, const pareto_entry_t &rhs);

struct INO_KNOBS
{
    INO_KNOBS(unsigned int nodes)
        : u_nodes(nodes)
    {
    }

    unsigned int u_nodes;
};

struct INO_KNOBS_CMP
{
    bool operator()(const struct INO_KNOBS &lhs, const struct INO_KNOBS &rhs)
    {
        return lhs.u_nodes < rhs.u_nodes;
    }
};

struct PARETO_ENTRY
{
    PARETO_ENTRY(unsigned int at_tick,
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
        return std::accumulate(v_load.begin(), v_load.end(), 0.0)/c_knobs.u_nodes;
    }

    double average_time() const
    {
        return std::accumulate(v_time.begin(), v_time.end(), 0.0)/c_knobs.u_nodes;
    }

    double average_energy() const
    {
        return std::accumulate(v_energy.begin(), v_energy.end(), 0.0)/c_knobs.u_nodes;
    }

    static bool CompareParetoLoad(const struct PARETO_ENTRY &lhs, const struct PARETO_ENTRY &rhs)
    {   
        // VV: Especially for load we're aiming for LARGER values
        return lhs.average_load() > rhs.average_load();
    }

    static bool CompareParetoTime(const struct PARETO_ENTRY &lhs, const struct PARETO_ENTRY &rhs)
    {   
        return lhs.average_time() < rhs.average_time();
    }

    static bool CompareParetoEnergy(const struct PARETO_ENTRY &lhs, const struct PARETO_ENTRY &rhs)
    {   
        return lhs.average_energy() < rhs.average_energy();
    }

    bool operator<(const pareto_entry_t &rhs) const {
        auto u_nodes = c_knobs.u_nodes;

        if (u_nodes < rhs.c_knobs.u_nodes)
            return true;
        else if ( u_nodes > rhs.c_knobs.u_nodes )
            return false;
        
        bool all_lt = true;
        bool any_gt = false;
        for (auto i=0u; i<u_nodes; ++i) {
            all_lt &= v_load[i] < rhs.v_load[i];
            any_gt |= v_load[i] > rhs.v_load[i];
        }

        if (all_lt)
            return true;
        if (any_gt)
            return false;

        all_lt = true;
        any_gt = false;
        for (auto i=0u; i<u_nodes; ++i) {
            all_lt &= v_time[i] < rhs.v_time[i];
            any_gt |= v_time[i] > rhs.v_time[i];
        }

        if (all_lt)
            return true;
        if (any_gt)
            return false;

        all_lt = true;
        any_gt = false;
        for (auto i=0u; i<v_load.size(); ++i) {
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

struct EXPLORATION
{
    EXPLORATION(unsigned int nodes)
        : u_nodes(nodes), b_explored(false)
    {
    }
    
    ino_knobs_t get_next()
    {
        assert(b_explored==false && "Generator has already been depleted");
        b_explored = true;
        return ino_knobs_t(u_nodes);
    }
    unsigned int u_nodes;
    bool b_explored;
};

typedef std::map<struct INO_KNOBS, struct PARETO_ENTRY, struct INO_KNOBS_CMP> cmap_history_t;
typedef cmap_history_t::const_iterator citer_history_t;

struct NODE_CONFIG
{

};

struct INTERNODE_OPTIMIZER
{
    INTERNODE_OPTIMIZER(unsigned int nodes,
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
                        const ino_knobs_t *previous_decision=nullptr);

    std::map<int, node_config_t> apply_policy(const std::vector<double> &cost) const;

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


        // V: <number_of_nodes, pareto_entry>

        cmap_history_t v_past;

        // VV: Exploration logistics
        std::vector<exploration_t> v_explore_logistics;
        std::set<ino_knobs_t, ino_knobs_cmp_t> s_forgotten, s_explore;
};

} // namespace components
} // namespace allscale

#endif // ALLSCALE_COMPONENTS_INTERNODEOPTIMIZER_HPP