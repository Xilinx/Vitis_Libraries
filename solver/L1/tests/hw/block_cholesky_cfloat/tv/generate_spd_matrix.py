#!/usr/bin/env python3
# Copyright (C) 2025-2025, Advanced Micro Devices, Inc.
# SPDX-License-Identifier: Apache-2.0
#
# Generate Hermitian positive-definite complex<float>-compatible test matrix A
# for block_cholesky_cfloat HLS TB (readTxt + row-major A[r][c] layout).
#
# File format: N*N lines, each "real imag" (space-separated). Order is row-major:
#   A[0,0], A[0,1], ... A[0,N-1], A[1,0], ...

import argparse
import random
import sys
from typing import List, Optional


def conj(z: complex) -> complex:
    return complex(z.real, -z.imag)


def matmul_hermitian_left(M: List[List[complex]]) -> List[List[complex]]:
    """Return A = M^H @ M (N x N)."""
    n = len(M)
    A = [[0j] * n for _ in range(n)]
    for i in range(n):
        for j in range(n):
            s = 0j
            for k in range(n):
                s += conj(M[k][i]) * M[k][j]
            A[i][j] = s
    return A


def hermitian_symmetrize(A: List[List[complex]]) -> None:
    n = len(A)
    for i in range(n):
        for j in range(i + 1, n):
            v = (A[i][j] + conj(A[j][i])) * 0.5
            A[i][j] = v
            A[j][i] = conj(v)


def add_diagonal_shift(A: List[List[complex]], shift: float) -> None:
    n = len(A)
    for i in range(n):
        A[i][i] += complex(shift, 0.0)


def random_complex_matrix(n: int, rng: random.Random) -> List[List[complex]]:
    return [
        [complex(rng.gauss(0.0, 1.0), rng.gauss(0.0, 1.0)) for _ in range(n)]
        for __ in range(n)
    ]


def build_spd_matrix(n: int, seed: int, diag_shift: Optional[float]) -> List[List[complex]]:
    rng = random.Random(seed)
    M = random_complex_matrix(n, rng)
    A = matmul_hermitian_left(M)
    shift = float(n) if diag_shift is None else float(diag_shift)
    add_diagonal_shift(A, shift)
    hermitian_symmetrize(A)
    return A


def write_A_txt(path: str, A: List[List[complex]]) -> None:
    n = len(A)
    with open(path, "w", encoding="ascii") as f:
        for i in range(n):
            for j in range(n):
                z = A[i][j]
                f.write(f"{float(z.real)} {float(z.imag)} \n")


def try_numpy_build(n: int, seed: int, diag_shift: Optional[float]) -> Optional[List[List[complex]]]:
    try:
        import numpy as np
    except ImportError:
        return None
    rng = np.random.default_rng(seed)
    G = (rng.standard_normal((n, n)) + 1j * rng.standard_normal((n, n))).astype(np.complex128)
    A = (G.conj().T @ G).astype(np.complex128)
    shift = float(n) if diag_shift is None else float(diag_shift)
    A += shift * np.eye(n, dtype=np.complex128)
    A = ((A + A.conj().T) * 0.5).astype(np.complex128)
    return [[complex(A[i, j]) for j in range(n)] for i in range(n)]


def main() -> int:
    p = argparse.ArgumentParser(description="Generate Hermitian SPD A_N.txt for block_cholesky_cfloat TB.")
    p.add_argument("-n", "--dim", type=int, required=True, help="Matrix order N (e.g. 128, 256).")
    p.add_argument(
        "-o",
        "--output",
        type=str,
        default=None,
        help="Output path (default: ../datas/A_<N>.txt next to this tv/ folder).",
    )
    p.add_argument("--seed", type=int, default=42, help="RNG seed.")
    p.add_argument(
        "--diag-shift",
        type=float,
        default=None,
        help="Add shift*I after M^H M (default: N). Larger improves conditioning.",
    )
    p.add_argument(
        "--numpy",
        action="store_true",
        help="Use NumPy if available (faster for large N); fall back to pure Python if not.",
    )
    args = p.parse_args()
    n = args.dim
    if n < 1:
        print("error: N must be positive", file=sys.stderr)
        return 2
    if args.output:
        out = args.output
    else:
        out = f"../datas/A_{n}.txt"

    if args.numpy:
        A = try_numpy_build(n, args.seed, args.diag_shift)
        if A is None:
            print("warning: NumPy not installed, using pure Python", file=sys.stderr)
            A = build_spd_matrix(n, args.seed, args.diag_shift)
    else:
        A = build_spd_matrix(n, args.seed, args.diag_shift)

    write_A_txt(out, A)
    print(f"Wrote {n}x{n} SPD Hermitian matrix to {out} ({n * n} lines)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
