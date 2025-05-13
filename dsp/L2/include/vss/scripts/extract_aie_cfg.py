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

parser = configparser.ConfigParser()

CUR_DIR = sys.argv[1]
PARAMS_FILE = sys.argv[2]
with open(f"{CUR_DIR}/{PARAMS_FILE}") as stream:
    parser.read_string(
        "[top]\n" + stream.read()
    )  # Workaround because configparser complains about headerless configurations


aie_config = dict(parser.items("aie"))
aie_parser = configparser.ConfigParser()
aie_parser["aie"] = aie_config
no_header_params = dict(parser.items("top"))
with open(f"{CUR_DIR}/aie_params.cfg", "w") as stream:
    for key, val in no_header_params.items():
        if key not in ["freqhz"]:
            stream.write(key + "=" + val)
            stream.write("\n")
    stream.write("[aie]\n")
    for key, val in aie_config.items():
        stream.write(key + "=" + val)
        stream.write("\n")
#        aie_parser.write(stream, str(aie_config))
