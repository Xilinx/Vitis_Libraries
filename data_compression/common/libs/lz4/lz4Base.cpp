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
#include "lz4Base.hpp"
#include "xxhash.h"

using namespace xf::compression;

void lz4Base::writeHeader(uint8_t*& out) {
    uint64_t input_size = m_InputSize;
    *(out++) = MAGIC_BYTE_1;
    *(out++) = MAGIC_BYTE_2;
    *(out++) = MAGIC_BYTE_3;
    *(out++) = MAGIC_BYTE_4;
    // FLG & BD bytes
    // --no-frame-crc flow
    // --content-size
    *(out++) = FLG_BYTE;

    uint8_t block_size_header = 0;
    // Default value 64K
    switch (m_BlockSizeInKb) {
        case 64:
            *(out++) = BSIZE_STD_64KB;
            block_size_header = BSIZE_STD_64KB;
            break;
        case 256:
            *(out++) = BSIZE_STD_256KB;
            block_size_header = BSIZE_STD_256KB;
            break;
        case 1024:
            *(out++) = BSIZE_STD_1024KB;
            block_size_header = BSIZE_STD_1024KB;
            break;
        case 4096:
            *(out++) = BSIZE_STD_4096KB;
            block_size_header = BSIZE_STD_4096KB;
            break;
        default:
            std::cout << "Invalid Block Size" << std::endl;
            break;
    }

    m_HostBufferSize = HOST_BUFFER_SIZE;

    if ((m_BlockSizeInKb * 1024) > input_size) m_HostBufferSize = m_BlockSizeInKb * 1024;

    uint8_t temp_buff[10] = {FLG_BYTE,         block_size_header, input_size,       input_size >> 8,  input_size >> 16,
                             input_size >> 24, input_size >> 32,  input_size >> 40, input_size >> 48, input_size >> 56};

    // xxhash is used to calculate hash value
    uint32_t xxh = XXH32(temp_buff, 10, 0);

    memcpy(out, (char*)&temp_buff[2], 8);
    out = out + 8;

    // Header CRC
    *(out++) = (uint8_t)(xxh >> 8);
}

uint64_t lz4Base::xilCompress(uint8_t* in, uint8_t* out, uint64_t input_size) {
    m_InputSize = input_size;

    // LZ4 header
    writeHeader(out);

    uint64_t enbytes;

    if (m_enableProfile) {
        total_start = std::chrono::high_resolution_clock::now();
    }
    // LZ4 multiple/single cu sequential version
    enbytes = compressEngine(in, out, m_InputSize, m_HostBufferSize);

    if (m_enableProfile) {
        total_end = std::chrono::high_resolution_clock::now();
        auto total_time_ns = std::chrono::duration<double, std::nano>(total_end - total_start);
        float throughput_in_mbps_1 = (float)input_size * 1000 / total_time_ns.count();
        std::cout << std::fixed << std::setprecision(2) << throughput_in_mbps_1;
    }
    // lz4 frame formatting
    out = out + enbytes;
    *(out++) = 0;
    *(out++) = 0;
    *(out++) = 0;
    *(out++) = 0;
    enbytes += 19;
    return enbytes;
}

uint64_t lz4Base::readHeader(uint8_t*& in) {
    // Read magic header 4 bytes
    char c = 0;
    int magic_hdr[] = {MAGIC_BYTE_1, MAGIC_BYTE_2, MAGIC_BYTE_3, MAGIC_BYTE_4};
    for (uint32_t i = 0; i < MAGIC_HEADER_SIZE; i++) {
        // inFile.get(v);
        c = *(in++);
        if (int(c) == magic_hdr[i])
            continue;
        else {
            std::cerr << "Problem with magic header " << c << " " << i << std::endl;
            exit(1);
        }
    }

    // Header Checksum
    c = *(in++);

    // Check if block size is 64 KB
    c = *(in++);

    switch (c) {
        case BSIZE_STD_64KB:
            m_BlockSizeInKb = 64;
            break;
        case BSIZE_STD_256KB:
            m_BlockSizeInKb = 256;
            break;
        case BSIZE_STD_1024KB:
            m_BlockSizeInKb = 1024;
            break;
        case BSIZE_STD_4096KB:
            m_BlockSizeInKb = 4096;
            break;
        default:
            std::cout << "Invalid Block Size" << std::endl;
            break;
    }

    // Original size
    uint64_t original_size = 0;

    memcpy(&original_size, in, 8);

    in = in + 9;
    c = *in;

    return original_size;
}

uint64_t lz4Base::xilDecompress(uint8_t* in, uint8_t* out, uint64_t input_size) {
    uint64_t original_size = readHeader(in);

    m_HostBufferSize = (m_BlockSizeInKb * 1024) * MAX_NUMBER_BLOCKS;

    if ((m_BlockSizeInKb * 1024) > original_size) m_HostBufferSize = m_BlockSizeInKb * 1024;

    uint64_t debytes;

    if (m_enableProfile) {
        total_start = std::chrono::high_resolution_clock::now();
    }

    // Decompression Engine multiple cus.
    debytes = decompressEngine(in, out, (input_size - 15), original_size, m_HostBufferSize);

    if (m_enableProfile) {
        total_end = std::chrono::high_resolution_clock::now();
        auto total_time_ns = std::chrono::duration<double, std::nano>(total_end - total_start);
        float throughput_in_mbps_1 = (float)input_size * 1000 / total_time_ns.count();
        std::cout << std::fixed << std::setprecision(2) << throughput_in_mbps_1;
    }
    return debytes;
}
