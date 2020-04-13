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

#ifndef XF_BLAS_GEMM_HOST_HPP
#define XF_BLAS_GEMM_HOST_HPP

#include "handle.hpp"
#include "host.hpp"

namespace xf {

namespace blas {

class GemmArgs : public BLASArgs {
   public:
    virtual ~GemmArgs() {}
    GemmArgs() = delete;
    GemmArgs(unsigned int p_aOffset,
             unsigned int p_bOffset,
             unsigned int p_cOffset,
             unsigned int p_xOffset,
             unsigned int p_m,
             unsigned int p_k,
             unsigned int p_n,
             unsigned int p_lda,
             unsigned int p_ldb,
             unsigned int p_ldc,
             unsigned int p_ldx,
             int p_postScale,
             int p_postShift)
        : m_GemmArgs({int(OpGemm), p_aOffset, p_bOffset, p_cOffset, p_xOffset, p_m, p_k, p_n, p_lda, p_ldb, p_ldc,
                      p_ldx, 0, 0, 0, 0}) {
        m_GemmArgs.m_postScaleVal = (p_postScale << 8) | (p_postShift & 0x000000ff);
    }
    size_t sizeInBytes() { return sizeof(m_GemmArgs); }
    char* asByteArray() { return reinterpret_cast<char*>(&m_GemmArgs); }

   protected:
    struct {
        int m_optype;
        unsigned int m_aOffset, m_bOffset, m_cOffset, m_xOffset, m_m, m_k, m_n, m_lda, m_ldb, m_ldc, m_ldx;
        int m_postScaleVal;
        int m_empty[3];
    } m_GemmArgs;
};

class GEMMHost : public BLASHost {
   public:
    GEMMHost() = delete;
    virtual ~GEMMHost() {}
    GEMMHost(const GEMMHost&) = delete;
    GEMMHost(const char* p_xclbin, xfblasStatus_t* p_status, unsigned int p_kernelIndex, unsigned int p_deviceIndex)
        : BLASHost(p_xclbin, p_status, p_kernelIndex, p_deviceIndex) {}

    virtual xfblasStatus_t addGEMMOp(void* p_a,
                                     void* p_b,
                                     void* p_c,
                                     void* p_bias,
                                     unsigned int p_m,
                                     unsigned int p_n,
                                     unsigned int p_k,
                                     unsigned int p_lda,
                                     unsigned int p_ldb,
                                     unsigned int p_ldc,
                                     unsigned int p_ldx,
                                     int p_postScale,
                                     int p_postShift) {
        if (this->m_bufHandle.find(p_a) == this->m_bufHandle.end() ||
            this->m_bufHandle.find(p_b) == this->m_bufHandle.end() ||
            this->m_bufHandle.find(p_c) == this->m_bufHandle.end() ||
            this->m_bufHandle.find(p_bias) == this->m_bufHandle.end()) {
            return XFBLAS_STATUS_ALLOC_FAILED;
        }
        unsigned int handle_A, handle_B, handle_C, handle_bias;
        auto& l_devPtr = this->m_bufHandle;

        handle_A = l_devPtr[p_a];
        handle_B = l_devPtr[p_b];
        handle_C = l_devPtr[p_c];
        handle_bias = l_devPtr[p_bias];

        xclBOProperties p;
        uint64_t address_A = !xclGetBOProperties(this->m_fpga->m_handle, handle_A, &p) ? p.paddr : -1;
        uint64_t address_B = !xclGetBOProperties(this->m_fpga->m_handle, handle_B, &p) ? p.paddr : -1;
        uint64_t address_C = !xclGetBOProperties(this->m_fpga->m_handle, handle_C, &p) ? p.paddr : -1;
        uint64_t address_bias = !xclGetBOProperties(this->m_fpga->m_handle, handle_bias, &p) ? p.paddr : -1;

        unsigned long long l_aOff, l_bOff, l_cOff, l_xOff;
        l_aOff = (unsigned long long)address_A;
        l_bOff = (unsigned long long)address_B;
        l_cOff = (unsigned long long)address_C;
        l_xOff = (unsigned long long)address_bias;

        l_aOff -= this->m_fpga->m_baseAddress[this->m_cuIndex];
        l_bOff -= this->m_fpga->m_baseAddress[this->m_cuIndex];
        l_cOff -= this->m_fpga->m_baseAddress[this->m_cuIndex];
        l_xOff -= this->m_fpga->m_baseAddress[this->m_cuIndex];

        l_aOff /= this->PAGE_SIZE;
        l_bOff /= this->PAGE_SIZE;
        l_cOff /= this->PAGE_SIZE;
        l_xOff /= this->PAGE_SIZE;

        GemmArgs l_gargs(l_aOff, l_bOff, l_cOff, l_xOff, p_m, p_k, p_n, p_lda, p_ldb, p_ldc, p_ldx, p_postScale,
                         p_postShift);
        this->addInstr(&l_gargs);
        this->enableRun();

        return XFBLAS_STATUS_SUCCESS;
    }
};

} // namespace blas

} // namespace xf

#endif
