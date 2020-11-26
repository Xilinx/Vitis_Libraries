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
/**
 * @file lz4Base.hpp
 * @brief Header for LZ4 Base functionality
 *
 * This file is part of Vitis Data Compression Library host code for lz4 compression.
 */

#ifndef _XFCOMPRESSION_LZ4_BASE_HPP_
#define _XFCOMPRESSION_LZ4_BASE_HPP_

#include <cassert>
#include <iomanip>
#include "xcl2.hpp"
#include "lz4_specs.hpp"
#include "compressBase.hpp"

/**
 *
 *  Maximum host buffer used to operate per kernel invocation
 */
const auto HOST_BUFFER_SIZE = (32 * 1024 * 1024);

/*
 * Default block size
 *
 */
const auto BLOCK_SIZE_IN_KB = 64;

/**
 * Maximum number of blocks based on host buffer size
 *
 */
const auto MAX_NUMBER_BLOCKS = (HOST_BUFFER_SIZE / (BLOCK_SIZE_IN_KB * 1024));

/**
 *  lz4Base class. Class containing methods for LZ4
 * compression and decompression to be executed on host side.
 */
class lz4Base : public compressBase {
   private:
    uint32_t m_BlockSizeInKb = 64;
    uint32_t m_HostBufferSize;
    uint64_t m_InputSize;
    std::chrono::system_clock::time_point total_start;
    std::chrono::system_clock::time_point total_end;

   public:
    /**
     * @brief Enable Profile
     * True: Prints the end to end throughput.
     * False: Prints Kernel throughput.
     */
    bool m_enableProfile;

    lz4Base(bool enable_profile) : m_enableProfile(enable_profile) {}

    /**
         * @brief Xilinx Compress
         *
         * @param in input byte sequence
         * @param out output byte sequence
         * @param actual_size input size
         */

    uint64_t xilCompress(uint8_t* in, uint8_t* out, uint64_t input_size) override;

    /**
         * @brief Xilinx Decompress
         *
         * @param in input byte sequence
         * @param out output byte sequence
         * @param compressed size
         */

    uint64_t xilDecompress(uint8_t* in, uint8_t* out, uint64_t input_size) override;

    /**
         * @brief Header Writer
         *
         * @param compress out stream
         */

    void writeHeader(uint8_t*& out);

    /**
         * @brief Header Reader
         *
         * @param Compress stream input header read
         */

    uint64_t readHeader(uint8_t*& in);

    /**
     * @brief Compress Engine
     *
     * @param in input byte sequence
     * @param out output byte sequence
     * @param actual_size input size
     * @param host_buffer_size host buffer size
     */
    virtual uint64_t compressEngine(uint8_t* in, uint8_t* out, uint64_t actual_size, uint32_t host_buffer_size) = 0;

    /**
     * @brief Decompress Engine.
     *
     * @param in input byte sequence
     * @param out output byte sequence
     * @param actual_size input size
     * @param original_size original size
     * @param host_buffer_size host buffer size
     */
    virtual uint64_t decompressEngine(
        uint8_t* in, uint8_t* out, uint64_t actual_size, uint64_t original_size, uint64_t host_buffer_size) = 0;
    lz4Base(){};
};
#endif // _XFCOMPRESSION_LZ4_BASE_HPP_
