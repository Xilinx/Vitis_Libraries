/*
 * (c) Copyright 2020 Xilinx, Inc. All rights reserved.
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
#ifndef _XFCOMPRESSION_ZSTD_HPP_
#define _XFCOMPRESSION_ZSTD_HPP_

#pragma once

#include <iomanip>
#include <iostream>
#include <stdint.h>
#include <vector>
#include <math.h>
#include <time.h>
#include <string>
#include <fstream>
#include "xcl2.hpp"
#include <thread>

#pragma once

// zstd maximum cr
#define MAX_CR 20

int validate(std::string& inFile_name, std::string& outFile_name);

uint64_t get_file_size(std::ifstream& file);

class xil_zstd {
   public:
    int init(const std::string& binaryFile);
    int release();
    uint64_t decompressSeq(uint8_t* in, uint8_t* out, uint64_t actual_size);
    uint64_t decompress_file(std::string& inFile_name, std::string& outFile_name, uint64_t input_size);
    // Binary flow compress/decompress
    xil_zstd(const std::string& binaryFile, uint8_t max_cr = MAX_CR, uint8_t device_id = 0);
    ~xil_zstd();

   private:
    uint8_t m_deviceid;
    const uint32_t m_minfilesize = 200;

    // Max cr
    uint8_t m_max_cr;

    cl::Program* m_program;
    cl::Context* m_context;
    cl::CommandQueue* m_q_dec;
    cl::CommandQueue* m_q_rd;
    cl::CommandQueue* m_q_rdd;
    cl::CommandQueue* m_q_wr;
    cl::CommandQueue* m_q_wrd;

    // Decompress Kernel Declaration
    cl::Kernel* decompress_kernel;
    cl::Kernel* data_writer_kernel;
    cl::Kernel* data_reader_kernel;

    // Kernel names
    std::string decompress_kernel_name = "xilZstdDecompressStream";
    std::string data_writer_kernel_name = "xilZlibDmWriter";
    std::string data_reader_kernel_name = "xilZlibDmReader";
};
#endif // _XFCOMPRESSION_ZSTD_HPP_
