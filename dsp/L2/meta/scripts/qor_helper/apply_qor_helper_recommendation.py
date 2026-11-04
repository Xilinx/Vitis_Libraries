#
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
import sys
from pathlib import Path

import pandas as pd

QOR_HELPER_DIR = Path(__file__).resolve().parent


def main():
    ap = argparse.ArgumentParser(
        description="Translate a config_qor_helper.py recommendation (graph-level "
        "param names) into a testbench-schema multi_params.json entry."
    )
    ap.add_argument("--csv_file", required=True, help="config_qor_helper.py --out_csv_file output")
    ap.add_argument("--multi_params_file", required=True, help="multi_params.json to append the new entry to")
    ap.add_argument("--param_set_name", required=True, help="name of the new entry to add")
    args = ap.parse_args()

    sys.path.insert(0, str(QOR_HELPER_DIR))

    with open(args.multi_params_file) as f:
        multi_params = json.load(f)
    default_config = multi_params[args.param_set_name]

    with open(QOR_HELPER_DIR / "data" / "config_translation.json") as f:
        forward_translation = json.load(f)
    reverse_translation = {v:k for k,v in forward_translation.items() if k in default_config}

    configs_df= pd.read_csv(args.csv_file)
    config_dict = configs_df.to_dict(orient="records")[0]
    config_dict = {reverse_translation[k]: str(v) for k,v in config_dict.items() if k in reverse_translation}

    for k,v in default_config.items():
        if k not in config_dict:
            config_dict[k] = str(v)

    multi_params[args.param_set_name] = config_dict
    with open(args.multi_params_file, "w") as f:
        json.dump(multi_params, f, indent=4)

    print(f"Added '{args.param_set_name}' to {args.multi_params_file}")


if __name__ == "__main__":
    main()
