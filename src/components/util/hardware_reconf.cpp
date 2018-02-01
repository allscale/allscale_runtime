#include <allscale/util/hardware_reconf.hpp>

#include <iostream>
#include <fstream>
#include <memory>
#include <mutex>
#include <cpufreq.h>

#include <boost/format.hpp>

namespace allscale { namespace components { namespace util {

    hardware_reconf::mutex_type hardware_reconf::freq_mtx_;

    std::vector<unsigned long> hardware_reconf::get_frequencies(unsigned int cpu)
    {
        std::vector<unsigned long> frequencies;
        struct cpufreq_available_frequencies* available_frequencies = (cpufreq_available_frequencies*)malloc(sizeof(struct cpufreq_available_frequencies));
        available_frequencies = cpufreq_get_available_frequencies(cpu);
        while (available_frequencies) {
            frequencies.push_back(available_frequencies->frequency);
            available_frequencies = available_frequencies->next;
        }

        if (available_frequencies != nullptr)
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


    int hardware_reconf::set_frequency(unsigned int min_cpu, unsigned int max_cpu, unsigned long target_frequency)
    {
        int res = 0;
	hardware_reconf::hw_topology topo = read_hw_topology();
        unsigned int max_cpu_id = topo.num_logical_cores;
        unsigned int hw_threads = topo.num_hw_threads;
	for (int cpu_id = min_cpu; cpu_id < min_cpu + (max_cpu * hw_threads); cpu_id += hw_threads)
        {
	    // cpu exists
	    if (!cpufreq_cpu_exists(cpu_id))
		{
		    res = cpufreq_set_frequency(cpu_id, target_frequency);
		}
        }

        return res;
    }


    void hardware_reconf::set_next_frequency(unsigned int freq_step, bool dec)
    {
        // Get available frequencies of core 0,
        // we assume all cores have the same set of frequencies
        // get_frequencies returns vector of freqs in decreasing order
        std::vector<unsigned long> all_freqs = get_frequencies(0);
        hardware_reconf::hw_topology topo = read_hw_topology();
        unsigned int max_cpu_id = topo.num_logical_cores;
        unsigned int hw_threads = topo.num_hw_threads;

        std::unique_lock<mutex_type> l(hardware_reconf::freq_mtx_);
        unsigned int target_freq_idx = 0;
        for (int cpu_id = 0; cpu_id < max_cpu_id; cpu_id += hw_threads)
        {
            unsigned long hw_freq = get_hardware_freq(cpu_id);
            for (int freq_idx = 0; freq_idx < all_freqs.size(); freq_idx++)
            {
                if (hw_freq == all_freqs[freq_idx])
                {
                    if ( !dec )
                    {
                        if ( freq_idx - freq_step >= 0 )
                            target_freq_idx = freq_idx - freq_step;
                        else if ( freq_idx >0 && freq_idx - freq_step < 0 )
                            target_freq_idx = 0;

                        cpufreq_set_frequency(cpu_id, all_freqs[target_freq_idx]);
                        break;
                    }
                    else if ( dec )
                    {
                        if ( freq_idx + freq_step < all_freqs.size() )
                            target_freq_idx = freq_idx + freq_step;
                        else if ( freq_idx < all_freqs.size() && freq_idx + freq_step >= all_freqs.size() )
                            target_freq_idx = all_freqs.size() - 1;

                        cpufreq_set_frequency(cpu_id, all_freqs[target_freq_idx]);
                        break;
                    }
                }
            }
        }
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


    unsigned long hardware_reconf::get_cpu_transition_latency(unsigned int cpu)
    {
        return cpufreq_get_transition_latency(cpu);
    }


    void hardware_reconf::set_frequencies_bulk(unsigned int num_cpus, unsigned long target_frequency)
    {
        int res = 0;
        int count = 0;

        hardware_reconf::hw_topology topo = read_hw_topology();
        unsigned int max_cpu_id = topo.num_logical_cores;
        unsigned int hw_threads = topo.num_hw_threads;

        std::unique_lock<mutex_type> l(hardware_reconf::freq_mtx_);
        for(unsigned int cpu_id = 0; cpu_id < max_cpu_id; cpu_id += hw_threads)
        {
            if (get_hardware_freq(cpu_id) != target_frequency)
            {
                if (count == num_cpus)
                    break;

                res = set_frequency(cpu_id, cpu_id + 1, target_frequency);
                HPX_ASSERT(res == 0);
                count++;
            }
        }
    }


    unsigned int hardware_reconf::num_cpus_with_frequency(unsigned long frequency)
    {
        hardware_reconf::hw_topology topo = read_hw_topology();
        unsigned int max_cpu_id = topo.num_logical_cores;
        unsigned int hw_threads = topo.num_hw_threads;
        unsigned int cpu_count = 0;

        std::unique_lock<mutex_type> l(hardware_reconf::freq_mtx_);
        for(unsigned int cpu_id = 0; cpu_id < max_cpu_id; cpu_id += hw_threads)
        {
            if (get_hardware_freq(cpu_id) == frequency)
            {
                cpu_count++;
            }
        }
        return cpu_count;
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


    hardware_reconf::hw_topology hardware_reconf::read_hw_topology()
    {
        hardware_reconf::hw_topology topo;

        hwloc_topology_t topology;
        hwloc_topology_init(&topology);
        hwloc_topology_load(topology);

        int core_depth = hwloc_get_type_depth(topology, HWLOC_OBJ_CORE);
        HPX_ASSERT(core_depth != HWLOC_TYPE_DEPTH_UNKNOWN);

        topo.num_physical_cores = hwloc_get_nbobjs_by_depth(topology, core_depth);
        topo.num_logical_cores = std::thread::hardware_concurrency();

//        HPX_ASSERT(topo.num_logical_cores % topo.num_physical_cores == 0);

        topo.num_hw_threads = topo.num_logical_cores / topo.num_physical_cores;

        return topo;
    }


    void hardware_reconf::make_cpus_offline(unsigned int min_cpu_id, unsigned int max_cpu_id)
    {
        std::vector<std::thread> threads;
        std::string sysfs_file = "/sys/devices/system/cpu/cpu%d/online";
        for (unsigned int cpu_id = min_cpu_id; cpu_id < max_cpu_id; cpu_id++)
        {
            std::string sysfs_cpu_file = boost::str(boost::format(sysfs_file) % cpu_id);
            std::cout << "Making offline: " << sysfs_cpu_file << std::endl;
            threads.push_back(std::move(std::thread(hardware_reconf::write_to_file, 0, sysfs_cpu_file)));
        }

        for (int i = min_cpu_id; i < max_cpu_id; i++)
        {
            threads[i].join();
        }
    }


    void hardware_reconf::make_cpus_online(unsigned int min_cpu_id, unsigned int max_cpu_id)
    {
        std::vector<std::thread> threads;
        std::string sysfs_file = "/sys/devices/system/cpu/cpu%d/online";
        for (unsigned int cpu_id = min_cpu_id; cpu_id < max_cpu_id; cpu_id++)
        {
            std::string sysfs_cpu_file = boost::str(boost::format(sysfs_file) % cpu_id);
            threads.push_back(std::move(std::thread(hardware_reconf::write_to_file, 1, sysfs_cpu_file)));
        }

        for (int i = min_cpu_id; i < max_cpu_id; i++)
        {
            threads[i].join();
        }
    }


    void hardware_reconf::write_to_file(int value, const std::string& file_name)
    {
        std::ofstream sysfs_cpu(file_name);
        sysfs_cpu << value;
        sysfs_cpu.close();
    }

}}}




