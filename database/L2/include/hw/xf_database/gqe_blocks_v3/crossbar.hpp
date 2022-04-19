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

/**
 * @file crossbar.hpp
 * @brief 4-to-8 crossbar, sub-sections divided by payload (row-id) = -1.
 *
 *
 * This file is part of Vitis Database Library.
 */

#ifndef GQE_ISV_CROSSBAR_HPP
#define GQE_ISV_CROSSBAR_HPP

#ifndef __cplusplus
#error "Vitis Database Library only works with C++."
#endif

// for using mul_ch_read & mux
#include "xf_database/hash_join_v2.hpp"

#include "ap_int.h"
#include "hls_stream.h"

// for using dup_signals
#include "xf_database/gqe_blocks_v3/gqe_traits.hpp"

#ifndef __SYNTHESIS__
#include <iostream>
#endif

namespace xf {
namespace database {
namespace details {

//-------------------------------dispatch--------------------------------------

/// @brief Calculate hash value based on key
template <int HASH_MODE, int KEYW, int HASHW>
void hash_wrapper(hls::stream<ap_uint<8> >& i_general_cfg_strm,
                  hls::stream<ap_uint<KEYW> >& i_key_strm,
                  hls::stream<bool>& i_e_strm,

                  hls::stream<ap_uint<HASHW> >& o_hash_strm,
                  hls::stream<ap_uint<KEYW> >& o_key_strm,
                  hls::stream<bool>& o_e_strm) {
#pragma HLS INLINE off

    hls::stream<ap_uint<KEYW> > key_strm_in;
#pragma HLS STREAM variable = key_strm_in depth = 8
#pragma HLS bind_storage variable = key_strm_in type = fifo impl = srl
    hls::stream<ap_uint<64> > hash_strm_out;
#pragma HLS STREAM variable = hash_strm_out depth = 8
#pragma HLS bind_storage variable = hash_strm_out type = fifo impl = srl

#ifndef __SYNTHESIS__
    unsigned int cnt = 0;
#endif
    ap_uint<8> general_cfg = i_general_cfg_strm.read();
    bool part_on = general_cfg[3];

    bool last = i_e_strm.read();
BUILD_HASH_LOOP:
    while (!last) {
#pragma HLS loop_tripcount min = 1 max = 1000
#pragma HLS PIPELINE II = 1

        bool blk = i_e_strm.empty() || i_key_strm.empty() || o_key_strm.full();
        if (!blk) {
            last = i_e_strm.read();
            ap_uint<KEYW> key = i_key_strm.read();

            o_key_strm.write(key);
            o_e_strm.write(false);

            if (HASH_MODE == 0) {
                // radix hash
                ap_uint<HASHW> s_hash_val = key(HASHW - 1, 0);
                o_hash_strm.write(s_hash_val);

            } else {
                // Jekins lookup3 hash
                key_strm_in.write(key);
                ap_uint<32> seed = 0xdeadbeef;
                if (part_on) {
                    seed = 13;
                }
                xf::database::hashLookup3<KEYW>(seed, key_strm_in, hash_strm_out);
                ap_uint<64> l_hash_val = hash_strm_out.read();

                ap_uint<HASHW> s_hash_val = l_hash_val(HASHW - 1, 0);
                o_hash_strm.write(s_hash_val);

#ifndef __SYNTHESIS__

#ifdef DEBUG_MISS
                if (key == 3680482 || key == 3691265 || key == 4605699 || key == 4987782)
                    std::cout << std::hex << "hashwrapper: key=" << key << " hash=" << s_hash_val << std::endl;
#endif

#ifdef DEBUG
                if (cnt < 10) {
                    std::cout << std::hex << "hash wrapper: cnt=" << cnt << " key = " << key
                              << " hash_val = " << s_hash_val << std::endl;
                }
#endif
                cnt++;
#endif
            }
        }
    }
    o_e_strm.write(true);
}

/// @brief Dispatch data to multiple PU based one the hash value, every PU with
/// different high bit hash_value.
template <int KEYW, int PW, int HASHWH, int HASHWL, int PU, int CH_NM>
void dispatch(hls::stream<ap_uint<8> >& i_general_cfg_strm,
              hls::stream<ap_uint<KEYW> >& i_key_strm,
              hls::stream<ap_uint<PW> >& i_pld_strm,
              hls::stream<ap_uint<HASHWH + HASHWL> >& i_hash_strm,
              hls::stream<bool>& i_e_strm,

              hls::stream<ap_uint<KEYW + PW + HASHWL + 1> > o_pack_strm[PU]) {
#pragma HLS INLINE off

    ap_uint<8> general_cfg = i_general_cfg_strm.read();
    bool bf_on = general_cfg[2];

    bool last = i_e_strm.read();
LOOP_DISPATCH:
    while (!last) {
#pragma HLS pipeline II = 1

        ap_uint<4> idx;
        ap_uint<HASHWH + HASHWL> hash_val = i_hash_strm.read();
        ap_uint<KEYW> key = i_key_strm.read();
        ap_uint<PW> pld = i_pld_strm.read();
        last = i_e_strm.read();

        ap_uint<HASHWL> hash_out = hash_val(HASHWL - 1, 0);
        if (HASHWH > 0)
            idx = hash_val(HASHWH + HASHWL - 1, HASHWL);
        else
            idx = 0;

        if (bf_on && (pld == ~ap_uint<PW>(0))) {
            for (int i = 0; i < PU; i++) {
#pragma HLS unroll
                ap_uint<KEYW + PW + HASHWL + 1> o_pack = 0;
                o_pack.range(KEYW + PW + HASHWL, PW + HASHWL + 1) = ~ap_uint<KEYW>(0);
                o_pack.range(PW + HASHWL, HASHWL + 1) = ~ap_uint<PW>(0);
                o_pack.range(HASHWL, 1) = ~ap_uint<HASHWL>(0);
                o_pack[0] = 0;
                o_pack_strm[i].write(o_pack);
            }
        } else {
            ap_uint<KEYW + PW + HASHWL + 1> o_pack = 0;
            o_pack.range(KEYW + PW + HASHWL, PW + HASHWL + 1) = key;
            o_pack.range(PW + HASHWL, HASHWL + 1) = pld;
            o_pack.range(HASHWL, 1) = hash_out;
            o_pack[0] = 0;
            o_pack_strm[idx].write(o_pack);
        }
    }

    // for do_while in merge function, why not use while() in merge function?
    for (int i = 0; i < PU; i++) {
#pragma HLS unroll
        // if add merge module, need uncomment
        ap_uint<KEYW + PW + HASHWL + 1> o_pack = 0;
        o_pack[0] = 1;
        o_pack_strm[i].write(o_pack);
    }
}

/// @brief top wrapper of the dispatcher
template <int HASH_MODE, int KEYW, int PW, int HASHWH, int HASHWL, int PU, int CH_NM>
void dispatch_unit(hls::stream<ap_uint<8> >& i_general_cfg_strm,
                   hls::stream<ap_uint<KEYW> >& i_key_strm,
                   hls::stream<ap_uint<PW> >& i_pld_strm,
                   hls::stream<bool>& i_e_strm,

                   hls::stream<ap_uint<KEYW + PW + HASHWL + 1> > o_pack_strm[PU]) {
#pragma HLS DATAFLOW

    hls::stream<ap_uint<HASHWH + HASHWL> > hash_strm;
#pragma HLS STREAM variable = hash_strm depth = 8
#pragma HLS bind_storage variable = hash_strm type = fifo impl = srl
    hls::stream<ap_uint<KEYW> > key_strm;
#pragma HLS STREAM variable = key_strm depth = 32
#pragma HLS bind_storage variable = key_strm type = fifo impl = srl
    hls::stream<bool> e_strm;
#pragma HLS STREAM variable = e_strm depth = 32

    hls::stream<ap_uint<8> > general_cfg_strms[2];
#pragma HLS stream variable = general_cfg_strms depth = 2
#pragma HLS bind_storage variable = general_cfg_strms type = fifo impl = srl

    dup_signals<2, 8>(i_general_cfg_strm, general_cfg_strms);

    hash_wrapper<HASH_MODE, KEYW, HASHWH + HASHWL>(general_cfg_strms[0], i_key_strm, i_e_strm, hash_strm, key_strm,
                                                   e_strm);

    dispatch<KEYW, PW, HASHWH, HASHWL, PU, CH_NM>(general_cfg_strms[1], key_strm, i_pld_strm, hash_strm, e_strm,
                                                  o_pack_strm);
}

//-------------------------------merge--------------------------------------

template <int KEYW, int PW, int HASHW>
void merge1_1(hls::stream<ap_uint<8> >& i_general_cfg_strm,
              hls::stream<ap_uint<KEYW + PW + HASHW + 1> >& i_pack_strm,

              hls::stream<ap_uint<KEYW> >& o_key_strm,
              hls::stream<ap_uint<PW> >& o_pld_strm,
              hls::stream<ap_uint<HASHW> >& o_hash_strm,
              hls::stream<bool>& o_e_strm) {
    ap_uint<8> general_cfg = i_general_cfg_strm.read();
    bool bf_on = general_cfg[2];
    bool last = 0;
LOOP_MERGE1_1:
    do {
#pragma HLS pipeline II = 1
        ap_uint<KEYW + PW + HASHW + 1> i_pack = i_pack_strm.read();
        ap_uint<KEYW> key = i_pack.range(KEYW + PW + HASHW, PW + HASHW + 1);
        ap_uint<PW> pld = i_pack.range(PW + HASHW, HASHW + 1);
        ap_uint<HASHW> hash_val = i_pack.range(HASHW, 1);
        last = i_pack[0];
        if (!last) {
            // dynamic filter with condition: row-id > 0
            if (!(bf_on && pld == 0)) {
                o_key_strm.write(key);
                o_pld_strm.write(pld);
                o_hash_strm.write(hash_val);
                o_e_strm.write(false);
            }
        }
    } while (!last);
    o_e_strm.write(true);
}

// @brief 2-to-1 merge, packed input & splitted output with all Fs as separator
// XXX: caution, this implementation is really tricky for the HLS, currently only verified with 2020.2 released
// Vitis-HLS.
// Better don't touch this module if not necessary, as logic may be mis-synthesized (Csim/Cosim mismatch).
template <int KEYW, int PW, int HASHW>
void merge2_1(hls::stream<ap_uint<8> >& i_general_cfg_strm,
              hls::stream<ap_uint<KEYW + PW + HASHW + 1> >& i0_pack_strm,
              hls::stream<ap_uint<KEYW + PW + HASHW + 1> >& i1_pack_strm,

              hls::stream<ap_uint<KEYW> >& o_key_strm,
              hls::stream<ap_uint<PW> >& o_pld_strm,
              hls::stream<ap_uint<HASHW> >& o_hash_strm,
              hls::stream<bool>& o_e_strm) {
    ap_uint<KEYW> key_arry[2];
#pragma HLS array_partition variable = key_arry dim = 1
    ap_uint<PW> pld_arry[2];
#pragma HLS array_partition variable = pld_arry dim = 1
    ap_uint<HASHW> hash_val_arry[2];
#pragma HLS array_partition variable = hash_val_arry dim = 1
    ap_uint<2> empty_e = 0;
    ap_uint<2> rd_e = 0;
    ap_uint<2> last = 0;
    ap_uint<2> sec_end = 0;
    ap_uint<KEYW + PW + HASHW + 1> i_pack;
#ifndef __SYNTHESIS__
    unsigned int cnt = 0;
#endif
    ap_uint<8> general_cfg = i_general_cfg_strm.read();
    bool bf_on = general_cfg[2];
LOOP_MERGE2_1:
    do {
#pragma HLS loop_tripcount min = 1 max = 5000
#pragma HLS PIPELINE II = 1
        empty_e[0] = !i0_pack_strm.empty() && !last[0] && !sec_end[0];
        empty_e[1] = !i1_pack_strm.empty() && !last[1] && !sec_end[1];
        rd_e = join_v2::mul_ch_read(empty_e);
        if (rd_e[0]) {
            i_pack = i0_pack_strm.read();
            key_arry[0] = i_pack.range(KEYW + PW + HASHW, PW + HASHW + 1);
            pld_arry[0] = i_pack.range(PW + HASHW, HASHW + 1);
            hash_val_arry[0] = i_pack.range(HASHW, 1);
            last[0] = i_pack[0];
            if (pld_arry[0] == ~ap_uint<PW>(0)) {
                sec_end[0] = 1;
            }
        }
        if (rd_e[1]) {
            i_pack = i1_pack_strm.read();
            key_arry[1] = i_pack.range(KEYW + PW + HASHW, PW + HASHW + 1);
            pld_arry[1] = i_pack.range(PW + HASHW, HASHW + 1);
            hash_val_arry[1] = i_pack.range(HASHW, 1);
            last[1] = i_pack[0];
            if (pld_arry[1] == ~ap_uint<PW>(0)) {
                sec_end[1] = 1;
            }
        }
        // only support 8 channels, 4 channels and 2 channels
        ap_uint<3> id = join_v2::mux<2>(rd_e);
        ap_uint<KEYW> key = key_arry[id];
        ap_uint<PW> pld = pld_arry[id];
        ap_uint<HASHW> hash_val = hash_val_arry[id];
        bool valid_n = last[id];
        if (!valid_n && rd_e != 0 && !sec_end[id]) {
#ifndef __SYNTHESIS__
            cnt++;
#endif
            // dynamic filter with condition: row-id > 0
            if (!(bf_on && pld == 0)) {
                o_key_strm.write(key);
                o_pld_strm.write(pld);
                o_hash_strm.write(hash_val);
                o_e_strm.write(false);
            }
        } else if (sec_end == 0x3) {
            o_key_strm.write(~ap_uint<KEYW>(0));
            o_pld_strm.write(~ap_uint<PW>(0));
            o_hash_strm.write(~ap_uint<HASHW>(0));
            o_e_strm.write(false);
            sec_end = 0;
        }
    } while (last != 0x3);
    o_e_strm.write(true);
}

// @brief 4-to-1 merge, packed input & splitted output with all Fs as separator
// XXX: caution, this implementation is really tricky for the HLS, currently only verified with 2020.2 released
// Vitis-HLS.
// Better don't touch this module if not necessary, as logic may be mis-synthesized (Csim/Cosim mismatch).
template <int KEYW, int PW, int HASHW>
void merge4_1(hls::stream<ap_uint<8> >& i_general_cfg_strm,
              hls::stream<ap_uint<KEYW + PW + HASHW + 1> >& i0_pack_strm,
              hls::stream<ap_uint<KEYW + PW + HASHW + 1> >& i1_pack_strm,
              hls::stream<ap_uint<KEYW + PW + HASHW + 1> >& i2_pack_strm,
              hls::stream<ap_uint<KEYW + PW + HASHW + 1> >& i3_pack_strm,

              hls::stream<ap_uint<KEYW> >& o_key_strm,
              hls::stream<ap_uint<PW> >& o_pld_strm,
              hls::stream<ap_uint<HASHW> >& o_hash_strm,
              hls::stream<bool>& o_e_strm) {
    ap_uint<KEYW> key_arry[4];
#pragma HLS array_partition variable = key_arry dim = 1
    ap_uint<PW> pld_arry[4];
#pragma HLS array_partition variable = pld_arry dim = 1
    ap_uint<HASHW> hash_val_arry[4];
#pragma HLS array_partition variable = hash_val_arry dim = 1
    ap_uint<4> empty_e = 0;
    ap_uint<4> rd_e = 0;
    ap_uint<4> last = 0;
    ap_uint<4> sec_end = 0;
    ap_uint<KEYW + PW + HASHW + 1> i_pack;
#ifndef __SYNTHESIS__
    unsigned int cnt = 0;
#endif
    ap_uint<8> general_cfg = i_general_cfg_strm.read();
    bool bf_on = general_cfg[2];
LOOP_MERGE4_1:
    do {
#pragma HLS loop_tripcount min = 1 max = 5000
#pragma HLS PIPELINE II = 1
        empty_e[0] = !i0_pack_strm.empty() && !last[0] && !sec_end[0];
        empty_e[1] = !i1_pack_strm.empty() && !last[1] && !sec_end[1];
        empty_e[2] = !i2_pack_strm.empty() && !last[2] && !sec_end[2];
        empty_e[3] = !i3_pack_strm.empty() && !last[3] && !sec_end[3];
        rd_e = join_v2::mul_ch_read(empty_e);
        if (rd_e[0]) {
            i_pack = i0_pack_strm.read();
            key_arry[0] = i_pack.range(KEYW + PW + HASHW, PW + HASHW + 1);
            pld_arry[0] = i_pack.range(PW + HASHW, HASHW + 1);
            hash_val_arry[0] = i_pack.range(HASHW, 1);
            last[0] = i_pack[0];
            if (pld_arry[0] == ~ap_uint<PW>(0)) {
                sec_end[0] = 1;
            }
        }
        if (rd_e[1]) {
            i_pack = i1_pack_strm.read();
            key_arry[1] = i_pack.range(KEYW + PW + HASHW, PW + HASHW + 1);
            pld_arry[1] = i_pack.range(PW + HASHW, HASHW + 1);
            hash_val_arry[1] = i_pack.range(HASHW, 1);
            last[1] = i_pack[0];
            if (pld_arry[1] == ~ap_uint<PW>(0)) {
                sec_end[1] = 1;
            }
        }
        if (rd_e[2]) {
            i_pack = i2_pack_strm.read();
            key_arry[2] = i_pack.range(KEYW + PW + HASHW, PW + HASHW + 1);
            pld_arry[2] = i_pack.range(PW + HASHW, HASHW + 1);
            hash_val_arry[2] = i_pack.range(HASHW, 1);
            last[2] = i_pack[0];
            if (pld_arry[2] == ~ap_uint<PW>(0)) {
                sec_end[2] = 1;
            }
        }
        if (rd_e[3]) {
            i_pack = i3_pack_strm.read();
            key_arry[3] = i_pack.range(KEYW + PW + HASHW, PW + HASHW + 1);
            pld_arry[3] = i_pack.range(PW + HASHW, HASHW + 1);
            hash_val_arry[3] = i_pack.range(HASHW, 1);
            last[3] = i_pack[0];
            if (pld_arry[3] == ~ap_uint<PW>(0)) {
                sec_end[3] = 1;
            }
        }
        // only support 8 channels, 4 channels and 2 channels
        ap_uint<3> id = join_v2::mux<4>(rd_e);
        ap_uint<KEYW> key = key_arry[id];
        ap_uint<PW> pld = pld_arry[id];
        ap_uint<HASHW> hash_val = hash_val_arry[id];
        bool valid_n = last[id];
        if (!valid_n && rd_e != 0 && !sec_end[id]) {
#ifndef __SYNTHESIS__
            cnt++;
#endif
            // dynamic filter with condition: row-id > 0
            if (!(bf_on && pld == 0)) {
                o_key_strm.write(key);
                o_pld_strm.write(pld);
                o_hash_strm.write(hash_val);
                o_e_strm.write(false);
            }
        } else if (sec_end == 0xf) {
            o_key_strm.write(~ap_uint<KEYW>(0));
            o_pld_strm.write(~ap_uint<PW>(0));
            o_hash_strm.write(~ap_uint<HASHW>(0));
            o_e_strm.write(false);
            sec_end = 0;
        }
    } while (last != 0xf);
    o_e_strm.write(true);
}

} // namespace details
} // namespace database
} // namespace xf

#endif // GQE_ISV_CROSSBAR_HPP
