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
#ifndef IFFT_BACK_TRANSPOSE_CLASS_H
#define IFFT_BACK_TRANSPOSE_CLASS_H

#include <complex>
#include <ap_fixed.h>
#include <hls_stream.h>
#include <hls_streamofblocks.h>
//#define __BACK_TRANSPOSE_DEBUG__

namespace back_transpose_simple {

template <int NFFT, int NSTREAM_IN>
class backTransposeSimpleCls {
   public:
    static constexpr unsigned int modePLffts() { return 1; }

    static constexpr unsigned int modeAIEffts() { return 0; }
    template <unsigned int len, unsigned int rnd>
    static constexpr unsigned int fnCeil() {
        return (len + rnd - 1) / rnd * rnd;
    }

    template <unsigned int TP_POINT_SIZE, unsigned int TP_VSS_MODE, unsigned int NUM_STREAM>
    static constexpr unsigned int fnPtSizeD1() {
        return TP_POINT_SIZE / NUM_STREAM;
    }
    static constexpr unsigned SAMPLES_PER_READ = 2;
    static constexpr unsigned NSTREAM_OUT = NSTREAM_IN * SAMPLES_PER_READ;
    static constexpr unsigned ptSizeCeil = fnCeil<NFFT, NSTREAM_OUT>(); // 65

    static constexpr unsigned ptSizeD1 = fnPtSizeD1<NFFT, modePLffts(), SSR>(); // 32
    static constexpr unsigned ptSizeD2 = NFFT / ptSizeD1;                       // 32
    static constexpr unsigned ptSizeD1Ceil = fnCeil<ptSizeD1, NSTREAM_OUT>();   // 32

    static constexpr unsigned DEPTH = (NFFT / ptSizeD1);     // Depth of each bank // 16
    static constexpr unsigned NROW = ptSizeD1 / NSTREAM_OUT; // # of rows of transforms per bank // 16

    static constexpr unsigned NBITS_IN = 128;  // Size of PLIO bus on PL side @ 312.5 MHz
    static constexpr unsigned NBITS_OUT = 128; // Size of PLIO bus on PL side @ 312.5 MHz

    int rdAddrLut[NSTREAM_OUT];
    int rdBnkLut[NSTREAM_OUT];
    int wrBnkLut0[NSTREAM_OUT];

    static constexpr unsigned int numLoadsPtSize = ptSizeD1 / NSTREAM_OUT; //
    static constexpr unsigned int numLoads = NFFT / NSTREAM_IN;            // numLoadsPtSize * ptSizeD2;   // 256*4
    static constexpr unsigned numStores = NFFT / NSTREAM_IN;               // numLoads;  // 7
    static constexpr unsigned numRows = fnCeil<ptSizeD2, NSTREAM_OUT>() / NSTREAM_OUT;
    typedef ap_uint<NBITS_IN> TT_DATA;                       // Equals two 'cint32' samples
    typedef ap_uint<NBITS_OUT / SAMPLES_PER_READ> TT_SAMPLE; // Samples are 'cint32'
    typedef hls::stream<TT_DATA> TT_STREAM_IN;
    typedef hls::stream<TT_DATA> TT_STREAM_OUT;
    typedef TT_SAMPLE buff_t[NSTREAM_OUT][NFFT / NSTREAM_OUT]; // 4 * 1024

    void unpack_inputs(TT_STREAM_OUT sig_int[NSTREAM_IN], hls::stream_of_blocks<buff_t>& inter_buff) {
#ifdef __BACK_TRANSPOSE_DEBUG__
        FILE* fptr = fopen("/home/uvimalku/4debug_unpack_inputs.txt", "a");
#endif //  __BACK_TRANSPOSE_DEBUG__

        hls::write_lock<buff_t> buff_in(inter_buff);
        TT_SAMPLE trans_i0[NSTREAM_OUT];
#pragma HLS array_partition variable = trans_i0 dim = 1
        int idx = 0;
    BUFF_LOOP:
        for (int ii = 0; ii < numStores / SAMPLES_PER_READ; ii++) {
#pragma HLS unroll factor2
#pragma HLS PIPELINE II = 2
        READ:
            for (int ss = 0; ss < NSTREAM_IN; ss++) {
#ifdef __BACK_TRANSPOSE_DEBUG__
                fprintf(fptr, "write into banks %d and %d \n", wrBnkLut0[2 * ss], wrBnkLut0[2 * ss + 1]);
#endif //  __BACK_TRANSPOSE_DEBUG__
                (trans_i0[2 * ss + 1], trans_i0[2 * ss]) = sig_int[ss].read();
            }

        STORE:
            for (int ss = 0; ss < NSTREAM_OUT; ss++) {
                buff_in[ss][idx] = trans_i0[wrBnkLut0[ss]];
            }
        MOD:
            for (int ss = 0; ss < NSTREAM_OUT; ss++) {
                wrBnkLut0[ss] = (wrBnkLut0[ss] < 2) ? (wrBnkLut0[ss] + NSTREAM_OUT) - 2 : wrBnkLut0[ss] - 2;
            }
            idx = idx + 1;
        }
#ifdef __BACK_TRANSPOSE_DEBUG__
        for (int ss = 0; ss < NSTREAM_OUT; ss++) {
            for (int pp = 0; pp < numLoads; pp++) {
                printf("[%d, %d] ", (buff_in[ss][pp] >> 32), (buff_in[ss][pp] % (1 << 31)));
            }
            printf("\n");
        }
        fclose(fptr);
#endif //  __BACK_TRANSPOSE_DEBUG__
    }

