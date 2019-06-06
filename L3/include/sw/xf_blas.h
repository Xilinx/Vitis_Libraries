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

#ifndef XF_BLAS_H
#define XF_BLAS_H

/**
 * @file xf_blas.h
 * @brief Top-level header for XF BLAS Libaray level-3.
 */

#include "xf_blas/handle.h"
#include "xf_blas/gemm_host.h"
#include "xf_blas/utility/utility.h"

using namespace xf::linear_algebra::blas;

/** 
 * @brief This function initializes the XFBLAS library and creates a handle for the specific engine. It must be called prior to any other XFBLAS library calls.
 * @param xclbin file path to FPGA bitstream
 * @param configFile file path to config_info.dat file
 * @param engineName XFBLAS engine to run
 * @param nPE numbers of kernels in the xclbin, default is 1
 * @retval xfblasStatus_t 0 if the initialization succeeded 
 * @retval xfblasStatus_t 1 if the opencl runtime initialization failed
 * @retval xfblasStatus_t 2 if the xclbin doesn't contain the engine
 * @retval xfblasStatus_t 4 if the engine is not supported for now
 */
xfblasStatus_t xfblasCreate(char * xclbin, string configFile, xfblasEngine_t engineName, unsigned int nPE = 1) {
    xfblasStatus_t l_status = buildConfigDict(configFile, engineName, &ConfigDict::instance().m_dict);  
    if (l_status != XFBLAS_STATUS_SUCCESS){
        return l_status;
    }
    
    if (engineName == XFBLAS_ENGINE_GEMM) {
        for (unsigned i = 0; i < nPE; i++) {
            if (ConfigDict::instance().m_dict["GEMX_runGemm"] != "1"){
                return XFBLAS_STATUS_INVALID_VALUE;
            }
            string l_kName = BLASHost<short*>::getKernelName(i);
            BLASHostHandle<void*>::instance().m_handlePtr.push_back(shared_ptr<BLASHost<void*> > ( new GEMMHost<void*>(xclbin, l_kName, &l_status) ));
            if (l_status != XFBLAS_STATUS_SUCCESS){
                return l_status;
            }
        }        
        return XFBLAS_STATUS_SUCCESS;
    } else {
        return XFBLAS_STATUS_NOT_SUPPORTED;
    }
}

/** 
 * @brief This function allocates memory for host memory row-major format matrix on the FPGA device.
 * @param rows number of rows in the matrix
 * @param cols number of cols in the matrix that is being used
 * @param elemSize number of bytes required to store each element in the matrix
 * @param A pointer to the matrix array in the host memory
 * @param lda leading dimension of the matrix that indicates the total number of cols in the matrix
 * @param PE index of kernel that is being used, default is 0
 * @retval xfblasStatus_t 0 if the allocation completed successfully
 * @retval xfblasStatus_t 1 if the library was not initialized
 * @retval xfblasStatus_t 2 if parameters rows, cols, elemSize, lda <= 0 or cols > lda 
 * @retval xfblasStatus_t 3 if there is memory already allocated to the same matrix
 * @retval xfblasStatus_t 4 if the engine is not supported for now
 * @retval xfblasStatus_t 5 if rows, cols or lda is not padded correctly
 */
xfblasStatus_t xfblasMalloc(int rows, int cols, int elemSize, void * A, int lda, unsigned int PE = 0){
    if (ConfigDict::instance().m_dict.find("not_initialized") != ConfigDict::instance().m_dict.end()){
        return XFBLAS_STATUS_NOT_INITIALIZED;       
    }
    if (rows <= 0 || cols <= 0 || lda <=0 || elemSize <=0 ||cols > lda){
        return XFBLAS_STATUS_INVALID_VALUE;
    }
    if (ConfigDict::instance().m_dict["GEMX_runGemm"] == "1"){
        int l_minSize = stoi(ConfigDict::instance().m_dict["minSize"]);
        if (rows % l_minSize != 0 || cols % l_minSize != 0 || lda % l_minSize != 0 ){
            return XFBLAS_STATUS_NOT_PADDED; 
        }
        unsigned long long l_bufSize = rows * lda * elemSize;
        xfblasStatus_t l_status = BLASHostHandle<void*>::instance().m_handlePtr[PE]->allocMat(A, A, l_bufSize);
        return l_status;
    } else {
        return XFBLAS_STATUS_NOT_SUPPORTED;
    }
}  

/**
 * @brief This function copies a matrix in host memory to FPGA device memory. xfblasMalloc() need to be called prior to this function.
 * @param A pointer to the matrix array in the host memory
 * @param PE index of kernel that is being used, default is 0
 * @retval xfblasStatus_t 0 if the operation completed successfully
 * @retval xfblasStatus_t 1 if the library was not initialized
 * @retval xfblasStatus_t 3 if there is no FPGA device memory allocated for the matrix
 */
