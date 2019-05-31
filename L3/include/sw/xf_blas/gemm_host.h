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

#ifndef XF_BLAS_GEMM_HOST_H
#define XF_BLAS_GEMM_HOST_H


#include "host.h"

namespace xf {
namespace linear_algebra {
namespace blas {

class GemmArgs: public BLASArgs {
public:
    virtual ~GemmArgs() {
    }
    GemmArgs() = delete;
    GemmArgs(unsigned int p_aOffset, unsigned int p_bOffset,
            unsigned int p_cOffset, unsigned int p_xOffset, unsigned int p_m, unsigned int p_k,
            unsigned int p_n, unsigned int p_lda, unsigned int p_ldb,
            unsigned int p_ldc, unsigned int p_ldx, int p_postScale, int p_postShift) :
                m_GemmArgs( { int(OpGemm),  p_aOffset, p_bOffset, p_cOffset, p_xOffset, p_m, p_k,
        p_n, p_lda, p_ldb, p_ldc, p_ldx, 0, 0, 0, 0 }) {
        m_GemmArgs.m_postScaleVal = (p_postScale << 8) | (p_postShift & 0x000000ff);
    }
    size_t sizeInBytes() {
        return sizeof(m_GemmArgs);
    }
    char *asByteArray() {
        return reinterpret_cast<char*>(&m_GemmArgs);
    }
protected:
    struct {
        int m_optype;
        unsigned int m_aOffset, m_bOffset, m_cOffset, m_xOffset, m_m, m_k, m_n,
        m_lda, m_ldb, m_ldc, m_ldx;
        int m_postScaleVal;
        int m_empty[3];
    } m_GemmArgs;
};

template<typename T>
class GEMMHost : public BLASHost<T> {
public:
    GEMMHost() = delete;
    virtual ~GEMMHost() {}
    GEMMHost(const GEMMHost<T> &) = delete;
    GEMMHost(const string & p_xclbin, const string & p_kernelName, xfblasStatus_t* p_status) : BLASHost<T> ( p_xclbin, p_kernelName, p_status) {}
    
  
    virtual xfblasStatus_t addGEMMOp(const T & p_a, const T & p_b, const T &p_c, const T & p_bias, unsigned int p_m, unsigned int p_k, unsigned int p_n, unsigned int p_lda, unsigned int p_ldb, unsigned int p_ldc, unsigned int p_ldx, int p_postScale, int p_postShift) {
        if (this->m_hostMat.find(p_a) == this->m_hostMat.end()
                || this->m_hostMat.find(p_b) == this->m_hostMat.end()
                || this->m_hostMat.find(p_c) == this->m_hostMat.end()
                || this->m_hostMat.find(p_bias) == this->m_hostMat.end()) {
            return XFBLAS_STATUS_ALLOC_FAILED;
        }
        unsigned long long l_aOff = 0, l_bOff = 0, l_cOff = 0, l_xOff = 0;

        cl_int l_err = xclGetMemObjDeviceAddress(this->m_devHandle[p_a].get(),XHost<T>::m_fpga->m_Device.get(),sizeof(unsigned long long), &l_aOff);
        l_err = xclGetMemObjDeviceAddress(this->m_devHandle[p_b].get(),XHost<T>::m_fpga->m_Device.get(),sizeof(unsigned long long), &l_bOff);
        l_err = xclGetMemObjDeviceAddress(this->m_devHandle[p_c].get(),XHost<T>::m_fpga->m_Device.get(),sizeof(unsigned long long), &l_cOff);
        l_err = xclGetMemObjDeviceAddress(this->m_devHandle[p_bias].get(),XHost<T>::m_fpga->m_Device.get(),sizeof(unsigned long long), &l_xOff);

        if (l_err != CL_SUCCESS){
            return XFBLAS_STATUS_NOT_INITIALIZED;
        }

        if (l_aOff <= this->m_ddrDeviceBaseAddr || l_bOff <= this->m_ddrDeviceBaseAddr || l_cOff <= this->m_ddrDeviceBaseAddr || l_xOff <= this->m_ddrDeviceBaseAddr) {
            return XFBLAS_STATUS_ALLOC_FAILED;  
        }

        l_aOff -= this->m_ddrDeviceBaseAddr;
        l_bOff -= this->m_ddrDeviceBaseAddr;
        l_cOff -= this->m_ddrDeviceBaseAddr;
        l_xOff -= this->m_ddrDeviceBaseAddr;

        if (l_aOff % this->PAGE_SIZE != 0 || l_bOff % this->PAGE_SIZE != 0 || l_cOff % this->PAGE_SIZE != 0 || l_xOff % this->PAGE_SIZE != 0) {
            return XFBLAS_STATUS_ALLOC_FAILED; 
        }

        l_aOff /= this->PAGE_SIZE;
        l_bOff /= this->PAGE_SIZE;
        l_cOff /= this->PAGE_SIZE;
        l_xOff /= this->PAGE_SIZE;

        GemmArgs l_gargs(l_aOff, l_bOff, l_cOff, l_xOff, p_m, p_k, p_n, p_lda, p_ldb, p_ldc, p_ldx, p_postScale, p_postShift);
        this->addInstr ( &l_gargs);
        return XFBLAS_STATUS_SUCCESS;
    }
  
  
};
  

}  
}
}





#endif
