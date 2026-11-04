#!/usr/bin/env python3
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
Generate random float/cfloat matrices with a guaranteed bounded condition number,
for use as input stimuli in the mp_pseudoinverse_svd test.

Background
----------
The one-sided Jacobi SVD used by the mp_pseudoinverse_svd kernel requires more sweep
passes (TP_PASSES) to converge when the input matrix has a high condition number.
Standard random integer stimuli from gen_input.tcl can produce matrices with
condition numbers of order 100 or more by chance, which will not converge within
a small number of passes (e.g. PASSES=3), causing the functional validation to
fail even when the implementation is algorithmically correct.

This script generates matrices of the form

    A = U * diag(sigma) * V^H        (for float:  V = V^T,  U real semi-orthogonal)
                                      (for cfloat: V unitary, U complex semi-unitary)

where U (DIM_ROWS x DIM_COLS) and V (DIM_COLS x DIM_COLS) are random
(semi-)unitary matrices and sigma are DIM_COLS singular values uniformly spaced
in the interval [scale/MAX_COND, scale].  This construction gives
cond(A) == MAX_COND exactly (assuming exact arithmetic), so the stimulus
condition number is fully controlled.

Data-type handling
------------------
float : generates a real matrix.  Two scalar float32 values per output line.
        e.g.  "A[row0,col0] A[row1,col0]\n"

cfloat: generates a complex matrix.  One complex cfloat sample per output line,
        written as its real and imaginary parts:
        e.g.  "re(A[row0,col0]) im(A[row0,col0])\n"

Both formats match the PLIO 64-bit stream convention used by gen_input.tcl and
the rest of the test infrastructure (helper.mk, depad_output.py, validation
scripts).

Usage
-----
    python3 gen_conditioned_input.py
        --output    <output file>
        --dim_rows  <TP_DIM_ROWS>
        --dim_cols  <TP_DIM_COLS>
        --niter     <iterations>
        --seed      <random seed>
        --data_type <float|cfloat>   (default: float)
        --max_cond  <maximum condition number  (default: 10.0)>
        --scale     <maximum singular value    (default: 32000.0)>
        --stim_type <0|1>            (default: 0)

STIM_TYPE values
----------------
    0  Random conditioned matrix (default behaviour).
         - float:  real conditioned matrix.
         - cfloat: fully complex conditioned matrix (non-zero real and imaginary parts).
    1  Real-valued matrix wrapped as cfloat (imaginary parts all zero).
         - float:  identical to STIM_TYPE=0 (float data has no imaginary component).
         - cfloat: a well-conditioned *real* matrix is generated and written with
                   imaginary=0.  The Moore-Penrose pseudoinverse of a real matrix is also
                   real, so ref_output will have imaginary=0 throughout.  Any non-zero
                   imaginary component in the UUT output therefore indicates a bug in the
                   complex arithmetic path (e.g. incorrect conjugation in MatMul2).
                   This makes STIM_TYPE=1 the simplest diagnostic stimulus for cfloat.
