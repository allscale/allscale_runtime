
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

namespace allscale
{
    optimizer_state get_optimizer_state()
    {
        float load = 1.f - monitor::get().get_idle_rate();
        return {load, monitor::get().get_task_times()};
    }

    void optimizer_update_policy(task_times const& times, std::vector<bool> mask)
    {
        scheduler::update_policy(times, mask);
    }
}

HPX_PLAIN_DIRECT_ACTION(allscale::get_optimizer_state, allscale_get_optimizer_state_action);
HPX_PLAIN_DIRECT_ACTION(allscale::optimizer_update_policy, allscale_optimizer_update_policy_action);

namespace allscale
{
    float tuning_objective::score(float speed, float efficiency, float power) const
    {
        return
            std::pow(speed, speed_exponent) *
            std::pow(efficiency, efficiency_exponent) *
            std::pow(1-power, power_exponent);
    }

    std::ostream& operator<<(std::ostream& os, tuning_objective const& obj)
    {
        std::cout
            << "t^" << obj.speed_exponent << " * "
            << "e^" << obj.efficiency_exponent << " * "
            << "p^" << obj.power_exponent;
        return os;
    }

    tuning_objective get_default_objective()
    {
        char* const c_obj = std::getenv("ALLSCALE_OBJECTIVE");
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
        for (char c: obj)
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
        return C * frequency * frequency * (U*U);
    }

    global_optimizer::global_optimizer()
      : num_active_nodes_(allscale::get_num_localities())
      , best_(0, 1.f)
      , best_score_(0.0f)
      , active_(true)
      , objective_(get_default_objective())
      , localities_(hpx::find_all_localities())
    {}

    void global_optimizer::tune(std::vector<float> const& state)
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
//             if (i < num_active_nodes_)
//             {
//                 used_cycles += state[i] * state[i].active_frequency * state[i].cores_per_node;
//                 avail_cycles += state[i].active_frequency * state[i].cores_per_node;
//                 used_power += estimate_power(state[i].active_frequency);
//                 active_frequency += state[i].active_frequency / num_active_nodes_;
//             }
//             max_cycles += max_frequency * state[i].cores_per_node;
            max_power = estimate_power(max_frequency);
        }


        float speed = used_cycles / float(max_cycles);
        float efficiency = used_cycles / float(avail_cycles);
        float power = used_power / max_power;

        float score = objective_.score(speed, efficiency, power);

        std::cerr << "\tSystem state:"
                <<  " spd=" << std::setprecision(2) << speed
                << ", eff=" << std::setprecision(2) << efficiency
                << ", pow=" << std::setprecision(2) << power
                << " => score: " << std::setprecision(2) << score
                << " for objective " << objective_ << "\n";

        // record current solution
        if (score > best_score_)
        {
            best_ = { num_active_nodes_, active_frequency };
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
            auto options = std::vector<float>(1.f);
#endif
            auto cur = std::find(options.begin(), options.end(), best_.second);
            HPX_ASSERT(cur != options.end());
            auto pos = cur - options.begin();

            std::vector<float> frequencies;
            if (pos != 0) frequencies.push_back(options[pos-1]);
            frequencies.push_back(options[pos]);
            if (pos+1<int(options.size())) frequencies.push_back(options[pos+1]);

            // get nearby node numbers
            std::vector<std::size_t> num_nodes;
            if (best_.first > 1) num_nodes.push_back(best_.first-1);
            num_nodes.push_back(best_.first);
            if (best_.first < allscale::get_num_localities()) num_nodes.push_back(best_.first+1);


            // create new options
            for(const auto& a : num_nodes) {
                for(const auto& b : frequencies) {
                    std::cerr << "\t\tAdding option " << a << " @ " << b << "\n";
                    explore_.push_back({a,b});
                }
            }

            // reset best options
            best_score_ = 0;
        }

        // if there are still no options, there is nothing to do
        if (explore_.empty()) return;

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
            localities_).then(
                [this, tuned](hpx::future<std::vector<optimizer_state>> future_state)
                {
                    auto state = future_state.get();
//                     std::vector<float> load;
//                     load.reserve(state.size());

                    // compute the load variance
                    float avg_load = 0.0f;
                    task_times times;

                    std::for_each(state.begin(), state.end(),
                        [&times, &avg_load](optimizer_state const& s)
                        {
//                             load.push_back(s.load_);
                            avg_load += s.load_;
                            times += s.task_times_;
                        });
                    avg_load = avg_load / num_active_nodes_;


                    float sum_dist = 0.f;
                    for(std::size_t i=0; i<num_active_nodes_; i++)
                    {
                        float dist = state[i].load_ - avg_load;
                        sum_dist +=  dist * dist;
                    }
                    float var = (num_active_nodes_ > 1) ? sum_dist / (num_active_nodes_ - 1) : 0.0f;

                    std::cerr
//                         << times << '\n'
                        << "Average load "      << std::setprecision(2) << avg_load
                        << ", load variance "   << std::setprecision(2) << var
//                         << ", average frequency "   << std::setprecision(2) << avg_state.active_frequency
                        << ", total progress " << std::setprecision(2) << (avg_load*num_active_nodes_)
                        << " on " << num_active_nodes_ << " nodes\n";
                    // -- Step 2: if stable, adjust number of nodes and clock speed

                    // if stable enough, allow meta-optimizer to manage load
                    if (tuned && var < 0.01f)
                    {
                        // adjust number of nodes and CPU frequency
//                         tune(state);
                    }
                    // -- Step 3: enforce selected number of nodes and clock speed, keep system balanced

                    // compute number of nodes to be used
                    std::vector<bool> mask(localities_.size());
                    for(std::size_t i=0; i<localities_.size(); i++) {
                        mask[i] = i < num_active_nodes_;
                    }

                    return hpx::lcos::broadcast<allscale_optimizer_update_policy_action>(
                        localities_, times, mask);

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
                }
            );
    }
}
