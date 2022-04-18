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

#ifndef __XILINX_ETHASH_HPP__
#define __XILINX_ETHASH_HPP__

#ifndef __SYNTHESIS__
#include <iostream>
#endif
#include <hls_stream.h>
#include <ap_int.h>
#include "sha3_ethash.hpp"
#include "xf_security/ethash_krl.hpp"

#define BATCH_SIZE 32

void ethash_acc::prefnv(ap_uint<32> full_size,
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

                        vpp_stream_32_1024& s0preStrm,
                        vpp_stream_256_1024& smix0preStrm0,
                        vpp_stream_256_1024& smix0preStrm1,
                        vpp_stream_256_1024& smix1preStrm0,
                        vpp_stream_256_1024& smix1preStrm1) {
#ifndef __SYNTHESIS__
    std::cout << "prefnv start.\n";
#endif
    const unsigned int num_full_pages = full_size; // / page_size;

    ap_uint<32> index;
    ap_uint<32> s0;
    ap_uint<512> smix0;
    ap_uint<512> smix1;
    ap_uint<2> addrH;
    ap_uint<25> addrL;

    for (int k = 0; k < batch_cnt_ap; k++) {
        for (ap_uint<8> iter = 0; iter < 64; iter++) {
            for (ap_uint<32> i = 0; i < BATCH_SIZE; i++) {
#pragma HLS PIPELINE enable_flush II = 1
                if (iter == 0) {
                    ap_uint<256> reg0 = sha3_512_Strm0.read();
                    ap_uint<256> reg1 = sha3_512_Strm1.read();
                    s0 = reg0.range(31, 0);
                    smix0.range(255, 0) = reg0;
                    smix0.range(511, 256) = reg1;
                    smix1.range(255, 0) = reg0;
                    smix1.range(511, 256) = reg1;
                } else {
                    s0 = s0pingStrm.read();
                    smix0 = smix0pingStrm.read();
                    smix1 = smix1pingStrm.read();
                }

                ap_uint<32> fnv_x = s0.range(31, 0) ^ iter;
                ap_uint<32> fnv_t1 = iter[4];
                ap_uint<32> fnv_t2 = iter.range(3, 0);

                ap_uint<512> pre_fnv_y = fnv_t1 == 0 ? smix0 : smix1;

                ap_uint<32> pre_fnv_y_array[16];
#pragma HLS array_partition variable = pre_fnv_y_array dim = 1 complete
                for (int j = 0; j < 16; j++) {
#pragma HLS unroll
                    pre_fnv_y_array[j] = pre_fnv_y.range(j * 32 + 31, j * 32);
                }

                ap_uint<32> fnv_y = pre_fnv_y_array[fnv_t2];
                ap_uint<32> index = special_mod<26>(fnv_hash(fnv_x, fnv_y), num_full_pages);

                ap_uint<12> index_remapL = index.range(11, 0);
                ap_uint<2> index_remapM = index.range(13, 12);
                ap_uint<12> index_remapH = index.range(25, 14);
                ap_uint<12> index_remapHR = index_remapH.reverse();

                addrH = index_remapM;
                addrL.range(23, 0) = index_remapHR.concat(index_remapL);
                addrL[24] = 0;

                if (addrH == 0) {
                    indexpreStrm0.write(addrL);
                } else if (addrH == 1) {
                    indexpreStrm1.write(addrL);
                } else if (addrH == 2) {
                    indexpreStrm2.write(addrL);
                } else if (addrH == 3) {
                    indexpreStrm3.write(addrL);
                }
                orderpreStrm.write(addrH);

                s0preStrm.write(s0);
                smix0preStrm0.write(smix0.range(255, 0));
                smix0preStrm1.write(smix0.range(511, 256));
                smix1preStrm0.write(smix1.range(255, 0));
                smix1preStrm1.write(smix1.range(511, 256));
            }
        }
    }

    addrL[24] = 1;
    indexpreStrm0.write(addrL);
    indexpreStrm1.write(addrL);
    indexpreStrm2.write(addrL);
    indexpreStrm3.write(addrL);
#ifndef __SYNTHESIS__
    std::cout << "prefnv end.\n";
#endif
}

