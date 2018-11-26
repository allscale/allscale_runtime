/*
Nelder Mead implementation for arbitrary number of knobs and number of objectives.

Developed explicitly for non-continuous search spaces.

Important information
---------------------

This implementation uses a cache coupled with the exploration-heuristic that is explained
bellow to refrain from evaluating the same set of knobs multiple times.

If NMD proposes to explore a knob-set that has been recently evaluated (i.e. there's a
non stale entry in the cache) the heuristic will instead propose the closest point that is
enclosed within the N-dimensional (where N = num_knobs) space near the knob set that NMD
initially proposed. The N-dimensional space takes a form of a square, Cube, Hypercube for
N=2, 3, 4. Each edge may be at most @max_distance_long (see generate_unique) for more info.

author: vasiliadis.vasilis@gmail.com
*/
#ifndef ALLSCALE_NMD_HEADER
#define ALLSCALE_NMD_HEADER
#include <cstddef>
#include <map>
#include <set>
#include <vector>
#include <cmath>

namespace allscale {
namespace components {

struct logistics {
    std::vector<double> objectives;
    std::vector<std::size_t> knobs;

    int64_t cache_ts, cache_dt;

    bool converged;
};

#define ALPHA 1.0   /* reflection coefficient */
#define BETA 0.5	/* contraction coefficient */
#define GAMMA 2.0   /* expansion coefficient */
#define DELTA 0.5   /* shrinking coefficient */

class NmdGeneric {
public:
    NmdGeneric();
    NmdGeneric(std::size_t num_knobs, std::size_t num_objectives,
               double conv_threshold, int64_t cache_expire_dt_ms,
               std::size_t max_iters);

    static double score_speed_efficiency_power(const double measurements[], const double weights[])
    {
        double ret = std::pow(measurements[0], weights[0]) *
                    std::pow(measurements[1], weights[1]) *
                    std::pow((1-measurements[2]), weights[2]);
        
        if ( std::isfinite(ret) == 0  || ret > 1.0 ) {
            ret = 1.0;
        }
        
        return 1.0 - ret;
    }

    void initialize(const std::size_t constraint_min[], const std::size_t constraint_max[],
                    const std::size_t *initial_config[], const double weights[],
                    double (*score_function)(const double[], const double []));

    void ensure_profile_consistency(std::size_t expected[], const std::size_t observed[]) const;

    void set_constraints_now(const std::size_t constraint_min[], 
                             const std::size_t constraint_max[]);

    double score(const double measurements[]) const;

    std::pair<std::vector<std::size_t>, bool> get_next(const double measurements[], 
                            const std::size_t observed_knobs[]);

protected:
    bool test_convergence();

    // VV: (measurements, weights) returns value in range [0.0, infinite)
    //     0.0 means perfect score (i.e. the larger the score, the worse it is)
    double (*score_function)(const double[], const double []);

    std::vector<std::size_t> do_warmup(const double measurements[], 
                            const std::size_t observed_knobs[]);
    std::vector<std::size_t> do_reflect(const double measurements[], 
                            const std::size_t observed_knobs[]);
    std::vector<std::size_t> do_expand(const double measurements[], 
                            const std::size_t observed_knobs[]);
    std::vector<std::size_t> do_contract_in(const double measurements[], 
                            const std::size_t observed_knobs[]);
    std::vector<std::size_t> do_contract_out(const double measurements[], 
                            const std::size_t observed_knobs[]);
    std::vector<std::size_t> do_shrink();
    std::vector<std::size_t> do_start(bool consult_cache);

    void sort_simplex(bool consult_cache=true);
    void compute_centroid();

    void generate_unique(std::size_t initial[], bool accept_stale,
                        const std::set<std::vector<std::size_t> > *extra) const;
    std::size_t compute_max_combinations() const;

    template<typename T>
    void apply_constraint(T knobs[]) const
    {
        for (auto i=0ul; i<num_knobs; ++i) {
            if ( knobs[i] < (T) constraint_min[i] )
                knobs[i] = constraint_min[i];
            if ( knobs[i] > (T) constraint_max[i] )
                knobs[i] = constraint_max[i];
        }
    }

    //VV: Used to generate all possible combinations of +-
    // from: https://stackoverflow.com/questions/4633584/
    template <typename Iter>
    bool next_binary(Iter begin, Iter end) const
    {
        while (begin != end)       // we're not done yet
        {
            --end;
            if ((*end & 1) == 0)   // even number is treated as zero
            {
                ++*end;            // increase to one
                return true;       // still more numbers to come
            }
            else                   // odd number is treated as one
            {
                --*end;            // decrease to zero and loop
            }
        }
        return false;              // that was the last number 
    }

    enum estate {warmup, start, reflect, expand, contract_in, contract_out, shrink};
    estate current_state;
    std::size_t warmup_step;

    double conv_threshold;
    std::size_t num_knobs;
    std::size_t num_objectives;

    double *scores;
    std::size_t **simplex, **initial_config;
    std::size_t *constraint_max, *constraint_min;
    std::size_t *point_reflect, *point_contract, *point_expand, *centroid;
    std::map< std::vector<std::size_t>, logistics> cache;
    int64_t cache_expire_dt_ms;
    double *weights;
    std::size_t times_reentered_start;
    double score_reflect, score_contract, score_expand;
    bool final_explore;
    std::size_t iteration, max_iters;
};

}
}

#endif