import sys
import numpy as np
import os
import argparse

def main():
    parser = argparse.ArgumentParser(description='Generate input files for substitution test.')
    parser.add_argument('--data_type', type=str, required=True)
    parser.add_argument('--dim_size', type=int, required=True)
    parser.add_argument('--subst_type', type=int, required=True)
    parser.add_argument('--l_leading', type=int, required=True)
    parser.add_argument('--grid_dim', type=int, required=True)
    parser.add_argument('--seed', type=int, required=True)
    parser.add_argument('--loc_in_L', type=str, required=True)
    parser.add_argument('--loc_in_y', type=str, required=True)
    parser.add_argument('--loc_ref_x', type=str, required=True)
    parser.add_argument('--niter', type=int, required=True)
    args = parser.parse_args()

    data_type_str = args.data_type
    dim_size = args.dim_size
    subst_type = args.subst_type
    l_leading = args.l_leading
    grid_dim = args.grid_dim
    seed = args.seed
    loc_in_L = args.loc_in_L
    loc_in_y = args.loc_in_y
    loc_ref_x = args.loc_ref_x
    niter = args.niter

    np.random.seed(seed)

    os.makedirs(os.path.dirname(loc_in_L), exist_ok=True)
    os.makedirs(os.path.dirname(loc_in_y), exist_ok=True)
    os.makedirs(os.path.dirname(loc_ref_x), exist_ok=True)

    if data_type_str == 'float':
        data_type = np.float32
    elif data_type_str == 'cfloat':
        data_type = np.complex64
    else:
        print(f"Unsupported data type: {data_type_str}")
        return

    for i in range(niter):
        L = (np.random.rand(dim_size, dim_size) + 1).astype(np.float32)
        L = np.tril(L)
#        if subst_type == 0: # FWD
#            L = np.tril(L)
#        else: # BWD
#            L = np.triu(L)

        y = (np.random.rand(dim_size) + 1).astype(np.float32)

        if data_type == np.complex64:
            L_imag = np.random.rand(dim_size, dim_size).astype(np.float32)
            L_imag = np.tril(L_imag)
#            if subst_type == 0: # FWD
#                L_imag = np.tril(L_imag)
#            else: # BWD
#                L_imag = np.triu(L_imag)
            np.fill_diagonal(L_imag, 0)
            L = L + 1j * L_imag
            y = y + 1j * np.random.rand(dim_size).astype(np.float32)

        x = np.linalg.solve(L, y)

        if l_leading:
            L = L.T

        kernel_dim = dim_size // grid_dim
        for i in range(grid_dim):
            # --- L matrix ---
            L_flat = L[i*kernel_dim:(i+1)*kernel_dim, i*kernel_dim:(i+1)*kernel_dim].flatten()
            with open(loc_in_L, "a") as f:
                if data_type == np.complex64:
                    for val in L_flat:
                        f.write(f"{val.real} {val.imag}" + os.linesep)
                else:  # np.float32
                    if len(L_flat) % 2 != 0:
                        L_flat = np.append(L_flat, 0.0)
                    for j in range(0, len(L_flat), 2):
                        f.write(f"{L_flat[j]} {L_flat[j+1]}" + os.linesep)

            # --- y vector ---
            y_flat = y[i*kernel_dim:(i+1)*kernel_dim].flatten()
            with open(loc_in_y, "a") as f:
                if data_type == np.complex64:
                    for val in y_flat:
                        f.write(f"{val.real} {val.imag}" + os.linesep)
                else:  # np.float32
                    if len(y_flat) % 2 != 0:
                        y_flat = np.append(y_flat, 0.0)
                    for j in range(0, len(y_flat), 2):
                        f.write(f"{y_flat[j]} {y_flat[j+1]}" + os.linesep)

            # --- x vector (reference) ---
            x_flat = x[i*kernel_dim:(i+1)*kernel_dim].flatten()
            with open(loc_ref_x, "a") as f:
                if data_type == np.complex64:
                    for val in x_flat:
                        f.write(f"{val.real} {val.imag}" + os.linesep)
                else:  # np.float32
                    if len(x_flat) % 2 != 0:
                        x_flat = np.append(x_flat, 0.0)
                    for j in range(0, len(x_flat), 2):
                        f.write(f"{x_flat[j]} {x_flat[j+1]}" + os.linesep)

if __name__ == "__main__":
    main()
