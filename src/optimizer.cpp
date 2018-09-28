#include <allscale/optimizer.hpp>
#ifdef ALLSCALE_HAVE_CPUFREQ
#include <allscale/util/hardware_reconf.hpp>
#endif

#include <allscale/scheduler.hpp>
#include <allscale/components/scheduler.hpp>
#include <allscale/monitor.hpp>
#include <allscale/components/monitor.hpp>

#include <hpx/include/actions.hpp>
#include <hpx/lcos/broadcast.hpp>
#include <hpx/async.hpp>
#include <hpx/util/assert.hpp>

#include <algorithm>
#include <cmath>
#include <iostream>
#include <iomanip>

#include <sys/types.h>
#include <unistd.h>

#define HISTORY_ITERATIONS 10

namespace allscale
{
optimizer_state get_optimizer_state()
{
    static unsigned long long last_energy = 0;
    unsigned long long this_energy = 0;

    float load = 1.f - (monitor::get().get_idle_rate() / 100.);

    float my_time = monitor::get().get_avg_time_last_iterations(HISTORY_ITERATIONS);

    if ((std::isfinite(my_time) == false) || (my_time < 0.01f))
        my_time = -1.f;

#ifdef ALLSCALE_HAVE_CPUFREQ
    float frequency = components::util::hardware_reconf::get_kernel_freq(0);
    auto current = components::util::hardware_reconf::read_system_energy();

    this_energy = current - last_energy;
    last_energy = read;
#else
    float frequency = 1.f;
#endif
    return optimizer_state(load,
                           my_time,
                           this_energy,
                           frequency,
                           scheduler::get().get_active_threads());
}

void optimizer_update_policy(std::vector<optimizer_state> const &state, std::vector<bool> mask)
{
    scheduler::update_policy(state, mask);
}

void optimizer_update_policy_ino(const std::vector<std::size_t> &new_mapping)
{
    scheduler::apply_new_mapping(new_mapping);
}

} // namespace allscale

HPX_PLAIN_DIRECT_ACTION(allscale::get_optimizer_state, allscale_get_optimizer_state_action);
HPX_PLAIN_DIRECT_ACTION(allscale::optimizer_update_policy, allscale_optimizer_update_policy_action);
HPX_PLAIN_DIRECT_ACTION(allscale::optimizer_update_policy_ino, allscale_optimizer_update_policy_action_ino);

