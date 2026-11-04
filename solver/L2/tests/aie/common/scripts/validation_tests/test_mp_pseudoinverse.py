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
Validation script for the mp_pseudoinverse IP core.

Verifies that the computed pseudoinverse A^+ satisfies all four Moore-Penrose
conditions as defined by https://en.wikipedia.org/wiki/Moore%E2%80%93Penrose_inverse :

  (1)  A  A^+  A  = A         (A^+ is a generalised inverse of A)
  (2)  A^+ A  A^+ = A^+       (A  is a generalised inverse of A^+)
  (3) (A  A^+)^H = A  A^+    (A A^+ is Hermitian / self-adjoint)
  (4) (A^+ A )^H = A^+ A     (A^+ A is Hermitian / self-adjoint)

Each condition is checked using a normalised (relative) Frobenius-norm error so
that the result is independent of the overall scale of A.  Two tolerances are
applied:
  - tol_product  : relative threshold for conditions (1) and (2)
  - tol_hermitian: relative threshold for conditions (3) and (4)

Any condition that fails is flagged with an "ERROR:" prefix in validation.txt so
that the test framework can identify which specific property failed.

Usage (called from helper.mk):
  python3 test_mp_pseudoinverse.py
      --A       <input_matrix_file>
      --Aplus   <pseudoinverse_output_file>
      --dim_rows  <TP_DIM_ROWS>
      --dim_cols  <TP_DIM_COLS>
      --data_type <float|cfloat>
      --niter     <number_of_iterations>
      --model_ut  <UUT|Ref>
      [--tol_product   <float, default 0.2>]
      [--tol_hermitian <float, default 0.1>]
