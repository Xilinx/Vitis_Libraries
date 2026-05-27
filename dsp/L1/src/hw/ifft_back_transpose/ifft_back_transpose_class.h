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
#include "vss_fft_ifft_1d_common.hpp"
//#define __BACK_TRANSPOSE_DEBUG__
using namespace xf::dsp::vss::common;

#ifndef POINT_SIZE_D1
#define POINT_SIZE_D1 1
#endif

namespace back_transpose {

template <int TP_POINT_SIZE, int TP_SSR, int TP_MODE = 1, int TP_POINT_SIZE_D1 = 1>
class backTransposeCls {
   public:
    static constexpr unsigned kPortWidth = 128; // Size of PLIO bus on PL side @ 312.5 MHz
    static constexpr unsigned kSamplesPerRead =
        2; // 128/sizeof(sample). 128 is the PL port widths to match the PLIO bandwidth at 312.5 MHz
    static constexpr unsigned kPtSizeD1 =
        (TP_POINT_SIZE_D1 == 1) ? fnPtSizeD1<TP_POINT_SIZE, modeAIEffts, TP_SSR>() : TP_POINT_SIZE_D1;
    static constexpr unsigned kPtSizeD2 = TP_POINT_SIZE / kPtSizeD1;
    static constexpr unsigned kPtSizeD1Ceil = fnCeil<kPtSizeD1, kSamplesPerRead * TP_SSR>();

    int rdAddrLut1[TP_SSR][kPtSizeD1 / kSamplesPerRead];
    int rdAddrLut2[TP_SSR][kPtSizeD1 / kSamplesPerRead];
    int rdBnkLut1[TP_SSR][kPtSizeD1 / kSamplesPerRead];
    int rdBnkLut2[TP_SSR][kPtSizeD1 / kSamplesPerRead];
    int wrBnkLut1[TP_SSR][kPtSizeD2 / kSamplesPerRead];
    int wrBnkLut2[TP_SSR][kPtSizeD2 / kSamplesPerRead];

    static constexpr unsigned int kNumLoadsD1 = fnCeil<kPtSizeD1, TP_SSR>() / TP_SSR;
    static constexpr unsigned int kNumLoads = kNumLoadsD1 * kPtSizeD2;
    static constexpr unsigned kNumStores = kNumLoads;
    static constexpr unsigned kNumRows = fnCeil<kPtSizeD2, TP_SSR>() / TP_SSR;
    typedef ap_uint<kPortWidth> TT_DATA;
    typedef ap_uint<kPortWidth / kSamplesPerRead> TT_SAMPLE;
    typedef hls::stream<TT_DATA> TT_STREAM;
    typedef TT_SAMPLE buff_t[TP_SSR][kNumStores];

    void unpack_inputs(TT_STREAM sig_i[TP_SSR], hls::stream_of_blocks<buff_t>& inter_buff) {
        hls::write_lock<buff_t> buff_in(inter_buff);
        TT_SAMPLE trans_i[kSamplesPerRead * TP_SSR];
#pragma HLS array_partition variable = trans_i dim = 1
        int idx = 0;
    BUFF_LOOP:
        for (int ii = 0; ii < kNumLoadsD1; ii++) {
#pragma HLS PIPELINE II = kPtSizeD2 / 2
        RPT_RD_LOOP:
            for (int pt = 0; pt < kPtSizeD2 / 2; pt++) { // 4
            READ:
                for (int ss = 0; ss < TP_SSR; ss++) {
                    (trans_i[(ss << 1) + 1], trans_i[(ss << 1) + 0]) = sig_i[ss].read();
                }
            STORE:
                for (int ss = 0; ss < TP_SSR; ss++) {
                    buff_in[wrBnkLut1[ss][pt]][idx] = trans_i[(ss << 1) + 0];
                    buff_in[wrBnkLut2[ss][pt]][idx + 1] = trans_i[(ss << 1) + 1];
                }
                idx = idx + 2;
            }
        }
    }

    void ifft_load_buff(hls::stream_of_blocks<buff_t>& inter_buff, TT_STREAM sig_o[TP_SSR]) {
        TT_SAMPLE trans_o[2 * TP_SSR];
#pragma HLS array_partition variable = trans_o dim = 1
        hls::read_lock<buff_t> buff_out(inter_buff);

        int rd_addr0[TP_SSR], rd_addr1[TP_SSR];
        TT_SAMPLE rd_data0[TP_SSR], rd_data1[TP_SSR];
        int rdBankAddr = 0;
    WR_LOOP:
        for (int cc = 0; cc < kNumRows; cc++) {
#pragma HLS PIPELINE II = kPtSizeD1 / 2
        RPT_LOOP:
            for (int pt = 0; pt < kPtSizeD1 / 2; pt++) {
                //#pragma HLS PIPELINE II=1
                for (int ss = 0; ss < TP_SSR; ss++) {
                    rd_addr0[ss] = rdBankAddr + rdAddrLut1[ss][pt];
                    rd_addr1[ss] = rdBankAddr + rdAddrLut2[ss][pt];
                }
                for (int ss = 0; ss < TP_SSR; ss++) {
                    trans_o[(ss << 1) + 0] = buff_out[rdBnkLut1[ss][pt]][rd_addr0[ss]];
                    trans_o[(ss << 1) + 1] = buff_out[rdBnkLut2[ss][pt]][rd_addr1[ss]];
                    sig_o[ss].write((trans_o[(ss << 1) + 1], trans_o[(ss << 1) + 0]));
                }
            }
            rdBankAddr += TP_SSR;
        }
    }

    void ifft_back_transpose_top(TT_STREAM sig_i[TP_SSR], TT_STREAM sig_o[TP_SSR]) {
        hls::stream_of_blocks<buff_t> inter_buff;
#pragma HLS array_partition variable = inter_buff dim = 1 type = complete
#pragma HLS bind_storage variable = inter_buff type = RAM_T2P impl = bram
#pragma HLS dependence variable = inter_buff type = intra false
#pragma HLS dependence variable = inter_buff type = inter false
#pragma HLS DATAFLOW
        unpack_inputs(sig_i, inter_buff);
        ifft_load_buff(inter_buff, sig_o);
    }

    backTransposeCls() {
        for (int ss = 0; ss < TP_SSR; ss++) {
            for (int i = 0; i < kPtSizeD1 / kSamplesPerRead; i++) {
                int n = ss + (i * kSamplesPerRead * TP_SSR);
                int offset = (n % kPtSizeD1) / TP_SSR * kPtSizeD2;
                rdAddrLut1[ss][i] = offset + n / kPtSizeD1;
                rdBnkLut1[ss][i] = ss;
            }
        }

        for (int ss = 0; ss < TP_SSR; ss++) {
            for (int ii = 0; ii < kPtSizeD1 / kSamplesPerRead; ii++) {
                int n = ss + TP_SSR + (ii * kSamplesPerRead * TP_SSR);
                int offset = (n % kPtSizeD1) / TP_SSR * kPtSizeD2;
                rdAddrLut2[ss][ii] = offset + n / kPtSizeD1;
                rdBnkLut2[ss][ii] = ss;
            }
        }
        for (int ss = 0; ss < TP_SSR; ss++) {
            for (int ii = 0; ii < kPtSizeD2 / 2; ii++) {
                int step = kPtSizeD1 % TP_SSR;
                wrBnkLut1[ss][ii] = (ss + 2 * step * ii) % TP_SSR;
                wrBnkLut2[ss][ii] = (ss + 2 * step * ii + step) % TP_SSR;
            }
        }
    }
};
};

#endif
