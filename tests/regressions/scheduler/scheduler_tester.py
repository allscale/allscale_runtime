#!/usr/bin/env python3

import sys
import time
import getopt
import sqlite3
import logging
import datetime

import statistics  # requires python3.4+

import query_manager
import utils



def collect_data(sqlite3_db_file, app_base_dir, app, app_arg, hpx_thread, objective):
    """Save time and energy benchmark results in sqlite3 db"""
    active_threads = []
    hpx_arg = "--hpx:threads={0} --hpx:queuing={1} --hpx:ini=allscale.objective!={2}".format(hpx_thread, hpx_queuing, objective)
    command = "{0}{1} {2} {3}".format(app_base_dir, app, app_arg, hpx_arg)
    print("Executing command:\n {0}".format(command))

    start_energy = utils.read_energy()
    start_time = time.time()

    #It is assumed that the scheduler prints out "... Active threads: num_threads"
    for resp in utils.run_benchmark(command):
        if "Active threads" in resp:
            active_threads.append(int(resp.split(':')[1].split('\\')[0]))

    end_time = time.time()
    end_energy = utils.read_energy()

    params = {}
    params['app_name'] = app
    params['app_arg'] = app_arg
    params['exec_time'] = end_time -start_time
    params['energy'] = end_energy - start_energy
    params['power'] = 0
    params['initial_threads'] = hpx_thread
    if active_threads:
        params['min_threads'] = min(active_threads)
        params['max_threads'] = max(active_threads)
        params['mean_threads'] = statistics.mean(active_threads)
        try:
            params['mode_threads'] = statistics.mode(active_threads)
        except statistics.StatisticsError:
           #FIXME
            params['mode_threads'] = 0
        params['stdev_threads'] = statistics.stdev(active_threads)

    params['hpx_queuing'] = hpx_queuing
    params['objective'] = objective
    params['date'] = '{:%Y-%m-%d %H:%M:%S}'.format(datetime.datetime.now())

    query_manager.insert_query(sqlite3_db_file, params)



if __name__ == "__main__":
#    app_base_dir = "/home/khalid/workspace/allscale_runtime/my_build_dbg/examples/pfor/"
    app_base_dir = "/home/khalid/Documents/lahiyeler/AllScale/allscale_runtime/my_build_dbg/examples/pfor/"
    app_names = ["fine_grained", "chain"]
    app_args = ["4435456 50 1", "8435456 50 1"];
    hpx_threads = [16, 8]
    hpx_queuing = "throttling"
    allscale_objectives = ["time_resource", "time"]

    sqlite3_db_file = "allscale_benchmarks_{:%Y-%m-%d}".format(datetime.datetime.now()) + ".sqlite"
    table_name = "scheduler"
    mode = "benchmark"

    try:
        options, remainder = getopt.getopt(
            sys.argv[1:],
            'b:f:t:m:vdh',
            ['app_base_dir=',
             'db_file=',
             'table=',
             'mode=',
             'verbose',
             'debug',
             'help'
            ])
    except getopt.GetoptError as err:
        print('Options error:', err)
        sys.exit(1)


    for opt, arg in options:
        if opt in ('-b', '--app_base_dir'):
            app_base_dir = arg
        elif opt in ('-f', '--db_file'):
            sqlite3_db_file = arg
        elif opt in ('-v', '--verbose'):
            verbose = True
        elif opt in ('-t', '--table'):
            table_name = arg
        elif opt in ('-m', '--mode'):
            mode = arg
        elif opt in ('-d', '--debug'):
            debug = True
        elif opt in ('-h', '--help'):
            help_str = """
                  Arguments:
                     -b (--app_base_dir) <app_base_dir> (Default is {0})
                     -f (--db_file) <sqlite3 db file>  (Default is {1})
                     -t (--table) <sqlite3 table name> (Default is {2})
                     -m (--mode) benchmark|plot (Default is {3})
                     -v (--verbose) (Default off)
                     -d (--debug)   (Default off)
                     -h (--help) (This output)
                 """.format(app_base_dir, sqlite3_db_file, table_name, mode)
            print(help_str.strip())
            sys.exit(0)

    if not mode in ["benchmark", "plot"]:
        print("Wrong mode requested: {0}. Valid values are 'benchmark' and 'plot'".format(mode))
        sys.exit(-1)

    if mode == "benchmark":
        query_manager.create_table(sqlite3_db_file, table_name)
        for app in app_names:
            for app_arg in app_args:
                for hpx_thread in hpx_threads:
                    for objective in allscale_objectives:
                        collect_data(sqlite3_db_file, app_base_dir, app, app_arg, hpx_thread, objective)
    elif mode == "plot":
        for app in app_names:
            for app_arg in app_args:
                utils.plot(sqlite3_db_file, table_name, app, app_arg, hpx_threads, allscale_objectives)



     
