#
# Copyright (C) 2019-2022, Xilinx, Inc.
# Copyright (C) 2022-2026, Advanced Micro Devices, Inc.
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
import math
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

def convert_to_fixed_notation(value):
    magnitude = math.floor(math.log10(value))     # determines how many digits are before the decimal point.
    decimals = max(2 - magnitude, 0)                    # determines how many digits are after the decimal point.
    rounded = round(value, 2 - magnitude)
    return f"{rounded:.{decimals}f}"

def parse_qor_query(query):
    query = query.strip()   # removs leading/trailing whitespace.
    operator, value = query[0], query[1:]
    if operator not in ['<', '>']:
        raise ValueError(f"Invalid operator {operator} in QoR query. Must be '<' or '>'.")
    return operator, value 

DEFAULT_METRICS = ["steady shortest latency (us)", "throughput (MSa/s)", "num aie"]
DEFAULT_SORT_BY = {"num aie": True} # Ordered dictionary. True=ascending, False=descending.

if __name__ == "__main__":

    parser = argparse.ArgumentParser(
        prog="config_qor_helper",
        description="""
            This script generates configs for an IP, ranked by num aie by default, which fulfill user constraints. 
            Please see the examples inside the constraints folder on how to structure a set of constraints, or read 
            README.md for detailed instructions. QoR estimates come from a machine learned model and therefore 
            estimation error exists.
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
    parameter_constraints = constraints.get("parameter_constraints") or {}
    qor_constraints = constraints.get("qor_constraints") or {}
    sort_by = constraints.get("sort_by") or DEFAULT_SORT_BY
    confidence = constraints.get("confidence") or 1
    verbose = constraints.get("verbose") or False

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

    for k,v in sort_by.items():
        if isinstance(v, bool):
            continue
        elif v == "ascending":
            sort_by[k] = True
        elif v == "descending":
            sort_by[k] = False
        else:
            raise RuntimeError(f"Sort order for `{k}` must be either 'ascending'/True or 'descending'/False.")

    # Generate constrained configs, predict QoR and return ranked list.
    sampler = ConfigSampler(ip_name, constraints_intersection, ordinal_params, categorical_params, no_iterate_params)

    # Defensive checks...
    for param in parameter_constraints:
        if param not in sampler.params:
            raise RuntimeError(f"Parameter constraint `{param}` is not a valid parameter for `{ip_name}`. Valid parameters are {sampler.params}.")
    for param, constraint in qor_constraints.items():
        if param not in dependent_params:
            raise RuntimeError(f"QoR constraint `{param}` is not a valid QoR parameter for `{ip_name}`. Valid QoR parameters are {dependent_params}.")
        parse_qor_query(constraint)   # Defensive check to ensure that QoR constraints are valid.
    for param in sort_by:
        if param not in dependent_params:
            raise RuntimeError(f"Sort parameter `{param}` is not a valid QoR parameter for `{ip_name}`. Valid QoR parameters are {dependent_params}.")
    if confidence < 0:
        raise RuntimeError(f"Confidence value {confidence} is invalid. Must be a non-negative float.")

    # Set up which parameters to display
    if verbose:
        metrics = dependent_params
    else:
        metrics = set(DEFAULT_METRICS) | set(qor_constraints.keys()) | set(sort_by.keys())
        metrics = [metric for metric in dependent_params if metric in metrics]

    configs_df = sampler.generate_configs(config_limit)
    config_params = configs_df.columns.tolist()

    if len(configs_df) == 0:
        raise RuntimeError("No valid configurations given the parameter constraints provided. Ensure constraints can support a legal configuration and do not violate conditions on restricted parameters.")
    logging.info(f"{len(configs_df)} configs found which adhere to parameter constraints.")

    print("Config Sample limits...")
    for param in ordinal_params:
        print(f"{param}: [{min(configs_df[param])} - {max(configs_df[param])}]")
    print("Note: limits are constrained as a consequence of parameter constraints, the config_limit, or the boundary of the training data.")

    qor_df = model.predict(configs_df)
    for col in qor_df.columns:
        if col in ["num aie", "num banks"]:
            qor_df[col] = qor_df[col].clip(lower=1).round().astype(int)

    # err_stdev is the residual stdev in each column's own y_process_pipe output space
    # (log_transform then StandardScaler), so bounds must be built in that
    # same space before inverse-transforming back to real units.

    qor_model_space = {}
    for col in qor_df.columns:
        qor_model_space[col] = model.y_process_pipes[col].transform(qor_df[[col]]).ravel()
    qor_model_space = pd.DataFrame(qor_model_space, index=qor_df.index)

    def compute_limit(model_space: pd.DataFrame, confidence_interval: pd.Series) -> pd.DataFrame:
        df_lim = {}
        for col in qor_df.columns:
            y_pred = model_space[[col]] + confidence_interval[col]
            y_pred_trans = model.y_process_pipes[col].inverse_transform(y_pred.values).ravel()
            df_lim[col] = y_pred_trans
        df_lim = pd.DataFrame(df_lim, index=qor_df.index)
        return df_lim.clip(lower=0)

    confidence_interval = model.err_stdev * confidence
    qor_lower = compute_limit(qor_model_space, -confidence_interval)
    qor_upper = compute_limit(qor_model_space, +confidence_interval)
    qor_range = pd.DataFrame(
        np.stack([qor_lower.values, qor_upper.values], axis=-1).tolist(),
        columns=qor_df.columns,
        index=qor_df.index
    )
    for col in qor_range.columns:
        if col in ["num aie", "num banks"]:
            qor_range[col] = qor_range[col].apply(lambda x: [round(x[0]), round(x[1])])
        else:
            qor_range[col] = qor_range[col].apply(lambda x: [convert_to_fixed_notation(x[0]), convert_to_fixed_notation(x[1])])

    for param, constraint in qor_constraints.items():
        operator, value = parse_qor_query(constraint)
        if operator == '<':
            qor_df[param] = qor_upper[param]
        elif operator == '>':
            qor_df[param] = qor_lower[param]

    synthetic_df = pd.concat([configs_df.reset_index(drop=True), qor_df], axis=1)
    for param, constraint in qor_constraints.items():
        QUERY = f"`{param}`{constraint}"
        synthetic_df = synthetic_df.query(QUERY)

    if len(synthetic_df) == 0:
        raise RuntimeError("No configurations adhere to the qor constraints provided.")

    synthetic_ranked_df = synthetic_df.sort_values(by=list(sort_by.keys()), ascending=list(sort_by.values()))
    synthetic_ranked_df[qor_df.columns] = qor_range
    synthetic_ranked_df = synthetic_ranked_df.reset_index(drop=True)

    if out_graphs_dir != "":
        os.makedirs(out_graphs_dir, exist_ok=True)
        ranked_configs = synthetic_ranked_df[config_params].head(num_graphs).to_dict('records')
        for i, config in enumerate(ranked_configs):
            graph_code = sampler.generate_graph(f"{ip_name}_config_{i}", config)["graph"]
            with open(f"{out_graphs_dir}/{i}.hpp", 'w') as file:
                file.write(graph_code)
        logging.info(f"Top {len(ranked_configs)} configuration graphs generated in {out_graphs_dir}/.")

    if out_csv_file != "":
        logging.info(f"All valid configurations sort and filtered printed to {out_csv_file}.")
        synthetic_ranked_df.to_csv(out_csv_file, index=False)

    iterable_config_params = [param for param in config_params if param not in no_iterate_params]
    synthetic_ranked_df = synthetic_ranked_df[iterable_config_params + metrics]

    logging.info(f"{len(synthetic_ranked_df)} remaining configs which adhere to QoR constraints.")
    logging.info(f"Top configurations sort and filtered:\n{synthetic_ranked_df.head(num_graphs)}")