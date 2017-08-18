
#include <string>
#include <iostream>
#include <thread>
#include <chrono>
#include <algorithm>
#include <hpx/util/lightweight_test.hpp>
#include <allscale/util/hardware_reconf.hpp>

#include <cpufreq.h>


int main(int argc, char** argv) {
    using hardware_reconf = allscale::components::util::hardware_reconf;  

/*
    std::vector<std::string> cpu_governors = hardware_reconf::get_governors(0);

    for (std::string governor : cpu_governors)
    {
        std::cout << "governor: " << governor << std::endl;
    }
*/
    const unsigned int cpu_id = 0;
//  Changing frequency and governor requires root/sudo access
    std::vector<unsigned long> cpu_freqs = hardware_reconf::get_frequencies(cpu_id);
    auto min_max_freqs = std::minmax_element(cpu_freqs.begin(), cpu_freqs.end()); 
    unsigned long min_freq = *min_max_freqs.first;
    unsigned long max_freq = *min_max_freqs.second;

    cpufreq_policy policy;
    std::string governor = "userspace";
    policy.governor = const_cast<char*>(governor.c_str());
    policy.min = min_freq;
    policy.max = max_freq;

    int res = hardware_reconf::set_freq_policy(cpu_id, policy); 
    HPX_TEST_EQ(res, 0);

    res = hardware_reconf::set_frequency(cpu_id, min_freq);
    HPX_TEST_EQ(res, 0);

    unsigned long latency = hardware_reconf::get_cpu_transition_latency(cpu_id);
    // Wait a bit for the changes to come into effect
    std::this_thread::sleep_for(std::chrono::nanoseconds(latency) * 100);

    unsigned int hardware_freq = hardware_reconf::get_hardware_freq(cpu_id);
    HPX_TEST_EQ(hardware_freq, min_freq);
   
    unsigned long long energy = hardware_reconf::read_system_energy();
    HPX_TEST(energy > 0);

    energy = hardware_reconf::read_system_energy("non-existent-file");
    HPX_TEST_EQ(energy, 0);

    return 0;
}





