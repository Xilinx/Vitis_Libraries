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
#include "common.hpp"
//#define __BACK_TRANSPOSE_DEBUG__

using namespace xf::dsp::vss::common;
namespace back_transpose_simple {

template <int TP_NFFT, int TP_NSTREAM_EXT, int TP_SAMPLE_WIDTH>
class backTransposeSimpleCls {
   public:
    template <unsigned int tp_point_size, unsigned int tp_ssr>
    static constexpr unsigned int fnPtSizeD1() {
        return tp_point_size / tp_ssr;
    }
    static constexpr unsigned kNbitsIn = 128;  // Size of PLIO bus on PL side @ 312.5 MHz
    static constexpr unsigned kNbitsOut = 128; // Size of PLIO bus on PL side @ 312.5 MHz

    static constexpr unsigned kSamplesPerRead = kNbitsIn / TP_SAMPLE_WIDTH; // 4
    static constexpr unsigned kNstreamInt = TP_NSTREAM_EXT * kSamplesPerRead;

    static constexpr unsigned kPtSizeD1 = fnPtSizeD1<TP_NFFT, TP_NSTREAM_EXT>(); // 32
    static constexpr unsigned kPtSizeD2 = TP_NFFT / kPtSizeD1;                   // 32
    static constexpr unsigned kPtSizeD1Ceil = fnCeil<kPtSizeD1, kNstreamInt>();  // 32

    int rdAddrLut[kNstreamInt];
    int rdBnkLut[kNstreamInt];
    int wrBnkLut0[kNstreamInt];

    static constexpr unsigned int kNumLoads = TP_NFFT / TP_NSTREAM_EXT;
    static constexpr unsigned kNumStores = TP_NFFT / TP_NSTREAM_EXT; // kNumLoads;  // 7
    typedef ap_uint<kNbitsIn> t_data;                                // Equals two 'cint32' samples
    typedef ap_uint<kNbitsOut / kSamplesPerRead> t_sample;           // Samples are 'cint32'
    typedef hls::stream<t_data> t_stream_in;
    typedef hls::stream<t_data> t_stream_out;
    typedef t_sample buff_t[kNstreamInt][TP_NFFT / kNstreamInt]; // 4 * 1024

    void unpack_inputs(t_stream_out sig_int[TP_NSTREAM_EXT], hls::stream_of_blocks<buff_t>& inter_buff) {
#ifdef __BACK_TRANSPOSE_DEBUG__
        FILE* fptr = fopen("/home/uvimalku/4debug_unpack_inputs.txt", "a");
#endif //  __BACK_TRANSPOSE_DEBUG__

        hls::write_lock<buff_t> buff_in(inter_buff);
        t_sample trans_i0[kNstreamInt];
#pragma HLS array_partition variable = trans_i0 dim = 1
        int idx = 0;
    BUFF_LOOP:
        for (int ii = 0; ii < kNumStores / kSamplesPerRead; ii++) {
#pragma HLS unroll factor2
#pragma HLS PIPELINE II = 2
        READ:
            for (int ss = 0; ss < TP_NSTREAM_EXT; ss++) {
#ifdef __BACK_TRANSPOSE_DEBUG__
                for (int samp = 0; samp < kSamplesPerRead; samp++) {
                    printf("write into bank %d idx = %d\n", wrBnkLut0[kSamplesPerRead * ss + samp],
                           kSamplesPerRead * ss + samp);
                }
#endif //  __BACK_TRANSPOSE_DEBUG__
                t_data sig_int_data = sig_int[ss].read();
                for (int jj = 0; jj < kSamplesPerRead; jj++) {
#pragma HLS unroll
                    // print trans_i0
                    // printf( "write into bank %d from idx %d to %d\n", ss*kSamplesPerRead + jj, (jj *
                    // TP_SAMPLE_WIDTH)+ (TP_SAMPLE_WIDTH-1), (jj * TP_SAMPLE_WIDTH));
                    (trans_i0[ss * kSamplesPerRead + jj]) =
                        sig_int_data((jj * TP_SAMPLE_WIDTH) + (TP_SAMPLE_WIDTH - 1), (jj * TP_SAMPLE_WIDTH));
                }
            }

        STORE:
            for (int ss = 0; ss < kNstreamInt; ss++) {
                buff_in[ss][idx] = trans_i0[wrBnkLut0[ss]];
            }
        MOD:
            for (int ss = 0; ss < kNstreamInt; ss++) {
                wrBnkLut0[ss] = (wrBnkLut0[ss] < kSamplesPerRead) ? (wrBnkLut0[ss] + kNstreamInt) - kSamplesPerRead
                                                                  : wrBnkLut0[ss] - kSamplesPerRead;
            }
            idx = idx + 1;
        }
#ifdef __BACK_TRANSPOSE_DEBUG__
        for (int ss = 0; ss < kNstreamInt; ss++) {
            for (int pp = 0; pp < kNumLoads; pp++) {
                printf("[%d, %d] ", (buff_in[ss][pp] >> TP_SAMPLE_WIDTH / 2),
                       (buff_in[ss][pp] % (1 << (TP_SAMPLE_WIDTH / 2 - 1))));
            }
            printf("\n");
        }
        fclose(fptr);
#endif //  __BACK_TRANSPOSE_DEBUG__
    }

