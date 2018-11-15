
#ifndef ALLSCALE_TUNER_HPP
#define ALLSCALE_TUNER_HPP

#include <allscale/tuning_objective.hpp>

#include <iostream>
#include <vector>

namespace allscale {
    // A configuration in the optimization space
    struct tuner_configuration
    {
        // the current selection of nodes being active
        std::vector<bool> node_mask;
        // the currently selected frequency active on all nodes
        std::uint64_t frequency;

        bool operator==(tuner_configuration const& other) const
        {
            return node_mask == other.node_mask && frequency == other.frequency;
        }

        bool operator!=(tuner_configuration const& other) const
        {
            return !(*this == other);
        }
    };
    std::ostream& operator<<(std::ostream& os, tuner_configuration const& cfg);

    struct tuner_state
    {
        float speed = 0.f;
        float efficiency = 0.f;
        float power = 0.f;
        float score = 0.f;
    };
    std::ostream& operator<<(std::ostream& os, tuner_state const& state);

    struct tuner
    {
        virtual ~tuner() {}

        virtual tuner_configuration next(tuner_configuration const& current_cfg, tuner_state const& current_state, tuning_objective) = 0;
    };

    struct simple_coordinate_descent : tuner
    {
        enum dimension
        {
            num_nodes = 0,
            frequency = 1
        };

        enum direction
        {
            up = 0,
            down = 1
        };


        dimension dim = num_nodes;
        direction dir = down;

        tuning_objective objective;

        tuner_configuration best;

        float best_score;

        simple_coordinate_descent(tuner_configuration const& cfg);

        tuner_configuration next(tuner_configuration const& current_cfg, tuner_state const& current_state, tuning_objective) override;

        void next_direction();
    };
}

#endif
