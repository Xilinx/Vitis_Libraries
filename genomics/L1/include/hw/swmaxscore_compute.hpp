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
#include "assert.h"
#include "sw.h"
#include <ap_int.h>
#include <stdio.h>
#include <string.h>
#include "hls_stream.h"
#include <stdint.h>

namespace xf {
namespace genomics {

typedef ap_uint<2> uint2_t;
typedef ap_uint<1> uint1_t;

typedef struct _pe {
    short d;
    short p;
} pe;

void initPE(pe* pex) {
#pragma HLS PIPELINE II = 1
    for (int i = 0; i < MAXPE; i++) {
        pex[i].d = 0;
        pex[i].p = 0;
    }
}

#ifdef _COMPUTE_FULL_MATRIX
short localMat[MAXROW][MAXCOL];
static short colIter = 0;
#endif

void updatePE(pe* pex, uint2_t d, uint2_t q, short n, short nw, short w, short r, short c) {
#pragma HLS PIPELINE II = 1
    short max = 0;
    short match = (d == q) ? MATCH : MISS_MATCH;
    short x1 = nw + match;
    short t1 = (x1 > max) ? x1 : max;
    short x2 = w + GAP;
    short t2 = (x2 > t1) ? x2 : t1;
    short x3 = n + GAP;
    max = (x3 > t2) ? x3 : t2;
    pex->p = max;
    pex->d = n;

#ifdef _COMPUTE_FULL_MATRIX
    localMat[r][colIter * MAXPE + c] = max;
#endif
}

void executePE(short r, uint2_t rowValue, short c, pe* pex, pe* ppex, hls::stream<ap_uint<2> >& refStream) {
#pragma HLS PIPELINE II = 1
    short nw, w, n;
    if (r == 0) {
        n = 0;
        nw = 0;
    } else {
        n = pex->p;
        nw = ppex->d;
    }

    w = ppex->p;
    uint2_t d1 = refStream.read();
    uint2_t q1 = rowValue;
    updatePE(pex, d1, q1, n, nw, w, r, c);
}

void executeFirstPE(short r, uint2_t rowValue, short c, pe* p, hls::stream<ap_uint<2> >& refStream, short nw, short w) {
#pragma HLS PIPELINE II = 1
    short n = (r == 0) ? 0 : p->p;
    uint2_t d1 = refStream.read();
    uint2_t q1 = rowValue;
    updatePE(p, d1, q1, n, nw, w, r, c);
}

template <int FACTOR>
void swCoreB(hls::stream<ap_uint<2> > refStream[MAXPE],
             hls::stream<ap_uint<2> >& readStream,
             hls::stream<bool>& swprocessDone,
             hls::stream<ap_uint<49> >& maxScoreStream) {
#pragma HLS INLINE
    pe myPE[MAXPE];
    short iterB[MAXROW];
#pragma HLS ARRAY_PARTITION variable = myPE dim = 0 complete
#pragma HLS BIND_STORAGE variable = iterB type = RAM_S2P impl = LUTRAM
    short stripes = MAXCOL / MAXPE;
    assert(stripes <= (MAXCOL + MAXPE - 1) / MAXPE);

    short rows = MAXROW;
    short maxc = MINVAL;
    short maxv = MINVAL;
    short maxr = MINVAL;

    for (bool done = swprocessDone.read(); done == false; done = swprocessDone.read()) {
    colLoop:
        for (short stripe = 0; stripe < stripes; stripe++) {
#ifdef _COMPUTE_FULL_MATRIX
            colIter = stripe;
#endif
            short w = 0; // Initial condition at the start of a row
            initPE(myPE);

        rowLoop:
            for (int loop = 0; loop < rows; ++loop) {
#pragma HLS PIPELINE II = 1
                short rowmaxv = MINVAL;
                short rowmaxpe = 0;
                ap_uint<2> rowValue = readStream.read();
            PELoop:
                for (int i = 0; i < MAXPE; i++) {
#pragma HLS INLINE recursive
                    if (i == 0) {
                        short nw = w;
                        w = (stripe == 0) ? 0 : iterB[loop];
                        executeFirstPE(loop, rowValue, i, &myPE[i], refStream[i], nw, w);
                    } else {
                        executePE(loop, rowValue, i, &myPE[i], &myPE[i - 1], refStream[i]);
                    }

                    if (i == MAXPE - 1) {
                        iterB[loop] = myPE[i].p;
                    }

                    if (myPE[i].p > rowmaxv) {
                        rowmaxv = myPE[i].p;
                        rowmaxpe = i;
                    }
                }

                if (rowmaxv > maxv) {
                    maxv = rowmaxv;
                    maxc = rowmaxpe + stripe * MAXPE; // log2(MAXPE);
                    maxr = loop;
                }
            }
        }
        ap_uint<49> maxScoreVal;
        maxScoreVal.range(15, 0) = maxr;
        maxScoreVal.range(31, 16) = maxc;
        maxScoreVal.range(47, 32) = maxv;
        maxScoreVal.range(48, 48) = 0;

        maxScoreStream << maxScoreVal;
    }
    ap_uint<49> maxScoreVal = 0;
    maxScoreVal.range(48, 48) = 1;
    maxScoreStream << maxScoreVal;
}

/*Only columns*/
void swSystolicBlocking(hls::stream<ap_uint<2> > refStream[MAXPE],
                        hls::stream<ap_uint<2> >& readStream,
                        hls::stream<bool>& swprocessDone,
                        hls::stream<ap_uint<49> >& outStream) {
#pragma HLS INLINE
    swCoreB<MAXPE>(refStream, readStream, swprocessDone, outStream);
}

void sw(hls::stream<ap_uint<2> > refStream[MAXPE],
        hls::stream<ap_uint<2> >& readStream,
        hls::stream<bool>& swprocessDone,
        hls::stream<ap_uint<49> >& outStream) {
#pragma HLS INLINE off
    swSystolicBlocking(refStream, readStream, swprocessDone, outStream);
}

template <int DATAWIDTH, int READSIZE, int FACTOR>
void downReadStream(hls::stream<ap_uint<DATAWIDTH> >& inReadStream,
                    hls::stream<bool>& processDone,
                    hls::stream<ap_uint<2> >& downSizeReadStream,
                    hls::stream<bool>& swprocessDone) {
#pragma HLS INLINE off
    const uint16_t numIter = MAXCOL / MAXPE;
    const uint32_t c_index = DATAWIDTH / 2;
    ap_uint<2> localBuffer[MAXROW];
#pragma HLS ARRAY_PARTITION variable = localBuffer cyclic factor = FACTOR

    for (bool done = processDone.read(); done == false; done = processDone.read()) {
        swprocessDone << done;
    downSizerRow:
        for (int i = 0; i < READSIZE; i++) {
#pragma HLS PIPELINE II = 1
            ap_uint<32> inValue = inReadStream.read();
            for (int j = 0; j < c_index; j++) {
                localBuffer[c_index * i + j] = (inValue & (3 << (2 * j))) >> (2 * j);
            }
        }

        for (uint32_t i = 0; i < numIter; i++) {
        bufferToStream_row:
            for (uint32_t j = 0; j < MAXROW; j++) {
#pragma HLS PIPELINE II = 1
                downSizeReadStream << localBuffer[j];
            }
        }
    }
    swprocessDone << 1;
}

template <int DATAWIDTH, int REFSIZE, int FACTOR>
void downRefStream(hls::stream<ap_uint<DATAWIDTH> >& inRefStream,
                   hls::stream<bool>& processDone,
                   hls::stream<ap_uint<2> > downSizeRefStream[MAXPE]) {
#pragma HLS INLINE off
    const uint16_t numIter = MAXCOL / MAXPE;
    const uint32_t c_index = DATAWIDTH / 2;
    ap_uint<2> localBuffer[MAXCOL];
#pragma HLS ARRAY_PARTITION variable = localBuffer cyclic factor = FACTOR

    for (bool done = processDone.read(); done == false; done = processDone.read()) {
    downSizerRow:
        for (int i = 0; i < REFSIZE; i++) {
#pragma HLS PIPELINE II = 1
            ap_uint<32> inValue = inRefStream.read();
            for (int j = 0; j < c_index; j++) {
                localBuffer[c_index * i + j] = (inValue & (3 << (2 * j))) >> (2 * j);
            }
        }

        for (uint32_t k = 0; k < numIter; k++) {
        bufferToStream_col:
            for (uint32_t i = 0; i < MAXROW; i++) {
#pragma HLS PIPELINE II = 1
            bufferToStream_col_unroll:
                for (uint32_t j = 0; j < MAXPE; j++) {
                    downSizeRefStream[j] << localBuffer[k * MAXPE + j];
                }
            }
        }
    }
}

/**
 * @brief Smithwaterman module computes the max score.
 *
 * @tparam DATAWIDTH input data width
 * @tparam FACTOR number of process engines
 *
 * @param inRefStream input reference data
 * @param refprocessDone reference end of stream
 * @param inReadStream input read data
 * @param readprocessDone read end of stream
 */
template <int DATAWIDTH, int FACTOR>
void swInt(hls::stream<ap_uint<DATAWIDTH> >& inRefStream,
           hls::stream<bool>& refprocessDone,
           hls::stream<ap_uint<DATAWIDTH> >& inReadStream,
           hls::stream<bool>& readprocessDone,
           hls::stream<ap_uint<49> >& outStream) {
    const uint32_t c_readFactor = DATAWIDTH / 2;
    const uint32_t c_streamDepth = MAXROW * (MAXCOL / MAXPE);
    hls::stream<ap_uint<2> > downSizeRefStream[MAXPE];
    hls::stream<ap_uint<2> > downSizeReadStream("readStream");
    hls::stream<bool> swprocessDone("swprocessDone");
#pragma HLS STREAM variable = downSizeRefStream depth = 64
#pragma HLS STREAM variable = downSizeReadStream depth = 64
#pragma HLS STREAM variable = swprocessDone depth = 32

#pragma HLS dataflow
    downReadStream<DATAWIDTH, MAXROW / c_readFactor, FACTOR>(inReadStream, readprocessDone, downSizeReadStream,
                                                             swprocessDone);
    downRefStream<DATAWIDTH, MAXCOL / c_readFactor, FACTOR>(inRefStream, refprocessDone, downSizeRefStream);
    sw(downSizeRefStream, downSizeReadStream, swprocessDone, outStream);
}
}
}