void ethash_acc::nodeLookup0(ap_uint<512>* dram0,
                             vpp_stream_25_1024& indexpreStrm0,
                             vpp_stream_256_32& dagStrm00,
                             vpp_stream_256_32& dagStrm01,
                             vpp_stream_256_32& dagStrm02,
                             vpp_stream_256_32& dagStrm03) {
#ifndef __SYNTHESIS__
    std::cout << "nodeLookup0 start.\n";
#endif
    ap_uint<25> index = 0;

    while (index[24] != 1) {
#pragma HLS PIPELINE enable_flush II = 2
        index = indexpreStrm0.read();
        ap_uint<25> index_for_512 = index << 1;
        ap_uint<512> dag_item[2];
        // limited address space to single HBM bank size (256MB)
        ap_uint<22> limit_addr = index.range(21, 0);
        limit_addr[0] = 0;
        for (int i = 0; i < 2; i++) {
            // dag_item[i] = full_node[index_for_512 + i];
            dag_item[i] = dram0[limit_addr + i];
        }
        ap_uint<1024> combined_dag_item;
        combined_dag_item.range(511, 0) = dag_item[0];
        combined_dag_item.range(1023, 512) = dag_item[1];
        dagStrm00.write(combined_dag_item.range(255, 0));
        dagStrm01.write(combined_dag_item.range(511, 256));
        dagStrm02.write(combined_dag_item.range(767, 512));
        dagStrm03.write(combined_dag_item.range(1023, 768));
    }
#ifndef __SYNTHESIS__
    std::cout << "nodeLookup0 end.\n";
#endif
}

void ethash_acc::nodeLookup1(ap_uint<512>* dram1,
                             vpp_stream_25_1024& indexpreStrm1,
                             vpp_stream_256_32& dagStrm10,
                             vpp_stream_256_32& dagStrm11,
                             vpp_stream_256_32& dagStrm12,
                             vpp_stream_256_32& dagStrm13) {
#ifndef __SYNTHESIS__
    std::cout << "nodeLookup1 start.\n";
#endif
    ap_uint<25> index = 0;

    while (index[24] != 1) {
#pragma HLS PIPELINE enable_flush II = 2
        index = indexpreStrm1.read();
        ap_uint<25> index_for_512 = index << 1;
        ap_uint<512> dag_item[2];
        // limited address space to single HBM bank size (256MB)
        ap_uint<22> limit_addr = index.range(21, 0);
        limit_addr[0] = 0;
        for (int i = 0; i < 2; i++) {
            // dag_item[i] = full_node[index_for_512 + i];
            dag_item[i] = dram1[limit_addr + i];
        }
        ap_uint<1024> combined_dag_item;
        combined_dag_item.range(511, 0) = dag_item[0];
        combined_dag_item.range(1023, 512) = dag_item[1];
        dagStrm10.write(combined_dag_item.range(255, 0));
        dagStrm11.write(combined_dag_item.range(511, 256));
        dagStrm12.write(combined_dag_item.range(767, 512));
        dagStrm13.write(combined_dag_item.range(1023, 768));
    }
#ifndef __SYNTHESIS__
    std::cout << "nodeLookup1 end.\n";
#endif
}

