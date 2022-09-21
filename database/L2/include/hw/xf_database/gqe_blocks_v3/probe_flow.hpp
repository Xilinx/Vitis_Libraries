/*
 * Copyright 2021 Xilinx, Inc.
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

#ifndef GQE_ISV_PROBE_FLOW_HPP
#define GQE_ISV_PROBE_FLOW_HPP

#include <hls_burst_maxi.h>

#include "xf_database/hash_multi_join_build_probe.hpp"
#include "xf_database/gqe_blocks_v3/gqe_traits.hpp"
#include "xf_database/gqe_blocks_v3/gqe_enums.hpp"
#include "xf_database/gqe_blocks_v3/demux_and_mux.hpp"
#include "xf_database/gqe_blocks_v3/uram_cache.hpp"
// share internal read_stb
#include "xf_database/gqe_blocks_v3/build_flow.hpp"

#ifndef __SYNTHESIS__
#include <iostream>
#endif

namespace xf {
namespace database {
namespace details {

/// @brief calculate multi-hashing
template <int HASHWH, int HASHWL, int KEYW, int PW>
void multi_hashing(hls::stream<ap_uint<8> >& i_general_cfg_strm,
                   hls::stream<ap_uint<36> >& i_bf_cfg_strm,

                   // input
                   hls::stream<ap_uint<HASHWL> >& i_hash_strm,
                   hls::stream<ap_uint<KEYW> >& i_key_strm,
                   hls::stream<ap_uint<PW> >& i_pld_strm,
                   hls::stream<bool>& i_e_strm,

                   // output
                   hls::stream<ap_uint<HASHWL> >& o_hash_strm,
                   hls::stream<ap_uint<KEYW> >& o_key_strm,
                   hls::stream<ap_uint<PW> >& o_pld_strm,
                   hls::stream<ap_uint<PW> >& o_bf_info_strm,
                   hls::stream<bool>& o_to_strm,
                   hls::stream<bool>& o_e_strm) {
    ap_uint<PW - 36> cacheLineBaseOffset;
    ap_uint<3> blockOffset[4];
#pragma HLS array_partition variable = blockOffset dim = 1
    ap_uint<6> bitPos[4];
#pragma HLS array_partition variable = bitPos dim = 1
    ap_uint<32> hash1, hash2, firstHash, totalBlockCountEach, blockIdx, blockIdxEach;
    bool to_overflow;

    ap_uint<8> general_cfg = i_general_cfg_strm.read();
    bool bf_on = general_cfg[2];
    ap_uint<36> bf_size_in_bits = i_bf_cfg_strm.read();

    bool last = i_e_strm.read();
    while (!last) {
#pragma HLS pipeline II = 1
        ap_uint<HASHWL> hash_val_tmp = i_hash_strm.read();
        ap_uint<KEYW> key = i_key_strm.read();
        ap_uint<PW> pld = i_pld_strm.read();
        last = i_e_strm.read();

        // multi-hashing
        ap_uint<HASHWL + 1> each_size = (bf_size_in_bits >> HASHWH);

        if (bf_on) {
            hash1 = hash_val_tmp.range(31, 0);
            hash2 = 0;
            hash2.range(31 - HASHWH, 0) = hash_val_tmp.range(HASHWL - 1, 32);
            firstHash = hash1 + hash2;
            totalBlockCountEach = each_size / 64;
            blockIdx = firstHash % totalBlockCountEach;
            // each HBM handle 1/2 of the probe
            to_overflow = blockIdx < (totalBlockCountEach / 2);
            if (!to_overflow) {
                blockIdxEach = blockIdx - totalBlockCountEach / 2;
            } else {
                blockIdxEach = blockIdx;
            }
            cacheLineBaseOffset = (ap_uint<PW - 36>)((blockIdxEach >> 3) << 1);
            for (int i = 0; i < 4; i++) {
#pragma HLS unroll
                ap_uint<32> combinedHash = hash1 + ((i + 2) * hash2);
                blockOffset[i] = combinedHash & 0x00000007;
                bitPos[i] = (combinedHash >> 3) & 63;
            }
        }

        o_hash_strm.write(hash_val_tmp);
        o_key_strm.write(key);
        o_pld_strm.write(pld);
        ap_uint<PW> bf_info = (cacheLineBaseOffset, blockOffset[3], bitPos[3], blockOffset[2], bitPos[2],
                               blockOffset[1], bitPos[1], blockOffset[0], bitPos[0]);
        o_bf_info_strm.write(bf_info);
        o_to_strm.write(to_overflow);
        o_e_strm.write(false);
    }
    o_e_strm.write(true);
}

/// @brief prepare for PART
template <int HASHWH, int HASHWL, int KEYW, int PW, int ARW>
void prepare_part(hls::stream<ap_uint<8> >& i_general_cfg_strm,
                  hls::stream<ap_uint<15> >& i_part_cfg_strm,

                  hls::stream<ap_uint<HASHWL> >& i_hash_strm,
                  hls::stream<ap_uint<KEYW> >& i_key_strm,
                  hls::stream<ap_uint<PW> >& i_pld_strm,
                  hls::stream<bool>& i_e_strm,

                  hls::stream<ap_uint<Log2<256>::value - HASHWH> >& o_bucket_idx_strm,
                  hls::stream<ap_uint<ARW - HASHWH> >& o_waddr_strm,
                  hls::stream<ap_uint<KEYW + PW> >& o_wdata_strm,
                  hls::stream<ap_uint<11> >& o_bucket_idx_cnt_strm,
                  hls::stream<bool>& o_e_strm) {
    ap_uint<8> general_cfg = i_general_cfg_strm.read();
    bool part_on = general_cfg[3];
    ap_uint<15> part_cfg = i_part_cfg_strm.read();
    ap_uint<11> k_depth = part_cfg.range(14, 4);
    const ap_uint<10> depth = k_depth % 1024;
    const int bit_num = part_cfg.range(3, 0) - HASHWH;

    // define the counter for each bucket
    ap_uint<10> bucket_cnt[256];
#pragma HLS array_partition variable = bucket_cnt complete dim = 0

    if (part_on) {
        for (int i = 0; i < 256; i++) {
#pragma HLS pipeline II = 1
            bucket_cnt[i] = 0;
        }

        ap_uint<Log2<256>::value - HASHWH> bucket_idx = 0;

        bool last = i_e_strm.read();
        while (!last) {
#pragma HLS pipeline II = 1

            ap_uint<KEYW> key = i_key_strm.read();
            ap_uint<PW> pld = i_pld_strm.read();
            ap_uint<HASHWL> hash_val = i_hash_strm.read();

            if (bit_num > 0) {
                bucket_idx = hash_val(bit_num - 1, 0);
            } else {
                bucket_idx = 0;
            }

            // update according bucket cnt + 1
            ap_uint<11> bucket_reg = bucket_cnt[bucket_idx];

            // generate waddr wdata
            ap_uint<ARW - HASHWH> waddr = bucket_idx * depth * 2 + bucket_reg;
            ap_uint<KEYW + PW> wdata = (pld, key);

            o_bucket_idx_strm.write(bucket_idx);
            o_waddr_strm.write(waddr);
            o_wdata_strm.write(wdata);
            o_bucket_idx_cnt_strm.write(bucket_reg + 1);

            bucket_cnt[bucket_idx] = bucket_reg + 1;
            o_e_strm.write(false);

            last = i_e_strm.read();
        }
        o_e_strm.write(true);
    }
}

/// @brief PART access URAM logic wrapper
template <int HASHWL, int HASHWH, int HASHWJ, int KEYW, int PW, int ARW, int NUM_X, int NUM_Y>
void part_flow(const ap_uint<10> part_depth,
               const int bit_num,
               const int BK,
               hls::stream<ap_uint<Log2<256>::value - HASHWH> >& i_bucket_idx_strm,
               hls::stream<ap_uint<ARW - HASHWH> >& i_waddr_strm,
               hls::stream<ap_uint<KEYW + PW> >& i_wdata_strm,
               hls::stream<ap_uint<11> >& i_bucket_idx_cnt_strm,
               hls::stream<bool>& i_part_e_strm,

               hls::stream<ap_uint<KEYW + PW> >& o_hp_kpld_strm,
               hls::stream<ap_uint<10> >& o_hp_nm_strm,
               hls::stream<ap_uint<10> >& o_hp_bk_strm,

               ap_uint<72> (*uram_buffer0)[NUM_X][NUM_Y][4096],
               ap_uint<72> (*uram_buffer1)[NUM_X][NUM_Y][4096],
               ap_uint<72> (*uram_buffer2)[NUM_X][NUM_Y][4096]) {
#pragma HLS INLINE off

    const int HASH_DEPTH = (1 << HASHWJ) / 3 + 1;
    UramCache<72, HASH_DEPTH, 8, NUM_X, NUM_Y> uram_inst0(uram_buffer0);
    UramCache<72, HASH_DEPTH, 8, NUM_X, NUM_Y> uram_inst1(uram_buffer1);
    UramCache<72, HASH_DEPTH, 8, NUM_X, NUM_Y> uram_inst2(uram_buffer2);

    ap_uint<10> rcnt = 0; // counter for uram read
    ap_uint<HASHWL + 1> bk_num;
    bool ren = false;

    hls::stream<ap_uint<Log2<256>::value + 1 - HASHWH> > wlist_strm;
#pragma HLS stream variable = wlist_strm depth = 256

    bool bucket_flag[256 / (1 << HASHWH)][2];
#pragma HLS array_partition variable = bucket_flag dim = 0
    ap_uint<10> bucket_cnt_copy[256 / (1 << HASHWH)];

    for (int i = 0; i < 256 / (1 << HASHWH); i++) {
#pragma HLS pipeline
        bucket_flag[i][0] = 0;
        bucket_flag[i][1] = 0;
        bucket_cnt_copy[i] = 0;
    }

    ap_uint<Log2<256>::value - HASHWH> bucket_idx;
    ap_uint<ARW - HASHWH> waddr;
    ap_uint<KEYW + PW> wdata;
    ap_uint<11> bucket_idx_cnt;
    ap_uint<1> pingpong;

    bool isNonBlocking = true;

    bool last = i_part_e_strm.read();
BU_WLOOP:
    while (!last) {
#pragma HLS pipeline II = 1
#pragma HLS dependence variable = uram_inst0.blocks inter false
#pragma HLS dependence variable = uram_inst1.blocks inter false
#pragma HLS dependence variable = uram_inst2.blocks inter false
        // write to uram
        if (isNonBlocking) {
            last = i_part_e_strm.read();

            bucket_idx = i_bucket_idx_strm.read();
            waddr = i_waddr_strm.read();
            wdata = i_wdata_strm.read();
            bucket_idx_cnt = i_bucket_idx_cnt_strm.read();
            pingpong = bucket_idx_cnt > 512 ? 1 : 0;
        }
#ifdef USER_DEBUG
        std::cout << "bucket_idx: " << bucket_idx << ", ";
        std::cout << "bucket_idx_cnt: " << bucket_idx_cnt << ", ";
        std::cout << "data: " << wdata.range(63, 0) << ", ";
        std::cout << "pingpong: " << pingpong << std::endl;
#endif

        if (bucket_flag[bucket_idx][pingpong] == 1) {
            isNonBlocking = false;
        } else {
            isNonBlocking = true;
        }

        if (isNonBlocking) {
            if ((bucket_idx_cnt == part_depth) || (bucket_idx_cnt == 2 * part_depth)) {
                bucket_flag[bucket_idx][pingpong] = 1;
                wlist_strm.write((bucket_idx, pingpong));
#ifdef USER_DEBUG
                std::cout << "flag[" << bucket_idx << "][" << pingpong << "]: " << bucket_flag[bucket_idx][pingpong]
                          << std::endl;
#endif
            }
            bucket_cnt_copy[bucket_idx] = bucket_idx_cnt;

            uram_inst0.write(waddr, wdata.range(63, 0));
            uram_inst1.write(waddr, wdata.range(127, 64));
            uram_inst2.write(waddr, wdata.range(191, 128));
        }

        // read wlist data out
        if (!wlist_strm.empty() && rcnt == 0) {
            bk_num = wlist_strm.read();
            ren = true;
        } else {
            ren = false;
        }

        ap_uint<HASHWL + 1> bksize = bk_num(bit_num, 0);

        ap_uint<HASHWL> bksize1 = 0;
        if (bit_num > 0) {
            bksize1 = bk_num(bit_num, 1);
        } else {
            bksize1 = 0;
        }

        ap_uint<1> rsector = bk_num[0];

        if (ren || (rcnt != 0)) { // write and read simulatously
            ap_uint<ARW - HASHWH> raddr = bksize * part_depth + rcnt;
            ap_uint<64> tmp0 = uram_inst0.read(raddr);
            ap_uint<64> tmp1 = uram_inst1.read(raddr);
            ap_uint<64> tmp2 = uram_inst2.read(raddr);
            ap_uint<KEYW + PW> rdata = 0;
            rdata.range(63, 0) = tmp0;
            rdata.range(127, 64) = tmp1;
            rdata.range(191, 128) = tmp2;

            o_hp_kpld_strm.write(rdata);
            if (rcnt == part_depth - 1) {
                o_hp_nm_strm.write(part_depth);
                o_hp_bk_strm.write(bksize1);
                rcnt = 0;
                bucket_flag[bksize1][rsector] = 0;
#ifdef USER_DEBUG
                std::cout << "clear flag[" << bksize1 << "][" << rsector << "]" << std::endl;
#endif
            } else {
                rcnt++;
            }
        }

    } // end while loop

#ifdef USER_DEBUG
    std::cout << "wlist_strm.size() = " << wlist_strm.size() << ", rcnt = " << rcnt << std::endl;
#endif

    ap_uint<2> state = (rcnt != 0) ? (ap_uint<2>)0 : (!wlist_strm.empty() ? (ap_uint<2>)1 : (ap_uint<2>)2);
    int bk_ptr = 0;
    do {
        ap_uint<10> dl, ul;
        ap_uint<ARW - HASHWH> offset;
        ap_uint<2> nxt_state;
        switch (state) {
            case 0:
                dl = rcnt;
                ul = part_depth;
                offset = rcnt;
                nxt_state = !wlist_strm.empty() ? (ap_uint<2>)1 : (ap_uint<2>)2;
                break;
            case 1:
                dl = 0;
                ul = part_depth;
                offset = 0;
                bk_num = wlist_strm.read();
                nxt_state = !wlist_strm.empty() ? (ap_uint<2>)1 : (ap_uint<2>)2;
                break;
            case 2:
                dl = 0;
                ul = 0;
                nxt_state = 2;
                break;
        }

    REMAIN_BUCKET_LOOP:
        for (ap_uint<10> i = 0; i < (ul - dl); i++) {
#pragma HLS PIPELINE II = 1
#pragma HLS dependence variable = uram_inst0.blocks inter false
#pragma HLS dependence variable = uram_inst1.blocks inter false
#pragma HLS dependence variable = uram_inst2.blocks inter false
            ap_uint<ARW - HASHWH> raddr;
            if (state == 0 || state == 1) raddr = bk_num(bit_num, 0) * part_depth + offset + i;
            ap_uint<KEYW + PW> rdata = 0;
            ap_uint<64> tmp0 = uram_inst0.read(raddr);
            ap_uint<64> tmp1 = uram_inst1.read(raddr);
            ap_uint<64> tmp2 = uram_inst2.read(raddr);
            rdata.range(63, 0) = tmp0;
            rdata.range(127, 64) = tmp1;
            rdata.range(191, 128) = tmp2;
            o_hp_kpld_strm.write(rdata);
        }

        if (state == 0 || state == 1) {
            o_hp_nm_strm.write(part_depth);
            if (bit_num > 0) {
                o_hp_bk_strm.write(bk_num(bit_num, 1));
            } else {
                o_hp_bk_strm.write(0);
            }
        }

        state = nxt_state;
    } while (state != 2);

    ap_uint<10> len;
    ap_uint<ARW - HASHWH> offset;
    for (int i = 0; i < BK; i++) {
        ap_uint<10> bucket_reg_left = bucket_cnt_copy[i];
        if (bucket_reg_left == 0) {
            len = 0;
            offset = 0;
        } else if ((bucket_reg_left > 0) && (bucket_reg_left < part_depth)) {
            len = bucket_reg_left;
            offset = 0;
        } else {
            len = bucket_reg_left - part_depth;
            offset = part_depth;
        }
        if (len != 0) {
            o_hp_nm_strm.write(len);
            o_hp_bk_strm.write(i);
        BUILD_INCOMPLETE_BUCKET_OUT_LOOP:
            for (int j = 0; j < len; j++) {
#pragma HLS PIPELINE II = 1
#pragma HLS dependence variable = uram_inst0.blocks inter false
#pragma HLS dependence variable = uram_inst1.blocks inter false
#pragma HLS dependence variable = uram_inst2.blocks inter false
                ap_uint<ARW - HASHWH> raddr = 2 * i * part_depth + offset + j; // need optimize
                ap_uint<64> tmp0 = uram_inst0.read(raddr);
                ap_uint<64> tmp1 = uram_inst1.read(raddr);
                ap_uint<64> tmp2 = uram_inst2.read(raddr);
                ap_uint<KEYW + PW> rdata = 0;
                rdata.range(63, 0) = tmp0;
                rdata.range(127, 64) = tmp1;
                rdata.range(191, 128) = tmp2;
                o_hp_kpld_strm.write(rdata);
            }
        }
    }

    // corner case, when only 1 data left after 512/1024, the data is saved in waddr, wdata
    if (!isNonBlocking && last) {
        o_hp_nm_strm.write(1);
        o_hp_bk_strm.write(bucket_idx);
        o_hp_kpld_strm.write(wdata);
    }

    o_hp_nm_strm.write(0);
}

template <int HASHWL, int HASHWJ, int KEYW, int PW, int B_PW, int ARW, int NUM_X, int NUM_Y>
void join_bf_flow(ap_uint<8> general_cfg,
                  ap_uint<32> depth,
                  hls::stream<ap_uint<HASHWL> >& i_hash_strm,
                  hls::stream<ap_uint<KEYW> >& i_key_strm,
                  hls::stream<ap_uint<PW> >& i_pld_strm,
                  hls::stream<ap_uint<PW> >& i_bf_info_strm,
                  hls::stream<bool>& i_to_strm,
                  hls::stream<bool>& i_e_strm,

                  hls::stream<ap_uint<B_PW> >& pld_base_strm,
                  hls::stream<ap_uint<PW> >& o_base_addr_strm,
                  hls::stream<ap_uint<ARW> >& o_nm0_strm,
                  hls::stream<bool>& o_e0_strm,
                  hls::stream<ap_uint<B_PW> >& pld_overflow_strm,
                  hls::stream<ap_uint<PW> >& o_overflow_addr_strm,
                  hls::stream<ap_uint<ARW> >& o_nm1_strm,
                  hls::stream<bool>& o_e1_strm,
                  hls::stream<ap_uint<KEYW> >& o_key_strm,
                  hls::stream<ap_uint<B_PW> >& o_pld_strm,
                  hls::stream<ap_uint<ARW> >& o_nm2_strm,
                  hls::stream<bool>& o_e2_strm,

                  ap_uint<72> (*uram_buffer0)[NUM_X][NUM_Y][4096],
                  ap_uint<72> (*uram_buffer1)[NUM_X][NUM_Y][4096]) {
#pragma HLS INLINE off

#ifndef __SYNTHESIS__
    unsigned int cnt = 0;
    unsigned int base_cnt = 0;
    unsigned int overflow_cnt = 0;
#endif

    const int HASH_DEPTH = (1 << HASHWJ) / 3 + 1;
    UramCache<72, HASH_DEPTH, 8, NUM_X, NUM_Y> uram_inst0(uram_buffer0);
    UramCache<72, HASH_DEPTH, 8, NUM_X, NUM_Y> uram_inst1(uram_buffer1);

    ap_uint<72> base_bitmap;
    ap_uint<72> overflow_bitmap;
    ap_uint<ARW> base_ht_addr;
    ap_uint<ARW> overflow_ht_addr;

    bool last = i_e_strm.read();
LOOP_PROBE:
    while (!last) {
#pragma HLS PIPELINE II = 1
#pragma HLS dependence variable = uram_inst0.blocks inter false
#pragma HLS dependence variable = uram_inst1.blocks inter false

        // read select field from stream and store them on local ram.
        ap_uint<HASHWL> hash_val_tmp = i_hash_strm.read();
        ap_uint<HASHWJ> hash_val = hash_val_tmp.range(HASHWJ - 1, 0);
        ap_uint<KEYW> key = i_key_strm.read();
        ap_uint<B_PW> pld = i_pld_strm.read(); // XXX trunc
        last = i_e_strm.read();
        ap_uint<PW> bf_info = i_bf_info_strm.read();
        bool to_overflow = i_to_strm.read();

        // mod 3 to calculate index for 24bit address
        ap_uint<HASHWJ> array_idx = hash_val / 3;
        ap_uint<HASHWJ> temp = array_idx * 3;
        ap_uint<2> bit_idx = hash_val - temp;

        // total hash count
        ap_uint<ARW> nm;

        // get total nm
        switch (general_cfg.range(4, 0)) {
            case gqe::JoinOn:
                base_bitmap = uram_inst0.read(array_idx);
                if (bit_idx == 0 || bit_idx == 3) {
                    nm = base_bitmap(23, 0);
                } else if (bit_idx == 1) {
                    nm = base_bitmap(47, 24);
                } else {
                    nm = base_bitmap(71, 48);
                }
                // calculate addr
                base_ht_addr = hash_val * depth;

                if ((bit_idx == 0 || bit_idx == 3) && array_idx > 0)
                    overflow_bitmap = uram_inst1.read(array_idx - 1);
                else
                    overflow_bitmap = uram_inst1.read(array_idx);

                if (bit_idx == 0 || bit_idx == 3) {
                    if (array_idx > 0) {
                        overflow_ht_addr = overflow_bitmap(71, 48);
                    } else {
                        overflow_ht_addr = 0;
                    }
                } else if (bit_idx == 1) {
                    overflow_ht_addr = overflow_bitmap(23, 0);
                } else {
                    overflow_ht_addr = overflow_bitmap(47, 24);
                }
                break;
            case gqe::BloomFilterOn:
                nm = 1;
                break;
#ifndef __SYNTHESIS__
            default:
                std::cerr << "Error: illegal kernel switching combination\n";
                exit(1);
#endif
        }

#ifndef __SYNTHESIS__
#ifdef DEBUG_MISS
        if (key == 8887)
            std::cout << std::hex << "probe_ahead: key=" << key << " hash_val=" << hash_val
                      << " array_idx=" << array_idx << " bit_idx=" << bit_idx << " nm=" << nm
                      << " base_ht_addr=" << base_ht_addr << " base_bitmap=" << base_bitmap
                      << " overflow_addr=" << overflow_ht_addr << " overflow_bitmap=" << overflow_bitmap << std::endl;
#endif
#endif
        // optimization: add bloom filter to filter out more row
        if (nm >= 0) {
#ifndef __SYNTHESIS__
            cnt++;
#endif

            // base number | higher space of bloom filter
            ap_uint<ARW> nm0;
            // overflow number | lower space of bloom filter
            ap_uint<ARW> nm1;

            // separate nm to nm0/nm1
            switch (general_cfg.range(4, 0)) {
                case gqe::JoinOn:
                    if (nm > depth) {
                        nm0 = depth;
                        nm1 = nm - depth;
                    } else {
                        nm0 = nm;
                        nm1 = 0;
                    }
                    break;
                case gqe::BloomFilterOn:
                    if (to_overflow) {
                        nm0 = 0;
                        nm1 = nm;
                    } else {
                        nm0 = nm;
                        nm1 = 0;
                    }
                    break;
#ifndef __SYNTHESIS__
                default:
                    std::cerr << "Error: illegal kernel switching combination\n";
                    exit(1);
#endif
            }

            if (nm0 > 0) {
#ifndef __SYNTHESIS__
                base_cnt++;
#endif
                ap_uint<PW> base_addr_out = 0;
                switch (general_cfg.range(4, 0)) {
                    case gqe::JoinOn:
                        base_addr_out.range(ARW - 1, 0) = base_ht_addr;
                        break;
                    case gqe::BloomFilterOn:
                        base_addr_out = bf_info;
                        pld_base_strm.write(pld);
                        break;
#ifndef __SYNTHESIS__
                    default:
                        std::cerr << "Error: illegal kernel switching combination\n";
                        exit(1);
#endif
                }
                o_base_addr_strm.write(base_addr_out);
                o_nm0_strm.write(nm0);
                o_e0_strm.write(false);
#ifndef __SYNTHESIS__
#ifdef DEBUG_MISS
                if (key == 8887)
                    std::cout << "probe_ahead: nm_base=" << nm0 << " base_addr=" << base_addr_out << std::endl;
#endif
#endif
            }
            if (nm1 > 0) {
#ifndef __SYNTHESIS__
                overflow_cnt++;
#endif
                ap_uint<PW> overflow_addr_out = 0;
                switch (general_cfg.range(4, 0)) {
                    case gqe::JoinOn:
                        overflow_addr_out.range(ARW - 1, 0) = overflow_ht_addr;
                        break;
                    case gqe::BloomFilterOn:
                        overflow_addr_out = bf_info;
                        pld_overflow_strm.write(pld);
                        break;
#ifndef __SYNTHESIS__
                    default:
                        std::cerr << "Error: illegal kernel switching combination\n";
                        exit(1);
#endif
                }
                o_overflow_addr_strm.write(overflow_addr_out);
                o_nm1_strm.write(nm1);
                o_e1_strm.write(false);
#ifndef __SYNTHESIS__
#ifdef DEBUG_MISS
                if (key == 8887)
                    std::cout << "probe_ahead: nm_overflow=" << nm1 << " overflow_addr=" << overflow_addr_out
                              << std::endl;
#endif
#endif
            }
            o_key_strm.write(key);
            o_pld_strm.write(pld);
            switch (general_cfg.range(4, 0)) {
                case gqe::JoinOn:
                    o_nm2_strm.write(nm);
                    break;
                case gqe::BloomFilterOn:
                    // send out base/overflow channel info
                    // avoid using 0 as it will go to join_unit_2
                    if (to_overflow) {
                        o_nm2_strm.write(1);
                    } else {
                        o_nm2_strm.write(2);
                    }
                    break;
#ifndef __SYNTHESIS__
                default:
                    std::cerr << "Error: illegal kernel switching combination\n";
                    exit(1);
#endif
            }
            o_e2_strm.write(false);
        }
    }

#ifndef __SYNTHESIS__
    std::cout << std::dec << "probe will read " << cnt << " block from stb, including " << base_cnt << " base block, "
              << overflow_cnt << " overflow block" << std::endl;
#endif

    o_e0_strm.write(true);
    o_e1_strm.write(true);
    o_e2_strm.write(true);
}

/// @brief access URAM for JOIN/BF/PART
template <int HASHWH, int HASHWL, int HASHWJ, int KEYW, int PW, int S_PW, int B_PW, int ARW, int NUM_X, int NUM_Y>
void access_uram(hls::stream<ap_uint<8> >& i_general_cfg_strm,
                 hls::stream<ap_uint<15> >& i_part_cfg_strm,
                 ap_uint<32>& depth,

                 // input large table from input demux
                 hls::stream<ap_uint<HASHWL> >& i_hash_strm,
                 hls::stream<ap_uint<KEYW> >& i_key_strm,
                 hls::stream<ap_uint<PW> >& i_pld_strm,
                 hls::stream<ap_uint<PW> >& i_bf_info_strm,
                 hls::stream<bool>& i_to_strm,
                 hls::stream<bool>& i_e_strm,
                 // input filtered table from output demux
                 hls::stream<ap_uint<Log2<256>::value - HASHWH> >& i_bucket_idx_strm,
                 hls::stream<ap_uint<ARW - HASHWH> >& i_waddr_strm,
                 hls::stream<ap_uint<KEYW + PW> >& i_wdata_strm,
                 hls::stream<ap_uint<11> >& i_bucket_idx_cnt_strm,
                 hls::stream<bool>& i_part_e_strm,

                 // output to generate probe addr
                 hls::stream<ap_uint<B_PW> >& pld_base_strm,
                 hls::stream<ap_uint<PW> >& o_base_addr_strm,
                 hls::stream<ap_uint<ARW> >& o_nm0_strm,
                 hls::stream<bool>& o_e0_strm,
                 hls::stream<ap_uint<B_PW> >& pld_overflow_strm,
                 hls::stream<ap_uint<PW> >& o_overflow_addr_strm,
                 hls::stream<ap_uint<ARW> >& o_nm1_strm,
                 hls::stream<bool>& o_e1_strm,
                 // output to join
                 hls::stream<ap_uint<KEYW> >& o_key_strm,
                 hls::stream<ap_uint<B_PW> >& o_pld_strm,
                 hls::stream<ap_uint<ARW> >& o_nm2_strm,
                 hls::stream<bool>& o_e2_strm,
                 // output partition result
                 hls::stream<ap_uint<KEYW + PW> >& o_hp_kpld_strm,
                 hls::stream<ap_uint<10> >& o_hp_nm_strm,
                 hls::stream<ap_uint<10> >& o_hp_bk_strm,

                 ap_uint<72> (*uram_buffer0)[NUM_X][NUM_Y][4096],
                 ap_uint<72> (*uram_buffer1)[NUM_X][NUM_Y][4096],
                 ap_uint<72> (*uram_buffer2)[NUM_X][NUM_Y][4096]) {
#pragma HLS INLINE off

    ap_uint<8> general_cfg = i_general_cfg_strm.read();
    bool part_on = general_cfg[3];
    ap_uint<15> part_cfg = i_part_cfg_strm.read();
    ap_uint<11> k_depth = part_cfg.range(14, 4);
    const ap_uint<10> part_depth = k_depth % 1024;
    const int bit_num = part_cfg.range(3, 0) - HASHWH;
    const int BK = 1 << bit_num;

    if (part_on) {
        part_flow<HASHWL, HASHWH, HASHWJ, KEYW, PW, ARW, NUM_X, NUM_Y>(
            part_depth, bit_num, BK, i_bucket_idx_strm, i_waddr_strm, i_wdata_strm, i_bucket_idx_cnt_strm,
            i_part_e_strm, //
            o_hp_kpld_strm, o_hp_nm_strm, o_hp_bk_strm, uram_buffer0, uram_buffer1, uram_buffer2);
    } else {
        join_bf_flow<HASHWL, HASHWJ, KEYW, PW, B_PW, ARW, NUM_X, NUM_Y>(
            general_cfg, depth, i_hash_strm, i_key_strm, i_pld_strm, i_bf_info_strm, i_to_strm, i_e_strm, //
            pld_base_strm, o_base_addr_strm, o_nm0_strm, o_e0_strm, pld_overflow_strm, o_overflow_addr_strm, o_nm1_strm,
            o_e1_strm, o_key_strm, o_pld_strm, o_nm2_strm, o_e2_strm, uram_buffer0, uram_buffer1);
    }
}

/// @brief handling URAM for JOIN/BF/PART
template <int HASHWH, int HASHWL, int HASHWJ, int KEYW, int PW, int S_PW, int B_PW, int ARW, int NUM_X, int NUM_Y>
void uram_handler(hls::stream<ap_uint<8> >& i_general_cfg_strm,
                  hls::stream<ap_uint<15> >& i_part_cfg_strm,
                  ap_uint<32>& depth,

                  // input large table from input demux
                  hls::stream<ap_uint<HASHWL> >& i_hash_strm,
                  hls::stream<ap_uint<KEYW> >& i_key_strm,
                  hls::stream<ap_uint<PW> >& i_pld_strm,
                  hls::stream<ap_uint<PW> >& i_bf_info_strm,
                  hls::stream<bool>& i_to_strm,
                  hls::stream<bool>& i_e_strm,
                  // input filtered table from output demux
                  hls::stream<ap_uint<HASHWL> >& i_part_hash_strm,
                  hls::stream<ap_uint<KEYW> >& i_part_key_strm,
                  hls::stream<ap_uint<PW> >& i_part_pld_strm,
                  hls::stream<bool>& i_part_e_strm,

                  // output to generate probe addr
                  hls::stream<ap_uint<B_PW> >& pld_base_strm,
                  hls::stream<ap_uint<PW> >& o_base_addr_strm,
                  hls::stream<ap_uint<ARW> >& o_nm0_strm,
                  hls::stream<bool>& o_e0_strm,
                  hls::stream<ap_uint<B_PW> >& pld_overflow_strm,
                  hls::stream<ap_uint<PW> >& o_overflow_addr_strm,
                  hls::stream<ap_uint<ARW> >& o_nm1_strm,
                  hls::stream<bool>& o_e1_strm,
                  // output to join
                  hls::stream<ap_uint<KEYW> >& o_key_strm,
                  hls::stream<ap_uint<B_PW> >& o_pld_strm,
                  hls::stream<ap_uint<ARW> >& o_nm2_strm,
                  hls::stream<bool>& o_e2_strm,
                  // output partition result
                  hls::stream<ap_uint<KEYW + PW> >& o_hp_kpld_strm,
                  hls::stream<ap_uint<10> >& o_hp_nm_strm,
                  hls::stream<ap_uint<10> >& o_hp_bk_strm,

                  ap_uint<72> (*uram_buffer0)[NUM_X][NUM_Y][4096],
                  ap_uint<72> (*uram_buffer1)[NUM_X][NUM_Y][4096],
                  ap_uint<72> (*uram_buffer2)[NUM_X][NUM_Y][4096]) {
#pragma HLS dataflow

    hls::stream<ap_uint<8> > general_cfg_strms[2];
#pragma HLS stream variable = general_cfg_strms depth = 2
#pragma HLS bind_storage variable = general_cfg_strms type = fifo impl = srl
    hls::stream<ap_uint<15> > part_cfg_strms[2];
#pragma HLS stream variable = part_cfg_strms depth = 2
#pragma HLS bind_storage variable = part_cfg_strms type = fifo impl = srl
    hls::stream<ap_uint<Log2<256>::value - HASHWH> > mid_bucket_idx_strm;
#pragma HLS stream variable = mid_bucket_idx_strm depth = 32
#pragma HLS bind_storage variable = mid_bucket_idx_strm type = fifo impl = srl
    hls::stream<ap_uint<ARW - HASHWH> > mid_waddr_strm;
#pragma HLS stream variable = mid_waddr_strm depth = 32
#pragma HLS bind_storage variable = mid_waddr_strm type = fifo impl = srl
    hls::stream<ap_uint<KEYW + PW> > mid_wdata_strm;
#pragma HLS stream variable = mid_wdata_strm depth = 32
#pragma HLS bind_storage variable = mid_wdata_strm type = fifo impl = srl
    hls::stream<ap_uint<11> > mid_bucket_idx_cnt_strm;
#pragma HLS stream variable = mid_bucket_idx_cnt_strm depth = 32
#pragma HLS bind_storage variable = mid_bucket_idx_cnt_strm type = fifo impl = srl
    hls::stream<bool> mid_e_strm;
#pragma HLS stream variable = mid_e_strm depth = 32
#pragma HLS bind_storage variable = mid_e_strm type = fifo impl = srl

    dup_signals<2, 8>(i_general_cfg_strm, general_cfg_strms);
    dup_signals<2, 15>(i_part_cfg_strm, part_cfg_strms);

    prepare_part<HASHWH, HASHWL, KEYW, PW, ARW>(
        general_cfg_strms[0], part_cfg_strms[0], i_part_hash_strm, i_part_key_strm, i_part_pld_strm,
        i_part_e_strm, //
        mid_bucket_idx_strm, mid_waddr_strm, mid_wdata_strm, mid_bucket_idx_cnt_strm, mid_e_strm);

    access_uram<HASHWH, HASHWL, HASHWJ, KEYW, PW, S_PW, B_PW, ARW, NUM_X, NUM_Y>(
        general_cfg_strms[1], part_cfg_strms[1], depth, i_hash_strm, i_key_strm, i_pld_strm, i_bf_info_strm, i_to_strm,
        i_e_strm, mid_bucket_idx_strm, mid_waddr_strm, mid_wdata_strm, mid_bucket_idx_cnt_strm, mid_e_strm, //
        pld_base_strm, o_base_addr_strm, o_nm0_strm, o_e0_strm, pld_overflow_strm, o_overflow_addr_strm, o_nm1_strm,
        o_e1_strm, o_key_strm, o_pld_strm, o_nm2_strm, o_e2_strm, o_hp_kpld_strm, o_hp_nm_strm, o_hp_bk_strm,
        uram_buffer0, uram_buffer1, uram_buffer2);
}

// generate probe addr
template <int HASHWL, int PW, int ARW>
void probe_addr_gen(
    // input
    hls::stream<ap_uint<8> >& i_general_cfg_strm,
    hls::stream<ap_uint<PW> >& i_base_stb_addr_strm,
    hls::stream<ap_uint<ARW> >& i_nm_strm,
    hls::stream<bool>& i_e_strm,

    // output
    hls::stream<ap_uint<PW> >& o_read_addr_strm,
    hls::stream<bool>& o_e_strm) {
#pragma HLS INLINE off

    ap_uint<8> general_cfg = i_general_cfg_strm.read();
    bool bf_on = general_cfg[2];
    bool is_first_loop = true;
    ap_uint<PW> addr;
    ap_uint<ARW> nm = 0;

    bool last = i_e_strm.read();
    while (!last || nm != 0) {
#pragma HLS pipeline II = 1
        if (bf_on) {
            bool rd_e = !i_base_stb_addr_strm.empty() && !i_nm_strm.empty() && !i_e_strm.empty();
            if (rd_e) {
                {
#pragma HLS latency min = 1 max = 1
                    i_base_stb_addr_strm.read_nb(addr);
                    ap_uint<ARW> drop;
                    i_nm_strm.read_nb(drop);
                    i_e_strm.read_nb(last);
                }
                {
#pragma HLS latency min = 1 max = 1
                    o_read_addr_strm.write(addr);
                    o_e_strm.write(false);
                }
            }
        } else {
            if (!last && is_first_loop) {
                addr = i_base_stb_addr_strm.read();
                nm = i_nm_strm.read();
                last = i_e_strm.read();

                is_first_loop = false;
            } else if (nm > 0) {
                nm--;
                o_read_addr_strm.write(addr);
                o_e_strm.write(false);

#ifndef __SYNTHESIS__
#ifdef DEBUG_PROBE
                std::cout << std::hex << "probe_addr_gen: probe_addr=" << addr << std::endl;
#endif
#endif

                addr++;
            } else if (!last && nm == 0) {
                is_first_loop = true;
            }
        }
    }
    o_e_strm.write(true);
}

// generate addr for read/write stb
template <int RW, int PW, int HASHWL>
void stb_addr_gen(hls::stream<ap_uint<8> >& i_general_cfg_strm,
                  hls::stream<ap_uint<PW> >& i_addr_strm,
                  hls::stream<bool>& i_e_strm,

                  hls::stream<ap_uint<PW> >& o_addr_strm,
                  hls::stream<bool>& o_e_strm) {
#pragma HLS INLINE off

    const int number_of_element_per_row =
        (RW % 256 == 0) ? RW / 256 : RW / 256 + 1; // number of output based on 256 bit

    ap_uint<8> general_cfg = i_general_cfg_strm.read();
    bool bf_on = general_cfg[2];
    ap_uint<PW> addr_in;

    bool last = i_e_strm.read();
    while (!last) {
#pragma HLS PIPELINE II = number_of_element_per_row
        bool rd_e = !i_addr_strm.empty() && !i_e_strm.empty();
        if (rd_e) {
            {
#pragma HLS latency min = 1 max = 1
                i_addr_strm.read_nb(addr_in);
                i_e_strm.read_nb(last);
            }
            if (bf_on) {
                {
#pragma HLS latency min = 1 max = 1
                    o_addr_strm.write(addr_in);
                    o_e_strm.write(false);
                }
            } else {
                ap_uint<PW> addr_base = addr_in * number_of_element_per_row;
                for (int i = 0; i < number_of_element_per_row; i++) {
                    o_addr_strm.write(addr_base++);
                    o_e_strm.write(false);
                }
            }
        }
    }

    o_e_strm.write(true);
}

template <int PW, int S_PW>
void bf_build_base(hls::stream<ap_uint<S_PW> >& i_pld_strm,
                   hls::stream<ap_uint<PW> >& i_addr_strm,
                   hls::stream<bool>& i_e_strm,
                   hls::burst_maxi<ap_uint<256> >& stb_buf) {
#pragma HLS INLINE off

    ap_uint<PW - 36> cacheLineBaseOffset;
    ap_uint<3> blockOffset[4];
#pragma HLS array_partition variable = blockOffset dim = 1
    ap_uint<6> bitPos[4];
#pragma HLS array_partition variable = bitPos dim = 1

    bool last = i_e_strm.read();
    while (!last) {
#pragma HLS pipeline off
        ap_uint<S_PW> i_pld = i_pld_strm.read();
        ap_uint<PW> addr = i_addr_strm.read();
        last = i_e_strm.read();
        // is not separator
        if (i_pld != ~ap_uint<S_PW>(0)) {
            (cacheLineBaseOffset, blockOffset[3], bitPos[3], blockOffset[2], bitPos[2], blockOffset[1], bitPos[1],
             blockOffset[0], bitPos[0]) = addr;
            ap_uint<512> data_in = 0;
            for (int i = 0; i < 4; i++) {
#pragma HLS unroll
                data_in[blockOffset[i] * 64 + bitPos[i]] = 1;
            }

            stb_buf.read_request(cacheLineBaseOffset, 2);
            ap_uint<256> tmp_low = stb_buf.read();
            ap_uint<256> tmp_high = stb_buf.read();
            ap_uint<512> data_tmp, data_out;
            data_tmp.range(tmp_low.width - 1, 0) = tmp_low;
            data_tmp.range(tmp_high.width + tmp_low.width - 1, tmp_low.width) = tmp_high;
            data_out = data_tmp | data_in;

            stb_buf.write_request(cacheLineBaseOffset, 2);
            stb_buf.write((ap_uint<256>)data_out.range(255, 0));
            stb_buf.write((ap_uint<256>)data_out.range(511, 256));
            stb_buf.write_response();
        }
    }
}

template <int KEYW, int PW, int S_PW, int OUTSTANDING>
void bf_probe_base(hls::stream<ap_uint<S_PW> >& i_pld_strm,
                   hls::stream<ap_uint<PW> >& i_addr_strm,
                   hls::stream<bool>& i_e_strm,
                   hls::burst_maxi<ap_uint<256> >& stb_buf,
                   bool part_on,

                   hls::stream<ap_uint<256> >& o_row_strm,
                   hls::stream<bool>& o_e_strm,
                   hls::stream<bool>& o_base_is_in_strm) {
#pragma HLS INLINE off

    ap_uint<PW - 36> cacheLineBaseOffset;
    ap_uint<3> blockOffset_w[4];
    ap_uint<3> blockOffset_r[4];
#pragma HLS array_partition variable = blockOffset_w dim = 1
#pragma HLS array_partition variable = blockOffset_r dim = 1
    ap_uint<6> bitPos_w[4];
    ap_uint<6> bitPos_r[4];
#pragma HLS array_partition variable = bitPos_w dim = 1
#pragma HLS array_partition variable = bitPos_r dim = 1

    ap_uint<OUTSTANDING* 2> check = 0;
    ap_uint<OUTSTANDING* 2> separator = 0;
    ap_uint<2> low_high = 0;
    ap_uint<S_PW> i_pld;
    ap_uint<PW> addr;
    ap_uint<256> data_mem[2];
    ap_uint<36> hash_dot[OUTSTANDING];
#pragma HLS bind_storage variable = hash_dot type = ram_t2p impl = bram
    int addr_w = 0;
    int addr_r = 0;

    bool last = i_e_strm.read();
    while (!last || check != 0) {
#pragma HLS PIPELINE II = 1
        bool check_l = check[OUTSTANDING * 2 - 1];
        bool is_separator = separator[OUTSTANDING * 2 - 1];
        if (check_l) {
            data_mem[low_high++] = stb_buf.read();
            if (low_high == 2) {
                (blockOffset_r[3], bitPos_r[3], blockOffset_r[2], bitPos_r[2], blockOffset_r[1], bitPos_r[1],
                 blockOffset_r[0], bitPos_r[0]) = hash_dot[addr_r % OUTSTANDING];
                addr_r++;
                low_high = 0;
                ap_uint<512> cache_line;
                cache_line.range(255, 0) = data_mem[0];
                cache_line.range(511, 256) = data_mem[1];

                bool bitIn[4] = {false};
                // multi-hashing
                for (int i = 0; i < 4; i++) {
#pragma HLS unroll
                    bitIn[i] = cache_line[blockOffset_r[i] * 64 + bitPos_r[i]];
                }
                bool isIn = true;
                for (int i = 0; i < 4; i++) {
#pragma HLS unroll
                    isIn &= bitIn[i];
                }

                ap_uint<KEYW> o_key = 0;
                ap_uint<S_PW> o_pld = isIn;
                if (is_separator) {
                    o_pld = ~ap_uint<S_PW>(0);
                }
                ap_uint<256> element = 0;
                element.range(S_PW - 1, 0) = o_pld;
                element.range(KEYW + S_PW - 1, S_PW) = o_key;
                if (part_on) {
                    o_base_is_in_strm.write(isIn);
                } else {
                    o_row_strm.write(element);
                    o_e_strm.write(false);
                }
            }
        }
        separator = separator << 1;
        check = check << 1;
        bool rd_e = !i_addr_strm.empty() && !i_pld_strm.empty() && !i_e_strm.empty();
        if (rd_e && check.range(1, 0) == 0 && addr_r <= addr_w) {
            {
#pragma HLS latency min = 1 max = 1
                i_addr_strm.read_nb(addr);
                i_pld_strm.read_nb(i_pld);
                i_e_strm.read_nb(last);
            }
            if (i_pld == ~ap_uint<S_PW>(0)) {
                separator.range(1, 0) = 3;
            }
            (cacheLineBaseOffset, blockOffset_w[3], bitPos_w[3], blockOffset_w[2], bitPos_w[2], blockOffset_w[1],
             bitPos_w[1], blockOffset_w[0], bitPos_w[0]) = addr;

            stb_buf.read_request(cacheLineBaseOffset, 2);
            check.range(1, 0) = 3;
            hash_dot[addr_w % OUTSTANDING] = (blockOffset_w[3], bitPos_w[3], blockOffset_w[2], bitPos_w[2],
                                              blockOffset_w[1], bitPos_w[1], blockOffset_w[0], bitPos_w[0]);
            addr_w++;
        }
    }
}

// read row from HBM/DDR
template <int KEYW, int PW, int S_PW, int HASHWH, int HASHWL, int OUTSTANDING>
void read_row(hls::stream<ap_uint<8> >& i_general_cfg_strm,
              hls::stream<ap_uint<S_PW> >& i_pld_strm,
              hls::burst_maxi<ap_uint<256> >& stb_buf,
              hls::stream<ap_uint<PW> >& i_addr_strm,
              hls::stream<bool>& i_e_strm,

              hls::stream<ap_uint<256> >& o_row_strm,
              hls::stream<bool>& o_e_strm,
              hls::stream<bool>& o_base_is_in_strm) {
#pragma HLS INLINE off

    ap_uint<8> general_cfg = i_general_cfg_strm.read();
    bool bf_bp_flag = general_cfg[7];
    bool part_on = general_cfg[3];

    ap_uint<PW> addr;
    ap_uint<OUTSTANDING> check = 0;

    bool last;
    switch (general_cfg.range(4, 0)) {
        case gqe::JoinOn:
            last = i_e_strm.read();

            while (!last || check != 0) {
#pragma HLS PIPELINE II = 1
                bool check_l = check[OUTSTANDING - 1];
                if (check_l) {
                    o_row_strm.write(stb_buf.read());
                    o_e_strm.write(false);
                }
                check = (check << 1);
                bool rd_e = !i_addr_strm.empty() && !i_e_strm.empty();
                if (rd_e) {
                    {
#pragma HLS latency min = 1 max = 1
                        i_addr_strm.read_nb(addr);
                        i_e_strm.read_nb(last);
                    }
                    stb_buf.read_request(addr, 1);
                    check[0] = 1;
                }
            }
            break;
        case gqe::PartOn | gqe::BloomFilterOn:
        case gqe::BloomFilterOn:
            if (bf_bp_flag) {
                bf_probe_base<KEYW, PW, S_PW, OUTSTANDING>(i_pld_strm, i_addr_strm, i_e_strm, stb_buf, part_on,
                                                           o_row_strm, o_e_strm, o_base_is_in_strm);
            } else {
                bf_build_base<PW, S_PW>(i_pld_strm, i_addr_strm, i_e_strm, stb_buf);
            }
            break;
#ifndef __SYNTHESIS__
        default:
            std::cerr << "Error: illegal kernel switching combination\n";
            exit(1);
#endif
    }

    o_e_strm.write(true);
}

// combine several 256 bit stream into one row
template <int RW, int ARW>
void combine_row(hls::stream<ap_uint<8> >& i_general_cfg_strm,
                 hls::stream<ap_uint<256> >& i_row_strm,
                 hls::stream<bool>& i_e_strm,

                 hls::stream<ap_uint<RW> >& o_row_strm,
                 hls::stream<bool>& o_e_strm) {
#pragma HLS INLINE off

#ifndef __SYNTHESIS__
    unsigned int cnt = 0;
#endif

    const int number_of_element_per_row =
        (RW % 256 == 0) ? RW / 256 : RW / 256 + 1; // number of output based on 256 bit

    ap_uint<8> general_cfg = i_general_cfg_strm.read();
    bool bf_on = general_cfg[2];

    bool last = i_e_strm.read();
    ap_uint<256 * number_of_element_per_row> row_temp = 0;
    ap_uint<4> mux = 0;
    ap_uint<256> element;

    while (!last) {
#pragma HLS PIPELINE II = 1
        bool rd_e = !i_row_strm.empty() && !i_e_strm.empty();
        if (rd_e) {
            {
#pragma HLS latency min = 1 max = 1
                i_row_strm.read_nb(element);
                i_e_strm.read_nb(last);
            }

            if (bf_on) {
                {
#pragma HLS latency min = 1 max = 1
                    o_row_strm.write(element.range(RW - 1, 0));
                    o_e_strm.write(false);
                }
            } else {
                row_temp((mux + 1) * 256 - 1, mux * 256) = element;
                if (mux == number_of_element_per_row - 1) {
                    ap_uint<RW> row = row_temp(RW - 1, 0);
                    {
#pragma HLS latency min = 1 max = 1
                        o_row_strm.write(row);
                        o_e_strm.write(false);
                    }

                    mux = 0;
                } else {
                    mux++;
                }
            }

#ifndef __SYNTHESIS__
            cnt++;
#endif
        }
    }
    o_e_strm.write(true);

#ifndef __SYNTHESIS__
#ifdef DEBUG_HBM
    std::cout << std::dec << "RW= " << RW << " II=" << number_of_element_per_row << std::endl;
    std::cout << std::dec << "STB read " << cnt / number_of_element_per_row << " rows" << std::endl;
#endif
#endif
}

/// @brief Read s-table from HBM/DDR
template <int HASHWH, int HASHWL, int ARW, int KEYW, int PW, int S_PW, int HBM_OUTSTANDING>
void read_stb(hls::stream<ap_uint<8> >& i_general_cfg_strm,
              hls::stream<ap_uint<S_PW> >& i_pld_strm,

              hls::burst_maxi<ap_uint<256> >& stb_buf,
              hls::stream<ap_uint<PW> >& i_addr_strm,
              hls::stream<bool>& i_e_strm,

              hls::stream<ap_uint<KEYW + S_PW> >& o_row_strm,
              hls::stream<bool>& o_e_strm,
              hls::stream<bool>& base_is_in_strm) {
#pragma HLS INLINE off
#pragma HLS DATAFLOW

    hls::stream<ap_uint<8> > general_cfg_strms[3];
#pragma HLS stream variable = general_cfg_strms depth = 2
#pragma HLS bind_storage variable = general_cfg_strms type = fifo impl = srl

    hls::stream<ap_uint<PW> > addr_strm;
#pragma HLS STREAM variable = addr_strm depth = 512
#pragma HLS bind_storage variable = addr_strm type = fifo impl = bram
    hls::stream<bool> e0_strm;
#pragma HLS STREAM variable = e0_strm depth = 512
#pragma HLS bind_storage variable = e0_strm type = fifo impl = srl

    hls::stream<ap_uint<256> > row_strm;
#pragma HLS STREAM variable = row_strm depth = 8
#pragma HLS bind_storage variable = row_strm type = fifo impl = srl
    hls::stream<bool> e1_strm;
#pragma HLS STREAM variable = e1_strm depth = 8
#pragma HLS bind_storage variable = e1_strm type = fifo impl = srl

    dup_signals<3, 8>(i_general_cfg_strm, general_cfg_strms);

    stb_addr_gen<KEYW + S_PW, PW, HASHWL>(general_cfg_strms[0], i_addr_strm, i_e_strm,

                                          addr_strm, e0_strm);

    read_row<KEYW, PW, S_PW, HASHWH, HASHWL, HBM_OUTSTANDING>(general_cfg_strms[1], i_pld_strm, stb_buf, addr_strm,
                                                              e0_strm,

                                                              row_strm, e1_strm, base_is_in_strm);

    combine_row<KEYW + S_PW, ARW>(general_cfg_strms[2], row_strm, e1_strm,

                                  o_row_strm, o_e_strm);
}

// eleminate temporary strm_end
template <typename type_t>
void eliminate_strm_end(hls::stream<type_t>& strm_end) {
#pragma HLS INLINE off

    bool end = strm_end.read();
    while (!end) {
        end = strm_end.read();
    }
}

template <int _WColIn, int _WCol1, int _WCol2>
void splitCol(hls::stream<ap_uint<_WColIn> >& din_strm,
              hls::stream<bool>& in_e_strm,
              hls::stream<ap_uint<_WCol1> >& dout1_strm,
              hls::stream<ap_uint<_WCol2> >& dout2_strm,
              hls::stream<bool>& out_e_strm) {
    bool e = in_e_strm.read();
    ap_uint<_WCol1> keyout1;
    ap_uint<_WCol2> keyout2;
    ap_uint<_WColIn> keyin;
    uint64_t width_t1 = keyout1.length();
    uint64_t width_t2 = keyout2.length();
    uint64_t width_in = keyin.length();
    while (!e) {
#pragma HLS pipeline II = 1
        bool rd_e = !din_strm.empty() && !in_e_strm.empty();
        if (rd_e) {
            {
#pragma HLS latency min = 1 max = 1
                din_strm.read_nb(keyin);
                in_e_strm.read_nb(e);
            }
            keyout1 = keyin.range(width_t1 - 1, 0);
            keyout2 = keyin.range(width_in - 1, width_t1);
            dout1_strm.write(keyout1);
            dout2_strm.write(keyout2);
            out_e_strm.write(0);
        }
    }
    out_e_strm.write(1);
}
/// @brief Probe stb which temporarily stored in HBM
template <int KEYW, int PW, int S_PW, int HASHWH, int HASHWL, int ARW, int HBM_OUTSTANDING>
void probe_base_stb(
    // input
    hls::stream<ap_uint<8> >& i_general_cfg_strm,
    hls::stream<ap_uint<S_PW> >& i_pld_strm,
    hls::stream<ap_uint<PW> >& i_base_stb_addr_strm,
    hls::stream<ap_uint<ARW> >& i_base_nm_strm,
    hls::stream<bool>& i_e_strm,

    // output probed small table
    hls::stream<ap_uint<KEYW> >& o_base_s_key_strm,
    hls::stream<ap_uint<S_PW> >& o_base_s_pld_strm,
    hls::stream<bool>& o_base_is_in_strm,

    // HBM
    hls::burst_maxi<ap_uint<256> >& stb_buf) {
#pragma HLS INLINE off
#pragma HLS DATAFLOW

    // for dup signals
    hls::stream<ap_uint<8> > general_cfg_strms[2];
#pragma HLS stream variable = general_cfg_strms depth = 2
#pragma HLS bind_storage variable = general_cfg_strms type = fifo impl = srl

    // for read stb
    hls::stream<ap_uint<PW> > read_addr_strm;
#pragma HLS stream variable = read_addr_strm depth = 8
#pragma HLS bind_storage variable = read_addr_strm type = fifo impl = srl
    hls::stream<bool> e0_strm;
#pragma HLS stream variable = e0_strm depth = 8
#pragma HLS bind_storage variable = e0_strm type = fifo impl = srl

    // for split probed stb
    hls::stream<ap_uint<KEYW + S_PW> > row_strm;
#pragma HLS stream variable = row_strm depth = 8
#pragma HLS bind_storage variable = row_strm type = fifo impl = srl
    hls::stream<bool> e1_strm;
#pragma HLS stream variable = e1_strm depth = 8
#pragma HLS bind_storage variable = e1_strm type = fifo impl = srl

    // eliminate end strm
    hls::stream<bool> e2_strm;
#pragma HLS stream variable = e2_strm depth = 8
#pragma HLS bind_storage variable = e2_strm type = fifo impl = srl

    dup_signals<2, 8>(i_general_cfg_strm, general_cfg_strms);

    // generate read addr from base addr
    probe_addr_gen<HASHWL, PW, ARW>(general_cfg_strms[0], i_base_stb_addr_strm, i_base_nm_strm, i_e_strm,

                                    read_addr_strm, e0_strm);

    // read HBM to get base stb
    read_stb<HASHWH, HASHWL, ARW, KEYW, PW, S_PW, HBM_OUTSTANDING>(general_cfg_strms[1], i_pld_strm,

                                                                   stb_buf, read_addr_strm, e0_strm, row_strm, e1_strm,
                                                                   o_base_is_in_strm);

    // MSB         LSB
    // row = key | pld
    // split base stb to key and pld
    splitCol<KEYW + S_PW, S_PW, KEYW>(row_strm, e1_strm, o_base_s_pld_strm, o_base_s_key_strm, e2_strm);

    // eleminate end signal of overflow unit
    eliminate_strm_end<bool>(e2_strm);
}

template <int KEYW, int PW, int S_PW, int ARW, int OUTSTANDING>
void bf_probe_overflow(hls::stream<ap_uint<S_PW> >& i_pld_strm,
                       hls::stream<ap_uint<PW> >& i_overflow_addr_strm,
                       hls::stream<ap_uint<ARW> >& i_overflow_nm_strm,
                       hls::stream<bool>& i_e_strm,
                       hls::burst_maxi<ap_uint<256> >& htb_buf,
                       bool part_on,

                       hls::stream<ap_uint<KEYW> >& o_overflow_s_key_strm,
                       hls::stream<ap_uint<S_PW> >& o_overflow_s_pld_strm,
                       hls::stream<bool>& o_overflow_is_in_strm) {
#pragma HLS INLINE off

    ap_uint<PW - 36> cacheLineBaseOffset;
    ap_uint<3> blockOffset_w[4];
    ap_uint<3> blockOffset_r[4];
#pragma HLS array_partition variable = blockOffset_w dim = 1
#pragma HLS array_partition variable = blockOffset_r dim = 1
    ap_uint<6> bitPos_w[4];
    ap_uint<6> bitPos_r[4];
#pragma HLS array_partition variable = bitPos_w dim = 1
#pragma HLS array_partition variable = bitPos_r dim = 1

    ap_uint<OUTSTANDING* 2> check = 0;
    ap_uint<OUTSTANDING* 2> separator = 0;
    ap_uint<2> low_high = 0;
    ap_uint<256> data_mem[2];

    ap_uint<S_PW> i_pld;
    ap_uint<PW> overflow_addr;
    ap_uint<ARW> nm;

    ap_uint<36> hash_dot[OUTSTANDING];
#pragma HLS bind_storage variable = hash_dot type = ram_t2p impl = bram
    int addr_w = 0;
    int addr_r = 0;

    bool last = i_e_strm.read();
    while (!last || check != 0) {
#pragma HLS PIPELINE II = 1
        bool check_l = check[OUTSTANDING * 2 - 1];
        bool is_separator = separator[OUTSTANDING * 2 - 1];
        if (check_l) {
            data_mem[low_high++] = htb_buf.read();
            if (low_high == 2) {
                (blockOffset_r[3], bitPos_r[3], blockOffset_r[2], bitPos_r[2], blockOffset_r[1], bitPos_r[1],
                 blockOffset_r[0], bitPos_r[0]) = hash_dot[addr_r % OUTSTANDING];
                addr_r++;
                low_high = 0;
                ap_uint<512> cache_line;
                cache_line.range(255, 0) = data_mem[0];
                cache_line.range(511, 256) = data_mem[1];

                bool bitIn[4] = {false};
                // multi-hash
                for (int i = 0; i < 4; i++) {
#pragma HLS unroll
                    bitIn[i] = cache_line[blockOffset_r[i] * 64 + bitPos_r[i]];
                }
                bool isIn = true;
                for (int i = 0; i < 4; i++) {
#pragma HLS unroll
                    isIn &= bitIn[i];
                }

                ap_uint<KEYW> o_key = 0;
                ap_uint<S_PW> o_pld = isIn;
                if (is_separator) {
                    o_pld = ~ap_uint<S_PW>(0);
                }
                if (part_on) {
                    o_overflow_is_in_strm.write(isIn);
                } else {
                    o_overflow_s_key_strm.write(o_key);
                    o_overflow_s_pld_strm.write(o_pld);
                }
            }
        }
        separator = separator << 1;
        check = (check << 1);
        bool rd_e =
            !i_overflow_addr_strm.empty() && !i_e_strm.empty() && !i_pld_strm.empty() && !i_overflow_nm_strm.empty();
        if (rd_e && check.range(1, 0) == 0 && addr_r <= addr_w) {
            {
#pragma HLS latency min = 1 max = 1
                i_overflow_addr_strm.read_nb(overflow_addr);
                i_e_strm.read_nb(last);
                i_pld_strm.read_nb(i_pld);
                i_overflow_nm_strm.read_nb(nm);
            }
            if (i_pld == ~ap_uint<S_PW>(0)) {
                separator.range(1, 0) = 3;
            }

            (cacheLineBaseOffset, blockOffset_w[3], bitPos_w[3], blockOffset_w[2], bitPos_w[2], blockOffset_w[1],
             bitPos_w[1], blockOffset_w[0], bitPos_w[0]) = overflow_addr;

            htb_buf.read_request(cacheLineBaseOffset, 2);
            check.range(1, 0) = 3;
            hash_dot[addr_w % OUTSTANDING] = (blockOffset_w[3], bitPos_w[3], blockOffset_w[2], bitPos_w[2],
                                              blockOffset_w[1], bitPos_w[1], blockOffset_w[0], bitPos_w[0]);
            addr_w++;
        }
    }
}

template <int PW, int S_PW, int ARW>
void bf_build_overflow(hls::stream<ap_uint<S_PW> >& i_pld_strm,
                       hls::stream<ap_uint<PW> >& i_overflow_addr_strm,
                       hls::stream<ap_uint<ARW> >& i_overflow_nm_strm,
                       hls::stream<bool>& i_e_strm,
                       hls::burst_maxi<ap_uint<256> >& htb_buf) {
#pragma HLS INLINE off

    ap_uint<PW - 36> cacheLineBaseOffset;
    ap_uint<3> blockOffset[4];
#pragma HLS array_partition variable = blockOffset dim = 1
    ap_uint<6> bitPos[4];
#pragma HLS array_partition variable = bitPos dim = 1

    bool last = i_e_strm.read();
    while (!last) {
#pragma HLS pipeline off
        ap_uint<S_PW> i_pld = i_pld_strm.read();
        ap_uint<PW> overflow_addr = i_overflow_addr_strm.read();
        ap_uint<ARW> nm = i_overflow_nm_strm.read();
        last = i_e_strm.read();
        // is not separator
        if (i_pld != ~ap_uint<S_PW>(0)) {
            (cacheLineBaseOffset, blockOffset[3], bitPos[3], blockOffset[2], bitPos[2], blockOffset[1], bitPos[1],
             blockOffset[0], bitPos[0]) = overflow_addr;
            ap_uint<512> data_in = 0;
            for (int i = 0; i < 4; i++) {
#pragma HLS unroll
                data_in[blockOffset[i] * 64 + bitPos[i]] = 1;
            }

            htb_buf.read_request(cacheLineBaseOffset, 2);
            ap_uint<256> tmp_low = htb_buf.read();
            ap_uint<256> tmp_high = htb_buf.read();
            ap_uint<512> data_tmp, data_out;
            data_tmp.range(tmp_low.width - 1, 0) = tmp_low;
            data_tmp.range(tmp_high.width + tmp_low.width - 1, tmp_low.width) = tmp_high;
            data_out = data_tmp | data_in;

            htb_buf.write_request(cacheLineBaseOffset, 2);
            htb_buf.write((ap_uint<256>)data_out.range(255, 0));
            htb_buf.write((ap_uint<256>)data_out.range(511, 256));
            htb_buf.write_response();
        }
    }
}

// probe stb which temporarily stored in HBM
template <int KEYW, int S_PW, int ARW, int HBM_OUTSTANDING>
void read_overflow_stb(
    // input
    ap_uint<ARW> overflow_addr,
    ap_uint<ARW> overflow_nm,

    // output probed small table
    hls::stream<ap_uint<KEYW> >& o_overflow_s_key_strm,
    hls::stream<ap_uint<S_PW> >& o_overflow_s_pld_strm,

    // HBM
    hls::burst_maxi<ap_uint<256> >& htb_buf) {
#pragma HLS INLINE off
#pragma HLS DATAFLOW

    // for read_stb
    hls::stream<ap_uint<ARW> > read_addr_strm;
#pragma HLS stream variable = read_addr_strm depth = 8
#pragma HLS bind_storage variable = read_addr_strm type = fifo impl = srl
    hls::stream<bool> e0_strm;
#pragma HLS stream variable = e0_strm depth = 8
#pragma HLS bind_storage variable = e0_strm type = fifo impl = srl

    // for split probed stb
    hls::stream<ap_uint<KEYW + S_PW> > row_strm;
#pragma HLS stream variable = row_strm depth = 8
#pragma HLS bind_storage variable = row_strm type = fifo impl = srl
    hls::stream<bool> e1_strm;
#pragma HLS stream variable = e1_strm depth = 8
#pragma HLS bind_storage variable = e1_strm type = fifo impl = srl

    // eliminate end strm
    hls::stream<bool> e2_strm;
#pragma HLS stream variable = e2_strm depth = 8
#pragma HLS bind_storage variable = e2_strm type = fifo impl = srl

    // generate read addr
    join_v3::sc::probe_addr_gen<ARW>(overflow_addr, overflow_nm,

                                     read_addr_strm, e0_strm);

    // read HBM to get base stb
    read_stb<ARW, KEYW + S_PW, HBM_OUTSTANDING>(htb_buf, read_addr_strm, e0_strm, row_strm, e1_strm);

    // split base stb to key and pld
    splitCol<KEYW + S_PW, S_PW, KEYW>(row_strm, e1_strm, o_overflow_s_pld_strm, o_overflow_s_key_strm, e2_strm);

    // eleminate end signal of overflow unit
    eliminate_strm_end<bool>(e2_strm);
}

/// @brief probe overflow stb which temporarily stored in HBM
template <int KEYW, int PW, int S_PW, int HASHWH, int HASHWL, int ARW, int HBM_OUTSTANDING>
void probe_overflow_stb(hls::stream<ap_uint<8> >& i_general_cfg_strm,
                        // input base addr
                        hls::stream<ap_uint<S_PW> >& i_pld_strm,
                        hls::stream<ap_uint<PW> >& i_overflow_addr_strm,
                        hls::stream<ap_uint<ARW> >& i_overflow_nm_strm,
                        hls::stream<bool>& i_e_strm,

                        // output probed small table
                        hls::stream<ap_uint<KEYW> >& o_overflow_s_key_strm,
                        hls::stream<ap_uint<S_PW> >& o_overflow_s_pld_strm,
                        hls::stream<bool>& o_overflow_is_in_strm,

                        // HBM
                        hls::burst_maxi<ap_uint<256> >& htb_buf) {
#pragma HLS INLINE off

    ap_uint<8> general_cfg = i_general_cfg_strm.read();
    bool bf_bp_flag = general_cfg[7];
    bool part_on = general_cfg[3];

    ap_uint<PW> overflow_addr;
    ap_uint<ARW> nm;
    bool last;
    switch (general_cfg.range(4, 0)) {
        case gqe::JoinOn:
            last = i_e_strm.read();
            while (!last) {
#pragma HLS PIPELINE off

                overflow_addr = i_overflow_addr_strm.read();
                nm = i_overflow_nm_strm.read();

                read_overflow_stb<KEYW, S_PW, ARW, HBM_OUTSTANDING>(
                    overflow_addr.range(ARW - 1, 0), nm, o_overflow_s_key_strm, o_overflow_s_pld_strm, htb_buf);
                last = i_e_strm.read();
            }
            break;
        case gqe::PartOn | gqe::BloomFilterOn:
        case gqe::BloomFilterOn:
            if (bf_bp_flag) {
                bf_probe_overflow<KEYW, PW, S_PW, ARW, HBM_OUTSTANDING>(
                    i_pld_strm, i_overflow_addr_strm, i_overflow_nm_strm, i_e_strm, htb_buf, part_on,
                    o_overflow_s_key_strm, o_overflow_s_pld_strm, o_overflow_is_in_strm);
            } else {
                bf_build_overflow<PW, S_PW, ARW>(i_pld_strm, i_overflow_addr_strm, i_overflow_nm_strm, i_e_strm,
                                                 htb_buf);
            }
            break;
#ifndef __SYNTHESIS__
        default:
            std::cerr << "Error: illegal kernel switching combination\n";
            exit(1);
#endif
    }
}

/// @brief bloom filter probe for PART flow
template <int HASHWL, int KEYW, int PW, int ARW>
void pre_part_filter(hls::stream<ap_uint<8> >& i_general_cfg_strm,
                     // input
                     hls::stream<ap_uint<HASHWL> >& i_hash_strm,
                     hls::stream<ap_uint<KEYW> >& i_key_strm,
                     hls::stream<ap_uint<PW> >& i_pld_strm,
                     hls::stream<ap_uint<ARW> >& i_nm_strm,
                     hls::stream<bool>& i_e_strm,
                     hls::stream<bool>& i_base_is_in_strm,
                     hls::stream<bool>& i_overflow_is_in_strm,
                     // output
                     hls::stream<ap_uint<HASHWL> >& o_hash_strm,
                     hls::stream<ap_uint<KEYW> >& o_key_strm,
                     hls::stream<ap_uint<PW> >& o_pld_strm,
                     hls::stream<ap_uint<ARW> >& o_nm_strm,
                     hls::stream<bool>& o_e_strm) {
    // if join on: bypass all rows (no hash value)
    // if bloom filter on only: by pass all rows (no hash value)
    // if part on and bloom filter set to probe: filter out invalid rows and separator all Fs (nm[0] == to_overflow)
    // if part on and bloom filter set to probe: bypass all rows and remove separator all Fs (nm[0] == to_overflow)
    ap_uint<8> general_cfg = i_general_cfg_strm.read();
    bool bf_bp_flag = general_cfg[7];

    ap_uint<HASHWL> hash;
    ap_uint<KEYW> key;
    ap_uint<PW> pld;
    ap_uint<ARW> nm;
    bool to_overflow;
    bool is_in = true;
    bool last = i_e_strm.read();
    while (!last) {
#pragma HLS pipeline II = 1
        switch (general_cfg.range(4, 0)) {
            case gqe::JoinOn:
            case gqe::BloomFilterOn:
                key = i_key_strm.read();
                pld = i_pld_strm.read();
                nm = i_nm_strm.read();
                last = i_e_strm.read();
                o_key_strm.write(key);
                o_pld_strm.write(pld);
                o_nm_strm.write(nm);
                o_e_strm.write(false);
                break;
            case gqe::PartOn | gqe::BloomFilterOn:
                hash = i_hash_strm.read();
                key = i_key_strm.read();
                pld = i_pld_strm.read();
                nm = i_nm_strm.read();
                last = i_e_strm.read();
                if (bf_bp_flag) {
                    to_overflow = nm[0];
                    if (to_overflow) {
                        is_in = i_overflow_is_in_strm.read();
                    } else {
                        is_in = i_base_is_in_strm.read();
                    }
                }
                // remove invalid rows for BF probe stage & group separator
                if (is_in && (pld != ~ap_uint<PW>(0))) {
                    o_hash_strm.write(hash);
                    o_key_strm.write(key);
                    o_pld_strm.write(pld);
                    o_e_strm.write(false);
                }
                break;
#ifndef __SYNTHESIS__
            default:
                std::cerr << "Error: illegal kernel switching combination\n";
                exit(1);
#endif
        }
    }
    o_e_strm.write(true);
}

/// @brief top wrapper for handling HBM for Join/BF/Part flows
template <int KEYW, int PW, int S_PW, int HASHWH, int HASHWL, int ARW, int HBM_OUTSTANDING>
void hbm_handler(hls::stream<ap_uint<8> >& i_general_cfg_strm,
                 // input from input mux
                 hls::stream<ap_uint<S_PW> >& i_base_pld_strm,
                 hls::stream<ap_uint<PW> >& i_base_addr_strm,
                 hls::stream<ap_uint<ARW> >& i_base_nm_strm,
                 hls::stream<bool>& i_base_e_strm,
                 hls::stream<ap_uint<S_PW> >& i_overflow_pld_strm,
                 hls::stream<ap_uint<PW> >& i_overflow_addr_strm,
                 hls::stream<ap_uint<ARW> >& i_overflow_nm_strm,
                 hls::stream<bool>& i_overflow_e_strm,

                 hls::stream<ap_uint<HASHWL> >& i_hash_strm,
                 hls::stream<ap_uint<KEYW> >& i_key_strm,
                 hls::stream<ap_uint<PW> >& i_pld_strm,
                 hls::stream<ap_uint<ARW> >& i_nm_strm,
                 hls::stream<bool>& i_e_strm,

                 // output
                 hls::stream<ap_uint<KEYW> >& o_base_s_key_strm,
                 hls::stream<ap_uint<S_PW> >& o_base_s_pld_strm,
                 hls::stream<ap_uint<KEYW> >& o_overflow_s_key_strm,
                 hls::stream<ap_uint<S_PW> >& o_overflow_s_pld_strm,

                 hls::stream<ap_uint<HASHWL> >& o_hash_strm,
                 hls::stream<ap_uint<KEYW> >& o_key_strm,
                 hls::stream<ap_uint<PW> >& o_pld_strm,
                 hls::stream<ap_uint<ARW> >& o_nm_strm,
                 hls::stream<bool>& o_e_strm,

                 // HBM
                 hls::burst_maxi<ap_uint<256> >& stb_buf,
                 hls::burst_maxi<ap_uint<256> >& htb_buf) {
#pragma HLS dataflow

    // dup cfg signals
    hls::stream<ap_uint<8> > general_cfg_strms[3];
#pragma HLS stream variable = general_cfg_strms depth = 2
#pragma HLS bind_storage variable = general_cfg_strms type = fifo impl = srl
    hls::stream<bool> mid_base_is_in_strm;
#pragma HLS stream variable = mid_base_is_in_strm depth = 1024
#pragma HLS bind_storage variable = mid_base_is_in_strm type = fifo impl = srl
    hls::stream<bool> mid_overflow_is_in_strm;
#pragma HLS stream variable = mid_overflow_is_in_strm depth = 1024
#pragma HLS bind_storage variable = mid_overflow_is_in_strm type = fifo impl = srl

    dup_signals<3, 8>(i_general_cfg_strm, general_cfg_strms);

    probe_base_stb<KEYW, PW, S_PW, HASHWH, HASHWL, ARW, HBM_OUTSTANDING>(
        general_cfg_strms[0], i_base_pld_strm, i_base_addr_strm, i_base_nm_strm, i_base_e_strm, //
        o_base_s_key_strm, o_base_s_pld_strm, mid_base_is_in_strm, stb_buf);

    probe_overflow_stb<KEYW, PW, S_PW, HASHWH, HASHWL, ARW, HBM_OUTSTANDING>(
        general_cfg_strms[1], i_overflow_pld_strm, i_overflow_addr_strm, i_overflow_nm_strm, i_overflow_e_strm, //
        o_overflow_s_key_strm, o_overflow_s_pld_strm, mid_overflow_is_in_strm, htb_buf);

    pre_part_filter<HASHWL, KEYW, PW, ARW>(general_cfg_strms[2], i_hash_strm, i_key_strm, i_pld_strm, i_nm_strm,
                                           i_e_strm, mid_base_is_in_strm, mid_overflow_is_in_strm, //
                                           o_hash_strm, o_key_strm, o_pld_strm, o_nm_strm, o_e_strm);
}

/// @brief Top function of hash multi join probe
template <int HASHWH,
          int HASHWL,
          int HASHWJ,
          int KEYW,
          int PW,
          int S_PW,
          int T_PW,
          int ARW,
          int HBM_OUTSTANDING,
          int NUM_X,
          int NUM_Y>
void multi_probe_wrapper(hls::stream<ap_uint<8> >& i_general_cfg_strm,
                         hls::stream<ap_uint<15> >& i_part_cfg_strm,
                         hls::stream<ap_uint<36> >& i_bf_cfg_strm,
                         ap_uint<32>& depth,

                         // input large table
                         hls::stream<ap_uint<HASHWL> >& i_hash_strm,
                         hls::stream<ap_uint<KEYW> >& i_key_strm,
                         hls::stream<ap_uint<PW> >& i_pld_strm,
                         hls::stream<bool>& i_e_strm,

                         // output for join
                         hls::stream<ap_uint<KEYW> >& o_t_key_strm,
                         hls::stream<ap_uint<T_PW> >& o_t_pld_strm,
                         hls::stream<ap_uint<ARW> >& o_t_nm_strm,
                         hls::stream<bool>& o_t_e_strm,

                         hls::stream<ap_uint<KEYW> >& o_base_s_key_strm,
                         hls::stream<ap_uint<S_PW> >& o_base_s_pld_strm,
                         hls::stream<ap_uint<KEYW> >& o_overflow_s_key_strm,
                         hls::stream<ap_uint<S_PW> >& o_overflow_s_pld_strm,

                         hls::burst_maxi<ap_uint<256> >& htb_buf,
                         hls::burst_maxi<ap_uint<256> >& stb_buf,
                         ap_uint<72> (*uram_buffer0)[NUM_X][NUM_Y][4096],
                         ap_uint<72> (*uram_buffer1)[NUM_X][NUM_Y][4096],
                         ap_uint<72> (*uram_buffer2)[NUM_X][NUM_Y][4096]) {
#pragma HLS INLINE off
#pragma HLS DATAFLOW

    // dup cfg signals
    hls::stream<ap_uint<8> > general_cfg_strms[7];
#pragma HLS stream variable = general_cfg_strms depth = 2
#pragma HLS bind_storage variable = general_cfg_strms type = fifo impl = srl

    // multi-hashing
    hls::stream<ap_uint<HASHWL> > mh_hash_strm;
#pragma HLS stream variable = mh_hash_strm depth = 8
#pragma HLS bind_storage variable = mh_hash_strm type = fifo impl = srl
    hls::stream<ap_uint<KEYW> > mh_key_strm;
#pragma HLS stream variable = mh_key_strm depth = 8
#pragma HLS bind_storage variable = mh_key_strm type = fifo impl = srl
    hls::stream<ap_uint<PW> > mh_pld_strm;
#pragma HLS stream variable = mh_pld_strm depth = 8
#pragma HLS bind_storage variable = mh_pld_strm type = fifo impl = srl
    hls::stream<ap_uint<PW> > mh_info_strm;
#pragma HLS stream variable = mh_info_strm depth = 8
#pragma HLS bind_storage variable = mh_info_strm type = fifo impl = srl
    hls::stream<bool> mh_to_strm;
#pragma HLS stream variable = mh_to_strm depth = 8
#pragma HLS bind_storage variable = mh_to_strm type = fifo impl = srl
    hls::stream<bool> mh_e_strm;
#pragma HLS stream variable = mh_e_strm depth = 8
#pragma HLS bind_storage variable = mh_e_strm type = fifo impl = srl
    // input demux
    hls::stream<ap_uint<HASHWL> > idmux_hash_strms[2];
#pragma HLS stream variable = idmux_hash_strms depth = 8
#pragma HLS bind_storage variable = idmux_hash_strms type = fifo impl = srl
    hls::stream<ap_uint<KEYW> > idmux_key_strms[2];
#pragma HLS stream variable = idmux_key_strms depth = 8
#pragma HLS bind_storage variable = idmux_key_strms type = fifo impl = srl
    hls::stream<ap_uint<PW> > idmux_pld_strms[2];
#pragma HLS stream variable = idmux_pld_strms depth = 8
#pragma HLS bind_storage variable = idmux_pld_strms type = fifo impl = srl
    hls::stream<ap_uint<PW> > idmux_info_strms[2];
#pragma HLS stream variable = idmux_info_strms depth = 8
#pragma HLS bind_storage variable = idmux_info_strms type = fifo impl = srl
    hls::stream<bool> idmux_to_strms[2];
#pragma HLS stream variable = idmux_to_strms depth = 8
#pragma HLS bind_storage variable = idmux_to_strms type = fifo impl = srl
    hls::stream<bool> idmux_e_strms[2];
#pragma HLS stream variable = idmux_e_strms depth = 8
#pragma HLS bind_storage variable = idmux_e_strms type = fifo impl = srl
    // base data path
    hls::stream<ap_uint<S_PW> > pld_base_strm("pld_base_strm");
#pragma HLS stream variable = pld_base_strm depth = 8
#pragma HLS bind_storage variable = pld_base_strm type = fifo impl = srl
    hls::stream<ap_uint<PW> > base_addr_strm;
#pragma HLS stream variable = base_addr_strm depth = 512
#pragma HLS bind_storage variable = base_addr_strm type = fifo impl = bram
    hls::stream<ap_uint<ARW> > nm0_strm;
#pragma HLS stream variable = nm0_strm depth = 512
#pragma HLS bind_storage variable = nm0_strm type = fifo impl = bram
    hls::stream<bool> e0_strm;
#pragma HLS stream variable = e0_strm depth = 512
#pragma HLS bind_storage variable = e0_strm type = fifo impl = srl
    // overflow data path
    hls::stream<ap_uint<S_PW> > pld_overflow_strm("pld_overflow_strm");
#pragma HLS stream variable = pld_overflow_strm depth = 8
#pragma HLS bind_storage variable = pld_overflow_strm type = fifo impl = srl
    hls::stream<ap_uint<PW> > overflow_addr_strm;
#pragma HLS stream variable = overflow_addr_strm depth = 8
#pragma HLS bind_storage variable = overflow_addr_strm type = fifo impl = srl
    hls::stream<ap_uint<ARW> > nm1_strm;
#pragma HLS stream variable = nm1_strm depth = 8
#pragma HLS bind_storage variable = nm1_strm type = fifo impl = srl
    hls::stream<bool> e1_strm;
#pragma HLS stream variable = e1_strm depth = 8
#pragma HLS bind_storage variable = e1_strm type = fifo impl = srl
    // big table bypass path
    hls::stream<ap_uint<KEYW> > t_key_strm;
#pragma HLS stream variable = t_key_strm depth = 8
#pragma HLS bind_storage variable = t_key_strm type = fifo impl = srl
    hls::stream<ap_uint<PW> > t_pld_strm;
#pragma HLS stream variable = t_pld_strm depth = 8
#pragma HLS bind_storage variable = t_pld_strm type = fifo impl = srl
    hls::stream<ap_uint<ARW> > t_nm_strm;
#pragma HLS stream variable = t_nm_strm depth = 8
#pragma HLS bind_storage variable = t_nm_strm type = fifo impl = srl
    hls::stream<bool> t_e_strm;
#pragma HLS stream variable = t_e_strm depth = 8
#pragma HLS bind_storage variable = t_e_strm type = fifo impl = srl
    // input mux
    hls::stream<ap_uint<S_PW> > imux_base_pld_strm;
#pragma HLS stream variable = imux_base_pld_strm depth = 8
#pragma HLS bind_storage variable = imux_base_pld_strm type = fifo impl = srl
    hls::stream<ap_uint<PW> > imux_base_addr_strm;
#pragma HLS stream variable = imux_base_addr_strm depth = 8
#pragma HLS bind_storage variable = imux_base_addr_strm type = fifo impl = srl
    hls::stream<ap_uint<ARW> > imux_base_nm_strm;
#pragma HLS stream variable = imux_base_nm_strm depth = 8
#pragma HLS bind_storage variable = imux_base_nm_strm type = fifo impl = srl
    hls::stream<bool> imux_base_e_strm;
#pragma HLS stream variable = imux_base_e_strm depth = 8
#pragma HLS bind_storage variable = imux_base_e_strm type = fifo impl = srl
    hls::stream<ap_uint<S_PW> > imux_overflow_pld_strm;
#pragma HLS stream variable = imux_overflow_pld_strm depth = 8
#pragma HLS bind_storage variable = imux_overflow_pld_strm type = fifo impl = srl
    hls::stream<ap_uint<PW> > imux_overflow_addr_strm;
#pragma HLS stream variable = imux_overflow_addr_strm depth = 8
#pragma HLS bind_storage variable = imux_overflow_addr_strm type = fifo impl = srl
    hls::stream<ap_uint<ARW> > imux_overflow_nm_strm;
#pragma HLS stream variable = imux_overflow_nm_strm depth = 8
#pragma HLS bind_storage variable = imux_overflow_nm_strm type = fifo impl = srl
    hls::stream<bool> imux_overflow_e_strm;
#pragma HLS stream variable = imux_overflow_e_strm depth = 8
#pragma HLS bind_storage variable = imux_overflow_e_strm type = fifo impl = srl
    hls::stream<ap_uint<HASHWL> > imux_t_hash_strm;
#pragma HLS stream variable = imux_t_hash_strm depth = 1024
#pragma HLS bind_storage variable = imux_t_hash_strm type = fifo impl = bram
    hls::stream<ap_uint<KEYW> > imux_t_key_strm;
#pragma HLS stream variable = imux_t_key_strm depth = 1024
#pragma HLS bind_storage variable = imux_t_key_strm type = fifo impl = bram
    hls::stream<ap_uint<PW> > imux_t_pld_strm;
#pragma HLS stream variable = imux_t_pld_strm depth = 1024
#pragma HLS bind_storage variable = imux_t_pld_strm type = fifo impl = bram
    hls::stream<ap_uint<ARW> > imux_t_nm_strm;
#pragma HLS stream variable = imux_t_nm_strm depth = 1024
#pragma HLS bind_storage variable = imux_t_nm_strm type = fifo impl = bram
    hls::stream<bool> imux_t_e_strm;
#pragma HLS stream variable = imux_t_e_strm depth = 1024
#pragma HLS bind_storage variable = imux_t_e_strm type = fifo impl = srl
    // uram handler
    hls::stream<ap_uint<KEYW + PW> > uram_kpld_strm;
#pragma HLS stream variable = uram_kpld_strm depth = 1024
#pragma HLS bind_storage variable = uram_kpld_strm type = fifo impl = bram
    hls::stream<ap_uint<10> > uram_nm_strm;
#pragma HLS stream variable = uram_nm_strm depth = 8
#pragma HLS bind_storage variable = uram_nm_strm type = fifo impl = srl
    hls::stream<ap_uint<10> > uram_bk_nm_strm;
#pragma HLS stream variable = uram_bk_nm_strm depth = 8
#pragma HLS bind_storage variable = uram_bk_nm_strm type = fifo impl = srl
    // hbm handler
    hls::stream<ap_uint<KEYW> > hbm_base_s_key_strm;
#pragma HLS stream variable = hbm_base_s_key_strm depth = 64
#pragma HLS bind_storage variable = hbm_base_s_key_strm type = fifo impl = srl
    hls::stream<ap_uint<S_PW> > hbm_base_s_pld_strm;
#pragma HLS stream variable = hbm_base_s_pld_strm depth = 64
#pragma HLS bind_storage variable = hbm_base_s_pld_strm type = fifo impl = srl
    hls::stream<ap_uint<KEYW> > hbm_overflow_s_key_strm;
#pragma HLS stream variable = hbm_overflow_s_key_strm depth = 64
#pragma HLS bind_storage variable = hbm_overflow_s_key_strm type = fifo impl = srl
    hls::stream<ap_uint<S_PW> > hbm_overflow_s_pld_strm;
#pragma HLS stream variable = hbm_overflow_s_pld_strm depth = 64
#pragma HLS bind_storage variable = hbm_overflow_s_pld_strm type = fifo impl = srl
    hls::stream<ap_uint<HASHWL> > hbm_t_hash_strm;
#pragma HLS stream variable = hbm_t_hash_strm depth = 512
#pragma HLS bind_storage variable = hbm_t_hash_strm type = fifo impl = bram
    hls::stream<ap_uint<KEYW> > hbm_t_key_strm;
#pragma HLS stream variable = hbm_t_key_strm depth = 512
#pragma HLS bind_storage variable = hbm_t_key_strm type = fifo impl = bram
    hls::stream<ap_uint<PW> > hbm_t_pld_strm;
#pragma HLS stream variable = hbm_t_pld_strm depth = 512
#pragma HLS bind_storage variable = hbm_t_pld_strm type = fifo impl = bram
    hls::stream<ap_uint<ARW> > hbm_t_nm_strm;
#pragma HLS stream variable = hbm_t_nm_strm depth = 512
#pragma HLS bind_storage variable = hbm_t_nm_strm type = fifo impl = bram
    hls::stream<bool> hbm_t_e_strm;
#pragma HLS stream variable = hbm_t_e_strm depth = 512
#pragma HLS bind_storage variable = hbm_t_e_strm type = fifo impl = srl
    // output demux
    hls::stream<ap_uint<KEYW> > odmux_base_s_key_strm;
#pragma HLS stream variable = odmux_base_s_key_strm depth = 8
#pragma HLS bind_storage variable = odmux_base_s_key_strm type = fifo impl = srl
    hls::stream<ap_uint<S_PW> > odmux_base_s_pld_strm;
#pragma HLS stream variable = odmux_base_s_pld_strm depth = 8
#pragma HLS bind_storage variable = odmux_base_s_pld_strm type = fifo impl = srl
    hls::stream<ap_uint<KEYW> > odmux_overflow_s_key_strm;
#pragma HLS stream variable = odmux_overflow_s_key_strm depth = 8
#pragma HLS bind_storage variable = odmux_overflow_s_key_strm type = fifo impl = srl
    hls::stream<ap_uint<S_PW> > odmux_overflow_s_pld_strm;
#pragma HLS stream variable = odmux_overflow_s_pld_strm depth = 8
#pragma HLS bind_storage variable = odmux_overflow_s_pld_strm type = fifo impl = srl
    hls::stream<ap_uint<HASHWL> > odmux_t_hash_strm;
#pragma HLS stream variable = odmux_t_hash_strm depth = 8
#pragma HLS bind_storage variable = odmux_t_hash_strm type = fifo impl = srl
    hls::stream<ap_uint<KEYW> > odmux_t_key_strms[2];
#pragma HLS stream variable = odmux_t_key_strms depth = 8
#pragma HLS bind_storage variable = odmux_t_key_strms type = fifo impl = srl
    hls::stream<ap_uint<PW> > odmux_t_pld_strms[2];
#pragma HLS stream variable = odmux_t_pld_strms depth = 8
#pragma HLS bind_storage variable = odmux_t_pld_strms type = fifo impl = srl
    hls::stream<ap_uint<ARW> > odmux_t_nm_strm;
#pragma HLS stream variable = odmux_t_nm_strm depth = 8
#pragma HLS bind_storage variable = odmux_t_nm_strm type = fifo impl = srl
    hls::stream<bool> odmux_t_e_strms[2];
#pragma HLS stream variable = odmux_t_e_strms depth = 8
#pragma HLS bind_storage variable = odmux_t_e_strms type = fifo impl = srl

    dup_signals<7, 8>(i_general_cfg_strm, general_cfg_strms);

    multi_hashing<HASHWH, HASHWL, KEYW, PW>(
        general_cfg_strms[0], i_bf_cfg_strm, i_hash_strm, i_key_strm, i_pld_strm, i_e_strm, // input
        mh_hash_strm, mh_key_strm, mh_pld_strm, mh_info_strm, mh_to_strm, mh_e_strm);       // output

#if !defined(__SYNTHESIS__) && PART_FLOW == 1
    input_demux<HASHWL, KEYW, PW>(general_cfg_strms[1], mh_hash_strm, mh_key_strm, mh_pld_strm, mh_info_strm,
                                  mh_to_strm, mh_e_strm, // from input
                                  idmux_hash_strms, idmux_key_strms, idmux_pld_strms, idmux_info_strms, idmux_to_strms,
                                  idmux_e_strms); // to uram handerl & input mux

    input_mux<HASHWL, KEYW, PW, T_PW, S_PW, ARW>(
        general_cfg_strms[3], idmux_hash_strms[1], idmux_key_strms[1], idmux_pld_strms[1], idmux_info_strms[1],
        idmux_to_strms[1], idmux_e_strms[1], // from input demux
        pld_base_strm, base_addr_strm, nm0_strm, e0_strm, pld_overflow_strm, overflow_addr_strm, nm1_strm, e1_strm,
        t_key_strm, t_pld_strm, t_nm_strm, t_e_strm, // from uram handler

        imux_base_pld_strm, imux_base_addr_strm, imux_base_nm_strm, imux_base_e_strm, imux_overflow_pld_strm,
        imux_overflow_addr_strm, imux_overflow_nm_strm, imux_overflow_e_strm, imux_t_hash_strm, imux_t_key_strm,
        imux_t_pld_strm, imux_t_nm_strm, imux_t_e_strm); // to hbm handler

    hbm_handler<KEYW, PW, S_PW, HASHWH, HASHWL, ARW, HBM_OUTSTANDING>(
        general_cfg_strms[4], imux_base_pld_strm, imux_base_addr_strm, imux_base_nm_strm, imux_base_e_strm,
        imux_overflow_pld_strm, imux_overflow_addr_strm, imux_overflow_nm_strm, imux_overflow_e_strm, imux_t_hash_strm,
        imux_t_key_strm, imux_t_pld_strm, imux_t_nm_strm, imux_t_e_strm, // from input mux

        hbm_base_s_key_strm, hbm_base_s_pld_strm, hbm_overflow_s_key_strm, hbm_overflow_s_pld_strm, hbm_t_hash_strm,
        hbm_t_key_strm, hbm_t_pld_strm, hbm_t_nm_strm, hbm_t_e_strm, // to output demux
        stb_buf, htb_buf);

    output_demux<HASHWL, KEYW, PW, T_PW, S_PW, ARW>(
        general_cfg_strms[5], depth, hbm_t_hash_strm, hbm_t_key_strm, hbm_t_pld_strm, hbm_t_nm_strm, hbm_t_e_strm,
        hbm_base_s_key_strm, hbm_base_s_pld_strm, hbm_overflow_s_key_strm,
        hbm_overflow_s_pld_strm, // from hbm handler

        odmux_t_hash_strm, odmux_t_key_strms, odmux_t_pld_strms, odmux_t_nm_strm, odmux_t_e_strms,
        odmux_base_s_key_strm, odmux_base_s_pld_strm, odmux_overflow_s_key_strm,
        odmux_overflow_s_pld_strm); // to uram handler & output mux

    uram_handler<HASHWH, HASHWL, HASHWJ, KEYW, PW, S_PW, T_PW, ARW, NUM_X, NUM_Y>(
        general_cfg_strms[2], i_part_cfg_strm, depth, //
        idmux_hash_strms[0], idmux_key_strms[0], idmux_pld_strms[0], idmux_info_strms[0], idmux_to_strms[0],
        idmux_e_strms[0], // from input demux
        odmux_t_hash_strm, odmux_t_key_strms[1], odmux_t_pld_strms[1],
        odmux_t_e_strms[1], // from output demux

        pld_base_strm, base_addr_strm, nm0_strm, e0_strm, pld_overflow_strm, overflow_addr_strm, nm1_strm, e1_strm,
        t_key_strm, t_pld_strm, t_nm_strm, t_e_strm,   // to input mux
        uram_kpld_strm, uram_nm_strm, uram_bk_nm_strm, // to output mux

        uram_buffer0, uram_buffer1, uram_buffer2);

    output_mux<HASHWL, KEYW, PW, T_PW, S_PW, ARW>(
        general_cfg_strms[6], depth, odmux_t_key_strms[0], odmux_t_pld_strms[0], odmux_t_nm_strm, odmux_t_e_strms[0],
        odmux_base_s_key_strm, odmux_base_s_pld_strm, odmux_overflow_s_key_strm,
        odmux_overflow_s_pld_strm,                     // from output demux
        uram_kpld_strm, uram_nm_strm, uram_bk_nm_strm, // from uram handler

        o_t_key_strm, o_t_pld_strm, o_t_nm_strm, o_t_e_strm, o_base_s_key_strm, o_base_s_pld_strm,
        o_overflow_s_key_strm, o_overflow_s_pld_strm); // to output

#else

    input_demux<HASHWL, KEYW, PW>(general_cfg_strms[1], mh_hash_strm, mh_key_strm, mh_pld_strm, mh_info_strm,
                                  mh_to_strm, mh_e_strm, // from input
                                  idmux_hash_strms, idmux_key_strms, idmux_pld_strms, idmux_info_strms, idmux_to_strms,
                                  idmux_e_strms); // to uram handerl & input mux

    uram_handler<HASHWH, HASHWL, HASHWJ, KEYW, PW, S_PW, T_PW, ARW, NUM_X, NUM_Y>(
        general_cfg_strms[2], i_part_cfg_strm, depth, //
        idmux_hash_strms[0], idmux_key_strms[0], idmux_pld_strms[0], idmux_info_strms[0], idmux_to_strms[0],
        idmux_e_strms[0], // from input demux
        odmux_t_hash_strm, odmux_t_key_strms[1], odmux_t_pld_strms[1],
        odmux_t_e_strms[1], // from output demux

        pld_base_strm, base_addr_strm, nm0_strm, e0_strm, pld_overflow_strm, overflow_addr_strm, nm1_strm, e1_strm,
        t_key_strm, t_pld_strm, t_nm_strm, t_e_strm,   // to input mux
        uram_kpld_strm, uram_nm_strm, uram_bk_nm_strm, // to output mux

        uram_buffer0, uram_buffer1, uram_buffer2);

    input_mux<HASHWL, KEYW, PW, T_PW, S_PW, ARW>(
        general_cfg_strms[3], idmux_hash_strms[1], idmux_key_strms[1], idmux_pld_strms[1], idmux_info_strms[1],
        idmux_to_strms[1], idmux_e_strms[1], // from input demux
        pld_base_strm, base_addr_strm, nm0_strm, e0_strm, pld_overflow_strm, overflow_addr_strm, nm1_strm, e1_strm,
        t_key_strm, t_pld_strm, t_nm_strm, t_e_strm, // from uram handler

        imux_base_pld_strm, imux_base_addr_strm, imux_base_nm_strm, imux_base_e_strm, imux_overflow_pld_strm,
        imux_overflow_addr_strm, imux_overflow_nm_strm, imux_overflow_e_strm, imux_t_hash_strm, imux_t_key_strm,
        imux_t_pld_strm, imux_t_nm_strm, imux_t_e_strm); // to hbm handler

    hbm_handler<KEYW, PW, S_PW, HASHWH, HASHWL, ARW, HBM_OUTSTANDING>(
        general_cfg_strms[4], imux_base_pld_strm, imux_base_addr_strm, imux_base_nm_strm, imux_base_e_strm,
        imux_overflow_pld_strm, imux_overflow_addr_strm, imux_overflow_nm_strm, imux_overflow_e_strm, imux_t_hash_strm,
        imux_t_key_strm, imux_t_pld_strm, imux_t_nm_strm, imux_t_e_strm, // from input mux

        hbm_base_s_key_strm, hbm_base_s_pld_strm, hbm_overflow_s_key_strm, hbm_overflow_s_pld_strm, hbm_t_hash_strm,
        hbm_t_key_strm, hbm_t_pld_strm, hbm_t_nm_strm, hbm_t_e_strm, // to output demux
        stb_buf, htb_buf);

    output_demux<HASHWL, KEYW, PW, T_PW, S_PW, ARW>(
        general_cfg_strms[5], depth, hbm_t_hash_strm, hbm_t_key_strm, hbm_t_pld_strm, hbm_t_nm_strm, hbm_t_e_strm,
        hbm_base_s_key_strm, hbm_base_s_pld_strm, hbm_overflow_s_key_strm,
        hbm_overflow_s_pld_strm, // from hbm handler

        odmux_t_hash_strm, odmux_t_key_strms, odmux_t_pld_strms, odmux_t_nm_strm, odmux_t_e_strms,
        odmux_base_s_key_strm, odmux_base_s_pld_strm, odmux_overflow_s_key_strm,
        odmux_overflow_s_pld_strm); // to uram handler & output mux

    output_mux<HASHWL, KEYW, PW, T_PW, S_PW, ARW>(
        general_cfg_strms[6], depth, odmux_t_key_strms[0], odmux_t_pld_strms[0], odmux_t_nm_strm, odmux_t_e_strms[0],
        odmux_base_s_key_strm, odmux_base_s_pld_strm, odmux_overflow_s_key_strm,
        odmux_overflow_s_pld_strm,                     // from output demux
        uram_kpld_strm, uram_nm_strm, uram_bk_nm_strm, // from uram handler

        o_t_key_strm, o_t_pld_strm, o_t_nm_strm, o_t_e_strm, o_base_s_key_strm, o_base_s_pld_strm,
        o_overflow_s_key_strm, o_overflow_s_pld_strm); // to output
#endif
}

} // namespace details
} // namespace database
} // namespace xf

#endif // GQE_ISV_PROBE_FLOW_HPP
