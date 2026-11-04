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
Validation script for the eigenvalue IP core.

Verifies that the computed eigenvalues E of a square matrix A satisfy three
algorithm-independent conditions, without requiring E to be in any
particular order (the UUT and REF deflate/converge eigenvalues in different
orders) and without comparing E to a second independently-computed
eigenvalue set (which would itself require an order-independent assignment
problem to be solved):

  (1) Trace:       sum(E)  ~= trace(A)
  (2) Determinant: prod(E) ~= det(A)      (compared in the log-magnitude
                                            domain -- see below -- to stay
                                            numerically well-behaved for
                                            large matrices, where the raw
                                            product/determinant can be many
                                            orders of magnitude beyond a
                                            useful relative-error comparison,
                                            though still within float64 range)
  (3) Per-eigenvalue residual: for every lambda in E, the smallest singular
      value of (A - lambda*I) must be small relative to ||A|| -- i.e. lambda
      really is close to an eigenvalue of A. This is the strongest check: it
      alone would catch a "wrong but plausible" eigenvalue that a trace/
      determinant check could miss via a compensating error elsewhere in E.

Usage (called from helper.mk):
  python3 test_eigenvalue.py
      --A         <input_matrix_file>
      --E         <eigenvalues_output_file>
      --dim_size  <TP_DIM>
      --data_type <float|cfloat>
      --niter     <number_of_iterations>
      --model_ut  <UUT|Ref>
      [--tolerance <float, default 0.05>]
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
a_dir = None
e_dir = None
dim_size = None
data_type = None
niter = 1
model_ut = "UUT"
tolerance = 0.05

argv = sys.argv
for i in range(len(argv)):
    if argv[i] == "--A":
        a_dir = argv[i + 1]
    elif argv[i] == "--E":
        e_dir = argv[i + 1]
    elif argv[i] == "--dim_size":
        dim_size = int(argv[i + 1])
    elif argv[i] == "--data_type":
        data_type = argv[i + 1]
    elif argv[i] == "--niter":
        niter = int(argv[i + 1])
    elif argv[i] == "--model_ut":
        model_ut = argv[i + 1]
    elif argv[i] == "--tolerance":
        tolerance = float(argv[i + 1])

if any(x is None for x in [a_dir, e_dir, dim_size, data_type]):
    print(
        "ERROR: Required arguments missing.  Need --A, --E, --dim_size, --data_type.",
        file=sys.stderr,
    )
    sys.exit(1)

complex_data = data_type == "cfloat"
n = dim_size

# ------------------------------------------------------------------ #
# Read simulation output files
# ------------------------------------------------------------------ #
# A : n x n (row-major, per the eigenvalue_graph input contract)
# E : 1 x n (the row-major/column-major distinction is irrelevant for a vector)
A_list = read_matrix_from_file(a_dir, n, n, niter, "row-major", complex_data)
E_list = read_matrix_from_file(e_dir, 1, n, niter, "row-major", complex_data)

# ------------------------------------------------------------------ #
# Per-iteration checks
# ------------------------------------------------------------------ #
ok_trace = True  # sum(E) = trace(A)
ok_det = True  # prod(E) = det(A) (log-magnitude domain)
ok_residual = True  # min singular value of (A - lambda*I) ~= 0 for every lambda in E

