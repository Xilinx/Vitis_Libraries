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

#include "zlib_decompress_quadcore_8kb.hpp"

const int c_historySize = LZ_MAX_OFFSET_LIMIT;

extern "C" {
void xilDecompress(hls::stream<ap_axiu<MULTIPLE_BYTES * 8, 0, 0, 0> >& inaxistreamd,
                   hls::stream<ap_axiu<MULTIPLE_BYTES * 8, TUSER_WIDTH, 0, 0> >& outaxistreamd) {
#ifdef FREE_RUNNING_KERNEL
#pragma HLS interface ap_ctrl_none port = return
#endif
#pragma HLS interface axis port = inaxistreamd
#pragma HLS interface axis port = outaxistreamd
#pragma HLS dataflow disable_start_propagation

    // Call for quadcore decompression
    xf::compression::inflateMultiCoreChkSum<NUM_CORE, DECODER_TYPE, MULTIPLE_BYTES, xf::compression::FileFormat::ZLIB,
                                            c_historySize, TUSER_WIDTH>(inaxistreamd, outaxistreamd);
}
}
