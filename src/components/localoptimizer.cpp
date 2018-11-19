
#include <allscale/components/monitor.hpp>
#include <allscale/components/localoptimizer.hpp>

#include <algorithm>
#include <iterator>
#include <limits>
#include <math.h>
#include <sstream>
#include <stdlib.h>
#include <stdexcept>

#define DEBUG_ 1
//#define DEBUG_INIT_ 1 // define to generate output during scheduler initialization
#define DEBUG_MULTIOBJECTIVE_ 1
#define DEBUG_CONVERGENCE_ 1
//#define MEASURE_MANUAL 1 // define to generate output consumed by the regression test
#define MEASURE_ 1
// only meant to be defined if one needs to measure the efficacy
// of the scheduler
//#define ALLSCALE_HAVE_CPUFREQ 1

namespace allscale
{
namespace components
{

double localoptimizer::evaluate_score(const double objectives[])
{
	if ( mo_initialized ) {
		return nmd.evaluate_score(objectives, nullptr);
	}

	return -1.0;
}
void localoptimizer::setobjectives(double time_weight, 
								   double energy_weight, 
								   double resource_weight)
{
	this->time_weight = time_weight;
	this->energy_weight = energy_weight;
	this->resource_weight = resource_weight;

#ifdef ALLSCALE_HAVE_CPUFREQ
	setCurrentFrequencyIdx(0);
#endif

	// VV: Modifying the objectives triggers restarting the optimizer
	//     from scratch
	
	mo_initialized = false;
	converged_ = false;
}

void localoptimizer::reset(int threads, int freq_idx)
{
	threads_param_ = threads;
	thread_param_values_.clear();
#ifdef ALLSCALE_HAVE_CPUFREQ
	frequency_param_ = freq_idx;
	frequency_param_values_.clear();
#endif
	converged_ = false;
};

#ifdef DEBUG_
void localoptimizer::printobjectives()
{
	std::cout << "[LocalOptimizer|DEBUG] Weights=[time:" << time_weight
			  << ", energy:" << energy_weight
			  << ", resource:" << resource_weight << "]" << std::endl;
}
#endif

bool localoptimizer::isConverged()
{	
	#if 0
	// VV: This is an attempt to make optimization choices for 
	//     tasks of smaller granularity (after splitting a task)
	if ( converged_ == false ) {
		return false;
	}

	auto timestamp_now = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now()).time_since_epoch().count();

