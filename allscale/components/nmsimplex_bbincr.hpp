/*
 * header for nmsimplex_bbincr.h
 * Author : Kostas Katrinis
 * kostas.katrinis@gmail.com
 * 
 * Extension of Michael F. Hutt implementation
 * of Nelder Mead Simplex search for black box,
 * incremental use (e.g. for sampled objective
 * function with complex analytical evaluation)
 *
 */

#ifndef NM_SIMPLEX_INCR_H
#define NM_SIMPLEX_INCR_H

//#include <stdio.h>
#include <iostream>
#include <stdlib.h>
#include <math.h>

#include <chrono>
#include <utility>
#include <map>

#ifdef MACOSX
#include <malloc/malloc.h>
#else
#include <malloc.h>
#endif

namespace allscale
{
namespace components
{

// VV: threads, freq_idx
#define NMD_NUM_KNOBS 2
// VV: time, energy/power, resources
#define NMD_NUM_OBJECTIVES 3


#if (NMD_NUM_OBJECTIVES != 3)
#error UNSUPPORTED number of Objectives
#endif

#if (NMD_NUM_KNOBS != 2)
#error UNSUPPORTED number of Knobs
#endif

#define MAX_IT 1000 /* maximum number of iterations */
#define ALPHA 1.0   /* reflection coefficient */
#define BETA 0.5	/* contraction coefficient */
#define GAMMA 2.0   /* expansion coefficient */
#define DELTA 0.5   /* shrinking coefficient */

#define CACHE_EXPIRE_AFTER_MS 35000

/* structure type of a single optimization step return status */
struct optstepresult
{
	/* true if optimization has converged for the specified objective */
	bool converged;
	/* number of threads for parameters to set for sampling */
	double threads;
	/* index to frequency vector for freq parameter to set for sampling*/
	int freq_idx;
	
	/******VV: Cache stuff******/
	double objectives[3]; // (time, energy, resource)
	// VV: _cache_expires denotes dt (in ms) after _cache_timestamp
	int64_t _cache_timestamp, _cache_expires_dt;
};

typedef std::map<std::pair<int, int>, optstepresult> MapCache_t;

/* enumeration encoding state that the incremental Nelder Mead optimizer is at */
enum iterationstates
{
	// VV: Need NMD_NUM_KNOBS + 1 values before we can start optimizing
	warmup,
	start,
	reflection,
	expansion,
	contraction,
	shrink
};


class NelderMead
{

  public:
	NelderMead(const NelderMead &other);
	NelderMead(double);
	// VV: For the time being 
	//     weights = [ W_time, W_energy/power, W_resources ]
	//     initial_simplex = double[NMD_NUM_KNOBS+1][NMD_NUM_KNOBS]
	//     constraint_min = [min_threads, min_freq_idx]
	void initialize_simplex(const double weights[NMD_NUM_OBJECTIVES],
							const double initial_simplex[][NMD_NUM_KNOBS],
							const double constraint_min[NMD_NUM_KNOBS],
							const double constraint_max[NMD_NUM_KNOBS]);
	
	void initialize_simplex(const double weights[NMD_NUM_OBJECTIVES],
							const double constraint_min[NMD_NUM_KNOBS],
							const double constraint_max[NMD_NUM_KNOBS]);

	void print_initial_simplex();
	void print_iteration();

	void set_scale(const double scale[NMD_NUM_OBJECTIVES]);

	double *getMinVertices()
	{
		return v[vs];
	}

	double getMinObjective()
	{
		return min;
	}

	// VV: Returns a [NMD_NUM_KNOS+1][NMD_NUM_KNOBS] array
	void get_simplex(double simplex[][NMD_NUM_KNOBS]) {
		for (auto i=0; i<NMD_NUM_KNOBS+1; ++i)
			for (auto j=0; j<NMD_NUM_KNOBS; ++j)
				simplex[i][j] = v[i][j];
	}

	unsigned long int getIterations() { return itr; }
	double evaluate_score(const double objectives[], const double *weights);
	void set_weights(const double weights[]);

	optstepresult step(const double objectives[], 
			double knob1, double knob2);

	void invalidate_cache();
	void reevaluate_scores();

  private:
	int warming_up_step;
	bool should_invalidate_cache, should_reevaluate_scores;
	double max_power_, max_time_;

	// VV: Utility to make sure that we generate new values and not something that already
	//     exists in the set of NMD_NUM_KNOBS+1 configuration points
	template <typename F>
	void generate_new(F &gen);
	enum direction {up, up_final, down, left, right, right_final};
	std::pair<int, direction> explore_next_extra(double *extra, int level, 
                        direction dir, int level_max, int level_nested_max);

	//VV: objective_type: { <threads, cpu-freq>: optstepresult }
	MapCache_t cache_;

	void do_invalidate_cache();
	void do_reevaluate_scores();

	optstepresult do_step_start();
	optstepresult do_step_reflect(const double objectives[], 
			double knob1, double knob2);
	optstepresult do_step_expand(const double objectives[], 
			double knob1, double knob2);
	optstepresult do_step_contract(const double objectives[], 
			double knob1, double knob2);
	optstepresult do_step_shrink(const double objectives[], 
			double knob1, double knob2);

	void sort_vertices(void);
	void my_constraints(double *);
	void centroid();
	bool testConvergence(std::size_t tested_combinations);

	// VV: Will return false if entry not in cache
	bool cache_update(int threads, int freq_idx,
					  const double objectives[],
					  bool add_if_new);

	bool convergence_reevaluating;
	int initial_configurations[NMD_NUM_KNOBS+1][NMD_NUM_KNOBS];
	double scale[NMD_NUM_OBJECTIVES];
	/* vertex with smallest value */
	int vs;

	/* vertex with next smallest value */
	int vh;

	/* vertex with largest value */
	int vg;

	int i, j, row;

	const int n = 2;

	/* track the number of function evaluations */
	int k;

	/* track the number of iterations */
	int itr;

	/* holds vertices of simplex */
	double v[NMD_NUM_KNOBS+1][NMD_NUM_KNOBS];

	/* value of function at each vertex */
	double f[NMD_NUM_KNOBS+1];

	/* value of function at reflection point */
	double fr;

	/* value of function at expansion point */
	double fe;

	/* value of function at contraction point */
	double fc;

	/* reflection - coordinates */
	double vr[NMD_NUM_KNOBS];

	/* expansion - coordinates */
	double ve[NMD_NUM_KNOBS];

	/* contraction - coordinates */
	double vc[NMD_NUM_KNOBS];

	/* centroid - coordinates */
	double vm[NMD_NUM_KNOBS];

	double min;

	double fsum, favg, s;

	double EPSILON;

	iterationstates state_;

	const int MAXITERATIONS = 15;

	double constraint_min[2];

	double constraint_max[2];

	double opt_weights[NMD_NUM_OBJECTIVES];
};

} // namespace components
} // namespace allscale
#endif
