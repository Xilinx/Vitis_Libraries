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
#ifndef IFFT_BACK_TRANSPOSE_CLASS_H
#define IFFT_BACK_TRANSPOSE_CLASS_H

#include <complex>
#include <ap_fixed.h>
#include <hls_stream.h>
#include <hls_streamofblocks.h>
//#define __BACK_TRANSPOSE_DEBUG__

namespace back_transpose {

template <int NFFT, int NSTREAM>
class backTransposeCls {
   public:
    template <unsigned int len, unsigned int rnd>
    static constexpr unsigned int fnCeil() {
        return (len + rnd - 1) / rnd * rnd;
    }

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

    static constexpr unsigned SAMPLES_PER_READ = 2;
    static constexpr unsigned ptSizeCeil = fnCeil<NFFT, SSR>(); // 65

    static constexpr unsigned ptSizeD1 = fnPtSizeD1<NFFT>();                             // 32
    static constexpr unsigned ptSizeD2 = NFFT / ptSizeD1;                                // 32
    static constexpr unsigned ptSizeD1Ceil = fnCeil<ptSizeD1, SAMPLES_PER_READ * SSR>(); // 32

    static constexpr unsigned EXTRA = 0; // # of extra zero-padded samples (to made divisible by NSTREAM)
    static constexpr unsigned DEPTH = (NFFT / ptSizeD1 + EXTRA); // Depth of each bank // 16
    static constexpr unsigned NROW = ptSizeD1 / NSTREAM;         // # of rows of transforms per bank // 16

    static constexpr unsigned NBITS = 128; // Size of PLIO bus on PL side @ 312.5 MHz

    int rdAddrLut1[NSTREAM][ptSizeD1 / SAMPLES_PER_READ];
    int rdAddrLut2[NSTREAM][ptSizeD1 / SAMPLES_PER_READ];
    int rdBnkLut1[NSTREAM][ptSizeD1 / SAMPLES_PER_READ];
    int rdBnkLut2[NSTREAM][ptSizeD1 / SAMPLES_PER_READ];
    int wrBnkLut1[NSTREAM][ptSizeD2 / SAMPLES_PER_READ];
    int wrBnkLut2[NSTREAM][ptSizeD2 / SAMPLES_PER_READ];

    static constexpr unsigned int numLoadsPtSize = fnCeil<ptSizeD1, SSR>() / SSR; //
    static constexpr unsigned int numLoads = numLoadsPtSize * ptSizeD2;
    static constexpr unsigned numStores = numLoads; // 7
    static constexpr unsigned numRows = fnCeil<ptSizeD2, SSR>() / SSR;
    typedef ap_uint<NBITS> TT_DATA;                      // Equals two 'cint32' samples
    typedef ap_uint<NBITS / SAMPLES_PER_READ> TT_SAMPLE; // Samples are 'cint32'
    typedef hls::stream<TT_DATA> TT_STREAM;
    typedef TT_SAMPLE buff_t[NSTREAM][numStores];

    void unpack_inputs(TT_STREAM sig_i[NSTREAM], hls::stream_of_blocks<buff_t>& inter_buff) {
        hls::write_lock<buff_t> buff_in(inter_buff);
        TT_SAMPLE trans_i[SAMPLES_PER_READ * NSTREAM];
#pragma HLS array_partition variable = trans_i dim = 1

        // *TODO: Reduce unroll factor to NSTREAM by padding at the input
        // int phAddr = 0;
        // int colNum = 0;
        // int rowNum = 0;
        int idx = 0;
    BUFF_LOOP:
        for (int ii = 0; ii < numLoadsPtSize; ii++) {
        //#pragma HLS PIPELINE II=ptSizeD2/2
        RPT_RD_LOOP:
            for (int pt = 0; pt < ptSizeD2 / 2; pt++) { // 4
#pragma HLS PIPELINE II = 1
            READ:
                for (int ss = 0; ss < NSTREAM; ss++) {
                    (trans_i[(ss << 1) + 1], trans_i[(ss << 1) + 0]) = sig_i[ss].read();
                    buff_in[wrBnkLut1[ss][pt]][idx] = trans_i[(ss << 1) + 0];
                    buff_in[wrBnkLut2[ss][pt]][idx + 1] = trans_i[(ss << 1) + 1];
                }

                idx = idx + 2;
            }
        }
    }

    void ifft_load_buff(hls::stream_of_blocks<buff_t>& inter_buff, TT_STREAM sig_o[NSTREAM]) {
        TT_SAMPLE trans_o[2 * NSTREAM];
#pragma HLS array_partition variable = trans_o dim = 1
        hls::read_lock<buff_t> buff_out(inter_buff);

        int rd_addr0[NSTREAM], rd_addr1[NSTREAM];
        TT_SAMPLE rd_data0[NSTREAM], rd_data1[NSTREAM];
        int rdBankAddr = 0;
    WR_LOOP:
        for (int cc = 0; cc < numRows; cc++) { // 2
        //#pragma HLS PIPELINE II=ptSizeD1/2
        RPT_LOOP:
            for (int pt = 0; pt < ptSizeD1 / 2; pt++) { // 4
                                                        //#pragma HLS unroll
                for (int ss = 0; ss < NSTREAM; ss++) {
                    rd_addr0[ss] = rdBankAddr + rdAddrLut1[ss][pt];
                    rd_addr1[ss] = rdBankAddr + rdAddrLut2[ss][pt];
                }
                for (int ss = 0; ss < NSTREAM; ss++) { // 5
                    trans_o[(ss << 1) + 0] = buff_out[ss][rd_addr0[ss]];
                    trans_o[(ss << 1) + 1] = buff_out[ss][rd_addr1[ss]];
                    sig_o[ss].write((trans_o[(ss << 1) + 1], trans_o[(ss << 1) + 0]));
                }
            }
            rdBankAddr += NSTREAM;
        }
    }

    void ifft_back_transpose_top(TT_STREAM sig_i[NSTREAM], TT_STREAM sig_o[NSTREAM]) {
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
        for (int ss = 0; ss < NSTREAM; ss++) {
            for (int i = 0; i < ptSizeD1 / SAMPLES_PER_READ; i++) {
                int n = ss + (i * SAMPLES_PER_READ * NSTREAM);
                int offset = (n % ptSizeD1) / NSTREAM * ptSizeD2;
                rdAddrLut1[ss][i] = offset + n / ptSizeD1;
                rdBnkLut1[ss][i] = (n % ptSizeD1) % NSTREAM;
            }
        }

        for (int ss = 0; ss < NSTREAM; ss++) {
            for (int ii = 0; ii < ptSizeD1 / SAMPLES_PER_READ; ii++) {
                int n = ss + NSTREAM + (ii * SAMPLES_PER_READ * NSTREAM);
                int offset = (n % ptSizeD1) / NSTREAM * ptSizeD2;
                rdAddrLut2[ss][ii] = offset + n / ptSizeD1;
                rdBnkLut2[ss][ii] = (n % ptSizeD1) % NSTREAM;
            }
        }

        for (int ss = 0; ss < NSTREAM; ss++) {
            for (int ii = 0; ii < ptSizeD2 / SAMPLES_PER_READ; ii++) {
                int step = ptSizeD1 % NSTREAM;
                wrBnkLut1[ss][ii] = (ss + 2 * step * ii) % NSTREAM;
                wrBnkLut2[ss][ii] = (ss + 2 * step * ii + step) % NSTREAM;
            }
        }
    }
};
};

#endif