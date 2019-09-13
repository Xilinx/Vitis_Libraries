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
 * @file getrf.hpp
 * @brief This file contains LU decomposition (with partial pivoting) for dense matrix.
 *
 * This file is part of XF Solver Library.
 */

#ifndef _XF_SOLVER_GETRF_
#define _XF_SOLVER_GETRF_

#include "getrf_nopivot.hpp"

namespace xf {
namespace solver {

/**
 * @brief This function computes the LU decomposition (with partial pivoting) of matrix \f$A\f$ \n
          \f{equation*} {A = L U, }\f}
          where \f$A\f$ is a dense matrix of size \f$m \times n\f$, \f$L\f$ is a lower triangular matrix with unit
 diagonal, and \f$U\f$ is a upper triangular matrix. This function implement partial pivoting.\n
   The maximum matrix size supported in FPGA is templated by NRMAX and NCMAX.
 *
 * @tparam T data type (support float and double)
 * @tparam NRMAX maximum number of rows for input matrix
 * @tparam NCMAX maximum number of columns for input matrix
 * @tparam NCU number of computation unit
 * @param[in] m real row number of input matrix
 * @param[in] n real column number of input matrix
 * @param[in,out] A input matrix
 * @param[in] lda leading dimention of input matrix
 * @param[out] info return value, if info=0, the LU factorization is successful
 */
template <class T, int NRMAX, int NCMAX, int NCU>
void getrf(int m, int n, T* A, int lda, int& info) {
    xf::solver::getrf_nopivot<T, NRMAX, NCMAX, NCU>(m, n, A, lda, info);
};
}
}

#endif
