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
 
#include "handle.hpp"
#include "gemm_host.hpp"
#include "gemv_host.hpp"
#include "helpers/funcs/fcn_host.hpp"
#include "api.hpp"

using namespace xf::blas;

bool xfblasCreate(char* xclbin, char* engineName ,char* logFile,
                 unsigned int kernelNumber, unsigned int deviceIndex) {

    int l_err = 0;
    shared_ptr<XFpga> l_xFpga(new XFpga(xclbin, logFile, &l_err, deviceIndex));
    XFpgaHold::instance().m_xFpgaPtr[deviceIndex] = l_xFpga;

    if (l_err != 0) {
        return false;
    }
    xfblasStatus_t l_status = XFBLAS_STATUS_SUCCESS;
    
    if (strcmp(engineName,"Gemm")==0) {
        for (unsigned int i = 0; i < kernelNumber; i++) {
            BLASHostHandle::instance().m_handlePtr[deviceIndex].push_back(
                shared_ptr<BLASHost>(new GEMMHost(xclbin, logFile, &l_status, i, deviceIndex)));
        }
        return true;
    } else if (strcmp(engineName,"Gemv")==0) { 
        for (unsigned int i = 0; i < kernelNumber; i++) {
            BLASHostHandle::instance().m_handlePtr[deviceIndex].push_back(
                shared_ptr<BLASHost>(new GEMVHost(xclbin, logFile, &l_status, i, deviceIndex)));
        }
        return true;    
    } else if (strcmp(engineName,"Fcn")==0) { 
        for (unsigned int i = 0; i < kernelNumber; i++) {
            BLASHostHandle::instance().m_handlePtr[deviceIndex].push_back(
                shared_ptr<BLASHost>(new FCNHost(xclbin, logFile, &l_status, i, deviceIndex)));
        }
        return true;   
    } else {
        return false;
    }
}


bool xfblasSend(void* A, unsigned long long numElem, int elemSize, unsigned int kernelIndex, unsigned int deviceIndex) {
      unsigned long long l_bufSize = numElem * elemSize;
      xfblasStatus_t l_status = BLASHostHandle::instance().m_handlePtr[deviceIndex][kernelIndex]->allocMatRestricted(A, A, l_bufSize);
      if (l_status != XFBLAS_STATUS_SUCCESS){
        return false;
      }
      l_status = BLASHostHandle::instance().m_handlePtr[deviceIndex][kernelIndex]->setMatToFPGARestricted(A);
      if (l_status != XFBLAS_STATUS_SUCCESS){
        return false;
      }
      return true;

}

bool xfblasGet(void* A, unsigned int kernelIndex, unsigned int deviceIndex) {
    xfblasStatus_t l_status = BLASHostHandle::instance().m_handlePtr[deviceIndex][kernelIndex]->execute();
    l_status = BLASHostHandle::instance().m_handlePtr[deviceIndex][kernelIndex]->getMatRestricted(A, A);
    if (l_status != XFBLAS_STATUS_SUCCESS){
        return false;
    }
    return true;
}

bool xfblasGemm(int m,int n,int k,
                int alpha,
                void* A,int lda,
                void* B,int ldb,
                int beta,
                void* C,int ldc,
                unsigned int kernelIndex,
                unsigned int deviceIndex) {
        if (alpha == 1 && beta == 1) {
            GEMMHost* l_gemmPtr = static_cast<GEMMHost*>(BLASHostHandle::instance().m_handlePtr[deviceIndex][kernelIndex].get());
            xfblasStatus_t l_status = l_gemmPtr->addGEMMOp(A, B, C, C, m, n, k, lda, ldb, ldc, ldc, 1, 0);
            if (l_status != XFBLAS_STATUS_SUCCESS){
                return false;
            }
            return true;
        } else {
            return false;
        }

}


bool xfblasGemv(int m,int n,
                int alpha,
                void* A,int lda,
                void* x,int incx,
                int beta,
                void* y,int incy,
                unsigned int kernelIndex,
                unsigned int deviceIndex) {

        if (alpha == 1 && beta == 1 && incx == 1 && incy == 1) {
            GEMVHost* l_gemvPtr = static_cast<GEMVHost*>(BLASHostHandle::instance().m_handlePtr[deviceIndex][kernelIndex].get());
            xfblasStatus_t l_status = l_gemvPtr->addGEMVOp(A, x, y, m, n, lda);
            if (l_status != XFBLAS_STATUS_SUCCESS){
                return false;
            }
            return true;
        } else {
            return false;
        }
    
}


bool xfblasFcn(int m,int n,int k,
                int alpha,
                void* A,int lda,
                void* B,int ldb,
                int beta,
                void* C,int ldc,
                void* X, int ldx,
                int p_postScale,
                int p_postShift,
                short p_preluScale, 
                short p_preluAlpha,
                unsigned int kernelIndex,
                unsigned int deviceIndex) {
        if (alpha == 1 && beta == 1) {
            FCNHost* l_fcnPtr = static_cast<FCNHost*>(BLASHostHandle::instance().m_handlePtr[deviceIndex][kernelIndex].get());
            xfblasStatus_t l_status = l_fcnPtr->addFCNOp(A, B, C, X, m, n, k, lda, ldb, ldc, ldx, p_postScale, p_postShift,p_preluScale,p_preluAlpha);
            if (l_status != XFBLAS_STATUS_SUCCESS){
                return false;
            }
            return true;
        } else {
            return false;
        }

}




