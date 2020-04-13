/*
 * Copyright 2019 Xilinx, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef XF_BLAS_WRAPPER_ASYNC_HPP
#define XF_BLAS_WRAPPER_ASYNC_HPP

#include "handle.hpp"
#include "gemm_host.hpp"
#include "gemv_host.hpp"
#include <future>

namespace xf {

namespace blas {

vector<future<xfblasStatus_t> > fuStatus;

/**
 * @brief This asynchronous function copies a matrix in FPGA device memory to host memory
 * @param rows number of rows in the matrix
 * @param cols number of cols in the matrix that is being used
 * @param elemSize number of bytes required to store each element in the matrix
 * @param d_A pointer to mapped memory
 * @param A pointer to the matrix array in the host memory
 * @param lda leading dimension of the matrix that indicates the total number of cols in the matrix
 * @param kernelIndex index of kernel that is being used, default is 0
 * @param deviceIndex index of device that is being used, default is 0
 */
void xfblasGetMatrixAsync(int rows,
                          int cols,
                          int elemSize,
                          short* d_A,
                          short* A,
                          int lda,
                          unsigned int kernelIndex = 0,
                          unsigned int deviceIndex = 0) {
    fuStatus.push_back(async(
        launch::async, [&] { return xfblasGetMatrix(rows, cols, elemSize, d_A, A, lda, kernelIndex, deviceIndex); }));
}

void xfblasGetMatrixAsync(int rows,
                          int cols,
                          int elemSize,
                          float* d_A,
                          float* A,
                          int lda,
                          unsigned int kernelIndex = 0,
                          unsigned int deviceIndex = 0) {
    fuStatus.push_back(async(
        launch::async, [&] { return xfblasGetMatrix(rows, cols, elemSize, d_A, A, lda, kernelIndex, deviceIndex); }));
}

/**
 * @brief This asynchronous function copies a vector in FPGA device memory to host memory
 * @param n number of elements in vector
 * @param elemSize number of bytes required to store each element in the vector
 * @param d_x pointer to mapped memory
 * @param x pointer to the vector in the host memory
 * @param incx the storage spacing between consecutive elements of vector x
 * @param kernelIndex index of kernel that is being used, default is 0
 * @param deviceIndex index of device that is being used, default is 0
 */
void xfblasGetVectorAsync(
    int n, int elemSize, short* d_x, short* x, int incx, unsigned int kernelIndex = 0, unsigned int deviceIndex = 0) {
    fuStatus.push_back(
        async(launch::async, [&] { return xfblasGetVector(n, elemSize, d_x, x, incx, kernelIndex, deviceIndex); }));
}

void xfblasGetVectorAsync(
    int n, int elemSize, float* d_x, float* x, int incx, unsigned int kernelIndex = 0, unsigned int deviceIndex = 0) {
    fuStatus.push_back(
        async(launch::async, [&] { return xfblasGetVector(n, elemSize, d_x, x, incx, kernelIndex, deviceIndex); }));
}

/**
 * @brief This asynchronous function copies a matrix in FPGA device memory to host memory
 * @param A pointer to matrix A in the host memory
 * @param kernelIndex index of kernel that is being used, default is 0
 * @param deviceIndex index of device that is being used, default is 0
 */
void xfblasGetMatrixRestrictedAsync(void* A, unsigned int kernelIndex = 0, unsigned int deviceIndex = 0) {
    fuStatus.push_back(async(launch::async, xfblasGetMatrixRestricted, A, kernelIndex, deviceIndex));
}

/**
 * @brief This asynchronous function copies a matrix in FPGA device memory to host memory
 * @param x pointer to vetcor x in the host memory
 * @param kernelIndex index of kernel that is being used, default is 0
 * @param deviceIndex index of device that is being used, default is 0
 */
void xfblasGetVectorRestrictedAsync(void* x, unsigned int kernelIndex = 0, unsigned int deviceIndex = 0) {
    fuStatus.push_back(async(launch::async, xfblasGetVectorRestricted, x, kernelIndex, deviceIndex));
}

void xfblasKernelSynchronize() {
    for (auto& fu : fuStatus) {
        fu.wait();
    }
    fuStatus.clear();
}

} // namespace blas

} // namespace xf

#endif