void ethash_acc::nodeLookup2(ap_uint<512>* dram2,
                             vpp_stream_25_1024& indexpreStrm2,
                             vpp_stream_256_32& dagStrm20,
                             vpp_stream_256_32& dagStrm21,
                             vpp_stream_256_32& dagStrm22,
                             vpp_stream_256_32& dagStrm23) {
#ifndef __SYNTHESIS__
    std::cout << "nodeLookup2 start.\n";
#endif
    ap_uint<25> index = 0;

    while (index[24] != 1) {
#pragma HLS PIPELINE enable_flush II = 2
        index = indexpreStrm2.read();
        ap_uint<25> index_for_512 = index << 1;
        ap_uint<512> dag_item[2];
        // limited address space to single HBM bank size (256MB)
        ap_uint<22> limit_addr = index.range(21, 0);
        limit_addr[0] = 0;
        for (int i = 0; i < 2; i++) {
            // dag_item[i] = full_node[index_for_512 + i];
            dag_item[i] = dram2[limit_addr + i];
        }
        ap_uint<1024> combined_dag_item;
        combined_dag_item.range(511, 0) = dag_item[0];
        combined_dag_item.range(1023, 512) = dag_item[1];
        dagStrm20.write(combined_dag_item.range(255, 0));
        dagStrm21.write(combined_dag_item.range(511, 256));
        dagStrm22.write(combined_dag_item.range(767, 512));
        dagStrm23.write(combined_dag_item.range(1023, 768));
    }
#ifndef __SYNTHESIS__
    std::cout << "nodeLookup2 end.\n";
#endif
}

void ethash_acc::nodeLookup3(ap_uint<512>* dram3_ret,
                             vpp_stream_25_1024& indexpreStrm3,
                             vpp_stream_256_32& dagStrm30,
                             vpp_stream_256_32& dagStrm31,
                             vpp_stream_256_32& dagStrm32,
                             vpp_stream_256_32& dagStrm33) {
#ifndef __SYNTHESIS__
    std::cout << "nodeLookup3 start.\n";
#endif
    ap_uint<25> index = 0;

    while (index[24] != 1) {
#pragma HLS PIPELINE enable_flush II = 2
        index = indexpreStrm3.read();
        ap_uint<25> index_for_512 = index << 1;
        ap_uint<512> dag_item[2];
        // limited address space to single HBM bank size (256MB)
        ap_uint<22> limit_addr = index.range(21, 0);
        limit_addr[0] = 0;
        for (int i = 0; i < 2; i++) {
            // dag_item[i] = full_node[index_for_512 + i];
            dag_item[i] = dram3_ret[limit_addr + i];
        }
        ap_uint<1024> combined_dag_item;
        combined_dag_item.range(511, 0) = dag_item[0];
        combined_dag_item.range(1023, 512) = dag_item[1];
        dagStrm30.write(combined_dag_item.range(255, 0));
        dagStrm31.write(combined_dag_item.range(511, 256));
        dagStrm32.write(combined_dag_item.range(767, 512));
        dagStrm33.write(combined_dag_item.range(1023, 768));
    }
#ifndef __SYNTHESIS__
    std::cout << "nodeLookup3 end.\n";
#endif
}

