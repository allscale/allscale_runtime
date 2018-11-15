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

#define TRULY_RANDOM_DEBUG

namespace allscale
{
    optimizer_state get_optimizer_state()
    {
        float load = 1.f - monitor::get().get_idle_rate();
        float my_time = monitor::get().get_avg_time_last_iterations(HISTORY_ITERATIONS);

        if ((std::isfinite(my_time) == false) || (my_time < 0.01f))
            my_time = -1.f;

        allscale::components::monitor *monitor_c = &allscale::monitor::get();
        float power_now = 100.f;
#if defined(POWER_ESTIMATE) || defined(ALLSCALE_HAVE_CPUFREQ)
        power_now = monitor_c->get_current_power();
#endif
        // VV: Use power as if it were energy
        return {
            load,
            monitor::get().get_task_times(),
            my_time,
            power_now,
            float(monitor_c->get_current_freq(0)),
            scheduler::get().get_active_threads()
        };
    }
// optimizer_state get_optimizer_state()
// {
//     static unsigned long long last_energy = 0;
//     unsigned long long this_energy = 0;
//
//     float load = 1.f - (monitor::get().get_idle_rate() / 100.);
//
//     float my_time = monitor::get().get_avg_time_last_iterations(HISTORY_ITERATIONS);
//
//     if ((std::isfinite(my_time) == false) || (my_time < 0.01f))
//         my_time = -1.f;
//
// #ifdef ALLSCALE_HAVE_CPUFREQ
//     float frequency = components::util::hardware_reconf::get_kernel_freq(0);
//     auto current = components::util::hardware_reconf::read_system_energy();
//
//     this_energy = current - last_energy;
//     last_energy = read;
// #else
//     float frequency = 1.f;
// #endif
//     return optimizer_state(load,
//                            my_time,
//                            this_energy,
//                            frequency,
//                            scheduler::get().get_active_threads());
// }