    void ifft_load_buff(hls::stream_of_blocks<buff_t>& inter_buff, t_stream_out sig_o[TP_NSTREAM_EXT]) {
#ifdef __BACK_TRANSPOSE_DEBUG__
        FILE* buff_data2 = fopen("/home/uvimalku/4debug_load_buff.txt", "a");
#endif //  __BACK_TRANSPOSE_DEBUG__
        t_sample trans_o0[kNstreamInt];
#pragma HLS array_partition variable = trans_o0 dim = 1
        hls::read_lock<buff_t> buff_out(inter_buff);
        int colCtr = 0;
        int rowCtr = 0;
        int tmp = 0;
    WR_LOOP:
        for (int cc = 0; cc < kNumLoads / kSamplesPerRead; cc++) { // 2
#pragma HLS unroll factor = 2
#pragma HLS PIPELINE II = 2
            for (int ss = 0; ss < kNstreamInt; ss++) { // 5
                trans_o0[ss] = buff_out[ss][rdAddrLut[ss]];
#ifdef __BACK_TRANSPOSE_DEBUG__
                fprintf(buff_data2, "rd from add = [%d][%d] \n", ss, rdAddrLut[ss]);
#endif //  __BACK_TRANSPOSE_DEBUG__
            }
            for (int ss = 0; ss < TP_NSTREAM_EXT; ss++) {
                t_data outDat;
                for (int samp = 0; samp < kSamplesPerRead; samp++) {
                    outDat(samp * TP_SAMPLE_WIDTH + TP_SAMPLE_WIDTH - 1, samp * TP_SAMPLE_WIDTH) =
                        trans_o0[rdBnkLut[ss + TP_NSTREAM_EXT * samp]];
                }
                sig_o[ss].write(outDat); // Write the output data
            }
            if (colCtr + 1 == kNumLoads / (kNstreamInt)) {
                for (int ss = 0; ss < kNstreamInt; ss++) {
                    rdBnkLut[ss] = (rdBnkLut[ss] + kSamplesPerRead) & (kNstreamInt - 1);
                    rdAddrLut[ss] =
                        ((ss / kSamplesPerRead + kNstreamInt - (1 + tmp)) & (kNstreamInt / kSamplesPerRead - 1));
                }
                tmp = tmp + 1;
                rowCtr = rowCtr + 1;
                colCtr = 0;
            } else {
                for (int ss = 0; ss < kNstreamInt; ss++) {
                    rdAddrLut[ss] = rdAddrLut[ss] + kNstreamInt / kSamplesPerRead;
                }
                colCtr = colCtr + 1;
            }
        }
#ifdef __BACK_TRANSPOSE_DEBUG__
        fclose(buff_data2);
#endif //  __BACK_TRANSPOSE_DEBUG__
    }

    void back_transpose_simple_top(t_stream_in sig_i[TP_NSTREAM_EXT], t_stream_out sig_o[kNstreamInt]) {
        hls::stream_of_blocks<buff_t> inter_buff;
#pragma HLS array_partition variable = inter_buff dim = 1 type = complete
#pragma HLS bind_storage variable = inter_buff type = RAM_T2P impl = bram
#pragma HLS DATAFLOW
        unpack_inputs(sig_i, inter_buff);
        ifft_load_buff(inter_buff, sig_o);
    }

    backTransposeSimpleCls() {
        for (int ss = 0; ss < kNstreamInt; ss++) {
            rdAddrLut[ss] = ss / kSamplesPerRead;
            rdBnkLut[ss] = ss;
#ifdef __BACK_TRANSPOSE_DEBUG__
            printf("1. stream= %d alculating bank: %d address : %d\n", ss, rdBnkLut[ss], rdAddrLut[ss]);
#endif //  __BACK_TRANSPOSE_DEBUG__
        }

        for (int ss = 0; ss < kNstreamInt; ss++) {
            wrBnkLut0[ss] = ss;
#ifdef __BACK_TRANSPOSE_DEBUG__
            printf("wrBnkLut0[%d] = %d\n", ss, wrBnkLut0[ss]);
#endif //  __BACK_TRANSPOSE_DEBUG__
        }
    }
};
};

#endif
