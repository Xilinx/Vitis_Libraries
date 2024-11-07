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
#ifndef IFFT_FRONT_TRANSPOSE_CLASS_H
#define IFFT_FRONT_TRANSPOSE_CLASS_H

#include <complex>
#include <ap_fixed.h>
#include <hls_stream.h>
#include <hls_vector.h>
#include <hls_streamofblocks.h>
//#define __FRONT_TRANSPOSE_DEBUG__

namespace front_transpose {

template <int NFFT, int NSTREAM>
class frontTransposeCls {
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

    template <unsigned int ptSizeD2, unsigned int offset, unsigned int numElems>
    static constexpr std::array<int, numElems> fnCalcRdAddr() {
        std::array<int, numElems> addrLoad = {0};
        for (int i = 0; i < numElems; i++) {
            addrLoad[i] = (offset * ptSizeD2 + i * (2 * ptSizeD2)) / numElems;
        }
        const std::array<int, numElems> addrLoadRet = addrLoad;
        return addrLoad;
    }

    template <unsigned int ptSizeD2, unsigned int offset, unsigned int numElems>
    static constexpr std::array<int, numElems> fnCalcRdBnk() {
        std::array<int, numElems> bnkLoad = {0};
        for (int i = 0; i < numElems; i++) {
            bnkLoad[i] = (offset * ptSizeD2 + i * (2 * ptSizeD2)) % numElems;
        }
        const std::array<int, numElems> bnkLoadRet = bnkLoad;
        return bnkLoad;
    }

    static constexpr unsigned samplesPerRead = 2;
    static constexpr unsigned ptSizeCeil = fnCeil<NFFT, SSR>(); // 65
    static constexpr unsigned NPHASES = SSR;
    static constexpr unsigned ptSizeD1 = fnPtSizeD1<NFFT>(); // 32
    static constexpr unsigned ptSizeD2 = NFFT / ptSizeD1;    // 32

    static constexpr unsigned EXTRA = 0; // # of extra zero-padded samples (to made divisible by NSTREAM)
    static constexpr unsigned DEPTH = (NFFT / ptSizeD1 + EXTRA); // Depth of each bank // 16
    static constexpr unsigned NROW = ptSizeD1 / NSTREAM;         // # of rows of transforms per bank // 16

    static constexpr unsigned NBITS = 128; // Size of PLIO bus on PL side @ 312.5 MHz

    // static constexpr std::array<int, NSTREAM> rdAddrLut1 = fnCalcRdAddr<ptSizeD1, 0, NSTREAM>();
    // static constexpr std::array<int, NSTREAM> rdAddrLut2 = fnCalcRdAddr<ptSizeD1, 1, NSTREAM>();
    // static constexpr std::array<int, NSTREAM> rdBnkLut1 = fnCalcRdBnk<ptSizeD1, 0, NSTREAM>();
    // static constexpr std::array<int, NSTREAM> rdBnkLut2 = fnCalcRdBnk<ptSizeD1, 1, NSTREAM>();

    int rdAddrLut1[NSTREAM][ptSizeD1 / samplesPerRead];
    int rdAddrLut2[NSTREAM][ptSizeD1 / samplesPerRead];
    int rdBnkLut1[NSTREAM][ptSizeD1 / samplesPerRead];
    int rdBnkLut2[NSTREAM][ptSizeD1 / samplesPerRead];

    static constexpr unsigned int numLoadsPtSize = fnCeil<ptSizeD2, SSR>() / SSR; //
    static constexpr unsigned int ptSizeD1Ceil = fnCeil<ptSizeD1, NSTREAM * samplesPerRead>();
    static constexpr unsigned int ptSizeD2Ceil = fnCeil<ptSizeD2, NSTREAM>();
    static constexpr unsigned int numLoads = numLoadsPtSize * ptSizeD1;
    static constexpr unsigned numStores = numLoadsPtSize * ptSizeD1Ceil; // 7
    typedef ap_uint<NBITS> TT_DATA;                                      // Equals two 'cint32' samples
    typedef ap_uint<NBITS / samplesPerRead> TT_SAMPLE;                   // Samples are 'cint32'
    typedef hls::stream<TT_DATA> TT_STREAM;
    typedef TT_SAMPLE buff_t[NSTREAM][numStores];

