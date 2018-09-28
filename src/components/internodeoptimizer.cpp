#include <cmath>
#include <iostream>
#include <algorithm>
#include <allscale/components/internodeoptimizer.hpp>

// https://stackoverflow.com/a/6943003
template <typename I>
I random_element(I begin, I end)
{
    const unsigned long n = std::distance(begin, end);

    assert((n > 0) && "Attempted to get random element from empty container");

    /*VV: RAND_MAX+1 might overflow*/
    const unsigned long divisor = (RAND_MAX) / n;

    unsigned long k;
    do
    {
        k = std::rand() / divisor;
    } while (k >= n);

    std::advance(begin, k);
    return begin;
}

namespace allscale
{
namespace components
{

internode_optimizer_t::internode_optimizer_t(std::size_t nodes,
                                           float target, float leeway,
                                           std::size_t reset_history_every)
    : c_last_choice({0}), u_nodes(nodes), u_choices(0),
      u_history_interval(reset_history_every), u_history_tick(0),
      d_target(target), d_leeway(leeway)
{
    u_min_nodes = ceil(d_target * u_nodes);
    u_max_nodes = ceil((d_target + d_leeway) * u_nodes);

    u_min_nodes = std::max(1ul, u_min_nodes);
    u_max_nodes = std::min(u_nodes, u_max_nodes);

    for (auto n = u_min_nodes; n < u_max_nodes + 1; ++n)
    {
        v_explore_logistics.push_back(exploration_t(n));
    }
}

ino_knobs_t 
internode_optimizer_t::get_node_configuration(const std::vector<float> &measure_load,
                                              const std::vector<float> &measure_time,
                                              const std::vector<float> &measure_energy,
                                              const ino_knobs_t *previous_decision)
{
    if (previous_decision == nullptr && u_choices > 0)
        previous_decision = &c_last_choice;

    if (previous_decision != nullptr)
    {
        record_point(*previous_decision,
                     // VV: measure_load could take into account cores per node
                     measure_load, measure_energy, measure_time);
    }

    u_history_tick++;

    try_forget();

    if ((u_choices < INO_EXPLORATION_PHASE_STEPS) || ((u_choices > INO_EXPLORATION_PHASE_STEPS)
#ifdef INO_RANDOM_EVERY
                                                      && ((u_choices - INO_EXPLORATION_PHASE_STEPS + 1) % (INO_RANDOM_EVERY + 1) == 0))
#endif
    )
    {
        // VV: Insert some randomness and observe its effects
        c_last_choice = get_next_explore();
    }
    else
    {
        c_last_choice = get_best();
    }

    u_choices++;
    return c_last_choice;
}

ino_knobs_t internode_optimizer_t::get_next_explore()
{
    if (s_explore.size() == 0)
        explore_configurations();

    if (s_explore.size() > 0)
    {
        auto it = random_element(s_explore.begin(), s_explore.end());
        auto ret = *it;

        s_explore.erase(it);

        return ret;
    }
    else
    {
        // VV: It's impossible to generate new exploration points, get one from the past
        auto it = random_element(v_past.begin(), v_past.end());
        return it->second.c_knobs;
    }
}

ino_knobs_t internode_optimizer_t::get_best()
{
    // VV: Computes the pareto frontier and returns one configuration from it

    // VV: Sort on load
    // sort should be maximized, time and energy minimized

    auto sorted = std::vector<pareto_entry_t>();
    auto pareto_frontier = std::set<pareto_entry_t>();

    auto pareto_eligible = [&pareto_frontier](const pareto_entry_t &entry,
                                              const ParetoEntryComparator &one,
                                              const ParetoEntryComparator &two) -> bool {
        bool candidate = true;
        auto it = pareto_frontier.rbegin();

        while (candidate && it != pareto_frontier.rend())
        {
            candidate = !one(*it, entry) || !two(*it, entry);
            ++it;
        }

        return candidate;
    };

    for (auto it : v_past)
    {
        sorted.push_back(it.second);
    }

    auto comparators = std::vector<ParetoEntryComparator>({pareto_entry_t::CompareParetoLoad,
                                                           pareto_entry_t::CompareParetoTime,
                                                           pareto_entry_t::CompareParetoEnergy});

    auto num_comparators = comparators.size();

    for (auto index = 0u; index < num_comparators && sorted.size(); ++index)
    {
        std::sort(sorted.begin(), sorted.end(), comparators[index]);
        // VV: By definition the best config is part of pareto
        pareto_frontier.insert(*sorted.begin());

        ParetoEntryComparator comp1 = comparators[(index + 1) % num_comparators];
        ParetoEntryComparator comp2 = comparators[(index + 1) % num_comparators];

        if (sorted.size())
        {
            auto it = sorted.begin();
            while (it != sorted.end())
            {
                auto dominate = pareto_eligible(*it, comp1, comp2);
                if (dominate)
                {
                    pareto_frontier.insert(*it);
                    // it = sorted.erase(it);
                    ++it;
                }
                else
                {
                    ++it;
                }
            }
        }
    }

#ifdef INO_DEBUG_PARETO
    for (auto it : pareto_frontier)
    {
        std::cout << "Pareto Frontier [" << it.c_knobs.u_nodes << "]:"
                  << " " << it.average_load()
                  << " " << it.average_time()
                  << " " << it.average_energy()
                  << std::endl;
    }
#endif
    return random_element(pareto_frontier.begin(), pareto_frontier.end())->c_knobs;
}

void internode_optimizer_t::record_point(const ino_knobs_t &knobs,
                                         const std::vector<float> &measure_load,
                                         const std::vector<float> &measure_time,
                                         const std::vector<float> &measure_energy)
{
    // static unsigned int cheat = 0;
    // cheat += 1u;
    // auto new_entry = pareto_entry_t(u_history_tick, knobs, measure_load, measure_time, measure_energy);
    // v_past.insert(std::pair<ino_knobs_t, pareto_entry_t>({cheat}, new_entry));

    auto it = v_past.find(knobs);
    if (it == v_past.end())
    {
        auto new_entry = pareto_entry_t(u_history_tick, knobs, measure_load, measure_time, measure_energy);
        v_past.insert(std::pair<ino_knobs_t, pareto_entry_t>(knobs, new_entry));
    }
    else
    {
        // VV: Already used this in the past, update it and record the new history tick
        it->second.u_at_tick = u_history_tick;
        it->second.v_load = std::vector<float>(measure_load);
        it->second.v_time = std::vector<float>(measure_time);
        it->second.v_energy = std::vector<float>(measure_energy);
    }
}

void internode_optimizer_t::explore_configurations()
{
    if (s_explore.size() >= INO_EXPLORATION_SOFT_LIMIT)
        return;

    auto may_find_new = v_explore_logistics.size() > 0 ? true : false;
    auto remaining = INO_EXPLORATION_SOFT_LIMIT - s_explore.size();

    // VV: Generate novel ino_knobs_t
    while (may_find_new && remaining > 0)
    {
        /* TODO VV: Perhaps it'd be nice to limit the number of 'novel' exploration
                    points so that we can intermittently visit past configurations */
        may_find_new = false;
        /* VV: Generate configurations in a round-robin fashion from each of the
               non-completed knob-exploration generators*/
        for (auto it = v_explore_logistics.begin(); it != v_explore_logistics.end(); ++it)
        {
            if (it->b_explored == false)
            {
                s_explore.insert(it->get_next());
                may_find_new |= !(it->b_explored);

                if (--remaining == 0)
                    break;
            }
        }
    }

    // VV: If there're any spots left, fill them in with forgotten knobs
    if (remaining)
    {
        auto available = s_forgotten.size();
        if (available)
        {
            auto use = std::min(available, remaining);
            remaining -= use;

            for (auto it = s_forgotten.begin(); use > 0; ++it, --use)
                s_explore.insert(*it);
        }
    }

    // VV: If there're still empty spots pick some of the non-forgotten past knobs
    if (remaining)
    {
        auto available = v_past.size();
        if (available)
        {
            auto use = std::min(available, remaining);

            for (auto it = v_past.begin(); use > 0; ++it, --use)
                s_explore.insert(it->second.c_knobs);
        }
    }
}

void internode_optimizer_t::try_forget()
{
    auto it = v_past.begin();

    while (it != v_past.end())
    {
        if (u_history_tick - it->second.u_at_tick > u_history_interval)
        {
            s_forgotten.insert(it->second.c_knobs);
            it = v_past.erase(it);
            continue;
        }
        ++it;
    }
}

} // namespace components
} // namespace allscale