	if ( reexplore_every_ms >0 && timestamp_now - last_convergence_ts > reexplore_every_ms )
	{	
		std::cout << "[LOCALOPTIMIZER] Re-exploring space!" << std::endl;
		initialize_nmd();
	}
	#endif 
	return converged_; 
}

void localoptimizer::printverbosesteps(actuation act)
{
	static int last_frequency_idx = 0;

	std::cout << "[INFO]";
	if (optmethod_ == random)
		std::cout << "Random ";
	else if (optmethod_ == allscale)
	{
		std::cout << "Allscale ";
	}
	std::cout << "Scheduler Step: Setting OS Threads to " << threads_param_;
#ifdef ALLSCALE_HAVE_CPUFREQ
	if (act.frequency_idx >= 0)
		last_frequency_idx = act.frequency_idx;
	std::cout << " , CPU Frequency to " << frequencies_param_allowed_[last_frequency_idx]
			  << std::endl;
#else
	std::cout << std::endl;
#endif
}

void localoptimizer::accumulate_objective_measurements()
{
	if (pending_num_times)
	{
		pending_time /= (double)pending_num_times;
		pending_threads /= (double)(pending_num_times*threads_dt);
		pending_energy /= (double)pending_num_times;
		pending_num_times = 0;
	}
}

void localoptimizer::setmaxthreads(std::size_t threads)
{
	max_threads_=threads;
	threads_param_=threads;
	#if 0
	double threads_tick = threads / 5.;

	if ( threads_tick < 1.0 )
		threads_tick = 1.0;
	
	threads_dt = (int) round(threads_tick);
	#elif 0
	if ( max_threads_ <= 4 )
		threads_dt = 1.;
	else if ( max_threads_ <= 8 )
		threads_dt = 2.;
	else if ( max_threads_ <= 32 )
		threads_dt = 4.;
	else
		threads_dt = 8.;
	#else 
		threads_dt = 1.;
	#endif
}

#ifdef ALLSCALE_HAVE_CPUFREQ
void localoptimizer::initialize_nmd(bool from_scratch)
{
	// VV: Place constraints to #threads and cpu_freq tunable knobs

	double constraint_min[] = {1, 0};
	double constraint_max[] = {ceil(max_threads_/(double)threads_dt),
							   (double)frequencies_param_allowed_.size() - 1};
	const double opt_weights[] = { time_weight, energy_weight, resource_weight };

	if( from_scratch == false ){
		double prev_simplex[NMD_NUM_KNOBS+1][NMD_NUM_KNOBS];
	
		nmd.get_simplex(prev_simplex);

		nmd.initialize_simplex(opt_weights,
								prev_simplex,
								constraint_min, 
								constraint_max);
	} else {
		nmd.initialize_simplex(opt_weights,
								nullptr,
								constraint_min, 
								constraint_max);
	}

	mo_initialized = true;
	explore_knob_domain = true;
	converged_ = false;
}
#endif

void localoptimizer::measureObjective(double iter_time, double power, double threads)
{
	std::cout << "Measuring objective: "
			  << iter_time << " "
			  << power << " "
			  << threads << std::endl;

	pending_time += iter_time;
	pending_energy += power;
	pending_threads += threads;
	pending_num_times++;
}

void localoptimizer::reset_accumulated_measurements()
{
	pending_time = 0.;
	pending_energy = 0.;
	pending_threads = 0.;
	pending_num_times = 0;
}

actuation localoptimizer::step(std::size_t active_threads)
{
	actuation act;
	// VV: Possibly amend erroneous information
	threads_param_  = active_threads;
	act.threads = threads_param_;
#ifdef ALLSCALE_HAVE_CPUFREQ
	act.frequency_idx = frequency_param_;
#endif
	/* random optimization step */
	if (optmethod_ == random)
	{
		act.threads = (rand() % max_threads_);
#ifdef ALLSCALE_HAVE_CPUFREQ
		act.frequency_idx = rand() % frequencies_param_allowed_.size();
#endif
	}
#ifdef ALLSCALE_HAVE_CPUFREQ
	else if (optmethod_ == allscale)
	{
		// VV: Keep track of dirty objectives
		if (mo_initialized == false)
			initialize_nmd(true);
				
		accumulate_objective_measurements();
		const double latest_measurements[] = {pending_time, 
											pending_energy, 
											pending_threads};
		reset_accumulated_measurements();

		if ( explore_knob_domain ){
			optstepresult nmd_res = nmd.step(latest_measurements,
											 active_threads,
#ifdef ALLSCALE_HAVE_CPUFREQ
											 frequency_param_
#else
											0
#endif
											 );

#ifdef DEBUG_MULTIOBJECTIVE_
			std::cout << "[LOCALOPTIMIZER|DEBUG] New Vertex to try:";
			std::cout << " Threads = " << nmd_res.threads;
			std::cout << " Freq Idx = " << nmd_res.freq_idx << std::endl;
			std::cout << " Converge Thresh = " << convergence_threshold_ << std::endl;
#endif
			if (nmd_res.converged)
			{
				double min_score = nmd.getMinObjective();
				double *minimization_point = nmd.getMinVertices();

#ifdef DEBUG_CONVERGENCE_
				std::cout << "[LOCALOPTIMIZER|INFO] NMD convergence\n";
				std::cout << "******************************************" << std::endl;
				std::cout << "[LOCALOPTIMIZER|INFO] Minimal Objective Value = " << min_score << " Threads = " << minimization_point[0] << " Freq_idx = " << minimization_point[1] << std::endl;
				std::cout << "******************************************" << std::endl;
#endif
				act.threads = minimization_point[0];
				act.frequency_idx = minimization_point[1];
				// VV: Stop searching for new knob_set
				explore_knob_domain = false;
				converged_ = true;
			} else {
				// VV: Have not converged yet, keep exploring
				act.threads = nmd_res.threads;
				act.frequency_idx = nmd_res.freq_idx;
			}
			
			act.threads *= threads_dt;

			threads_param_ = act.threads;
#ifdef DEBUG_MULTIOBJECTIVE_
			std::cout << "[LOCALOPTIMIZER|DEBUG] ACTUAL Vertex to try:";
			std::cout << " Threads = " << act.threads;
			std::cout << " Freq Idx = " << act.frequency_idx << std::endl;
#endif
		}
	}
#endif // ALLSCALE_HAVE_CPUFREQ

validate_act:

	if (act.threads > max_threads_)
	{
		act.threads = max_threads_;
	}
	else if (act.threads < 1)
	{
		act.threads = getCurrentThreads();
	}
#ifdef ALLSCALE_HAVE_CPUFREQ
	// VV: If freq_idx is -1 then set it to last used frequency (frequency_param_)
	if (act.frequency_idx < 0)
		act.frequency_idx = frequency_param_;
	else if (act.frequency_idx > frequencies_param_allowed_.size() - 1)
		act.frequency_idx = frequencies_param_allowed_.size() - 1;
#endif
	
	threads_param_ = act.threads;
#ifdef ALLSCALE_HAVE_CPUFREQ
	frequency_param_ = act.frequency_idx;
#endif
	return act;
}
} // namespace components
} // namespace allscale
