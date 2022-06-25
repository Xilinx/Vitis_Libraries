/*
 * Copyright 2021 Xilinx, Inc.
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
/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
#ifndef _JPGCODER__HLS_HH_
#define _JPGCODER__HLS_HH_
#include "XAcc_jpegdecoder.hpp"
#include "XAcc_jfifparser.hpp"
#include "XAcc_common.hpp"
#include "multi_cu.hpp"
#include <string>
template <typename T>
T* aligned_alloc(std::size_t num) {
    void* ptr = nullptr;
    if (posix_memalign(&ptr, 4096, num * sizeof(T))) throw std::bad_alloc();
    return reinterpret_cast<T*>(ptr);
}
bool hls_decode_jpeg_kernel(std::string xclbin_path,
                            int filecnt,
                            std::vector<uint8_t*> datatoDDR,
                            std::vector<int> jpgSize,
                            std::vector<struct_arith>& arith,
                            std::vector<uint8_t*> res,
                            std::vector<uint32_t>& left,
                            std::vector<uint32_t>& rst);
bool hls_decode_jpeg_kernel(ap_uint<AXI_WIDTH>* datatoDDR, int size, struct_arith& arith, uint8_t* res);

#endif
