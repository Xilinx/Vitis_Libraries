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

namespace back_transpose {

template <int NFFT, int NSTREAM, int MODE>
class backTransposeCls {
   public:
    static constexpr unsigned int modePLffts() { return 1; }

    static constexpr unsigned int modeAIEffts() { return 0; }
    template <unsigned int len, unsigned int rnd>
    static constexpr unsigned int fnCeil() {
        return (len + rnd - 1) / rnd * rnd;
    }

    template <unsigned int TP_POINT_SIZE, unsigned int TP_VSS_MODE, unsigned int TP_SSR>
    static constexpr unsigned int fnPtSizeD1() {
        if (TP_VSS_MODE == modeAIEffts()) {
            unsigned int sqrtVal =
                TP_POINT_SIZE == 65536
                    ? 256
                    : TP_POINT_SIZE == 32768
                          ? 256
                          : TP_POINT_SIZE == 16384
                                ? 128
                                : TP_POINT_SIZE == 8192
                                      ? 128
                                      : TP_POINT_SIZE == 4096
                                            ? 64
                                            : TP_POINT_SIZE == 2048
                                                  ? 64
                                                  : TP_POINT_SIZE == 1024
                                                        ? 32
                                                        : TP_POINT_SIZE == 512
                                                              ? 32
                                                              : TP_POINT_SIZE == 256
                                                                    ? 16
                                                                    : TP_POINT_SIZE == 128
                                                                          ? 16
                                                                          : TP_POINT_SIZE == 64
                                                                                ? 8
                                                                                : TP_POINT_SIZE == 32
                                                                                      ? 8
                                                                                      : TP_POINT_SIZE == 16 ? 4 : 0;
            return sqrtVal;

        } else {
            return TP_POINT_SIZE / TP_SSR;
        }
        return 0;
    }

    static constexpr unsigned SAMPLES_PER_READ = 2;
    static constexpr unsigned ptSizeCeil = fnCeil<NFFT, SSR>();
    static constexpr unsigned vssMode = MODE == 1 ? modeAIEffts() : modePLffts();
    static constexpr unsigned ptSizeD1 = fnPtSizeD1<NFFT, vssMode, SSR>();
    static constexpr unsigned ptSizeD2 = NFFT / ptSizeD1;
    static constexpr unsigned ptSizeD1Ceil = fnCeil<ptSizeD1, SAMPLES_PER_READ * SSR>();

    static constexpr unsigned EXTRA = 0;
    static constexpr unsigned DEPTH = (NFFT / ptSizeD1 + EXTRA);
    static constexpr unsigned NROW = ptSizeD1 / NSTREAM;

    static constexpr unsigned NBITS = 128; // Size of PLIO bus on PL side @ 312.5 MHz

    int rdAddrLut1[NSTREAM][ptSizeD1 / SAMPLES_PER_READ];
    int rdAddrLut2[NSTREAM][ptSizeD1 / SAMPLES_PER_READ];
    int rdBnkLut1[NSTREAM][ptSizeD1 / SAMPLES_PER_READ];
    int rdBnkLut2[NSTREAM][ptSizeD1 / SAMPLES_PER_READ];
    int wrBnkLut1[NSTREAM][ptSizeD2 / SAMPLES_PER_READ];
    int wrBnkLut2[NSTREAM][ptSizeD2 / SAMPLES_PER_READ];

    static constexpr unsigned int numLoadsPtSize = fnCeil<ptSizeD1, SSR>() / SSR;
    static constexpr unsigned int numLoads = numLoadsPtSize * ptSizeD2;
    static constexpr unsigned numStores = numLoads;
    static constexpr unsigned numRows = fnCeil<ptSizeD2, SSR>() / SSR;
    typedef ap_uint<NBITS> TT_DATA;
    typedef ap_uint<NBITS / SAMPLES_PER_READ> TT_SAMPLE;
    typedef hls::stream<TT_DATA> TT_STREAM;
    typedef TT_SAMPLE buff_t[NSTREAM][numStores];

