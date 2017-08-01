#!/usr/bin/env python3

import os
import sys
import time
import getopt
import sqlite3
import logging
import datetime

import query_manager
import utils



def collect_data(sqlite3_db_file, app_base_dir, app, app_arg, hpx_thread, hpx_queuing, objective):
    """Save time and energy benchmark results in sqlite3 db"""
    import statistics  # requires python3.4+

    active_threads = []
    hpx_arg = "--hpx:threads={0} --hpx:queuing={1} --hpx:ini=allscale.objective!={2}".format(hpx_thread, hpx_queuing, objective)
    command = "{0} {1} {2}".format(os.path.join(os.sep, app_base_dir, app), app_arg, hpx_arg)
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

    sqlite3_db_file = "allscale_benchmarks_{:%Y-%m-%d-%H-%M-%S}".format(datetime.datetime.now()) + ".sqlite"
    table_name = "scheduler"
    mode = "benchmark"
    config_file = "./config.json"

    try:
        options, remainder = getopt.getopt(
            sys.argv[1:],
            'f:t:c:m:dh',
            [
             'db_file=',
             'table=',
             'config=',
             'mode=',
             'debug',
             'help'
            ])
    except getopt.GetoptError as err:
        print('Options error:', err)
        sys.exit(1)


    for opt, arg in options:
        if opt in ('-f', '--db_file'):
            sqlite3_db_file = arg
        elif opt in ('-t', '--table'):
            table_name = arg
        elif opt in ('-c', '--config'):
            config_file = arg
            if not os.path.isfile(config_file):
                print("Config file {0} does not exist".format(config_file))
                sys.exit(-1)
        elif opt in ('-m', '--mode'):
            mode = arg
        elif opt in ('-d', '--debug'):
            debug = True
        elif opt in ('-h', '--help'):
            help_str = """
                  Arguments:
                     -f (--db_file) <sqlite3 db file>  (Default is {0})
                     -t (--table) <sqlite3 table name> (Default is {1})
                     -c (--config) <config_file> (Default is {2})
                     -m (--mode) benchmark|plot (Default is {3})
                     -d (--debug) (Default off)
                     -h (--help) (This output)
                 """.format(sqlite3_db_file, table_name, config_file, mode)
            print(help_str)
            sys.exit(0)

    config_data = utils.read_config(config_file)
   
    try: 
        app_base_dir = config_data['app_base_dir']
        app_names = config_data['app_names']
        app_arguments = config_data['app_arguments']
        hpx_threads = config_data['hpx_threads']
        hpx_queuing = config_data['hpx_queuing']
        allscale_objectives = config_data['objectives']
    except KeyError as ke:
        print("KeyError: {0}. Check config file {1}.".format(ke,config_file))
        sys.exit(-1)

    if not mode in ["benchmark", "plot"]:
        print("Wrong mode requested: {0}. Valid values are 'benchmark' and 'plot'".format(mode))
        sys.exit(-1)

    if mode == "benchmark":
        query_manager.create_table(sqlite3_db_file, table_name)
        for app in app_names:
            for app_arg in app_arguments:
                for hpx_thread in hpx_threads:
                    for objective in allscale_objectives:
                        collect_data(sqlite3_db_file, app_base_dir, app, app_arg, hpx_thread, hpx_queuing, objective)
    elif mode == "plot":
        for app in app_names:
            for app_arg in app_arguments:
                utils.plot(sqlite3_db_file, table_name, app, app_arg, hpx_threads, allscale_objectives)



     
