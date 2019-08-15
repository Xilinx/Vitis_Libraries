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
namespace linear_algebra {
namespace blas {

vector<unsigned int> concurrentKernels;
vector<future<xfblasStatus_t>> fuStatus;

/**
 * @brief This asynchronous function copies a matrix in host memory to FPGA device memory. xfblasMalloc() need to be called prior to
 * this function.
 * @param rows number of rows in the matrix
 * @param cols number of cols in the matrix that is being used
 * @param elemSize number of bytes required to store each element in the matrix
 * @param A pointer to the matrix array in the host memory
 * @param lda leading dimension of the matrix that indicates the total number of cols in the matrix
 * @param d_A pointer to mapped memory
 * @param kernelIndex index of kernel that is being used, default is 0
 */
xfblasStatus_t xfblasSetMatrix(
    int rows, int cols, int elemSize, short* A, int lda, short* d_A, unsigned int kernelIndex = 0) {
  concurrentKernels.push_back( kernelIndex );
  fuStatus.push_back( async(launch::async, xfblasSetMatrix, rows, cols, elemSize, A, lda, d_A, kernelIndex ); 
  
}

void xfblasSetMatrix(
    int rows, int cols, int elemSize, float* A, int lda, float* d_A, unsigned int kernelIndex = 0) {
  concurrentKernels.push_back( kernelIndex );
  fuStatus.push_back( async(launch::async, xfblasSetMatrix, rows, cols, elemSize, A, lda, d_A, kernelIndex ); 
}


/**
 * @brief This asynchronous function copies a vector in host memory to FPGA device memory. xfblasMalloc() need to be called prior to
 * this function.
 * @param n number of elements in vector
 * @param elemSize number of bytes required to store each element in the vector
 * @param x pointer to the vector in the host memory
 * @param incx the storage spacing between consecutive elements of vector x
 * @param d_x pointer to mapped memory
 * @param kernelIndex index of kernel that is being used, default is 0
 */
void xfblasSetVectorAsync(
    int n, int elemSize, short* x, int incx, short* d_x, unsigned int kernelIndex = 0) {
  concurrentKernels.push_back( kernelIndex );
  fuStatus.push_back( async(launch::async, xfblasSetVector, n, elemSize, x, incx, d_x, kernelIndex );
}

void xfblasSetVectorAsync(
    int n, int elemSize, float* x, int incx, float* d_x, unsigned int kernelIndex = 0) {
  concurrentKernels.push_back( kernelIndex );
  fuStatus.push_back( async(launch::async, xfblasSetVector, n, elemSize, x, incx, d_x, kernelIndex );
  
}

/**
 * @brief This asynchronous function copies a matrix in host memory to FPGA device memory. xfblasMallocRestricted() need to be called
 * prior to this function.
 * @param A pointer to the matrix array in the host memory
 * @param kernelIndex index of kernel that is being used, default is 0
 */
void xfblasSetMatrixRestrictedAsync(void* A, unsigned int kernelIndex = 0) {
  concurrentKernels.push_back( kernelIndex );
  fuStatus.push_back( async(launch::async, xfblasSetMatrixRestricted, A, kernelIndex) );
}

/**
 * @brief This asynchronous function copies a vector in host memory to FPGA device memory. xfblasMallocRestricted() need to be called
 * prior to this function.
 * @param x pointer to the vector in the host memory
 * @param kernelIndex index of kernel that is being used, default is 0
 */
void xfblasSetVectorRestricted(void* x, unsigned int kernelIndex = 0) {
  concurrentKernels.push_back( kernelIndex );
  fuStatus.push_back( async(launch::async, xfblasSetVectorRestricted, x, kernelIndex) );
}

/**
 * @brief This asynchronous function copies a matrix in FPGA device memory to host memory
 * @param rows number of rows in the matrix
 * @param cols number of cols in the matrix that is being used
 * @param elemSize number of bytes required to store each element in the matrix
 * @param d_A pointer to mapped memory
 * @param A pointer to the matrix array in the host memory
 * @param lda leading dimension of the matrix that indicates the total number of cols in the matrix
 * @param kernelIndex index of kernel that is being used, default is 0
 */
void xfblasGetMatrixAsync(
    int rows, int cols, int elemSize, short* d_A, short* A, int lda, unsigned int kernelIndex = 0) {
  concurrentKernels.push_back( kernelIndex );
  fuStatus.push_back( async(launch::async, xfblasGetMatrix, rows, cols, elemSize, d_A, A, lda, kernelIndex ); 
}

void xfblasGetMatrixAsync(
    int rows, int cols, int elemSize, float* d_A, float* A, int lda, unsigned int kernelIndex = 0) {
  concurrentKernels.push_back( kernelIndex );
  fuStatus.push_back( async(launch::async, xfblasGetMatrix, rows, cols, elemSize, d_A, A, lda, kernelIndex ); 
}

/**
 * @brief This asynchronous function copies a vector in FPGA device memory to host memory
 * @param n number of elements in vector
 * @param elemSize number of bytes required to store each element in the vector
 * @param d_x pointer to mapped memory
 * @param x pointer to the vector in the host memory
 * @param incx the storage spacing between consecutive elements of vector x
 * @param kernelIndex index of kernel that is being used, default is 0
 */
void xfblasGetVectorAsync(
    int n, int elemSize, short* d_x, short* x, int incx, unsigned int kernelIndex = 0) {
  concurrentKernels.push_back( kernelIndex );
  fuStatus.push_back( async(launch::async, xfblasGetVector, n, elemSize, d_x, x, incx, kernelIndex );
}

void xfblasGetVectorAsync(
    int n, int elemSize, float* d_x, float* x, int incx, unsigned int kernelIndex = 0) {
  concurrentKernels.push_back( kernelIndex );
  fuStatus.push_back( async(launch::async, xfblasGetVector, n, elemSize, d_x, x, incx, kernelIndex );
}

/**
 * @brief This asynchronous function copies a matrix in FPGA device memory to host memory
 * @param A pointer to matrix A in the host memory
 * @param kernelIndex index of kernel that is being used, default is 0
 */
void xfblasGetMatrixRestrictedAsync(void* A, unsigned int kernelIndex = 0) {
  concurrentKernels.push_back( kernelIndex );
  fuStatus.push_back( async(launch::async, xfblasGetMatrixRestricted, A, kernelIndex) );
}

/**
 * @brief This function copies a matrix in FPGA device memory to host memory
 * @param x pointer to vetcor x in the host memory
 * @param kernelIndex index of kernel that is being used, default is 0
 * @retval xfblasStatus_t 0 if the operation completed successfully
 * @retval xfblasStatus_t 1 if the library was not initialized
 * @retval xfblasStatus_t 3 if there is no FPGA device memory allocated for the matrix
 */
void xfblasGetVectorRestrictedAsync(void* x, unsigned int kernelIndex = 0) {
  concurrentKernels.push_back( kernelIndex );
  fuStatus.push_back( async(launch::async, xfblasGetVectorRestricted, x, kernelIndex) );
}


void xfblasKernelSynchronize(){
  for(auto &fu : fuStatus) { fu.wait(); }
}

void xfblasKernelSynchronize( unsigned int kernelIndex ){
  for( int i=0; i<concurrentKernels.size(); i++ ){ if(concurrentKernels[i]==kernelIndex) fuStatus[i].wait(); }
}


} // namespace blas
} // namespace linear_algebra
} // namespace xf

#endif
