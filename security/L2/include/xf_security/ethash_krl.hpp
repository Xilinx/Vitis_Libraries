/*
 * Copyright 2022 Xilinx, Inc.
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

#ifndef _XF_SECURITY_ETHASH_KERNEL_HPP_
#define _XF_SECURITY_ETHASH_KERNEL_HPP_

#pragma once

#include "vpp_acc.hpp"
#include "vpp_stream.hpp"
#include "ap_int.h"

const int NODE_WORDS = 64 / 4; // 16
const int ETHASH_MIX_BYTES = 128;
const int MIX_WORDS = ETHASH_MIX_BYTES / 4;   // 32
const int MIX_NODES = MIX_WORDS / NODE_WORDS; // 2
const int CHANNEL_NODES = (1 << 28) / 64;

// WA for XidanePass memory blowup for static streams
#ifdef TARGET_FLOW_sw_emu
#define STATIC static
#else
#define STATIC
#endif

typedef vpp::stream<ap_uint<2>, 1024> vpp_stream_2_1024;
typedef vpp::stream<ap_uint<25>, 1024> vpp_stream_25_1024;
typedef vpp::stream<ap_uint<32>, 1024> vpp_stream_32_1024;
typedef vpp::stream<ap_uint<32>, 4096> vpp_stream_32_4096;
typedef vpp::stream<ap_uint<256>, 32> vpp_stream_256_32;
typedef vpp::stream<ap_uint<256>, 1024> vpp_stream_256_1024;
typedef vpp::stream<ap_uint<256>, 4096> vpp_stream_256_4096;
typedef vpp::stream<ap_uint<512>, 4096> vpp_stream_512_4096;

/**
 *
 * @param x input operand x.
 * @param y input operand y.
 *
 * @return (x * FNV_PRIME) xor y.
 *
 */
static ap_uint<32> fnv_hash(ap_uint<32> x, ap_uint<32> y) {
    const ap_uint<25> FNV_PRIME = 0x01000193;
    ap_uint<32> tmp = x * FNV_PRIME;
    return tmp ^ y;
}

/**
 *
 * @tparam W bit width of b.
 *
 * @param a input operand a.
 * @param b input operand b.
 *
 * @return a % b, but b[24] must be 1.
 *
 */
template <int W>
inline ap_uint<W> special_mod(ap_uint<32> a, ap_uint<W> b) {
    ap_uint<33> tmp = a;
    for (int i = 0; i < (33 - W); i++) {
        if (tmp.range(32, 32 - W) >= b) {
            tmp.range(32, 32 - W) = tmp.range(32, 32 - W) - b;
        }
        tmp <<= 1;
    }
    return tmp.range(32, 33 - W);
}

class ethash_acc : public VPP_ACC<ethash_acc, 2> { // number of CU
    // binding specific device buffer to a specific off-chip memory bank
    ZERO_COPY(dram0);
    ZERO_COPY(dram1);
    ZERO_COPY(dram2);
    ZERO_COPY(dram3_ret);

    FREE_RUNNING(fsk_passback);

    SYS_PORT(dram0, DDR[0]);
    SYS_PORT(dram1, DDR[1]);
    SYS_PORT(dram2, DDR[2]);
    SYS_PORT(dram3_ret, DDR[3]);

    SYS_PORT_PFM(u50, dram0, (HBM[0] : HBM[4]));
    SYS_PORT_PFM(u50, dram1, (HBM[1] : HBM[5]));
    SYS_PORT_PFM(u50, dram2, (HBM[2] : HBM[6]));
    SYS_PORT_PFM(u50, dram3_ret, (HBM[3] : HBM[7]));

    SYS_PORT_PFM(u55n, dram0, (HBM[0] : HBM[4]));
    SYS_PORT_PFM(u55n, dram1, (HBM[1] : HBM[5]));
    SYS_PORT_PFM(u55n, dram2, (HBM[2] : HBM[6]));
    SYS_PORT_PFM(u55n, dram3_ret, (HBM[3] : HBM[7]));

    ASSIGN_SLR(prefnv, (SLR0 : SLR1));
    ASSIGN_SLR(nodeLookup0, (SLR0 : SLR1));
    ASSIGN_SLR(nodeLookup1, (SLR0 : SLR1));
    ASSIGN_SLR(nodeLookup2, (SLR0 : SLR1));
    ASSIGN_SLR(nodeLookup3, (SLR0 : SLR1));
    ASSIGN_SLR(postfnv, (SLR0 : SLR1));
    ASSIGN_SLR(pre_sha3_512_40, (SLR0 : SLR1));
    ASSIGN_SLR(compress, (SLR0 : SLR1));
    ASSIGN_SLR(post_sha3_512_40, (SLR0 : SLR1));
    ASSIGN_SLR(sha3_256_96_unit, (SLR0 : SLR1));
    ASSIGN_SLR(check, (SLR0 : SLR1));
    ASSIGN_SLR(fsk_passback, (SLR0 : SLR1));

   public:
    static void prefnv(ap_uint<32> full_size,
                       ap_uint<32> batch_cnt_ap,

                       vpp_stream_256_4096& sha3_512_Strm0,
                       vpp_stream_256_4096& sha3_512_Strm1,

                       vpp_stream_32_4096& s0pingStrm,
                       vpp_stream_512_4096& smix0pingStrm,
                       vpp_stream_512_4096& smix1pingStrm,

                       vpp_stream_2_1024& orderpreStrm,
                       vpp_stream_25_1024& indexpreStrm0,
                       vpp_stream_25_1024& indexpreStrm1,
                       vpp_stream_25_1024& indexpreStrm2,
                       vpp_stream_25_1024& indexpreStrm3,

                       vpp_stream_32_1024& s0preStrm0,
                       vpp_stream_256_1024& smix0preStrm0,
                       vpp_stream_256_1024& smix0preStrm1,
                       vpp_stream_256_1024& smix1preStrm0,
                       vpp_stream_256_1024& smix1preStrm1);