    void ifft_load_buff(hls::stream_of_blocks<buff_t>& inter_buff, TT_STREAM_OUT sig_o[NSTREAM_IN]) {
#ifdef __BACK_TRANSPOSE_DEBUG__
        FILE* buff_data2 = fopen("/home/uvimalku/4debug_load_buff.txt", "a");
#endif //  __BACK_TRANSPOSE_DEBUG__
        TT_SAMPLE trans_o0[NSTREAM_OUT];
#pragma HLS array_partition variable = trans_o0 dim = 1
        hls::read_lock<buff_t> buff_out(inter_buff);
        int colCtr = 0;
        int rowCtr = 0;
        int tmp = 0;
    WR_LOOP:
        for (int cc = 0; cc < numLoads / SAMPLES_PER_READ; cc++) { // 2
                                                                   // #pragma HLS unroll factor=2
#pragma HLS PIPELINE II = 1
            for (int ss = 0; ss < NSTREAM_OUT; ss++) { // 5
                trans_o0[ss] = buff_out[ss][rdAddrLut[ss]];
#ifdef __BACK_TRANSPOSE_DEBUG__
                fprintf(buff_data2, "rd from add = [%d][%d] \n", ss, rdAddrLut[ss]);
#endif //  __BACK_TRANSPOSE_DEBUG__
            }
            for (int ss = 0; ss < NSTREAM_IN; ss++) {
                sig_o[ss].write((trans_o0[rdBnkLut[ss + NSTREAM_IN]], trans_o0[rdBnkLut[ss]]));
            }
            if (colCtr + 1 == numLoads / (NSTREAM_OUT)) {
                for (int ss = 0; ss < NSTREAM_OUT; ss++) {
                    rdBnkLut[ss] = (rdBnkLut[ss] + 2) & (NSTREAM_OUT - 1);
                    rdAddrLut[ss] = ((ss / 2 + NSTREAM_OUT - (1 + tmp)) & (NSTREAM_OUT / 2 - 1));
                }
                tmp = tmp + 1;
                rowCtr = rowCtr + 1;
                colCtr = 0;
            } else {
                for (int ss = 0; ss < NSTREAM_OUT; ss++) {
                    rdAddrLut[ss] = rdAddrLut[ss] + NSTREAM_OUT / 2;
                }
                colCtr = colCtr + 1;
            }
        }
#ifdef __BACK_TRANSPOSE_DEBUG__
        fclose(buff_data2);
#endif //  __BACK_TRANSPOSE_DEBUG__
    }

    void back_transpose_simple_top(TT_STREAM_IN sig_i[NSTREAM_IN], TT_STREAM_OUT sig_o[NSTREAM_OUT]) {
        hls::stream_of_blocks<buff_t> inter_buff;
#pragma HLS array_partition variable = inter_buff dim = 1 type = complete
#pragma HLS bind_storage variable = inter_buff type = RAM_T2P impl = bram
#pragma HLS DATAFLOW
        unpack_inputs(sig_i, inter_buff);
        ifft_load_buff(inter_buff, sig_o);
    }

    backTransposeSimpleCls() {
        for (int ss = 0; ss < NSTREAM_OUT; ss++) {
            rdAddrLut[ss] = ss / 2;
            rdBnkLut[ss] = ss;
#ifdef __BACK_TRANSPOSE_DEBUG__
            printf("1. stream= %d alculating bank: %d address : %d\n", ss, rdBnkLut[ss], rdAddrLut[ss]);
#endif //  __BACK_TRANSPOSE_DEBUG__
        }

        for (int ss = 0; ss < NSTREAM_OUT; ss++) {
            wrBnkLut0[ss] = ss;
#ifdef __BACK_TRANSPOSE_DEBUG__
            printf("wrBnkLut0[%d] = %d\n", ss, wrBnkLut0[ss]);
#endif //  __BACK_TRANSPOSE_DEBUG__
        }
    }
};
};

#endif
