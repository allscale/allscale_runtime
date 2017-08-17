#ifndef ALLSCALE_HARDWARE_RECONF_HPP
#define ALLSCALE_HARDWARE_RECONF_HPP

#include <vector>
#include <string>
#include <cpufreq.h>

namespace allscale { namespace components { namespace util {


    ///
    ///  A wrapper class around cpufreq APIs
    ///
    struct hardware_reconf 
    {
        static std::vector<unsigned long> get_frequencies(unsigned int cpu);
        static std::vector<std::string> get_governors(unsigned int cpu);
        static int set_frequency(unsigned int cpu, unsigned long target_frequency);
        static int set_freq_policy(unsigned int cpu, cpufreq_policy policy);

        static unsigned long get_kernel_freq(unsigned int cpu);
        static unsigned long get_hardware_freq(unsigned int cpu);

        /// \brief This function reads system energy from sysfs on POWER8/+ machines and             
        ///        returns comulative energy.
        ///
        /// \param sysfs_file   [in] The absolute path of sysfs file that provides energy readings.
        ///
        /// \returns            This function reads system energy from sysfs on POWER8/+ machines.
        ///                     If the file does not exist, i.e. on architectures other than ppc,
        ///                     or if there is parsing error it will return zero.
        ///
        /// \note               This function may be removed and moved into the monitoring component
        ///                     in future. 
        ///
        static unsigned long long read_system_energy(const std::string &sysfs_file = "/sys/devices/system/cpu/occ_sensors/system/system-energy");

    };
}}}
#endif

