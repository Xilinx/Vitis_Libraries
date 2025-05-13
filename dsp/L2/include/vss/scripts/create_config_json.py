#
# Copyright (C) 2019-2022, Xilinx, Inc.
# Copyright (C) 2022-2025, Advanced Micro Devices, Inc.#
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

# python script which takes as input a cfg file containing the list of parameters for the VSS and creates a config.json file that will then be used by the metadata checker to check the configuration.
# requires 2 arguments : arg 1 : CUR_DIR ; arg 2 : cfg file
import configparser
import sys
import string

parser = configparser.ConfigParser()

CUR_DIR = sys.argv[1]
CFG_FILE = sys.argv[2]

with open(f"{CUR_DIR}/{CFG_FILE}") as stream:
    parser.read_string(
        "[top]\n" + stream.read()
    )  # Workaround because configparser complains about headerless configurations

preamble = f"""
{{
    \"spec\": \"vss_fft_ifft_1d.json\",
    \"outdir\": \"./\",
    \"parameters\":{{
"""
postamble = f"""
    }}
}}
"""
hls_top_config = dict(parser.items(str("top")))
hls_app_config = dict(parser.items(str("APP_PARAMS")))
with open("config.json", "w") as file:
    file.write(preamble)
    for k, v in hls_top_config.items():
        code = '\t"' + str(k).upper() + '" : "' + str(v) + '",\n'
        file.write(str(code))  # Write a line to the file
    last_key, last_value = list(hls_app_config.items())[-1]
    for k, v in hls_app_config.items():
        if k == last_key:
            code = '\t"' + str(k).upper() + '" : "' + str(v) + '"'
        else:
            code = '\t"' + str(k).upper() + '" : "' + str(v) + '",\n'
        file.write(str(code))  # Write a line to the file
    file.write(postamble)
