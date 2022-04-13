/*
 * (c) Copyright 2022 Xilinx, Inc. All rights reserved.
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
 *
 */
#ifndef OCL_HPP
#define OCL_HPP

#include "host_types.hpp"
#include "CL/cl.h"
#include <CL/cl_ext.h>
#define BWT_SIZE 775451201
#include "xcl2.hpp"

namespace xf {
namespace genomics {
class xilSmem {
   public:
    xilSmem(uint32_t* bwt, uint64_t* bwt_para, uint64_t bwt_size, int batch_size, char* binary, bool runflow = false)
        : m_bwt{bwt},
          m_bwtpara{bwt_para},
          m_bwtSize{bwt_size},
          m_batch_size{batch_size},
          m_binary{binary},
          m_runflow{runflow} {}
    int init_fpga_cpp(std::string& binfile);
    void ocl_init(void);
    void ocl_invoke_seq(uint8_t* seq,
                        uint8_t* seq_len,
                        bwtintv_t* mem_output,
                        int* mem_num,
                        int batch_size,
                        double* cur_kernel_time,
                        uint64_t bwt_size);
    void ocl_invoke_overlap(uint8_t* seq,
                            uint8_t* seq_len,
                            bwtintv_t* mem_output,
                            int* mem_num,
                            int batch_size,
                            double* cur_kernel_time,
                            uint64_t bwt_size);
    int smem_ocl(uint8_t* seq, uint8_t* seq_len, bwtintv_t* mem_output, int* mem_num, double kernel_time[COMPUTE_UNIT]);
    bool ocl_cmp(char* fname_golden, bwtintv_v& mem, int* mem_num);
    ~xilSmem();

   private:
    uint32_t* m_bwt = nullptr;
    uint64_t* m_bwtpara = nullptr;
    uint64_t m_bwtSize = 0;
    std::string m_binary;
    int m_batch_size = 0;
    bool m_runflow;

    bool is_seq(void) { return m_runflow; }
    // New declarations
    cl::Device m_device;
    cl_context_properties m_platform;
    cl::Context* m_context = nullptr;
    cl::Program* m_program = nullptr;
    cl::CommandQueue* m_comqueue = nullptr;
    cl::Kernel* m_kernel_smem[COMPUTE_UNIT];
    cl::Kernel* m_kernel_dram[COMPUTE_UNIT];
    cl::Buffer* m_bwt_buffer[COMPUTE_UNIT] = {nullptr};
    cl::Buffer* m_bwt_para_buffer[COMPUTE_UNIT] = {nullptr};
    cl::Buffer* m_seq_buffer[COMPUTE_UNIT] = {nullptr};
    cl::Buffer* m_seq_len_buffer[COMPUTE_UNIT] = {nullptr};
    cl::Buffer* m_mem_buffer[COMPUTE_UNIT] = {nullptr};
    cl::Buffer* m_mem_num_buffer[COMPUTE_UNIT] = {nullptr};
};
}
}
#endif