void ethash_acc::postfnv(ap_uint<32> batch_cnt_ap,

                         vpp_stream_32_1024& s0preStream0,
                         vpp_stream_256_1024& smix0preStrm0,
                         vpp_stream_256_1024& smix0preStrm1,
                         vpp_stream_256_1024& smix1preStrm0,
                         vpp_stream_256_1024& smix1preStrm1,

                         vpp_stream_2_1024& orderStrm,
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
                         vpp_stream_256_32& mixStrm11) {
#ifndef __SYNTHESIS__
    std::cout << "postfnv start.\n";
#endif
    for (int k = 0; k < batch_cnt_ap; k++) {
        for (ap_uint<8> iter = 0; iter < 64; iter++) {
            for (ap_uint<32> i = 0; i < BATCH_SIZE; i++) {
#pragma HLS PIPELINE enable_flush II = 1
                ap_uint<2> order = orderStrm.read();
                ap_uint<1024> dag_r;
                if (order == 0) {
                    dag_r.range(255, 0) = dagStrm00.read();
                    dag_r.range(511, 256) = dagStrm01.read();
                    dag_r.range(767, 512) = dagStrm02.read();
                    dag_r.range(1023, 768) = dagStrm03.read();
                } else if (order == 1) {
                    dag_r.range(255, 0) = dagStrm10.read();
                    dag_r.range(511, 256) = dagStrm11.read();
                    dag_r.range(767, 512) = dagStrm12.read();
                    dag_r.range(1023, 768) = dagStrm13.read();
                } else if (order == 2) {
                    dag_r.range(255, 0) = dagStrm20.read();
                    dag_r.range(511, 256) = dagStrm21.read();
                    dag_r.range(767, 512) = dagStrm22.read();
                    dag_r.range(1023, 768) = dagStrm23.read();
                } else if (order == 3) {
                    dag_r.range(255, 0) = dagStrm30.read();
                    dag_r.range(511, 256) = dagStrm31.read();
                    dag_r.range(767, 512) = dagStrm32.read();
                    dag_r.range(1023, 768) = dagStrm33.read();
                }

                ap_uint<512> smix0;
                smix0.range(255, 0) = smix0preStrm0.read();
                smix0.range(511, 256) = smix0preStrm1.read();
                ap_uint<512> smix1;
                smix1.range(255, 0) = smix1preStrm0.read();
                smix1.range(511, 256) = smix1preStrm1.read();

                ap_uint<512> dag0 = dag_r.range(511, 0);
                ap_uint<512> dag1 = dag_r.range(1023, 512);

                ap_uint<512> smix0nxt;
                ap_uint<512> smix1nxt;

                for (int k = 0; k < NODE_WORDS; k++) {
#pragma HLS unroll
                    smix0nxt.range(k * 32 + 31, k * 32) =
                        fnv_hash(smix0.range(k * 32 + 31, k * 32), dag0.range(k * 32 + 31, k * 32));
                }

                for (int k = 0; k < NODE_WORDS; k++) {
#pragma HLS unroll
                    smix1nxt.range(k * 32 + 31, k * 32) =
                        fnv_hash(smix1.range(k * 32 + 31, k * 32), dag1.range(k * 32 + 31, k * 32));
                }

                if (iter == 63) {
                    s0preStream0.read();
                    mixStrm00.write(smix0nxt.range(255, 0));
                    mixStrm01.write(smix0nxt.range(511, 256));
                    mixStrm10.write(smix1nxt.range(255, 0));
                    mixStrm11.write(smix1nxt.range(511, 256));
                } else {
                    ap_uint<32> s0 = s0preStream0.read();
                    s0pongStrm.write(s0);
                    smix0pongStrm.write(smix0nxt);
                    smix1pongStrm.write(smix1nxt);
                }
            }
        }
    }

    dagStrm00.read();
    dagStrm01.read();
    dagStrm02.read();
    dagStrm03.read();
    dagStrm10.read();
    dagStrm11.read();
    dagStrm12.read();
    dagStrm13.read();
    dagStrm20.read();
    dagStrm21.read();
    dagStrm22.read();
    dagStrm23.read();
    dagStrm30.read();
    dagStrm31.read();
    dagStrm32.read();
    dagStrm33.read();
#ifndef __SYNTHESIS__
    std::cout << "postfnv end.\n";
#endif
}

void ethash_acc::pre_sha3_512_40(ap_uint<256> header_hash_ap,
                                 ap_uint<64> nonce_start_ap,
                                 ap_uint<32> batch_cnt_ap,

                                 vpp_stream_256_4096& sha3_512_Strm0,
                                 vpp_stream_256_4096& sha3_512_Strm1) {
#ifndef __SYNTHESIS__
    std::cout << "pre sha3_512_40 start.\n";
#endif
    for (ap_uint<32> i = 0; i < batch_cnt_ap * BATCH_SIZE; i++) {
#pragma HLS PIPELINE off
        ap_uint<512> s0;
        s0.range(255, 0) = header_hash_ap;
        s0.range(319, 256) = nonce_start_ap + i;
        ap_uint<512> tmp = sha3_512_40(s0);
        sha3_512_Strm0.write(tmp.range(255, 0));
        sha3_512_Strm1.write(tmp.range(511, 256));
    }
#ifndef __SYNTHESIS__
    std::cout << "pre sha3_512_40 end.\n";
#endif
}

