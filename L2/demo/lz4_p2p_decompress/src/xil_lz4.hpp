/*
 * (c) Copyright 2019 Xilinx, Inc. All rights reserved.
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
#ifndef _XFP2PDECOMPRESSION_XIL_LZ4_HPP_
#define _XFP2PDECOMPRESSION_XIL_LZ4_HPP_

#pragma once
#include "defns.h"

// Maximum compute units supported
#if (C_COMPUTE_UNIT > D_COMPUTE_UNIT)
#define MAX_COMPUTE_UNITS C_COMPUTE_UNIT
#else
#define MAX_COMPUTE_UNITS D_COMPUTE_UNIT
#endif

// Maximum host buffer used to operate
// per kernel invocation
#define HOST_BUFFER_SIZE (2 * 1024 * 1024)

// Default block size
#define BLOCK_SIZE_IN_KB 64

// Value below is used to associate with
// Overlapped buffers, ideally overlapped
// execution requires 2 resources per invocation
#define OVERLAP_BUF_COUNT 4

// Max Input buffer Size
#define MAX_IN_BUFFER_SIZE (128 * 1024 * 1024)

// Max Input Buffer Partitions
#define MAX_IN_BUFFER_PARTITION MAX_IN_BUFFER_SIZE / HOST_BUFFER_SIZE

// Maximum number of blocks based on host buffer size
#define MAX_NUMBER_BLOCKS (HOST_BUFFER_SIZE / (BLOCK_SIZE_IN_KB * 1024))

// Below are the codes as per LZ4 standard for
// various maximum block sizes supported.
#define BSIZE_STD_64KB 0x40
#define BSIZE_STD_256KB 0x50
#define BSIZE_STD_1024KB 0x60
#define BSIZE_STD_4096KB 0x70

// Maximum block sizes supported by LZ4
#define MAX_BSIZE_64KB (64 * 1024)
#define MAX_BSIZE_256KB (256 * 1024)
#define MAX_BSIZE_1024KB (1024 * 1024)
#define MAX_BSIZE_4096KB (4096 * 1024)

// This value is used to set
// uncompressed block size value
// 4th byte is always set to below
// and placed as uncompressed byte
#define NO_COMPRESS_BIT 128

// In case of uncompressed block
// Values below are used to set
// 3rd byte to following values
// w.r.t various maximum block sizes
// supported by standard
#define BSIZE_NCOMP_64 1
#define BSIZE_NCOMP_256 4
#define BSIZE_NCOMP_1024 16
#define BSIZE_NCOMP_4096 64

int validate(std::string& inFile_name, std::string& outFile_name);

static uint64_t get_file_size(std::ifstream& file) {
    file.seekg(0, file.end);
    uint64_t file_size = file.tellg();
    file.seekg(0, file.beg);
    return file_size;
}

class xfLz4 {
   public:
    uint64_t decompress_ssd_file(std::string& inFile_name, std::string& outFile_name);
    uint64_t p2p_inline_decompress(int, uint8_t* out, uint64_t actual_size, uint64_t original_size);
    uint64_t get_event_duration_ns(const cl::Event& event);
    void bufferExtensionAssignments(bool flow);
    xfLz4(const std::string& binaryFile);
    // Binary flow compress/decompress
    bool m_bin_flow;

    // Block Size
    uint32_t m_block_size_in_kb;
    uint32_t host_buffer_size;
    uint32_t max_num_blks;
    uint8_t* h_buf_in_p2p[MAX_IN_BUFFER_PARTITION];
    uint32_t buf_size_in_partitions;
    // Switch between FPGA/Standard flows
    bool m_switch_flow;

    // xfLz4();
    ~xfLz4();

   private:
    cl::Program* m_program;
    cl::Context* m_context;
    cl::CommandQueue* m_q[OVERLAP_BUF_COUNT];
    cl::Kernel* compress_kernel_lz4[C_COMPUTE_UNIT];
    cl::Kernel* decompress_kernel_lz4[D_COMPUTE_UNIT];
    cl::Kernel* unpacker_kernel_lz4;

    // Compression related
    std::vector<uint8_t, aligned_allocator<uint8_t> > h_buf_in[OVERLAP_BUF_COUNT];
    std::vector<uint8_t, aligned_allocator<uint8_t> > h_buf_out[OVERLAP_BUF_COUNT][MAX_COMPUTE_UNITS];
    std::vector<uint32_t, aligned_allocator<uint8_t> > h_blksize[OVERLAP_BUF_COUNT];
    std::vector<uint32_t, aligned_allocator<uint8_t> > h_compressSize[OVERLAP_BUF_COUNT];

    // Device buffers
    cl::Buffer* buffer_input[OVERLAP_BUF_COUNT];
    cl::Buffer* buffer_output[OVERLAP_BUF_COUNT][MAX_COMPUTE_UNITS];
    cl::Buffer* buffer_no_blocks[OVERLAP_BUF_COUNT];
    cl::Buffer* buffer_in_start_index[OVERLAP_BUF_COUNT];
    cl::Buffer* buffer_original_size[OVERLAP_BUF_COUNT];
    cl::Buffer* buffer_compressed_size[OVERLAP_BUF_COUNT];
    cl::Buffer* buffer_block_size[OVERLAP_BUF_COUNT];
    cl::Buffer* buffer_block_start_idx[OVERLAP_BUF_COUNT];
    cl::Buffer* buffer_no_blocks_per_cu[OVERLAP_BUF_COUNT];

    // Decompression related
    std::vector<uint32_t> m_blkSize[OVERLAP_BUF_COUNT][MAX_COMPUTE_UNITS];
    std::vector<uint32_t> m_compressSize[OVERLAP_BUF_COUNT][MAX_COMPUTE_UNITS];
    std::vector<bool> m_is_compressed[OVERLAP_BUF_COUNT][MAX_COMPUTE_UNITS];
    std::vector<uint32_t, aligned_allocator<uint8_t> > h_no_blocks[OVERLAP_BUF_COUNT];
    std::vector<uint32_t, aligned_allocator<uint8_t> > h_in_start_index[OVERLAP_BUF_COUNT];
    std::vector<uint32_t, aligned_allocator<uint8_t> > h_original_size[OVERLAP_BUF_COUNT];
    std::vector<uint32_t, aligned_allocator<uint8_t> > h_block_start_idx[OVERLAP_BUF_COUNT];
    std::vector<uint32_t, aligned_allocator<uint8_t> > h_no_blocks_per_cu[OVERLAP_BUF_COUNT];

    // DDR buffer extensions
    cl_mem_ext_ptr_t inExt[OVERLAP_BUF_COUNT];
    cl_mem_ext_ptr_t outExt[OVERLAP_BUF_COUNT][MAX_COMPUTE_UNITS];
    cl_mem_ext_ptr_t pbExt[OVERLAP_BUF_COUNT];
    cl_mem_ext_ptr_t siExt[OVERLAP_BUF_COUNT];
    cl_mem_ext_ptr_t osExt[OVERLAP_BUF_COUNT];
    cl_mem_ext_ptr_t csExt[OVERLAP_BUF_COUNT];
    cl_mem_ext_ptr_t bsExt[OVERLAP_BUF_COUNT];
    cl_mem_ext_ptr_t bsiExt[OVERLAP_BUF_COUNT];
    cl_mem_ext_ptr_t pbpcExt[OVERLAP_BUF_COUNT];

    // Kernel names
    std::vector<std::string> compress_kernel_names = {
        "xil_lz4_cu1", "xil_lz4_cu2", "xil_lz4_cu3", "xil_lz4_cu4",
        "xil_lz4_cu5", "xil_lz4_cu6", "xil_lz4_cu7", "xil_lz4_cu8",
    };

    std::vector<std::string> unpacker_kernel_names = {"xilLz4Unpacker"};

    std::vector<std::string> decompress_kernel_names = {"xilLz4P2PDecompress"};
    // DDR numbers
    std::vector<uint32_t> comp_ddr_nums = {XCL_MEM_DDR_BANK0};
    std::vector<uint32_t> decomp_ddr_nums = {XCL_MEM_DDR_BANK0};
};

#endif