for k, (Ak, Ek) in enumerate(zip(A_list, E_list)):
    eigs = np.asarray(Ek).flatten()

    # ---- Condition (1): trace ----
    trace_A = np.trace(Ak)
    trace_E = np.sum(eigs)
    trace_err = abs(trace_E - trace_A) / (abs(trace_A) + 1e-30)
    cond1_k = trace_err <= tolerance
    if not cond1_k:
        print(f"[iter {k}] Condition 1 (trace) FAILED: |sum(E)-trace(A)|/|trace(A)| = {trace_err:.4f} (tol = {tolerance})")
    ok_trace = ok_trace and cond1_k

    # ---- Condition (2): determinant, compared in log-magnitude domain ----
    # Raw products of many eigenvalues can span hundreds of orders of
    # magnitude for larger matrices -- comparing log|det| avoids requiring
    # a huge-dynamic-range relative-error comparison to be meaningful.
    _sign_A, logabsdet_A = np.linalg.slogdet(Ak)
    logabsdet_E = np.sum(np.log(np.abs(eigs) + 1e-300))
    det_err = abs(logabsdet_E - logabsdet_A) / (abs(logabsdet_A) + 1.0)
    cond2_k = det_err <= tolerance
    if not cond2_k:
        print(
            f"[iter {k}] Condition 2 (determinant) FAILED: "
            f"|log|prod(E)| - log|det(A)||/(|log|det(A)||+1) = {det_err:.4f} (tol = {tolerance})"
        )
    ok_det = ok_det and cond2_k

    # ---- Condition (3): per-eigenvalue residual ----
    normA = np.linalg.norm(Ak, "fro") + 1e-30
    for j, lam in enumerate(eigs):
        M = Ak - lam * np.eye(n, dtype=Ak.dtype)
        smin = np.linalg.svd(M, compute_uv=False)[-1]
        cond3_j = smin <= tolerance * normA
        if not cond3_j:
            print(
                f"[iter {k}] Condition 3 (residual) FAILED for E[{j}]={lam}: "
                f"min_sv(A - lambda*I)/||A||_F = {smin / normA:.4f} (tol = {tolerance})"
            )
        ok_residual = ok_residual and cond3_j

# ------------------------------------------------------------------ #
# Report results to logs/validation.txt
# ------------------------------------------------------------------ #
report_validation(ok_trace, "Eigenvalue Condition 1: sum(E) = trace(A)", model_ut)
report_validation(ok_det, "Eigenvalue Condition 2: prod(E) = det(A) [log-magnitude]", model_ut)
report_validation(ok_residual, "Eigenvalue Condition 3: min_sv(A - lambda*I) ~= 0 for every lambda in E", model_ut)

all_ok = ok_trace and ok_det and ok_residual
report_validation(all_ok, "Eigenvalue All Conditions (1-3)", model_ut)

# ------------------------------------------------------------------ #
# Write definitive OVERALL result to logs/diff.txt.
#
# This overrides the numerical diff.tcl comparison (UUT vs plain unshifted
# QR REF) and makes these three algorithm-independent conditions the sole
# PASS/FAIL determinant read by diff_exit.tcl -- mirroring
# test_mp_pseudoinverse.py's identical rationale: the UUT (Hessenberg +
# shifted/deflated QR) and REF (plain unshifted QR) are mathematically
# equivalent but numerically different algorithms, and eigenvalues have no
# guaranteed output order, so a raw numerical diff is not a valid pass/fail
# criterion here.
# ------------------------------------------------------------------ #
os.makedirs("./logs", exist_ok=True)
with open("./logs/diff.txt", "w") as _df:
    if all_ok:
        # "identical" is the pass keyword checked by diff_exit.tcl and
        # get_common_config.tcl.  It must appear in diff.txt for the test
        # framework to record FUNC=1 and exit 0.
        _df.write("Files are identical, i.e. all eigenvalue conditions are within defined tolerance bounds.\n")
        _df.write("OVERALL: pass (eigenvalue conditions satisfied)\n")
        for _i, (_ok, _name) in enumerate(
            [
                (ok_trace, "sum(E) = trace(A)"),
                (ok_det, "prod(E) = det(A) [log-magnitude]"),
                (ok_residual, "min_sv(A - lambda*I) ~= 0 for every lambda in E"),
            ],
            start=1,
        ):
            _df.write(f"  Condition {_i}: pass  ({_name})\n")
    else:
        _df.write("OVERALL: fail\n")
        for _i, (_ok, _name) in enumerate(
            [
                (ok_trace, "sum(E) = trace(A)"),
                (ok_det, "prod(E) = det(A) [log-magnitude]"),
                (ok_residual, "min_sv(A - lambda*I) ~= 0 for every lambda in E"),
            ],
            start=1,
        ):
            _status = "pass" if _ok else "fail"
            _prefix = "ERROR: " if not _ok else "  "
            _df.write(f"{_prefix}Condition {_i}: {_status}  ({_name})\n")
