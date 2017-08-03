import datetime
import sqlite3
import sys


def dict_factory(cursor, row):
    """Lets us to get results as dictionary with column names being keys.
       See https://stackoverflow.com/questions/3300464/how-can-i-get-dict-from-sqlite-query.
    """
    d = {}
    for idx, col in enumerate(cursor.description):
        d[col[0]] = row[idx]
    return d


def create_table(sqlite3_file, table_name):
    """Creates sqlite3 table with predefined fields"""
    table_fields = """
                   app_name TEXT, 
                   app_args TEXT, 
                   exec_time FLOAT, 
                   energy INTEGER, 
                   power INTEGER, 
                   initial_threads INTEGER, 
                   min_threads INTEGER, 
                   max_threads INTEGER,
                   mean_threads FLOAT,
                   mode_threads FLOAT,
                   stdev_threads FLOAT,
                   hpx_queuing TEXT,
                   objective TEXT,
                   date TEXT
               """

    create_table_query = "CREATE TABLE {tn} ({tf})".format(tn = table_name, tf = table_fields)
    print(create_table_query)

    connection = sqlite3.connect(sqlite3_file)
    cursor = connection.cursor()
    cursor.execute(create_table_query)
    connection.close()



def insert_query(sqlite3_file, params):
    """Create sqlite3 insert query that inserts benchmark inputs and outputs into a table"""

    values = (\
              params['app_name'],\
              params['app_arg'],\
              params['exec_time'],\
              params['energy'],\
              params['power'],\
              params['initial_threads'],\
              params.get('min_threads', params['initial_threads']),\
              params.get('max_threads', params['initial_threads']),\
              params.get('mean_threads', params['initial_threads']),\
              params.get('mode_threads', params['initial_threads']),\
              params.get('stdev_threads', 0),\
              params['hpx_queuing'],\
              params['objective'],\
              params['date']\
             )

    insert_query = "INSERT INTO SCHEDULER VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?)"

    connection = sqlite3.connect(sqlite3_file)

    with connection:
        cursor = connection.cursor()
        cursor.execute(insert_query, values)

    connection.close()
    


def read_from_sqlite3(sqlite3_db_file, app_name, app_arg, hpx_threads, objective):
    """Selects data from sqlite3 and returns it in a list of dictionaries"""

    select_query = """SELECT
                          app_name, 
                          app_args,
                          initial_threads,
                          exec_time,
                          min_threads,
                          mean_threads,
                          stdev_threads,
                          energy,
                          objective
                      FROM scheduler WHERE
                          app_name=? AND
                          app_args=? AND
                          objective=? AND
                          initial_threads in ({0})
                   """
    connection = sqlite3.connect(sqlite3_db_file)
    connection.row_factory = dict_factory
    cursor = connection.cursor()
    params = [app_name, app_arg, objective]
    params.extend(hpx_threads)
    in_params = "?," * len(hpx_threads)
    cursor.execute(select_query.format(in_params[:-1]), params)
    all_rows = cursor.fetchall()
    connection.close()

    return all_rows