namespace allscale
{
float tuning_objective::score(float speed, float efficiency, float power) const
{
    return std::pow(speed, speed_exponent) *
           std::pow(efficiency, efficiency_exponent) *
           std::pow(1 - power, power_exponent);
}

std::ostream &operator<<(std::ostream &os, tuning_objective const &obj)
{
    std::cout
        << "t^" << obj.speed_exponent << " * "
        << "e^" << obj.efficiency_exponent << " * "
        << "p^" << obj.power_exponent;
    return os;
}

tuning_objective get_default_objective()
{
    char *const c_obj = std::getenv("ALLSCALE_OBJECTIVE");

    if (!c_obj)
        return tuning_objective::speed();

    std::string obj = c_obj;
    if (obj == "speed")
        return tuning_objective::speed();
    if (obj == "efficiency")
        return tuning_objective::efficiency();
    if (obj == "power")
        return tuning_objective::power();

    float speed = 0.0f;
    float efficiency = 0.0f;
    float power = 0.0f;
    for (char c : obj)
    {
        switch (c)
        {
        case 's':
            speed += 1.0f;
            break;
        case 'e':
            efficiency += 1.0f;
            break;
        case 'p':
            power += 1.0f;
            break;
        default:
            std::cerr << "Unkown tuning objective: " << obj << ", falling back to speed\n";
            return tuning_objective::speed();
        }
    }
    return tuning_objective(speed, efficiency, power);
}

float estimate_power(float frequency)
{
    // simply estimate 100 pJ per cycle
    // TODO: integrate frequency

    // estimation: something like  C * f * U^2 * cycles
    // where C is some hardware constant
    float C = 100.f;

    // voltage U should be adapted to the frequency
    float U = 0.8; // TODO: adapt to frequency

    // so we obtain
    return C * frequency * frequency * (U * U);
}

global_optimizer::global_optimizer()
    : num_active_nodes_(allscale::get_num_localities()), best_(0, 1.f), best_score_(0.0f), active_(true), objective_(get_default_objective()), localities_(hpx::find_all_localities()),
    f_resource_max(-1.0f), f_resource_leeway(-1.0f),
    u_balance_every(10)
{
    char *const c_policy = std::getenv("ALLSCALE_SCHEDULING_POLICY");

    if (strncasecmp(c_policy, "ino", 3) == 0 )
    {
        char *const c_resource_max = std::getenv("ALLSCALE_RESOURCE_MAX");
        char *const c_resource_leeway = std::getenv("ALLSCALE_RESOURCE_LEEWAY");
        char *const c_balance_every = std::getenv("ALLSCALE_INO_BALANCE_EVERY");

        if ( c_balance_every )
            u_balance_every = (std::size_t) atoi(c_balance_every);

        if ( !c_resource_leeway )
            f_resource_leeway = 0.25f;
        else
            f_resource_leeway = atof(c_resource_leeway);

        if (!c_resource_max)
            f_resource_max = 0.75f;
        else
            f_resource_max = atof(c_resource_max);

        o_ino = allscale::components::internode_optimizer_t(localities_.size(),
                                                            (double) f_resource_max,
                                                            (double) f_resource_leeway,
                                                            INO_DEFAULT_FORGET_AFTER);
    }
}

void global_optimizer::tune(std::vector<optimizer_state> const &state)
{
    #ifdef ALLSCALE_HAVE_CPUFREQ
    float max_frequency = components::util::hardware_reconf::get_frequencies(0).back();
    #else
    float max_frequency = 1.f;
    #endif
    // Assume all CPUs have the same frequency for now...
    //         for (std::size_t i = 0; i != cores_per_node; ++i)
    //         {
    //             auto freq = hardware_reconf::get_kernel_freq(i) * 1000f;
    //             if (freq > max_frequency)
    //                 max_frequency = freq;
    //         }

    float active_frequency = 0.0f;

    std::uint64_t used_cycles = 0;
    std::uint64_t avail_cycles = 0;
    std::uint64_t max_cycles = 0;
    float used_power = 0;
    float max_power = 0;

    for (std::size_t i = 0; i < allscale::get_num_localities(); ++i)
    {
        if (i < num_active_nodes_)
        {
            used_cycles += state[i].load * state[i].active_frequency * state[i].cores_per_node;
            avail_cycles += state[i].active_frequency * state[i].cores_per_node;
            used_power += estimate_power(state[i].active_frequency);
            active_frequency += state[i].active_frequency / num_active_nodes_;
        }
        max_cycles += max_frequency * state[i].cores_per_node;
        max_power = estimate_power(max_frequency);
    }

    float speed = used_cycles / float(max_cycles);
    float efficiency = used_cycles / float(avail_cycles);
    float power = used_power / max_power;

    float score = objective_.score(speed, efficiency, power);

    for (const auto &s : state)
        std::cout << "load:" << s.load << " time:" << s.avg_time << " freq:" << s.active_frequency << " cores:" << s.cores_per_node << std::endl;

    std::cerr << "\tSystem state:"
              << " spd=" << std::setprecision(2) << speed
              << ", eff=" << std::setprecision(2) << efficiency
              << ", pow=" << std::setprecision(2) << power
              << " => score: " << std::setprecision(2) << score
              << " for objective " << objective_ << "\n";

    // record current solution
    if (score > best_score_)
    {
        best_ = {num_active_nodes_, active_frequency};
        best_score_ = score;
    }

    // pick next state
    if (explore_.empty())
    {
        // nothing left to explore => generate new points
        std::cerr << "\t\tPrevious best option " << best_.first << " @ " << best_.second << " with score " << best_score_ << "\n";

        // get nearby frequencies
    #ifdef ALLSCALE_HAVE_CPUFREQ
        auto options = components::util::hardware_reconf::get_frequencies(0);
    #else
        auto options = std::vector<float>(1, 1.f);
    #endif
        auto cur = std::find(options.begin(), options.end(), best_.second);

        HPX_ASSERT(cur != options.end());
        auto pos = cur - options.begin();

        std::vector<float> frequencies;
        if (pos != 0)
            frequencies.push_back(options[pos - 1]);
        frequencies.push_back(options[pos]);
        if (pos + 1 < int(options.size()))
            frequencies.push_back(options[pos + 1]);

        // get nearby node numbers
        std::vector<std::size_t> num_nodes;
        if (best_.first > 1)
            num_nodes.push_back(best_.first - 1);
        num_nodes.push_back(best_.first);
        if (best_.first < allscale::get_num_localities())
            num_nodes.push_back(best_.first + 1);

        // create new options
        for (const auto &a : num_nodes)
        {
            for (const auto &b : frequencies)
            {
                std::cerr << "\t\tAdding option " << a << " @ " << b << "\n";
                explore_.push_back({a, b});
            }
        }

        // reset best options
        best_score_ = 0;
    }

    // if there are still no options, there is nothing to do
    if (explore_.empty())
        return;

    // take next option and evaluate
    auto next = explore_.back();
    explore_.pop_back();

    num_active_nodes_ = next.first;
    active_frequency_ = next.second;

    std::cout << "\t\tSwitching to " << num_active_nodes_ << " @ " << active_frequency_ << "\n";
}

hpx::future<void> global_optimizer::balance(bool tuned)
{
    // The load balancing / optimization is split into two parts:
    //  Part A: the balance function, attempting to even out resource utilization between nodes
    //			for a given number of active nodes and a CPU clock frequency
    //  Part B: in evenly balanced cases, the tune function is evaluating the current
    //          performance score and searching for better alternatives

    // -- Step 1: collect node distribution --

    // collect the load of all nodes
    // FIXME: make resilient: filter out failed localities...
    return hpx::lcos::broadcast<allscale_get_optimizer_state_action>(
               localities_)
        .then(
            [this, tuned](hpx::future<std::vector<optimizer_state>> future_state) {
                auto state = future_state.get();

                // compute the load variance
                optimizer_state avg_state = std::accumulate(
                    state.begin(), state.begin() + num_active_nodes_,
                        optimizer_state(0.f, 0.f, 0ull, 0.f, 0),
                    [this](optimizer_state const &lhs, optimizer_state const &rhs) {
                        return optimizer_state(
                            (lhs.load + rhs.load),
                            (lhs.avg_time + rhs.avg_time),
                            (lhs.energy + rhs.energy),
                            (lhs.active_frequency + rhs.active_frequency),
                            (lhs.cores_per_node + rhs.cores_per_node));
                    });

                // VV: Make sure to divide by num_active_nodes_ just once after accumulating
                avg_state.load /= num_active_nodes_;
                avg_state.avg_time /= num_active_nodes_;
                avg_state.active_frequency /= num_active_nodes_;
                avg_state.cores_per_node /= num_active_nodes_;

                float sum_dist = 0.f;
                for (std::size_t i = 0; i < num_active_nodes_; i++)
                {
                    float dist = state[i].load - avg_state.load;
                    sum_dist += dist * dist;
                }
                float var = sum_dist / (num_active_nodes_ - 1);
                if (num_active_nodes_ == 1)
                    var = 0.f;

                for (const auto &s : state)
                    std::cout << "load:" << s.load << " time:" << s.avg_time << " freq:" << s.active_frequency << " cores:" << s.cores_per_node << std::endl;

                std::cerr
                    << "Average load " << std::setprecision(2) << avg_state.load
                    << ", load variance " << std::setprecision(2) << var
                    << ", average frequency " << std::setprecision(2) << avg_state.active_frequency
                    << ", total progress " << std::setprecision(2) << (avg_state.load * num_active_nodes_)
                    << " on " << num_active_nodes_ << " nodes\n";
                // -- Step 2: if stable, adjust number of nodes and clock speed

                // if stable enough, allow meta-optimizer to manage load
                if (tuned && var < 0.01)
                {
                    // adjust number of nodes and CPU frequency
                    tune(state);
                }
                // -- Step 3: enforce selected number of nodes and clock speed, keep system balanced

                // compute number of nodes to be used
                std::vector<bool> mask(localities_.size());
                for (std::size_t i = 0; i < localities_.size(); i++)
                {
                    mask[i] = i < num_active_nodes_;
                }

                hpx::lcos::broadcast_apply<allscale_optimizer_update_policy_action>(
                    localities_, state, mask);
                //                     // get the local scheduler
                //                     auto& scheduleService = node.getService<com::HierarchyService<ScheduleService>>().get(0);
                //
                //                     // get current policy
                //                     auto policy = scheduleService.getPolicy();
                //                     assert_true(policy.isa<DecisionTreeSchedulingPolicy>());
                //
                //                     // re-balance load
                //                     auto curPolicy = policy.getPolicy<DecisionTreeSchedulingPolicy>();
                //                     auto newPolicy = DecisionTreeSchedulingPolicy::createReBalanced(curPolicy,load,mask);
                //
                //                     // distribute new policy
            });
}

hpx::future<void> global_optimizer::balance_ino(const std::vector<std::size_t> &old_mapping)
{
    /*VV: Compute the new ino_knobs (i.e. number of Nodes), then assign tasks to
          nodes and broadcast the scheduling information to all nodes.
    */
    return hpx::lcos::broadcast<allscale_get_optimizer_state_action>(localities_)
        .then(
            [this, old_mapping](hpx::future<std::vector<optimizer_state> > future_state) {
                auto state = future_state.get();
                auto get_max_avg_time = [](float curr_max, const optimizer_state &s) -> float {
                    return std::max(curr_max, s.avg_time);
                };

                float max_time = std::accumulate(state.begin()+1,
                                                state.end(),
                                                state.begin()->avg_time,
                                                get_max_avg_time);

                if (max_time  < 0.1f ) {
                    max_time = 0.1f;
                }

                for (auto it = state.begin(); it != state.end(); ++it) {
                    if (it->avg_time < 0.1f )
                        it->avg_time = max_time;
                }

                #ifdef INO_DEBUG_DECIDE_SCHEDULE
                for (const auto &s : state)
                     std::cerr << "load:" << s.load << " time:" << s.avg_time << " freq:" << s.active_frequency << " cores:" << s.cores_per_node << std::endl;
                #endif

                std::map<std::size_t, std::vector<std::size_t> > node_map;
                std::vector<size_t> new_mapping;
                std::map<std::size_t, float> node_loads, node_times;
                // VV: This is using a vector instead of a map, should make both
                //     APIs use the same parameter format
                std::vector<float> ino_energies, ino_loads, ino_times;

                // VV: Create node_map from old mapping
                std::size_t idx = 0ul;

                for (auto it = old_mapping.begin(); it != old_mapping.end(); ++it, ++idx)
                    node_map[*it].push_back(idx);

                // VV: Create node_loads, and node_times from optimizer_state
                idx = 0ul;
                auto node = state.begin();
                for (idx=0ul; idx<=num_active_nodes_; ++idx, ++node)
                {
                    node_loads[idx] = node->load;
                    node_times[idx++] = node->avg_time;

                    ino_loads.push_back(node->load);
                    ino_times.push_back(node->avg_time);
                    ino_energies.push_back((float)node->energy);
                }

                auto knobs = o_ino.get_node_configuration(ino_loads,
                                                          ino_times,
                                                          ino_energies);

                num_active_nodes_ = knobs.u_nodes;

                auto ino_schedule = components::internode_optimizer_t \
                                              ::decide_schedule(node_map,
                                                                node_loads,
                                                                node_times,
                                                                knobs);
                if (ino_schedule.size())
                {
                    #ifdef INO_DEBUG_DECIDE_SCHEDULE
                    std::cerr << "Ino picked a schedule" << std::endl;
                    #endif
                    for (auto node_wis : ino_schedule)
                        for (auto wi : node_wis.second.v_work_items)
                            new_mapping[wi] = node_wis.first;

                    hpx::lcos::broadcast_apply<allscale_optimizer_update_policy_action_ino>(localities_, new_mapping);
                }
                #ifdef INO_DEBUG_DECIDE_SCHEDULE
                else {
                    std::cerr << "Ino did not modify schedule" << std::endl;
                }
                #endif

                // std::cerr << "Broadcasting" << std::endl << std::flush;
                // hpx::lcos::broadcast_apply<allscale_optimizer_update_policy_action_ino>(localities_, old_mapping);
                // std::cerr << "Broadcasting [DONE]" << std::endl << std::flush;
            });
}
} // namespace allscale
