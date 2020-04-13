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

#ifndef XF_BLAS_WRAPPER_HPP
#define XF_BLAS_WRAPPER_HPP

#include "handle.hpp"
#include "gemm_host.hpp"
#include "gemv_host.hpp"

namespace xf {

namespace blas {

/**
 * @brief This function initializes the XFBLAS library and creates a handle for the specific engine. It must be called
 * prior to any other XFBLAS library calls.
 * @param xclbin file path to FPGA bitstream
 * @param configFile file path to config_info.dat file
 * @param engineName XFBLAS engine to run
 * @param kernelNumber number of kernels that is being used, default is 1
 * @param deviceIndex index of device that is being used, default is 0
 * @retval xfblasStatus_t 0 if the initialization succeeded
 * @retval xfblasStatus_t 1 if the opencl runtime initialization failed
 * @retval xfblasStatus_t 2 if the xclbin doesn't contain the engine
 * @retval xfblasStatus_t 4 if the engine is not supported for now
 */
xfblasStatus_t xfblasCreate(const char* xclbin,
                            string configFile,
                            xfblasEngine_t engineName,
                            unsigned int kernelNumber = 1,
                            unsigned int deviceIndex = 0) {
    xfblasStatus_t l_status = buildConfigDict(configFile, engineName, &ConfigDict::instance().m_dict);
    if (l_status != XFBLAS_STATUS_SUCCESS) {
        return l_status;
    }

    int l_err = 0;

    shared_ptr<XFpga> l_xFpga(new XFpga(xclbin, &l_err, deviceIndex));
    XFpgaHold::instance().m_xFpgaPtr[deviceIndex] = l_xFpga;

    if (l_err != 0) {
        return XFBLAS_STATUS_NOT_INITIALIZED;
    }

    if (engineName == XFBLAS_ENGINE_GEMM) {
        if (ConfigDict::instance().m_dict["GEMX_runGemm"] != "1") {
            return XFBLAS_STATUS_INVALID_VALUE;
        }

        for (unsigned int i = 0; i < kernelNumber; i++) {
            BLASHostHandle::instance().m_handlePtr[deviceIndex].push_back(
                shared_ptr<BLASHost>(new GEMMHost(xclbin, &l_status, i, deviceIndex)));
        }
        return l_status;
    } else if (engineName == XFBLAS_ENGINE_GEMV) {
        if (ConfigDict::instance().m_dict["GEMX_runGemv"] != "1") {
            return XFBLAS_STATUS_INVALID_VALUE;
        }

        for (unsigned int i = 0; i < kernelNumber; i++) {
            BLASHostHandle::instance().m_handlePtr[deviceIndex].push_back(
                shared_ptr<BLASHost>(new GEMVHost(xclbin, &l_status, i, deviceIndex)));
        }
        return l_status;

    } else {
        return XFBLAS_STATUS_NOT_SUPPORTED;
    }
}

/**
 * @brief This function allocates memory on the FPGA device.
 * @param devPtr pointer to mapped memory
 * @param rows number of rows in the matrix
 * @param lda leading dimension of the matrix that indicates the total number of cols in the matrix
 * @param elemSize number of bytes required to store each element in the matrix
 * @param kernelIndex index of kernel that is being used, default is 0
 * @param deviceIndex index of device that is being used, default is 0
 * @retval xfblasStatus_t 0 if the allocation completed successfully
 * @retval xfblasStatus_t 1 if the library was not initialized
 * @retval xfblasStatus_t 2 if parameters rows, cols, elemSize, lda <= 0 or cols > lda or data types are not matched
 * @retval xfblasStatus_t 3 if there is memory already allocated to the same matrix
 * @retval xfblasStatus_t 4 if the engine is not supported for now
 */
xfblasStatus_t xfblasMalloc(
    short** devPtr, int rows, int lda, int elemSize, unsigned int kernelIndex = 0, unsigned int deviceIndex = 0) {
    if (ConfigDict::instance().m_dict.empty()) {
        return XFBLAS_STATUS_NOT_INITIALIZED;
    }
    if (rows <= 0 || lda <= 0 || elemSize <= 0) {
        return XFBLAS_STATUS_INVALID_VALUE;
    }

    if (ConfigDict::instance().m_dict["GEMX_dataType"] != "short") {
        return XFBLAS_STATUS_INVALID_VALUE;
    }

    if (ConfigDict::instance().m_dict["GEMX_runGemm"] == "1") {
        int l_minSize = stoi(ConfigDict::instance().m_dict["minSize"]);
        xfblasStatus_t l_status = XFBLAS_STATUS_SUCCESS;
        if (rows % l_minSize != 0 || lda % l_minSize != 0) {
            int l_paddedRows = getPaddedSize(rows, l_minSize);
            int l_paddedLda = getPaddedSize(lda, l_minSize);
            unsigned long long l_bufSize = l_paddedRows * l_paddedLda * elemSize;
            l_status =
                BLASHostHandle::instance().m_handlePtr[deviceIndex][kernelIndex]->allocMat<short*>(devPtr, l_bufSize);
        } else {
            unsigned long long l_bufSize = rows * lda * elemSize;
            l_status =
                BLASHostHandle::instance().m_handlePtr[deviceIndex][kernelIndex]->allocMat<short*>(devPtr, l_bufSize);
        }
        return l_status;
    } else if (ConfigDict::instance().m_dict["GEMX_runGemv"] == "1") {
        xfblasStatus_t l_status = XFBLAS_STATUS_SUCCESS;
        int l_minSize = stoi(ConfigDict::instance().m_dict["minSize"]);
        int l_paddedRows, l_paddedLda;
        if (lda == 1) {
            l_paddedRows = getPaddedSize(rows, l_minSize);
            l_paddedLda = lda;
        } else {
            l_paddedRows = getPaddedSize(rows, l_minSize);
            l_paddedLda = getPaddedSize(lda, l_minSize);
        }
        unsigned long long l_bufSize = l_paddedRows * l_paddedLda * elemSize;
        l_status =
            BLASHostHandle::instance().m_handlePtr[deviceIndex][kernelIndex]->allocMat<short*>(devPtr, l_bufSize);
        return l_status;

    } else {
        return XFBLAS_STATUS_NOT_SUPPORTED;
    }
}

xfblasStatus_t xfblasMalloc(
    float** devPtr, int rows, int lda, int elemSize, unsigned int kernelIndex = 0, unsigned int deviceIndex = 0) {
    if (ConfigDict::instance().m_dict.empty()) {
        return XFBLAS_STATUS_NOT_INITIALIZED;
    }
    if (rows <= 0 || lda <= 0 || elemSize <= 0) {
        return XFBLAS_STATUS_INVALID_VALUE;
    }

    if (ConfigDict::instance().m_dict["GEMX_dataType"] != "float") {
        return XFBLAS_STATUS_INVALID_VALUE;
    }

    if (ConfigDict::instance().m_dict["GEMX_runGemm"] == "1") {
        int l_minSize = stoi(ConfigDict::instance().m_dict["minSize"]);
        xfblasStatus_t l_status = XFBLAS_STATUS_SUCCESS;
        if (rows % l_minSize != 0 || lda % l_minSize != 0) {
            int paddedRows = getPaddedSize(rows, l_minSize);
            int paddedLda = getPaddedSize(lda, l_minSize);
            unsigned long long l_bufSize = paddedRows * paddedLda * elemSize;
            l_status =
                BLASHostHandle::instance().m_handlePtr[deviceIndex][kernelIndex]->allocMat<float*>(devPtr, l_bufSize);
        } else {
            unsigned long long l_bufSize = rows * lda * elemSize;
            l_status =
                BLASHostHandle::instance().m_handlePtr[deviceIndex][kernelIndex]->allocMat<float*>(devPtr, l_bufSize);
        }
        return l_status;

    } else if (ConfigDict::instance().m_dict["GEMX_runGemv"] == "1") {
        xfblasStatus_t l_status = XFBLAS_STATUS_SUCCESS;
        int l_minSize = stoi(ConfigDict::instance().m_dict["minSize"]);
        int l_paddedRows, l_paddedLda;
        if (lda == 1) {
            l_paddedRows = getPaddedSize(rows, l_minSize);
            l_paddedLda = lda;
        } else {
            l_paddedRows = getPaddedSize(rows, l_minSize);
            l_paddedLda = getPaddedSize(lda, l_minSize);
        }
        unsigned long long l_bufSize = l_paddedRows * l_paddedLda * elemSize;
        l_status =
            BLASHostHandle::instance().m_handlePtr[deviceIndex][kernelIndex]->allocMat<float*>(devPtr, l_bufSize);
        return l_status;
    } else {
        return XFBLAS_STATUS_NOT_SUPPORTED;
    }
}

/**
 * @brief This function allocates memory for host row-major format matrix on the FPGA device.
 * @param rows number of rows in the matrix
 * @param cols number of cols in the matrix that is being used
 * @param elemSize number of bytes required to store each element in the matrix
 * @param A pointer to the matrix array in the host memory
 * @param lda leading dimension of the matrix that indicates the total number of cols in the matrix
 * @param kernelIndex index of kernel that is being used, default is 0
 * @param deviceIndex index of device that is being used, default is 0
 * @retval xfblasStatus_t 0 if the allocation completed successfully
 * @retval xfblasStatus_t 1 if the library was not initialized
 * @retval xfblasStatus_t 2 if parameters rows, cols, elemSize, lda <= 0 or cols > lda or data types are not matched
 * @retval xfblasStatus_t 3 if there is memory already allocated to the same matrix
 * @retval xfblasStatus_t 4 if the engine is not supported for now
 * @retval xfblasStatus_t 5 if rows, cols or lda is not padded correctly
 */
xfblasStatus_t xfblasMallocRestricted(
    int rows, int cols, int elemSize, void* A, int lda, unsigned int kernelIndex = 0, unsigned int deviceIndex = 0) {
    if (ConfigDict::instance().m_dict.find("not_initialized") != ConfigDict::instance().m_dict.end()) {
        return XFBLAS_STATUS_NOT_INITIALIZED;
    }
    if (rows <= 0 || cols <= 0 || lda <= 0 || elemSize <= 0 || cols > lda) {
        return XFBLAS_STATUS_INVALID_VALUE;
    }

    if (getTypeSize(ConfigDict::instance().m_dict["GEMX_dataType"]) != elemSize) {
        return XFBLAS_STATUS_INVALID_VALUE;
    }

    if (ConfigDict::instance().m_dict["GEMX_runGemm"] == "1") {
        int l_minSize = stoi(ConfigDict::instance().m_dict["minSize"]);
        if (rows % l_minSize != 0 || cols % l_minSize != 0 || lda % l_minSize != 0) {
            return XFBLAS_STATUS_NOT_PADDED;
        } else {
            unsigned long long l_bufSize = rows * lda * elemSize;
            xfblasStatus_t l_status =
                BLASHostHandle::instance().m_handlePtr[deviceIndex][kernelIndex]->allocMatRestricted(A, A, l_bufSize);
            return l_status;
        }

    } else if (ConfigDict::instance().m_dict["GEMX_runGemv"] == "1") {
        int l_minSize = stoi(ConfigDict::instance().m_dict["minSize"]);
        if (lda == 1) {
            if (rows % l_minSize != 0) {
                return XFBLAS_STATUS_NOT_PADDED;
            } else {
                unsigned long long l_bufSize = rows * lda * elemSize;
                xfblasStatus_t l_status =
                    BLASHostHandle::instance().m_handlePtr[deviceIndex][kernelIndex]->allocMatRestricted(A, A,
                                                                                                         l_bufSize);
                return l_status;
            }
        } else {
            if (rows % l_minSize != 0 || cols % l_minSize != 0 || lda % l_minSize != 0) {
                return XFBLAS_STATUS_NOT_PADDED;
            } else {
                unsigned long long l_bufSize = rows * lda * elemSize;
                xfblasStatus_t l_status =
                    BLASHostHandle::instance().m_handlePtr[deviceIndex][kernelIndex]->allocMatRestricted(A, A,
                                                                                                         l_bufSize);
                return l_status;
            }
        }
    } else {
        return XFBLAS_STATUS_NOT_SUPPORTED;
    }
}

/**
 * @brief This function allocates memory on the FPGA device, rewrites the leading dimension size after padding.
 * @param devPtr pointer to mapped memory
 * @param paddedLda leading dimension of the matrix after padding
 * @param rows number of rows in the matrix
 * @param lda leading dimension of the matrix that indicates the total number of cols in the matrix
 * @param elemSize number of bytes required to store each element in the matrix
 * @param kernelIndex index of kernel that is being used, default is 0
 * @param deviceIndex index of device that is being used, default is 0
 * @retval xfblasStatus_t 0 if the allocation completed successfully
 * @retval xfblasStatus_t 1 if the library was not initialized
 * @retval xfblasStatus_t 2 if parameters rows, cols, elemSize, lda <= 0 or cols > lda or data types are not matched
 * @retval xfblasStatus_t 3 if there is memory already allocated to the same matrix
 * @retval xfblasStatus_t 4 if the engine is not supported for now
 */
xfblasStatus_t xfblasMallocManaged(short** devPtr,
                                   int* paddedLda,
                                   int rows,
                                   int lda,
                                   int elemSize,
                                   unsigned int kernelIndex = 0,
                                   unsigned int deviceIndex = 0) {
    if (ConfigDict::instance().m_dict.empty()) {
        return XFBLAS_STATUS_NOT_INITIALIZED;
    }
    if (rows <= 0 || lda <= 0 || elemSize <= 0) {
        return XFBLAS_STATUS_INVALID_VALUE;
    }

    if (ConfigDict::instance().m_dict["GEMX_dataType"] != "short") {
        return XFBLAS_STATUS_INVALID_VALUE;
    }

    if (ConfigDict::instance().m_dict["GEMX_runGemm"] == "1") {
        int l_minSize = stoi(ConfigDict::instance().m_dict["minSize"]);
        xfblasStatus_t l_status = XFBLAS_STATUS_SUCCESS;
        if (rows % l_minSize != 0 || lda % l_minSize != 0) {
            int l_paddedRows = getPaddedSize(rows, l_minSize);
            int l_paddedLda = getPaddedSize(lda, l_minSize);
            unsigned long long l_bufSize = l_paddedRows * l_paddedLda * elemSize;
            *paddedLda = getPaddedSize(lda, l_minSize);
            l_status =
                BLASHostHandle::instance().m_handlePtr[deviceIndex][kernelIndex]->allocMat<short*>(devPtr, l_bufSize);
        } else {
            unsigned long long l_bufSize = rows * lda * elemSize;
            *paddedLda = lda;
            l_status =
                BLASHostHandle::instance().m_handlePtr[deviceIndex][kernelIndex]->allocMat<short*>(devPtr, l_bufSize);
        }
        return l_status;

    } else if (ConfigDict::instance().m_dict["GEMX_runGemv"] == "1") {
        xfblasStatus_t l_status = XFBLAS_STATUS_SUCCESS;
        int l_minSize = stoi(ConfigDict::instance().m_dict["minSize"]);
        int l_paddedRows, l_paddedLda;
        if (lda == 1) {
            l_paddedRows = getPaddedSize(rows, l_minSize);
            l_paddedLda = lda;
            *paddedLda = lda;
        } else {
            l_paddedRows = getPaddedSize(rows, l_minSize);
            l_paddedLda = getPaddedSize(lda, l_minSize);
            *paddedLda = l_paddedLda;
        }
        unsigned long long l_bufSize = l_paddedRows * l_paddedLda * elemSize;
        l_status =
            BLASHostHandle::instance().m_handlePtr[deviceIndex][kernelIndex]->allocMat<short*>(devPtr, l_bufSize);
        return l_status;
    } else {
        return XFBLAS_STATUS_NOT_SUPPORTED;
    }
}

xfblasStatus_t xfblasMallocManaged(float** devPtr,
                                   int* paddedLda,
                                   int rows,
                                   int lda,
                                   int elemSize,
                                   unsigned int kernelIndex = 0,
                                   unsigned int deviceIndex = 0) {
    if (ConfigDict::instance().m_dict.empty()) {
        return XFBLAS_STATUS_NOT_INITIALIZED;
    }
    if (rows <= 0 || lda <= 0 || elemSize <= 0) {
        return XFBLAS_STATUS_INVALID_VALUE;
    }

    if (ConfigDict::instance().m_dict["GEMX_dataType"] != "float") {
        return XFBLAS_STATUS_INVALID_VALUE;
    }

    if (ConfigDict::instance().m_dict["GEMX_runGemm"] == "1") {
        int l_minSize = stoi(ConfigDict::instance().m_dict["minSize"]);
        xfblasStatus_t l_status = XFBLAS_STATUS_SUCCESS;
        if (rows % l_minSize != 0 || lda % l_minSize != 0) {
            int l_paddedRows = getPaddedSize(rows, l_minSize);
            int l_paddedLda = getPaddedSize(lda, l_minSize);
            unsigned long long l_bufSize = l_paddedRows * l_paddedLda * elemSize;
            *paddedLda = getPaddedSize(lda, l_minSize);
            l_status =
                BLASHostHandle::instance().m_handlePtr[deviceIndex][kernelIndex]->allocMat<float*>(devPtr, l_bufSize);
        } else {
            unsigned long long l_bufSize = rows * lda * elemSize;
            *paddedLda = lda;
            l_status =
                BLASHostHandle::instance().m_handlePtr[deviceIndex][kernelIndex]->allocMat<float*>(devPtr, l_bufSize);
        }
        return l_status;
    } else if (ConfigDict::instance().m_dict["GEMX_runGemv"] == "1") {
        xfblasStatus_t l_status = XFBLAS_STATUS_SUCCESS;
        int l_minSize = stoi(ConfigDict::instance().m_dict["minSize"]);
        int l_paddedRows, l_paddedLda;
        if (lda == 1) {
            l_paddedRows = getPaddedSize(rows, l_minSize);
            l_paddedLda = lda;
            *paddedLda = lda;
        } else {
            l_paddedRows = getPaddedSize(rows, l_minSize);
            l_paddedLda = getPaddedSize(lda, l_minSize);
            *paddedLda = l_paddedLda;
        }
        unsigned long long l_bufSize = l_paddedRows * l_paddedLda * elemSize;
        l_status =
            BLASHostHandle::instance().m_handlePtr[deviceIndex][kernelIndex]->allocMat<float*>(devPtr, l_bufSize);
        return l_status;
    } else {
        return XFBLAS_STATUS_NOT_SUPPORTED;
    }
}

/**
 * @brief This function copies a matrix in host memory to FPGA device memory. xfblasMalloc() need to be called prior to
 * this function.
 * @param rows number of rows in the matrix
 * @param cols number of cols in the matrix that is being used
 * @param elemSize number of bytes required to store each element in the matrix
 * @param A pointer to the matrix array in the host memory
 * @param lda leading dimension of the matrix that indicates the total number of cols in the matrix
 * @param d_A pointer to mapped memory
 * @param kernelIndex index of kernel that is being used, default is 0
 * @param deviceIndex index of device that is being used, default is 0
 * @retval xfblasStatus_t 0 if the operation completed successfully
 * @retval xfblasStatus_t 1 if the library was not initialized
 * @retval xfblasStatus_t 2 if parameters rows, cols, elemSize, lda <= 0 or cols > lda or data types are not matched
 * @retval xfblasStatus_t 3 if there is no FPGA device memory allocated for the matrix
 * @retval xfblasStatus_t 4 if the engine is not supported for now
 */
xfblasStatus_t xfblasSetMatrix(int rows,
                               int cols,
                               int elemSize,
                               short* A,
                               int lda,
                               short* d_A,
                               unsigned int kernelIndex = 0,
                               unsigned int deviceIndex = 0) {
    if (ConfigDict::instance().m_dict.empty()) {
        return XFBLAS_STATUS_NOT_INITIALIZED;
    }
    if (rows <= 0 || cols <= 0 || lda <= 0 || elemSize <= 0 || cols > lda) {
        return XFBLAS_STATUS_INVALID_VALUE;
    }

    if (ConfigDict::instance().m_dict["GEMX_dataType"] != "short") {
        return XFBLAS_STATUS_INVALID_VALUE;
    }

    if (ConfigDict::instance().m_dict["GEMX_runGemm"] == "1" || ConfigDict::instance().m_dict["GEMX_runGemv"] == "1") {
        int l_minSize = stoi(ConfigDict::instance().m_dict["minSize"]);
        int paddedLda = getPaddedSize(lda, l_minSize);
        xfblasStatus_t l_status =
            BLASHostHandle::instance().m_handlePtr[deviceIndex][kernelIndex]->setMatToFPGA<short*>(d_A, rows, lda,
                                                                                                   paddedLda, A, d_A);
        return l_status;
    } else {
        return XFBLAS_STATUS_NOT_SUPPORTED;
    }
}

xfblasStatus_t xfblasSetMatrix(int rows,
                               int cols,
                               int elemSize,
                               float* A,
                               int lda,
                               float* d_A,
                               unsigned int kernelIndex = 0,
                               unsigned int deviceIndex = 0) {
    if (ConfigDict::instance().m_dict.empty()) {
        return XFBLAS_STATUS_NOT_INITIALIZED;
    }
    if (rows <= 0 || cols <= 0 || lda <= 0 || elemSize <= 0 || cols > lda) {
        return XFBLAS_STATUS_INVALID_VALUE;
    }

    if (ConfigDict::instance().m_dict["GEMX_dataType"] != "float") {
        return XFBLAS_STATUS_INVALID_VALUE;
    }

    if (ConfigDict::instance().m_dict["GEMX_runGemm"] == "1" || ConfigDict::instance().m_dict["GEMX_runGemv"] == "1") {
        int l_minSize = stoi(ConfigDict::instance().m_dict["minSize"]);
        int paddedLda = getPaddedSize(lda, l_minSize);
        xfblasStatus_t l_status =
            BLASHostHandle::instance().m_handlePtr[deviceIndex][kernelIndex]->setMatToFPGA<float*>(d_A, rows, lda,
                                                                                                   paddedLda, A, d_A);
        return l_status;
    } else {
        return XFBLAS_STATUS_NOT_SUPPORTED;
    }
}

/**
 * @brief This function copies a vector in host memory to FPGA device memory. xfblasMalloc() need to be called prior to
 * this function.
 * @param n number of elements in vector
 * @param elemSize number of bytes required to store each element in the vector
 * @param x pointer to the vector in the host memory
 * @param incx the storage spacing between consecutive elements of vector x
 * @param d_x pointer to mapped memory
 * @param kernelIndex index of kernel that is being used, default is 0
 * @param deviceIndex index of device that is being used, default is 0
 * @retval xfblasStatus_t 0 if the operation completed successfully
 * @retval xfblasStatus_t 1 if the library was not initialized
 * @retval xfblasStatus_t 2 if parameters rows, cols, elemSize, lda <= 0 or cols > lda or data types are not matched
 * @retval xfblasStatus_t 3 if there is no FPGA device memory allocated for the vector
 * @retval xfblasStatus_t 4 if the engine is not supported for now
 */
xfblasStatus_t xfblasSetVector(
    int n, int elemSize, short* x, int incx, short* d_x, unsigned int kernelIndex = 0, unsigned int deviceIndex = 0) {
    if (ConfigDict::instance().m_dict.empty()) {
        return XFBLAS_STATUS_NOT_INITIALIZED;
    }
    if (n <= 0 || elemSize <= 0) {
        return XFBLAS_STATUS_INVALID_VALUE;
    }

    if (ConfigDict::instance().m_dict["GEMX_dataType"] != "short") {
        return XFBLAS_STATUS_INVALID_VALUE;
    }

    if (ConfigDict::instance().m_dict["GEMX_runGemv"] == "1") {
        xfblasStatus_t l_status =
            BLASHostHandle::instance().m_handlePtr[deviceIndex][kernelIndex]->setMatToFPGA<short*>(d_x, n, 1, 1, x,
                                                                                                   d_x);
        return l_status;
    } else {
        return XFBLAS_STATUS_NOT_SUPPORTED;
    }
}

xfblasStatus_t xfblasSetVector(
    int n, int elemSize, float* x, int incx, float* d_x, unsigned int kernelIndex = 0, unsigned int deviceIndex = 0) {
    if (ConfigDict::instance().m_dict.empty()) {
        return XFBLAS_STATUS_NOT_INITIALIZED;
    }
    if (n <= 0 || elemSize <= 0) {
        return XFBLAS_STATUS_INVALID_VALUE;
    }

    if (ConfigDict::instance().m_dict["GEMX_dataType"] != "float") {
        return XFBLAS_STATUS_INVALID_VALUE;
    }

    if (ConfigDict::instance().m_dict["GEMX_runGemv"] == "1") {
        xfblasStatus_t l_status =
            BLASHostHandle::instance().m_handlePtr[deviceIndex][kernelIndex]->setMatToFPGA<float*>(d_x, n, 1, 1, x,
                                                                                                   d_x);
        return l_status;
    } else {
        return XFBLAS_STATUS_NOT_SUPPORTED;
    }
}

/**
 * @brief This function copies a matrix in host memory to FPGA device memory. xfblasMallocRestricted() need to be called
 * prior to this function.
 * @param A pointer to the matrix array in the host memory
 * @param kernelIndex index of kernel that is being used, default is 0
 * @param deviceIndex index of device that is being used, default is 0
 * @retval xfblasStatus_t 0 if the operation completed successfully
 * @retval xfblasStatus_t 1 if the library was not initialized
 * @retval xfblasStatus_t 3 if there is no FPGA device memory allocated for the matrix
 */
xfblasStatus_t xfblasSetMatrixRestricted(void* A, unsigned int kernelIndex = 0, unsigned int deviceIndex = 0) {
    if (ConfigDict::instance().m_dict.find("not_initialized") != ConfigDict::instance().m_dict.end()) {
        return XFBLAS_STATUS_NOT_INITIALIZED;
    }
    xfblasStatus_t l_status =
        BLASHostHandle::instance().m_handlePtr[deviceIndex][kernelIndex]->setMatToFPGARestricted(A);
    return l_status;
}

/**
 * @brief This function copies a vector in host memory to FPGA device memory. xfblasMallocRestricted() need to be called
 * prior to this function.
 * @param x pointer to the vector in the host memory
 * @param kernelIndex index of kernel that is being used, default is 0
 * @param deviceIndex index of device that is being used, default is 0
 * @retval xfblasStatus_t 0 if the operation completed successfully
 * @retval xfblasStatus_t 1 if the library was not initialized
 * @retval xfblasStatus_t 3 if there is no FPGA device memory allocated for the vector
 */
xfblasStatus_t xfblasSetVectorRestricted(void* x, unsigned int kernelIndex = 0, unsigned int deviceIndex = 0) {
    if (ConfigDict::instance().m_dict.find("not_initialized") != ConfigDict::instance().m_dict.end()) {
        return XFBLAS_STATUS_NOT_INITIALIZED;
    }
    xfblasStatus_t l_status =
        BLASHostHandle::instance().m_handlePtr[deviceIndex][kernelIndex]->setMatToFPGARestricted(x);
    return l_status;
}

/**
 * @brief This function will synchronize all the device memory to host memory
 * @param kernelIndex index of kernel that is being used, default is 0
 * @param deviceIndex index of device that is being used, default is 0
 * @retval xfblasStatus_t 0 if the operation completed successfully
 * @retval xfblasStatus_t 1 if the library was not initialized
 * @retval xfblasStatus_t 3 if there is no FPGA device memory allocated for some of the matrices in the host memory
 */
xfblasStatus_t xfblasDeviceSynchronize(unsigned int kernelIndex = 0, unsigned int deviceIndex = 0) {
    if (ConfigDict::instance().m_dict.find("not_initialized") != ConfigDict::instance().m_dict.end()) {
        return XFBLAS_STATUS_NOT_INITIALIZED;
    }
    xfblasStatus_t l_status = BLASHostHandle::instance().m_handlePtr[deviceIndex][kernelIndex]->deviceSync();
    if (l_status != XFBLAS_STATUS_SUCCESS) {
        return l_status;
    }
    l_status = BLASHostHandle::instance().m_handlePtr[deviceIndex][kernelIndex]->execute();
    l_status = BLASHostHandle::instance().m_handlePtr[deviceIndex][kernelIndex]->getMatManaged();
    return l_status;
}

/**
 * @brief This function copies a matrix in FPGA device memory to host memory
 * @param rows number of rows in the matrix
 * @param cols number of cols in the matrix that is being used
 * @param elemSize number of bytes required to store each element in the matrix
 * @param d_A pointer to mapped memory
 * @param A pointer to the matrix array in the host memory
 * @param lda leading dimension of the matrix that indicates the total number of cols in the matrix
 * @param kernelIndex index of kernel that is being used, default is 0
 * @param deviceIndex index of device that is being used, default is 0
 * @retval xfblasStatus_t 0 if the operation completed successfully
 * @retval xfblasStatus_t 1 if the library was not initialized
 * @retval xfblasStatus_t 3 if there is no FPGA device memory allocated for the matrix
 */
xfblasStatus_t xfblasGetMatrix(int rows,
                               int cols,
                               int elemSize,
                               short* d_A,
                               short* A,
                               int lda,
                               unsigned int kernelIndex = 0,
                               unsigned int deviceIndex = 0) {
    if (ConfigDict::instance().m_dict.empty()) {
        return XFBLAS_STATUS_NOT_INITIALIZED;
    }
    if (rows <= 0 || cols <= 0 || lda <= 0 || elemSize <= 0 || cols > lda) {
        return XFBLAS_STATUS_INVALID_VALUE;
    }

    if (ConfigDict::instance().m_dict["GEMX_dataType"] != "short") {
        return XFBLAS_STATUS_INVALID_VALUE;
    }

    if (ConfigDict::instance().m_dict["GEMX_runGemm"] == "1" || ConfigDict::instance().m_dict["GEMX_runGemv"] == "1") {
        int l_minSize = stoi(ConfigDict::instance().m_dict["minSize"]);
        int paddedLda = getPaddedSize(lda, l_minSize);
        xfblasStatus_t l_status = BLASHostHandle::instance().m_handlePtr[deviceIndex][kernelIndex]->execute();
        l_status = BLASHostHandle::instance().m_handlePtr[deviceIndex][kernelIndex]->getMat<short*>(d_A, rows, lda,
                                                                                                    paddedLda, A, d_A);
        return l_status;
    } else {
        return XFBLAS_STATUS_NOT_SUPPORTED;
    }
}

xfblasStatus_t xfblasGetMatrix(int rows,
                               int cols,
                               int elemSize,
                               float* d_A,
                               float* A,
                               int lda,
                               unsigned int kernelIndex = 0,
                               unsigned int deviceIndex = 0) {
    if (ConfigDict::instance().m_dict.empty()) {
        return XFBLAS_STATUS_NOT_INITIALIZED;
    }
    if (rows <= 0 || cols <= 0 || lda <= 0 || elemSize <= 0 || cols > lda) {
        return XFBLAS_STATUS_INVALID_VALUE;
    }

    if (ConfigDict::instance().m_dict["GEMX_dataType"] != "float") {
        return XFBLAS_STATUS_INVALID_VALUE;
    }

    if (ConfigDict::instance().m_dict["GEMX_runGemm"] == "1" || ConfigDict::instance().m_dict["GEMX_runGemv"] == "1") {
        int l_minSize = stoi(ConfigDict::instance().m_dict["minSize"]);
        int paddedLda = getPaddedSize(lda, l_minSize);
        xfblasStatus_t l_status = BLASHostHandle::instance().m_handlePtr[deviceIndex][kernelIndex]->execute();
        l_status = BLASHostHandle::instance().m_handlePtr[deviceIndex][kernelIndex]->getMat<float*>(d_A, rows, lda,
                                                                                                    paddedLda, A, d_A);
        return l_status;
    } else {
        return XFBLAS_STATUS_NOT_SUPPORTED;
    }
}

/**
 * @brief This function copies a vector in FPGA device memory to host memory
 * @param n number of elements in vector
 * @param elemSize number of bytes required to store each element in the vector
 * @param d_x pointer to mapped memory
 * @param x pointer to the vector in the host memory
 * @param incx the storage spacing between consecutive elements of vector x
 * @param kernelIndex index of kernel that is being used, default is 0
 * @param deviceIndex index of device that is being used, default is 0
 * @retval xfblasStatus_t 0 if the operation completed successfully
 * @retval xfblasStatus_t 1 if the library was not initialized
 * @retval xfblasStatus_t 3 if there is no FPGA device memory allocated for the vector
 */
xfblasStatus_t xfblasGetVector(
    int n, int elemSize, short* d_x, short* x, int incx, unsigned int kernelIndex = 0, unsigned int deviceIndex = 0) {
    if (ConfigDict::instance().m_dict.empty()) {
        return XFBLAS_STATUS_NOT_INITIALIZED;
    }
    if (n <= 0 || elemSize <= 0) {
        return XFBLAS_STATUS_INVALID_VALUE;
    }

    if (ConfigDict::instance().m_dict["GEMX_dataType"] != "short") {
        return XFBLAS_STATUS_INVALID_VALUE;
    }

    if (ConfigDict::instance().m_dict["GEMX_runGemv"] == "1") {
        xfblasStatus_t l_status = BLASHostHandle::instance().m_handlePtr[deviceIndex][kernelIndex]->execute();
        l_status =
            BLASHostHandle::instance().m_handlePtr[deviceIndex][kernelIndex]->getMat<short*>(d_x, n, 1, 1, x, d_x);
        return l_status;
    } else {
        return XFBLAS_STATUS_NOT_SUPPORTED;
    }
}

xfblasStatus_t xfblasGetVector(
    int n, int elemSize, float* d_x, float* x, int incx, unsigned int kernelIndex = 0, unsigned int deviceIndex = 0) {
    if (ConfigDict::instance().m_dict.empty()) {
        return XFBLAS_STATUS_NOT_INITIALIZED;
    }
    if (n <= 0 || elemSize <= 0) {
        return XFBLAS_STATUS_INVALID_VALUE;
    }

    if (ConfigDict::instance().m_dict["GEMX_dataType"] != "float") {
        return XFBLAS_STATUS_INVALID_VALUE;
    }

    if (ConfigDict::instance().m_dict["GEMX_runGemv"] == "1") {
        xfblasStatus_t l_status = BLASHostHandle::instance().m_handlePtr[deviceIndex][kernelIndex]->execute();
        l_status =
            BLASHostHandle::instance().m_handlePtr[deviceIndex][kernelIndex]->getMat<float*>(d_x, n, 1, 1, x, d_x);
        return l_status;
    } else {
        return XFBLAS_STATUS_NOT_SUPPORTED;
    }
}

/**
 * @brief This function copies a matrix in FPGA device memory to host memory
 * @param A pointer to matrix A in the host memory
 * @param kernelIndex index of kernel that is being used, default is 0
 * @param deviceIndex index of device that is being used, default is 0
 * @retval xfblasStatus_t 0 if the operation completed successfully
 * @retval xfblasStatus_t 1 if the library was not initialized
 * @retval xfblasStatus_t 3 if there is no FPGA device memory allocated for the matrix
 */
xfblasStatus_t xfblasGetMatrixRestricted(void* A, unsigned int kernelIndex = 0, unsigned int deviceIndex = 0) {
    if (ConfigDict::instance().m_dict.find("not_initialized") != ConfigDict::instance().m_dict.end()) {
        return XFBLAS_STATUS_NOT_INITIALIZED;
    }
    xfblasStatus_t l_status = BLASHostHandle::instance().m_handlePtr[deviceIndex][kernelIndex]->execute();
    l_status = BLASHostHandle::instance().m_handlePtr[deviceIndex][kernelIndex]->getMatRestricted(A, A);
    return l_status;
}

/**
 * @brief This function copies a matrix in FPGA device memory to host memory
 * @param x pointer to vetcor x in the host memory
 * @param kernelIndex index of kernel that is being used, default is 0
 * @param deviceIndex index of device that is being used, default is 0
 * @retval xfblasStatus_t 0 if the operation completed successfully
 * @retval xfblasStatus_t 1 if the library was not initialized
 * @retval xfblasStatus_t 3 if there is no FPGA device memory allocated for the matrix
 */
xfblasStatus_t xfblasGetVectorRestricted(void* x, unsigned int kernelIndex = 0, unsigned int deviceIndex = 0) {
    if (ConfigDict::instance().m_dict.find("not_initialized") != ConfigDict::instance().m_dict.end()) {
        return XFBLAS_STATUS_NOT_INITIALIZED;
    }
    xfblasStatus_t l_status = BLASHostHandle::instance().m_handlePtr[deviceIndex][kernelIndex]->execute();
    l_status = BLASHostHandle::instance().m_handlePtr[deviceIndex][kernelIndex]->getMatRestricted(x, x);
    return l_status;
}

/**
 * @brief This function frees memory in FPGA device.
 * @param A pointer to matrix A in the host memory
 * @param kernelIndex index of kernel that is being used, default is 0
 * @param deviceIndex index of device that is being used, default is 0
 * @retval xfblasStatus_t 0 if the operation completed successfully
 * @retval xfblasStatus_t 1 if the library was not initialized
 * @retval xfblasStatus_t 3 if there is no FPGA device memory allocated for the matrix
 */
xfblasStatus_t xfblasFree(void* A, unsigned int kernelIndex = 0, unsigned int deviceIndex = 0) {
    if (ConfigDict::instance().m_dict.empty()) {
        return XFBLAS_STATUS_NOT_INITIALIZED;
    }
    xfblasStatus_t l_status = BLASHostHandle::instance().m_handlePtr[deviceIndex][kernelIndex]->freeMat(A);
    return l_status;
}

/**
 * @brief This function frees instrution
 * @param kernelIndex index of kernel that is being used, default is 0
 * @param deviceIndex index of device that is being used, default is 0
 * @retval xfblasStatus_t 0 if the operation completed successfully
 * @retval xfblasStatus_t 1 if the library was not initialized
 */
xfblasStatus_t xfblasFreeInstr(unsigned int kernelIndex = 0, unsigned int deviceIndex = 0) {
    if (ConfigDict::instance().m_dict.empty()) {
        return XFBLAS_STATUS_NOT_INITIALIZED;
    }
    BLASHostHandle::instance().m_handlePtr[deviceIndex][kernelIndex]->clearInstrBuf();
    return XFBLAS_STATUS_SUCCESS;
}

/**
 * @brief This function releases handle used by the XFBLAS library.
 * @param kernelNumber number of kernels that is being used, default is 1
 * @param deviceIndex index of device that is being used, default is 0
 * @retval xfblasStatus_t 0 if the shut down succeeded
 * @retval xfblasStatus_t 1 if the library was not initialized
 */
xfblasStatus_t xfblasDestroy(unsigned int kernelNumber = 1, unsigned int deviceIndex = 0) {
    if (ConfigDict::instance().m_dict.empty()) {
        return XFBLAS_STATUS_NOT_INITIALIZED;
    }
    xfblasStatus_t l_status = XFBLAS_STATUS_SUCCESS;
    for (unsigned int i = 0; i < kernelNumber; i++) {
        BLASHostHandle::instance().m_handlePtr[deviceIndex][i]->clearInstrBuf();
        l_status = BLASHostHandle::instance().m_handlePtr[deviceIndex][i]->closeContext(i);
    }
    BLASHostHandle::instance().m_handlePtr[deviceIndex][0]->closeDevice();
    XFpgaHold::instance().m_xFpgaPtr.clear();
    BLASHostHandle::instance().m_handlePtr.clear();
    ConfigDict::instance().m_dict.clear();
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
 * @param kernelIndex index of kernel that is being used, default is 0
 * @param deviceIndex index of device that is being used, default is 0
 * @retval xfblasStatus_t 0 if the operation completed successfully
 * @retval xfblasStatus_t 1 if the library was not initialized
 * @retval xfblasStatus_t 3 if not all the matrices have FPGA devie memory allocated
 * @retval xfblasStatus_t 4 if the engine is not supported for now
 */
xfblasStatus_t xfblasGemm(xfblasOperation_t transa,
                          xfblasOperation_t transb,
                          int m,
                          int n,
                          int k,
                          int alpha,
                          void* A,
                          int lda,
                          void* B,
                          int ldb,
                          int beta,
                          void* C,
                          int ldc,
                          unsigned int kernelIndex = 0,
                          unsigned int deviceIndex = 0) {
    if (ConfigDict::instance().m_dict.empty()) {
        return XFBLAS_STATUS_NOT_INITIALIZED;
    }
    if (ConfigDict::instance().m_dict["GEMX_runGemm"] == "1") {
        if (transa == XFBLAS_OP_N && transb == XFBLAS_OP_N && alpha == 1 && beta == 1) {
            GEMMHost* l_gemmPtr =
                static_cast<GEMMHost*>(BLASHostHandle::instance().m_handlePtr[deviceIndex][kernelIndex].get());
            xfblasStatus_t l_status;
            int l_minSize = stoi(ConfigDict::instance().m_dict["minSize"]);
            if (m % l_minSize != 0 || n % l_minSize != 0 || k % l_minSize != 0) {
                int padded_m = getPaddedSize(m, l_minSize);
                int padded_n = getPaddedSize(n, l_minSize);
                int padded_k = getPaddedSize(k, l_minSize);
                int paddedLda = getPaddedSize(lda, l_minSize);
                int paddedLdb = getPaddedSize(ldb, l_minSize);
                int paddedLdc = getPaddedSize(ldc, l_minSize);
                l_status = l_gemmPtr->addGEMMOp(A, B, C, C, padded_m, padded_n, padded_k, paddedLda, paddedLdb,
                                                paddedLdc, paddedLdc, 1, 0);
            } else {
                l_status = l_gemmPtr->addGEMMOp(A, B, C, C, m, n, k, lda, ldb, ldc, ldc, 1, 0);
            }
            return l_status;
        } else {
            return XFBLAS_STATUS_NOT_SUPPORTED;
        }
    } else {
        return XFBLAS_STATUS_NOT_SUPPORTED;
    }
}

/**
 * @brief This function performs the matrix-vector multiplication y = alpha*op(A) x+ beta*y
 * @param transa operation op(A) that is non- or (conj.) transpose
 * @param m number of rows in matrix A
 * @param n number of cols in matrix A
 * @param alpha scalar used for multiplication
 * @param A pointer to matrix A in the host memory
 * @param lda leading dimension of matirx A
 * @param x pointer to vector x in the host memory
 * @param incx stride between consecutive elements of x
 * @param beta scalar used for multiplication
 * @param y pointer to vector y in the host memory
 * @param incy stride between consecutive elements of y
 * @param kernelIndex index of kernel that is being used, default is 0
 * @param deviceIndex index of device that is being used, default is 0
 * @retval xfblasStatus_t 0 if the operation completed successfully
 * @retval xfblasStatus_t 1 if the library was not initialized
 * @retval xfblasStatus_t 3 if not all the matrices have FPGA devie memory allocated
 * @retval xfblasStatus_t 4 if the engine is not supported for now
 */
xfblasStatus_t xfblasGemv(xfblasOperation_t trans,
                          int m,
                          int n,
                          int alpha,
                          void* A,
                          int lda,
                          void* x,
                          int incx,
                          int beta,
                          void* y,
                          int incy,
                          unsigned int kernelIndex = 0,
                          unsigned int deviceIndex = 0) {
    if (ConfigDict::instance().m_dict.empty()) {
        return XFBLAS_STATUS_NOT_INITIALIZED;
    }
    if (ConfigDict::instance().m_dict["GEMX_runGemv"] == "1") {
        if (trans == XFBLAS_OP_N && alpha == 1 && beta == 1 && incx == 1 && incy == 1) {
            GEMVHost* l_gemvPtr =
                static_cast<GEMVHost*>(BLASHostHandle::instance().m_handlePtr[deviceIndex][kernelIndex].get());
            xfblasStatus_t l_status;
            int l_minSize = stoi(ConfigDict::instance().m_dict["minSize"]);
            if (m % l_minSize != 0 || n % l_minSize != 0) {
                int paddedM = getPaddedSize(m, l_minSize);
                int paddedN = getPaddedSize(n, l_minSize);
                int paddedLda = getPaddedSize(lda, l_minSize);
                l_status = l_gemvPtr->addGEMVOp(A, x, y, paddedM, paddedN, paddedLda);
            } else {
                l_status = l_gemvPtr->addGEMVOp(A, x, y, m, n, lda);
            }
            return l_status;
        } else {
            return XFBLAS_STATUS_NOT_SUPPORTED;
        }
    } else {
        return XFBLAS_STATUS_NOT_SUPPORTED;
    }
}

} // namespace blas

} // namespace xf

#endif