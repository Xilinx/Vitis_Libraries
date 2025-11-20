#
# Copyright (C) 2019-2022, Xilinx, Inc.
# Copyright (C) 2022-2025, Advanced Micro Devices, Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
import argparse
import json
import pandas as pd
import models
from models import LinearRegressionPipeline, PolynomialRegressionPipeline, SVRPipeline, RandomForestPipeline
import random
import collections
import logging

from pathlib import Path
import sys
sys.path.append(str(Path.cwd().parent))
import metadata_api

# TODO: SINGLE_BUF support

logging.basicConfig(level=logging.INFO)
file_handler = logging.FileHandler("error_log.log")
file_handler.setLevel(logging.ERROR)
logger = logging.getLogger()
logger.setLevel(logging.INFO)
logger.addHandler(file_handler)

def generate_configs(ip_name, constraints_from_file, flavor="recursive", N_explore=1000, N_legal=10000):
    """ 
    Generates legal configs for a given IP.
    There are two flavors: 'recursive' and 'random'.

    Recursive: Given a set of constraints, will generate all configs which fit said constraints. This works well for IPs where this number is tractable.
    Random: Explores an IP via randomly sampling with updaters to define the limits of a config hypercube. This hypercube is then uniformly sampled from until the desired number of legal configs is reached.

    Parameters:
        - ip_name: Name of the IP
        - constraints_from_file: The constraints dictionary
        - flavor: Must be one of "recursive" or "random"
        - N_explore: The number of random walks to take in the exploration phase. Only relevant for flavor="random"
        - N_legal: The number of legal configs to randomly select. Only relevant for flavor="random"
    """
    def recursive_update(config, recursive_lvl):
        if recursive_lvl >= len(ip.params): # base case
            legal_configs.append(dict(config))
            if len(legal_configs) % 100000 == 0:
                print(f"{len(legal_configs)} generated...")
            return
        
        # else...
        param = ip.params[recursive_lvl]
        ip.param_obj_dict[param].param_update(ip.args, ip.module)

        if "enum" in ip.param_obj_dict[param].update_result:
            legal_vals = set(ip.param_obj_dict[param].update_result["enum"])
        elif "minimum" in ip.param_obj_dict[param].update_result:
            min_val = int(ip.param_obj_dict[param].update_result["minimum"])
            max_val = int(ip.param_obj_dict[param].update_result["maximum"])
            legal_vals = range(min_val, max_val+1)
        elif "len" in ip.param_obj_dict[param].update_result:
            len_ = int(ip.param_obj_dict[param].update_result["len"])
            legal_vals = {range(len_)}

        if param in no_validation_params:
            legal_vals = {next(iter(legal_vals))}   # modify legal vals to only have one value.

        elif constraints[param]:  # if there exists constraints for this parameter...
            if isinstance(legal_vals, range):
                legal_vals = constraints[param]
            else:
                legal_vals = legal_vals & constraints[param]    # take the intersection of legal vals and constraints.

        for val in legal_vals:  # walking through values...
            ip.args[param] = val
            ip.param_obj_dict[param].param_validate(ip.args, ip.module, print_err=0)
            if ip.param_obj_dict[param].valid == "True":
                config[param] = ip.args[param]
                recursive_update(config, recursive_lvl+1)   # recursive call to next parameter
                if len(legal_configs) == 1000:  # hard limit
                    return
    
    ip = metadata_api.IP(ip_name, {})
    for k,v in constraints_from_file.items():
        if not isinstance(v, list):
            constraints_from_file[k] = [v]

    constraints = { param:{} for param in ip.params }
    for k,v in constraints_from_file.items():
        if k in constraints:
            constraints[k] = set(v)

    no_performance_impact_set = {"TP_SHIFT", "TP_RND", "TP_SAT"}
    no_performance_impact_params = no_performance_impact_set & set(ip.params)  # adding TP_SHIFT, TP_RND, TP_SAT to iteration..
    hidden_params = {param for param in ip.params if any(char.islower() for char in param)} # not exposed to user.
    no_validation_params = hidden_params | no_performance_impact_params

    legal_configs = []

    if flavor == "recursive":
        recursive_update({}, 0) # first call to recursive function.
    else:
        raise ValueError("ERROR: flavor must be one of 'recursive'.")

    configs_df = pd.DataFrame.from_records(legal_configs, columns=legal_configs[0].keys())
    return configs_df


