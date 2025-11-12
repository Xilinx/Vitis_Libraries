/*
 * Copyright (C) 2019-2022, Xilinx, Inc.
 * Copyright (C) 2022-2025, Advanced Micro Devices, Inc.
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
#include "ap_int.h"
#include <hls_stream.h>
template <unsigned int IN_DATA_WIDTH, unsigned int NSTREAM_IN, unsigned int NSTREAM_OUT>
class axisSplitJoin {
    static constexpr unsigned outDataWidth = (IN_DATA_WIDTH * NSTREAM_IN) / NSTREAM_OUT;
    typedef ap_uint<IN_DATA_WIDTH> TT_DATA_IN;
    typedef ap_uint<outDataWidth> TT_DATA_OUT;

   public:
    void stream_splitter(hls::stream<TT_DATA_IN> sig_i[NSTREAM_IN], hls::stream<TT_DATA_OUT> sig_o[NSTREAM_OUT]) {
#pragma HLS PIPELINE II = 1
        for (int ss = 0; ss < NSTREAM_IN; ss++) {
            TT_DATA_IN in = sig_i[ss].read();
            TT_DATA_OUT tmp;
            for (int samp = 0; samp < (NSTREAM_OUT / NSTREAM_IN); samp++) {
                tmp = in.range((samp + 1) * outDataWidth - 1, samp * outDataWidth);
                sig_o[ss + samp * (NSTREAM_IN)].write(tmp);
            }
        }
    }

    void stream_joiner(hls::stream<TT_DATA_IN> sig_i[NSTREAM_IN], hls::stream<TT_DATA_OUT> sig_o[NSTREAM_OUT]) {
#pragma HLS PIPELINE II = 1
        for (int ss = 0; ss < NSTREAM_OUT; ss++) {
            TT_DATA_IN in[NSTREAM_IN / NSTREAM_OUT];
            TT_DATA_OUT outSamp;
            for (int samp = 0; samp < NSTREAM_IN / NSTREAM_OUT; samp++) {
                in[samp] = sig_i[ss + (NSTREAM_OUT)*samp].read();
                outSamp((samp + 1) * IN_DATA_WIDTH - 1, samp * IN_DATA_WIDTH) = in[samp];
            }
            sig_o[ss].write(outSamp);
        }
    }
};