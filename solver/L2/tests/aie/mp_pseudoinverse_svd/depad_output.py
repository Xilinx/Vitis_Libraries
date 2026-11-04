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
Strip kStoreCols alignment-padding rows from the mp_pseudoinverse_svd UUT output.

The production graph (mp_pseudoinverse_svd_graph) outputs A^+ as a
  (kStoreCols x DIM_ROWS)  col-major buffer per iteration,
where kStoreCols >= DIM_COLS is the SVD alignment boundary set by the SVD
kernel's vector-store alignment requirements.

The pipeline computes A^+ = V * S^{-1} * U^T where V is passed directly from
the SVD output (unmodified) and S^{-1} is a diagonal matrix with 1/s_k on
the diagonal for valid singular values (k < DIM_COLS) and zero for the
padding positions (k >= DIM_COLS, since those SVD S outputs are 0).  The
padding rows A^+[DIM_COLS:kStoreCols, :] are stripped by this script before
comparison with the reference model output.

For the common case where DIM_COLS is already a multiple of kStoreAlign,
kStoreCols == DIM_COLS and this script is a no-op (copies the file verbatim).

Usage (called from helper.mk):
    python3 depad_output.py
        --input       <padded UUT output file>
        --output      <depadded output file>
        --dim_cols    <TP_DIM_COLS>
        --dim_rows    <TP_DIM_ROWS>
        --data_type   <float|cfloat>
        --niter       <number of iterations>
        --aie_variant <1|2|22>
"""

import argparse
import shutil
import sys


def compute_kstorecols(dim_cols, data_type, aie_variant="1"):
    """
    Mirror the kStoreCols calculation from the AIE graph / kernel
    (fnVecStoreAlign() in svd_traits.hpp).

    kStoreAlign is the vector-store alignment in samples: bit_width / 8 /
    sizeof(sample). bit_width is 256-bit on AIE1/AIE-ML (AIE_VARIANT 1/2),
    but 512-bit on AIE-MLv2 (AIE_VARIANT 22) -- see fnVecStoreAlign()'s
    "__AIE_ARCH__ >= 22" branch. Using the wrong width here silently
    produces a depadded file half (or double) the size the AIE_VARIANT=22
    graph actually emits.

    For float  (32-bit): kStoreAlign = 8  (256-bit) or 16 (512-bit, AIE-MLv2)
    For cfloat (64-bit): kStoreAlign = 4  (256-bit) or 8  (512-bit, AIE-MLv2)
    kSchedCols = dim_cols + (dim_cols % 2)  [round up to even]
    kStoreCols = ceil(kSchedCols / kStoreAlign) * kStoreAlign
    """
    bit_width = 512 if aie_variant == "22" else 256
    sample_bytes = 8 if data_type == "cfloat" else 4
    k_store_align = bit_width // 8 // sample_bytes
    k_sched_cols = dim_cols + (dim_cols % 2)
    k_store_cols = ((k_sched_cols + k_store_align - 1) // k_store_align) * k_store_align
    return k_store_cols


def main():
    parser = argparse.ArgumentParser(
        description="Strip kStoreCols padding rows from mp_pseudoinverse_svd UUT output."
    )
    parser.add_argument("--input",     required=True,
                        help="Padded UUT output file (kStoreCols x DIM_ROWS per iteration)")
    parser.add_argument("--output",    required=True,
                        help="Depadded output file (DIM_COLS x DIM_ROWS per iteration)")
    parser.add_argument("--dim_cols",  type=int, required=True, help="TP_DIM_COLS")
    parser.add_argument("--dim_rows",  type=int, required=True, help="TP_DIM_ROWS")
    parser.add_argument("--data_type", default="float",
                        help="Data type: float or cfloat (default: float)")
    parser.add_argument("--niter",     type=int, default=1,
                        help="Number of kernel iterations (default: 1)")
    parser.add_argument("--aie_variant", default="1",
                        help="AIE_VARIANT: 1 (AIE1), 2 (AIE-ML), or 22 (AIE-MLv2, 512-bit "
                             "vector-store alignment). Default: 1")
    args = parser.parse_args()

    n            = args.dim_cols
    m            = args.dim_rows
    data_type    = args.data_type
    niter        = args.niter
    aie_variant  = args.aie_variant

    k_store_cols = compute_kstorecols(n, data_type, aie_variant)

    if k_store_cols == n:
        # No padding — copy verbatim
        shutil.copy(args.input, args.output)
        print(f"depad_output.py: kStoreCols == DIM_COLS == {n}; copied {args.input} -> {args.output}")
        return

    # The AIE PLIO writes 2 float values per text line in all cases:
    #   - float : each line contains 2 scalar samples
    #   - cfloat: each line contains 1 complex sample (real, imag)
    if data_type == "cfloat":
        samples_per_line = 1
    else:
        samples_per_line = 2

    # Lines per column in the padded input and the valid output
    lines_per_col_in  = k_store_cols // samples_per_line
    lines_per_col_out = n            // samples_per_line

    # Sanity check
    expected_in_lines = niter * m * lines_per_col_in
    with open(args.input, "r") as f_in:
        in_lines = f_in.readlines()

    if len(in_lines) != expected_in_lines:
        print(
            f"WARNING: depad_output.py: expected {expected_in_lines} lines in "
            f"{args.input} but found {len(in_lines)}. "
            f"(niter={niter}, DIM_ROWS={m}, kStoreCols={k_store_cols}, "
            f"samples_per_line={samples_per_line})",
            file=sys.stderr
        )

    out_lines = []
    line_idx  = 0
    for _it in range(niter):
        for _col in range(m):
            # Keep the first DIM_COLS rows (valid pseudoinverse elements)
            out_lines.extend(in_lines[line_idx : line_idx + lines_per_col_out])
            # Advance past the full padded column (including discarded padding rows)
            line_idx += lines_per_col_in

    with open(args.output, "w") as f_out:
        f_out.writelines(out_lines)

    print(
        f"depad_output.py: {args.input} -> {args.output} "
        f"({niter}x{m} cols, kept {lines_per_col_out}/{lines_per_col_in} lines/col, "
        f"kStoreCols={k_store_cols} DIM_COLS={n})"
    )


if __name__ == "__main__":
    main()
