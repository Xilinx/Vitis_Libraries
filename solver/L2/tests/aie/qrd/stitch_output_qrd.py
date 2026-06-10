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
import sys
import os 

PATH = os.path.dirname(os.path.abspath(__file__))
sys.path.append(os.path.join(PATH, "../common/scripts"))

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
        if sys.argv[i] == "--loc_output_file_Q":
            LOC_OUTPUT_FILE_Q = sys.argv[i + 1]
        if sys.argv[i] == "--loc_output_file_R":
            LOC_OUTPUT_FILE_R = sys.argv[i + 1]
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
        if sys.argv[i] == "--dim_q_leading":
            DIM_Q_LEADING = int(sys.argv[i + 1])
        if sys.argv[i] == "--dim_r_leading":
            DIM_R_LEADING = int(sys.argv[i + 1])
        if sys.argv[i] == "--niter":
            NITER = int(sys.argv[i + 1])
        if sys.argv[i] == "--plioWidth":
            PLIO_WIDTH = int(sys.argv[i + 1])

if 'DIM_Q_LEADING' not in locals():
    DIM_Q_LEADING = 0
if 'DIM_R_LEADING' not in locals():
    DIM_R_LEADING = 0

print("Stitching outputs... \n")
col_dim_kernel_list= load_utils.qrd_load_split(AIE_VARIANT, DATA_TYPE, ROW_DIM_SIZE, COL_DIM_SIZE, CASC_LEN, NUM_FRAMES)
wr_sample = PLIO_WIDTH/data_width_bit_dict[DATA_TYPE]
rd_sample = 1
if data_complex_dict[DATA_TYPE]:
    wr_sample = wr_sample * 2
    rd_sample = rd_sample * 2

Q_matrix_frm_niter = [[[[None for _ in range(ROW_DIM_SIZE * COL_DIM_SIZE)] for _ in range(NITER)] for _ in range(NUM_FRAMES)] for _ in range(CASC_LEN)]
R_matrix_frm_niter = [[[[None for _ in range(COL_DIM_SIZE * COL_DIM_SIZE)] for _ in range(NITER)] for _ in range(NUM_FRAMES)] for _ in range(CASC_LEN)]

col_dist = 0

Q_matrix_out = [None] * (ROW_DIM_SIZE * COL_DIM_SIZE * NUM_FRAMES * NITER * rd_sample)
R_matrix_out = [None] * (COL_DIM_SIZE * COL_DIM_SIZE * NUM_FRAMES * NITER * rd_sample)

for casc in range(CASC_LEN):
    # print(f"casc={casc}")
    col_size = col_dim_kernel_list[casc]

    base_name_q = os.path.splitext(os.path.basename(LOC_OUTPUT_FILE_Q))[0]
    LOC_OUTPUT_FILE_Q_rd = f"{os.path.dirname(LOC_OUTPUT_FILE_Q)}/{base_name_q}_{casc}.txt"

    base_name_r = os.path.splitext(os.path.basename(LOC_OUTPUT_FILE_R))[0]
    LOC_OUTPUT_FILE_R_rd = f"{os.path.dirname(LOC_OUTPUT_FILE_R)}/{base_name_r}_{casc}.txt"

    with open(LOC_OUTPUT_FILE_Q_rd, "r") as f:
        with open(LOC_OUTPUT_FILE_Q_rd, "r") as f:
            Q_matrix = [val for line in f for val in line.strip().split()]

        with open(LOC_OUTPUT_FILE_R_rd, "r") as f:
            R_matrix = [val for line in f for val in line.strip().split()]
    k_q=0
    k_r=0
    for j in range(NITER):
        for i in range(NUM_FRAMES):
            # print(f"Processing cascade {casc}, iteration {j}, frame {i}...\n")
            rd_start_q = ((((j * NUM_FRAMES) + i) * COL_DIM_SIZE * ROW_DIM_SIZE) + col_dist) * rd_sample
            rd_start_r = ((((j * NUM_FRAMES) + i) * COL_DIM_SIZE * COL_DIM_SIZE) + col_dist) * rd_sample
            # print(f"rd_start_r={rd_start_r}")
            if DIM_Q_LEADING == 1:
                for row in range(ROW_DIM_SIZE):
                    for col in range(col_size):
                        idxq = rd_start_q  + (row*(COL_DIM_SIZE) + col) *rd_sample
                        Q_matrix_out[idxq:idxq+rd_sample] = Q_matrix[k_q:k_q+rd_sample]
                        k_q += rd_sample
                rd_start_q = rd_start_q + col_size

            else:
                read_start_q = ROW_DIM_SIZE * col_size * (i + NUM_FRAMES * j) * rd_sample
                read_end_q = ROW_DIM_SIZE * col_size * (i + 1 + NUM_FRAMES * j) * rd_sample
                Q_matrix_frm_niter[casc][i][j] = (Q_matrix[read_start_q:read_end_q])

            if DIM_R_LEADING == 1:
                for row in range(COL_DIM_SIZE):
                    for col in range(col_size):
                        idxr = rd_start_r + (row*(COL_DIM_SIZE) + col)*rd_sample
                        R_matrix_out[idxr:idxr+rd_sample] = R_matrix[k_r:k_r+rd_sample]
                        # print(f"k_r={k_r}")
                        # print(f"idxr={idxr}")
                        k_r += rd_sample
                rd_start_r = rd_start_r + col_size
            else:
                read_start_r = COL_DIM_SIZE * col_size * (i + NUM_FRAMES * j) * rd_sample
                read_end_r = COL_DIM_SIZE * col_size * (i + 1 + NUM_FRAMES * j) * rd_sample
                R_matrix_frm_niter[casc][i][j] = (R_matrix[read_start_r:read_end_r])
    col_dist = col_size + col_dist

def write_matrix(filepath, data, wr_sample):
    wr_sample = int(wr_sample)
    with open(filepath, "a") as f:
        for k in range(0, len(data), wr_sample):
            chunk = data[k:k + wr_sample]
            f.write(" ".join(str(v).strip() for v in chunk) + " \n")


if DIM_Q_LEADING==1:
    write_matrix(LOC_OUTPUT_FILE_Q, Q_matrix_out, wr_sample)
else:
    for j in range(NITER):
        for i in range(NUM_FRAMES):  
            for casc in range(CASC_LEN):     
                write_matrix(LOC_OUTPUT_FILE_Q, Q_matrix_frm_niter[casc][i][j], wr_sample)

if DIM_R_LEADING==1:
    write_matrix(LOC_OUTPUT_FILE_R, R_matrix_out, wr_sample)
else:
    for j in range(NITER):
        for i in range(NUM_FRAMES):   
            for casc in range(CASC_LEN):    
                write_matrix(LOC_OUTPUT_FILE_R, R_matrix_frm_niter[casc][i][j], wr_sample)
