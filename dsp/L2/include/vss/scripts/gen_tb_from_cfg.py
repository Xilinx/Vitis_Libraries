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
import os
import importlib.util

LIB_ROOT = sys.argv[1]
CUR_DIR = sys.argv[2]
PARAMS_FILE = sys.argv[3]


# using the function from scripts/instance_generator.py
def run_function(path, method, *args):
    sys.path.append(os.path.dirname(path))
    spec = importlib.util.spec_from_file_location(
        os.path.basename(path).rstrip(".py"), path
    )
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    fn = getattr(mod, method)
    return fn(*args)


parser = configparser.ConfigParser()
with open(f"{CUR_DIR}/{PARAMS_FILE}") as stream:
    parser.read_string(
        "[top]\n" + stream.read()
    )  # Workaround because configparser complains about headerless configurations

details_dict = dict(parser.items("APP_PARAMS"))
new_dict = {k.upper(): v.lower() for k, v in details_dict.items()}
od = run_function(
    f"{LIB_ROOT}/L2/tests/aie/common/scripts/tb_gen.py", "generate_testbench", new_dict
)
with open(f"{CUR_DIR}/config.h", "w") as f:
    f.write(od)
    f.write("\n")
