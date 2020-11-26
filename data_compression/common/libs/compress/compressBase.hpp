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
 * @file compressBase.hpp
 * @brief Header for base  host functionality
 *
 * This file is part of Vitis Data Compression Library host code.
 */
#ifndef _XFCOMPRESSION_COMPRESS_BASE_HPP_
#define _XFCOMPRESSION_COMPRESS_BASE_HPP_
#include "xcl2.hpp"

class compressBase {
   public:
    enum State { COMPRESS, DECOMPRESS, BOTH };

    // Xilinx compression
    virtual uint64_t xilCompress(uint8_t* in, uint8_t* out, uint64_t input_size) = 0;

    // Xilinx decompression
    virtual uint64_t xilDecompress(uint8_t* in, uint8_t* out, uint64_t input_size) = 0;
};

#endif // _XFCOMPRESSION_COMPRESS_BASE_HPP_
