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
from models import *
from config_sampler import ConfigSampler
import logging

logging.basicConfig(level=logging.INFO)
file_handler = logging.FileHandler("error_log.log")
file_handler.setLevel(logging.ERROR)
logger = logging.getLogger()
logger.setLevel(logging.INFO)
logger.addHandler(file_handler)


if __name__ == "__main__":

    parser = argparse.ArgumentParser(
        prog="config_qor_helper",
        description="""
            This script generates configs for an IP, ranked by NUM_AIE, which fulfill user constraints. Please see the 
            examples inside the constraints folder on how to structure a set of constraints, or read README.md for 
            detailed instructions. QoR estimates come from a machine learned model and therefore estimation error
            exists.
            """
    )
    parser.add_argument("--ip", required=True, help="IP name")
    parser.add_argument("--constraints_file", required=True, help="filepath of constraints json file.")
    parser.add_argument("--out_csv_file", default="", help="csv file to print all valid configs")
    parser.add_argument("--out_graphs_dir", default="", help="directory to generate top graphs")
    parser.add_argument("--num_graphs", type=int, default=10, help="number of graphs to generate")
    parser.add_argument("--config_limit", type=int, default=100000, help="maximum number of configs to generate.")
    args = parser.parse_args()

    ip_name = args.ip           # e.g. "bitonic_sort"
    constraints_file = args.constraints_file
    out_csv_file = args.out_csv_file
    out_graphs_dir = args.out_graphs_dir
    num_graphs = args.num_graphs
    config_limit = args.config_limit
    model_name = "polynomial"

    with open("data/model_params.json", 'r') as file:
        MODEL_PARAMS = json.load(file)
        model_params = MODEL_PARAMS[ip_name]
        no_iterate_params = MODEL_PARAMS["no_iterate_params"]

    categorical_params = model_params["categorical"]
    ordinal_params = model_params["ordinal"]
    dependent_params = model_params["dependent"]
    restricted_params = model_params["restricted"] if "restricted" in model_params else {}
    restricted_params = {param: set(restricted_params[param]) for param in restricted_params}

    for param, vals in restricted_params.items():
        print(f"{param} is restricted to {vals}.")

    # Define and load the ML model.
    model = models.load_model(f"models/{ip_name}_{model_name}.pkl")
    
    with open(constraints_file, 'r') as file:
        constraints = json.load(file)
    parameter_constraints = constraints["parameter_constraints"]
    qor_constraints = constraints["qor_constraints"].lower()
    confidence = constraints["confidence"]

    for k,v in dict(parameter_constraints).items():
        if not isinstance(v, list):
            parameter_constraints[k] = [v]
        if len(parameter_constraints[k]) == 0:
            del parameter_constraints[k]

    constraints_intersection = parameter_constraints | restricted_params
    for k,v in constraints_intersection.items():
        if k in parameter_constraints:
            constraints_intersection[k] = sorted(set(constraints_intersection[k]) & set(parameter_constraints[k]))
        if k in restricted_params:
            constraints_intersection[k] = sorted(set(constraints_intersection[k]) & set(restricted_params[k]))
        if not constraints_intersection[k]:
            constraints_intersection[k] = []

    if "sort_by" in constraints:
        sort_by = constraints["sort_by"]
        if not isinstance(sort_by, list):
            sort_by = [sort_by]
        sort_by = [col.lower() for col in sort_by]
    else:
        sort_by = ["num_aie"]
    
    if "ascending" in constraints:
        ascending = constraints["ascending"]
        if not isinstance(ascending, list):
            ascending = [ascending]
    else:
        ascending = [True]

    # Generate constrained configs, predict QoR and return ranked list.
    sampler = ConfigSampler(ip_name, constraints_intersection, ordinal_params, categorical_params, no_iterate_params)
    configs_df = sampler.generate_configs(config_limit)
    configs_df = configs_df.drop(columns=no_iterate_params, axis=1, errors="ignore")

    if len(configs_df) == 0:
        raise RuntimeError("No valid configurations given the parameter constraints provided. Ensure constraints can support a legal configuration and do not violate conditions on restricted parameters.")
    logging.info(f"{len(configs_df)} configs found which adhere to parameter constraints.")

    print("Config Sample limits...")
    for param in ordinal_params:
        print(f"{param}: [{min(configs_df[param])} - {max(configs_df[param])}]")
    print("Note: limits are constrained as a consequence of parameter constraints, the config_limit, or the boundary of the training data.")

    qor_df = model.predict(configs_df)
    qor_df.columns = qor_df.columns.str.lower()

    def compute_limit(df: pd.DataFrame, confidence_interval: np.array):
        df_log = np.log(df + 1)
        df_lim = df_log + confidence_interval
        df_lim = (np.exp(df_lim) - 1).clip(lower=1).round().astype(int)
        return df_lim

    confidence_interval = model.err_stdev * confidence
    confidence_interval.index = confidence_interval.index.str.lower()
    qor_lower = compute_limit(qor_df, -confidence_interval)
    qor_upper = compute_limit(qor_df, +confidence_interval)
    qor_range = pd.DataFrame(
        np.stack([qor_lower.values, qor_upper.values], axis=-1).tolist(),
        columns=qor_df.columns,
        index=qor_df.index
    )

    qor_constraint_components = qor_constraints.split()
    for component in qor_constraint_components:     # Schema is "*parameter**operator**value*" e.g "throughput>500"
        if '<' in component:                        # TODO: More robust query checking?
            operator_idx = component.index('<')
            parameter = component[:operator_idx]
            qor_df[parameter] = qor_upper[parameter]
        elif '>' in component:
            operator_idx = component.index('>')
            parameter = component[:operator_idx]
            qor_df[parameter] = qor_lower[parameter]

    synthetic_df = pd.concat([configs_df.reset_index(drop=True), qor_df], axis=1)
    synthetic_df.columns = synthetic_df.columns.str.lower()
    synthetic_constrained_df = synthetic_df.query(qor_constraints)

    if len(synthetic_constrained_df) == 0:
        raise RuntimeError("No configurations adhere to the qor constraints provided.")

    synthetic_ranked_df = synthetic_constrained_df.sort_values(by=sort_by, ascending=ascending)
    synthetic_ranked_df[qor_df.columns] = qor_range
    synthetic_ranked_df = synthetic_ranked_df.reset_index(drop=True)

    logging.info(f"{len(synthetic_ranked_df)} remaining configs which adhere to QoR constraints.")
    logging.info(f"Top configurations sort and filtered:\n{synthetic_ranked_df.head(num_graphs)}")

    if out_csv_file != "":
        logging.info(f"All valid configurations sort and filtered printed to {out_csv_file}.")
        synthetic_ranked_df.to_csv(out_csv_file, index=False)

    # if out_graphs_dir != "":
    #     os.makedirs(out_graphs_dir, exist_ok=True )
    #     ip = metadata_api.IP(ip_name, {})
        
    #     ranked_dicts = synthetic_ranked_df.head(num_graphs).to_dict('records')
    #     for i,config in enumerate(ranked_dicts):
    #         config["AIE_VARIANT"] = aie_reverse_mapper[config["AIE_VARIANT"]]
    #         config_df = generate_configs(ip_name, ip.params, config, 1)
    #         config_graph = config_df.to_dict('records')[0]
            
    #         for param in config_graph:
    #             ip.args[param] = config_graph[param]

    #         ip.generate_graph(i)
    #         with open(f"{out_graphs_dir}/{i}.txt", 'w') as file:
    #             file.write(ip.graph["graph"])

    #     logging.info(f"Top {num_graphs} configuration graphs generated in {out_graphs_dir}/.")