xfblasStatus_t xfblasSetMatrix(void * A, unsigned int PE = 0){
    if (ConfigDict::instance().m_dict.find("not_initialized") != ConfigDict::instance().m_dict.end()){
        return XFBLAS_STATUS_NOT_INITIALIZED;       
    }
    xfblasStatus_t l_status = BLASHostHandle<void*>::instance().m_handlePtr[PE]->setMatToFPGA(A);
    return l_status;
}  

/**
 * @brief This function performs the matrix-matrix multiplication C = alpha*op(A)op(B) + beta*C 
 * @param transa operation op(A) that is non- or (conj.) transpose
 * @param transb operation op(B) that is non- or (conj.) transpose
 * @param m number of rows in matrix A, matrix C
 * @param n number of cols in matrix B, matrix C
 * @param k number of cols in matrix A, number of rows in matrix B
 * @param alpha scalar used for multiplication
 * @param A pointer to matrix A in the host memory
 * @param lda leading dimension of matirx A
 * @param B pointer to matrix B in the host memory
 * @param ldb leading dimension of matrix B
 * @param beta scalar used for multiplication
 * @param C pointer to matrix C in the host memory
 * @param ldc leading dimension of matrix C
 * @param PE index of kernel that is being used, default is 0
 * @retval xfblasStatus_t 0 if the operation completed successfully
 * @retval xfblasStatus_t 1 if the library was not initialized
 * @retval xfblasStatus_t 3 if not all the matrices have FPGA devie memory allocated
 * @retval xfblasStatus_t 4 if the engine is not supported for now
 */
xfblasStatus_t xfblasSgemm(xfblasOperation_t transa, xfblasOperation_t transb, int m, int n, int k, int alpha, void * A, int lda, void * B, int ldb, int beta, void * C, int ldc, unsigned int PE = 0){
    if (ConfigDict::instance().m_dict.find("not_initialized") != ConfigDict::instance().m_dict.end()){
        return XFBLAS_STATUS_NOT_INITIALIZED;       
    }
    if (ConfigDict::instance().m_dict["GEMX_runGemm"] == "1"){
        if (transa == XFBLAS_OP_N && transb == XFBLAS_OP_N && alpha == 1 && beta == 1){
            GEMMHost<void*>* l_gemmPtr = static_cast<GEMMHost<void*> *> (BLASHostHandle<void*>::instance().m_handlePtr[PE].get());
            xfblasStatus_t l_status = l_gemmPtr->addGEMMOp(A, B, C, C, m, k, n, lda, ldb, ldc, ldc, 1, 0);
            l_status = l_gemmPtr -> execute();
            return l_status;
        } else {
            return XFBLAS_STATUS_NOT_SUPPORTED;
        }
    } else {
        return XFBLAS_STATUS_NOT_SUPPORTED;
    }
}

/**
 * @brief This function copies a matrix in FPGA device memory to host memory
 * @param A pointer to matrix A in the host memory
 * @param PE index of kernel that is being used, default is 0
 * @retval xfblasStatus_t 0 if the operation completed successfully
 * @retval xfblasStatus_t 1 if the library was not initialized
 * @retval xfblasStatus_t 3 if there is no FPGA device memory allocated for the matrix
 */
xfblasStatus_t xfblasGetMatrix(void *A, unsigned int PE = 0) {
    if (ConfigDict::instance().m_dict.find("not_initialized") != ConfigDict::instance().m_dict.end()){
        return XFBLAS_STATUS_NOT_INITIALIZED;       
    }
    xfblasStatus_t l_status = BLASHostHandle<void*>::instance().m_handlePtr[PE]->getMat(A, true);
    BLASHostHandle<void*>::instance().m_handlePtr[PE]->clearInstrBuf();
    return l_status;
}
/**
 * @brief This function frees memory in FPGA device.
 * @param A pointer to matrix A in the host memory
 * @param PE index of kernel that is being used, default is 0
 * @retval xfblasStatus_t 0 if the operation completed successfully
 * @retval xfblasStatus_t 1 if the library was not initialized
 * @retval xfblasStatus_t 3 if there is no FPGA device memory allocated for the matrix
 */
xfblasStatus_t xfblasFree(void *A, unsigned int PE = 0) {
    if (ConfigDict::instance().m_dict.find("not_initialized") != ConfigDict::instance().m_dict.end()){
        return XFBLAS_STATUS_NOT_INITIALIZED;       
    }
    xfblasStatus_t l_status = BLASHostHandle<void*>::instance().m_handlePtr[PE]->freeMat(A);
    return l_status;
}

/**
 * @brief This function releases handle used by the XFBLAS library.
 * @retval xfblasStatus_t 0 if the shut down succeeded
 * @retval xfblasStatus_t 1 if the library was not initialized
 */
xfblasStatus_t xfblasDestory(){
    if (ConfigDict::instance().m_dict.find("not_initialized") != ConfigDict::instance().m_dict.end()){
        return XFBLAS_STATUS_NOT_INITIALIZED;       
    }  
    BLASHostHandle<void*>::instance().m_handlePtr.clear();
    ConfigDict::instance().m_dict.clear();
    return XFBLAS_STATUS_SUCCESS;
}
  


#endif