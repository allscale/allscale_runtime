
#include <iostream>
#include <allscale/util/hardware_reconf.hpp>


int main(int argc, char** argv) {
    using hardware_reconf = allscale::components::util::hardware_reconf;  

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
   

    return 0;
}