void ethash_acc::compress(ap_uint<32> batch_cnt_ap,
                          vpp_stream_256_32& mixStrm00,
                          vpp_stream_256_32& mixStrm01,
                          vpp_stream_256_32& mixStrm10,
                          vpp_stream_256_32& mixStrm11,

                          vpp_stream_256_4096& compressStrm) {
#ifndef __SYNTHESIS__
    std::cout << "compress start.\n";
#endif
    ap_uint<512> s_mix[2] = {0, 0};
    ap_uint<32> s0;
    ap_uint<256> res_hash;

    for (int i = 0; i < batch_cnt_ap * BATCH_SIZE; i++) {
#pragma HLS PIPELINE II = 1
        s_mix[0].range(255, 0) = mixStrm00.read();
        s_mix[0].range(511, 256) = mixStrm01.read();
        s_mix[1].range(255, 0) = mixStrm10.read();
        s_mix[1].range(511, 256) = mixStrm11.read();

        for (int w = 0; w < MIX_WORDS; w += 4) {
#pragma HLS unroll
            const ap_uint<32> t1 = w / NODE_WORDS;
            const ap_uint<32> t2 = w % NODE_WORDS;
            const ap_uint<512> local_mix = t1 == 0 ? s_mix[0] : s_mix[1];

            ap_uint<32> reduction = local_mix.range(t2 * 32 + 31, t2 * 32);
            reduction = fnv_hash(reduction, local_mix.range((t2 + 1) * 32 + 31, (t2 + 1) * 32));
            reduction = fnv_hash(reduction, local_mix.range((t2 + 2) * 32 + 31, (t2 + 2) * 32));
            reduction = fnv_hash(reduction, local_mix.range((t2 + 3) * 32 + 31, (t2 + 3) * 32));
            res_hash.range((w / 4) * 32 + 31, (w / 4) * 32) = reduction;
        }
        compressStrm.write(res_hash);
    }
#ifndef __SYNTHESIS__
    std::cout << "compress end.\n";
#endif
}

void ethash_acc::post_sha3_512_40(ap_uint<256> header_hash_ap,
                                  ap_uint<64> nonce_start_ap,
                                  ap_uint<32> batch_cnt_ap,
                                  vpp_stream_256_4096& compressStrm,

                                  vpp_stream_256_32& sha512Strm0,
                                  vpp_stream_256_32& sha512Strm1,
                                  vpp_stream_256_32& mixStrm) {
#ifndef __SYNTHESIS__
    std::cout << "post sha3_512_40 start.\n";
#endif
    for (int i = 0; i < batch_cnt_ap * BATCH_SIZE; i++) {
#pragma HLS PIPELINE off
        ap_uint<512> s0;
        s0.range(255, 0) = header_hash_ap;
        s0.range(319, 256) = nonce_start_ap + i;
        ap_uint<512> tmp = sha3_512_40(s0);

        sha512Strm0.write(tmp.range(255, 0));
        sha512Strm1.write(tmp.range(511, 256));
        mixStrm.write(compressStrm.read());
    }
#ifndef __SYNTHESIS__
    std::cout << "post sha3_512_40 end.\n";
#endif
}

void ethash_acc::sha3_256_96_unit(ap_uint<32> batch_cnt_ap,
                                  vpp_stream_256_32& sha512Strm0,
                                  vpp_stream_256_32& sha512Strm1,
                                  vpp_stream_256_32& mixStrm,

                                  vpp_stream_256_32& mixhashStrm,
                                  vpp_stream_256_32& resultStrm) {
#ifndef __SYNTHESIS__
    std::cout << "sha3_256_96 start.\n";
#endif
    for (int i = 0; i < batch_cnt_ap * BATCH_SIZE; i++) {
#pragma HLS PIPELINE off
        ap_uint<512> s0;
        ap_uint<512> mix;
        ap_uint<32> nonce;
        s0.range(255, 0) = sha512Strm0.read();
        s0.range(511, 256) = sha512Strm1.read();
        mix.range(255, 0) = mixStrm.read();
        mix.range(511, 256) = 0;
        ap_uint<256> tmp = sha3_256_96(s0, mix);
        mixhashStrm.write(mix.range(255, 0));
        resultStrm.write(tmp);
    }
#ifndef __SYNTHESIS__
    std::cout << "sha3_256_96 end.\n";
#endif
}

