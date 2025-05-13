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
import os, string, sys
import json

kernel = sys.argv[1]
suffix = "_config.cfg"
cfg_file = kernel + suffix
src_path = os.getcwd()
kernel_dict = {"KERNEL_NAME": str(kernel)}
f = open(
    "multi_params.json",
)
params_file = json.load(f)
default_params = params_file["test_0_tool_canary_aie"]
full_dict = {**kernel_dict, **default_params}
with open("./hls_config.tmpl", "r") as fr:
    t = fr.read()
with open(cfg_file, "w") as f:
    print("7")
    f.write(string.Template(t).substitute(**full_dict))
    print("8")
