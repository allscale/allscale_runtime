
#ifndef ALLSCALE_OPTIMIZER_HPP
#define ALLSCALE_OPTIMIZER_HPP

#include <allscale/get_num_localities.hpp>

#include <hpx/lcos/future.hpp>
#include <hpx/traits/is_bitwise_serializable.hpp>

#include <iosfwd>
#include <vector>

namespace allscale {
	/**
	 * A class to model user-defined tuning objectives.
	 *
	 * Objectives are defined as
	 *
	 * 	score = t^m * e^n * (1-p)^k
	 *
	 * where t, e, p \in [0..1] is the current system speed, efficiency
	 * and power dissipation. The exponents m, n, and k can be user defined.
	 *
	 * The optimizer aims to maximize the overall score.
	 */
    struct tuning_objective
    {
        float speed_exponent;
        float efficiency_exponent;
        float power_exponent;

        tuning_objective()
          : speed_exponent(0.0f)
          , efficiency_exponent(0.0f)
          , power_exponent(0.0f)
        {}

        tuning_objective(float speed, float efficiency, float power)
          : speed_exponent(speed)
          , efficiency_exponent(efficiency)
          , power_exponent(power)
        {}

        static tuning_objective speed()
        {
            return {1.0f, 0.0f, 0.0f };
        }

        static tuning_objective efficiency()
        {
            return {0.0f, 1.0f, 0.0f };
        }

        static tuning_objective power()
        {
            return {0.0f, 1.0f, 0.0f };
        }

        float score(float speed, float efficiency, float power) const;

        friend std::ostream& operator<<(std::ostream& os, tuning_objective const&);
    };

    tuning_objective get_default_objective();

    struct optimizer_state
    {
        optimizer_state() : load(1.f), active_frequency(1000.f), cores_per_node(1)
        {}

        optimizer_state(float l, 
                        float avg_last_iterations_time, 
                        float freq,
                        std::size_t cores)
          : load(l)
          , avg_time(avg_last_iterations_time)
          , active_frequency(freq)
          , cores_per_node(cores)
        {}

        float load;
        float avg_time;
        float active_frequency;
        std::size_t cores_per_node;

        template <typename Archive>
        void serialize(Archive& ar, unsigned)
        {
            ar & load;
            ar & avg_time;
            ar & active_frequency;
            ar & cores_per_node;
        }
    };

    struct global_optimizer
    {
        global_optimizer();

        bool active() const
        {
            return active_;
        }

        void update(std::size_t num_active_nodes)
        {
            num_active_nodes_ = num_active_nodes;
        }

        hpx::future<void> balance(bool);

    private:
        void tune(std::vector<optimizer_state> const& state);

        std::size_t num_active_nodes_;
        float active_frequency_;

        using config = std::pair<std::size_t, float>;
        // Hill climbing data
        config best_;
        float best_score_;
        std::vector<config> explore_;

        // local state information
        bool active_;

        tuning_objective objective_;
        std::vector<hpx::id_type> localities_;
    };
}

HPX_IS_BITWISE_SERIALIZABLE(allscale::optimizer_state);

#endif
