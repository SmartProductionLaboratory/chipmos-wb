import json
import os
import re
import pandas as pd
import argparse
import copy
import signal
import subprocess as sp
from datetime import datetime



def filePreprocessing(path, files:dict, csv_config):

    for file in files:
        file_path = os.path.join(path, file["filename"])
        file_function = file["function"]

        if(not file_function in csv_config):
            csv_config[file_function] = []
        try:
            if(re.search(r'.csv', file_path) != None):
                df = pd.read_csv(file_path, dtype=str, encoding = "utf-8")
                file_path = file_path.replace(r'.csv', '')
            elif(re.search(r'.xlsx', file_path) != None):
                df = pd.read_excel(file_path, dtype=str)
                file_path = file_path.replace(r'.xlsx', '')
        except ValueError as e:
            df = pd.read_csv(file_path, dtype=str, encoding='ISO-8859-1')
            # print(e)
            # print("file path : ", file_path)
            # exit(-1)


        df = df.rename(columns=lambda x : x.strip())

        if "columns" in file:
            df = df[file["columns"]]
            for col in file["columns"]:
                df[col] = df[col].str.strip()
        
        df.dropna(
                    axis=0,
                    how='all',
                    inplace=True
                )
        df.to_csv(file_path + ".csv", index=False)
        csv_config[file_function].append(file_path + ".csv")
    return csv_config

def processing(conf:dict):
    all_df = []
    for entry in conf:
        dfs = preprocess_entry(entry)
        all_df.append(pd.concat(dfs))
    
    all_df = pd.concat(all_df)
    all_df.to_csv("config.csv", index=False)

def product(args, repeat=1):
    pools = [tuple(pool) for pool in args] * repeat
    result = [[]]
    for pool in pools:
        result = [x+[y] for x in result for y in pool]
    for prod in result:
        yield tuple(prod)

def preprocess_entry(conf:dict):
    csv_config = {}
    path = conf["file_path"]
    no = conf["no"]
    csv_config["no"] = conf["no"]
    # csv_config_file_path = "config.csv"
    csv_config = filePreprocessing(path, conf["preprocess_files"], csv_config)
    csv_config = filePreprocessing(path, conf["nopreprocess_files"], csv_config)

    wip_filename = csv_config["wip"][0]
    time = re.split(r"_|\.csv", wip_filename)[1]
    dt = datetime.strptime(time, "%Y%m%d%H%M%S")

    csv_config["std_time"] = [dt.strftime("%y-%m-%d %H:%M")]
    items = []
    params = []
    for item in conf["parameters"]:
        items.append(item)
        params.append(conf["parameters"][item])
    print("params : ")
    print(params)
    results = list(product(params))
    dfs = []
    no_suffix = 0
    for result in results:
        new_csv_config = copy.deepcopy(csv_config)
        new_csv_config["no"] = ''.join([str(no), "-", str(no_suffix)])
        new_csv_config["lots"] = os.path.join("output_" + new_csv_config["no"], "lots.csv");
        no_suffix += 1
        size = len(items)
        print(result)
        for i in range(size):
            new_csv_config[items[i]] = result[i]
        df = pd.DataFrame(new_csv_config)
        dfs.append(df)
    return dfs

def select():
    conf = pd.read_csv("config.csv")
    indi = pd.read_csv("indicator.csv")

    indi = indi.drop(indi[indi.Setup > 168].index)
    indi["Utilization"] = indi["Utilization"].replace({'%':''}, regex=True)
    indi["Utilization"] = indi["Utilization"].astype(float)

    best = indi.nlargest(3, "Utilization").index.tolist()
    conf = conf.loc[best]
    conf.to_csv("config.csv", index=False)

def run(cmd, timeout):
    proc = sp.Popen(cmd.split())
    try:
        outs, errs = proc.communicate(timeout=timeout)
    except sp.TimeoutExpired:
        if os.name == 'posix':
            proc.send_signal(signal.SIGTSTP)
        else:
            proc.send_signal(signal.SIGTERM)
        outs, errs = proc.communicate()

def exec(time_limit):
    half_time = time_limit / 2
    run('./main -f=config.csv', half_time)
    indicator = sp.Popen('./indic')
    indicator.wait()
    select()
    run('./main -f=config.csv', half_time)
    indicator = sp.Popen('./indic')
    indicator.wait()

if __name__ == '__main__':
    arg_parser = argparse.ArgumentParser()
    exegroup = arg_parser.add_mutually_exclusive_group() 

    exegroup.add_argument("-nr", "--norun", 
            help="do not run the algorithm directly", 
            action="store_false")

    exegroup.add_argument("-r", "--run", type=int,
            help="run the algorithm directly")

    plotgroup = arg_parser.add_mutually_exclusive_group()
    
    plotgroup.add_argument("-p", "--plot",
            help="plot the result of scheduling",
            action="store_true")

    plotgroup.add_argument("-np", "--noplot", 
            help="Plot the result of scheduling", 
            action="store_false")

    arg_parser.add_argument("config", help="Specify the config file")
    
    arg_parser.add_argument("-e", "--bin", 
            help="Specify the algorithm binary file", default="main")
    args = arg_parser.parse_args()

    config = args.config
    csv_config_file_path = processing(json.load(open(config))) 

    if args.run:
        time_limit = args.run * 60
        exec(time_limit)
