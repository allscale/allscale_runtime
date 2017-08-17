
#include <string>
#include <iostream>
#include <hpx/util/lightweight_test.hpp>
#include <allscale/util/hardware_reconf.hpp>

#include <cpufreq.h>


int main(int argc, char** argv) {
    using hardware_reconf = allscale::components::util::hardware_reconf;  

/*
    unsigned int kernel_freq = hardware_reconf::get_kernel_freq(0); 
    unsigned int hardware_freq = hardware_reconf::get_hardware_freq(0);
    std::vector<std::string> cpu_governors = hardware_reconf::get_governors(0);

    std::cout << "kernel_freq: " << kernel_freq << ", hw_freq: " << hardware_freq << std::endl;

    for (std::string governor : cpu_governors)
    {
        std::cout << "governor: " << governor << std::endl;
    }

    std::vector<unsigned long> cpu_freqs = hardware_reconf::get_frequencies(0);
    for (unsigned long freq : cpu_freqs)
    {
        std::cout << "freq: " << freq << std::endl;
    }
*/  

// It needs to be run with sudo or you should be granted to change cpu governors
    cpufreq_policy policy;
    std::string governor = "ondemand";
    policy.governor = const_cast<char*>(governor.c_str());
    policy.min = 2061000;
    policy.max = 2061000;

    int res = hardware_reconf::set_freq_policy(0, policy); 
    HPX_TEST_EQ(res, 0);
   
    unsigned long long energy = hardware_reconf::read_system_energy();
    HPX_TEST(energy > 0);

    energy = hardware_reconf::read_system_energy("non-existent-file");
    HPX_TEST(energy == 0);

    return 0;
}





