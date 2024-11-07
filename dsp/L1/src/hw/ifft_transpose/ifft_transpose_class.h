/*
 * Copyright (C) 2019-2022, Xilinx, Inc.
 * Copyright (C) 2022-2024, Advanced Micro Devices, Inc.
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
//#define __MID_TRANSPOSE_DEBUG__
namespace ifft_mid_transpose {
template <int NFFT, int NSTREAM>
class midTransposeCls {
   public:
    template <unsigned int TP_POINT_SIZE>
    static constexpr unsigned int fnPtSizeD1() {
        unsigned int sqrtVal =
            TP_POINT_SIZE == 65536
                ? 256
                : TP_POINT_SIZE == 32768
                      ? 128
                      : TP_POINT_SIZE == 16384
                            ? 128
                            : TP_POINT_SIZE == 8192
                                  ? 64
                                  : TP_POINT_SIZE == 4096
                                        ? 64
                                        : TP_POINT_SIZE == 2048
                                              ? 32
                                              : TP_POINT_SIZE == 1024
                                                    ? 32
                                                    : TP_POINT_SIZE == 512
                                                          ? 16
                                                          : TP_POINT_SIZE == 256
                                                                ? 16
                                                                : TP_POINT_SIZE == 128
                                                                      ? 8
                                                                      : TP_POINT_SIZE == 64
                                                                            ? 8
                                                                            : TP_POINT_SIZE == 32
                                                                                  ? 4
                                                                                  : TP_POINT_SIZE == 16 ? 4 : 0;
        return sqrtVal;
    }

    template <unsigned int len, unsigned int rnd>
    static constexpr unsigned int fnCeil() {
        return (len + rnd - 1) / rnd * rnd;
    }

    static constexpr unsigned NBITS = 128; // Size of PLIO bus on PL side @ 312.5 MHz
    typedef ap_uint<NBITS> TT_DATA;        // Equals two 'cint32' samples
    static constexpr unsigned SAMPLE_SIZE = 64;
    static constexpr unsigned NPHASES = NSTREAM;
    static constexpr unsigned samplesPerRead = NBITS / SAMPLE_SIZE;
    static constexpr unsigned ptSizeD1 = fnPtSizeD1<POINT_SIZE>();
    static constexpr unsigned ptSizeD1Ceil = fnCeil<ptSizeD1, NSTREAM * samplesPerRead>(); // 70
    static constexpr unsigned ptSizeD2 = POINT_SIZE / ptSizeD1;
    static constexpr unsigned ptSizeD2Ceil = fnCeil<ptSizeD2, NSTREAM>();                    // 65
    static constexpr unsigned ptSizeD1CeilRd = fnCeil<ptSizeD1, NSTREAM>();                  // 65
    static constexpr unsigned ptSizeD2CeilRd = fnCeil<ptSizeD2, NSTREAM * samplesPerRead>(); // 65

    static constexpr unsigned NROW = (ptSizeD1 + NSTREAM - 1) / NSTREAM; // # of rows of transforms per bank
    static constexpr unsigned EXTRA =
        ptSizeD1Ceil - ptSizeD1; // # of extra zero-padded samples (to made divisible by NSTREAM)
    static constexpr unsigned DEPTH =
        NSTREAM != 1 ? ptSizeD1Ceil : 2 * ptSizeD1Ceil; //(POINT_SIZE / ptSizeD1 + EXTRA);   // Depth of each bank

    typedef ap_uint<SAMPLE_SIZE> TT_SAMPLE; // Samples are 'cint32'  //
    typedef hls::stream<TT_DATA> TT_STREAM;
    typedef TT_SAMPLE buff_t[NSTREAM][(ptSizeD1Ceil * ptSizeD2Ceil) / NSTREAM];
    static constexpr unsigned oddPhases = (NSTREAM % 2 == 1) ? 1 : 0;
    static constexpr unsigned offsetVal = (oddPhases == 1) ? ptSizeD1Ceil : 0;
    static constexpr int midPh = NSTREAM / 2;
    static constexpr int midRdPh = (NSTREAM + 1) / 2; //
    static constexpr int roundPh = (ptSizeD1 / (samplesPerRead * NSTREAM));
    static constexpr int oddPhWr = (ptSizeD1 % (samplesPerRead * NSTREAM)) / samplesPerRead;
    static constexpr int oddPhRd = (ptSizeD2 % (samplesPerRead * NSTREAM)) / samplesPerRead;
    // first NSTREAM dimension is for the number of banks. second NSTREAM dimension is used to track the phase. There
    // are a total of NSTREAM phases of read and write after which the read and write pattern repeats.
    int wrBnkLut1[NSTREAM][NSTREAM];
    int wrBnkLut2[NSTREAM][NSTREAM];

    int rdAddrLut1[NSTREAM][NSTREAM];
    int rdAddrLut2[NSTREAM][NSTREAM];

    int rdBnkLut1[NSTREAM][NSTREAM];
    int rdBnkLut2[NSTREAM][NSTREAM];

    void ifft_unpack_write(TT_STREAM sig_i[NSTREAM], hls::stream_of_blocks<buff_t>& inter_buff) {
        static int wrBankAddr = 0;
        static int wrBankAddrCol = 0;
        TT_SAMPLE trans_i[2 * NSTREAM];
#pragma HLS array_partition variable = trans_i dim = 1
        hls::write_lock<buff_t> buff_in(inter_buff);

        for (int p2 = 0; p2 < ptSizeD2Ceil / NSTREAM; p2++) { // 13
            wrBankAddr = p2 * ptSizeD1Ceil;
            for (int ii = 0; ii < (ptSizeD1Ceil) / (samplesPerRead * NPHASES); ii++) { // 70/(2*5) = 7
#pragma HLS PIPELINE II = NPHASES
                for (int ph = 0; ph < NPHASES; ph++) { // 5
                    //#pragma HLS PIPELINE II=1
                    if (ii * NPHASES + ph < ptSizeD1 / samplesPerRead) {
                    READ:
                        for (int ss = 0; ss < NSTREAM; ss++) {
                            (trans_i[(ss << 1) + 1], trans_i[(ss << 1) + 0]) = sig_i[ss].read();
                        }
                        // Write data mux:
                        TT_SAMPLE wr_data0[NSTREAM], wr_data1[NSTREAM];

                        int wr_addr0 = wrBankAddr;
                        int wr_addr1 = wrBankAddr + 1;
                        for (int ss = 0; ss < NSTREAM; ss++) {
                            buff_in[wrBnkLut1[ss][ph]][wr_addr0] = trans_i[ss * 2];
                            buff_in[wrBnkLut2[ss][ph]][wr_addr1] = trans_i[ss * 2 + 1];
                        }
                        wrBankAddr = wrBankAddr + samplesPerRead;
                    }
                }
            }
        }
    }

    void ifft_write_streams(hls::stream_of_blocks<buff_t>& inter_buff, TT_STREAM sig_o[NSTREAM]) {
        // FILE *fptr      = fopen("/home/uvimalku/debug.txt", "a");
        static int rd_bank_addr = 0;
        static int d1Ptr = 0; //
        static int d2Ptr = 0; //

        TT_SAMPLE trans_o[2 * NSTREAM];
#pragma HLS array_partition variable = trans_o dim = 1
        hls::read_lock<buff_t> buff_out(inter_buff);

        // Read address mux:
        // for odd numbers
        int offset = 0;
        int rd_addr0[NSTREAM], rd_addr1[NSTREAM];

        rd_bank_addr = 0;
        for (int p1 = 0; p1 < (ptSizeD1CeilRd) / (NSTREAM); p1++) { // 13
            d2Ptr = 0;
            d1Ptr = p1 * NSTREAM;
            rd_bank_addr = d1Ptr;
            for (int ii = 0; ii < (ptSizeD2CeilRd) / (samplesPerRead * NPHASES); ii++) { //
#pragma HLS PIPELINE II = NPHASES
                for (int ph = 0; ph < NPHASES; ph++) {
                    //#pragma HLS PIPELINE II=1
                    if (ii * NPHASES + ph < ptSizeD2 / samplesPerRead) {
                        int offset = (ph == midPh) ? offsetVal : 0;
                        for (int ss = 0; ss < NSTREAM; ss++) {
                            rd_addr0[ss] = rd_bank_addr + rdAddrLut1[ss][ph];
                            rd_addr1[ss] = rd_bank_addr + rdAddrLut2[ss][ph] + offset;
                        }

                        // Read and Write:
                        TT_SAMPLE rd_data0[NSTREAM], rd_data1[NSTREAM];

                        for (int ss = 0; ss < NSTREAM; ss++) {
                            trans_o[ss * 2] = buff_out[rdBnkLut1[ss][ph]][rd_addr0[ss]];
                            trans_o[ss * 2 + 1] = buff_out[rdBnkLut2[ss][ph]][rd_addr1[ss]];
                        }

                    WRITE:
                        for (int ss = 0; ss < NSTREAM; ss++) {
                            sig_o[ss].write((trans_o[(ss << 1) + 1], trans_o[(ss << 1) + 0]));
                        }

                        // condition for odd numbers
                        if (ph == midRdPh - 1 || ph == NSTREAM - 1) {
                            rd_bank_addr = rd_bank_addr + DEPTH;
                        }

                        //
                    }
                }
            }
        }
    }

    void ifft_transpose_top(TT_STREAM sig_i[NSTREAM], TT_STREAM sig_o[NSTREAM]) {
        //#pragma HLS interface mode=ap_ctrl_none port=return
        hls::stream_of_blocks<buff_t> inter_buff;
#pragma HLS array_partition variable = inter_buff dim = 1 type = complete
#pragma HLS bind_storage variable = inter_buff type = RAM_T2P impl = bram
#pragma HLS dependence variable = inter_buff type = inter false
#pragma HLS dependence variable = inter_buff type = intra false
#pragma HLS DATAFLOW

        // for(int i = 0; i < POINT_SIZE/(NSTREAM); i++){
        // Unpack samples:
        ifft_unpack_write(sig_i, inter_buff);

        // Format output streams:
        ifft_write_streams(inter_buff, sig_o);
        // }
    }

    midTransposeCls() {
        for (int ss = 0; ss < NSTREAM; ss++) {
            for (int ph = 0; ph < NSTREAM; ph++) {
                wrBnkLut1[ss][ph] =
                    (ss + samplesPerRead * ph) % NSTREAM; // SS-0: 0, 3, 1, 4, 2   // SS-1: 4, 2, 0, 3, 1   //
                wrBnkLut2[ss][ph] =
                    (ss + 1 + samplesPerRead * ph) % NSTREAM; // SS-0: 4, 2, 0, 3, 1   // SS-1: 3, 1, 4, 2, 0   //
            }
        }

        for (int ss = 0; ss < NSTREAM; ss++) {
            for (int ph = 0; ph < NSTREAM; ph++) {
                rdBnkLut1[ss][ph] = (ss + samplesPerRead * ph) % NSTREAM;
                rdBnkLut2[ss][ph] = (ss + 1 + samplesPerRead * ph) % NSTREAM;
            }
        }

        for (int ss = 0; ss < NSTREAM; ss++) {
            for (int ph = 0; ph < NSTREAM; ph++) {
                rdAddrLut1[ss][ph] = (ss + ph * ptSizeD1Ceil) % NSTREAM;
                rdAddrLut2[ss][ph] = (ss + ph * ptSizeD1Ceil) % NSTREAM;
            }
        }
    }
};
};
#endif