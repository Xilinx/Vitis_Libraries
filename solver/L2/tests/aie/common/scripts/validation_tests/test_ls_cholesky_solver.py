#
# Copyright (C) 2026, Advanced Micro Devices, Inc.
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
"""
Validation script for the ls_cholesky_solver AIE IP.

The IP solves Ax = y where A is a symmetric positive definite (SPD) matrix.

Validation strategy
-------------------
Given:
  A  -- input SPD matrix (written column-major in the input file)
  y  -- input right-hand-side vector
  x  -- UUT output solution vector

The check performed is:

  Residual check: verify that  A * x  approximately equals  y

  Specifically, for each problem instance k:
    residual = A_k * x_k - y_k
    The test passes if max(|residual|) <= tolerance * max(1, max(|y_k|))

This validates the correctness of the full Cholesky + forward/backward
substitution chain without requiring a separate reference output file.
Floating-point accumulation through sqrt and inverse operations means a
relative tolerance is expected and appropriate.

Usage
-----
  python3 test_ls_cholesky_solver.py \\
      --A          <path/to/in_A.txt>        \\
      --x          <path/to/uut_output.txt>  \\
      --y          <path/to/in_y.txt>        \\
      --dim_size   <N>                       \\
      --data_type  <float|cfloat>            \\
      --num_frames <F>                       \\
      --niter      <I>                       \\
      [--tolerance <tol>]                    \\
      [--model_ut  <tag_string>]
"""

from verification_utils import read_matrix_from_file, report_validation
import sys
import numpy as np


# ---------------------------------------------------------------------------
# Parse command-line arguments
# ---------------------------------------------------------------------------
a_dir      = None
x_dir      = None
y_dir      = None
dim_size   = None
data_type  = None
num_frames = None
niter      = None
tolerance  = None
model_ut   = "UUT"

for i in range(len(sys.argv)):
    if sys.argv[i] == "--A":
        a_dir = sys.argv[i + 1]
    if sys.argv[i] == "--x":
        x_dir = sys.argv[i + 1]
    if sys.argv[i] == "--y":
        y_dir = sys.argv[i + 1]
    if sys.argv[i] == "--dim_size":
        dim_size = int(sys.argv[i + 1])
    if sys.argv[i] == "--data_type":
        data_type = sys.argv[i + 1]
    if sys.argv[i] == "--num_frames":
        num_frames = int(sys.argv[i + 1])
    if sys.argv[i] == "--niter":
        niter = int(sys.argv[i + 1])
    if sys.argv[i] == "--tolerance":
        tolerance = float(sys.argv[i + 1])
    if sys.argv[i] == "--model_ut":
        model_ut = sys.argv[i + 1]

# Default tolerance: Cholesky + two substitution stages accumulate
# floating-point error.  A tolerance of 10% of the RHS magnitude is
# generally acceptable for single-precision float/cfloat on AIE.
if tolerance is None:
    tolerance = 1e-1

complex_data = (data_type == "cfloat")

# ---------------------------------------------------------------------------
# Load data files
# ---------------------------------------------------------------------------
num_matrices = num_frames * niter

# A is dim_size x dim_size, stored column-major
a_mat = read_matrix_from_file(
    a_dir, dim_size, dim_size, num_matrices,
    order='column-major', complex_data=complex_data)

# y is a column vector (dim_size x 1), stored column-major
y_mat = read_matrix_from_file(
    y_dir, dim_size, 1, num_matrices,
    order='column-major', complex_data=complex_data)

# x (UUT output) is stored the same way as y (dim_size x 1, column-major)
x_mat = read_matrix_from_file(
    x_dir, dim_size, 1, num_matrices,
    order='column-major', complex_data=complex_data)

# ---------------------------------------------------------------------------
# Residual check:  A * x  ≈  y
#
# For each matrix instance k, compute:
#   residual  = A_k * x_k - y_k
#   abs_error = max(|residual|)            [infinity norm of error]
#   y_scale   = max(|y_k|, 1.0)           [infinity norm of RHS, floored at 1]
#
# Pass condition:  abs_error <= tolerance * y_scale
#
# Using a mixed absolute/relative tolerance avoids false failures for
# near-zero RHS vectors while remaining sensitive to large errors.
# ---------------------------------------------------------------------------
check_residual = True

for k in range(num_matrices):
    A = a_mat[k]             # shape: (dim_size, dim_size)
    x = x_mat[k].flatten()  # shape: (dim_size,)
    y = y_mat[k].flatten()  # shape: (dim_size,)

    Ax       = A @ x
    residual = Ax - y

    abs_error = float(np.max(np.abs(residual)))
    y_scale   = float(max(np.max(np.abs(y)), 1.0))

    if abs_error > tolerance * y_scale:
        check_residual = False
        break

# ---------------------------------------------------------------------------
# Report results
# ---------------------------------------------------------------------------
report_validation(check_residual, "Residual Check (A*x approx y)", model_ut)

if check_residual:
    report_validation(True, "All Checks", model_ut)