void ethash_acc::check(ap_uint<32> batch_cnt_ap,
                       unsigned index,
                       vpp_stream_256_32& mixhashStrm,
                       vpp_stream_256_32& resultStrm,
                       ap_uint<512>* dram3_ret) {
#ifndef __SYNTHESIS__
    std::cout << "check start.\n";
#endif
    ap_uint<32> sum = 0;
    for (int i = 0; i < batch_cnt_ap * BATCH_SIZE; i++) {
#pragma HLS PIPELINE II = 1
        ap_uint<512> tmp;
        tmp.range(255, 0) = resultStrm.read();
        tmp.range(511, 256) = mixhashStrm.read();
        sum = sum + tmp.range(31, 0);
    }
    dram3_ret[(256UL * 1024UL * 1024UL - 4096UL) / 64].range((index + 1) * 32 - 1, index * 32) = sum;
#ifndef __SYNTHESIS__
    std::cout << "check end.\n";
#endif
}

void ethash_acc::fsk_passback(vpp_stream_32_4096& s0pongStrm,
                              vpp_stream_512_4096& smix0pongStrm,
                              vpp_stream_512_4096& smix1pongStrm,

                              vpp_stream_32_4096& s0pingStrm,
                              vpp_stream_512_4096& smix0pingStrm,
                              vpp_stream_512_4096& smix1pingStrm) {
#pragma HLS PIPELINE enable_flush II = 1
    ap_uint<32> s0 = s0pongStrm.read();
    ap_uint<512> smix0 = smix0pongStrm.read();
    ap_uint<512> smix1 = smix1pongStrm.read();
    s0pingStrm.write(s0);
    smix0pingStrm.write(smix0);
    smix1pingStrm.write(smix1);
}

