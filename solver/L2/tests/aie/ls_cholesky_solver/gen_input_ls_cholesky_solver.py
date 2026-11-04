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
Generate input data files for the ls_cholesky_solver AIE test.

The IP solves Ax = y where:
  A  -- Hermitian positive definite (HPD) square matrix (DIM_SIZE x DIM_SIZE)
  y  -- right-hand side vector (DIM_SIZE)
  x  -- solution vector (DIM_SIZE)  [written as reference output]

Matrix generation strategy
--------------------------
A well-conditioned HPD matrix is built in three steps:

  1. Random seed matrix Z with small integer entries drawn from [-128, 128].
     For cfloat, both real and imaginary parts of Z are generated this way.

  2. Form A_raw = Z @ Z^H.  This is Hermitian positive semi-definite by
     construction.  The small entry range keeps intermediate values
     manageable even for large DIM_SIZE.

  3. Eigenvalue conditioning: decompose A_raw, clamp any eigenvalue that
     is below (max_eigenvalue / desired_condition_number), then reconstruct.
     This enforces a bounded condition number (default 10) and strict
     positive definiteness.

  4. Scale A so that max(|A|) <= INT16_MAX // 2 (~16383).  This keeps all
     matrix values within the int16 value range, preventing overflow and
     catastrophic cancellation in float32 Cholesky arithmetic on the AIE.

The y vector is drawn as random integers in [-32767, 32767] (int16 range).

All intermediate linear-algebra is done in float64 / complex128 for
accuracy; results are cast to float32 / complex64 for file output.

Output files
------------
  <loc_in_A>_gx_gy.txt   : sub-matrix tile (gx, gy) of A  (column-major,
                            lower-triangular grid of tiles)
  <loc_in_y>_gx_0.txt    : sub-vector slice gx of y
  <loc_ref_x>_gx_0.txt   : reference sub-vector slice gx of x = A^{-1} y
  <loc_full_A>            : full (untiled) A matrix for validation script
  <loc_full_y>            : full y vector for validation script
