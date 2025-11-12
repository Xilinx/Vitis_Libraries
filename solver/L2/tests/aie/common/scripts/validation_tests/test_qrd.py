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
from verification_utils import *
import sys
import warnings 


for i in range(len(sys.argv)):
    if sys.argv[i] == "--A":
        a_dir = sys.argv[i + 1]
    if sys.argv[i] == "--Q":
        q_dir = sys.argv[i + 1]
    if sys.argv[i] == "--R":
        r_dir = sys.argv[i + 1]
    if sys.argv[i] == "--dim_rows":
        dim_rows = int(sys.argv[i + 1])
    if sys.argv[i] == "--dim_cols":
        dim_cols = int(sys.argv[i + 1])
    if sys.argv[i] == "--data_type":
        data_type = sys.argv[i + 1]
    if sys.argv[i] == "--dim_a_leading":
        dim_a_leading = int(sys.argv[i + 1])
    if sys.argv[i] == "--dim_q_leading":
        dim_q_leading = int(sys.argv[i + 1])
    if sys.argv[i] == "--dim_r_leading":
        dim_r_leading = int(sys.argv[i + 1])
    if sys.argv[i] == "--num_frames":
        num_frames = int(sys.argv[i + 1])
    if sys.argv[i] == "--niter":
        niter = int(sys.argv[i + 1])
    if sys.argv[i] == "--model_ut":
        model_ut = sys.argv[i + 1]        

complex_data=False    
if data_type =="cfloat":
    complex_data=True

if 'dim_a_leading' not in locals():
    dim_a_leading = 0
if 'dim_q_leading' not in locals():
    dim_q_leading = 0
if 'dim_r_leading' not in locals():
    dim_r_leading = 0

if dim_a_leading == 0: order_a = 'column-major'
elif dim_a_leading == 1: order_a = 'row-major'
if dim_q_leading == 0: order_q = 'column-major'
elif dim_q_leading == 1: order_q = 'row-major'
if dim_r_leading == 0: order_r = 'column-major' 
elif dim_r_leading == 1: order_r = 'row-major'    

num_matrices=num_frames*niter
a_mat=read_matrix_from_file(a_dir, dim_rows, dim_cols, num_matrices, order_a, complex_data)
q_mat=read_matrix_from_file(q_dir, dim_rows, dim_cols, num_matrices, order_q, complex_data)
r_mat=read_matrix_from_file(r_dir, dim_cols, dim_cols, num_matrices, order_r, complex_data)

tolerance=0.005
check_mul=multiply_and_compare(q_mat, r_mat, a_mat, tol=tolerance)
check_r_mat=is_triangular(r_mat, triangular_type='upper')
check_q_mat_orthogonal_col=is_orthogonal(q_mat, order='column-major', tol=tolerance)
check_q_mat_orthonormal_col=is_orthonormal(q_mat, order='column-major', tol=tolerance)
check_q_mat_orthogonal_row=is_orthogonal(q_mat, order='row-major', tol=tolerance)
check_q_mat_orthonormal_row=is_orthonormal(q_mat, order='row-major', tol=tolerance)


report_validation(check_mul, "Multiplication Check", model_ut)
report_validation(check_r_mat, "Upper Triangular Check", model_ut)
if dim_rows == dim_cols: #orthogonality and orthonormality are only valid for square Q matrices, otherwise they are reported as warnings
    report_validation(check_q_mat_orthogonal_col, "Orthogonality Check - Column Major", model_ut)
    report_validation(check_q_mat_orthonormal_col, "Orthonormality Check - Column Major", model_ut)
    report_validation(check_q_mat_orthogonal_row, "Orthogonality Check - Row Major", model_ut)
    report_validation(check_q_mat_orthonormal_row, "Orthonormality Check - Row Major", model_ut)
    if check_mul and check_r_mat and check_q_mat_orthogonal_col and check_q_mat_orthogonal_col and check_q_mat_orthogonal_row and check_q_mat_orthogonal_row == True:#this step is for the designer to double check if all tests are performed.
        report_validation(check_mul and check_r_mat and check_q_mat_orthogonal_col and check_q_mat_orthogonal_col and check_q_mat_orthogonal_row and check_q_mat_orthogonal_row, "All Checks", model_ut)

else:
    report_warning(check_q_mat_orthogonal_col, "Orthogonality Check - Column Major", model_ut)
    report_warning(check_q_mat_orthonormal_col, "Orthonormality Check - Column Major", model_ut)
    report_warning(check_q_mat_orthogonal_row, "Orthogonality Check - Row Major", model_ut)
    report_warning(check_q_mat_orthonormal_row, "Orthonormality Check - Row Major", model_ut)
    if check_mul and check_r_mat == True:#this step is for the designer to double check if all tests are performed.
        report_validation(check_mul and check_r_mat, "All Checks", model_ut)

