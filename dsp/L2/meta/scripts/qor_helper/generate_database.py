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
from scipy.spatial.distance import cdist
from config_sampler import ConfigSampler
from models import declare_model, preprocess_dataframe

def generate_configs_df(configs_dict, reverse_translation, canary_test, niter, clean_test):
    generated_df = pd.DataFrame.from_dict(configs_dict)
    generated_df = generated_df.rename(columns=reverse_translation)
    for key in canary_test:
        if key not in generated_df.columns:
                generated_df[key] = canary_test[key]
    keep_cols = list(canary_test.keys())   # drop columns like "weights"
    generated_df = generated_df[keep_cols]
    generated_df["NITER"] = niter
    generated_df["CLEAN_TEST"] = int(clean_test)
    generated_df = generated_df.astype(str)
    generated_df["name"] = "test_"
    generated_df["name"] += [str(i) for i in range(len(generated_df))]
    generated_df["name"] += "_aie"
    generated_df["name"] += generated_df["AIE_VARIANT"].apply(lambda x: str(x))
    generated_df["name"] += "_hw_qor"
    generated_df = generated_df.set_index(generated_df["name"])
    generated_df = generated_df.drop(columns=["name"])
    return generated_df

def best_candidate_algorithm_iteration(blue_set: np.array, candidates: np.array):
    """Iteration of the best candidate algorithm for generating blue noise using scipy."""
    distances = cdist(blue_set, candidates, metric='euclidean') # Calculate pairwise Euclidean distances
    min_distances = distances.min(axis=0)   # Find the minimum distance from each candidate to the blue set
    return np.argmax(min_distances)   # get the index of the furthest candidate.


if __name__ == "__main__":
    import argparse
    import os

    parser = argparse.ArgumentParser(
        prog="generate_database",
        description="""
            This is a utility script to be used to generate a database of configs which will be simulated for
            training data for the QoR models. Data can be generated uniformly or according to a blue distribution in 
            vector embedded space.
            """
    )
    parser.add_argument("-ip") 
    parser.add_argument("--num_generated_configs", type=int, default=1000, help="number of configs to generate.")
    parser.add_argument("--generated_csv_filepath", help="output path for generated csv file.")
    parser.add_argument("--niter", type=int, default=16, help="number of iterations to run for each config.")
    parser.add_argument("--clean_test", action="store_true", default=False, help="flag for if clean test should be run.")
    parser.add_argument("--runmake", action="store_true", default=False, help="flag for if script is being piped into runmake.")
    parser.add_argument("--uniform", action="store_true", default=False, help="enumerate all legal configs exhaustively then sample uniformly at random.")
    parser.add_argument("--blue", action="store_true", default=True, help="run blue noise in preprocessor embedding space (encodes categorical params).")
    args = parser.parse_args()

    ip_name = args.ip
    num_generated_configs = args.num_generated_configs
    generated_csv_filepath = args.generated_csv_filepath
    runmake = args.runmake
    niter = args.niter

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

    with open("data/config_translation.json", "r") as file:
        FORWARD_CONFIG_TRANSLATION = json.load(file)

    REVERSE_CONFIG_TRANSLATION = {}
    for key in test_0_tool_canary_aie:
        if key in FORWARD_CONFIG_TRANSLATION:
            val = FORWARD_CONFIG_TRANSLATION[key]
            REVERSE_CONFIG_TRANSLATION[val] = key
    

    if args.uniform:
        print("Enumerating all legal configs exhaustively...")
        all_configs_df = sampler.generate_configs(config_limit=10_000_000)
        print(f"Total legal configs found: {len(all_configs_df)}")
        sampled_df = all_configs_df.sample(n=num_generated_configs)
        generated_configs = sampled_df.to_dict(orient='records')
        print(f"Sampled {len(generated_configs)} configs uniformly at random.")

        generated_df = generate_configs_df(generated_configs, REVERSE_CONFIG_TRANSLATION, test_0_tool_canary_aie, niter, args.clean_test)
        filename = f"{sampler.tests_folder}/multi_params_database_uniform.json"
        generated_dict = generated_df.to_dict(orient="index")
        with open(filename, "w") as json_file:
            json.dump(generated_dict, json_file, indent=4)
        print(f"Saved to {filename}")
        if generated_csv_filepath:
            generated_df.to_csv(generated_csv_filepath, index=False)
        exit(0)

    elif args.blue:
        # Step 1. Generate 1M white points for us to fit embedding pipeline and draw samples from.
        N = 100 * num_generated_configs
        white_noise_configs = []
        print(f"Generating {N} white noise configs...")
        n = 0
        while len(white_noise_configs) < N:
            config = sampler.sample_path()
            if config and config != "BUGGED":
                white_noise_configs.append(config)
            n += 1
            if n % 10_000 == 0:
                print(f"{n} trials, {len(white_noise_configs)} white noise configs generated...")

        candidates_df = pd.DataFrame(white_noise_configs)
        candidates_df = preprocess_dataframe(candidates_df, independent_params, [], FORWARD_CONFIG_TRANSLATION, sampler.aie_mapper)
        candidates_df = candidates_df.drop_duplicates()
        
        # Scrub duplicates from list of configs...
        indices = list(candidates_df.index)
        white_noise_configs = [white_noise_configs[i] for i in indices]
        candidates_df = candidates_df.reset_index(drop=True)
        print(f"{len(candidates_df)} after duplicates dropped.")

        # Step 2. Fit embedding pipeline based on white samples.
        embedding_model = declare_model("linear", model_params, pca_variance=0.99)
        embedding_preprocessor = embedding_model.preprocessor
        embedding_preprocessor.fit(candidates_df)
        embedded_candidates = embedding_preprocessor.transform(candidates_df)
        print(f"Embedding pipeline fitted and candidates embedded shape={embedded_candidates.shape}.")
        
        # Step 3. Select blue noise configs using best candidate algorithm in embedding space.
        print(f"Selecting {num_generated_configs} blue noise configs in embedding space...")
        blue_configs = [white_noise_configs[0]]
        blue_embeddings = np.zeros((num_generated_configs, embedded_candidates.shape[1]))   # initialize full array.
        blue_embeddings[0] = embedded_candidates[0]

        for i in range(1, num_generated_configs):
            perm = np.random.permutation( len(embedded_candidates) )[:i+1]
            bluest_with_respect_to_perm = best_candidate_algorithm_iteration(blue_embeddings[:i], embedded_candidates[perm])
            bluest_idx = perm[bluest_with_respect_to_perm] # translated idx with respect to the original set.
            blue_configs.append(white_noise_configs[bluest_idx])
            blue_embeddings[i] = embedded_candidates[bluest_idx]

            if i % 100 == 0:
                print(f"{i} blue configs generated...")

        generated_df = generate_configs_df(blue_configs, REVERSE_CONFIG_TRANSLATION, test_0_tool_canary_aie, niter, args.clean_test)
        generated_dict = generated_df.to_dict(orient="index")

        if args.runmake:
            filename = f"{sampler.tests_folder}/multi_params_random.json"
        else:   
            filename = f"{sampler.tests_folder}/multi_params_database.json"

        with open(filename, "w") as json_file:
            json.dump(generated_dict, json_file, indent=4)

        if generated_csv_filepath:
            generated_df.to_csv(generated_csv_filepath, index=False)