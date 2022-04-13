/*
 * (c) Copyright 2022 Xilinx, Inc. All rights reserved.
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

/*
 * Systolic implementation of smith-waterman
*/
#include "sw.h"
#include "hls_stream.h"

#include "opencl_sw_maxscore_systolic.hpp"

template <int DATAWIDTH>
void smithwatermanMaxScore(hls::stream<ap_uint<DATAWIDTH> > refStream[NUMPACKED],
                           hls::stream<bool> refprocessDone[NUMPACKED],
                           hls::stream<ap_uint<DATAWIDTH> > readStream[NUMPACKED],
                           hls::stream<bool> readprocessDone[NUMPACKED],
                           hls::stream<ap_uint<49> > maxScoreStream[NUMPACKED]) {
/*instantiate NUMPACKED PE*/
swMaxScoreLoop:
    for (int i = 0; i < NUMPACKED; ++i) {
#pragma HLS UNROLL
        xf::genomics::swInt<DATAWIDTH, MAXPE>(refStream[i], refprocessDone[i], readStream[i], readprocessDone[i],
                                              maxScoreStream[i]);
    }
}

template <int DATAWIDTH>
void mm2s(ap_uint<DATAWIDTH>* input,
          hls::stream<ap_uint<DATAWIDTH> > refStream[NUMPACKED],
          hls::stream<bool> refprocessDone[NUMPACKED],
          hls::stream<ap_uint<DATAWIDTH> > readStream[NUMPACKED],
          hls::stream<bool> readprocessDone[NUMPACKED],
          int* size) {
    // memcpy(readRefPacked, (unsigned int *)(input + loop * PACKEDSZ * NUMPACKED), UINTSZ * PACKEDSZ * NUMPACKED);
    const uint32_t c_dataWidth = DATAWIDTH;
    const uint32_t c_index = PACKEDSZ_K / 3;
    uint32_t numIter = *size;
mm2s_numIter:
    for (uint32_t i = 0; i < numIter; i++) {
        uint32_t inputIdx = i * NUMPACKED * PACKEDSZ_K;
    mm2s_numpack:
        for (uint32_t j = 0; j < NUMPACKED; j++) {
            readprocessDone[j] << 0;
            refprocessDone[j] << 0;
            uint32_t inIdx = inputIdx + (j * PACKEDSZ_K);
        mm2s_packSize:
            for (uint32_t k = 0; k < PACKEDSZ_K; k++) {
#pragma HLS PIPELINE II = 1
                ap_uint<c_dataWidth> inValue = input[inIdx + k];
                if (k < c_index)
                    readStream[j] << inValue;
                else
                    refStream[j] << inValue;
            }
        }
    }

mm2sDone:
    for (uint16_t i = 0; i < NUMPACKED; i++) {
#pragma HLS PIPELINE II = 1
        readprocessDone[i] << 1;
        refprocessDone[i] << 1;
    }
}

void multiToSigleStream(hls::stream<ap_uint<49> > maxScoreStream[NUMPACKED],
                        hls::stream<ap_uint<48> >& outStream,
                        hls::stream<bool>& endOfStream) {
    // memcpy((unsigned int *)(output + 3 * NUMPACKED * loop), outbuf, sizeof(unsigned int) * 3 * NUMPACKED);
    ap_uint<NUMPACKED> is_pending;
    for (int i = 0; i < NUMPACKED; i++) {
#pragma HLS UNROLL
        is_pending.range(i, i) = 1;
    }

    while (is_pending) {
    singlemaxStream:
        for (uint32_t i = 0; i < NUMPACKED; i++) {
#pragma HLS PIPELINE II = 1
            if (!maxScoreStream[i].empty()) {
                ap_uint<49> inValue = maxScoreStream[i].read();
                if (inValue.range(48, 48) == 1) {
                    is_pending(i, i) = 0;
                } else {
                    outStream << inValue;
                    endOfStream << 0;
                    is_pending(i, i) = 1;
                }
            }
        }
    }
    outStream << 0;
    endOfStream << 1;
}

