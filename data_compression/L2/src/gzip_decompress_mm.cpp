/*
 * (c) Copyright 2019-2022 Xilinx, Inc. All rights reserved.
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

#include "gzip_decompress_mm.hpp"

const int c_historySize = LZ_MAX_OFFSET_LIMIT;

extern "C" {

void xilDecompress(const ap_uint<GMEM_DWIDTH>* in,
                   ap_uint<GMEM_DWIDTH>* out,
                   uint32_t* encodedSize,
                   uint32_t inputSize) {
    constexpr int c_gmem0_width = GMEM_DWIDTH;
#pragma HLS INTERFACE m_axi port = in offset = slave bundle = gmem max_widen_bitwidth = \
    c_gmem0_width max_read_burst_length = 64 num_read_outstanding = 8
#pragma HLS INTERFACE m_axi port = out offset = slave bundle = gmem max_write_burst_length = \
    64 num_write_outstanding = 8
#pragma HLS INTERFACE m_axi port = encodedSize offset = slave bundle = gmem
#pragma HLS INTERFACE s_axilite port = inputSize
#pragma HLS INTERFACE s_axilite port = return
#pragma HLS INTERFACE ap_ctrl_chain port = return
#pragma HLS dataflow
    constexpr int c_decoderType = (int)HUFFMAN_TYPE;

    // Inflate MM Call
    xf::compression::inflateMultiByteMM<GMEM_DWIDTH, GMEM_BURST_SIZE, c_decoderType, MULTIPLE_BYTES,
                                        xf::compression::FileFormat::BOTH, LL_MODEL, HISTORY_SIZE>(in, out, encodedSize,
                                                                                                   inputSize);
}
}
