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
import os
import sys
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
if data_complex_dict[DATA_TYPE]:
    wr_sample = wr_sample * 2

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
    if wr_sample == 1:
        for num in matrix_serial:
            file.write(str(num)+"\n")
    elif wr_sample == 2:
        for i in range(0, len(matrix_serial), 2):
            file.write(str(matrix_serial[i]) + " " + str(matrix_serial[i+1]) + "\n")


col_dim_kernel_list= load_utils.qrd_load_split(AIE_VARIANT, DATA_TYPE, ROW_DIM_SIZE, COL_DIM_SIZE, CASC_LEN, NUM_FRAMES)
print("Column distribution over kernels: \n")
print(col_dim_kernel_list)

for i in range(NITER):
    for j in range(NUM_FRAMES):
        print(f"Generating input matrix for iteration {i}, frame {j}...\n")
        kernel = 0
        if data_complex_dict[DATA_TYPE]:
            wr_start = ((i * NUM_FRAMES) + j) * COL_DIM_SIZE * ROW_DIM_SIZE * 2
        else:
            wr_start = ((i * NUM_FRAMES) + j) * COL_DIM_SIZE * ROW_DIM_SIZE

        for col_dim in col_dim_kernel_list:
            if data_width_bit_dict[DATA_TYPE] == 32:
                file_size = ROW_DIM_SIZE * col_dim
            elif data_width_bit_dict[DATA_TYPE] == 64:
                file_size = ROW_DIM_SIZE * col_dim * 2

            base_name = os.path.splitext(os.path.basename(LOC_INPUT_FILE))[0]
            LOC_INPUT_FILE_CASC = f"{os.path.dirname(LOC_INPUT_FILE)}/{base_name}_{kernel}.txt"
            with open(LOC_INPUT_FILE_CASC, "a") as file:
                if wr_sample == 1:
                    if DIM_A_LEADING == 1:
                        for row in range(ROW_DIM_SIZE):
                            for col in range(col_dim*2):
                                idx1 = wr_start + row*(ROW_DIM_SIZE)*2 + col
                                file.write(str(matrix_serial[idx1]) + "\n")
                        wr_start = wr_start + (col_dim * 2) 

                    else:
                        for num in matrix_serial[wr_start : wr_start + file_size]:
                            file.write(str(num)+"\n")
                        wr_start = wr_start + file_size


                elif wr_sample == 2:    
                    if DIM_A_LEADING == 1:
                        sample_num = False
                        for row in range(ROW_DIM_SIZE):
                            for col in range(col_dim):
                                idx1 = wr_start + row*(ROW_DIM_SIZE) + col
                                if sample_num:  
                                    file.write(str(wr_val_pre) + " " + str(matrix_serial[idx1]) + "\n")
                                sample_num = not sample_num
                                wr_val_pre = matrix_serial[idx1]    
                        wr_start = wr_start + col_dim        
                                    
                    else:                        
                        for num in range(wr_start, wr_start + file_size, 2):
                            file.write(str(matrix_serial[num]) + " " + str(matrix_serial[num+1]) + "\n")

                        wr_start = wr_start + file_size
            kernel += 1