    void optimizer_update_policy(task_times const& times, std::vector<bool> mask, std::uint64_t freq)
    {
        scheduler::update_policy(times, mask, freq);
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
    os
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
    : u_balance_every(10), u_steps_till_rebalance(u_balance_every),
    active_nodes_(allscale::get_num_localities(), true), tuner_(new simple_coordinate_descent(tuner_configuration{active_nodes_, allscale::monitor::get().get_current_freq(0)})),
    objective_(get_default_objective()),
    active_(true), localities_(hpx::find_all_localities()),
    f_resource_max(-1.0f), f_resource_leeway(-1.0f)
{
    char *const c_policy = std::getenv("ALLSCALE_SCHEDULING_POLICY");

    if (c_policy && strncasecmp(c_policy, "ino", 3) == 0 )
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
//     else if ( strncasecmp(c_policy, "truly_random", 12) == 0 ) {
//         char *const c_balance_every = std::getenv("ALLSCALE_TRULY_RANDOM_BALANCE_EVERY");
//
//         if ( c_balance_every ) {
//             u_balance_every = (std::size_t) atoi(c_balance_every);
//             u_steps_till_rebalance = u_balance_every;
//         }
//     }
}

void global_optimizer::tune(std::vector<optimizer_state> const &state)
{
    allscale::components::monitor *monitor_c = &allscale::monitor::get();
    std::uint64_t max_frequency = monitor_c->get_max_freq(0);

    std::size_t num_active_nodes = std::count(active_nodes_.begin(), active_nodes_.end(), true);

    // compute overall speed

    float total_speed = 0.f;
    float total_efficiency = 0.f;
    float used_power = 0.f;
    float max_power = 0.f;
    float cur_power = 0.f;

    for (std::size_t i = 0; i != active_nodes_.size(); ++i)
    {
        if (i < num_active_nodes)
        {
            total_speed += state[i].load_;
            total_efficiency += state[i].load_ * (float(state[i].active_frequency_ * state[i].cores_per_node_) / float(max_frequency * state[i].cores_per_node_));;
            used_power += state[i].energy_;
        }
#ifdef POWER_ESTIMATE
        max_power += monitor_c->get_max_power();
#endif
    }

    tuner_state tune_state;

    tune_state.speed = total_speed / active_nodes_.size();
    tune_state.efficiency = total_efficiency / active_nodes_.size();
    tune_state.power = used_power / max_power;

    tune_state.score = objective_.score(tune_state.speed, tune_state.efficiency, tune_state.power);

    std::cerr << tune_state << " for objective " << objective_ << '\n';

    auto next_cfg = tuner_->next(tuner_configuration{active_nodes_, active_frequency_}, tune_state, objective_);
    active_nodes_ = std::move(next_cfg.node_mask);
    active_frequency_ = next_cfg.frequency;
}

    hpx::future<void> global_optimizer::balance(bool tuned)
    {
        // The load balancing / optimization is split into two parts:
        //  Part A: the balance function, attempting to even out resource utilization between nodes
        //          for a given number of active nodes and a CPU clock frequency
        //  Part B: in evenly balanced cases, the tune function is evaluating the current
        //          performance score and searching for better alternatives


        // -- Step 1: collect node distribution --

        // collect the load of all nodes
        // FIXME: make resilient: filter out failed localities...
        return hpx::lcos::broadcast<allscale_get_optimizer_state_action>(
            localities_).then(
                [this, tuned](hpx::future<std::vector<optimizer_state>> future_state) mutable
                {
                    task_times times;
                    {
                        std::lock_guard<mutex_type> lk(mtx_);
                        auto state = future_state.get();
    //                     std::vector<float> load;
    //                     load.reserve(state.size());

                        // compute the load variance
                        float avg_load = 0.0f;

                        std::size_t num_active_nodes = std::count(active_nodes_.begin(), active_nodes_.end(), true);
                        std::for_each(state.begin(), state.end(),
                            [&times, &avg_load](optimizer_state const& s)
                            {
    //                             load.push_back(s.load_);
                                avg_load += s.load_;
                                times += s.task_times_;
                            });
                        avg_load = avg_load / num_active_nodes;


                        float sum_dist = 0.f;
                        for(std::size_t i=0; i<num_active_nodes; i++)
                        {
                            float dist = state[i].load_ - avg_load;
                            sum_dist +=  dist * dist;
                        }
                        float var = (num_active_nodes > 1) ? sum_dist / (num_active_nodes - 1) : 0.0f;

                        std::cerr
    //                         << times << '\n'
                            << "Average load "      << std::setprecision(2) << avg_load
                            << ", load variance "   << std::setprecision(2) << var
    //                         << ", average frequency "   << std::setprecision(2) << avg_state.active_frequency
                            << ", total progress " << std::setprecision(2) << (avg_load*num_active_nodes)
                            << " on " << num_active_nodes << " nodes\n";
                        // -- Step 2: if stable, adjust number of nodes and clock speed

                        // if stable enough, allow meta-optimizer to manage load
                        if (tuned && var < 0.01f)
                        {
                            // adjust number of nodes and CPU frequency
                            tune(state);
                        }
                    }
                    // -- Step 3: enforce selected number of nodes and clock speed, keep system balanced

                    return hpx::lcos::broadcast<allscale_optimizer_update_policy_action>(
                        localities_, times, active_nodes_, active_frequency_);
                }
            );
    }

bool global_optimizer::may_rebalance()
{
    if (u_steps_till_rebalance) {
        u_steps_till_rebalance --;
        return false;
    }

    return true;
}

hpx::future<void> global_optimizer::decide_random_mapping(const std::vector<std::size_t> &old_mapping)
{
    auto num_localities = localities_.size();

    // VV: Simulate random nodes leaving/joining the computation
    static auto exclude = std::vector<std::size_t>();
    exclude.clear();

    auto get_random_node = [num_localities] () -> std::size_t {
        static std::random_device rd;
        static std::mt19937 rng(rd());
        static std::uniform_int_distribution<std::size_t> random_uid(0, num_localities-1);

        std::size_t ret = 0ul;
        std::vector<std::size_t>::const_iterator iter;

        do {
            ret = random_uid(rng);
            iter = std::find(exclude.begin(), exclude.end(), ret);
        } while ( iter != exclude.end() );

        return ret;
    };

    auto how_many_to_exclude = get_random_node();

    if ( how_many_to_exclude < num_localities ) {
        #ifdef TRULY_RANDOM_DEBUG
        std::cerr << "Will exclude " << how_many_to_exclude << " out of " << num_localities << std::endl;
        #endif
        #if 1
        for (auto i=0ul; i<how_many_to_exclude; ++i) {
            auto new_exclude = get_random_node();
            exclude.push_back(new_exclude);

            #ifdef TRULY_RANDOM_DEBUG
            std::cerr << "Excluded: " << new_exclude << std::endl;
            #endif
        }
        #else
        for ( auto i=num_localities-how_many_to_exclude; i<num_localities; ++i) {
            exclude.push_back(num_localities-i-1);

            #ifdef TRULY_RANDOM_DEBUG
            std::cerr << "Excluded: " << i << std::endl;
            #endif
        }
        #endif
    }

    u_steps_till_rebalance = u_balance_every;

    return hpx::lcos::broadcast<allscale_get_optimizer_state_action>(localities_)
        .then(
            [this, old_mapping, get_random_node](hpx::future<std::vector<optimizer_state> > future_state) {
                auto new_mapping = std::vector<std::size_t>(old_mapping.size(), 0ul);
                std::transform(new_mapping.begin(), new_mapping.end(), new_mapping.begin(),
                [get_random_node](std::size_t dummy) -> std::size_t {
                    return get_random_node();
                });

                #ifdef TRULY_RANDOM_DEBUG
                std::cerr << "New random schedule: (every " << u_balance_every << ") ";
                for ( const auto & wi:new_mapping )
                    std::cerr << wi << ' ';
                std::cerr << std::endl;
                #endif

                hpx::lcos::broadcast_apply<allscale_optimizer_update_policy_action_ino>(localities_, new_mapping);
            }
        );
}

hpx::future<void> global_optimizer::balance_ino(const std::vector<std::size_t> &old_mapping)
{
    /*VV: Compute the new ino_knobs (i.e. number of Nodes), then assign tasks to
          nodes and broadcast the scheduling information to all nodes.
    */
    u_steps_till_rebalance = u_balance_every;
    return hpx::lcos::broadcast<allscale_get_optimizer_state_action>(localities_)
        .then(
            [this, old_mapping](hpx::future<std::vector<optimizer_state> > future_state) {
                auto state = future_state.get();
                auto get_max_avg_time = [](float curr_max, const optimizer_state &s) -> float {
                    return std::max(curr_max, s.avg_time_);
                };

                float max_time = std::accumulate(state.begin()+1,
                                                state.end(),
                                                state.begin()->avg_time_,
                                                get_max_avg_time);

                if (max_time  < 0.1f ) {
                    max_time = 0.1f;
                }

                for (auto it = state.begin(); it != state.end(); ++it) {
                    if (it->avg_time_ < 0.1f )
                        it->avg_time_ = max_time;
                }

                #ifdef INO_DEBUG_DECIDE_SCHEDULE
                for (const auto &s : state)
                     std::cerr << "load:" << s.load_ << " time:" << s.avg_time_ << " freq:" << s.active_frequency_ << " cores:" << s.cores_per_node_ << std::endl;
                #endif

                std::map<std::size_t, std::vector<std::size_t> > node_map;
                std::vector<size_t> new_mapping(old_mapping.size());
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
                // FIXME: this is racey....
                std::size_t num_active_nodes = std::count(active_nodes_.begin(), active_nodes_.end(), true);
                for (idx=0ul; idx<=num_active_nodes; ++idx, ++node)
                {
                    node_loads[idx] = node->load_;
                    node_times[idx++] = node->avg_time_;

                    ino_loads.push_back(node->load_);
                    ino_times.push_back(node->avg_time_);
                    ino_energies.push_back((float)node->energy_);
                }

                auto knobs = o_ino.get_node_configuration(ino_loads,
                                                          ino_times,
                                                          ino_energies);

                num_active_nodes = knobs.u_nodes;

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
