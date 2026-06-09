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

import json
import pandas as pd
import numpy as np
import random
from scipy.spatial.distance import cdist
from config_sampler import ConfigSampler


def best_candidate_algorithm_iteration(blue_set: np.array, candidates: np.array):
    """Iteration of the best candidate algorithm for generating blue noise using scipy."""
    distances = cdist(blue_set, candidates, metric='euclidean') # Calculate pairwise Euclidean distances
    min_distances = distances.min(axis=0)   # Find the minimum distance from each candidate to the blue set
    return np.argmax(min_distances)   # get the index of the furthest candidate.


if __name__ == "__main__":
    import argparse
    import os

    with open("data/config_translation.json", "r") as file:
        FORWARD_CONFIG_TRANSLATION = json.load(file)

    parser = argparse.ArgumentParser(
        prog="generate_database",
        description="""
            This is a utility script to be used to generate a database of configs which will be simulated for
            training data for the QoR models. Data can be generated uniformly (unbiased), or biased by a given
            benchmark suite. If a benchmark csv is provided, users can fit ordinal data to a
            lognormal distribution, and categorical data to a categorical distribution.
            """
    )
    parser.add_argument("-ip")
    parser.add_argument("--num_generated_configs", type=int, default=1000, help="number of configs to generate.")
    parser.add_argument("--num_partitions", type=int, default=1, help="number of partitions to break the dataset into.")
    parser.add_argument("--generated_csv_filepath", help="output path for generated csv file.")
    parser.add_argument("--checkpoint_pickup", help="If true, the script will comb the test dir for an existing database and append to it.")
    parser.add_argument("--seed", type=int, default=42, help="seed for random number generators.")
    parser.add_argument("--runmake", type=int, default=0, help="flag for if script is being piped into runmake.")
    args = parser.parse_args()

    ip_name = args.ip
    num_generated_configs = args.num_generated_configs
    num_partitions = args.num_partitions
    generated_csv_filepath = args.generated_csv_filepath
    checkpoint_pickup = args.checkpoint_pickup
    seed = args.seed
    runmake = args.runmake

    num_configs_per_partition = num_generated_configs // num_partitions

    random.seed(seed)
    np.random.seed(seed)


    with open("data/model_params.json", 'r') as file:
        MODEL_PARAMS = json.load(file)
        no_iterate_params = MODEL_PARAMS["no_iterate_params"]
        model_params = MODEL_PARAMS[ip_name]
        restricted_params = model_params["restricted"] if "restricted" in model_params else {}

    categorical_params = model_params["categorical"]
    ordinal_params = model_params["ordinal"]
    independent_params = categorical_params + ordinal_params

    sampler = ConfigSampler(ip_name, restricted_params, ordinal_params, categorical_params, no_iterate_params)

    # Initialising test_tool_canary and translations...
    test_canary_path = f"{sampler.tests_folder}/multi_params.json"
    with open(test_canary_path, 'r') as file:
        multi_params_dict = json.load(file)
        test_0_tool_canary_aie = multi_params_dict["test_0_tool_canary_aie"]

    REVERSE_CONFIG_TRANSLATION = {}
    for key in test_0_tool_canary_aie:
        if key in FORWARD_CONFIG_TRANSLATION:
            val = FORWARD_CONFIG_TRANSLATION[key]
            REVERSE_CONFIG_TRANSLATION[val] = key
    
    ordinal_coords = []
    generated_configs = []
    n_invalid_configs = 0
    n_duplicated_configs = 0
    n_bugged_configs = 0
    n_samples = 0

    database = {}
    if checkpoint_pickup:
        for file in os.listdir(sampler.tests_folder):
            if file.startswith("multi_params_database_"):
                print(f"Loading {file}")
                with open(f"{sampler.tests_folder}/{file}", "r") as file:
                    database |= json.load(file) # append partition
        
    for config in database:
        config_sample = {k:v for k,v in database[config].items() if k in FORWARD_CONFIG_TRANSLATION}
        config_sample = {FORWARD_CONFIG_TRANSLATION[k]:v for k,v in config_sample.items()}
        config_sample = {k:v for k,v in config_sample.items() if k in independent_params}
        config_sample = {k:(int(v) if v.isnumeric() else v) for k,v in config_sample.items()}
        generated_configs.append(config_sample)

        sample_ordinal = [v for k,v in config_sample.items() if k in ordinal_params]
        sample_ordinal = [np.log(v+1) for v in sample_ordinal]
        ordinal_coords.append(sample_ordinal)
    del database

    def generate_configs_df(configs_dict, start_idx, end_idx):
        generated_df = pd.DataFrame.from_dict(configs_dict)
        generated_df = generated_df.rename(columns=REVERSE_CONFIG_TRANSLATION)
        for key in test_0_tool_canary_aie:
            if key not in generated_df.columns:
                    generated_df[key] = test_0_tool_canary_aie[key]
        keep_cols = list(test_0_tool_canary_aie.keys())   # drop columns like "weights"
        generated_df = generated_df[keep_cols]

        generated_df["NITER"] = 16
        generated_df = generated_df.astype(str)
        generated_df["name"] = "test_"
        generated_df["name"] += [str(i) for i in range(start_idx, end_idx)]
        generated_df["name"] += "_CLEAN_aie"
        generated_df["name"] += generated_df["AIE_VARIANT"].apply(lambda x: str(x))
        generated_df["name"] += "_hw_database"
        generated_df = generated_df.set_index(generated_df["name"])
        generated_df = generated_df.drop(columns=["name"])
        return generated_df

    print("generating configs...")
    while len(generated_configs) < num_generated_configs:
        
        n_candidates = (len(generated_configs)) + 1
        config_candidates = []
        ordinal_candidates = []
        while len(config_candidates) < n_candidates:

            config_sample = sampler.sample_path()
            n_samples += 1

            if n_samples % 100000 == 0:
                print(f"{ip_name} : {n_samples} trialled. {len(generated_configs)} configs generated. {n_bugged_configs} bugged configs. {n_invalid_configs} invalid configs. {n_duplicated_configs} duplicate configs.")
            
            if config_sample == "BUGGED":
                n_bugged_configs +=1
                continue

            if not config_sample:
                n_invalid_configs += 1
                continue

            sample_ordinal = [v for k,v in config_sample.items() if k in ordinal_params]
            sample_ordinal = [np.log(v+1) for v in sample_ordinal]

            config_candidates.append(config_sample)
            ordinal_candidates.append(sample_ordinal)

        if len(generated_configs) == 0:
            ordinal_coords.append(sample_ordinal)
            generated_configs.append(config_sample)
            continue

        candidates = np.array(ordinal_candidates)
        existing = np.array(ordinal_coords)
        bluest_idx = best_candidate_algorithm_iteration(existing, candidates)

        if config_candidates[bluest_idx] in generated_configs:
            n_duplicated_configs += 1
            continue
        
        generated_configs.append(config_candidates[bluest_idx])
        ordinal_coords.append(ordinal_candidates[bluest_idx])

        if len(generated_configs) % num_configs_per_partition == 0:
            if generated_csv_filepath:
                generated_df = generate_configs_df(generated_configs, 0, len(generated_configs))
                generated_df.to_csv(generated_csv_filepath, index=False)
            
            end_idx = len(generated_configs)
            start_idx = end_idx - num_configs_per_partition
            file_idx = start_idx // num_configs_per_partition
            
            if runmake:
                filename = f"{sampler.tests_folder}/multi_params_random.json"
            else:   
                filename = f"{sampler.tests_folder}/multi_params_database_{file_idx}.json"
            
            generated_df = generate_configs_df(generated_configs[start_idx : end_idx], start_idx, end_idx)
            generated_dict = generated_df.to_dict(orient="index")
            with open(filename, "w") as json_file:
                json.dump(generated_dict, json_file, indent=4)
        
    print(f"{ip_name} : {n_samples} trialled. {len(generated_configs)} configs generated. {n_bugged_configs} bugged configs. {n_invalid_configs} invalid configs. {n_duplicated_configs} duplicate configs.")