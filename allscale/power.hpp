#ifndef ALLSCALE_POWER_HPP
#define ALLSCALE_POWER_HPP

#include <cstdlib>
#include <vector>
#include <iostream>
#include <fstream>
#include <string>


#ifdef CRAY_COUNTERS
#define NUM_PM_COUNTERS 3
#define FRESHNESS_COUNTER 0
#define ENERGY_COUNTER 1
#define POWER_COUNTER 2

#define PM_MAX_ATTEMPTS 10
#endif

namespace allscale { namespace power
{

#ifdef CRAY_COUNTERS
       std::ifstream pm_files[NUM_PM_COUNTERS];
#endif

//             std::vector<unsigned long long> power_history;
//             std::vector<unsigned long long> energy_history;
       double last_instant_power;
       double last_instant_energy;
       double instant_power;
       double instant_energy;


       void init_power_measurements()
       {

#ifdef CRAY_COUNTERS
          pm_files[FRESHNESS_COUNTER].open("/sys/cray/pm_counters/freshness");
          if(!pm_files[FRESHNESS_COUNTER]) {
                std::cerr << "ERROR: Cannot open /sys/cray/pm_counters/freshness\n";
                exit(1);
          }

          pm_files[ENERGY_COUNTER].open("/sys/cray/pm_counters/energy");
          if(!pm_files[ENERGY_COUNTER]) {
                std::cerr << "ERROR: Cannot open /sys/cray/pm_counters/energy\n";
                exit(1);
          }

          pm_files[POWER_COUNTER].open("/sys/cray/pm_counters/power");
          if(!pm_files[POWER_COUNTER]) {
                std::cerr << "ERROR: Cannot open /sys/cray/pm_counters/power\n";
                exit(1);
          }
#endif
          last_instant_power = instant_power = last_instant_energy = instant_energy = 0.0;
       }



       void finish_power_measurements()
       {
#ifdef CRAY_COUNTERS
          if(pm_files[FRESHNESS_COUNTER].is_open()) pm_files[FRESHNESS_COUNTER].close();

          if(pm_files[ENERGY_COUNTER].is_open()) pm_files[ENERGY_COUNTER].close();

          if(pm_files[POWER_COUNTER].is_open()) pm_files[POWER_COUNTER].close();
#endif
       }


       // Returns last instant power in Watts
       double get_instant_power() { return instant_power; }

       // Returns last instant energy in J
       double get_instant_energy() { return instant_energy; }

       // Returns all power samples
//       std::vector<unsigned long long> get_power_history() { return power_history; }

       // Returns all energy samples
//       std::vector<unsigned long long> get_energy_history() { return energy_history; }


#ifdef CRAY_COUNTERS
       // Returns 0 is the results are valid
       int read_pm_counters() {

          int freshness1, freshness2, n_attempts = 0;
          std::string line;
          unsigned long long tmp_energy, tmp_power;

          // We need to check that the counters have not been updated while we were accessing them
          do {

             n_attempts++;
             pm_files[FRESHNESS_COUNTER].seekg(0);
             std::getline(pm_files[FRESHNESS_COUNTER], line);

             freshness1 = std::atoi(line.c_str());

             // Read energy
             pm_files[ENERGY_COUNTER].seekg(0);
             std::getline(pm_files[ENERGY_COUNTER], line);

             tmp_energy = std::strtoull(line.c_str(), NULL, 10);

             // Read power
             pm_files[POWER_COUNTER].seekg(0);
             std::getline(pm_files[POWER_COUNTER], line);

             tmp_power = std::strtoull(line.c_str(), NULL, 10);

             pm_files[FRESHNESS_COUNTER].seekg(0);
             std::getline(pm_files[FRESHNESS_COUNTER], line);

             freshness2 = std::atoi(line.c_str());

          } while(n_attempts < PM_MAX_ATTEMPTS && freshness1 != freshness2);

          if(freshness1 != freshness2) return 1;
          else {
//              power_history.push_back(tmp_power);
//              energy_history.push_back(tmp_energy);

	      instant_power = (double)tmp_power - last_instant_power;
              last_instant_power = (double)tmp_power;

	      instant_energy = (double)tmp_energy - last_instant_energy;
              last_instant_energy = (double)tmp_energy;

              return 0;
          }
       }


#endif

#ifdef POWER_ESTIMATE

       // Estimate basic power
       double estimate_power(std::uint64_t frequency)
       {
           static const char * file_name = "/sys/class/i2c-dev/i2c-3/device/3-002d/regulator/regulator.1/microvolts";
           std::ifstream file;
           std::uint64_t microvolts = 0;
           float U = 0.9;

           // Estimate using C * U^2 * f

           // for now C is 30pF, not able to find it for Cortex A53
           auto C = 30 * 1.0e-12; //


           // voltage U
#ifdef READ_VOLTAGE_FILE
	   file.open(file_name);
           if(!file)
                std::cerr << "Warning: Cannot read voltage from /sys/class, using 0.9 instead" << std::endl;
           else {
              file >> microvolts;
              U = (double)microvolts * 1.0e-6;

	      file.close();
	   }
#endif
           instant_power = (double)C * (U * U) * (double)(frequency * 1000);  // freq is in kHz

           return instant_power;
       }
#endif



}}

#endif

