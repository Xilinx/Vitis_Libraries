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
import json
import sys
import string


# load json file into a dict
# open cfg file and write in a format
def system_params(test, fname, aie_graph_name):
    PART = test["PART"]
    FREQ = test["FREQ"]
    macro_body_str = f"""
part={PART}
freqhz={FREQ}
"""
    if "vc1902" in PART:
        macro_body_str += f"""[aie]
enable-partition=6:35:{aie_graph_name}
"""
    else:
        macro_body_str += f"""[aie]
enable-partition=0:38:{aie_graph_name}
"""
    with open(f"{CUR_DIR}/{fname}", "w") as f:
        f.write(macro_body_str)
        f.write("\n")


def uut_params(cur_dir, test, fname):
    macro_body = []
    for key, value in test.items():
        if key != "PART":
            if key in [
                "DATA_TYPE",
                "TWIDDLE_TYPE",
                "POINT_SIZE",
                "FFT_NIFFT",
                "SHIFT",
                "API_IO",
                "ROUND_MODE",
                "SAT_MODE",
                "TWIDDLE_MODE",
                "SSR",
                "AIE_PLIO_WIDTH",
                "VSS_MODE",
                "ADD_FRONT_TRANSPOSE",
                "ADD_BACK_TRANSPOSE",
            ]:
                macro_body.append(
                    f"""
{key}={value}"""
                )
    macro_body_str = "".join(macro_body)
    # Use formatted multi-line string to avoid a lot of \n and \t
    macro_body_str = f"""

[APP_PARAMS]
{macro_body_str}
"""
    with open(f"{cur_dir}/{fname}", "a") as f:
        f.write(macro_body_str)
        f.write("\n")


ROOT_DIR = sys.argv[1]
CUR_DIR = sys.argv[2]
TEST_CASE = sys.argv[3]
PARAMS_FNAME = sys.argv[4]
FFT_GRAPH_NAME = sys.argv[5]

with open(f"{CUR_DIR}/multi_params.json", "r") as fd:
    params = json.load(fd)
    test = params[TEST_CASE]

system_params(test, PARAMS_FNAME, FFT_GRAPH_NAME)
uut_params(CUR_DIR, test, PARAMS_FNAME)
