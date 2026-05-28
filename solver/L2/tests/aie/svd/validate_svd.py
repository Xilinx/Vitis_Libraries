#!/usr/bin/env python3
"""
SVD Multi-Kernel Output Validation Script

This script:
1. Reads the original input matrix
2. Reconstructs U, S, V from multiple kernel outputs
3. Computes reference SVD using NumPy
4. Compares UUT output with reference
"""

import numpy as np
import argparse
import sys

def read_float_file(filename):
    """Read float values from text file"""
    values = []
    with open(filename, 'r') as f:
        for line in f:
            line = line.strip()
            if line and not line.startswith('T') and not line.startswith('X'):
                # Handle format with multiple values per line (e.g., "value1 value2")
                parts = line.split()
                for part in parts:
                    try:
                        values.append(float(part))
                    except ValueError:
                        pass
    return np.array(values)

def reconstruct_matrix(file_pattern, num_kernels, rows_per_kernel, cols, is_v_matrix=False):
    """
    Reconstruct matrix from multiple kernel outputs
    
    For U matrix: Each kernel outputs its row partition (rows_per_kernel × cols)
    For V matrix: Each kernel outputs its row partition (v_rows_per_kernel × cols)
                  where v_rows_per_kernel = cols / num_kernels
    For S matrix: Each kernel outputs the same singular values (cols values)
    """
    if is_v_matrix:
        # V matrix: each kernel outputs its row partition in column-major format
        # Need to reconstruct by stacking row partitions vertically
        v_rows_per_kernel = cols // num_kernels
        
        # Read each kernel's partition and reshape it
        partitions = []
        for k in range(num_kernels):
            filename = file_pattern.format(k)
            values = read_float_file(filename)
            # Each partition is v_rows_per_kernel × cols in column-major format
            partition = np.array(values).reshape(v_rows_per_kernel, cols, order='F').T  # Transpose to get cols × v_rows
            partitions.append(partition)
        
        # Stack partitions horizontally (along columns, which are rows of V^T)
        V_T = np.hstack(partitions)  # This gives us cols × cols
        return V_T.T  # Transpose back to get V
    else:
        # U matrix: concatenate row partitions from all kernels
        all_values = []
        for k in range(num_kernels):
            filename = file_pattern.format(k)
            values = read_float_file(filename)
            all_values.extend(values)
        
        total_rows = rows_per_kernel * num_kernels
        return np.array(all_values).reshape(total_rows, cols, order='F')  # Column-major

def read_input_matrix(filename, rows, cols):
    """Read input matrix from file"""
    values = read_float_file(filename)
    # Input is column-major
    return values[:rows*cols].reshape(rows, cols, order='F')

def compute_reference_svd(A):
    """Compute reference SVD using NumPy"""
    U, S, Vt = np.linalg.svd(A, full_matrices=False)
    V = Vt.T
    return U, S, V

def compare_matrices(mat1, mat2, name, tolerance=1e-3):
    """Compare two matrices with tolerance"""
    # Handle sign ambiguity in SVD
    # Columns of U and V can be multiplied by -1
    mat1_normalized = mat1.copy()
    mat2_normalized = mat2.copy()
    
    for col in range(mat1.shape[1]):
        # Check if flipping sign gives better match
        diff_normal = np.linalg.norm(mat1[:, col] - mat2[:, col])
        diff_flipped = np.linalg.norm(mat1[:, col] + mat2[:, col])
        
        if diff_flipped < diff_normal:
            mat2_normalized[:, col] = -mat2[:, col]
    
    diff = np.abs(mat1_normalized - mat2_normalized)
    max_diff = np.max(diff)
    mean_diff = np.mean(diff)
    
    print(f"\n{name} Comparison:")
    print(f"  Shape: {mat1.shape}")
    print(f"  Max difference: {max_diff:.6e}")
    print(f"  Mean difference: {mean_diff:.6e}")
    print(f"  Tolerance: {tolerance:.6e}")
    
    if max_diff > tolerance:
        print(f"  ❌ FAILED - Exceeds tolerance")
        print(f"\n  UUT {name}:")
        print(mat1)
        print(f"\n  Reference {name}:")
        print(mat2_normalized)
        return False
    else:
        print(f"  ✅ PASSED")
        return True

