#include <allscale/util/hardware_reconf.hpp>

#include <iostream>
#include <memory>
#include <cpufreq.h>

namespace allscale { namespace components { namespace util {


    std::vector<unsigned long> hardware_reconf::get_frequencies(unsigned int cpu)
    {
        std::vector<unsigned long> frequencies;
        struct cpufreq_available_frequencies* available_frequencies = (cpufreq_available_frequencies*)malloc(sizeof(struct cpufreq_available_frequencies));
        available_frequencies = cpufreq_get_available_frequencies(cpu);
        while (available_frequencies) {
            frequencies.push_back(available_frequencies->frequency);
            available_frequencies = available_frequencies->next;
        }

        if (available_frequencies != NULL)
            cpufreq_put_available_frequencies(available_frequencies);

        return frequencies;
    }
   
    std::vector<std::string> hardware_reconf::get_governors(unsigned int cpu)
    {
        std::vector<std::string> cpu_governors;
        struct cpufreq_available_governors *available_governors = new cpufreq_available_governors;
        available_governors = cpufreq_get_available_governors(cpu);
        while (available_governors) {
            cpu_governors.push_back(available_governors->governor);
            available_governors = available_governors->next;
        }

        if (available_governors != NULL)
            cpufreq_put_available_governors(available_governors);
    
        return cpu_governors; 
    }


    int hardware_reconf::set_frequency(unsigned int cpu, unsigned long target_frequency)
    {
        throw std::logic_error("Not implemented yet");
    }


    int hardware_reconf::set_freq_policy(unsigned int cpu, std::pair<std::string, std::pair<unsigned long, unsigned long>> policy)
    {
        std::unique_ptr<cpufreq_policy> freq_policy(new cpufreq_policy); //(cpufreq_policy*) malloc(sizeof(struct cpufreq_policy));
//        cpufreq_policy* freq_policy = (cpufreq_policy*) malloc(sizeof(struct cpufreq_policy));
        freq_policy->governor = const_cast<char *>(policy.first.c_str());
        freq_policy->min = policy.second.first;
        freq_policy->max = policy.second.second;
        int res = cpufreq_set_policy(cpu, freq_policy.get());
    }


    unsigned long hardware_reconf::get_kernel_freq(unsigned int cpu)
    {
        return cpufreq_get_freq_kernel(cpu);
    }

    unsigned long hardware_reconf::get_hardware_freq(unsigned int cpu)
    {
        return cpufreq_get_freq_hardware(cpu);
    }


}}}
