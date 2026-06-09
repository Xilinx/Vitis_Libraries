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

# python script which takes as input a cfg file containing the list of parameters for the VSS and creates a config.json file that will then be used by the metadata checker to check the configuration.
# requires 2 arguments : arg 1 : CUR_DIR ; arg 2 : cfg file
import configparser
import sys
import json
import os

parser = configparser.ConfigParser()

CUR_DIR = sys.argv[1]
CFG_FILE = sys.argv[2]

# Determine the path to vss_fft_ifft_1d.json (relative to script location)
script_dir = os.path.dirname(os.path.abspath(__file__))
metadata_json_path = os.path.join(script_dir, "../../../meta/vss_fft_ifft_1d.json")

# Load type information from vss_fft_ifft_1d.json
with open(metadata_json_path) as meta_file:
    metadata = json.load(meta_file)

# Build a dictionary of parameter names to their types
param_types = {}
for param in metadata.get("parameters", []):
    param_types[param["name"].upper()] = param.get("type", "typename")

with open(f"{CUR_DIR}/{CFG_FILE}") as stream:
    parser.read_string(
        "[top]\n" + stream.read()
    )  # Workaround because configparser complains about headerless configurations

hls_top_config = dict(parser.items(str("top")))
hls_app_config = dict(parser.items(str("APP_PARAMS")))

# Build output dictionary with proper types
output = {
    "spec": "vss_fft_ifft_1d.json",
    "outdir": "./",
    "parameters": {}
}

for k, v in hls_top_config.items():
    key = k.upper()
    param_type = param_types.get(key, "typename")
    if param_type in ["int", "uint"]:
        output["parameters"][key] = int(v)
    else:
        output["parameters"][key] = v

for k, v in hls_app_config.items():
    key = k.upper()
    param_type = param_types.get(key, "typename")
    if param_type in ["int", "uint"]:
        output["parameters"][key] = int(v)
    else:
        output["parameters"][key] = v

# Write using json.dump for proper formatting
with open("config.json", "w") as file:
    json.dump(output, file, indent=4)