def main():
    parser = argparse.ArgumentParser(description='Validate SVD multi-kernel output')
    parser.add_argument('--input', required=True, help='Input matrix file')
    parser.add_argument('--u_pattern', default='./data/uut_output_U_0_{}.txt', 
                       help='U output file pattern (use {} for kernel index)')
    parser.add_argument('--s_pattern', default='./data/uut_output_S_0_{}.txt',
                       help='S output file pattern')
    parser.add_argument('--v_pattern', default='./data/uut_output_V_0_{}.txt',
                       help='V output file pattern')
    parser.add_argument('--rows', type=int, required=True, help='Number of rows')
    parser.add_argument('--cols', type=int, required=True, help='Number of columns')
    parser.add_argument('--casc_len', type=int, required=True, help='Number of kernels')
    parser.add_argument('--tolerance', type=float, default=1e-2, help='Comparison tolerance')
    
    args = parser.parse_args()
    
    rows_per_kernel = args.rows // args.casc_len
    
    print("="*60)
    print("SVD Multi-Kernel Validation")
    print("="*60)
    print(f"Input matrix: {args.rows} × {args.cols}")
    print(f"Cascade length: {args.casc_len}")
    print(f"Rows per kernel: {rows_per_kernel}")
    print(f"Tolerance: {args.tolerance}")
    
    # Read input matrix
    print(f"\nReading input from: {args.input}")
    A_input = read_input_matrix(args.input, args.rows, args.cols)
    print(f"Input matrix shape: {A_input.shape}")
    print(f"Input matrix:\n{A_input}")
    
    # Reconstruct UUT outputs
    print(f"\nReconstructing UUT outputs...")
    U_uut = reconstruct_matrix(args.u_pattern, args.casc_len, rows_per_kernel, args.cols, is_v_matrix=False)
    
    # For S, just read from first kernel (all should be the same)
    S_uut = read_float_file(args.s_pattern.format(0))[:args.cols]
    
    V_uut = reconstruct_matrix(args.v_pattern, args.casc_len, rows_per_kernel, args.cols, is_v_matrix=True)
    
    # Kernels now sort outputs by descending singular value
    
    print(f"UUT U shape: {U_uut.shape}")
    print(f"UUT S shape: {S_uut.shape}")
    print(f"UUT V shape: {V_uut.shape}")
    
    print(f"\nUUT U matrix:\n{U_uut}")
    print(f"\nUUT S values: {S_uut}")
    print(f"\nUUT V matrix:\n{V_uut}")
    
    # Compute reference SVD
    print(f"\nComputing reference SVD...")
    U_ref, S_ref, V_ref = compute_reference_svd(A_input)
    
    print(f"Reference U shape: {U_ref.shape}")
    print(f"Reference S shape: {S_ref.shape}")
    print(f"Reference V shape: {V_ref.shape}")
    
    print(f"\nReference U matrix:\n{U_ref}")
    print(f"\nReference S values: {S_ref}")
    print(f"\nReference V matrix:\n{V_ref}")
    
    # Compare results
    print("\n" + "="*60)
    print("VALIDATION RESULTS")
    print("="*60)
    
    u_pass = compare_matrices(U_uut, U_ref, "U Matrix", args.tolerance)
    
    # Compare singular values
    s_diff = np.abs(S_uut - S_ref)
    s_max_diff = np.max(s_diff)
    s_mean_diff = np.mean(s_diff)
    
    print(f"\nSingular Values Comparison:")
    print(f"  Max difference: {s_max_diff:.6e}")
    print(f"  Mean difference: {s_mean_diff:.6e}")
    print(f"  Tolerance: {args.tolerance:.6e}")
    
    if s_max_diff > args.tolerance:
        print(f"  ❌ FAILED - Exceeds tolerance")
        s_pass = False
    else:
        print(f"  ✅ PASSED")
        s_pass = True
    
    v_pass = compare_matrices(V_uut, V_ref, "V Matrix", args.tolerance)
    
    # Verify SVD property: A = U * S * V^T
    print(f"\nVerifying SVD property: A = U * S * V^T")
    S_diag = np.diag(S_uut)
    A_reconstructed = U_uut @ S_diag @ V_uut.T
    reconstruction_diff = np.abs(A_input - A_reconstructed)
    reconstruction_max_diff = np.max(reconstruction_diff)
    
    print(f"  Max reconstruction error: {reconstruction_max_diff:.6e}")
    print(f"  Tolerance: {args.tolerance:.6e}")
    
    if reconstruction_max_diff > args.tolerance:
        print(f"  ❌ FAILED - Reconstruction error too large")
        print(f"\n  Original A:\n{A_input}")
        print(f"\n  Reconstructed A:\n{A_reconstructed}")
        reconstruction_pass = False
    else:
        print(f"  ✅ PASSED")
        reconstruction_pass = True
    
    # Overall result
    print("\n" + "="*60)
    if u_pass and s_pass and v_pass and reconstruction_pass:
        print("✅ OVERALL: PASSED")
        print("="*60)
        return 0
    else:
        print("❌ OVERALL: FAILED")
        print("="*60)
        return 1

if __name__ == "__main__":
    sys.exit(main())