template <int DATAWIDTH>
void divideoutData(hls::stream<ap_uint<48> >& inStream,
                   hls::stream<bool>& inEoStream,
                   hls::stream<ap_uint<DATAWIDTH> >& outStream,
                   hls::stream<bool>& outEoStream) {
    const uint16_t factor = DATAWIDTH / 8;
    ap_uint<2 * DATAWIDTH> outValue;
    uint8_t idx = 0;
upSizeStream:
    for (bool eos = inEoStream.read(); eos == false; eos = inEoStream.read()) {
#pragma HLS PIPELINE II = 1
        ap_uint<48> inValue = inStream.read();
        outValue.range((idx + 6) * 8 - 1, idx * 8) = inValue;
        idx += 6;
        if (idx >= factor) {
            outStream << outValue.range(DATAWIDTH - 1, 0);
            outEoStream << 0;
            idx -= factor;
        }
    }
    if (idx >= factor) {
        outStream << outValue.range(DATAWIDTH - 1, 0);
        outEoStream << 0;
    }

    ap_uint<16> inValue = inStream.read();
    outEoStream << 1;
}

template <int DATAWIDTH>
void s2mm(hls::stream<ap_uint<DATAWIDTH> >& inStream, hls::stream<bool>& endOfStream, ap_uint<DATAWIDTH>* output) {
    uint32_t i = 0;
s2mm:
    for (bool eosFlag = endOfStream.read(); eosFlag == false; eosFlag = endOfStream.read()) {
#pragma HLS PIPELINE II = 1
        output[i++] = inStream.read();
    }
}

extern "C" {
void opencl_sw_maxscore(ap_uint<NUMPACKED * 2>* input, ap_uint<NUMPACKED * 2>* output, int* size) {
#pragma HLS INLINE off
#pragma HLS INTERFACE m_axi port = input offset = slave bundle = gmem
#pragma HLS INTERFACE m_axi port = output offset = slave bundle = gmem
#pragma HLS INTERFACE m_axi port = size offset = slave bundle = gmem
#pragma HLS INTERFACE s_axilite port = input bundle = control
#pragma HLS INTERFACE s_axilite port = output bundle = control
#pragma HLS INTERFACE s_axilite port = size bundle = control
#pragma HLS INTERFACE s_axilite port = return bundle = control

    const uint32_t c_inBufferDepth = PACKEDSZ_K * 2;
    const uint32_t c_outBufferDepth = 8;
    const uint32_t c_dataWidth = NUMPACKED * 2;
    hls::stream<ap_uint<c_dataWidth> > refStream[NUMPACKED];
    hls::stream<ap_uint<c_dataWidth> > readStream[NUMPACKED];
    hls::stream<bool> refprocessDone[NUMPACKED];
    hls::stream<bool> readprocessDone[NUMPACKED];
    hls::stream<ap_uint<49> > maxScoreStream[NUMPACKED];
    hls::stream<ap_uint<48> > singlemaxStream;
    hls::stream<bool> maxendOfStream;
    hls::stream<ap_uint<c_dataWidth> > outStream;
    hls::stream<bool> endOfStream;
#pragma HLS STREAM variable = refStream depth = 64
#pragma HLS STREAM variable = readStream depth = 64
#pragma HLS STREAM variable = refprocessDone depth = 32
#pragma HLS STREAM variable = readprocessDone depth = 32
#pragma HLS STREAM variable = maxScoreStream depth = 32
#pragma HLS STREAM variable = singlemaxStream depth = 32
#pragma HLS STREAM variable = maxendOfStream depth = 32
#pragma HLS STREAM variable = outStream depth = 32
#pragma HLS STREAM variable = endOfStream depth = 32

#pragma HLS dataflow
    mm2s<c_dataWidth>(input, refStream, refprocessDone, readStream, readprocessDone, size);
    smithwatermanMaxScore<c_dataWidth>(refStream, refprocessDone, readStream, readprocessDone, maxScoreStream);
    multiToSigleStream(maxScoreStream, singlemaxStream, maxendOfStream);
    divideoutData(singlemaxStream, maxendOfStream, outStream, endOfStream);
    s2mm<c_dataWidth>(outStream, endOfStream, output);

    return;
}
}