    void unpack_inputs(TT_STREAM sig_i[NSTREAM], hls::stream_of_blocks<buff_t>& inter_buff) {
        hls::write_lock<buff_t> buff_in(inter_buff);
        TT_SAMPLE trans_i[SAMPLES_PER_READ * NSTREAM];
#pragma HLS array_partition variable = trans_i dim = 1
        int idx = 0;
    BUFF_LOOP:
        for (int ii = 0; ii < numLoadsPtSize; ii++) {
#pragma HLS PIPELINE II = ptSizeD2 / 2
        RPT_RD_LOOP:
            for (int pt = 0; pt < ptSizeD2 / 2; pt++) { // 4
            READ:
                for (int ss = 0; ss < NSTREAM; ss++) {
                    (trans_i[(ss << 1) + 1], trans_i[(ss << 1) + 0]) = sig_i[ss].read();
                }
            STORE:
                for (int ss = 0; ss < NSTREAM; ss++) {
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
        for (int cc = 0; cc < numRows; cc++) {
#pragma HLS PIPELINE II = ptSizeD1 / 2
        RPT_LOOP:
            for (int pt = 0; pt < ptSizeD1 / 2; pt++) {
                //#pragma HLS PIPELINE II=1
                for (int ss = 0; ss < NSTREAM; ss++) {
                    rd_addr0[ss] = rdBankAddr + rdAddrLut1[ss][pt];
                    rd_addr1[ss] = rdBankAddr + rdAddrLut2[ss][pt];
                }
                for (int ss = 0; ss < NSTREAM; ss++) {
                    trans_o[(ss << 1) + 0] = buff_out[rdBnkLut1[ss][pt]][rd_addr0[ss]];
                    trans_o[(ss << 1) + 1] = buff_out[rdBnkLut2[ss][pt]][rd_addr1[ss]];
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
        if
            constexpr(MODE == 1) {
                for (int ss = 0; ss < NSTREAM; ss++) {
                    for (int i = 0; i < ptSizeD1 / SAMPLES_PER_READ; i++) {
                        int n = ss + (i * SAMPLES_PER_READ * NSTREAM);
                        int offset = (n % ptSizeD1) / NSTREAM * ptSizeD2;
                        rdAddrLut1[ss][i] = offset + n / ptSizeD1;
                        rdBnkLut1[ss][i] = ss;
                    }
                }

                for (int ss = 0; ss < NSTREAM; ss++) {
                    for (int ii = 0; ii < ptSizeD1 / SAMPLES_PER_READ; ii++) {
                        int n = ss + NSTREAM + (ii * SAMPLES_PER_READ * NSTREAM);
                        int offset = (n % ptSizeD1) / NSTREAM * ptSizeD2;
                        rdAddrLut2[ss][ii] = offset + n / ptSizeD1;
                        rdBnkLut2[ss][ii] = ss;
                    }
                }
            }
        else {
            for (int ss = 0; ss < NSTREAM; ss++) {
                for (int i = 0; i < ptSizeD1 / SAMPLES_PER_READ; i++) {
                    int n = ss;
                    int offset = (n % ptSizeD1) / NSTREAM * ptSizeD2;
                    rdAddrLut1[ss][i] = (ss + (i * 2 * NSTREAM)) % ptSizeD1;
                    rdBnkLut1[ss][i] = (ss + ((i * 2 * NSTREAM) / ptSizeD1)) % NSTREAM;
                }
            }

            for (int ss = 0; ss < NSTREAM; ss++) {
                for (int ii = 0; ii < ptSizeD1 / SAMPLES_PER_READ; ii++) {
                    int n = ss;
                    int offset = (n % ptSizeD1) / NSTREAM * ptSizeD2;
                    rdAddrLut2[ss][ii] = (ss + (ii * 2 * NSTREAM + NSTREAM)) % ptSizeD1;
                    rdBnkLut2[ss][ii] = (ss + ((ii * 2 * NSTREAM + NSTREAM) / ptSizeD1)) % NSTREAM;
                }
            }
        }
        for (int ss = 0; ss < NSTREAM; ss++) {
            for (int ii = 0; ii < ptSizeD2 / 2; ii++) {
                int step = MODE == 1 ? ptSizeD1 % NSTREAM : (ptSizeD2 == NSTREAM) ? 1 : 0;
                wrBnkLut1[ss][ii] = (ss + 2 * step * ii) % NSTREAM;
                wrBnkLut2[ss][ii] = (ss + 2 * step * ii + step) % NSTREAM;
            }
        }
    }
};
};

#endif