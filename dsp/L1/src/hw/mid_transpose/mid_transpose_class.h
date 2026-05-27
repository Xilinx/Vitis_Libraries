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
#include <complex>
#include <ap_fixed.h>
#include <hls_stream.h>
#include "ap_int.h"
#include "common.hpp"
#include "vss_fft_ifft_1d_common.hpp"
//#define __MID_TRANSPOSE_DEBUG__
using namespace xf::dsp::vss::common;
#ifndef POINT_SIZE_D1
#define POINT_SIZE_D1 1
#endif

template <unsigned int TP_NFFT, unsigned int TP_NSTREAM, unsigned int TP_SAMPLE_SIZE, unsigned int TP_POINT_SIZE_D1 = 1>
class midTransposeCls {
   public:
    static constexpr unsigned NBITS = TP_SAMPLE_SIZE; // restrict to always read and write only single sample per cycle
    static constexpr unsigned samplesPerRead = 1;
    int rdAddr[TP_NSTREAM];
    int rdBnk[TP_NSTREAM];
    int wrBnk[TP_NSTREAM];
    int wrAddr[TP_NSTREAM];

    static constexpr unsigned XDIM =
        (TP_POINT_SIZE_D1 == 1) ? fnPtSizeD1<TP_NFFT, modeAIEffts, TP_NSTREAM>() : TP_POINT_SIZE_D1;
    static constexpr unsigned YDIM = TP_NFFT / XDIM;
    typedef ap_uint<NBITS> TT_DATA;
    typedef ap_uint<NBITS / samplesPerRead> TT_SAMPLE;
    typedef hls::stream<TT_DATA> TT_STREAM;
    typedef hls::stream<TT_SAMPLE> TT_STREAM_IN;

    static constexpr int k_xDimCl = fnCeil<XDIM, TP_NSTREAM>();
    static constexpr int k_yDimCl = fnCeil<YDIM, TP_NSTREAM>();
    static constexpr unsigned k_numInputs = (XDIM * k_yDimCl) / TP_NSTREAM;
    static constexpr unsigned k_numOutputs = (YDIM * k_xDimCl) / TP_NSTREAM;
    static constexpr unsigned k_buffSize = (k_xDimCl * k_yDimCl) / TP_NSTREAM;

