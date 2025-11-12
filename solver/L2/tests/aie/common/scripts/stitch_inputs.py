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
import argparse
import numpy as np
import os

dtype_mapping = {
    "float": np.float32,
    "cfloat": np.complex64,
}

def stitch_matrices(matrices):
    """
    Accepts numpy array of shape (N_GRID_ROWS, N_GRID_COLS, ROW_DIM_PER_MATRIX, COL_DIM_PER_MATRIX) to be stitched together.
    """
    row_matrices_concat = []
    for i in range(len(matrices)):
        row_matrices_concat.append( np.hstack(matrices[i]) )
    return np.vstack(row_matrices_concat)

if __name__ == "__main__":

    parser = argparse.ArgumentParser()
    parser.add_argument("ROW_DIM_PER_MATRIX", type=int)
    parser.add_argument("COL_DIM_PER_MATRIX", type=int)
    parser.add_argument("N_GRID_ROWS", type=int)
    parser.add_argument("N_GRID_COLS", type=int)
    parser.add_argument("NUM_MATRICES", type=int)
    parser.add_argument("DATA_TYPE")
    parser.add_argument("--COL_MAJOR", type=bool, default=True)
    parser.add_argument("--HANDLE_MISSING", type=bool, default=False, help="If set to True, will substitute zeroed matrices characterized by specified arguments.")
    args = parser.parse_args()

    ROW_DIM_PER_MATRIX = args.ROW_DIM_PER_MATRIX
    COL_DIM_PER_MATRIX =args.COL_DIM_PER_MATRIX
    N_GRID_ROWS = args.N_GRID_ROWS
    N_GRID_COLS = args.N_GRID_COLS
    NUM_MATRICES = args.NUM_MATRICES
    DATA_TYPE = args.DATA_TYPE
    COL_MAJOR = args.COL_MAJOR
    HANDLE_MISSING = args.HANDLE_MISSING

    sub_matrices = []
    for i in range(N_GRID_ROWS):
        matrices_row = []
        for j in range(N_GRID_COLS):
            
            if os.path.isfile(f"data/uut_output_{i}_{j}.txt"):
                file_data = np.loadtxt(f"data/uut_output_{i}_{j}.txt", dtype=np.float32)    # Read the data as floats then
                file_data = file_data.view(dtype_mapping[DATA_TYPE])                        # interpret as target dtype.
                sub_matrix = file_data.reshape((NUM_MATRICES, ROW_DIM_PER_MATRIX, COL_DIM_PER_MATRIX))
            else:
                if HANDLE_MISSING == True:
                    sub_matrix = np.zeros((NUM_MATRICES, ROW_DIM_PER_MATRIX, COL_DIM_PER_MATRIX), dtype=dtype_mapping[DATA_TYPE])
                else:
                    raise FileNotFoundError(f"No file found at data/uut_output_{i}_{j}.txt")

            if COL_MAJOR:
                sub_matrix = sub_matrix.transpose((0,2,1))

            matrices_row.append(sub_matrix)
        sub_matrices.append(matrices_row)
    sub_matrices = np.array(sub_matrices)

    N_STITCHED_MATRICES = sub_matrices.shape[2] # 3rd dimension is associated with NUM_FRAMES * NITERS.

    # Once this is all said and done, this is the shape of our list of lists of lists of lists of lists (wow):
    #
    # ROW_MAJOR:    (N_GRID_ROWS, N_GRID_COLS, N_STITCHED_MATRICES, ROW_DIM_PER_MATRIX, COL_DIM_PER_MATRIX)
    # COL_MAJOR:    (N_GRID_ROWS, N_GRID_COLS, N_STITCHED_MATRICES, COL_DIM_PER_MATRIX, ROW_DIM_PER_MATRIX)
    #
    # It is much easier to stitch sub-matrices together in isolation (one matrix to be stitched per function call)
    # than to try and do it while worrying about NUM_FRAMES / NITERS. Therefore, we want to re-organise our tensor
    # such that the N_STITCHED_MATRICES is at the front.

    sub_matrices = sub_matrices.transpose((2, 0, 1, 3, 4))
    # -> assuming ROW_MAJOR: (N_STITCHED_MATRICES, N_GRID_ROWS, N_GRID_COLS, ROW_DIM_PER_MATRIX, COL_DIM_PER_MATRIX)

    matrices_stitched = []
    for i in range(N_STITCHED_MATRICES):
        matrices_stitched.append( stitch_matrices(sub_matrices[i]) )
    matrices_stitched = np.array(matrices_stitched)
    
    if COL_MAJOR:
        matrices_stitched = matrices_stitched.transpose((0, 2, 1))
    
    matrices_stitched_contigious = np.ascontiguousarray(matrices_stitched)  # views only possible on contigious memory.
    matrices_serial = matrices_stitched_contigious.view(np.float32).flatten()

    with open("data/uut_output.txt", "a") as file:
        for elem in matrices_serial:
            file.write(str(elem) + "\n")
