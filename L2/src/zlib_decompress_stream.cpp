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

#include "zlib_decompress_stream.hpp"

extern "C" {
void xilDecompressStream(uint32_t input_size,
                         hls::stream<ap_axiu<kGMemDWidth, 0, 0, 0> >& inaxistreamd,
                         hls::stream<ap_axiu<kGMemDWidth, 0, 0, 0> >& outaxistreamd,
                         hls::stream<ap_axiu<64, 0, 0, 0> >& sizestreamd) {
#pragma HLS INTERFACE s_axilite port = input_size bundle = control
#pragma HLS interface axis port = inaxistreamd
#pragma HLS interface axis port = outaxistreamd
#pragma HLS interface axis port = sizestreamd
#pragma HLS INTERFACE s_axilite port = return bundle = control
    // Call for decompression
    xf::compression::inflate<DECODER_TYPE, kGMemDWidth, HISTORY_SIZE, LOW_OFFSET>(inaxistreamd, outaxistreamd,
                                                                                  sizestreamd, input_size);
}
}
