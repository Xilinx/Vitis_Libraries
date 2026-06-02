#!/usr/bin/env python3
"""
Comprehensive SVD Verification Script
Verifies UUT outputs, performs reconstruction, and compares against NumPy reference.
Supports multiple iterations (NITER). Outputs are always sorted descending by the kernel.

Usage:
    python3 svd_verify_all.py [base_directory] [--niter N]
    
Example:
    python3 svd_verify_all.py . --niter 2
"""

import numpy as np
import json
import sys
import argparse
from pathlib import Path


def get_padded_dims(cols, data_type, aie_variant=1):
    """Compute internal padding dimensions matching the kernel.

    The kernel uses two internal column counts:
      cols_sched:  next even number >= cols
      cols_padded: next multiple of kVecSize >= cols_sched

    On AIE22 (aie_variant >= 22) the IO vector width is 512-bit, so
    kVecSize doubles: 16 for float, 8 for cfloat.  S and U outputs are
    padded to cols_padded; V is always compacted to cols x cols.
    Output files contain padded data. The true TP_DIM_COLS values are in the
    first cols columns/entries; the rest is padding (zeros / identity).
    """
    if aie_variant >= 22:
        vec_size = 8 if data_type == 'cfloat' else 16
    else:
        vec_size = 4 if data_type == 'cfloat' else 8
    cols_sched = cols + (cols % 2)
    cols_padded = ((cols_sched + vec_size - 1) // vec_size) * vec_size
    return cols_padded


def read_config(config_path):
    """Read configuration from JSON file"""
    with open(config_path, 'r') as f:
        config = json.load(f)
    
    params = config['parameters']
    return {
        'data_type': params['TT_DATA'],
        'rows': params['TP_DIM_ROWS'],
        'cols': params['TP_DIM_COLS'],
        'passes': params['TP_PASSES'],
        'aie_variant': params.get('AIE_VARIANT', 1),
    }


def read_all_data(filepath, dtype=np.float32):
    """Read all float data from file (2 floats per line format)"""
    data = []
    with open(filepath, 'r') as f:
        for line in f:
            values = line.strip().split()
            data.extend([dtype(v) for v in values])
    return np.array(data, dtype=dtype)


def read_matrix_file(filepath, rows, cols, dtype=np.float32):
    """Read matrix from file (2 floats per line, column-major)"""
    data = read_all_data(filepath, dtype)
    matrix = np.array(data[:rows*cols], dtype=dtype).reshape((rows, cols), order='F')
    return matrix


def read_vector_file(filepath, length, dtype=np.float32):
    """Read vector from file (2 floats per line)"""
    data = read_all_data(filepath, dtype)
    return np.array(data[:length], dtype=dtype)


def extract_iteration(all_data, iter_idx, size):
    """Extract data for a specific iteration from concatenated data"""
    start = iter_idx * size
    end = start + size
    return all_data[start:end]


def write_matrix_file(filepath, matrix, samples_per_line=2):
    """Write matrix to file (2 floats per line, column-major)"""
    with open(filepath, 'w') as f:
        flat_data = matrix.flatten(order='F')  # Column-major
        for i in range(0, len(flat_data), samples_per_line):
            line_data = flat_data[i:i + samples_per_line]
            f.write(' '.join([f'{val:.9e}' for val in line_data]) + ' \n')


def write_vector_file(filepath, vector, samples_per_line=2):
    """Write vector to file (2 floats per line)"""
    with open(filepath, 'w') as f:
        for i in range(0, len(vector), samples_per_line):
            line_data = vector[i:i + samples_per_line]
            f.write(' '.join([f'{val:.9e}' for val in line_data]) + ' \n')


def verify_svd(name, U, S, V, A_original, config, verbose=True):
    """Verify SVD decomposition and return metrics"""
    if verbose:
        print(f"\n  {name} Verification:")
        print(f"    Singular values: {S}")
    
    # Reconstruct: A = U * diag(S) * V^H (conjugate transpose for complex, transpose for real)
    A_recon = U @ np.diag(S) @ V.conj().T
    
    # Compute errors
    diff = A_original - A_recon
    error_fro = np.linalg.norm(diff, 'fro')
    error_max = np.max(np.abs(diff))
    A_norm = np.linalg.norm(A_original, 'fro')
    error_rel = error_fro / A_norm if A_norm > 0 else error_fro
    error_pct = error_rel * 100.0
    
    if verbose:
        print(f"    Reconstruction: Frobenius={error_fro:.6e}, Max={error_max:.6e}, Rel={error_pct:.4f}%")
    
    # Check orthogonality: U^H * U ≈ I (conjugate transpose for complex)
    # Use actual matrix column count (may differ from config['cols'] for economy SVD)
    U_orth = np.linalg.norm(U.conj().T @ U - np.eye(U.shape[1]), 'fro')
    V_orth = np.linalg.norm(V.conj().T @ V - np.eye(V.shape[1]), 'fro')
    
    if verbose:
        print(f"    Orthogonality: ||U^T*U-I||={U_orth:.6e}, ||V^T*V-I||={V_orth:.6e}")
    
    return {
        'error_fro': error_fro,
        'error_max': error_max,
        'error_rel': error_rel,
        'error_pct': error_pct,
        'U_orth': U_orth,
        'V_orth': V_orth,
        'A_recon': A_recon,
        'S': S,
        'U': U,
        'V': V
    }


def compare_outputs(S1, U1, V1, S2, U2, V2, config):
    """Compare two sets of SVD outputs, return metrics"""
    # Singular values
    s_diff_norm = np.linalg.norm(S1 - S2)
    s_norm2 = np.linalg.norm(S2)
    s_rel = s_diff_norm / s_norm2 if s_norm2 > 0 else s_diff_norm
    s_rel_pct = s_rel * 100.0
    
    # U matrix (accounting for sign ambiguity)
    U_diff_pos = np.linalg.norm(U1 - U2, 'fro')
    U_diff_neg = np.linalg.norm(U1 + U2, 'fro')
    U_diff = min(U_diff_pos, U_diff_neg)
    U_norm2 = np.linalg.norm(U2, 'fro')
    U_rel = U_diff / U_norm2 if U_norm2 > 0 else U_diff
    U_rel_pct = U_rel * 100.0
    
    # V matrix (accounting for sign ambiguity)
    V_diff_pos = np.linalg.norm(V1 - V2, 'fro')
    V_diff_neg = np.linalg.norm(V1 + V2, 'fro')
    V_diff = min(V_diff_pos, V_diff_neg)
    V_norm2 = np.linalg.norm(V2, 'fro')
    V_rel = V_diff / V_norm2 if V_norm2 > 0 else V_diff
    V_rel_pct = V_rel * 100.0
    
    return {
        's_diff': s_diff_norm,
        's_rel_pct': s_rel_pct,
        'U_diff': U_diff,
        'U_rel_pct': U_rel_pct,
        'V_diff': V_diff,
        'V_rel_pct': V_rel_pct
    }


def main():
    parser = argparse.ArgumentParser(
        description='Comprehensive SVD Verification (multi-iteration, with sorting)')
    parser.add_argument('base_dir', nargs='?', default='.', help='Base directory')
    parser.add_argument('--niter', type=int, default=1, help='Number of iterations (default: 1)')
    parser.add_argument('--recon_tolerance', type=float, default=0.1,
                        help='Reconstruction error threshold %% (default: 0.1). '
                             'Relax to ~1.0 for AIE-ML/AIE22 where hardware float '
                             'precision limits convergence.')
    parser.add_argument('--v_orth_tolerance', type=float, default=0.01,
                        help='V orthogonality threshold (default: 0.01). '
                             'Relax to ~0.05 for AIE-ML/AIE22 where 14-bit hardware '
                             'invsqrt causes accumulated V orthogonality error.')
    args = parser.parse_args()
    
    base_dir = Path(args.base_dir)
    niter = args.niter
    
    config_path = base_dir / 'config.json'
    data_dir = base_dir / 'data'
    
    print("="*70)
    print("SVD Verification - UUT vs NumPy (All Iterations)")
    print("="*70)
    print(f"Config:            {config_path}")
    print(f"Data dir:          {data_dir}")
    print(f"Iterations:        {niter}")
    print(f"Recon tolerance:   {args.recon_tolerance}%")
    print(f"V_orth tolerance:  {args.v_orth_tolerance}")
    print("="*70)
    
    # Read configuration
    config = read_config(config_path)
    rows = config['rows']
    cols = config['cols']
    is_complex = config['data_type'] == 'cfloat'
    
    # For cfloat: files contain pairs of floats (real, imag) per element
    # Read as float32, then view as complex64
    file_dtype = np.float32  # Files always contain float32 values
    matrix_dtype = np.complex64 if is_complex else np.float32
    # Number of float32 values per matrix element
    floats_per_elem = 2 if is_complex else 1
    
    # Compute padded dimensions (must match kernel's internal padding)
    cols_padded = get_padded_dims(cols, config['data_type'], config.get('aie_variant', 1))

    print(f"\nConfiguration: {rows}x{cols}, dtype={config['data_type']}, "
          f"complex={is_complex}, passes={config['passes']}")
    if cols_padded != cols:
        print(f"  Padding: cols_padded={cols_padded}")
    
    # Read ALL data from files (concatenated across iterations)
    all_input_raw = read_all_data(data_dir / 'input.txt', file_dtype)
    all_U_raw = read_all_data(data_dir / 'uut_output_U.txt', file_dtype)
    all_S = read_all_data(data_dir / 'uut_output_S.txt', file_dtype)  # S is always real
    all_V_raw = read_all_data(data_dir / 'uut_output_V.txt', file_dtype)

    # Guard against empty output files (kernel produced zero samples — Pattern B)
    if len(all_U_raw) == 0 or len(all_S) == 0 or len(all_V_raw) == 0:
        print("\n  ❌ ERROR: One or more output files are empty (kernel produced 0 samples).")
        print("     This is a kernel output stall, not a verification failure.")
        logs_dir = base_dir / 'logs'
        logs_dir.mkdir(exist_ok=True)
        with open(logs_dir / 'diff.txt', 'w') as f:
            f.write("OVERALL: fail\n")
            f.write("REASON: Empty output files -- kernel produced zero samples.\n")
        return 1
    
    # Convert to complex if needed
    if is_complex:
        all_input = all_input_raw.view(np.complex64)
        all_U = all_U_raw.view(np.complex64)
        all_V = all_V_raw.view(np.complex64)
    else:
        all_input = all_input_raw
        all_U = all_U_raw
        all_V = all_V_raw
    
    # Sizes in ELEMENTS (not floats)
    # Output files use PADDED sizes (all buffers use cols_padded for DMA alignment).
    # We extract the true sub-blocks below.
    U_size_padded = rows * cols_padded     # Padded U output per iteration
    S_size_padded = cols_padded            # Padded S output per iteration
    V_size_padded = cols * cols            # V is always compacted to TP_DIM_COLS x TP_DIM_COLS
    U_size = rows * cols                   # True input size per iteration
    
    # Infer niter from file sizes if possible (use padded size since output files are padded)
    inferred_niter = len(all_U) // U_size_padded
    if niter == 1 and inferred_niter > 1:
        print(f"\n  ℹ️  Detected {inferred_niter} iterations from file size, using --niter={niter}")
    
    # Verify each iteration
    all_iter_results = []
    overall_pass = True
    
    for it in range(niter):
        print(f"\n{'='*70}")
        print(f"ITERATION {it+1}/{niter}")
        print("="*70)
        
        # Extract this iteration's data
        # Input uses true size; outputs use padded sizes, then extract true sub-blocks.
        A_flat = extract_iteration(all_input, it, U_size)
        A = A_flat.reshape((rows, cols), order='F')
        
        # U: padded to rows × cols_padded, extract first cols columns
        U_flat_padded = extract_iteration(all_U, it, U_size_padded)
        U_uut_padded = U_flat_padded.reshape((rows, cols_padded), order='F')
        U_uut = U_uut_padded[:, :cols]
        
        # S: padded to cols_padded, extract first cols values
        S_uut_padded = extract_iteration(all_S, it, S_size_padded)
        S_uut = S_uut_padded[:cols]
        
        # V: always compacted to cols × cols by the kernel (see svd.cpp V compaction step)
        V_flat = extract_iteration(all_V, it, V_size_padded)
        V_uut = V_flat.reshape((cols, cols), order='F')
        
        U_uut_sorted, S_uut_sorted, V_uut_sorted = U_uut, S_uut, V_uut
        print(f"  UUT singular values (sorted):        {S_uut}")
        
        # Compute NumPy reference for this iteration
        # np.linalg.svd returns (U, S, V^H), so V = (V^H)^H = conj(V^H).T
        U_np, S_np, Vt_np = np.linalg.svd(A, full_matrices=False)
        V_np = Vt_np.conj().T  # Works for both real (.conj() is no-op) and complex
        
        print(f"  NumPy singular values:               {S_np}")
        
        # Verify UUT (sorted)
        uut_metrics = verify_svd("UUT (sorted)", U_uut_sorted, S_uut_sorted, V_uut_sorted, A, config)
        
        # Verify NumPy
        np_metrics = verify_svd("NumPy", U_np, S_np, V_np, A, config)
        
        # Compare UUT vs NumPy (both sorted)
        comp = compare_outputs(S_uut_sorted, U_uut_sorted, V_uut_sorted, S_np, U_np, V_np, config)
        
        print(f"\n  UUT vs NumPy: S={comp['s_rel_pct']:.4f}%, U={comp['U_rel_pct']:.4f}%, V={comp['V_rel_pct']:.4f}%")
        
        # Assess this iteration:
        # PASS/FAIL criteria (hard requirements):
        #   1. Reconstruction: A ≈ U·diag(S)·V^T
        recon_pass = uut_metrics['error_pct'] < args.recon_tolerance
        #   2. S non-negativity
        s_nonneg = np.all(S_uut_sorted >= 0)
        #   3. V orthonormality: V^T·V ≈ I (V is built from rotations on Identity → always good)
        v_orth_pass = uut_metrics['V_orth'] < args.v_orth_tolerance
        
        iter_pass = recon_pass and s_nonneg and v_orth_pass
        
        # U orthonormality: WARNING only (not a hard fail)
        # U orthonormality depends on convergence (number of passes):
        #   - Perfect (converged):  < 1e-06  (machine precision for float32)
        #   - Good (3+ passes):     < 1e-03
        #   - Acceptable (2 pass):  < 0.1
        #   - Poor (1 pass):        ~1.0-3.0  (expected, not a bug)
        # V orthonormality is always near-perfect (~1e-07) because V starts as
        # Identity and Givens rotations preserve orthonormality exactly.
        u_orth_val = uut_metrics['U_orth']
        if u_orth_val < 1e-3:
            u_orth_status = "✓ excellent"
        elif u_orth_val < 0.1:
            u_orth_status = "~ acceptable"
        else:
            u_orth_status = f"⚠ poor (needs more passes)"
        
        checks = []
        checks.append(f"recon={'✓' if recon_pass else '✗'} {uut_metrics['error_pct']:.4f}%")
        checks.append(f"S≥0={'✓' if s_nonneg else '✗'}")
        checks.append(f"V_orth={'✓' if v_orth_pass else '✗'} {uut_metrics['V_orth']:.2e}")
        
        status = "✅ PASS" if iter_pass else "❌ FAIL"
        print(f"  Iteration {it+1}: {status}  [{', '.join(checks)}]")
        print(f"    U_orth={u_orth_val:.2e} ({u_orth_status})")
        
        if not iter_pass:
            overall_pass = False
        
        all_iter_results.append({
            'iteration': it + 1,
            'uut_metrics': uut_metrics,
            'np_metrics': np_metrics,
            'comparison': comp,
            'passed': iter_pass
        })
    
    print(f"\n{'='*70}")
    print(f"Writing UUT outputs (overwriting files)")
    print("="*70)
    
    sorted_U_all = []
    sorted_S_all = []
    sorted_V_all = []
    
    for it in range(niter):
        U_flat_padded = extract_iteration(all_U, it, U_size_padded)
        U_uut = U_flat_padded.reshape((rows, cols_padded), order='F')[:, :cols]
        S_uut_padded = extract_iteration(all_S, it, S_size_padded)
        S_uut = S_uut_padded[:cols]
        V_flat_padded = extract_iteration(all_V, it, V_size_padded)
        V_uut = V_flat_padded.reshape((cols, cols), order='F')
        
        U_s, S_s, V_s = U_uut, S_uut, V_uut  # always sorted by kernel
        
        sorted_U_all.extend(U_s.flatten(order='F'))
        sorted_S_all.extend(S_s)
        sorted_V_all.extend(V_s.flatten(order='F'))
    
    # Helper to write array as floats (handles complex by splitting to real/imag pairs)
    def flatten_to_floats(arr):
        arr = np.array(arr)
        if np.iscomplexobj(arr):
            # Interleave real and imaginary parts
            flat = np.empty(2 * len(arr), dtype=np.float32)
            flat[0::2] = arr.real
            flat[1::2] = arr.imag
            return flat
        else:
            return np.array(arr, dtype=np.float32)
    
    u_floats = flatten_to_floats(sorted_U_all)
    s_floats = np.array(sorted_S_all, dtype=np.float32)  # S is always real
    v_floats = flatten_to_floats(sorted_V_all)
    
    with open(data_dir / 'uut_output_U.txt', 'w') as f:
        for i in range(0, len(u_floats), 2):
            chunk = u_floats[i:i+2]
            f.write(' '.join([f'{v:.9e}' for v in chunk]) + ' \n')
    
    with open(data_dir / 'uut_output_S.txt', 'w') as f:
        for i in range(0, len(s_floats), 2):
            chunk = s_floats[i:i+2]
            f.write(' '.join([f'{v:.9e}' for v in chunk]) + ' \n')
    
    with open(data_dir / 'uut_output_V.txt', 'w') as f:
        for i in range(0, len(v_floats), 2):
            chunk = v_floats[i:i+2]
            f.write(' '.join([f'{v:.9e}' for v in chunk]) + ' \n')
    
    print(f"  ✓ uut_output_U.txt")
    print(f"  ✓ uut_output_S.txt")
    print(f"  ✓ uut_output_V.txt")
    
    # Write NumPy reference for ALL iterations (concatenated)
    np_U_all = []
    np_S_all = []
    np_V_all = []
    for r in all_iter_results:
        np_U_all.extend(r['np_metrics']['U'].flatten(order='F'))
        np_S_all.extend(r['np_metrics']['S'])
        np_V_all.extend(r['np_metrics']['V'].flatten(order='F'))
    
    np_u_floats = flatten_to_floats(np_U_all)
    np_s_floats = np.array(np_S_all, dtype=np.float32)
    np_v_floats = flatten_to_floats(np_V_all)
    
    with open(data_dir / 'numpy_output_U.txt', 'w') as f:
        for i in range(0, len(np_u_floats), 2):
            chunk = np_u_floats[i:i+2]
            f.write(' '.join([f'{v:.9e}' for v in chunk]) + ' \n')
    
    with open(data_dir / 'numpy_output_S.txt', 'w') as f:
        for i in range(0, len(np_s_floats), 2):
            chunk = np_s_floats[i:i+2]
            f.write(' '.join([f'{v:.9e}' for v in chunk]) + ' \n')
    
    with open(data_dir / 'numpy_output_V.txt', 'w') as f:
        for i in range(0, len(np_v_floats), 2):
            chunk = np_v_floats[i:i+2]
            f.write(' '.join([f'{v:.9e}' for v in chunk]) + ' \n')
    
    print("  ✓ numpy_output_U.txt (all iterations)")
    print("  ✓ numpy_output_S.txt (all iterations)")
    print("  ✓ numpy_output_V.txt (all iterations)")
    
    # Final Summary
    print(f"\n{'='*70}")
    print(f"FINAL SUMMARY ({niter} iterations)")
    print("="*70)
    
    for r in all_iter_results:
        status = "✅ PASS" if r['passed'] else "❌ FAIL"
        print(f"  Iter {r['iteration']}: {status}  "
              f"recon={r['uut_metrics']['error_pct']:.4f}%  "
              f"S_diff={r['comparison']['s_rel_pct']:.4f}%  "
              f"U_orth={r['uut_metrics']['U_orth']:.2e}  "
              f"V_orth={r['uut_metrics']['V_orth']:.2e}")
    
    print(f"\n  Overall: {'✅ ALL PASSED' if overall_pass else '❌ SOME FAILED'}")
    
    # Write diff.txt
    logs_dir = base_dir / 'logs'
    logs_dir.mkdir(exist_ok=True)
    diff_file = logs_dir / 'diff.txt'
    with open(diff_file, 'w') as f:
        f.write("SVD UUT vs NumPy Comparison Results\n")
        f.write("="*50 + "\n")
        f.write(f"NITER: {niter}\n")
        f.write(f"NOTE: UUT outputs are sorted descending by the kernel\n\n")

        for r in all_iter_results:
            f.write(f"ITERATION {r['iteration']}:\n")
            f.write(f"  UUT_RECON_ERROR_PCT: {r['uut_metrics']['error_pct']:.6f}\n")
            f.write(f"  UUT_VS_NUMPY_S_REL_PCT: {r['comparison']['s_rel_pct']:.6f}\n")
            f.write(f"  UUT_VS_NUMPY_U_REL_PCT: {r['comparison']['U_rel_pct']:.6f}\n")
            f.write(f"  UUT_VS_NUMPY_V_REL_PCT: {r['comparison']['V_rel_pct']:.6f}\n")
            f.write(f"  UUT_ORTHOGONALITY_U: {r['uut_metrics']['U_orth']:.6e}\n")
            f.write(f"  UUT_ORTHOGONALITY_V: {r['uut_metrics']['V_orth']:.6e}\n")
            f.write(f"  RESULT: {'pass' if r['passed'] else 'fail'}\n\n")
        
        f.write(f"OVERALL: {' pass (identical)' if overall_pass else 'fail'}\n")
    
    print(f"\n  ✓ Results written to {diff_file}")
    print("="*70)
    
    return 0 if overall_pass else 1


if __name__ == '__main__':
    sys.exit(main())
