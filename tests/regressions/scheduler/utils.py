import subprocess
import logging
import json
import os

import query_manager

def run_benchmark(exe):
    """Executes external command using system shell and returns output as generator"""
 
    app = exe.split(" ")[0]
    if not (os.path.isfile(app) and os.access(app, os.X_OK)):
        raise Exception("File {0} either does not exist or it is not executable".format(app))

    p = subprocess.Popen(exe, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, shell=True)
    while (True):
        ret_code = p.poll()
        line = p.stdout.readline()
        yield str(line)
        if (ret_code is not None): # break if subprocess is not running
            break


def plot(sqlite3_db_file, table_name, app_name, app_arg, hpx_threads, objectives):
    """Creates graphs out of the given lists"""
    import matplotlib.pyplot as plt
    from matplotlib.backends.backend_pdf import PdfPages

    pp = PdfPages("{0}-{1}.pdf".format(app_name, app_arg.split(" ")[0]))
    fig, ax = plt.subplots()
    for objective in objectives:
        rows = query_manager.read_from_sqlite3(sqlite3_db_file, app_name, app_arg, hpx_threads, objective)
        threads = [rec[2] for rec in rows]
        times = [rec[3] for rec in rows]
        ax.plot(threads, times, marker="o", label = objective)
        ax.set_xlabel("Number of threads")
        ax.set_ylabel("Execution time")
        legend = ax.legend(loc='upper left', shadow=True, fontsize='x-large')
    
    #FIXME make it dynamic
    plt.xlim(0, 32) 
    plt.grid(True)
    #FIXME make title parametric
    plt.title("Execution time with different policies")
    pp.savefig()
    pp.close()
    plt.close()




def read_energy(file_name = "/sys/devices/system/cpu/occ_sensors/system/system-energy"):
    """Read power and energy sensors which are available on POWER8"""
    import os

    logging.basicConfig(filename = 'scheduler_tester.log', level = logging.DEBUG)

    if not os.path.exists(file_name):
        logging.debug("Sensor file: {0} does not exist".format(file_name))
        energy = 0
    else:
        with open(file_name) as f:
            energy = int(f.read().split(" ")[0])

    return energy


def read_config(json_file):
    """Reads confi from the provided json file and returns dict"""
    config = {}
    with open(json_file, 'r') as f:
        config = json.load(f)

    return config