void ethash_acc::compute(unsigned index,
                         unsigned full_size,
                         ap_uint<256> header_hash,
                         ap_uint<64> nonce_start,
                         unsigned batch_cnt,

                         ap_uint<512>* dram0,
                         ap_uint<512>* dram1,
                         ap_uint<512>* dram2,
                         ap_uint<512>* dram3_ret) {
    ap_uint<32> full_size_ap = full_size;
    ap_uint<256> header_hash_ap = header_hash;
    ap_uint<64> nonce_start_ap = nonce_start;
    ap_uint<32> batch_cnt_ap = batch_cnt;

    STATIC vpp_stream_256_4096 sha3_512_Strm0;
    STATIC vpp_stream_256_4096 sha3_512_Strm1;

    pre_sha3_512_40(header_hash_ap, nonce_start_ap, batch_cnt_ap, sha3_512_Strm0, sha3_512_Strm1);

    STATIC vpp_stream_256_32 mixStrm00;
    STATIC vpp_stream_256_32 mixStrm01;
    STATIC vpp_stream_256_32 mixStrm10;
    STATIC vpp_stream_256_32 mixStrm11;

    STATIC vpp_stream_32_4096 s0pingStrm;
    STATIC vpp_stream_512_4096 smix0pingStrm;
    STATIC vpp_stream_512_4096 smix1pingStrm;
    STATIC vpp_stream_32_4096 s0pongStrm;
    STATIC vpp_stream_512_4096 smix0pongStrm;
    STATIC vpp_stream_512_4096 smix1pongStrm;

    STATIC vpp_stream_2_1024 orderpreStrm;
    STATIC vpp_stream_25_1024 indexpreStrm0;
    STATIC vpp_stream_25_1024 indexpreStrm1;
    STATIC vpp_stream_25_1024 indexpreStrm2;
    STATIC vpp_stream_25_1024 indexpreStrm3;
    STATIC vpp_stream_32_1024 s0preStrm0;
    STATIC vpp_stream_256_1024 smix0preStrm0;
    STATIC vpp_stream_256_1024 smix0preStrm1;
    STATIC vpp_stream_256_1024 smix1preStrm0;
    STATIC vpp_stream_256_1024 smix1preStrm1;

    prefnv(full_size, batch_cnt_ap, sha3_512_Strm0, sha3_512_Strm1, s0pingStrm, smix0pingStrm, smix1pingStrm,
           orderpreStrm, indexpreStrm0, indexpreStrm1, indexpreStrm2, indexpreStrm3, s0preStrm0, smix0preStrm0,
           smix0preStrm1, smix1preStrm0, smix1preStrm1);

    STATIC vpp_stream_256_32 dagStrm00;
    STATIC vpp_stream_256_32 dagStrm01;
    STATIC vpp_stream_256_32 dagStrm02;
    STATIC vpp_stream_256_32 dagStrm03;
    STATIC vpp_stream_256_32 dagStrm10;
    STATIC vpp_stream_256_32 dagStrm11;
    STATIC vpp_stream_256_32 dagStrm12;
    STATIC vpp_stream_256_32 dagStrm13;
    STATIC vpp_stream_256_32 dagStrm20;
    STATIC vpp_stream_256_32 dagStrm21;
    STATIC vpp_stream_256_32 dagStrm22;
    STATIC vpp_stream_256_32 dagStrm23;
    STATIC vpp_stream_256_32 dagStrm30;
    STATIC vpp_stream_256_32 dagStrm31;
    STATIC vpp_stream_256_32 dagStrm32;
    STATIC vpp_stream_256_32 dagStrm33;

    nodeLookup0(dram0, indexpreStrm0, dagStrm00, dagStrm01, dagStrm02, dagStrm03);
    nodeLookup1(dram1, indexpreStrm1, dagStrm10, dagStrm11, dagStrm12, dagStrm13);
    nodeLookup2(dram2, indexpreStrm2, dagStrm20, dagStrm21, dagStrm22, dagStrm23);
    nodeLookup3(dram3_ret, indexpreStrm3, dagStrm30, dagStrm31, dagStrm32, dagStrm33);

    postfnv(batch_cnt_ap, s0preStrm0, smix0preStrm0, smix0preStrm1, smix1preStrm0, smix1preStrm1, orderpreStrm,
            dagStrm00, dagStrm01, dagStrm02, dagStrm03, dagStrm10, dagStrm11, dagStrm12, dagStrm13, dagStrm20,
            dagStrm21, dagStrm22, dagStrm23, dagStrm30, dagStrm31, dagStrm32, dagStrm33, s0pongStrm, smix0pongStrm,
            smix1pongStrm, mixStrm00, mixStrm01, mixStrm10, mixStrm11);

    fsk_passback(s0pongStrm, smix0pongStrm, smix1pongStrm, s0pingStrm, smix0pingStrm, smix1pingStrm);

    STATIC vpp_stream_256_4096 compressStrm;

    compress(batch_cnt_ap, mixStrm00, mixStrm01, mixStrm10, mixStrm11, compressStrm);

    STATIC vpp_stream_256_32 sha512Strm0;
    STATIC vpp_stream_256_32 sha512Strm1;
    STATIC vpp_stream_256_32 mixStrm;

    post_sha3_512_40(header_hash_ap, nonce_start_ap, batch_cnt_ap, compressStrm, sha512Strm0, sha512Strm1, mixStrm);

    STATIC vpp_stream_256_32 mixhashStrm("mixhash");
    STATIC vpp_stream_256_32 resultStrm("result");

    sha3_256_96_unit(batch_cnt_ap, sha512Strm0, sha512Strm1, mixStrm, mixhashStrm, resultStrm);

    check(batch_cnt_ap, index, mixhashStrm, resultStrm, dram3_ret);
}

#endif
