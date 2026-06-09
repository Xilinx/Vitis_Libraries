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
import sys
import os
import importlib
import random
import numpy as np
import pandas as pd

def parse_json(file):
    import json
    with open(file, "r") as file:
        param_dict_list = json.load(file)["parameters"]
        param_dicts = dict()
        for param_dict in param_dict_list:
            param = param_dict["name"]
            param_dicts[param] = param_dict

    for param in list(param_dicts):
        if not param.isupper():
            param_dicts.pop(param) 
    return param_dicts

class ConfigSampler:
    def __init__(self, ip_name, constrained_params, ordinal_params, categorical_params, no_iterate_params):

        self.aie_mapper = {"AIE": 1, "AIE-ML": 2, "AIE-MLv2": 22}
        self.set_metadata_dirs()
        ip_module = importlib.import_module(ip_name)
        metadata_dir = os.path.dirname(os.path.abspath(ip_module.__file__))
        param_dicts = parse_json(f"{metadata_dir}/{ip_name}.json")

        self.tests_folder = f"{metadata_dir}/../tests/aie/{ip_name}/" 
        self.params = [param for param in param_dicts]
        self.validators = {param: getattr(ip_module, param_dicts[param]["validator"]["function"]) for param in param_dicts}
        self.updaters = {param: getattr(ip_module, param_dicts[param]["updater"]["function"]) for param in param_dicts}
        
        self.test_max = {param: None for param in param_dicts}  # initializing
        for param in param_dicts:
            if "test_max" in param_dicts[param]:
                self.test_max[param] = param_dicts[param]["test_max"]

        self.constrained = constrained_params    # for parameters with constrained values.
        self.ordinal_params = ordinal_params
        self.categorical_params = categorical_params
        self.no_iterate_params = no_iterate_params

    def set_metadata_dirs(self, libraries=["xf_dsp", "xf_solver"]):
        qor_helper_dir = os.path.dirname(os.path.abspath(__file__))
        libraries_dir = qor_helper_dir + "/../../../../../"
        entries = os.listdir(libraries_dir)
        dir_names = [entry for entry in entries if os.path.isdir(os.path.join(libraries_dir, entry))]
        for dir in dir_names:
            if dir in libraries:
                repo_dir = os.path.join(libraries_dir, dir)
                metadata_dir = repo_dir + "/L2/meta"
                sys.path.append(metadata_dir)

    def get_legal_values(self, updated_dict):
        """Returns the values given an updated_dict."""
        if "enum" in updated_dict:
            return sorted(updated_dict["enum"])
        elif "minimum" in updated_dict:
            return range(int(updated_dict["minimum"]), int(updated_dict["maximum"])+1)  # TODO: Resolve the updaters returning integers.
        elif "len" in updated_dict:
            len_ = int(updated_dict["len"])
            return {range(len_)}
        else:   # no legal values...
            return {}
        
    def constrain_values(self, legal_vals: list, constrained_vals: list) -> list:
        """Returns constrained set of values."""
        if isinstance(legal_vals, range):
            legal_vals = [val for val in constrained_vals if val in legal_vals]
        else:
            legal_vals = set(legal_vals) & set(constrained_vals)    # take the intersection of legal vals and constraints.
        return list(legal_vals)

    def clip_values(self, legal_vals, clipped_max: int):
        """Returns clipped legal_vals depending in input type."""
        if isinstance(legal_vals, range):
            min_val = legal_vals[0]
            max_val = min(legal_vals[-1], clipped_max)
            return range(min_val, max_val+1)
        else:
            legal_vals = [val for val in legal_vals if val <= clipped_max]
            return legal_vals


    def validate_config(self, config: dict, test_max_included: bool=True, report_illegal=False) -> bool:
        """Verifies a config is accessible via the updaters. If parameter is ommitted, default value is accepted."""
        config_walk = {}
        true_flag = True   # by default
        for param in self.params:
            validator = self.validators[param]
            updater = self.updaters[param]
            updated_dict = updater(config_walk)

            legal_vals = self.get_legal_values(updated_dict)
            if param in self.constrained:
                legal_vals = self.constrain_values(legal_vals, self.constrained[param])
                
            if not legal_vals:
                true_flag = False
                break
                
            if param in config:
                config_walk[param] = config[param]
                if (param == "AIE_VARIANT") and (config_walk[param] in self.aie_mapper):
                    config_walk[param] = self.aie_mapper[config_walk[param]]    # map from "AIE", "AIE-ML" to 1, 2

                if config_walk[param] not in legal_vals:
                    true_flag = False
                    break
            else:
                config_walk[param] = next(iter(legal_vals))

            validated_dict = validator(config)
            if not validated_dict["is_valid"]:
                true_flag = False
                break
            
            if test_max_included:
                if self.test_max[param]:
                    if config_walk[param] > self.test_max[param]:
                        true_flag = False
                        break

        if report_illegal and true_flag == False:
            print(f"{config} is illegal!")
        return true_flag


    def sample_path(self):
        """Walks a random path from root to leaf of the dependency tree."""
        config = {}

        try:
            for param in self.params:
                validator = self.validators[param]
                updater = self.updaters[param]
                updated_dict = updater(config)
                
                # * Debug clauses...
                if updated_dict == False:   # if invalid path...
                    return {}
                if "minimum" in updated_dict:
                    if updated_dict["minimum"] > updated_dict["maximum"]:
                        raise Exception()   # This exception is intended to fire for illegal ranges.

                # * Define legal set...
                legal_vals = self.get_legal_values(updated_dict)
                if param in self.constrained:
                    legal_vals = self.constrain_values(legal_vals, self.constrained[param])
                
                if not legal_vals:
                    return {}

                elif param in self.ordinal_params:
                    min_val = legal_vals[0]
                    max_val = legal_vals[-1]
                    
                    if self.test_max[param]:
                        max_val = min(max_val, self.test_max[param])    # clipping the max value to the testable range.
                        if min_val > max_val:   # for cases where the testable_max is below the min allowed by metadata.
                            return {}

                    min_val_log = np.log(min_val + 1)
                    max_val_log = np.log(max_val + 1)
                    sampled_log = random.uniform(min_val_log, max_val_log)
                    config[param] = round(np.exp(sampled_log) - 1)    # prioritizing smaller values. Consider fft: sampling uniformly between 16 and 65536 will overrepresent larger point sizes.

                elif param in self.categorical_params:
                    config[param] = random.choice(legal_vals)

                elif param in self.no_iterate_params:
                    config[param] = next(iter(legal_vals))

                else:   # Handling non-model parameters...
                    config[param] = random.choice(legal_vals)  # default parameter

                # * Validate parameter and update if necessary...
                validated_dict = validator(config)
                if not validated_dict["is_valid"]:   # if our random guess was illegal...
                    updated_dict = updater(config)
                    config[param] = updated_dict["actual"]  # ...snap to nearest value
        except:
            print(f"Bugged config. Verify by running it through config_helper: {config}")
            return "BUGGED"
        return config
    

    def generate_configs(self, config_limit=100000) -> pd.DataFrame:
        
        def recursive_update(config, recursive_lvl):
            if recursive_lvl >= len(self.params):   # base case
                legal_configs.append(dict(config))
                return
            
            # else...
            param = self.params[recursive_lvl]
            validator = self.validators[param]
            updater = self.updaters[param]
            updated_dict = updater(config)

            # * Debug clauses...
            if updated_dict == False:   # if invalid path...
                return {}
            if "minimum" in updated_dict:
                if updated_dict["minimum"] > updated_dict["maximum"]:
                    raise Exception()   # This exception is intended to fire for illegal ranges.

            # * Establish the legal values for this parameter...
            legal_vals = self.get_legal_values(updated_dict)
            if param in self.constrained:
                legal_vals = self.constrain_values(legal_vals, self.constrained[param])

            # This clause prevents the model making predictions beyond its training limits.
            if self.test_max[param]:
                legal_vals = self.clip_values(legal_vals, self.test_max[param])

            if not legal_vals:    # dead end path...
                return

            if param in self.no_iterate_params:    # if we should not iterate over this parameter (e.g. TP_SHIFT, TP_RND, TP_SAT)...
                legal_vals = {next(iter(legal_vals))}   # modify legal vals to only have one value.

            # * Recursively walk down all legal paths...
            for val in legal_vals:
                config[param] = val
                validated_dict = validator(config)
                if validated_dict["is_valid"]:
                    recursive_update(config, recursive_lvl+1)

                if len(legal_configs) == config_limit:
                    return
                
        legal_configs = []
        recursive_update({}, 0) # first call to recursive function
        configs_df = pd.DataFrame.from_records(legal_configs)
        return configs_df