import subprocess
import logging
import json
import datetime
#import time
import os
from timeit import default_timer as timer


import query_manager

class Utils:

    def __init__(self, db_created = False, max_threads = 20):
        self.db_created = db_created
        self.cmd_ret_code = 0
        self.max_threads = max_threads
        logging.basicConfig(filename = 'scheduler_tester.log', level = logging.DEBUG)


    def run_benchmark(self, exe, app_timeout):
        """Executes external command using system shell and returns output as generator"""
        app = exe.split(" ")[0]
        if not (os.path.isfile(app) and os.access(app, os.X_OK)):
            logging.debug("File {0} either does not exist or it is not executable".format(app))
            raise Exception("File {0} either does not exist or it is not executable".format(app))

        if "--hpx:ini=allscale.objective!=energy" in exe:
            # energy policy requires sudo for dvfs
            command = "sudo {0} {1}".format(app_timeout, exe)
        else:
            command = "{0} {1}".format(app_timeout, exe)
        print("Executing command: {0}".format(command))
        logging.debug("Executing command: {0}".format(command))
        p = subprocess.Popen(command, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, shell=True)
        while (True):
            ret_code = p.poll()
            line = p.stdout.readline()
            logging.debug(line)
            yield str(line)
            if (ret_code is not None): # break if subprocess is not running
                break

        self.cmd_ret_code = ret_code
        

    def plot(self, sqlite3_db_file, table_name, app_name, app_arg, hpx_threads, objectives, plot_key, arch):
        """Creates graphs out of the given lists"""
        import matplotlib.pyplot as plt
        from matplotlib.backends.backend_pdf import PdfPages


        pp = PdfPages("{0}-{1}-{2}-{3}-{4}.pdf".format(app_name, app_arg.split(" ")[0], app_arg.split(" ")[1], app_arg.split(" ")[2], plot_key))

        fig, ax = plt.subplots()
        for objective in objectives:
            rows = query_manager.read_from_sqlite3(sqlite3_db_file, app_name, app_arg, hpx_threads, objective)
            threads = [rec['initial_threads'] for rec in rows] 

            values = [rec[plot_key] for rec in rows]

            if objective in ["time", "resource"]:
                min_threads = [rec['min_threads'] for rec in rows]
                stdev_threads = [round(rec['stdev_threads'], 2) for rec in rows]
#                i = 0
#                for x, y in zip(threads, values):
#                    ax.annotate(min_threads[i], xy=(x, y), xytext=(x+1, y +1))
#                    i += 1
            
            ax.plot(threads, values, marker="o", label = objective)
            ax.set_xlabel("Number of Threads")

            y_label = "Execution Time (Sec)"
            if plot_key == "energy":
                y_label = "Energy (Joule)"
            elif plot_key == "resource":
                y_label = "Resource ( num_threads * time ) "

            ax.set_ylabel(y_label)
            plt.xticks(threads)
            legend = ax.legend(loc='upper center', shadow=True, fontsize='small')
        
        plt.xlim(0, max(hpx_threads) * 1.2) 
        plt.grid(True)
        #FIXME make title parametric
#        plt.title("Execution time. [{0} {1}].\nAnnotations are the minimum number of threads during throttlig".format(app_name, app_arg), fontsize=11)
        plt.title("Experiments on {0} [{1} {2}]".format(arch, app_name, app_arg), fontsize=11)
        pp.savefig()
        pp.close()
        plt.close()


    def read_energy(self, file_name = "/sys/devices/system/cpu/occ_sensors/system/system-energy"):
        """Read power and energy sensors which are available on POWER8"""
        import os

        if not os.path.exists(file_name):
            logging.debug("Sensor file: {0} does not exist".format(file_name))
            energy = 0
        else:
            with open(file_name) as f:
                energy = int(f.read().split(" ")[0])

        return energy


    def read_config(self, json_file):
        """Reads confi from the provided json file and returns dict"""
        config = {}
        with open(json_file, 'r') as f:
            config = json.load(f)

        return config


    def collect_data(self, sqlite3_db_file, table_name, app_timeout, app_base_dir, app, app_arg, hpx_thread, hpx_queuing, objective):
        """Save time and energy benchmark results in sqlite3 db"""
        import statistics  # requires python3.4+

        active_threads = []

        if (objective != "none"):
            hpx_arg = "--hpx:threads={0} --hpx:queuing={1} --hpx:ini=allscale.objective!={2}".format(hpx_thread, hpx_queuing, objective)
        else:
            hpx_arg = "--hpx:threads={0} --hpx:queuing={1}".format(hpx_thread, hpx_queuing)
            
        command = "{0} {1} {2}".format(os.path.join(os.sep, app_base_dir, app), app_arg, hpx_arg) 

        exceptions = ["std::exception", "hpx::exception"]

        self.max_threads = hpx_thread
        start_energy = self.read_energy()
#        start_time = time.time()
        start_time = timer()

        resource_usage = 0

        #It is assumed that the scheduler prints out "... Active threads: num_threads"
        for resp in self.run_benchmark(command, app_timeout):
            for exception in exceptions:
                if exception in resp:
                    raise Exception("Error: {0}".format(resp))
            if "Active threads" in resp:
                active_threads.append(int(resp.split(':')[1].split('\\')[0]))

            if "Resource" in resp:
                resource_usage = float(resp.split(':')[1].split('\\')[0])

        print("Command ended with code: {0}".format(self.cmd_ret_code))

        if self.cmd_ret_code != 0:
            print("Command ended with code: {0}".format(self.cmd_ret_code))
            return -1

#        end_time = time.time()
        end_time = timer()
        exec_time = end_time - start_time
        end_energy = self.read_energy()

        params = {}
        params['app_name'] = app
        params['app_arg'] = app_arg
        params['time'] = exec_time
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

        if resource_usage == 0:
            # Being here means there were no throttling
            resource_usage = params['time'] * params['initial_threads']

        params['resource'] = resource_usage
        params['hpx_queuing'] = hpx_queuing
        params['objective'] = objective
        params['date'] = '{:%Y-%m-%d %H:%M:%S}'.format(datetime.datetime.now())

        if not self.db_created:
            query_manager.create_table(sqlite3_db_file, table_name)
            self.db_created = True

        query_manager.insert_query(sqlite3_db_file, params)

        return 0