    static constexpr int k_totMatSize = k_xDimCl * k_yDimCl;
    static constexpr int yincrs = k_totMatSize / (TP_NSTREAM);
    static constexpr int xincrs = k_totMatSize / (TP_NSTREAM) / k_yDimCl;
    static constexpr unsigned k_ssrSquare = SSR * SSR;
    static constexpr unsigned int k_numssrSquares = TP_NFFT / k_ssrSquare;
    static constexpr unsigned int k_numxdimssrsquares = XDIM / k_ssrSquare;
    static constexpr unsigned int k_numXdim_Buff = k_buffSize / XDIM;
    void square_transpose(TT_STREAM_IN sig_in_st_buff[TP_NSTREAM], TT_STREAM_IN sig_o_st_buff[TP_NSTREAM]) {
#ifdef __MID_TRANSPOSE_DEBUG__
        FILE* fptr = fopen("/home/uvimalku/4mid_unpack_inputs.txt", "a");
#endif // __MID_TRANSPOSE_DEBUG__

        static TT_SAMPLE interBuff[TP_NSTREAM][k_buffSize];

        // separate accountancy variables for read and write sides
        static int rot_st = 0;
        static bool ping_st = 0;
        static int idx = 0;
        static int baseIdx = 0;
        static int wrCtr = 0;
        static int xbase = 0;
        static int ybase = 0;

        static int rot_ld = 0;
        static bool ping_ld = 1;
        static int rdIdx = 0;
        static int rdBaseIdx = 0;
        static int rdCtr = 0;
        static int rdxbase = 0;
        static int rdybase = 0;

        int totIdx;
        int totIdxRd;

        TT_SAMPLE in[TP_NSTREAM];
        TT_SAMPLE out[TP_NSTREAM];
        TT_SAMPLE tmp[TP_NSTREAM];

        static bool full = 0;
        static bool wrEn;
        static bool rdEn;
        static int numFramesIn;
        static int numFramesOut;
        static bool frameArrived;

        for (int count = 0; count < TP_NFFT / TP_NSTREAM; count++) {
#pragma HLS PIPELINE II = 1
            bool dataIsAvailable = !sig_in_st_buff[0].empty();

            if (wrEn) // assuming that data arrives at all input streams in the same cycle
            {
                for (int ss = 0; ss < TP_NSTREAM; ss++) {
                    in[ss] = sig_in_st_buff[ss].read();
                }
                if (ping_st == 0) {
                    totIdx = baseIdx + idx;
                    for (int ss = 0; ss < TP_NSTREAM; ss++) {
                        interBuff[ss][totIdx] = in[wrBnk[ss]];
                    }
                    rot_st = idx + 1 == XDIM ? 0 : (rot_st) == (TP_NSTREAM - 1) ? 0 : rot_st + 1;
                    baseIdx = idx + 1 == XDIM ? baseIdx + k_xDimCl == (k_buffSize) ? 0 : baseIdx + k_xDimCl : baseIdx;
                    idx = idx + 1 == XDIM ? 0 : idx + 1;
                    for (int ss = 0; ss < TP_NSTREAM; ss++) {
                        wrBnk[ss] =
                            (ss - rot_st + TP_NSTREAM) > (TP_NSTREAM - 1) ? ss - rot_st : (ss - rot_st + TP_NSTREAM);
                    }
                } else if (ping_st == 1) {
                    totIdx = xbase + ybase;
                    for (int ss = 0; ss < TP_NSTREAM; ss++) {
                        interBuff[ss][totIdx + wrBnk[ss]] = in[wrBnk[ss]];
                    }
                    xbase = (idx + 1 == XDIM && ybase == (yincrs - k_xDimCl))
                                ? xbase == (k_xDimCl - TP_NSTREAM) ? 0 : xbase + TP_NSTREAM
                                : xbase;
                    ybase =
                        idx + 1 == XDIM
                            ? 0
                            : rot_st == (TP_NSTREAM - 1) ? ybase == (yincrs - k_xDimCl) ? 0 : ybase + k_xDimCl : ybase;
                    rot_st = idx + 1 == XDIM ? 0 : (rot_st) == (TP_NSTREAM - 1) ? 0 : rot_st + 1;
                    idx = idx + 1 == XDIM ? 0 : idx + 1;
                    for (int ss = 0; ss < TP_NSTREAM; ss++) {
                        wrBnk[ss] =
                            (ss - rot_st + TP_NSTREAM) > (TP_NSTREAM - 1) ? ss - rot_st : (ss - rot_st + TP_NSTREAM);
                    }
                }
                if (wrCtr == (k_numInputs - 1)) {
                    ping_st = 1 - ping_st;
                }
                numFramesIn = (wrCtr == k_numInputs - 1) ? (numFramesIn + 1 == 3 ? 0 : numFramesIn + 1)
                                                         : numFramesIn; // TODO: magic number 2  // at any point you
                                                                        // could have written only 1 more frame than you
                                                                        // have read
                wrCtr = (wrCtr == k_numInputs - 1) ? 0 : wrCtr + 1;
            }
            if (rdEn) {
                if (ping_ld == 0) {
                    totIdxRd = rdBaseIdx + rdIdx;
                    for (int ss = 0; ss < TP_NSTREAM; ss++) {
                        tmp[ss] = interBuff[ss][totIdxRd];
                    }
                    for (int ss = 0; ss < TP_NSTREAM; ss++) {
                        out[ss] = tmp[rdBnk[ss]];
                    }
                    rdBaseIdx =
                        rdIdx + 1 == XDIM ? rdBaseIdx + k_xDimCl == (k_buffSize) ? 0 : rdBaseIdx + k_xDimCl : rdBaseIdx;
                    rot_ld = rdIdx + 1 == XDIM ? 0 : (rot_ld) == (TP_NSTREAM - 1) ? 0 : rot_ld + 1;
                    rdIdx = rdIdx + 1 == XDIM ? 0 : rdIdx + 1;
                    for (int ss = 0; ss < TP_NSTREAM; ss++) {
                        rdBnk[ss] = (ss + rot_ld) > (TP_NSTREAM - 1) ? ss + rot_ld - TP_NSTREAM : ss + rot_ld;
                    }
                } else if (ping_ld == 1) {
                    totIdxRd = rdxbase + rdybase;
                    for (int ss = 0; ss < TP_NSTREAM; ss++) {
                        tmp[ss] = interBuff[ss][totIdxRd + rdAddr[ss]];
                    }
                    for (int ss = 0; ss < TP_NSTREAM; ss++) {
                        out[ss] = tmp[rdBnk[ss]];
                    }
                    rdxbase = (rdIdx + 1 == XDIM && rdybase == (yincrs - k_xDimCl))
                                  ? rdxbase == (k_xDimCl - TP_NSTREAM) ? 0 : rdxbase + TP_NSTREAM
                                  : rdxbase;
                    rdybase = rdIdx + 1 == XDIM
                                  ? 0
                                  : rot_ld == (TP_NSTREAM - 1) ? rdybase == (yincrs - k_xDimCl) ? 0 : rdybase + k_xDimCl
                                                               : rdybase;
                    rot_ld = rdIdx + 1 == XDIM ? 0 : (rot_ld) == (TP_NSTREAM - 1) ? 0 : rot_ld + 1;
                    rdIdx = rdIdx + 1 == XDIM ? 0 : rdIdx + 1;
                    for (int ss = 0; ss < TP_NSTREAM; ss++) {
                        rdAddr[ss] =
                            (ss - rot_ld + TP_NSTREAM) > (TP_NSTREAM - 1) ? ss - rot_ld : (ss - rot_ld + TP_NSTREAM);
                        rdBnk[ss] = (ss + rot_ld) > (TP_NSTREAM - 1) ? ss + rot_ld - TP_NSTREAM : ss + rot_ld;
                    }
                }
                for (int ss = 0; ss < TP_NSTREAM; ss++) {
                    sig_o_st_buff[ss].write(out[ss]);
                }

                if (rdCtr == (k_numOutputs - 1)) {
                    ping_ld = 1 - ping_ld;
                }
                numFramesOut = (rdCtr == k_numOutputs - 1) ? (numFramesOut + 1 == 3 ? 0 : numFramesOut + 1)
                                                           : numFramesOut; // TODO: this is 3 since at any point the
                                                                           // maximum difference between number of
                                                                           // inFrames and outFrames is 1
                rdCtr = (rdCtr == k_numOutputs - 1) ? 0 : rdCtr + 1;
            }
            frameArrived = numFramesIn != numFramesOut;
            rdEn = frameArrived;
            full = (wrCtr == rdCtr && frameArrived);
            wrEn = (!full && dataIsAvailable);
        }
    }

