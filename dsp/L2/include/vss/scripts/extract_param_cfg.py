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
import configparser
import sys
import string

parser = configparser.ConfigParser()

CUR_DIR = sys.argv[1]
PARAM = sys.argv[2]
PARAM_HEAD = sys.argv[3]
PARAMS_FILE = sys.argv[4]

def get_aie_arch_from_part_name(part_name):
    if not part_name:
        return 1
    elif "vc1902" in part_name:
        return 1
    elif "vc1802" in part_name:
        return 1
    elif "vc1702" in part_name:
        return 1
    elif "ve1752" in part_name:
        return 1
    elif "vc1502" in part_name:
        return 1
    elif "vp2802" in part_name:
        return 1
    elif "vp2502" in part_name:
        return 1
    elif "ve2802" in part_name:
        return 2
    elif "v70" in part_name:
        return 2
    elif "v65" in part_name:
        return 1
    elif "vc2802" in part_name:
        return 2
    elif "ve2602" in part_name:
        return 2
    elif "vc2602" in part_name:
        return 2
    elif "ve2302" in part_name:
        return 2
    elif "ve2202" in part_name:
        return 2
    elif "ve2102" in part_name:
        return 2
    elif "ve2002" in part_name:
        return 2
    elif "10S70" in part_name:
        return 1
    elif "10AIE24x5" in part_name:
        return 2
    elif "10AIE2P_ML" in part_name:
        return "aie2p"
    elif "10Turin" in part_name:
        return "22"
    elif "xc2ve3304" in part_name:
        return "22"
    elif "xc2ve3358" in part_name:
        return "22"
    elif "xc2ve3504" in part_name:
        return "22"
    elif "xc2ve3558" in part_name:
        return "22"
    elif "xc2ve3804" in part_name:
        return "22"
    elif "xc2ve3858" in part_name:
        return "22"
    elif "xa2ve3288" in part_name:
        return "22"
    elif "xc10T21" in part_name:
        return "22"
    elif "10MDS1" in part_name:
        return "aie4"
    elif "10SWV1" in part_name:
        return "aie4a"
    elif "vr1602" in part_name:
        return 1
    elif "vr1652" in part_name:
        return 1
    else:
        return -1
    return 1

with open(f"{PARAMS_FILE}") as stream:
    parser.read_string(
        "[top]\n" + stream.read()
    )  # Workaround because configparser complains about headerless configurations
try:
    hls_config = dict(parser.items(str(PARAM_HEAD)))
    if PARAM in ["enable-partition"]:
        print(hls_config[PARAM.lower()].split(':')[-1])
    elif PARAM == "AIE_VARIANT":
        part = hls_config["part"]
        print(get_aie_arch_from_part_name(part))
    else:
        print(hls_config[PARAM.lower()])
except KeyError:
    print("-1")
