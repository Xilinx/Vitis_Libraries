/*
 * Copyright (C) 2025, Advanced Micro Devices, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "device_defs.h"
#include <array>
#include <cmath>

namespace xf {
namespace solver {
namespace aie {
namespace qrd {

// constexpr-compatible integer square root implementation
/**
 * This function calculates the largest integer less than or equal to the square root of the input value `n`
 * using the Newton-Raphson iterative method.
 */

constexpr unsigned int isqrt(unsigned int n) {
    unsigned int x = n;
    unsigned int y = (x + 1) / 2;
    while (y < x) {
        x = y;
        y = (x + n / x) / 2;
    }
    return x;
}

/**
 * @brief Splits the QRD column load among kernels in a cascaded architecture.
 *
 * This function calculates the number of columns to assign to a given kernel (identified by kernel_num)
 * such that the total QRD workload is distributed as evenly as possible, while also respecting memory constraints.
 */
template <typename TT_DATA, unsigned int TP_DIM_COLS, unsigned int TP_DIM_ROWS, unsigned int TP_CASC_LEN, unsigned int TP_NUM_FRAMES>
constexpr unsigned int qrd_load_split(
    unsigned int col_dim_distributed, unsigned int kernel_num) {
            
    // Calculate the maximum number of columns per kernel based on memory constraints for R and Q matrices
    unsigned int max_col_per_kernel_r = (__DATA_MEM_BYTES__)/(TP_DIM_COLS * sizeof(TT_DATA)* TP_NUM_FRAMES);
    unsigned int max_col_per_kernel_q = (__DATA_MEM_BYTES__)/(TP_DIM_ROWS * sizeof(TT_DATA)* TP_NUM_FRAMES);
    unsigned int max_col_per_kernel = (max_col_per_kernel_r > max_col_per_kernel_q) ? max_col_per_kernel_q : max_col_per_kernel_r;

    // Calculate the total QRD workload (number of column pairs)
    unsigned int total_load = TP_DIM_COLS * (TP_DIM_COLS -1) / 2;
    unsigned int load_distributed = col_dim_distributed * (col_dim_distributed - 1) / 2;
    unsigned int load_remained = total_load - load_distributed;
    unsigned int load_per_core = load_remained / (TP_CASC_LEN - kernel_num);
    unsigned int col_dim_kernel; 
    unsigned int a = 1;
    unsigned int b = -1;
    unsigned int c = -(2 * load_per_core + col_dim_distributed * (col_dim_distributed - 1));
    if (kernel_num == TP_CASC_LEN - 1) {
        // Last kernel takes all remaining columns
        col_dim_kernel = TP_DIM_COLS - col_dim_distributed;
    } else {
        // Solve quadratic equation to determine columns for this kernel
        unsigned int discriminant = b * b - 4 * a * c;
        col_dim_kernel = (-b + isqrt(discriminant)) / (2 * a);
        col_dim_kernel = col_dim_kernel - col_dim_distributed;
    }

    // Enforce memory constraint
    if (max_col_per_kernel < col_dim_kernel){
        col_dim_kernel = max_col_per_kernel;
    }

    return col_dim_kernel;
}

/**
 *
 * This function returns two arrays:
 *   - The number of columns assigned to each kernel.
 *   - The cumulative number of columns distributed up to each kernel.
 *
 */
template <typename TT_DATA, unsigned int TP_DIM_COLS, unsigned int TP_DIM_ROWS, unsigned int TP_CASC_LEN, unsigned int TP_NUM_FRAMES>
constexpr std::pair<std::array<unsigned int, TP_CASC_LEN>, std::array<unsigned int, TP_CASC_LEN>> get_qrd_loads_of_kernels() {
    std::array<unsigned int, TP_CASC_LEN> col_dim_kernel_arr = {};
    std::array<unsigned int, TP_CASC_LEN> col_dim_dist_kernel_arr = {};
    constexpr unsigned int total_load = TP_DIM_COLS * (TP_DIM_COLS - 1) / 2;
    unsigned int col_dim_kernel_load = 0, col_dim_kernel_dist = 0;
    for (int i = 0; i < TP_CASC_LEN; i++) {
        // Calculate columns assigned to this kernel
        col_dim_kernel_load = qrd_load_split<TT_DATA, TP_DIM_COLS, TP_DIM_ROWS, TP_CASC_LEN, TP_NUM_FRAMES>(col_dim_kernel_dist, i);
        // Update cumulative distributed columns
        col_dim_kernel_dist = col_dim_kernel_dist + col_dim_kernel_load;
        col_dim_kernel_arr[i] = col_dim_kernel_load;
        col_dim_dist_kernel_arr[i] = col_dim_kernel_dist;
    }
    return {col_dim_kernel_arr, col_dim_dist_kernel_arr};

}



}
} // namespace aie
} // namespace solver
} // namespace xf