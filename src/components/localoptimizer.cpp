
#include <allscale/components/monitor.hpp>
#include <allscale/components/localoptimizer.hpp>

#include <algorithm>
#include <iterator>
#include <limits>
#include <math.h>
#include <sstream>
#include <stdlib.h>
#include <stdexcept>

//#define DEBUG_ 1
//#define DEBUG_INIT_ 1 // define to generate output during scheduler initialization
//#define DEBUG_MULTIOBJECTIVE_ 1
//#define DEBUG_CONVERGENCE_ 1
//#define MEASURE_MANUAL 1 // define to generate output consumed by the regression test
#define MEASURE_ 1
// only meant to be defined if one needs to measure the efficacy
// of the scheduler
//#define ALLSCALE_HAVE_CPUFREQ 1
#define ALLSCALE_USE_CORE_OFFLINING 1

namespace allscale {
namespace components {

localoptimizer::localoptimizer(std::list<objective> targetobjectives)
  : objectives_((int)targetobjectives.size()),
    nmd(0.01),
    param_changes_(0),
    steps_(0),
    current_param_(thread),
    converged_(false)
  {
    for (objective o : targetobjectives) {
      //std::cout << o.type << "," << o.leeway << "," << o.priority << '\n';
      objectives_[o.priority] = o;
      objectives_[o.priority].localmin=10000;
      objectives_[o.priority].globalmin=10000;
      objectives_[o.priority].localmax=0.0;
      objectives_[o.priority].globalmax=0.0;
      objectives_[o.priority].converged=false;
      objectives_[o.priority].initialized=false;
      objectives_[o.priority].min_params_idx=0;
      objectives_[o.priority].converged_minimum=0;
    }
#ifdef ALLSCALE_HAVE_CPUFREQ
    setCurrentFrequencyIdx(0);
#endif
};

void localoptimizer::setobjectives(std::list<objective> targetobjectives){
  objectives_.clear();
  objectives_.resize((int)targetobjectives.size());
  for (objective o : targetobjectives) {
    //std::cout << o.type << "," << o.leeway << "," << o.priority << '\n';
    objectives_[o.priority] = o;
    objectives_[o.priority].localmin=10000;
    objectives_[o.priority].globalmin=10000;
    objectives_[o.priority].localmax=0.0;
    objectives_[o.priority].globalmax=0.0;
    objectives_[o.priority].converged=false;
    objectives_[o.priority].initialized=false;
    objectives_[o.priority].min_params_idx=0;
    objectives_[o.priority].converged_minimum=0;
  }
  steps_=0;
  param_changes_=0;
  current_param_=thread;
#ifdef ALLSCALE_HAVE_CPUFREQ
  setCurrentFrequencyIdx(0);
#endif
  converged_=false;
}

void localoptimizer::reset(int threads, int freq_idx){
  threads_param_ = threads;
  param_changes_=0;
  thread_param_values_.clear();
#ifdef ALLSCALE_HAVE_CPUFREQ
  frequency_param_= freq_idx;
  frequency_param_values_.clear();
#endif
  current_objective_idx_=0;
  steps_=0;
  current_param_=thread;
  converged_=false;
};

#ifdef DEBUG_
void localoptimizer::printobjectives(){
  for(auto& el: objectives_){
    std::cout << "Objective" << "\t\t" << "Priority" << "\t\t" << "Leeway" <<
    std::endl;
    switch (el.type){
      case time:
        std::cout << "Time" << "\t\t" << el.priority << "\t\t" << el.leeway <<
        std::endl;
        break;
      case energy:
        std::cout << "Energy" << "\t\t" << el.priority << "\t\t" << el.leeway <<
        std::endl;
        break;
      case resource:
        std::cout << "Resource" << "\t\t" << el.priority << "\t\t" << el.leeway <<
        std::endl;
        break;
    }
  }
}

void localoptimizer::printverbosesteps(actuation act){
  std::cout << "[INFO]";
  if (optmethod_==random)
    std::cout << "Random ";
  else if (optmethod_==allscale){
    std::cout << "Allscale ";
  }
  std::cout << "Scheduler Step: Setting OS Threads to " << threads_param_;
#ifdef ALLSCALE_HAVE_CPUFREQ
  std::cout << ", CPU Frequency to " << frequencies_param_allowed_[act.frequency_idx]
    << std::endl;
#else
  std::cout << std::endl;
#endif

}

#endif

void localoptimizer::measureObjective(double iter_time, double power, double threads){
  for(auto& el: objectives_){
    switch (el.type){
      case time:
        el.samples.insert(el.samples.begin(),iter_time);
        if (el.samples.size()>1000)
          el.samples.resize(500);

        el.threads_samples.insert(el.threads_samples.begin(),threads);
        if (el.threads_samples.size()>1000)
          el.threads_samples.resize(500);

#ifdef ALLSCALE_HAVE_CPUFREQ
        el.freq_samples.insert(el.freq_samples.begin(),getCurrentFrequencyIdx());
        if (el.freq_samples.size()>1000)
          el.freq_samples.resize(500);
#endif

        if (el.globalmin > iter_time){
          el.globalmin = iter_time;
          el.min_params_idx=param_changes_;
        }
        if (el.globalmax < iter_time)
          el.globalmax = iter_time;
#ifdef DEBUG__
        std::cout << "Iteration Time Minimum: " << el.globalmin << std::endl;
        std::cout << "Iteration Time Maximum: " << el.globalmax << std::endl;
        std::cout << "Iteration Time Samples: ";
        for(auto& samp: el.samples)
          std::cout << samp << ",";
        std::cout << std::endl;
#endif
        break;
      case energy:
        el.samples.insert(el.samples.begin(),power);
        if (el.samples.size()>1000)
          el.samples.resize(500);

        el.threads_samples.insert(el.threads_samples.begin(),threads);
        if (el.threads_samples.size()>1000)
          el.threads_samples.resize(500);

#ifdef ALLSCALE_HAVE_CPUFREQ
        el.freq_samples.insert(el.freq_samples.begin(),getCurrentFrequencyIdx());
        if (el.freq_samples.size()>1000)
          el.freq_samples.resize(500);
#endif

        if (el.globalmin > power){
          el.globalmin = power;
          el.min_params_idx=param_changes_;
        }
        if (el.globalmax < power)
          el.globalmax = power;
#ifdef DEBUG__
        std::cout << "Power Consumption Minimum: " << el.globalmin << std::endl;
        std::cout << "Power Consumption Maximum: " << el.globalmax << std::endl;
        std::cout << "Power Consumption Samples: ";
        for(auto& samp: el.samples)
          std::cout << samp << ",";
        std::cout << std::endl;
#endif
        break;
      case resource:
        el.samples.insert(el.samples.begin(),threads);
        if (el.samples.size()>1000)
          el.samples.resize(500);

        el.threads_samples.insert(el.threads_samples.begin(),threads);
        if (el.threads_samples.size()>1000)
          el.threads_samples.resize(500);

#ifdef ALLSCALE_HAVE_CPUFREQ
        el.freq_samples.insert(el.freq_samples.begin(),getCurrentFrequencyIdx());
        if (el.freq_samples.size()>1000)
          el.freq_samples.resize(500);
#endif

        if (el.globalmin > threads){
          el.globalmin = threads;
          el.min_params_idx=param_changes_;
        }
        if (el.globalmax < threads)
          el.globalmax = threads;
#ifdef DEBUG__
        std::cout << "Threads Minimum: " << el.globalmin << std::endl;
        std::cout << "Threads Maximum: " << el.globalmax << std::endl;
        std::cout << "Threads Samples: ";
        for(auto& samp: el.samples)
          std::cout << samp << ",";
        std::cout << std::endl;
#endif
        break;
    }
  }
}

actuation localoptimizer::step()
{
    steps_++;
    actuation act;
    act.delta_threads=threads_param_;
#ifdef ALLSCALE_HAVE_CPUFREQ
    act.frequency_idx=frequency_param_;
#endif
    /* random optimization step */
    if (optmethod_ == random)
    {
        act.delta_threads = (rand() % max_threads_) - threads_param_;
#ifdef ALLSCALE_HAVE_CPUFREQ
        act.frequency_idx = rand() % frequencies_param_allowed_.size();
        if (act.frequency_idx == frequency_param_)
            act.frequency_idx = -1;
#endif
    }

    else if (optmethod_ == allscale)
    {
        if (current_objective_idx_ > objectives_.size())
  	    	return act;

        if (steps_ < warmup_steps_)
        {

#ifdef DEBUG_MULTIOBJECTIVE_
            std::cout << "[LOCALOPTIMIZER|INFO] Optimizer No-OP: either at warm-up or optimizer has completed\n";
#endif
            // set some random parametrization to collect at least 3 different
            // vertices to be used as input to the optimizer
    	    act.delta_threads = rand() % max_threads_;
#ifdef ALLSCALE_HAVE_CPUFREQ
    	    act.frequency_idx = rand() % frequencies_param_allowed_.size();
#endif
            return act;
        }

        // iterate over all objectives in decreasing priority
        objective obj = objectives_[current_objective_idx_];

        // initialize optimizer for this objective, if not already done so
        if (!obj.initialized)
        {
#ifdef DEBUG_MULTIOBJECTIVE_
            std::cout << "[LOCALOPTIMIZER|INFO] Initializing optimizer for new objective\n";
	        std::cout << "[LOCALOPTIMIZER|DEBUG] Samples: " << std::flush;
	        for (auto& sam: obj.samples)
            {
	            std::cout << sam << "," << std::flush;
	        }
            std::cout << "\n" << std::flush;

            std::cout << "[LOCALOPTIMIZER|DEBUG] Thread Param of Samples: " << std::flush;
            for (auto& sam: obj.threads_samples)
            {
                std::cout << sam << "," << std::flush;
            }
            std::cout << "\n" << std::flush;

#ifdef ALLSCALE_HAVE_CPUFREQ
            std::cout << "[LOCALOPTIMIZER|DEBUG] Freq Param of Samples: " << std::flush;
            for (auto& sam: obj.freq_samples){
                std::cout << sam << "," << std::flush;
            }
            std::cout << "\n" << std::flush;
#endif
#endif
            int samplenr = obj.samples.size();
#ifdef ALLSCALE_HAVE_CPUFREQ
            double params[3][2]={
                {obj.threads_samples[samplenr-1],obj.freq_samples[samplenr-1]},
                {obj.threads_samples[samplenr-2],obj.freq_samples[samplenr-2]},
                {obj.threads_samples[samplenr-3],obj.freq_samples[samplenr-3]},
            };
            double values[3]={obj.samples[samplenr-1],obj.samples[samplenr-2],obj.samples[samplenr-3]};

            double constraint_min[]={1,0};
            double constraint_max[]={(double)max_threads_,
                (double)frequencies_param_allowed_.size()};

            nmd.initialize_simplex(params,values,constraint_min,constraint_max);
            objectives_[current_objective_idx_].initialized=true;
#endif
        }

#ifdef DEBUG_MULTIOBJECTIVE_
        std::cout << "[LOCALOPTIMIZER|DEBG] Current Optimized Objective =";
        switch (obj.type)
        {
            case energy:
                std::cout << "********** Energy\n" << std::flush;
                break;
            case time:
                std::cout << "&&&&&&&&&& Time\n" << std::flush;
                break;
            case resource:
                std::cout << "oooooooooo Resource\n" << std::flush;
                break;
        }
        std::cout << "[LOCALOPTIMIZER|DEBUG] Samples: " << std::flush;
        for (auto& sam: obj.samples)
        {
            std::cout << sam << "," << std::flush;
        }
        std::cout << "\n" << std::flush;

        std::cout << "[LOCALOPTIMIZER|DEBUG] Freq Param of Samples: " << std::flush;
#ifdef ALLSCALE_HAVE_CPUFREQ
        for (auto& sam: obj.freq_samples)
        {
            std::cout << sam << "," << std::flush;
        }
        std::cout << "\n" << std::flush;
#endif
#endif

        optstepresult nmd_res = nmd.step(obj.samples[0]);
#ifdef DEBUG_MULTIOBJECTIVE_
        std::cout << "[LOCALOPTIMIZER|DEBUG] Calling NMD Optimizer Step, Param = \n";
        std::cout << "[LOCALOPTIMIZER|DEBUG] New Vertex to try: ";
        std::cout << "Threads = " << nmd_res.threads;
#ifdef ALLSCALE_HAVE_CPUFREQ
        std::cout << " Freq Idx = " << nmd_res.freq_idx << std::endl;
#endif
        std::cout << "Converg Thresh = " << convergence_threshold_ << std::endl;
#endif
        if (nmd_res.converged)
        {
            objectives_[current_objective_idx_].converged = true;
            objectives_[current_objective_idx_].converged_minimum = nmd.getMinObjective();
            double* minimization_point = nmd.getMinVertices();
            objectives_[current_objective_idx_].minimization_params[0]=
                minimization_point[0];
            objectives_[current_objective_idx_].minimization_params[1]=
                minimization_point[1];
#ifdef DEBUG_CONVERGENCE_
            std::cout << "[LOCALOPTIMIZER|INFO] NMD convergence\n";
            std::cout << "******************************************" << std::endl;
            std::cout << "[LOCALOPTIMIZER|INFO] Minimal Objective Value = " <<
                objectives_[current_objective_idx_].converged_minimum <<
                "Threads = " << minimization_point[0] << "Freq_idx = " << minimization_point[1] <<
                std::endl;
            std::cout << "******************************************" << std::endl;
#endif
            act.delta_threads=minimization_point[0];
#ifdef ALLSCALE_HAVE_CPUFREQ
            act.frequency_idx=minimization_point[1];
#endif
            current_objective_idx_++;
            if (current_objective_idx_ == objectives_.size())
            {
                converged_=true;
#ifdef DEBUG_CONVERGENCE_
                std::cout << "[LOCALOPTIMIZER|INFO] ALL OBJECTIVES HAVE CONVERGED " << std::endl;
#endif
            }
        }
        else
        {
            // if a higher priority objective starts getting off leeway margin,
            // decide convergence of the current param at this parameter point
            if (current_objective_idx_>0)
                for (int i=0;i<current_objective_idx_;i++)
                {
                    objective priority_obj=objectives_[i];
                    double max_leeway_value = priority_obj.converged_minimum +
                        priority_obj.leeway*(priority_obj.globalmax - priority_obj.converged_minimum);
                    if (priority_obj.samples[0] > max_leeway_value &&
                            priority_obj.samples[1] > max_leeway_value)
                    {
                        objectives_[current_objective_idx_].converged = true;
                        objectives_[current_objective_idx_].converged_minimum = nmd.getMinObjective();
                        double* minimization_point = nmd.getMinVertices();
                        objectives_[current_objective_idx_].minimization_params[0]=
                            minimization_point[0];
                        objectives_[current_objective_idx_].minimization_params[1]=
                            minimization_point[1];

#ifdef DEBUG_CONVERGENCE_
                        std::cout << "[LOCALOPTIMIZER|INFO] Leeway convergence\n";
                        std::cout << "******************************************" << std::endl;
                        std::cout << "[LOCALOPTIMIZER|INFO] Minimal Objective Value = " <<
                            objectives_[current_objective_idx_].converged_minimum <<
                            "Threads = " << minimization_point[0] << "Freq_idx = " << minimization_point[1] <<
                            std::endl;
                        std::cout << "******************************************" << std::endl;
#endif
                        // find the parameter point that scores the leeway margin value
						act.delta_threads = (int)priority_obj.minimization_params[0]*
                            (max_leeway_value/priority_obj.converged_minimum);
#ifdef ALLSCALE_HAVE_CPUFREQ
                        act.frequency_idx = (int)priority_obj.minimization_params[1]*
                            (max_leeway_value/priority_obj.converged_minimum);
#endif
                        //act.delta_threads=minimization_point[0];
			            //act.frequency_idx=minimization_point[1];
			            current_objective_idx_++;
			            if (current_objective_idx_ == objectives_.size())
                        {
                            converged_=true;
#ifdef DEBUG_CONVERGENCE_
                            std::cout << "[LOCALOPTIMIZER|INFO] ALL OBJECTIVES HAVE CONVERGED " << std::endl;
#endif
                        }
                        return act;
                    }
    		}
            act.delta_threads=(nmd_res.threads==0)?getCurrentThreads():nmd_res.threads;
#ifdef ALLSCALE_HAVE_CPUFREQ
            act.frequency_idx=nmd_res.freq_idx;
#endif
        }
    }
    return act;
}
}
}
