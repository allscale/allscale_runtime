#include <allscale/util/hardware_reconf.hpp>

#include <iostream>
#include <fstream>
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
        cpufreq_available_governors *available_governors = new cpufreq_available_governors;
        available_governors = cpufreq_get_available_governors(cpu);
        while (available_governors) {
            cpu_governors.push_back(available_governors->governor);
            available_governors = available_governors->next;
        }

        //TODO check if we have to use the following function
        cpufreq_put_available_governors(available_governors);
    
        return cpu_governors; 
    }


    int hardware_reconf::set_frequency(unsigned int cpu, unsigned long target_frequency)
    {
        throw std::logic_error("Not implemented yet");
    }


    int hardware_reconf::set_freq_policy(unsigned int cpu, cpufreq_policy policy)
    {
        int res = cpufreq_set_policy(cpu, &policy);
        return res;
    }


    unsigned long hardware_reconf::get_kernel_freq(unsigned int cpu)
    {
        return cpufreq_get_freq_kernel(cpu);
    }

    unsigned long hardware_reconf::get_hardware_freq(unsigned int cpu)
    {
        return cpufreq_get_freq_hardware(cpu);
    }


    unsigned long long hardware_reconf::read_system_energy(const std::string &sysfs_file)
    {
        unsigned long long energy = 0;
        std::string line;
        std::ifstream ifile(sysfs_file.c_str());
        std::getline(ifile, line);
        ifile.close();

        try {
            energy = std::stoull(line);
        } catch (const std::invalid_argument& ia)
        { 
            std::cerr << "Error reading energy sensor: " << ia.what() << ", " << line <<'\n';
        } catch (const std::out_of_range& ofr)
        {
            std::cerr << "Error reading energy sensor: " << ofr.what() << ", " << line << '\n';
        } catch ( const std::exception& e )
        {
            std::cerr << "Error reading energy sensor: " << e.what() << ", " << line << '\n';
        }
        catch ( ... )
        {
            std::cerr << "Error reading energy sensor, " << line << '\n';
        }

        return energy;
    }

}}}
