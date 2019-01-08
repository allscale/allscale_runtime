
#ifndef ALLSCALE_TUNER_HPP
#define ALLSCALE_TUNER_HPP

#include <allscale/tuning_objective.hpp>
#include <allscale/components/nmd.hpp>

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

    struct nmd_optimizer : tuner
    {
        nmd_optimizer(std::size_t nodes_min, std::size_t nodes_max);
        components::NmdGeneric nmd;
        std::vector<std::size_t> avail_freqs;
        std::vector<std::size_t> best;
        bool converged;
        bool initialized;
        // VV: even though NmdGeneric supports arbitrary number of optimization parameters
        //     we're applying it to number of nodes and CPU frequency, it is trivial to 
        //     add number of threads
        std::size_t constraint_min[2], constraint_max[2];

        tuner_configuration next(tuner_configuration const& current_cfg, tuner_state const& current_state, tuning_objective) override;

        double previous_weights[3];
    };
}

#endif
