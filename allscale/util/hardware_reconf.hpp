#ifndef ALLSCALE_HARDWARE_RECONF_HPP
#define ALLSCALE_HARDWARE_RECONF_HPP

#include <vector>
#include <string>
#include <cpufreq.h>

namespace allscale { namespace components { namespace util {


    /**
     *  A wrapper class around cpufreq APIs
     *
     */
    struct hardware_reconf 
    {
        static std::vector<unsigned long> get_frequencies(unsigned int cpu);
        static std::vector<std::string> get_governors(unsigned int cpu);
        static int set_frequency(unsigned int cpu, unsigned long target_frequency);
        static int set_freq_policy(unsigned int cpu, cpufreq_policy policy);

        static unsigned long get_kernel_freq(unsigned int cpu);
        static unsigned long get_hardware_freq(unsigned int cpu);

    };
}}}
#endif

