/*
 * Copyright (C) 2019-2022, Xilinx, Inc.
 * Copyright (C) 2022-2026, Advanced Micro Devices, Inc.
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
#ifndef IFFT_TRANSPOSE_CLASS_H
#define IFFT_TRANSPOSE_CLASS_H

#include <complex>
#include <ap_fixed.h>
#include <hls_stream.h>
#include <hls_streamofblocks.h>
#include "common.hpp"
#include "vss_fft_ifft_1d_common.hpp"
using namespace xf::dsp::vss::common;
namespace ifft_mid_transpose {
template <int TP_NFFT, int TP_NSTREAM, int TP_POINT_SIZE_D1 = 1>
class midTransposeCls {
   public:
    static constexpr unsigned NBITS = 128; // Size of PLIO bus on PL side @ 312.5 MHz
    typedef ap_uint<NBITS> TT_DATA;        // Equals two 'cint32' samples
    static constexpr unsigned SAMPLE_SIZE = 64;
    static constexpr unsigned NPHASES = TP_NSTREAM;
    static constexpr unsigned samplesPerRead = NBITS / SAMPLE_SIZE;
    static constexpr unsigned ptSizeD1 =
        (TP_POINT_SIZE_D1 == 1) ? fnPtSizeD1<TP_NFFT, modeAIEffts, TP_NSTREAM>() : TP_POINT_SIZE_D1;
    static constexpr unsigned ptSizeD1Ceil = fnCeil<ptSizeD1, TP_NSTREAM * samplesPerRead>(); // 70
    static constexpr unsigned ptSizeD2 = TP_NFFT / ptSizeD1;
    static constexpr unsigned ptSizeD2Ceil = fnCeil<ptSizeD2, TP_NSTREAM>();                    // 65
    static constexpr unsigned ptSizeD1CeilRd = fnCeil<ptSizeD1, TP_NSTREAM>();                  // 65
    static constexpr unsigned ptSizeD2CeilRd = fnCeil<ptSizeD2, TP_NSTREAM * samplesPerRead>(); // 65

    static constexpr unsigned NROW = (ptSizeD1 + TP_NSTREAM - 1) / TP_NSTREAM; // # of rows of transforms per bank
    static constexpr unsigned EXTRA =
        ptSizeD1Ceil - ptSizeD1; // # of extra zero-padded samples (to made divisible by TP_NSTREAM)
    static constexpr unsigned DEPTH =
        TP_NSTREAM != 1 ? ptSizeD1Ceil : 2 * ptSizeD1Ceil; //(TP_NFFT / ptSizeD1 + EXTRA);   // Depth of each bank

    typedef ap_uint<SAMPLE_SIZE> TT_SAMPLE; // Samples are 'cint32'  //
    typedef hls::stream<TT_DATA> TT_STREAM;
    typedef TT_SAMPLE buff_t[TP_NSTREAM][(ptSizeD1Ceil * ptSizeD2Ceil) / TP_NSTREAM];
    static constexpr unsigned oddPhases = (TP_NSTREAM % 2 == 1) ? 1 : 0;
    static constexpr unsigned offsetVal = (oddPhases == 1) ? ptSizeD1Ceil : 0;
    static constexpr int midPh = TP_NSTREAM / 2;
    static constexpr int midRdPh = (TP_NSTREAM + 1) / 2; //
    static constexpr int roundPh = (ptSizeD1 / (samplesPerRead * TP_NSTREAM));
    static constexpr int oddPhWr = (ptSizeD1 % (samplesPerRead * TP_NSTREAM)) / samplesPerRead;
    static constexpr int oddPhRd = (ptSizeD2 % (samplesPerRead * TP_NSTREAM)) / samplesPerRead;
    // first TP_NSTREAM dimension is for the number of banks. second TP_NSTREAM dimension is used to track the phase.
    // There are a total of TP_NSTREAM phases of read and write after which the read and write pattern repeats.
    int wrBnkLut1[TP_NSTREAM][TP_NSTREAM];
    int wrBnkLut2[TP_NSTREAM][TP_NSTREAM];

    int rdAddrLut1[TP_NSTREAM][TP_NSTREAM];
    int rdAddrLut2[TP_NSTREAM][TP_NSTREAM];

    int rdBnkLut1[TP_NSTREAM][TP_NSTREAM];
    int rdBnkLut2[TP_NSTREAM][TP_NSTREAM];

    void ifft_unpack_write(TT_STREAM sig_i[TP_NSTREAM], hls::stream_of_blocks<buff_t>& inter_buff) {
        static int wrBankAddr = 0;
        static int wrBankAddrCol = 0;
        TT_SAMPLE trans_i[2 * TP_NSTREAM];
#pragma HLS array_partition variable = trans_i dim = 1
        hls::write_lock<buff_t> buff_in(inter_buff);

        for (int p2 = 0; p2 < ptSizeD2Ceil / TP_NSTREAM; p2++) { // 13
            wrBankAddr = p2 * ptSizeD1Ceil;
            for (int ii = 0; ii < (ptSizeD1Ceil) / (samplesPerRead * NPHASES); ii++) { // 70/(2*5) = 7
#pragma HLS PIPELINE II = NPHASES
                for (int ph = 0; ph < NPHASES; ph++) { // 5
                    //#pragma HLS PIPELINE II=1
                    if (ii * NPHASES + ph < ptSizeD1 / samplesPerRead) {
                    READ:
                        for (int ss = 0; ss < TP_NSTREAM; ss++) {
                            (trans_i[(ss << 1) + 1], trans_i[(ss << 1) + 0]) = sig_i[ss].read();
                        }
                        // Write data mux:
                        TT_SAMPLE wr_data0[TP_NSTREAM], wr_data1[TP_NSTREAM];

                        int wr_addr0 = wrBankAddr;
                        int wr_addr1 = wrBankAddr + 1;
                        for (int ss = 0; ss < TP_NSTREAM; ss++) {
                            buff_in[wrBnkLut1[ss][ph]][wr_addr0] = trans_i[ss * 2];
                            buff_in[wrBnkLut2[ss][ph]][wr_addr1] = trans_i[ss * 2 + 1];
                        }
                        wrBankAddr = wrBankAddr + samplesPerRead;
                    }
                }
            }
        }
    }

    void ifft_write_streams(hls::stream_of_blocks<buff_t>& inter_buff, TT_STREAM sig_o[TP_NSTREAM]) {
        // FILE *fptr      = fopen("/home/uvimalku/debug.txt", "a");
        static int rd_bank_addr = 0;
        static int d1Ptr = 0; //
        static int d2Ptr = 0; //

        TT_SAMPLE trans_o[2 * TP_NSTREAM];
#pragma HLS array_partition variable = trans_o dim = 1
        hls::read_lock<buff_t> buff_out(inter_buff);

        // Read address mux:
        // for odd numbers
        int offset = 0;
        int rd_addr0[TP_NSTREAM], rd_addr1[TP_NSTREAM];

        rd_bank_addr = 0;
        for (int p1 = 0; p1 < (ptSizeD1CeilRd) / (TP_NSTREAM); p1++) { // 13
            d2Ptr = 0;
            d1Ptr = p1 * TP_NSTREAM;
            rd_bank_addr = d1Ptr;
            for (int ii = 0; ii < (ptSizeD2CeilRd) / (samplesPerRead * NPHASES); ii++) { //
#pragma HLS PIPELINE II = NPHASES
                for (int ph = 0; ph < NPHASES; ph++) {
                    //#pragma HLS PIPELINE II=1
                    if (ii * NPHASES + ph < ptSizeD2 / samplesPerRead) {
                        int offset = (ph == midPh) ? offsetVal : 0;
                        for (int ss = 0; ss < TP_NSTREAM; ss++) {
                            rd_addr0[ss] = rd_bank_addr + rdAddrLut1[ss][ph];
                            rd_addr1[ss] = rd_bank_addr + rdAddrLut2[ss][ph] + offset;
                        }

                        // Read and Write:
                        TT_SAMPLE rd_data0[TP_NSTREAM], rd_data1[TP_NSTREAM];

                        for (int ss = 0; ss < TP_NSTREAM; ss++) {
                            trans_o[ss * 2] = buff_out[rdBnkLut1[ss][ph]][rd_addr0[ss]];
                            trans_o[ss * 2 + 1] = buff_out[rdBnkLut2[ss][ph]][rd_addr1[ss]];
                        }

                    WRITE:
                        for (int ss = 0; ss < TP_NSTREAM; ss++) {
                            sig_o[ss].write((trans_o[(ss << 1) + 1], trans_o[(ss << 1) + 0]));
                        }

                        // condition for odd numbers
                        if (ph == midRdPh - 1 || ph == TP_NSTREAM - 1) {
                            rd_bank_addr = rd_bank_addr + DEPTH;
                        }

                        //
                    }
                }
            }
        }
    }

    void ifft_transpose_top(TT_STREAM sig_i[TP_NSTREAM], TT_STREAM sig_o[TP_NSTREAM]) {
        //#pragma HLS interface mode=ap_ctrl_none port=return
        hls::stream_of_blocks<buff_t> inter_buff;
#pragma HLS array_partition variable = inter_buff dim = 1 type = complete
#pragma HLS bind_storage variable = inter_buff type = RAM_T2P impl = bram
#pragma HLS dependence variable = inter_buff type = inter false
#pragma HLS dependence variable = inter_buff type = intra false
#pragma HLS DATAFLOW

        // for(int i = 0; i < TP_NFFT/(TP_NSTREAM); i++){
        // Unpack samples:
        ifft_unpack_write(sig_i, inter_buff);

        // Format output streams:
        ifft_write_streams(inter_buff, sig_o);
        // }
    }

    midTransposeCls() {
        for (int ss = 0; ss < TP_NSTREAM; ss++) {
            for (int ph = 0; ph < TP_NSTREAM; ph++) {
                wrBnkLut1[ss][ph] =
                    (ss + samplesPerRead * ph) % TP_NSTREAM; // SS-0: 0, 3, 1, 4, 2   // SS-1: 4, 2, 0, 3, 1   //
                wrBnkLut2[ss][ph] =
                    (ss + 1 + samplesPerRead * ph) % TP_NSTREAM; // SS-0: 4, 2, 0, 3, 1   // SS-1: 3, 1, 4, 2, 0   //
            }
        }

        for (int ss = 0; ss < TP_NSTREAM; ss++) {
            for (int ph = 0; ph < TP_NSTREAM; ph++) {
                rdBnkLut1[ss][ph] = (ss + samplesPerRead * ph) % TP_NSTREAM;
                rdBnkLut2[ss][ph] = (ss + 1 + samplesPerRead * ph) % TP_NSTREAM;
            }
        }

        for (int ss = 0; ss < TP_NSTREAM; ss++) {
            for (int ph = 0; ph < TP_NSTREAM; ph++) {
                rdAddrLut1[ss][ph] = (ss + ph * ptSizeD1Ceil) % TP_NSTREAM;
                rdAddrLut2[ss][ph] = (ss + ph * ptSizeD1Ceil) % TP_NSTREAM;
            }
        }
    }
};
};
#endif