if __name__ == "__main__":
    import os
    import subprocess

    DEFAULT_CONSTRAINTS_DIR = str(Path.cwd()) + "/constraints/"
    DEFAULT_TRAINING_DIR = str(Path.cwd()) + "/../../../../docs/src/csv_data_files/L2/"

    parser = argparse.ArgumentParser(
        prog="config_qor_helper",
        description="""
            This script generates configs for an IP, ranked by NUM_AIE, which fulfill user constraints. Please see the 
            examples inside the constraints folder on how to structure a set of constraints, or read README.md for 
            detailed instructions. QoR estimates come from a machine learned model and therefore estimation error
            exists. This script will handle the training of a model, if one does not currently exist for the IP.
            """
    )
    parser.add_argument("--ip")
    parser.add_argument("--constraints_file", default="", help="path and name of constraints json file")
    parser.add_argument("--out_csv_file", default="", help="csv file to print all valid configs")
    parser.add_argument("--out_graphs_dir", default="", help="directory to generate top graphs")
    parser.add_argument("--model", default="linear", help="model type. can be one of [linear, polynomial, random_forest, svr]")
    parser.add_argument("--num_configs", type=int, default=10, help="# top configs to print to generate graphs for")
    parser.add_argument("--training_csv_path", default="", help="path to training data")
    args = parser.parse_args()

    ip_name = args.ip           # e.g. "bitonic_sort"
    model_name = args.model     # e.g. "polynomial"
    constraints_file = args.constraints_file
    out_csv_file = args.out_csv_file
    out_graphs_dir = args.out_graphs_dir

    if constraints_file == "":
        constraints_file = f"{DEFAULT_CONSTRAINTS_DIR}empty_constraints.json"
        logging.warning(f"No constraints_file defined. Will default to {constraints_file}")

    # Define and load the ML model.
    if not os.path.exists(f"models/{ip_name}_{model_name}.pkl"):
        logging.info(f"No {model_name} model file. Training model.")
        training_csv_path = args.training_csv_path
        if training_csv_path == "":
            training_csv_path = f"{DEFAULT_TRAINING_DIR}{ip_name}_benchmark.csv"
            logging.warning(f"No training_csv_path defined. Will default to {training_csv_path}")
        subprocess.run(["python", "models.py", "--ip", ip_name, "--model", model_name, "--training_csv_path", training_csv_path])
        
    model = models.load_model(f"models/{ip_name}_{model_name}.pkl")
    
    # Set the application and QoR constraints.
    with open(constraints_file, 'r') as file:
        constraints = json.load(file)
    parameter_constraints = constraints["parameter_constraints"]
    qor_constraints = constraints["qor_constraints"]

    # Generate constrained configs, predict QoR and return ranked list.
    column_ordering = model.independent_cols
    configs_df = generate_configs(ip_name, parameter_constraints)[column_ordering].drop_duplicates()

    if len(configs_df) == 0:
        logging.warning("No valid configs given the parameter constraints.")

    aie_mapper = {1: "AIE", 2: "AIE-ML", 22: "AIE-MLv2"}
    aie_reverse_mapper = {v:k for k,v in aie_mapper.items()}
    configs_df["AIE_VARIANT"] = configs_df["AIE_VARIANT"].apply(lambda x: aie_mapper[x])
    qor_df = model.predict(configs_df).clip(lower=1)
    synthetic_df = pd.concat([configs_df.reset_index(drop=True), qor_df], axis=1)
    synthetic_constrained_df = synthetic_df.query(qor_constraints)
    synthetic_ranked_df = synthetic_constrained_df.sort_values(by='NUM_AIE').reset_index(drop=True)

    logging.info(f"Top configurations ranked by NUM_AIE:\n{synthetic_ranked_df.head(args.num_configs)}")

    if out_csv_file != "":
        logging.info(f"All valid configurations ranked by NUM_AIE printed to {out_csv_file}.")
        synthetic_ranked_df.to_csv(out_csv_file)

    if out_graphs_dir != "":
        os.makedirs(out_graphs_dir, exist_ok=True )
        ip = metadata_api.IP(ip_name, {})
        ip.args = { param:0 for param in ip.params }    # default values
        
        ranked_dicts = synthetic_ranked_df.head(args.num_configs).to_dict('records')
        for i,config in enumerate(ranked_dicts):
            for param in config:
                ip.args[param] = config[param]

            ip.args["AIE_VARIANT"] = aie_reverse_mapper[ip.args["AIE_VARIANT"]]
            # TODO: Support needed to nudge TP_SHIFT, TP_RND, TP_SAT to default values

            ip.generate_graph(i)
            with open(f"{out_graphs_dir}/{i}.txt", 'w') as file:
                file.write(ip.graph["graph"])

        logging.info(f"Top {args.num_configs} configuration graphs generated in {out_graphs_dir}/.")
