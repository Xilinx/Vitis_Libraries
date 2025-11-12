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

for i in range(len(sys.argv)):
    if sys.argv[i] == "--A":
        a_dir = sys.argv[i + 1]
    if sys.argv[i] == "--L":
        l_dir = sys.argv[i + 1]
    if sys.argv[i] == "--data_type":
        data_type = sys.argv[i + 1]
    if sys.argv[i] == "--dim_size":
        dim_size = int(sys.argv[i + 1])
    if sys.argv[i] == "--num_frames":
        num_frames = int(sys.argv[i + 1])
    if sys.argv[i] == "--niter":
        niter = int(sys.argv[i + 1])
    if sys.argv[i] == "--model_ut":
        model_ut = sys.argv[i + 1]        

complex_data=False    
if data_type =="cfloat":
    complex_data=True

if 'order_a' not in locals():
    order_a = 'column-major'
if 'order_l' not in locals():
    order_l = 'column-major'


num_matrices=num_frames*niter
a_mat=read_matrix_from_file(a_dir, dim_size, dim_size, num_matrices, order_a, complex_data)
l_mat=read_matrix_from_file(l_dir, dim_size, dim_size, num_matrices, order_l, complex_data)
l_mat_T=conjugate_transpose_matrices(l_mat) #transpose of L matrix

tolerance_upper_tri=1e-2
tolerance_mul=1e-1
check_mul=multiply_and_compare(l_mat, l_mat_T, a_mat, tol=tolerance_mul)
check_l_mat=is_triangular(l_mat, triangular_type='lower', tol=tolerance_upper_tri)

report_validation(check_mul, "Multiplication Check", model_ut)
report_validation(check_l_mat, "Lower Triangular Check", model_ut)

if check_mul and check_l_mat == True:#this step is for the designer to double check if all tests are performed.
    report_validation(check_mul and check_l_mat, "All Checks", model_ut)