"""

import argparse
import os

import numpy as np


# ---------------------------------------------------------------------------
# File-writing helpers
# ---------------------------------------------------------------------------

def write_float_vec(path, vec, complex_data, append=False):
    """Append (or write) a float/cfloat vector to a text file.

    For float (real) data two values are packed onto each line to match the
    64-bit PLIO bus width used by the AIE simulator.
    For cfloat (complex) data each element occupies one line as
    '<real> <imag>'.
    """
    mode = "a" if append else "w"
    with open(path, mode) as f:
        if complex_data:
            for v in vec:
                f.write(f"{float(v.real)} {float(v.imag)}\n")
        else:
            flat = [float(x) for x in vec]
            # Pad to even length so every line carries two values.
            if len(flat) % 2 != 0:
                flat.append(0.0)
            for i in range(0, len(flat), 2):
                f.write(f"{flat[i]} {flat[i+1]}\n")


def write_float_mat_col_major(path, mat, complex_data, append=False):
    """Write a 2-D numpy matrix column-by-column (Fortran / column-major)."""
    col_major_vec = mat.flatten(order='F')
    write_float_vec(path, col_major_vec, complex_data, append=append)


# ---------------------------------------------------------------------------
# Matrix generation
# ---------------------------------------------------------------------------

def make_spd_matrix(dim_size, complex_data, rng, desired_cn=10,
                    entry_range=128, int16_target=16383):
    """Return a well-conditioned HPD matrix A scaled to int16 value range.

    Parameters
    ----------
    dim_size      : matrix dimension
    complex_data  : True for cfloat (Hermitian PD), False for float (SPD)
    rng           : numpy.random.Generator instance
    desired_cn    : target condition number upper bound (default 10)
    entry_range   : integer range for the random seed matrix Z entries,
                    drawn from [-entry_range, entry_range]
    int16_target  : target for max(|A|); keeps A within int16 value range

    Returns
    -------
    A  : numpy array, float32 (real) or complex64 (complex), shape (N, N)
    """
    # Step 1: random seed matrix with small integer entries
    Z_re = rng.integers(-entry_range, entry_range + 1,
                         size=(dim_size, dim_size)).astype(np.float64)
    if complex_data:
        Z_im = rng.integers(-entry_range, entry_range + 1,
                             size=(dim_size, dim_size)).astype(np.float64)
        Z = Z_re + 1j * Z_im
    else:
        Z = Z_re

    # Step 2: Hermitian PSD matrix  A_raw = Z @ Z^H
    A_raw = Z @ Z.conj().T

    # Step 3: Eigenvalue conditioning
    # np.linalg.eigh is numerically stable for Hermitian matrices and
    # returns real eigenvalues in ascending order.
    eigenvalues, eigenvectors = np.linalg.eigh(A_raw)

    # Guard against tiny negative eigenvalues from numerical noise.
    eigenvalues = np.abs(eigenvalues)

    max_eval = float(np.max(eigenvalues)) if np.max(eigenvalues) > 0.0 else 1.0
    min_threshold = max_eval / float(desired_cn)
    eigenvalues_adj = np.maximum(eigenvalues, min_threshold)

    # Reconstruct: A_wc = V @ diag(λ_adj) @ V^H
    A_wc = eigenvectors @ np.diag(eigenvalues_adj) @ eigenvectors.conj().T

    # Enforce exact Hermitian symmetry and (for complex) real diagonal.
    A_wc = (A_wc + A_wc.conj().T) * 0.5
    if complex_data:
        # Diagonal of a Hermitian matrix must be real.
        np.fill_diagonal(A_wc, A_wc.diagonal().real + 0j)

    # Step 4: Scale to int16 value range.
    max_elem = float(np.max(np.abs(A_wc)))
    if max_elem > 0.0:
        A_wc = A_wc * (int16_target / max_elem)

    # Cast to the target single-precision type.
    if complex_data:
        return A_wc.astype(np.complex64), A_wc  # float32 view + float64 copy
    else:
        return A_wc.real.astype(np.float32), A_wc.real  # float32 + float64


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def main():
    parser = argparse.ArgumentParser(
        description="Generate input/reference files for ls_cholesky_solver AIE test.")
    parser.add_argument("--data_type",  type=str, required=True,
                        help="Data type: 'float' or 'cfloat'")
    parser.add_argument("--dim_size",   type=int, required=True,
                        help="Matrix dimension (square, e.g. 8)")
    parser.add_argument("--grid_dim",   type=int, default=1,
                        help="Tiling grid dimension (default 1)")
    parser.add_argument("--seed",       type=int, default=1,
                        help="Random seed (reproducible generation)")
    parser.add_argument("--loc_in_A",   type=str, required=True,
                        help="Base path for tiled A input files "
                             "(suffix _gx_gy.txt appended)")
    parser.add_argument("--loc_in_y",   type=str, required=True,
                        help="Base path for y input files "
                             "(suffix _gx_0.txt appended)")
    parser.add_argument("--loc_ref_x",  type=str, required=True,
                        help="Base path for reference x output files "
                             "(suffix _gx_0.txt appended)")
    parser.add_argument("--loc_full_A", type=str, default=None,
                        help="Path for the full (untiled) A matrix "
                             "used by the validation script")
    parser.add_argument("--loc_full_y", type=str, default=None,
                        help="Path for the full y vector "
                             "used by the validation script")
    parser.add_argument("--niter",      type=int, required=True,
                        help="Total number of problem instances to generate "
                             "(NITER * NUM_FRAMES)")
    args = parser.parse_args()

    data_type  = args.data_type
    dim_size   = args.dim_size
    grid_dim   = args.grid_dim
    seed       = args.seed
    loc_in_A   = args.loc_in_A
    loc_in_y   = args.loc_in_y
    loc_ref_x  = args.loc_ref_x
    loc_full_A = args.loc_full_A
    loc_full_y = args.loc_full_y
    niter      = args.niter

    complex_data = (data_type == "cfloat")

    # Use the new Generator API for reproducibility.
    rng = np.random.default_rng(seed)

    INT16_MAX    = 32767
    INT16_TARGET = INT16_MAX // 2   # 16383 — comfortable headroom below int16 max

    # Ensure output directories exist.
    for base in [loc_in_A, loc_in_y, loc_ref_x]:
        d = os.path.dirname(base)
        if d:
            os.makedirs(d, exist_ok=True)

    kernel_dim = dim_size // grid_dim

    # Pre-clear all output files so append-mode writes start fresh.
    for gy in range(grid_dim):
        for gx in range(gy + 1):
            open(loc_in_A.replace(".txt", f"_{gx}_{gy}.txt"), "w").close()
    for gx in range(grid_dim):
        open(loc_in_y.replace(".txt",   f"_{gx}_0.txt"), "w").close()
        open(loc_ref_x.replace(".txt",  f"_{gx}_0.txt"), "w").close()
    if loc_full_A:
        open(loc_full_A, "w").close()
    if loc_full_y:
        open(loc_full_y, "w").close()

    # ------------------------------------------------------------------
    # Generate niter problem instances
    # ------------------------------------------------------------------
    for it in range(niter):

        # ---- Build a well-conditioned HPD matrix A in int16 range --------
        #
        # make_spd_matrix() returns:
        #   A_f32  : float32 / complex64  — written to input files
        #   A_f64  : float64 / complex128 — used for double-precision solve
        #
        A_f32, A_f64 = make_spd_matrix(
            dim_size, complex_data, rng,
            desired_cn=10,
            entry_range=128,
            int16_target=INT16_TARGET,
        )

        # ---- Right-hand side y: random int16-range integers ---------------
        if complex_data:
            y_re = rng.integers(-INT16_MAX, INT16_MAX + 1,
                                 dim_size).astype(np.float64)
            y_im = rng.integers(-INT16_MAX, INT16_MAX + 1,
                                 dim_size).astype(np.float64)
            y_f64 = y_re + 1j * y_im
            y_f32 = y_f64.astype(np.complex64)
        else:
            y_f64 = rng.integers(-INT16_MAX, INT16_MAX + 1,
                                   dim_size).astype(np.float64)
            y_f32 = y_f64.astype(np.float32)

        # ---- Reference solution x = A^{-1} y (double precision) ----------
        x_f64 = np.linalg.solve(A_f64, y_f64)
        if complex_data:
            x_f32 = x_f64.astype(np.complex64)
        else:
            x_f32 = x_f64.real.astype(np.float32)

        # ---- Write tiled A sub-matrices (column-major, lower-tri grid) ----
        for gy in range(grid_dim):
            for gx in range(gy + 1):
                row_start = gy * kernel_dim
                row_end   = row_start + kernel_dim
                col_start = gx * kernel_dim
                col_end   = col_start + kernel_dim
                A_tile = A_f32[row_start:row_end, col_start:col_end]
                path_A = loc_in_A.replace(".txt", f"_{gx}_{gy}.txt")
                write_float_mat_col_major(path_A, A_tile, complex_data,
                                          append=True)

        # ---- Write y slices (one file per grid column) --------------------
        for gx in range(grid_dim):
            y_slice = y_f32[gx * kernel_dim : (gx + 1) * kernel_dim]
            path_y  = loc_in_y.replace(".txt", f"_{gx}_0.txt")
            write_float_vec(path_y, y_slice, complex_data, append=True)

        # ---- Write reference x slices -------------------------------------
        for gx in range(grid_dim):
            x_slice = x_f32[gx * kernel_dim : (gx + 1) * kernel_dim]
            path_x  = loc_ref_x.replace(".txt", f"_{gx}_0.txt")
            write_float_vec(path_x, x_slice, complex_data, append=True)

        # ---- Write full (untiled) A and y for the validation script -------
        if loc_full_A:
            write_float_mat_col_major(loc_full_A, A_f32, complex_data,
                                      append=True)
        if loc_full_y:
            write_float_vec(loc_full_y, y_f32, complex_data, append=True)


if __name__ == "__main__":
    main()
