#
# Copyright (C) 2025-2026, Advanced Micro Devices, Inc.
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
import os
import sys
import numpy as np
PATH = os.path.dirname(os.path.abspath(__file__))
sys.path.append(os.path.join(PATH, "../common/scripts"))

import random
import gen_matrix_utils as gen_utils
import kernel_load_utils as load_utils


data_width_bit_dict = {
    "float": 32,
    "cfloat": 64,
}

data_complex_dict = {
    "float": False,
    "cfloat": True,
}

if __name__ == "__main__":
    for i in range(len(sys.argv)):
        if sys.argv[i] == "--loc_input_file":
            LOC_INPUT_FILE = sys.argv[i + 1]
        if sys.argv[i] == "--aie_variant":
            AIE_VARIANT = int(sys.argv[i + 1])
        if sys.argv[i] == "--data_type":
            DATA_TYPE = sys.argv[i + 1]
        if sys.argv[i] == "--row_dim_size":
            ROW_DIM_SIZE = int(sys.argv[i + 1])
        if sys.argv[i] == "--col_dim_size":
            COL_DIM_SIZE = int(sys.argv[i + 1])
        if sys.argv[i] == "--casc_len":
            CASC_LEN = int(sys.argv[i + 1])
        if sys.argv[i] == "--num_frames":
            NUM_FRAMES = int(sys.argv[i + 1])
        if sys.argv[i] == "--dim_a_leading":
            DIM_A_LEADING = int(sys.argv[i + 1])
        if sys.argv[i] == "--niter":
            NITER = int(sys.argv[i + 1])
        if sys.argv[i] == "--seed":
            SEED = int(sys.argv[i + 1])
        if sys.argv[i] == "--pliowidth":
            PLIOWIDTH = int(sys.argv[i + 1])


if 'DIM_A_LEADING' not in locals():
    DIM_A_LEADING = 0

os.makedirs(os.path.dirname(LOC_INPUT_FILE), exist_ok=True)
random.seed(SEED)

wr_sample = PLIOWIDTH // data_width_bit_dict[DATA_TYPE]
rd_sample = 1
if data_complex_dict[DATA_TYPE]:
    wr_sample = wr_sample * 2
    rd_sample = rd_sample * 2

print("Generating input matrix... \n")

line_to_file = ""
matrix_serial = []
for i in range(NITER):
    for j in range(NUM_FRAMES):
        if DATA_TYPE == "float":
            matrix = gen_utils.generate_real_matrix(ROW_DIM_SIZE, COL_DIM_SIZE) 
        elif DATA_TYPE == "cfloat":
            matrix = gen_utils.generate_complex_matrix(ROW_DIM_SIZE, COL_DIM_SIZE)
        else:
            raise TypeError("Only float and cfloat supported.")
    
        matrix_serial.extend(gen_utils.get_serial(matrix, row_major=True))
    
with open(LOC_INPUT_FILE, "a") as file:
    for i in range(0, len(matrix_serial), wr_sample):
        line_elements = [str(matrix_serial[j]) for j in range(i, i + wr_sample)]
        file.write(" ".join(line_elements) + "\n")


row_per_kernel = ROW_DIM_SIZE // CASC_LEN
base_name = os.path.splitext(os.path.basename(LOC_INPUT_FILE))[0]
matrix_size_in_sample_per_kernel = row_per_kernel * COL_DIM_SIZE * rd_sample

if (DIM_A_LEADING == 0):
    for i in range(NITER):
        for j in range(NUM_FRAMES):
            # print(f"Generating input matrix for iteration {i}, frame {j}...\n")
            wr_start_matrix = ((i * NUM_FRAMES) + j) * COL_DIM_SIZE * ROW_DIM_SIZE * rd_sample
            for col in range(COL_DIM_SIZE):
                wr_start_col = wr_start_matrix + col * ROW_DIM_SIZE *rd_sample
                for casc_stage in range(CASC_LEN):
                    LOC_INPUT_FILE_CASC = f"{os.path.dirname(LOC_INPUT_FILE)}/{base_name}_{casc_stage}.txt"
                    wr_start = wr_start_col + casc_stage * row_per_kernel * rd_sample
                    with open(LOC_INPUT_FILE_CASC, "a") as file:
                        for num in range(wr_start, wr_start + row_per_kernel*rd_sample, wr_sample):
                            line_elements = [str(matrix_serial[j]) for j in range(num, num + wr_sample)]
                            file.write(" ".join(line_elements) + "\n")

elif (DIM_A_LEADING == 1):
    for i in range(NITER):
        for j in range(NUM_FRAMES):
            # print(f"Generating input matrix for iteration {i}, frame {j}...\n")
            wr_start_matrix = ((i * NUM_FRAMES) + j) * COL_DIM_SIZE * ROW_DIM_SIZE * rd_sample             
            for casc_stage in range(CASC_LEN):
                LOC_INPUT_FILE_CASC = f"{os.path.dirname(LOC_INPUT_FILE)}/{base_name}_{casc_stage}.txt"
                wr_start = wr_start_matrix + casc_stage * matrix_size_in_sample_per_kernel
                with open(LOC_INPUT_FILE_CASC, "a") as file:
                    for num in range(wr_start, wr_start + matrix_size_in_sample_per_kernel, wr_sample):
                        line_elements = [str(matrix_serial[j]) for j in range(num, num + wr_sample)]
                        file.write(" ".join(line_elements) + "\n")
                    