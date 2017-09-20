The Python files in this folder provide automatic regression tests for AllScale runtime/scheduler. It can use any AllScale benchmark and poplulate performance data, such as execution time, energy consumption, initial number of threads,  statistics of the number of threads after throttling (min, max, mean, standard deviation). In addition, benchmark related data, such as benchmark name, benchmark arguments, etc. are populated as well. All these information are kept in SQLite3 database and can be used to plot the benchmark results and save it in pdf files.

## The design of the regression system:

The test system consists of four main files:

* **config.json**: benchmark and runtime specific configuration parameters
* **query_manager.py**: python file that is responsible for SQLite3 query manager, such as *creating table*, *inserting into table*, and *reading from table*.
* **scheduler_tester.py**: the driver file that executes the benchmarks or plots results of the already executed benchmarks.
* **utils.py**: this file performs several tasks, such as executing the benchmarks that are specified in the config file, parsing benchmark outputs and sending them to the query manager for insertion into the SQLite3 table, reading config file, reading system energy, and plotting.


## Steps to run regression tests:

* Provide configurations in the config.json file by specifiying app_timeout, app_base_dir, app_names, app_arguments, hpx_threads, objectives, and hpx_queuing.
    * **app_timeout**:  this parameter is limit how long the benchmark/application may run. It is helpfull in case the application hangs due to deadlock.
    * **app_base_dir**: the absolute path where the benchmarks reside
    * **app_names**: comma separated list of benchmark names that reside in the app_base_dir. The regression test will be executed for all of them in a loop. It is true for any config parameter that is list.
    * **app_arguments**: comma separated list of benchmark arguments. Please note that if a given benchmark has more than one argument, all of them considered as a single list element. For example, fine_grained can have three arguments as 10000 10 2. They all needs to be included as a single list element as "10000 10 2".
    * **hpx_threads**: comma separated list of number of OS threads that HPX should use.
    * **objectives**: comma separated list of objectives that AllScale runtime can understand. E.g. *time*, *resource*, *energy*, *time:0.1 energy*, *none*. The last one will execute the benchmark wihout using any strategic optimization policy.
    * **plot_keys**: comma separated list of measurements that is going to be on the *y-axis* of the generated plots. E.g.  *time*, *resource*, *energy*.
    * **hpx_queueing**: it is the name of HPX scheduling policy. It needs to be "throttle" always. Objective with "none" will ignore it.
    * **arch**: the system architecture that the experiments were performed on. It is used on the plot title.
 
* Once the configuration defined, the test can be executed by simply running scheduler_tester.py. To see different options for running this file pleaes use *./scheduler_tester.py -h* .
    * **f**: this command line option is optional and needs to be used only if you would like to give a specific name to the generated sqlite3 db file. By default the file name is as following: *allscale_benchmarks_<timestamp>.sqlite*. 
    * **t**: optional sqlite3 table name. By default the table name will be "scheduler".
    * **c**: json config file name. By default config.json file will be used from the directory where scheduler_tester.py is executed.
    * **m**: mode option is the most used option. By default the mode is "benchmark" which means the benchamrks will be executed and results saved in sqlite3. The other option is "plot" that is used to plot results.
    * **d**: debug option has not been used yet.
    * **h** (--help): to print usage information into command line.

## How plotting works

The *plot* function in **utils.py** creates one pdf file per app_name and app_arg combination. This function can plot either execution time, resource usage, or energy (y-axis) against different number of OS threads (x-axis). The *plot_keys* config parameter defines the desired y-axis values.

## Notes

The *collect_data* function in *utils.py* assumes that scheduler_component.cpp prints active threads as "It is assumed that the scheduler prints out *... Active threads: num_threads* and resource usage as *Resource usage: num_resource_usage*.
