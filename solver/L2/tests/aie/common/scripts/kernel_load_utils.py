#
# Copyright (C) 2025-2026, Advanced Micro Devices, Inc.
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
import gen_matrix_utils as gen_utils

# A kernel in the middle of a cascade chain
# effectively reaches only 3 memory modules, since it shares the modules along the chain with its
# neighbours.
# Estimated sysmem per kernel:
#   stack          : 1024 B                     (default kernel stack size, seen in the kernel .map file)
#   kernel object  : dim_rows * sizeof(TT_DATA) (QrdCascData[TP_DIM_ROWS] member in qrd.hpp)
#   runtime data   : 128 B                      (sync buffer and atexit data, ~70 B in the .map file, rounded up)

QRD_SYSMEM_STACK_BYTES = 1024
QRD_SYSMEM_RUNTIME_BYTES = 128


def qrd_sysmem_bytes(data_type, dim_rows):
    """Estimated system memory (stack + kernel object + runtime data) of one QRD kernel, in bytes."""
    return QRD_SYSMEM_STACK_BYTES + dim_rows * gen_utils.sizeof(data_type) + QRD_SYSMEM_RUNTIME_BYTES


def qrd_max_r_buffer_bytes(aie_variant, data_type, dim_rows, casc_len):
    """
    Largest R buffer allowed per QRD kernel, in bytes. A and Q may each fill a whole memory module.

    The first and last kernel of a cascade chain (and a single kernel) reach 4 memory modules, so R may
    fill a module too and sysmem goes to the 4th. A kernel in the middle of the chain (casc_len > 2)
    effectively reaches only 3 modules, so one of its buffers has to leave room for the kernel's sysmem
    in its module. R is used for that, as it is the smallest of the three buffers (dim_cols <= dim_rows).
    """
    if casc_len > 2:
        return gen_utils.k_data_memory_bytes[aie_variant] - qrd_sysmem_bytes(data_type, dim_rows)
    return gen_utils.k_data_memory_bytes[aie_variant]


def isqrt(n):
    """
    Compute the integer square root of a non-negative integer n using the Newton-Raphson method.

    Args:
        n (int): Non-negative integer whose integer square root is to be computed.

    Returns:
        int: The greatest integer less than or equal to the square root of n.

    Note:
        This function does not handle negative inputs.
    """
    x = n
    y = (x + 1) // 2
    while y < x:
        x = y
        y = (x + n // x) // 2
    return x


def qrd_load_split(aie_variant, data_type, dim_rows, dim_cols, casc_len, num_frames):
    """
    Splits the computational load for QR decomposition across multiple kernels in a cascade.

    This function distributes the total number of column pairs (representing the computational load)
    among a specified number of kernels (casc_len) such that each kernel receives a balanced portion
    of the workload. The distribution is calculated to minimize load imbalance, especially for the
    last kernel in the cascade.

    Args:
        aie_variant (str): 
            The AIE hardware variant used, which determines memory and compute characteristics.
        data_type (str): 
            The data type of the matrix elements (e.g., 'float', 'int16').
        dim_rows (int): 
            The number of rows in the matrix to be processed.
        dim_cols (int): 
            The number of columns in the matrix to be processed. Determines the total computational load.
        casc_len (int): 
            The number of kernels (or processing elements) in the cascade over which the load is to be split.

    Returns:
        list of int: 
            A list where each element represents the number of columns assigned to each kernel in the cascade,
            in order.

    Note:
        The columns per kernel are capped so that A and Q each fit in a memory module and R fits in
        qrd_max_r_buffer_bytes(), which leaves room for the kernel's system memory when casc_len > 2.
        If the cap is hit, the columns may not add up to dim_cols; callers must check sum() of the
        returned list.
    """
    col_per_kernel_max_q = (gen_utils.k_data_memory_bytes[aie_variant]) // (dim_rows * gen_utils.sizeof(data_type) * num_frames)
    col_per_kernel_max_r = qrd_max_r_buffer_bytes(aie_variant, data_type, dim_rows, casc_len) // (
        dim_cols * gen_utils.sizeof(data_type) * num_frames
    )
    col_per_kernel_max = min(col_per_kernel_max_q, col_per_kernel_max_r)
    
    col_dim_kernel_list = []
    col_dim_distributed = 0
    for kernel in range(casc_len):
        total_load = (dim_cols * (dim_cols -1) // 2)
        load_distributed = (col_dim_distributed * (col_dim_distributed - 1) // 2)
        load_remained = (total_load - load_distributed)
        load_per_core = (load_remained // (casc_len - kernel))
        a = 1
        b = -1
        c = -(2 * load_per_core + col_dim_distributed * (col_dim_distributed - 1))

        if (kernel == casc_len - 1):
            col_dim_kernel = dim_cols - col_dim_distributed 
        else:
            col_dim_kernel = int((-b + isqrt(b * b - 4 * a * c)) // (2 * a))            
            col_dim_kernel = col_dim_kernel - col_dim_distributed
        
        if col_dim_kernel > col_per_kernel_max:
            col_dim_kernel = col_per_kernel_max

        col_dim_distributed += col_dim_kernel
        col_dim_kernel_list.append(col_dim_kernel)
    return col_dim_kernel_list
