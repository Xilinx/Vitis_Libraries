/*
 * Copyright 2019-2021 Xilinx, Inc.
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

#include "gesvdj.hpp"

void xgesvdj::compute(
    double dataA[NA * NA], double sigma[NA * NA], double dataU[NA * NA], double dataV[NA * NA], int matrixSize) {
    gesvdj(dataA, sigma, dataU, dataV, matrixSize);
}

void xgesvdj::gesvdj(
    double dataA[NA * NA], double sigma[NA * NA], double dataU[NA * NA], double dataV[NA * NA], int matrixSize) {
    gesvdj_wrapper(dataA, sigma, dataU, dataV, matrixSize);
}

#if defined(__SYNTHESIS__) || defined(TARGET_FLOW_sw_emu)
#include "xf_solver_L2.hpp"
#endif

void gesvdj_wrapper(double* dataA, double* sigma, double* dataU, double* dataV, int matrixSize) {
#if defined(__SYNTHESIS__) || defined(TARGET_FLOW_sw_emu)
    int info;
    xf::solver::gesvdj<double, NA, 4>(matrixSize, dataA, matrixSize, sigma, dataU, matrixSize, dataV, matrixSize, info);
#endif
}
