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
import numpy as np

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
        if sys.argv[i] == "--dsp_root_dir":
            DSP_ROOT_DIR = sys.argv[i + 1]

include_path = DSP_ROOT_DIR + "/L2/meta"
sys.path.insert(0, include_path)
import aie_common as com

if 'DIM_Q_LEADING' not in locals():
    DIM_Q_LEADING = 0
if 'DIM_R_LEADING' not in locals():
    DIM_R_LEADING = 0

print("Stitching outputs... \n")
wr_sample = PLIO_WIDTH//data_width_bit_dict[DATA_TYPE]
rd_sample = 1
if data_complex_dict[DATA_TYPE]:
    wr_sample = wr_sample * 2
    rd_sample = rd_sample * 2

vecSampleNum = int(com.k_max_read_write_bytes[AIE_VARIANT] / (data_width_bit_dict[DATA_TYPE] // 8))
col_chunck_size = COL_DIM_SIZE // vecSampleNum # this is num chunks in rows we need to concantenate 
row_dim_per_kernel = ROW_DIM_SIZE // CASC_LEN
row_chunk_size_per_kernel = row_dim_per_kernel // vecSampleNum # this is the num of chunks in a kernel
casc_num = int(np.ceil(col_chunck_size / row_chunk_size_per_kernel)) # this is the num of cascades we need to sweep

# Initialize a two-dimensional list (list of lists) with zeros
r_data = [[0 for _ in range(NUM_FRAMES)] for _ in range(CASC_LEN)]
q_data = [[0 for _ in range(NUM_FRAMES)] for _ in range(CASC_LEN)]

for casc in range(CASC_LEN):
    base_name_q = os.path.splitext(os.path.basename(LOC_OUTPUT_FILE_Q))[0]
    LOC_OUTPUT_FILE_Q_rd = f"{os.path.dirname(LOC_OUTPUT_FILE_Q)}/{base_name_q}_{casc}.txt"

    # # Read Q matrix from file
    try:
        with open(LOC_OUTPUT_FILE_Q_rd, 'r') as file_q:
            q_lines = file_q.readlines()
            q_data[casc]=([float(num) for line in q_lines for num in line.split()])
    except FileNotFoundError:
        print(f"File not found: {LOC_OUTPUT_FILE_Q_rd}")

for casc in range(casc_num): # only scan through necessary cascades
    base_name_r = os.path.splitext(os.path.basename(LOC_OUTPUT_FILE_R))[0]
    LOC_OUTPUT_FILE_R_rd = f"{os.path.dirname(LOC_OUTPUT_FILE_R)}/{base_name_r}_{casc}.txt"
    # Read R matrix from file
    try:
        with open(LOC_OUTPUT_FILE_R_rd, 'r') as file_r:
            r_lines = file_r.readlines()
            r_data[casc]=([float(num) for line in r_lines for num in line.split()])
    except FileNotFoundError:
        print(f"File not found: {LOC_OUTPUT_FILE_R_rd}")

RrowDim=[]
for casc in range(casc_num):
    rowColected = casc * row_dim_per_kernel
    # print(f"rowColected:{rowColected}")
    # print(f"COL_DIM_SIZE -  rowColected:{COL_DIM_SIZE -  rowColected:}")
    if row_dim_per_kernel <= COL_DIM_SIZE -  rowColected:
        if COL_DIM_SIZE < row_dim_per_kernel:
            RrowDim.append(COL_DIM_SIZE)
        else: RrowDim.append(row_dim_per_kernel)
    else:
        RrowDim.append(COL_DIM_SIZE - rowColected)
# print(f"RrowDim {RrowDim}, casc_num {casc_num}, row_dim_per_kernel {row_dim_per_kernel}, col_dim_size {COL_DIM_SIZE}")

if DIM_R_LEADING == 0:
    rout=[]
    for j in range(NITER):
        for i in range(NUM_FRAMES):
            for col in range(COL_DIM_SIZE):
                for casc in range(casc_num): 
                    start_address_matrix=((j * NUM_FRAMES) + i) * COL_DIM_SIZE * RrowDim[casc] * rd_sample #current matrix start address              
                    start_addr = col * RrowDim[casc] *rd_sample + start_address_matrix
                    # print(f"iteration {j}, frame {i}...")
                    # print(f"start_address_matrix {start_address_matrix}")
                    # print(f"casc {casc}, col {col}, start_addr {start_addr}, RrowDim {RrowDim[casc]}")
                    for col_row in range(RrowDim[casc]*rd_sample):
                        # print(f"col_row {col_row}")
                        value=r_data[casc][start_addr+col_row]
                        rout.append(value)


elif DIM_R_LEADING == 1:
    rout=[]
    for j in range(NITER):
        for i in range(NUM_FRAMES):
            for casc in range(casc_num): 
                start_address_matrix=((j * NUM_FRAMES) + i) * COL_DIM_SIZE * RrowDim[casc] * rd_sample #current matrix start address              
                rout.extend(r_data[casc][start_address_matrix:start_address_matrix+COL_DIM_SIZE * RrowDim[casc]* rd_sample])

with open(LOC_OUTPUT_FILE_R, "w") as f:
    for k in range(0, len(rout), wr_sample):
        line_elements = [f"{rout[j]:.9e}" for j in range(k, k + wr_sample)]
        f.write(" ".join(line_elements) + " \n")


qout=[]
if DIM_Q_LEADING == 0:
    start_addr = 0
    for j in range(NITER):
        for i in range(NUM_FRAMES):
            for col in range(COL_DIM_SIZE):
                for casc in range(CASC_LEN):
                    # print(f"Processing cascade {casc}, iteration {j}, frame {i}...")
                    # print(f"start_addr {start_addr}, row_dim_per_kernel {row_dim_per_kernel}, rd_sample {rd_sample}\n")
                    for val in q_data[casc][start_addr:start_addr+row_dim_per_kernel*rd_sample]:
                        qout.append(val)
                start_addr = start_addr + row_dim_per_kernel * rd_sample
elif DIM_Q_LEADING == 1:
    for j in range(NITER):
        for i in range(NUM_FRAMES):
            start_address_matrix=((j * NUM_FRAMES) + i) * COL_DIM_SIZE * row_dim_per_kernel* rd_sample #current matrix start address              
            for casc in range(CASC_LEN): 
                qout.extend(q_data[casc][start_address_matrix:start_address_matrix+ COL_DIM_SIZE * row_dim_per_kernel* rd_sample])
        
with open(LOC_OUTPUT_FILE_Q, "w") as f:
    for k in range(0, len(qout), wr_sample):
        line_elements = [f"{qout[j]:.9e}" for j in range(k, k + wr_sample)]
        f.write(" ".join(line_elements) + " \n")