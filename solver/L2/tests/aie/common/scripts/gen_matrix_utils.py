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
import random
import numpy as np

# Define AI Engine variants
AIE = 1
AIE_ML = 2
AIE_MLv2 = 22

k_data_memory_bytes = {AIE: 32768, AIE_ML: 65536, AIE_MLv2: 65536}
data_width_bit_dict = {
    "float": 32,
    "cfloat": 64,
}

def sizeof(data_type):
    if data_type in data_width_bit_dict:
        return data_width_bit_dict[data_type] // 8
    else:
        raise ValueError(f"Data type {data_type} not supported.")
    
    
def generate_real_matrix(M, N, low=-2**7, high=2**7):
    matrix = [ [] for _ in range(M) ]
    for i in range(M):
        for j in range(N):
            matrix[i].append(random.randint(low, high))
    return matrix

def generate_complex_matrix(M, N, low=-2**7, high=2**7):
    matrix_real = generate_real_matrix(M, N, low, high)
    matrix_imag = generate_real_matrix(M, N, low, high)# * 1j
    matrix_complex = [ [] for _ in range(M) ]
    for i in range(M):
        for j in range(N):
            matrix_complex[i].append(complex(matrix_real[i][j], matrix_imag[i][j]))
    return matrix_complex


def get_transpose(matrix):
    """ This only works with 2D matrices. """
    M = len(matrix)
    N = len(matrix[0])
    matrix_transpose = [ [] for _ in range(N) ]
    for i in range(M):
        for j in range(N):
            matrix_transpose[i].append(matrix[j][i])
    return matrix_transpose

def get_conjugate(matrix):
    M = len(matrix)
    matrix_conjugate = [ [] for _ in range(M) ]
    for i in range(M):
        for j in range(M):
            matrix_conjugate[i].append( matrix[i][j].conjugate() )
    return matrix_conjugate


def matrix_mult(A, B):
    rows_A, cols_A = len(A), len(A[0])
    rows_B, cols_B = len(B), len(B[0])

    #check if the matrices can be multiplied
    if cols_A != rows_B:
        raise ValueError("Cannot multiply matrices:  incompatible dimensions.")
    #create the result matrix 
    C =[[0 for _ in range(cols_B)] for _ in range(rows_A)]
    for i in range(rows_A):
        for j in range(cols_B):
            for k in range(cols_A):
                C[i][j] += A[i][k] * B[k][j]
    return C

def get_hermetian(matrix):
    """
    If a real matrix is fed as input, this function will return a symmetric matrix. Only works on M*M matrices.
    """
    matrix_conj_T = get_transpose(get_conjugate(matrix))
    return matrix_mult(matrix, matrix_conj_T)

def get_sub_matrices(matrix, n_rows, n_cols):
    """
    Returns a 2D list containing the sub-matrices which grid together to make the input matrix.
    Grid is defined by row_split and col_split.
    """
    matrix = np.array(matrix)
    sub_matrices = [[None for j in range(n_cols)] for i in range(n_rows)]

    matrix_row_split = np.split(matrix, n_rows)
    for i, sub_matrix_row in enumerate(matrix_row_split):
        sub_matrix_row_col_split = np.split(sub_matrix_row.T, n_cols)
        for j, sub_matrix_row_col in enumerate(sub_matrix_row_col_split):
            sub_matrices[i][j] = sub_matrix_row_col.T

    return sub_matrices

def get_serial(matrix, row_major=True):
    """
    Serializes matrix.
    """
    matrix = np.array(matrix)
    matrix_serial = []
    
    if row_major:
        matrix_flat = matrix.flatten('C')  # C-style (row-major)
    else:
        matrix_flat = matrix.flatten('F')  # Fortran-style (col-major)
    
    if isinstance(matrix_flat[0], complex):
        for elem in matrix_flat:
            matrix_serial.append(int(elem.real))
            matrix_serial.append(int(elem.imag))
    else:
        matrix_serial = matrix_flat

    return matrix_serial
