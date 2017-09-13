The Python files in this folder provide automatic regression tests for AllScale runtime/scheduler. It can use any AllScale benchmark and poplulate performance data, such as execution time, energy consumption, initial number of threads,  statistics of the number of threads after throttling (min, max, mean, standard deviation). In addition, benchmark related data, such as benchmark name, benchmark arguments, etc. are populated as well. All these information are kept in SQLite3 database and can be used to plot the benchmark results and save it in pdf files.

## Steps to run regression tests:

* Provide configurations in the config.json file by specifiying app_timeout, app_base_dir, app_names, app_arguments, hpx_threads, objectives, and hpx_queuing.
    * **app_timeout**:  this parameter is limit how long the benchmark/application may run. It is helpfull in case the application hangs due to deadlock.
    * **app_base_dir**: the absolute path where the benchmarks reside
    * **app_names**: comma separated list of benchmark names that reside in the app_base_dir. The regression test will be executed for all of them in a loop. It is true for any config parameter that is list.
    * **app_arguments**: comma separated list of benchmark arguments. Please note that if a given benchmark has more than one argument, all of them considered as a single list element. For example, fine_grained can have three arguments as 10000 10 2. They all needs to be included as a single list element as "10000 10 2".
    * **hpx_threads**: comma separated list of number of OS threads that HPX should use.
    * **objectives**: comma separated list of objectives that AllScale runtime can understand. E.g. "time", "resource", "energy", "time:0.1 energy", "none". The last one will execute the benchmark wihout using any strategic optimization policy.
    * **hpx_queueing**: it is the name of HPX scheduling policy. It needs to be "thorrle" always. Objective with "none" will ignore it.
 
* Once the configuration defined, the test can be executed by simply running scheduler_tester.py. To see different options for running this file pleaes use ./scheduler_tester.py -h .
    * **f**: this command line option is optional and needs to be used only if you would like to give a specific name to the generated sqlite3 db file. By default the file name is as following: allscale_benchmarks_<timestamp>.sqlite. 
    * **t**: optional sqlite3 table name. By default the table name will be "scheduler".
    * **c**: json config file name. By default config.json file will be used from the directory where scheduler_tester.py is executed.
    * **m**: mode option is the most used option. By default the mode is "benchmark" which means the benchamrks will be executed and results saved in sqlite3. The other option is "plot" that is used to plot results.
    * **d**: debug option has not been used yet.
    * **h** (--help): to print usage information into command line.


## The design of the regression system:

