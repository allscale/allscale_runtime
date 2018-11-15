
#include <allscale/tuner.hpp>
#include <allscale/monitor.hpp>
#include <allscale/components/monitor.hpp>
#include <allscale/utils/printer/vectors.h>
#include <allscale/utils/optional.h>

namespace allscale {
    std::ostream& operator<<(std::ostream& os, tuner_configuration const& cfg)
    {
        os << cfg.node_mask << '@' << cfg.frequency / 1000. << "GHz";
        return os;
    }

    std::ostream& operator<<(std::ostream& os, tuner_state const& state)
    {
        os  << "System State: "
            << "  speed=" << state.speed
            << ", effic=" << state.efficiency
            << ", power=" << state.power
            << " ==> score=" << state.score
            ;
        return os;
    }

    simple_coordinate_descent::simple_coordinate_descent(tuner_configuration const& cfg)
      : objective(get_default_objective())
      , best(cfg)
    {}

    namespace
    {
        allscale::utils::optional<std::vector<bool>> inc(std::vector<bool> const& mask)
        {
            std::size_t num_active_nodes = std::count(mask.begin(), mask.end(), true);
            if (num_active_nodes == mask.size())
            {
                return {};
            }
            std::vector<bool> new_mask(mask);
            for (std::size_t i = 0; i != new_mask.size(); ++i)
            {
                if (!new_mask[i])
                {
                    new_mask[i] = true;
                    return new_mask;
                }
            }
            return new_mask;
        }
        allscale::utils::optional<std::vector<bool>> dec(std::vector<bool> const& mask)
        {
            std::size_t num_active_nodes = std::count(mask.begin(), mask.end(), true);
            if (num_active_nodes == 1)
            {
                return {};
            }
            std::vector<bool> new_mask(mask);
            std::size_t size = new_mask.size();
            for (std::size_t i = 0; i != size; ++i)
            {
                if (new_mask[size - i -1])
                {
                    new_mask[size - i - 1] = false;
                    return new_mask;
                }
            }
            return new_mask;
        }

        allscale::utils::optional<std::uint64_t> inc(std::uint64_t freq)
        {
            auto freqs = monitor::get().get_available_freqs(0);
            auto cur = std::find(freqs.begin(), freqs.end(), freq);
            assert_true(cur != freqs.end());
            auto pos = cur - freqs.begin();

            if(freqs.begin() > freqs.end())
            {
                if (pos == 0)
                {
                    return {};
                }
                else
                {
                    return freqs[pos-1];
                }
            }
            else
            {
                if (pos == freqs.size() - 1)
                {
                    return {};
                }
                else
                {
                    return freqs[pos+1];
                }
            }
        }

        allscale::utils::optional<std::uint64_t> dec(std::uint64_t freq)
        {
            auto freqs = monitor::get().get_available_freqs(0);
            auto cur = std::find(freqs.begin(), freqs.end(), freq);
            assert_true(cur != freqs.end());
            auto pos = cur - freqs.begin();

            if(freqs.begin() > freqs.end())
            {
                if (pos == freqs.size() - 1)
                {
                    return {};
                }
                else
                {
                    return freqs[pos+1];
                }
            }
            else
            {
                if (pos == 0)
                {
                    return {};
                }
                else
                {
                    return freqs[pos-1];
                }
            }
        }
    }

    tuner_configuration simple_coordinate_descent::next(tuner_configuration const& current_cfg, tuner_state const& current_state, tuning_objective obj)
    {
        // test if objective has changed
        if (obj != objective)
        {
            // forget solution so far
            best_score = -1.f;
            // update objective
            objective = obj;
        }

        // make sure there is configuration space
        assert_true(inc(current_cfg.node_mask) || dec(current_cfg.node_mask) || inc(current_cfg.frequency) || dec(current_cfg.frequency));

        // decide whether this causes a change in the direction space
        if (current_state.score < best_score)
        {
            next_direction();
        }

        // remember the best score
        if (current_state.score > best_score)
        {
            best_score = current_state.score;
            best = current_cfg;
        }

        // compute next configuration
        tuner_configuration res = best;

        std::cerr << "\tCurrent state: " << current_cfg << " with score " << current_state.score << '\n';
        std::cerr << "\tCurrent best: " << best << " with score " << best_score << '\n';

        while (true)
        {
            if (dim == num_nodes)
            {
                if(allscale::utils::optional<std::vector<bool>> n = (dir == up) ? inc(res.node_mask) : dec(res.node_mask))
                {
                    res.node_mask = std::move(*n);
                    return res;
                }
            }
            else
            {
                if(allscale::utils::optional<std::uint64_t> n = (dir == up) ? inc(res.frequency) : dec(res.frequency))
                {
                    res.frequency = *n;
                    return res;
                }

            }
            next_direction();
        }

        // done.
        return res;
    }

    void simple_coordinate_descent::next_direction()
    {
        // swap direction
        dir = direction(1-dir);

        // swap dimension if necessary
        if (dir == up)
        {
            dim = dimension(1-dim);
        }

        // print a status message
        std::cerr << "New search direction: " << (dim == num_nodes ? "#nodes" : "frequency") << " " << (dir == up ? "up" : "down") << "\n";
    }
}
