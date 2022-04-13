/*
 * Copyright 2021 Xilinx, Inc.
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

#include "kernel_svd.hpp"
#include "xf_solver_L1.hpp"

struct my_svd_config : xf::solver::svdTraits<ROWS, COLS, MATRIX_IN_T, MATRIX_OUT_T> {
    // static const int NUM_SWEEPS = 10;
    static const int ARCH = SEL_ARCH;
};

extern "C" int kernel_svd_0(hls::stream<MATRIX_IN_T>& matrixAStream,
                            hls::stream<MATRIX_OUT_T>& matrixSStream,
                            hls::stream<MATRIX_OUT_T>& matrixUStream,
                            hls::stream<MATRIX_OUT_T>& matrixVStream) {
    xf::solver::svd<ROWS, COLS, MATRIX_IN_T, MATRIX_OUT_T, my_svd_config>(matrixAStream, matrixSStream, matrixUStream,
                                                                          matrixVStream);
    return 0;
}
