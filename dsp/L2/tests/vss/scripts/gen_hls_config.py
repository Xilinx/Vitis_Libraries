#
# Copyright (C) 2019-2022, Xilinx, Inc.
# Copyright (C) 2022-2024, Advanced Micro Devices, Inc.
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
print("1")
import json
kernel=sys.argv[1]
suffix = "_config.cfg"
cfg_file=kernel+suffix
print("2")
src_path=os.getcwd()
kernel_dict = {"KERNEL_NAME": str(kernel)}
f = open("multi_params.json", ) 
print("2")
params_file = json.load(f)
print("3")
default_params = params_file["test_0_tool_canary_aie"]
print("4")
full_dict = { **kernel_dict, **default_params}
print("5")
with open('./hls_config.tmpl', 'r') as fr:
        t = fr.read()
print("6")
print(cfg_file)
with open(cfg_file, 'w') as f:
        print("7")
        f.write(string.Template(t).substitute(**full_dict))
        print("8")