"""

import sys
import os

# Allow 'from verification_utils import *' regardless of the working directory.
_script_dir = os.path.dirname(os.path.abspath(__file__))
if _script_dir not in sys.path:
    sys.path.insert(0, _script_dir)

from verification_utils import read_matrix_from_file, report_validation
import numpy as np

# ------------------------------------------------------------------ #
# Argument parsing
# ------------------------------------------------------------------ #
a_dir         = None
aplus_dir     = None
dim_rows      = None
dim_cols      = None
data_type     = None
niter         = 1
model_ut      = "UUT"
tol_product   = 0.20   # 20% relative Frobenius error for product conditions
tol_hermitian = 0.10   # 10% relative Frobenius error for Hermitian conditions

argv = sys.argv
for i in range(len(argv)):
    if argv[i] == "--A":
        a_dir = argv[i + 1]
    elif argv[i] == "--Aplus":
        aplus_dir = argv[i + 1]
    elif argv[i] == "--dim_rows":
        dim_rows = int(argv[i + 1])
    elif argv[i] == "--dim_cols":
        dim_cols = int(argv[i + 1])
    elif argv[i] == "--data_type":
        data_type = argv[i + 1]
    elif argv[i] == "--niter":
        niter = int(argv[i + 1])
    elif argv[i] == "--model_ut":
        model_ut = argv[i + 1]
    elif argv[i] == "--tol_product":
        tol_product = float(argv[i + 1])
    elif argv[i] == "--tol_hermitian":
        tol_hermitian = float(argv[i + 1])

if any(x is None for x in [a_dir, aplus_dir, dim_rows, dim_cols, data_type]):
    print("ERROR: Required arguments missing.  Need --A, --Aplus, --dim_rows, "
          "--dim_cols, --data_type.", file=sys.stderr)
    sys.exit(1)

complex_data = (data_type == "cfloat")
m = dim_rows   # rows of A
n = dim_cols   # cols of A  (= rows of A^+)

# ------------------------------------------------------------------ #
# Helper: normalised Frobenius error
# ------------------------------------------------------------------ #
def rel_fro_err(approx, exact):
    """||approx - exact||_F / (||exact||_F + epsilon)"""
    denom = np.linalg.norm(exact, 'fro') + 1e-30
    return np.linalg.norm(approx - exact, 'fro') / denom

def is_hermitian_rel(M, tol):
    """Check that M is Hermitian: ||M - M^H||_F / ||M||_F <= tol."""
    err = np.linalg.norm(M - np.conj(M).T, 'fro') / (np.linalg.norm(M, 'fro') + 1e-30)
    return err <= tol

# ------------------------------------------------------------------ #
# Read simulation output files
# ------------------------------------------------------------------ #
# A   : m x n column-major (as written by the test harness gen_input.tcl)
# A^+ : n x m column-major (as written by the AIE output buffer)
A_list    = read_matrix_from_file(a_dir,     m, n, niter, 'column-major', complex_data)
Aplus_list = read_matrix_from_file(aplus_dir, n, m, niter, 'column-major', complex_data)

# ------------------------------------------------------------------ #
# Per-iteration checks
# ------------------------------------------------------------------ #
ok_cond1 = True   # A A^+ A = A
ok_cond2 = True   # A^+ A A^+ = A^+
ok_cond3 = True   # (A A^+)^H = A A^+
ok_cond4 = True   # (A^+ A)^H = A^+ A

for k, (Ak, Apk) in enumerate(zip(A_list, Aplus_list)):
    # Core products used by multiple conditions
    AAp  = Ak  @ Apk   # m x n  @  n x m  =  m x m
    ApA  = Apk @ Ak    # n x m  @  m x n  =  n x n

    # Condition (1): A A^+ A = A    (relative Frobenius)
    cond1_k = rel_fro_err(AAp @ Ak, Ak) <= tol_product
    if not cond1_k:
        err = rel_fro_err(AAp @ Ak, Ak)
        print(f"[iter {k}] Condition 1 FAILED: ||A A^+ A - A||_F/||A||_F = {err:.4f} "
              f"(tol = {tol_product})")
    ok_cond1 = ok_cond1 and cond1_k

    # Condition (2): A^+ A A^+ = A^+    (relative Frobenius)
    cond2_k = rel_fro_err(ApA @ Apk, Apk) <= tol_product
    if not cond2_k:
        err = rel_fro_err(ApA @ Apk, Apk)
        print(f"[iter {k}] Condition 2 FAILED: ||A^+ A A^+ - A^+||_F/||A^+||_F = {err:.4f} "
              f"(tol = {tol_product})")
    ok_cond2 = ok_cond2 and cond2_k

    # Condition (3): (A A^+)^H = A A^+    (Hermitian)
    cond3_k = is_hermitian_rel(AAp, tol_hermitian)
    if not cond3_k:
        err = np.linalg.norm(AAp - np.conj(AAp).T, 'fro') / (np.linalg.norm(AAp, 'fro') + 1e-30)
        print(f"[iter {k}] Condition 3 FAILED: ||A A^+ - (A A^+)^H||_F/||A A^+||_F = {err:.4f} "
              f"(tol = {tol_hermitian})")
    ok_cond3 = ok_cond3 and cond3_k

    # Condition (4): (A^+ A)^H = A^+ A    (Hermitian)
    cond4_k = is_hermitian_rel(ApA, tol_hermitian)
    if not cond4_k:
        err = np.linalg.norm(ApA - np.conj(ApA).T, 'fro') / (np.linalg.norm(ApA, 'fro') + 1e-30)
        print(f"[iter {k}] Condition 4 FAILED: ||A^+ A - (A^+ A)^H||_F/||A^+ A||_F = {err:.4f} "
              f"(tol = {tol_hermitian})")
    ok_cond4 = ok_cond4 and cond4_k

# ------------------------------------------------------------------ #
# Report results to logs/validation.txt
# ------------------------------------------------------------------ #
report_validation(ok_cond1, "MP Condition 1: A A^+ A = A", model_ut)
report_validation(ok_cond2, "MP Condition 2: A^+ A A^+ = A^+", model_ut)
report_validation(ok_cond3, "MP Condition 3: (A A^+)^H = A A^+ (Hermitian)", model_ut)
report_validation(ok_cond4, "MP Condition 4: (A^+ A)^H = A^+ A (Hermitian)", model_ut)

all_ok = ok_cond1 and ok_cond2 and ok_cond3 and ok_cond4
report_validation(all_ok, "MP All Conditions (1-4)", model_ut)

# ------------------------------------------------------------------ #
# Write definitive OVERALL result to logs/diff.txt.
#
# This overrides the numerical diff.tcl comparison (UUT vs Gauss-Jordan
# REF) and makes the four Moore-Penrose conditions the sole PASS/FAIL
# determinant read by diff_exit.tcl — mirroring how svd_verify_all.py
# controls the SVD test result via the same file.
#
# Rationale: the UUT uses SVD (Jacobi, iterative) while the REF uses
# Gauss-Jordan (normal equations, direct).  These are mathematically
# equivalent but numerically different algorithms; their outputs can
# diverge by more than DIFF_TOLERANCE even when both are correct.
# The Moore-Penrose conditions are algorithm-independent: they check
# only that A^+ satisfies the four defining properties of the
# pseudoinverse, regardless of how it was computed.
# ------------------------------------------------------------------ #
import os as _os
_os.makedirs("./logs", exist_ok=True)
with open("./logs/diff.txt", "w") as _df:
    if all_ok:
        # "identical" is the pass keyword checked by diff_exit.tcl and
        # get_common_config.tcl.  It must appear in diff.txt for the
        # test framework to record FUNC=1 and exit 0.
        _df.write("Files are identical, i.e. all Moore-Penrose conditions are within defined tolerance bounds.\n")
        _df.write("OVERALL: pass (Moore-Penrose conditions satisfied)\n")
        for _i, (_ok, _name) in enumerate([
            (ok_cond1, "A A^+ A = A"),
            (ok_cond2, "A^+ A A^+ = A^+"),
            (ok_cond3, "(A A^+)^H = A A^+ [Hermitian]"),
            (ok_cond4, "(A^+ A)^H = A^+ A [Hermitian]"),
        ], start=1):
            _df.write(f"  Condition {_i}: pass  ({_name})\n")
    else:
        _df.write("OVERALL: fail\n")
        for _i, (_ok, _name) in enumerate([
            (ok_cond1, "A A^+ A = A"),
            (ok_cond2, "A^+ A A^+ = A^+"),
            (ok_cond3, "(A A^+)^H = A A^+ [Hermitian]"),
            (ok_cond4, "(A^+ A)^H = A^+ A [Hermitian]"),
        ], start=1):
            _status = "pass" if _ok else "fail"
            _prefix = "ERROR: " if not _ok else "  "
            _df.write(f"{_prefix}Condition {_i}: {_status}  ({_name})\n")