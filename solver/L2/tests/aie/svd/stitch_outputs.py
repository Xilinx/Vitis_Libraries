#!/usr/bin/env python3
"""
Stitch SVD Outputs from Cascaded Kernels

This script combines the outputs from multiple cascaded SVD kernels into single
output files. The input data is split in a column-major "card cutting" method
where each kernel processes a portion of each column.

Supports arbitrary TP_DIM_COLS (including odd and non-multiple-of-kVecSize).
The kernel internally pads columns:
  cols_even:  next even number >= TP_DIM_COLS  (for parallel pairs schedule)
  cols_padded: next multiple of kVecSize       (for V SIMD alignment)
Output files use padded sizes. The verify script extracts true sub-blocks.

Usage:
    python3 stitch_outputs.py --rows <TP_DIM_ROWS> --cols <TP_DIM_COLS> \\
                              --casc_len <TP_CASC_LEN> --niter <NITER> \\
                              --data_dir <path_to_data_directory>

Example from Makefile:
    python3 stitch_outputs.py --rows $(DIM_ROWS) --cols $(DIM_COLS) \\
                              --casc_len $(CASC_LEN) --niter $(NITER) \\
                              --data_dir ./x86simulator_output/data
"""

import numpy as np
import argparse
from pathlib import Path


def get_padded_dims(cols, data_type, aie_variant=1):
    """Compute internal storage padding (kStoreCols) matching the kernel.

    kStoreAlign is 256-bit on AIE1/AIE-ML (vec_size 8/4) but 512-bit on
    AIE22 (vec_size 16/8). This matches fnVecStoreAlign() in svd_traits.hpp.
    S and U outputs are padded to cols_padded; V is always compacted to
    cols x cols by the kernel before output.
    """
    if aie_variant >= 22:
        vec_size = 8 if data_type == 'cfloat' else 16
    else:
        vec_size = 4 if data_type == 'cfloat' else 8
    cols_sched = cols + (cols % 2)
    cols_padded = ((cols_sched + vec_size - 1) // vec_size) * vec_size
    return cols_padded


def read_data_file(filepath, dtype=np.float32):
    """
    Read data from file (2 floats per line format)
    Returns flattened array of all values
    """
    data = []
    with open(filepath, 'r') as f:
        for line in f:
            values = line.strip().split()
            data.extend([dtype(v) for v in values])
    return np.array(data, dtype=dtype)


def write_data_file(filepath, data, samples_per_line=2):
    """
    Write data to file (2 floats per line format)
    Data should be provided as a flattened array
    """
    with open(filepath, 'w') as f:
        for i in range(0, len(data), samples_per_line):
            line_data = data[i:i + samples_per_line]
            f.write(' '.join([f'{val:.9e}' for val in line_data]) + ' \n')


def stitch_matrix_outputs(data_dir, matrix_name, rows, cols, casc_len, niter, dtype,
                          floats_per_elem=1):
    """
    Stitch matrix outputs (U or V) from cascaded kernels
    
    For column-major data with card-cutting:
    - Each kernel holds rows_per_kernel consecutive rows of each column
    - rows_per_kernel = rows / casc_len
    - Data is interleaved: kernel 0 has rows [0:rows_per_kernel] of all columns,
      kernel 1 has rows [rows_per_kernel:2*rows_per_kernel] of all columns, etc.
    
    For cfloat data, each matrix element is stored as 2 consecutive float32 values
    (real, imag), so floats_per_elem=2. All index arithmetic operates on float32
    counts, not element counts.
    
    Args:
        data_dir: Directory containing output files
        matrix_name: 'U' or 'V'
        rows: Number of rows in the matrix
        cols: Number of columns in the matrix (padded, as output by kernel)
        casc_len: Number of cascade kernels
        niter: Number of iterations
        dtype: Data type (np.float32 or np.float64)
        floats_per_elem: Number of float32 values per matrix element (1 for float, 2 for cfloat)
    
    Returns:
        Stitched matrix data as flattened array (column-major, in float32 values)
    """
    rows_per_kernel = rows // casc_len
    
    # Load all kernel outputs
    # Format: uut_output_<matrix_name>_<k>_0.txt where k is the cascade index
    kernel_data = []
    for k in range(casc_len):
        filepath = data_dir / f'uut_output_{matrix_name}_0_{k}.txt'
        data = read_data_file(filepath, dtype)
        kernel_data.append(data)
    
    # Stitch the data
    # Each kernel has: rows_per_kernel * cols * floats_per_elem * niter float samples
    # We need to interleave them to reconstruct the full matrix
    stitched_data = []
    
    # Number of float32 values per kernel per iteration
    floats_per_kernel_per_iter = rows_per_kernel * cols * floats_per_elem
    # Number of float32 values per column-chunk per kernel
    floats_per_col_chunk = rows_per_kernel * floats_per_elem
    
    for iter_idx in range(niter):
        # For each iteration, reconstruct the full matrix
        for col in range(cols):
            # For each column, gather rows from all kernels
            for k in range(casc_len):
                # Calculate the starting index for this kernel's contribution
                # to this column in this iteration (in float32 values)
                kernel_offset = iter_idx * floats_per_kernel_per_iter
                column_offset = col * floats_per_col_chunk
                start_idx = kernel_offset + column_offset
                end_idx = start_idx + floats_per_col_chunk
                
                # Extract this kernel's portion of the column
                stitched_data.extend(kernel_data[k][start_idx:end_idx])
    
    return np.array(stitched_data, dtype=dtype)


def stitch_vector_outputs(data_dir, niter, cols, dtype):
    """
    Read S vector output (singular values always sorted, identical on all kernels).
    Reads from kernel 0; all kernels produce the same S.

    Args:
        data_dir: Directory containing output files
        niter: Number of iterations
        cols: Number of singular values (padded, as output by kernel)
        dtype: Data type

    Returns:
        S vector data as flattened array
    """
    filepath = data_dir / f'uut_output_S_0_0.txt'
    s_data = read_data_file(filepath, dtype)
    
    # S contains cols values per iteration, for niter iterations
    return s_data[:cols * niter]


def main():
    parser = argparse.ArgumentParser(
        description='Stitch SVD outputs from cascaded kernels',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Example:
    python3 stitch_outputs.py --rows 16 --cols 4 --casc_len 4 --niter 1 \\
                              --data_dir ./x86simulator_output/data
        """
    )
    
    parser.add_argument('--rows', type=int, required=True,
                        help='TP_DIM_ROWS: Number of rows in the matrix')
    parser.add_argument('--cols', type=int, required=True,
                        help='TP_DIM_COLS: Number of columns in the matrix')
    parser.add_argument('--casc_len', type=int, required=True,
                        help='TP_CASC_LEN: Number of cascade kernels')
    parser.add_argument('--niter', type=int, required=True,
                        help='NITER: Number of SVD iterations')
    parser.add_argument('--data_dir', type=str, required=True,
                        help='Directory containing uut_output files')
    parser.add_argument('--dtype', type=str, default='float32',
                        choices=['float32', 'float64'],
                        help='Data type (default: float32)')
    parser.add_argument('--data_type', type=str, default='float',
                        choices=['float', 'cfloat'],
                        help='TT_DATA type - float or cfloat (default: float)')
    parser.add_argument('--aie_variant', type=int, default=1,
                        help='AIE variant (1=AIE1, 2=AIE-ML, 22=AIE22). '
                             'AIE22 uses 512-bit storage alignment so cols_padded=16 for float.')
    
    args = parser.parse_args()
    
    # Convert data type
    dtype = np.float32 if args.dtype == 'float32' else np.float64
    is_complex = (args.data_type == 'cfloat')
    # cfloat elements are stored as 2 consecutive float32 values (real, imag)
    floats_per_elem = 2 if is_complex else 1
    
    data_dir = Path(args.data_dir)
    
    # Compute padded dimensions (must match kernel's internal padding)
    cols_padded = get_padded_dims(args.cols, args.data_type, args.aie_variant)
    
    print("="*70)
    print("Stitching SVD Outputs from Cascaded Kernels")
    print("="*70)
    print(f"Configuration:")
    print(f"  Rows:        {args.rows}")
    print(f"  Cols:        {args.cols}")
    if cols_padded != args.cols:
        print(f"  Cols (pad):  {cols_padded}")
    print(f"  Cascade Len: {args.casc_len}")
    print(f"  Iterations:  {args.niter}")
    print(f"  Data Dir:    {data_dir}")
    print(f"  Data Type:   {args.data_type} (floats_per_elem={floats_per_elem})")
    print("="*70)
    
    # Validate
    if args.rows % args.casc_len != 0:
        print(f"ERROR: rows ({args.rows}) must be divisible by casc_len ({args.casc_len})")
        return 1
    
    rows_per_kernel = args.rows // args.casc_len
    print(f"\nRows per kernel: {rows_per_kernel}")
    
    # Print expected sizes per file (in float32 values, not elements)
    # For cfloat, each element is 2 floats (real + imag)
    # S is always real regardless of TT_DATA
    # Output files use PADDED sizes: U and S use cols_padded, V uses cols_padded x cols_padded.
    # All output buffers are multiples of 16 bytes for DMA alignment on AIE1.
    U_per_kernel_per_iter = rows_per_kernel * cols_padded * floats_per_elem
    S_per_kernel_per_iter = cols_padded
    # V is always compacted to cols x cols by the kernel (see svd.cpp V compaction step)
    V_per_kernel_per_iter = cols_padded * cols_padded * floats_per_elem  # full kStoreCols² output
    
    print(f"\nExpected samples per file (per kernel, per iter -> total with NITER={args.niter}):")
    print(f"  U per kernel:  {U_per_kernel_per_iter} x {args.niter} = {U_per_kernel_per_iter * args.niter}")
    print(f"  S per kernel:  {S_per_kernel_per_iter} x {args.niter} = {S_per_kernel_per_iter * args.niter}")
    print(f"  V per kernel:  {V_per_kernel_per_iter} x {args.niter} = {V_per_kernel_per_iter * args.niter}")
    print(f"\nExpected stitched output sizes (all iters, padded):")
    print(f"  U stitched:    {args.rows * cols_padded} x {args.niter} = {args.rows * cols_padded * args.niter}")
    print(f"  S stitched:    {cols_padded} x {args.niter} = {cols_padded * args.niter}")
    print(f"  V stitched:    {args.cols * args.cols} x {args.niter} = {args.cols * args.cols * args.niter} (compacted cols x cols)")
    
    # Validate actual file sizes
    for k in range(args.casc_len):
        u_path = data_dir / f'uut_output_U_0_{k}.txt'
        if u_path.exists():
            u_data = read_data_file(u_path, dtype)
            expected = U_per_kernel_per_iter * args.niter
            actual = len(u_data)
            status = "✓" if actual == expected else f"✗ MISMATCH"
            print(f"  U kernel {k}: actual={actual}, expected={expected} {status}")
    
    s_path = data_dir / f'uut_output_S_0_0.txt'
    if s_path.exists():
        s_data = read_data_file(s_path, dtype)
        expected = S_per_kernel_per_iter * args.niter
        actual = len(s_data)
        status = "✓" if actual == expected else f"✗ MISMATCH"
        print(f"  S kernel 0: actual={actual}, expected={expected} {status}")
    
    v_path = data_dir / f'uut_output_V_0_0.txt'
    if v_path.exists():
        v_data_check = read_data_file(v_path, dtype)
        expected = V_per_kernel_per_iter * args.niter
        actual = len(v_data_check)
        status = "✓" if actual == expected else f"✗ MISMATCH"
        print(f"  V kernel 0: actual={actual}, expected={expected} {status}")
    
    # Check that expected input files exist before attempting to stitch
    missing = []
    for k in range(args.casc_len):
        for name in [f'uut_output_U_0_{k}.txt']:
            if not (data_dir / name).exists():
                missing.append(name)
    for name in ['uut_output_S_0_0.txt', 'uut_output_V_0_0.txt']:
        if not (data_dir / name).exists():
            missing.append(name)
    if missing:
        print(f"\nERROR: Missing output files (simulation may have failed):")
        for f in missing:
            print(f"  {data_dir / f}")
        return 1

    # Stitch U matrix (using cols_padded for DMA-aligned output)
    print(f"\nStitching U matrix ({args.rows}x{cols_padded}, floats_per_elem={floats_per_elem})...")
    U_stitched = stitch_matrix_outputs(
        data_dir, 'U', args.rows, cols_padded, args.casc_len, args.niter, dtype,
        floats_per_elem=floats_per_elem
    )
    output_U_path = data_dir / 'uut_output_U.txt'
    write_data_file(output_U_path, U_stitched)
    print(f"  ✓ Written to {output_U_path}")

    # V matrix: kernel compacts V to cols×cols at the start of a kStoreCols×kStoreCols
    # buffer. The port outputs kStoreCols×kStoreCols per iteration; extract only the
    # first cols×cols compact block from each iteration's worth of output.
    print(f"\nCopying V matrix ({args.cols}x{args.cols}, compacted from {cols_padded}x{cols_padded}) from kernel 0...")
    V_filepath = data_dir / 'uut_output_V_0_0.txt'
    V_data_raw = read_data_file(V_filepath, dtype)
    v_full_per_iter = cols_padded * cols_padded * floats_per_elem   # full output per iter
    v_compact_per_iter = args.cols * args.cols * floats_per_elem    # compact block per iter
    V_data = np.concatenate([
        V_data_raw[it * v_full_per_iter : it * v_full_per_iter + v_compact_per_iter]
        for it in range(args.niter)
    ])
    output_V_path = data_dir / 'uut_output_V.txt'
    write_data_file(output_V_path, V_data)
    print(f"  ✓ Written to {output_V_path}")

    # S vector (cols_padded values per iteration, identical on all kernels)
    print(f"\nReading S vector ({cols_padded} values per iter) from kernel 0...")
    S_stitched = stitch_vector_outputs(
        data_dir, args.niter, cols_padded, dtype
    )
    output_S_path = data_dir / 'uut_output_S.txt'
    write_data_file(output_S_path, S_stitched)
    print(f"  ✓ Written to {output_S_path}")
    
    print("\n" + "="*70)
    print("✅ Stitching completed successfully!")
    print("="*70)
    print(f"\nOutput files (padded sizes, verify script extracts true {args.cols}-col sub-blocks):")
    print(f"  {output_U_path}")
    print(f"  {output_S_path}")
    print(f"  {output_V_path}")
    print("="*70)
    
    return 0


if __name__ == '__main__':
    exit(main())
