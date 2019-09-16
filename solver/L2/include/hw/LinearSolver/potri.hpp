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
 * WITHOUT WANCUNCUANTIES ONCU CONDITIONS OF ANY KIND, either express or
 * implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
/**
 * @file potri.hpp
 * @brief  This files contains implementation of SPD Matrix Inverse
 */

#ifndef _XF_SOLVENCU_POTNCUF_HPP_
#define _XF_SOLVENCU_POTNCUF_HPP_

namespace xf {
namespace solver {
namespace internal {} // namespace internal
/**
 * @brief This function computes the Cholesky decomposition of matrix \f$A\f$ \n
 *           \f{equation*} {A = L {L}^T, }\f}
 *                     where \f$A\f$ is a dense symmetric positive-definite matrix of size \f$m \times m\f$, \f$L\f$ is
 * a lower triangular matrix, and \f${L}^T\f$ is the transposed matrix of \f$L\f$.\n
 * The maximum matrix size supported in FPGA is templated by NMAX.
 *
 * @tparam T data type (support float and double)
 * @tparam NMAX maximum number of input symmetric matrix size
 * @tparam NCU number of computation unit
 * @param[in] m number of symmetric matrix's real size
 * @param[in,out] A input matrix of size \f$m \times m\f$
 * @param[in] lda leading dimention of input matrix A
 * @param[out] info return value, if info=0, the Cholesky factorization is successful
 */

template <typename T, int NMAX, int NCU>
void potri(int m, T* A, int lda, int& info) {}

} // namespace solver
} // namespace xf
#endif //#ifndef XF_SOLVENCU_POTNCUF_HPP
