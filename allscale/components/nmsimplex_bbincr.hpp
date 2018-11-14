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
	double score;
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
	NelderMead(double);
	// VV: For the time being 
	//     weights = [ W_time, W_energy/power, W_resources ]
	//     constraint_min = [min_threads, min_freq_idx]
	void initialize_simplex(double weights[NMD_NUM_OBJECTIVES],
							double constraint_min[NMD_NUM_KNOBS],
							double constraint_max[NMD_NUM_KNOBS]);

	void print_initial_simplex();
	void print_iteration();

	double *getMinVertices()
	{
		return v[vs];
	}

	double getMinObjective()
	{
		return min;
	}

	unsigned long int getIterations() { return itr; }
	double evaluate_score(const double objectives[], const double *weights) const;
	void set_weights(double weights[]);

	optstepresult step(const double objectives[]);

  private:
	int warming_up_step;

	// VV: Utility to make sure that we generate new values and not something that already
	//     exists in the set of NMD_NUM_KNOBS+1 configuration points
	template <typename F>
	void generate_new(F &gen);
	enum direction {up, up_final, down, left, right, right_final};
	std::pair<int, direction> explore_next_extra(double *extra, int level, 
                        direction dir, int level_max, int level_nested_max);

	//VV: objective_type: { <threads, cpu-freq>: optstepresult }
	MapCache_t cache_;

	optstepresult do_step_start();
	optstepresult do_step_reflect(const double objectives[]);
	optstepresult do_step_expand(const double objectives[]);
	optstepresult do_step_contract(const double objectives[]);
	optstepresult do_step_shrink(const double objectives[]);

	void sort_vertices(void);
	void my_constraints(double *);
	void centroid();
	bool testConvergence(std::size_t tested_combinations);

	// VV: Will return false if entry not in cache
	bool cache_update(int threads, int freq_idx,
					  const double objectives[],
					  bool add_if_new);

	double round2(double num, int precision)
	{
		double rnum = 0.0;
		int tnum;

		if (num == 0.0)
			return num;

		rnum = num * pow(10, precision);
		tnum = (int)(rnum < 0 ? rnum - 0.5 : rnum + 0.5);
		rnum = tnum / pow(10, precision);

		return rnum;
	}

	bool convergence_reevaluating;
	int initial_configurations[NMD_NUM_KNOBS+1][NMD_NUM_KNOBS];
	
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
	double **v;

	/* value of function at each vertex */
	double *f;

	/* value of function at reflection point */
	double fr;

	/* value of function at expansion point */
	double fe;

	/* value of function at contraction point */
	double fc;

	/* reflection - coordinates */
	double *vr;

	/* expansion - coordinates */
	double *ve;

	/* contraction - coordinates */
	double *vc;

	/* centroid - coordinates */
	double *vm;

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
