#!/usr/bin/env python3

import os
import sys
import time
import getopt
import sqlite3
import logging
import datetime

import query_manager
from utils import Utils



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

    utils = Utils()
    config_data = utils.read_config(config_file)
   
    try: 
        app_timeout = config_data['app_timeout']
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
        for app in app_names:
            for app_arg in app_arguments:
                for hpx_thread in hpx_threads:
                    for objective in allscale_objectives:
                        utils.collect_data(sqlite3_db_file, table_name, app_timeout, app_base_dir, app, app_arg, hpx_thread, hpx_queuing, objective)
    elif mode == "plot":
        for app in app_names:
            for app_arg in app_arguments:
                utils.plot(sqlite3_db_file, table_name, app, app_arg, hpx_threads, allscale_objectives)



     
