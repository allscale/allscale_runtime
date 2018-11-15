
#ifndef ALLSCALE_TUNING_OBJECTIVE_HPP
#define ALLSCALE_TUNING_OBJECTIVE_HPP

#include <hpx/util/tuple.hpp>

#include <iostream>

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

        bool operator==(tuning_objective const& other) const
        {
            return hpx::util::tie(speed_exponent, efficiency_exponent, power_exponent)
                == hpx::util::tie(other.speed_exponent, other.efficiency_exponent, other.power_exponent);
        }

        bool operator!=(tuning_objective const& other) const
        {
            return !(*this == other);
        }

        float score(float speed, float efficiency, float power) const;

        friend std::ostream& operator<<(std::ostream& os, tuning_objective const&);
    };

    tuning_objective get_default_objective();
}

#endif
