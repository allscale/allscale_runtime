
#ifndef ALLSCALE_COMPONENTS_LOCALOPTIMIZER_HPP
#define ALLSCALE_COMPONENTS_LOCALOPTIMIZER_HPP

#include <allscale/components/nmsimplex_bbincr.hpp>

#if defined(ALLSCALE_HAVE_CPUFREQ)
#include <allscale/util/hardware_reconf.hpp>
#endif

#include <deque>
#include <vector>
#include <stdlib.h>
#include <time.h>
#include <string>

//#define MEASURE_MANUAL_ 1
#define MEASURE_ 1
#define DEBUG_ 1
#define DEBUG_MULTIOBJECTIVE_ 1

namespace allscale
{
namespace components
{

enum objectiveType
{
	time,
	energy,
	resource
};

enum searchPolicy
{
	allscale,
	random,
	manual,
	none
};


/* structure type modelling an optimization actuation action to be taken
       by the scheduler */
struct actuation
{
	unsigned int threads;
	int frequency_idx;
};

struct localoptimizer
{
	localoptimizer();
	bool isConverged();
	double evaluate_score(const double objectives[]);
	void setPolicy(searchPolicy pol)
	{
		optmethod_ = pol;
#ifdef DEBUG_
		std::cout << "Local Optimizer Initialized with "
				  << policyToString(pol)
				  << " policy for multi-objective search."
				  << std::endl;
#endif
	}
	void initialize_nmd(bool from_scratch);
	searchPolicy getPolicy() { return optmethod_; }
	
	// VV: Modifying the objectives triggers restarting the optimizer
	void setobjectives(double time_weight, 
						double energy_weight, 
						double resource_weight);

	void getobjectives(double *time_weight, 
					   double *energy_weight,
					   double *resource_weight)
	{
		if ( time_weight != nullptr )
			*time_weight = this->time_weight;
		if ( energy_weight != nullptr )
			*energy_weight = this->energy_weight;
		if ( resource_weight != nullptr )
			*resource_weight = this->resource_weight;
	}

	void set_objectives_scale(const double objectives_scale[3]);

	std::size_t getCurrentThreads() { return threads_param_; }

	void setCurrentThreads(std::size_t threads) { threads_param_ = threads; }

	unsigned int getCurrentFrequencyIdx()
	{
		return frequency_param_;
	}

	void setCurrentFrequencyIdx(unsigned int idx) { frequency_param_ = idx; }

	const std::vector<unsigned long>
	setfrequencies(std::vector<unsigned long> frequencies)
	{
		#if 0
		const std::size_t max_freqs = 10;
		std::size_t keep_every = (std::size_t) ceilf(frequencies.size() / (float) max_freqs);

		if ( keep_every > 1 ) {
			std::vector<unsigned long> new_freqs;

			int i, j, len;

			for (j=0, i=0, len=frequencies.size(); i<len; ++i ) {
				if ( (i==len-1) || ( (i % keep_every) == 0 )) {
				new_freqs.push_back(frequencies[i]);
				}
			}      

			frequencies = new_freqs;
		}
		#endif

		frequencies_param_allowed_ = frequencies;
		//std::cout << "**************** = " << frequency_param_ << std::endl;
		//for(auto& el: frequencies_param_allowed_)
		//  std::cout << "***>>>> " << el << std::endl;
		return frequencies_param_allowed_;
	}

	std::size_t getmaxthreads()
	{
		return max_threads_;
	}

	void setmaxthreads(std::size_t threads);

	/* executes one step of multi-objective optimization */
	actuation step(std::size_t active_threads);

	/* adds a measurement sample to the specified objective */
	void measureObjective(double iter_time, double power, double threads);

	/* restarts multi-objective optimization from current best solution */
	void reset(int, int);

#ifdef DEBUG_
	void printobjectives();
	void printverbosesteps(actuation);
#endif

	std::string policyToString(searchPolicy pol)
	{
		std::string str;
		switch (pol)
		{
		case random:
			str = "random";
			break;
		case allscale:
			str = "allscale";
			break;
		case manual:
			str = "manual";
			break;
		}
		return str;
	}

  private:
	double time_weight, energy_weight, resource_weight;

	// VV: Used to convert thread_idx to actual number of threads
	std::size_t threads_dt;

	void accumulate_objective_measurements();
	void reset_accumulated_measurements();

	std::vector<double> samples_energy;
	std::vector<double> samples_time;
	std::vector<double> samples_threads;
	std::vector<double> samples_freq;

	bool explore_knob_domain;

	double pending_time, pending_energy, pending_threads;
	unsigned long pending_num_times;

	bool mo_initialized;

	NelderMead nmd;

	/* single objective optimization method used */
	searchPolicy optmethod_ = none;

	/* active optimization parameter - nr of OS threads active */
	int threads_param_;

	/* ordered set of OS thread values that have been assigned to the
           runtime by the optimization algorithm. The most recent value is
           stored at the end of the vector */
	std::vector<unsigned long> thread_param_values_;

	/* maximum number of OS threads supported by the runtime */
	std::size_t max_threads_;

	/* active optimization parameter - current CPU frequency index */
	unsigned int frequency_param_;

	/* vector containing sorted list of frequencies supported by the
           processor */
	std::vector<unsigned long> frequencies_param_allowed_;

	/* threshold (percentage in [0,1]) to decide convergence of optimization
           steps */
	double convergence_threshold_;

	/***** optimization state variables ******/

	/* initial warm-up steps */
	const unsigned int warmup_steps_ = 3;

	/* set to true if local optimizer has converged over all objectives */
	bool converged_;

	double objectives_scale[3];
};
} // namespace components
} // namespace allscale

#endif