    void unpack_inputs(TT_STREAM sig_i[NSTREAM], hls::stream_of_blocks<buff_t>& inter_buff) {
        hls::write_lock<buff_t> buff_in(inter_buff);
        TT_SAMPLE trans_i[samplesPerRead * NSTREAM];
#pragma HLS array_partition variable = trans_i dim = 1
        int idx = 0;
    P2_LOOP:
        for (int p2 = 0; p2 < ptSizeD2Ceil / NSTREAM; p2++) {
        P1_LOOP:
            for (int p1 = 0; p1 < ptSizeD1; p1 += samplesPerRead) {
#pragma HLS PIPELINE II = 1
            READ:
                for (int ss = 0; ss < NSTREAM; ss++) {
                    (trans_i[(ss << 1) + 1], trans_i[(ss << 1) + 0]) = sig_i[ss].read();
                    buff_in[ss][idx] = trans_i[(ss << 1) + 0];
                    buff_in[ss][idx + 1] = trans_i[(ss << 1) + 1];
                }
                idx += 2;
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

    WR_D2_LOOP:
        for (int d2 = 0; d2 < ptSizeD2Ceil / NSTREAM; d2++) {
        WR_D1_LOOP:
            for (int d1 = 0; d1 < ptSizeD1Ceil / (NPHASES * samplesPerRead); d1++) {
#pragma HLS PIPELINE II = NPHASES
                rdBankAddr = d1 * ptSizeD2 * samplesPerRead + d2;
            PH_LOOP:
                for (int ph = 0; ph < NPHASES; ph++) {
#pragma HLS unroll
                    if (d1 * (NPHASES * samplesPerRead) + ph * samplesPerRead < ptSizeD1) {
                        for (int ss = 0; ss < NSTREAM; ss++) {
                            rd_addr0[ss] = rdBankAddr + rdAddrLut1[ss][ph];
                            rd_addr1[ss] = rdBankAddr + rdAddrLut2[ss][ph];
                        }
                        for (int ss = 0; ss < NSTREAM; ss++) { // 5
                            trans_o[(ss << 1) + 0] = buff_out[rdBnkLut1[ss][ph]][rd_addr0[ss]];
                            trans_o[(ss << 1) + 1] = buff_out[rdBnkLut2[ss][ph]][rd_addr1[ss]];
                            sig_o[ss].write((trans_o[(ss << 1) + 1], trans_o[(ss << 1) + 0]));
                        }
                    }
                }
            }
        }
    }

    void ifft_front_transpose_top(TT_STREAM sig_i[NSTREAM], TT_STREAM sig_o[NSTREAM]) {
        hls::stream_of_blocks<buff_t> inter_buff;
#pragma HLS array_partition variable = inter_buff dim = 1 type = complete
#pragma HLS bind_storage variable = inter_buff type = RAM_T2P impl = bram
#pragma HLS dependence variable = inter_buff type = inter false
#pragma HLS dependence variable = inter_buff type = intra false
#pragma HLS DATAFLOW
        unpack_inputs(sig_i, inter_buff);
        ifft_load_buff(inter_buff, sig_o);
    }

    frontTransposeCls() {
        for (int ss = 0; ss < NSTREAM; ss++) {
            for (int ph = 0; ph < NPHASES; ph++) {
                rdBnkLut1[ss][ph] = (ss + ph * (samplesPerRead * (ptSizeD2 % NSTREAM))) % NSTREAM; // 0, 4, 8, 12
                rdBnkLut2[ss][ph] = (ss + ph * (samplesPerRead * (ptSizeD2 % NSTREAM)) + (ptSizeD2 % NSTREAM)) %
                                    NSTREAM; // 2, 6, 10, 14
            }
        }

        for (int ss = 0; ss < NSTREAM; ss++) {
            for (int ph = 0; ph < NPHASES; ph++) {
                rdAddrLut1[ss][ph] = (ss + samplesPerRead * ph * ptSizeD2) / NSTREAM;
                rdAddrLut2[ss][ph] = (ss + samplesPerRead * ph * ptSizeD2 + ptSizeD2) / NSTREAM;
            }
        }
    }
};
};

#endif