"""

import argparse
import sys

try:
    import numpy as np
except ImportError:
    print(
        "ERROR: gen_conditioned_input.py requires numpy.  "
        "Ensure the Python environment has numpy installed.",
        file=sys.stderr,
    )
    sys.exit(1)


# ---------------------------------------------------------------------------
# Matrix generators
# ---------------------------------------------------------------------------

def gen_conditioned_matrix_real(rng, dim_rows, dim_cols, max_cond, scale):
    """
    Return a (dim_rows x dim_cols) real float64 matrix with condition number
    exactly equal to max_cond and maximum singular value equal to scale.

    Algorithm:
      1. Generate dim_cols singular values sigma_k = scale/max_cond + k*(scale - scale/max_cond)/(m-1)
         so sigma ranges from scale/max_cond to scale (condition = scale / (scale/max_cond) = max_cond).
      2. Generate random orthogonal V (dim_cols x dim_cols) via QR decomposition of a
         random normal matrix (sign-corrected for uniqueness).
      3. Generate random semi-orthogonal U (dim_rows x dim_cols) as the first dim_cols
         columns of a random dim_rows x dim_rows orthogonal matrix (from QR).
      4. Return A = U * diag(sigma) * V^T.
    """
    n, m = dim_rows, dim_cols

    # Singular values: m values from sigma_min to sigma_max
    sigma_min = scale / max_cond
    sigma_max = scale
    if m == 1:
        sigmas = np.array([sigma_max], dtype=np.float64)
    else:
        sigmas = np.linspace(sigma_min, sigma_max, m)

    # Random orthogonal V (m x m): QR of real random normal, fix sign for uniqueness
    B = rng.standard_normal((m, m))
    Q_v, R_v = np.linalg.qr(B)
    Q_v = Q_v * np.sign(np.diag(R_v))  # make diagonal of R positive

    # Random semi-orthogonal U (n x m): first m columns of QR of n x n real random
    A_rand = rng.standard_normal((n, n))
    Q_u, R_u = np.linalg.qr(A_rand)
    Q_u = Q_u * np.sign(np.diag(R_u))
    U = Q_u[:, :m]   # n x m, columns are orthonormal

    # A = U * diag(sigmas) * V^T  => cond(A) = sigma_max / sigma_min = max_cond
    return U @ np.diag(sigmas) @ Q_v.T


def gen_conditioned_matrix_complex(rng, dim_rows, dim_cols, max_cond, scale):
    """
    Return a (dim_rows x dim_cols) complex128 matrix with condition number
    exactly equal to max_cond and maximum singular value equal to scale.

    Algorithm (complex variant):
      1. Same real singular values sigma_k as the real case.
      2. Generate random unitary V (dim_cols x dim_cols) via QR of a complex
         random normal matrix (sign-corrected so the diagonal of R is positive-real).
      3. Generate random complex semi-unitary U (dim_rows x dim_cols) as the first
         dim_cols columns of a QR factorisation of a complex random normal matrix.
      4. Return A = U * diag(sigma) * V^H.

    The resulting A is complex with cond(A) = max_cond (same as the real case,
    since singular values are real and unchanged by the unitary factors).
    """
    n, m = dim_rows, dim_cols

    sigma_min = scale / max_cond
    sigma_max = scale
    if m == 1:
        sigmas = np.array([sigma_max], dtype=np.float64)
    else:
        sigmas = np.linspace(sigma_min, sigma_max, m)

    # Random unitary V (m x m): QR of complex random normal
    B = (rng.standard_normal((m, m)) + 1j * rng.standard_normal((m, m)))
    Q_v, R_v = np.linalg.qr(B)
    # Sign correction: make diagonal of R positive-real so Q is unique
    Q_v = Q_v * np.sign(np.diag(R_v).real)

    # Random complex semi-unitary U (n x m): first m columns of n x n unitary
    A_rand = (rng.standard_normal((n, n)) + 1j * rng.standard_normal((n, n)))
    Q_u, R_u = np.linalg.qr(A_rand)
    Q_u = Q_u * np.sign(np.diag(R_u).real)
    U = Q_u[:, :m]   # n x m semi-unitary

    # A = U * diag(sigmas) * V^H
    return U @ np.diag(sigmas) @ Q_v.conj().T


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def main():
    parser = argparse.ArgumentParser(
        description=(
            "Generate well-conditioned random matrices as input stimuli for "
            "the mp_pseudoinverse_svd AIE test.  Supports float and cfloat data types."
        )
    )
    parser.add_argument("--output",    required=True,
                        help="Output file path (PLIO text format)")
    parser.add_argument("--dim_rows",  type=int, required=True,
                        help="Number of matrix rows (TP_DIM_ROWS)")
    parser.add_argument("--dim_cols",  type=int, required=True,
                        help="Number of matrix columns (TP_DIM_COLS)")
    parser.add_argument("--niter",     type=int, default=1,
                        help="Number of test iterations / matrices to generate (default: 1)")
    parser.add_argument("--seed",      type=int, default=1,
                        help="Random seed for reproducibility (default: 1)")
    parser.add_argument("--data_type", default="float",
                        choices=["float", "cfloat"],
                        help="AIE data type: float or cfloat (default: float)")
    parser.add_argument("--max_cond",  type=float, default=10.0,
                        help="Maximum condition number of each generated matrix (default: 10.0)")
    parser.add_argument("--scale",     type=float, default=32000.0,
                        help="Maximum singular value / approximate element scale (default: 32000.0)")
    parser.add_argument("--stim_type", type=int, default=0, choices=[0, 1],
                        help=(
                            "Stimulus type (default: 0).  "
                            "0=random conditioned matrix; "
                            "1=real-valued matrix wrapped as cfloat (imag=0, "
                            "pseudoinverse is also real — diagnostic for complex path)."
                        ))
    args = parser.parse_args()

    n, m       = args.dim_rows, args.dim_cols
    niter      = args.niter
    max_cond   = args.max_cond
    scale      = args.scale
    stim_type  = args.stim_type
    is_complex = (args.data_type == "cfloat")

    # For STIM_TYPE=1 with cfloat: generate a real matrix, write with imag=0.
    # This is diagnostic: A^+ of a real matrix is real (imag=0).
    # float is unaffected (no imaginary component exists anyway).
    force_real = (stim_type == 1)

    if max_cond < 1.0:
        print(f"ERROR: --max_cond must be >= 1.0 (got {max_cond})", file=sys.stderr)
        sys.exit(1)

    rng = np.random.default_rng(args.seed)

    lines_out = []

    for _it in range(niter):
        if is_complex:
            # -----------------------------------------------------------------
            # cfloat: generate complex matrix, write 1 complex element per line
            # PLIO 64-bit = 1 cfloat = (real:float32, imag:float32).
            # Output line format: "<real> <imag>\n"
            # -----------------------------------------------------------------
            if force_real:
                # STIM_TYPE=1: real-valued matrix wrapped as cfloat (imag=0).
                # The pseudoinverse of a real matrix is real — this makes the
                # expected output trivially verifiable (ref imag == 0 throughout).
                A_real = gen_conditioned_matrix_real(rng, n, m, max_cond, scale)
                flat = A_real.flatten(order='F')  # column-major real float64
                for v in flat:
                    lines_out.append(f"{float(v):.9e} {0.0:.9e}\n")
            else:
                # STIM_TYPE=0: fully complex conditioned matrix.
                A = gen_conditioned_matrix_complex(rng, n, m, max_cond, scale)
                flat = A.flatten(order='F')  # column-major, complex128 elements
                for v in flat:
                    lines_out.append(f"{float(v.real):.9e} {float(v.imag):.9e}\n")
        else:
            # -----------------------------------------------------------------
            # float: generate real matrix, write 2 float samples per line
            # PLIO 64-bit = 2 float32 = 2 real samples.
            # Output line format: "<val0> <val1>\n"
            # -----------------------------------------------------------------
            A = gen_conditioned_matrix_real(rng, n, m, max_cond, scale)
            flat = A.real.flatten(order='F')   # column-major, float64
            for k in range(0, len(flat), 2):
                v0 = float(flat[k])
                v1 = float(flat[k + 1]) if (k + 1) < len(flat) else 0.0
                lines_out.append(f"{v0:.9e} {v1:.9e}\n")

    with open(args.output, "w") as f:
        f.writelines(lines_out)

    sigma_min = scale / max_cond
    type_label = "complex" if is_complex else "real"
    if force_real and is_complex:
        type_label = "real-wrapped-cfloat (STIM_TYPE=1)"
    print(
        f"gen_conditioned_input.py: generated {niter} x ({n}x{m}) {type_label} matrices  "
        f"cond={max_cond:.1f}  sigma=[{sigma_min:.4g}, {scale:.4g}]  "
        f"seed={args.seed}  data_type={args.data_type}  stim_type={stim_type}  -> {args.output}"
    )


if __name__ == "__main__":
    main()
