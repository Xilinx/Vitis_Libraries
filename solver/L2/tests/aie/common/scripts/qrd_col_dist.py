#
# Copyright (C) 2025, Advanced Micro Devices, Inc.
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
import kernel_load_utils as load_utils
import sys

if "-params" in sys.argv:
    params_index = sys.argv.index("-params") + 1
    params = sys.argv[params_index].split(",")
if "-h" in sys.argv or "--help" in sys.argv:
    print("Usage: python qrd_col_dist.py -params aie_variant,data_type,dim_rows,dim_cols,casc_len,num_frames")
    print("Example: python qrd_col_dist.py -params 1,float,64,64,4,1")
    sys.exit(0)

if params is None or len(params) != 6:
    print("Error: -params argument not found")
    print("Please provide parameters in the format:")
    print("-params aie_variant,data_type,dim_rows,dim_cols,casc_len,num_frames")
    sys.exit(1)

aie_variant=int(params[0])
data_type=params[1]
dim_rows=int(params[2])
dim_cols=int(params[3])
casc_len=int(params[4])
num_frames=int(params[5])

col_list=load_utils.qrd_load_split(aie_variant, data_type, dim_rows, dim_cols, casc_len, num_frames)

for i in range(casc_len):
    print(f"Column distribution for kernel {i}: {col_list[i]}")