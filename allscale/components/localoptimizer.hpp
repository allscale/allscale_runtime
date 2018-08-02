
#ifndef ALLSCALE_COMPONENTS_LOCALOPTIMIZER_HPP
#define ALLSCALE_COMPONENTS_LOCALOPTIMIZER_HPP

#include <allscale/components/nmsimplex_bbincr.hpp>
#include <allscale/components/scheduler_network.hpp>
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
//#define DEBUG_ 1

namespace allscale { namespace components {

    enum objectiveType {time,energy,resource};

    enum parameterType {thread, frequency};

    enum searchPolicy {allscale, random, manual};

    /* structure type of a single optimization objective */
    struct objective{
      objectiveType type;
      /* leeway threshold desired, 0-1 double */
      double leeway;
      /* non-negative integer priority of the objective, 0 is highest priority*/
      int priority;
      /* local minimum during single objective optimization */
      double localmin;
      /* local maximum during single objective optimization */
      double localmax;
      /* local minimum during single objective optimization */
      double globalmin;
      /* local minimum during single objective optimization */
      double globalmax;
      /* current deviation of the objective value from observed min */
      double currentthreshold;
      /* sampled objective values throughout execution */
      std::vector<double> samples;
      /* thread number that lead to the objective value in samples vector */
      std::vector<double> threads_samples;
      /* frequency index that lead to the objective value in samples vector */
      std::vector<double> freq_samples;
      /* true if optimization of objective has converged, false otherwise */
      bool converged;
      /* true if optimizer for objective has been initialized, false otherwise */
      bool initialized;
      /* index to the parameter vectors for setup that has so far achieved
         the minimum over all samples */
      long int min_params_idx;
      double converged_minimum;
      double minimization_params[2];
    };


    /* structure type modelling an optimization actuation action to be taken
       by the scheduler */
    struct actuation{
       /* number of threads to resume (>0) or suspend (<0). If set to zero,
          number of threads will stay unchanged. */
       int delta_threads;

#if defined(ALLSCALE_HAVE_CPUFREQ)
        /* index to the global cpu-supported frequencies vector pointing to
           the new frequency to be set. If set to -1, frequency will stay
           unchanged */
       int frequency_idx;
#endif
    };

    struct localoptimizer
    {
        localoptimizer()
            :current_objective_idx_(0),nmd(0.01),
#if defined(ALLSCALE_HAVE_CPUFREQ)
            frequency_param_(0),
#endif
            converged_(false){
            if (optmethod_==random)
                srand (std::time(NULL));
            }

        localoptimizer(std::list<objective>);

        bool isConverged(){return converged_;}

        void setPolicy(searchPolicy pol){
          optmethod_ = pol;
#ifdef DEBUG_
          std::cout << "Local Optimizer Initialized with "
                    << policyToString(pol)
                    << " policy for single objective search."
                    << std::endl;
#endif
        }

        searchPolicy getPolicy(){return optmethod_;}

        void setobjectives(std::list<objective>);

        std::size_t getCurrentThreads(){return threads_param_;}

        void setCurrentThreads(std::size_t threads){threads_param_ = threads;}

#if defined(ALLSCALE_HAVE_CPUFREQ)
        unsigned int getCurrentFrequencyIdx(){return frequency_param_;}

        void setCurrentFrequencyIdx(unsigned int idx){frequency_param_ = idx;}

        const std::vector<unsigned long>
        setfrequencies(std::vector<unsigned long> frequencies){
            frequencies_param_allowed_=frequencies;
            //std::cout << "**************** = " << frequency_param_ << std::endl;
            //for(auto& el: frequencies_param_allowed_)
            //  std::cout << "***>>>> " << el << std::endl;
            return frequencies_param_allowed_;
        }
#endif

        void setmaxthreads(std::size_t threads){
            max_threads_=threads;
            threads_param_=threads;
        }

        /* executes one step of multi-objective optimization */
        actuation step();

        /* adds a measurement sample to the specified objective */
        void measureObjective(double iter_time, double power, double threads);

        /* restarts multi-objective optimization from current best solution */
        void reset(int,int);

#ifdef DEBUG_
        void printobjectives();
        void printverbosesteps(actuation);
#endif

        std::string policyToString(searchPolicy pol){
          std::string str;
          switch (pol){
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

        /* vector of active optimization objectives. Objectives are stored
           in the vector in decreasing priority order */
        std::vector<objective> objectives_;

        NelderMead nmd;

        /* counts number of parameter changes (as pair) */
        unsigned long long int param_changes_;

        /* single objective optimization method used */
        searchPolicy optmethod_ = random;

        /* active optimization parameter - nr of OS threads active */
        int threads_param_;

        /* ordered set of OS thread values that have been assigned to the
           runtime by the optimization algorithm. The most recent value is
           stored at the end of the vector */
        std::vector<unsigned long> thread_param_values_;

        /* maximum number of OS threads supported by the runtime */
        std::size_t max_threads_;

#if defined(ALLSCALE_HAVE_CPUFREQ)
        /* active optimization parameter - current CPU frequency index */
        unsigned int frequency_param_;

        /* ordered set of frequency values that the CPU has been set to by
           the optimization algorithm. The most recent value is stored at the
           end of the vector */
        std::vector<unsigned long> frequency_param_values_;

        /* vector containing sorted list of frequencies supported by the
           processor */
        std::vector<unsigned long> frequencies_param_allowed_;

        /* index to the vector of allowed frequencies that points to the highest
           frequency. The ordering of the vector, as reported by hardware
           reconfiguration can be platform specific, and therefore we need this
           index to make sorted access to the vector platform agnostic */
        const short unsigned int highest_frequency_allowed_idx_ = 0;
#endif

        /* threshold (percentage in [0,1]) to decide convergence of optimization
           steps against a single objective */
        const double convergence_threshold_ = 0.02;

        /***** optimization state variables ******/

        /* index to the _objectives vector of currently optimized objective */
        short int current_objective_idx_;

        /* number of times the optimizer step() has been invoked, this is for
           init and housekeeping purposes */
         unsigned long long int steps_;

        /* currently optimized parameter */
        parameterType current_param_;

        /* initial warm-up steps */
        const int warmup_steps_=3;

        /* maximum number of optimization steps allowed */
        const int max_steps_=100;

        /* set to true if local optimizer has converged over all objectives */
        bool converged_;
    };
}
}

#endif