    void rect_transpose(TT_STREAM_IN sig_in_st_buff[TP_NSTREAM], TT_STREAM_IN sig_o_st_buff[TP_NSTREAM]) {
        static TT_SAMPLE interBuff[TP_NSTREAM][k_buffSize];

        // separate accountancy variables for read and write sides
        static int rot_st = 0;
        static int idx = 0;
        static int baseIdx = 0;
        static int wrCtr = 0;
        static int xbase = 0;
        static int ybase = 0;

        static int rot_ld = 0;
        static int rdIdx = 0;
        static int rdBaseIdx = 0;
        static int rdCtr = 0;
        static int rdxbase = 0;
        static int rdybase = 0;
        static int intJump = 0;
        static int rdintJump = 1;

        static int curXdim = SSR;
        static int rdcurXdim = XDIM; // >= k_totMatSize/TP_NSTREAM ? (XDIM)/k_numssrSquares : XDIM;
        int totIdx;
        int totIdxRd;

        TT_SAMPLE in[TP_NSTREAM];
        TT_SAMPLE out[TP_NSTREAM];
        TT_SAMPLE tmp[TP_NSTREAM];

        static bool full = 0;
        static bool wrEn;
        static bool rdEn;
        static int numFramesIn;
        static int numFramesOut;
        static bool frameArrived;

        for (int count = 0; count < TP_NFFT / TP_NSTREAM; count++) {
#pragma HLS PIPELINE II = 1
            if (count == 0) {
                for (int ss = 0; ss < TP_NSTREAM; ss++) {
                    printf("\nbank %d = ", ss);
                    for (int w = 0; w < k_buffSize; w++) {
                        printf(" %d , %d ", interBuff[ss][w] >> 16, interBuff[ss][w] % (1 << 15));
                    }
                }
            }
            bool dataIsAvailable = !sig_in_st_buff[0].empty();

            if (wrEn) // assuming that data arrives at all input streams in the same cycle
            {
                for (int ss = 0; ss < TP_NSTREAM; ss++) {
                    in[ss] = sig_in_st_buff[ss].read();
                    printf("read from stream %d = [%d, %d]\n", ss, in[ss] >> 16, in[ss] % (1 << 15));
                }
                totIdx = xbase + ybase;
                for (int ss = 0; ss < TP_NSTREAM; ss++) {
                    interBuff[ss][totIdx + wrAddr[ss]] = in[wrBnk[ss]];
                    printf("buff[%d][%d] = [%d, %d] wrBnk = %d wrAddr = %d\n", ss, totIdx + wrAddr[ss],
                           in[wrBnk[ss]] >> 16, in[wrBnk[ss]] % (1 << 15), wrBnk[ss], wrAddr[ss]);
                }
                xbase = (rot_st == (TP_NSTREAM - 1) && ybase == (yincrs - curXdim))
                            ? xbase == (curXdim - TP_NSTREAM) ? 0 : xbase + TP_NSTREAM
                            : xbase;
                ybase = rot_st == (TP_NSTREAM - 1) ? ybase == (yincrs - curXdim) ? 0 : ybase + curXdim : ybase;
                rot_st = idx + 1 == XDIM ? 0 : (rot_st) == (TP_NSTREAM - 1) ? 0 : rot_st + 1;
                idx = idx + 1 == XDIM ? 0 : idx + 1;
                printf(" xbase = %d ybase = %d curXdim = %d\n", xbase, ybase, curXdim);
                if (wrCtr == k_numInputs - 1) {
                    // curXdim = curXdim * (XDIM/SSR);
                    ////fprintf(fptr, "curxdim cur = %d\n", curXdim);
                    // if (curXdim >= k_totMatSize/(TP_NSTREAM)){
                    //    curXdim = curXdim / (k_numssrSquares);
                    //    if(curXdim >= k_totMatSize/TP_NSTREAM){
                    //        curXdim = curXdim / k_numssrSquares;
                    //    }
                    ////fprintf(fptr, "curxdim cur 2 = %d\n", curXdim);
                    //}
                    if ((curXdim / k_numXdim_Buff) < (TP_NSTREAM)) {
                        curXdim = (curXdim * XDIM) / TP_NSTREAM;
                        // if(rdcurXdim >= k_totMatSize/TP_NSTREAM){
                        //    rdcurXdim = rdcurXdim / k_numssrSquares;
                        //}
                        // fprintf(lptr, "rdcurXdim cur 2 = %d\n", rdcurXdim);
                    } else {
                        curXdim = (curXdim / (k_numXdim_Buff));
                    }
                    intJump = 1 - intJump;
                }
                for (int ss = 0; ss < TP_NSTREAM; ss++) {
                    wrBnk[ss] =
                        (ss - rot_st + TP_NSTREAM) > (TP_NSTREAM - 1) ? ss - rot_st : (ss - rot_st + TP_NSTREAM);
                    wrAddr[ss] =
                        intJump == 0
                            ? rot_st
                            : ((ss - rot_st + TP_NSTREAM) > (TP_NSTREAM - 1)) ? ss - rot_st : ss - rot_st + TP_NSTREAM;
                }
                numFramesIn = (wrCtr == k_numInputs - 1) ? (numFramesIn + 1 == 3 ? 0 : numFramesIn + 1)
                                                         : numFramesIn; // TODO: magic number 2  // at any point you
                                                                        // could have written only 1 more frame than you
                                                                        // have read
                wrCtr = (wrCtr == k_numInputs - 1) ? 0 : wrCtr + 1;
            }
            if (rdEn) {
                totIdx = rdxbase + rdybase;
                for (int ss = 0; ss < TP_NSTREAM; ss++) {
                    tmp[ss] = interBuff[ss][totIdx + rdAddr[ss]];
                    printf("beat = %d read from location [%d][%d]\n", rot_ld, ss, totIdx + rdAddr[ss]);
                }
                for (int ss = 0; ss < TP_NSTREAM; ss++) {
                    out[ss] = tmp[rdBnk[ss]];
                }
                rdxbase = (rot_ld == (TP_NSTREAM - 1) && rdybase == (yincrs - rdcurXdim))
                              ? rdxbase == (rdcurXdim - TP_NSTREAM) ? 0 : rdxbase + TP_NSTREAM
                              : rdxbase;
                rdybase =
                    rot_ld == (TP_NSTREAM - 1) ? rdybase == (yincrs - rdcurXdim) ? 0 : rdybase + rdcurXdim : rdybase;
                rot_ld = (rot_ld) == (TP_NSTREAM - 1) ? 0 : rot_ld + 1;
                printf("rdxbase = %d, rdybase = %d rdcurxdim = %d\n", rdxbase, rdybase, rdcurXdim);
                for (int ss = 0; ss < TP_NSTREAM; ss++) {
                    sig_o_st_buff[ss].write(out[ss]);
                }

                if (rdCtr == (TP_NFFT / TP_NSTREAM) - 1) {
                    rdintJump = 1 - rdintJump;
                    // rdcurXdim = rdcurXdim * SSR*k_numxdimssrsquares;
                    printf("rdcurXdim cur = %d\n", rdcurXdim);
                    // if(rdCur)
                    if ((rdcurXdim / k_numXdim_Buff) < (TP_NSTREAM)) {
                        rdcurXdim = (rdcurXdim * XDIM) / TP_NSTREAM;
                        // if(rdcurXdim >= k_totMatSize/TP_NSTREAM){
                        //    rdcurXdim = rdcurXdim / k_numssrSquares;
                        //}
                        // fprintf(lptr, "rdcurXdim cur 2 = %d\n", rdcurXdim);
                    } else {
                        rdcurXdim = (rdcurXdim / (k_numXdim_Buff));
                    }
                }
                for (int ss = 0; ss < TP_NSTREAM; ss++) {
                    rdBnk[ss] = (ss + rot_ld) > (TP_NSTREAM - 1) ? ss + rot_ld - TP_NSTREAM : (ss + rot_ld);
                    rdAddr[ss] =
                        rdintJump == 0
                            ? rot_ld
                            : ((ss - rot_ld + TP_NSTREAM) > (TP_NSTREAM - 1)) ? ss - rot_ld : ss - rot_ld + TP_NSTREAM;
                }
                numFramesOut = (rdCtr == k_numOutputs - 1) ? (numFramesOut + 1 == 3 ? 0 : numFramesOut + 1)
                                                           : numFramesOut; // TODO: this is 3 since at any point the
                                                                           // maximum difference between number of
                                                                           // inFrames and outFrames is 1
                rdCtr = (rdCtr == (XDIM * YDIM) / SSR - 1) ? 0 : rdCtr + 1;
            }
            frameArrived = numFramesIn != numFramesOut;
            rdEn = frameArrived;
            full = (wrCtr == rdCtr && frameArrived);
            wrEn = (!full && dataIsAvailable);
        }
    }

    void mid_transpose_top(TT_STREAM sig_i[TP_NSTREAM], TT_STREAM sig_o[TP_NSTREAM]) {
#pragma HLS DATAFLOW
        if
            constexpr(XDIM == YDIM) { square_transpose(sig_i, sig_o); }
        else {
            rect_transpose(sig_i, sig_o);
        }
    }
    midTransposeCls() {
        for (int ss = 0; ss < TP_NSTREAM; ss++) {
            wrAddr[ss] = 0;
            wrBnk[ss] = ss;
            rdBnk[ss] = ss;
            rdAddr[ss] = ss;
        }
    }
};