    static void nodeLookup0(ap_uint<512>* dram0,
                            vpp_stream_25_1024& indexpreStrm0,
                            vpp_stream_256_32& dagStrm00,
                            vpp_stream_256_32& dagStrm01,
                            vpp_stream_256_32& dagStrm02,
                            vpp_stream_256_32& dagStrm03);

    static void nodeLookup1(ap_uint<512>* dram1,
                            vpp_stream_25_1024& indexpreStrm1,
                            vpp_stream_256_32& dagStrm10,
                            vpp_stream_256_32& dagStrm11,
                            vpp_stream_256_32& dagStrm12,
                            vpp_stream_256_32& dagStrm13);

    static void nodeLookup2(ap_uint<512>* dram2,
                            vpp_stream_25_1024& indexpreStrm2,
                            vpp_stream_256_32& dagStrm20,
                            vpp_stream_256_32& dagStrm21,
                            vpp_stream_256_32& dagStrm22,
                            vpp_stream_256_32& dagStrm23);

    static void nodeLookup3(ap_uint<512>* dram3_ret,
                            vpp_stream_25_1024& indexpreStrm3,
                            vpp_stream_256_32& dagStrm30,
                            vpp_stream_256_32& dagStrm31,
                            vpp_stream_256_32& dagStrm32,
                            vpp_stream_256_32& dagStrm33);

    static void postfnv(ap_uint<32> batch_cnt_ap,

                        vpp_stream_32_1024& s0preStrm0,
                        vpp_stream_256_1024& smix0preStrm0,
                        vpp_stream_256_1024& smix0preStrm1,
                        vpp_stream_256_1024& smix1preStrm0,
                        vpp_stream_256_1024& smix1preStrm1,

                        vpp_stream_2_1024& orderpreStrm,
                        vpp_stream_256_32& dagStrm00,
                        vpp_stream_256_32& dagStrm01,
                        vpp_stream_256_32& dagStrm02,
                        vpp_stream_256_32& dagStrm03,

                        vpp_stream_256_32& dagStrm10,
                        vpp_stream_256_32& dagStrm11,
                        vpp_stream_256_32& dagStrm12,
                        vpp_stream_256_32& dagStrm13,

                        vpp_stream_256_32& dagStrm20,
                        vpp_stream_256_32& dagStrm21,
                        vpp_stream_256_32& dagStrm22,
                        vpp_stream_256_32& dagStrm23,

                        vpp_stream_256_32& dagStrm30,
                        vpp_stream_256_32& dagStrm31,
                        vpp_stream_256_32& dagStrm32,
                        vpp_stream_256_32& dagStrm33,

                        vpp_stream_32_4096& s0pongStrm,
                        vpp_stream_512_4096& smix0pongStrm,
                        vpp_stream_512_4096& smix1pongStrm,

                        vpp_stream_256_32& mixStrm00,
                        vpp_stream_256_32& mixStrm01,
                        vpp_stream_256_32& mixStrm10,
                        vpp_stream_256_32& mixStrm11);

    static void pre_sha3_512_40(ap_uint<256> header_hash_ap,
                                ap_uint<64> nonce_start_ap,
                                ap_uint<32> batch_cnt_ap,

                                vpp_stream_256_4096& sha3_512_Strm0,
                                vpp_stream_256_4096& sha3_512_Strm1);

    static void compress(ap_uint<32> batch_cnt_ap,
                         vpp_stream_256_32& mixStrm00,
                         vpp_stream_256_32& mixStrm01,
                         vpp_stream_256_32& mixStrm10,
                         vpp_stream_256_32& mixStrm11,

                         vpp_stream_256_4096& compressStrm);

    static void post_sha3_512_40(ap_uint<256> header_hash_ap,
                                 ap_uint<64> nonce_start_ap,
                                 ap_uint<32> batch_cnt_ap,
                                 vpp_stream_256_4096& compressStrm,

                                 vpp_stream_256_32& sha512Strm0,
                                 vpp_stream_256_32& sha512Strm1,
                                 vpp_stream_256_32& mixStrm);

    static void sha3_256_96_unit(ap_uint<32> batch_cnt_ap,
                                 vpp_stream_256_32& sha512Strm0,
                                 vpp_stream_256_32& sha512Strm1,
                                 vpp_stream_256_32& mixStrm,

                                 vpp_stream_256_32& mixhashStrm,
                                 vpp_stream_256_32& resultStrm);

    static void check(ap_uint<32> batch_cnt_ap,
                      unsigned index,
                      vpp_stream_256_32& mixhashStrm,
                      vpp_stream_256_32& resultStrm,
                      ap_uint<512>* dram3_ret);

    static void fsk_passback(vpp_stream_32_4096& s0pongStrm,
                             vpp_stream_512_4096& smix0pongStrm,
                             vpp_stream_512_4096& smix1pongStrm,

                             vpp_stream_32_4096& s0pingStrm,
                             vpp_stream_512_4096& smix0pingStrm,
                             vpp_stream_512_4096& smix1pingStrm);

    static void compute(unsigned index,
                        unsigned full_size,
                        ap_uint<256> header_hash,
                        ap_uint<64> nonce_start,
                        unsigned batch_cnt,
                        ap_uint<512>* dram0,
                        ap_uint<512>* dram1,
                        ap_uint<512>* dram2,
                        ap_uint<512>* dram3_ret);
};

#endif // _XF_SECURITY_ETHASH_KERNEL_HPP_
