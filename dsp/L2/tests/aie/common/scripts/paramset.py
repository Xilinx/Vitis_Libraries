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
import os
import sys

parameter_file = sys.argv[1]
parameter_set = sys.argv[2]
# print(parameter_file,parameter_set)
f = open(parameter_file)
if not f:
    print(parameter_file + " open failed")
    exit(1)
parameters = json.load(f)
MAX_FIR_PER_KERNEL = 256
INTERPOLATE_FACTOR = parameters[parameter_set]["INTERPOLATE_FACTOR"]
COEFF_TYPE = parameters[parameter_set]["COEFF_TYPE"]

# COEFF_SIZE depends on bytes in COEFF_TYPE
if COEFF_TYPE in ["int32", "cint16", "float"]:
    COEFF_SIZE = 4
elif COEFF_TYPE in ["cint32", "cfloat"]:
    COEFF_SIZE = 8
else:
    COEFF_SIZE = 2

print(int(MAX_FIR_PER_KERNEL) * int(INTERPOLATE_FACTOR) * int(COEFF_SIZE) + 1536)
