#if defined(ALLSCALE_HAVE_CPUFREQ)
#include <string>
#include <iostream>
#include <thread>
#include <chrono>
#include <algorithm>

#include <cpufreq.h>

#include <hpx/util/lightweight_test.hpp>
#include <allscale/util/hardware_reconf.hpp>


int main(int argc, char** argv) {
    using hardware_reconf = allscale::components::util::hardware_reconf;

    hardware_reconf::hw_topology topo =hardware_reconf::read_hw_topology();

/*
    std::vector<std::string> cpu_governors = hardware_reconf::get_governors(0);

    for (std::string governor : cpu_governors)
    {
        std::cout << "governor: " << governor << std::endl;
    }
*/
    int cpu = 0;
    std::vector<unsigned long> cpu_freqs = hardware_reconf::get_frequencies(cpu);
    auto min_max_freqs = std::minmax_element(cpu_freqs.begin(), cpu_freqs.end());
    unsigned long min_freq = *min_max_freqs.first;
    unsigned long max_freq = *min_max_freqs.second;

    cpufreq_policy policy;
    std::string governor = "userspace";
    policy.governor = const_cast<char*>(governor.c_str());
    policy.min = min_freq;
    policy.max = max_freq;

    int res = hardware_reconf::set_freq_policy(cpu, policy);
    HPX_TEST_EQ(res, 0);

    unsigned long target_freq = min_freq;
    unsigned long hardware_freq = min_freq;

    for (unsigned int cpu_id = 0; cpu_id < topo.num_logical_cores; cpu_id += topo.num_hw_threads)
    {
        // changing frequency and governor requires root/sudo access
        res = hardware_reconf::set_frequency(cpu_id, cpu_id + 1, target_freq);
        HPX_TEST_EQ(res, 0);

        //unsigned long latency = hardware_reconf::get_cpu_transition_latency(cpu_id);
        // wait a bit for the changes to come into effect
        std::this_thread::sleep_for(std::chrono::microseconds(1));

        // get_hardware_freq requires sudo access
        hardware_freq = hardware_reconf::get_hardware_freq(cpu_id);
        HPX_TEST_EQ(hardware_freq, target_freq);
    }

    unsigned long long energy = hardware_reconf::read_system_energy();
    HPX_TEST(energy > 0);

    energy = hardware_reconf::read_system_energy("non-existent-file");
    HPX_TEST_EQ(energy, 0);

    target_freq = max_freq;
    unsigned int target_cpu_count = 4;
    hardware_reconf::set_frequencies_bulk(target_cpu_count, target_freq);
    std::this_thread::sleep_for(std::chrono::microseconds(1));

    unsigned int affected_cpu_count = hardware_reconf::num_cpus_with_frequency(target_freq);
    HPX_TEST_EQ(affected_cpu_count, target_cpu_count);

//    hardware_reconf::make_cpus_online(0, 8);
//    hardware_reconf::make_cpus_offline(0, 8);
//    std::cout << "Topology: [" << topo.num_logical_cores << ", " << topo.num_physical_cores << ", " << topo.num_hw_threads << "]" << std::endl;

    return 0;
}


#else
int main(int argc, char** argv) {
    return 0;
}
#endif

