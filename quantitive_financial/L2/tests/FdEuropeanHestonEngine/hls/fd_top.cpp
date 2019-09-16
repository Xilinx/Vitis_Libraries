/*
 * Copyright 2019 Xilinx, Inc.
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
/**
 *  @brief Kernel wrapper header
 *
 *  $DateTime: 2019/04/09 12:00:00 $
 */
#include "fd_top.hpp"
#include "xf_fintech/fd_solver.hpp"

void fd_top(double A[a_size],
            unsigned int Ar[a_size],
            unsigned int Ac[a_size],
            unsigned int nnz,
            double A1[m_size][a1_dim],
            double A2[m_size][a2_dim],
            double X1[m_size][a1_dim],
            double X2[m_size][a2_dim],
            double b[m_size],
            double u0[m_size],
            unsigned int M1,
            unsigned int M2,
            unsigned int N,
            double price[m_size]) {
    xf::fintech::FdDouglas<double, 8, 16, a_size, m_size, log2_m_size, a1_dim, a2_dim>(A, Ar, Ac, nnz, A1, A2, X1, X2,
                                                                                       b, u0, M1, M2, N, price);

    return;
}
