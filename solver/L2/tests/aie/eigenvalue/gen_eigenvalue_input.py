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
Generate random square float/cfloat matrices with known, well-separated
eigenvalues, for use as input stimuli in the eigenvalue test.

Background
----------
The QR-algorithm's convergence rate for a given (deflating) eigenvalue
depends on the ratio of its magnitude to its neighbour's -- closely-spaced
(near-degenerate) eigenvalue magnitudes converge extremely slowly and could
fail to deflate within a small TP_PASSES, or (for the reference model's
fixed 500-sweep unshifted iteration) fail to converge at all. This script
therefore constructs matrices with EXACTLY KNOWN, geometrically well
-separated eigenvalues by similarity transform:

    A = Q * diag(eigs) * Q^H     (Q unitary/orthogonal, chosen via QR of a
                                   random Gaussian matrix)

Since Q is unitary, A is similar to diag(eigs), so eigenvalues(A) = eigs
exactly (up to floating point), regardless of whether eigs are real or
complex.

Data-type handling
-------------------
float : eigs are chosen real, and Q is real orthogonal. Since eigs is real
        and Q is orthogonal, A = Q diag(eigs) Q^T is automatically
        SYMMETRIC -- this is required for TT_DATA=float (see the library
        documentation: the QR algorithm in real arithmetic only fully
        converges to a diagonal form when all eigenvalues are real, which
        is only guaranteed for symmetric/normal matrices). Two scalar
        float32 values per output line (matches the PLIO 64-bit stream
        convention).

cfloat: eigs are chosen with well-separated magnitudes and random phases
        (genuinely complex, demonstrating the complex-eigenvalue case), and
        Q is complex unitary. One complex cfloat sample per output line,
        written as its real and imaginary parts.

Output is flattened ROW-MAJOR (matches the eigenvalue_graph's documented
row-major input contract for A -- unlike mp_pseudoinverse/svd, which use
column-major).

Usage
-----
    python3 gen_eigenvalue_input.py
        --output    <output file>
        --dim_size  <TP_DIM>
        --niter     <iterations>
        --seed      <random seed>
        --data_type <float|cfloat>   (default: float)
        --scale     <largest eigenvalue magnitude (default: 1000.0)>
        --ratio     <geometric ratio between adjacent sorted eigenvalue
                      magnitudes (default: 0.85)>
"""

import argparse
import sys

try:
    import numpy as np
except ImportError:
    print(
        "ERROR: gen_eigenvalue_input.py requires numpy.  "
        "Ensure the Python environment has numpy installed.",
        file=sys.stderr,
    )
    sys.exit(1)


def gen_matrix_with_known_eigs_real(rng, n, scale, ratio):
    """Return an (n x n) real, SYMMETRIC float64 matrix with exactly known,
    geometrically well-separated real eigenvalues."""
    eigs = scale * (ratio ** np.arange(n))

    B = rng.standard_normal((n, n))
    Q, R = np.linalg.qr(B)
    Q = Q * np.sign(np.diag(R))  # sign-correct for a well-defined orthogonal Q

    return Q @ np.diag(eigs) @ Q.T


def gen_matrix_with_known_eigs_complex(rng, n, scale, ratio):
    """Return an (n x n) complex128 matrix with exactly known, geometrically
    well-separated complex eigenvalues (random phases)."""
    magnitudes = scale * (ratio ** np.arange(n))
    phases = rng.uniform(0.0, 2.0 * np.pi, n)
    eigs = magnitudes * np.exp(1j * phases)

    B = rng.standard_normal((n, n)) + 1j * rng.standard_normal((n, n))
    Q, R = np.linalg.qr(B)
    Q = Q * np.sign(np.diag(R).real)  # sign-correct for a well-defined unitary Q

    return Q @ np.diag(eigs) @ Q.conj().T


def main():
    parser = argparse.ArgumentParser(
        description=(
            "Generate random square matrices with known, well-separated "
            "eigenvalues as input stimuli for the eigenvalue AIE test. "
            "Supports float and cfloat data types."
        )
    )
    parser.add_argument("--output", required=True, help="Output file path (PLIO text format)")
    parser.add_argument("--dim_size", type=int, required=True, help="Dimension of the square matrix (TP_DIM)")
    parser.add_argument(
        "--niter", type=int, default=1, help="Number of test iterations / matrices to generate (default: 1)"
    )
    parser.add_argument("--seed", type=int, default=1, help="Random seed for reproducibility (default: 1)")
    parser.add_argument(
        "--data_type", default="float", choices=["float", "cfloat"], help="AIE data type: float or cfloat (default: float)"
    )
    parser.add_argument(
        "--scale", type=float, default=1000.0, help="Largest eigenvalue magnitude (default: 1000.0)"
    )
    parser.add_argument(
        "--ratio",
        type=float,
        default=0.85,
        help="Geometric ratio between adjacent sorted eigenvalue magnitudes (default: 0.85)",
    )
    args = parser.parse_args()

    n = args.dim_size
    is_complex = args.data_type == "cfloat"
    rng = np.random.default_rng(args.seed)

    lines_out = []
    for _it in range(args.niter):
        if is_complex:
            A = gen_matrix_with_known_eigs_complex(rng, n, args.scale, args.ratio)
            flat = A.flatten(order="C")  # row-major, complex128 elements
            for v in flat:
                lines_out.append(f"{float(v.real):.9e} {float(v.imag):.9e}\n")
        else:
            A = gen_matrix_with_known_eigs_real(rng, n, args.scale, args.ratio)
            flat = A.real.flatten(order="C")  # row-major, float64
            for k in range(0, len(flat), 2):
                v0 = float(flat[k])
                v1 = float(flat[k + 1]) if (k + 1) < len(flat) else 0.0
                lines_out.append(f"{v0:.9e} {v1:.9e}\n")

    with open(args.output, "w") as f:
        f.writelines(lines_out)

    type_label = "complex" if is_complex else "real symmetric"
    print(
        f"gen_eigenvalue_input.py: generated {args.niter} x ({n}x{n}) {type_label} matrices  "
        f"scale={args.scale:.4g}  ratio={args.ratio}  seed={args.seed}  "
        f"data_type={args.data_type}  -> {args.output}"
    )


if __name__ == "__main__":
    main()
