/*
 * Copyright 2019 Xilinx, Inc.
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

#ifndef GQE_ISV_DEMUX_AND_MUX_HPP
#define GQE_ISV_DEMUX_AND_MUX_HPP

#include "xf_database/gqe_blocks_v3/gqe_enums.hpp"

#include "ap_int.h"
#include "hls_stream.h"

#ifndef __SYNTHESIS__
#include <stdio.h>
#endif

namespace xf {
namespace database {
namespace details {

// ************************************ Input ***************************************

/// @brief demux data to uram handler or input mux
template <int HASHWL, int KEYW, int PW>
void input_demux(hls::stream<ap_uint<8> >& i_general_cfg_strm,

                 // input
                 hls::stream<ap_uint<HASHWL> >& i_hash_strm,
                 hls::stream<ap_uint<KEYW> >& i_key_strm,
                 hls::stream<ap_uint<PW> >& i_pld_strm,
                 hls::stream<ap_uint<PW> >& i_bf_info_strm,
                 hls::stream<bool>& i_to_strm,
                 hls::stream<bool>& i_e_strm,

                 // output
                 hls::stream<ap_uint<HASHWL> > o_hash_strm[2],
                 hls::stream<ap_uint<KEYW> > o_key_strm[2],
                 hls::stream<ap_uint<PW> > o_pld_strm[2],
                 hls::stream<ap_uint<PW> > o_bf_info_strm[2],
                 hls::stream<bool> o_to_strm[2],
                 hls::stream<bool> o_e_strm[2]) {
    ap_uint<8> general_cfg = i_general_cfg_strm.read();
    bool part_on = general_cfg[3];

    bool last = i_e_strm.read();
    while (!last) {
#pragma HLS pipeline II = 1
        ap_uint<HASHWL> hash = i_hash_strm.read();
        ap_uint<KEYW> key = i_key_strm.read();
        ap_uint<PW> pld = i_pld_strm.read();
        ap_uint<PW> bf_info = i_bf_info_strm.read();
        bool to_overflow = i_to_strm.read();
        last = i_e_strm.read();

        switch (general_cfg.range(4, 0)) {
            case gqe::JoinOn:
            case gqe::BloomFilterOn:
                o_hash_strm[0].write(hash);
                o_key_strm[0].write(key);
                o_pld_strm[0].write(pld);
                o_bf_info_strm[0].write(bf_info);
                o_to_strm[0].write(to_overflow);
                o_e_strm[0].write(false);
                break;
            case gqe::PartOn | gqe::BloomFilterOn:
                o_hash_strm[1].write(hash);
                o_key_strm[1].write(key);
                o_pld_strm[1].write(pld);
                o_bf_info_strm[1].write(bf_info);
                o_to_strm[1].write(to_overflow);
                o_e_strm[1].write(false);
                break;
#ifndef __SYNTHESIS__
            default:
                std::cerr << "Error: illegal kernel switching combination\n";
                exit(1);
#endif
        }
    }
    if (part_on) {
        o_e_strm[1].write(true);
    } else {
        o_e_strm[0].write(true);
    }
}

template <int PW, int B_PW, int ARW>
void imux_read_join(
    // input
    hls::stream<ap_uint<PW> >& i_addr_strm,
    hls::stream<ap_uint<ARW> >& i_nm_strm,
    hls::stream<bool>& i_e_strm,
    // output
    hls::stream<ap_uint<PW> >& o_addr_strm,
    hls::stream<ap_uint<ARW> >& o_nm_strm,
    hls::stream<bool>& o_e_strm) {
    bool last = i_e_strm.read();
    while (!last) {
#pragma HLS pipeline II = 1
        ap_uint<PW> addr = i_addr_strm.read();
        ap_uint<ARW> nm = i_nm_strm.read();
        last = i_e_strm.read();
        o_addr_strm.write(addr);
        o_nm_strm.write(nm);
        o_e_strm.write(false);
    }
    o_e_strm.write(true);
}

template <int PW, int B_PW, int ARW>
void imux_read_bf(
    // input
    hls::stream<ap_uint<B_PW> >& i_pld_strm,
    hls::stream<ap_uint<PW> >& i_addr_strm,
    hls::stream<ap_uint<ARW> >& i_nm_strm,
    hls::stream<bool>& i_e_strm,
    // output
    hls::stream<ap_uint<B_PW> >& o_pld_strm,
    hls::stream<ap_uint<PW> >& o_addr_strm,
    hls::stream<ap_uint<ARW> >& o_nm_strm,
    hls::stream<bool>& o_e_strm) {
    bool last = i_e_strm.read();
    while (!last) {
#pragma HLS pipeline II = 1
        ap_uint<B_PW> pld = i_pld_strm.read();
        ap_uint<PW> addr = i_addr_strm.read();
        ap_uint<ARW> nm = i_nm_strm.read();
        last = i_e_strm.read();
        o_pld_strm.write(pld);
        o_addr_strm.write(addr);
        o_nm_strm.write(nm);
        o_e_strm.write(false);
    }
    o_e_strm.write(true);
}

template <int KEYW, int B_PW, int ARW>
void imux_row(
    // input
    hls::stream<ap_uint<KEYW> >& i_t_key_strm,
    hls::stream<ap_uint<B_PW> >& i_t_pld_strm,
    hls::stream<ap_uint<ARW> >& i_t_nm_strm,
    hls::stream<bool>& i_t_e_strm,
    // output
    hls::stream<ap_uint<KEYW> >& o_key_strm,
    hls::stream<ap_uint<B_PW> >& o_pld_strm,
    hls::stream<ap_uint<ARW> >& o_nm_strm,
    hls::stream<bool>& o_e_strm) {
    bool last = i_t_e_strm.read();
    while (!last) {
#pragma HLS pipeline II = 1
        ap_uint<KEYW> key = i_t_key_strm.read();
        ap_uint<B_PW> pld = i_t_pld_strm.read();
        ap_uint<ARW> nm = i_t_nm_strm.read();
        last = i_t_e_strm.read();
        o_key_strm.write(key);
        o_pld_strm.write(pld);
        o_nm_strm.write(nm);
        o_e_strm.write(false);
    }
    o_e_strm.write(true);
}

template <int HASHWL, int KEYW, int PW, int B_PW, int ARW>
void imux_for_join(
    // input from uram handler
    hls::stream<ap_uint<PW> >& i_base_addr_strm,
    hls::stream<ap_uint<ARW> >& i_base_nm_strm,
    hls::stream<bool>& i_base_e_strm,
    hls::stream<ap_uint<PW> >& i_overflow_addr_strm,
    hls::stream<ap_uint<ARW> >& i_overflow_nm_strm,
    hls::stream<bool>& i_overflow_e_strm,
    hls::stream<ap_uint<KEYW> >& i_t_key_strm,
    hls::stream<ap_uint<B_PW> >& i_t_pld_strm,
    hls::stream<ap_uint<ARW> >& i_t_nm_strm,
    hls::stream<bool>& i_t_e_strm,
    // output to hbm handler
    hls::stream<ap_uint<PW> >& o_base_addr_strm,
    hls::stream<ap_uint<ARW> >& o_base_nm_strm,
    hls::stream<bool>& o_base_e_strm,
    hls::stream<ap_uint<PW> >& o_overflow_addr_strm,
    hls::stream<ap_uint<ARW> >& o_overflow_nm_strm,
    hls::stream<bool>& o_overflow_e_strm,
    hls::stream<ap_uint<KEYW> >& o_key_strm,
    hls::stream<ap_uint<PW> >& o_pld_strm,
    hls::stream<ap_uint<ARW> >& o_nm_strm,
    hls::stream<bool>& o_e_strm) {
#pragma HLS dataflow
    imux_read_join<PW, B_PW, ARW>(i_base_addr_strm, i_base_nm_strm, i_base_e_strm, //
                                  o_base_addr_strm, o_base_nm_strm, o_base_e_strm);
    imux_read_join<PW, B_PW, ARW>(i_overflow_addr_strm, i_overflow_nm_strm, i_overflow_e_strm, //
                                  o_overflow_addr_strm, o_overflow_nm_strm, o_overflow_e_strm);
    imux_row<KEYW, B_PW, ARW>(i_t_key_strm, i_t_pld_strm, i_t_nm_strm, i_t_e_strm, //
                              o_key_strm, o_pld_strm, o_nm_strm, o_e_strm);
}

template <int HASHWL, int KEYW, int PW, int B_PW, int ARW>
void imux_for_bf(
    // input from uram handler
    hls::stream<ap_uint<B_PW> >& i_base_pld_strm,
    hls::stream<ap_uint<PW> >& i_base_addr_strm,
    hls::stream<ap_uint<ARW> >& i_base_nm_strm,
    hls::stream<bool>& i_base_e_strm,
    hls::stream<ap_uint<B_PW> >& i_overflow_pld_strm,
    hls::stream<ap_uint<PW> >& i_overflow_addr_strm,
    hls::stream<ap_uint<ARW> >& i_overflow_nm_strm,
    hls::stream<bool>& i_overflow_e_strm,
    hls::stream<ap_uint<KEYW> >& i_t_key_strm,
    hls::stream<ap_uint<B_PW> >& i_t_pld_strm,
    hls::stream<ap_uint<ARW> >& i_t_nm_strm,
    hls::stream<bool>& i_t_e_strm,
    // output to hbm handler
    hls::stream<ap_uint<B_PW> >& o_base_pld_strm,
    hls::stream<ap_uint<PW> >& o_base_addr_strm,
    hls::stream<ap_uint<ARW> >& o_base_nm_strm,
    hls::stream<bool>& o_base_e_strm,
    hls::stream<ap_uint<B_PW> >& o_overflow_pld_strm,
    hls::stream<ap_uint<PW> >& o_overflow_addr_strm,
    hls::stream<ap_uint<ARW> >& o_overflow_nm_strm,
    hls::stream<bool>& o_overflow_e_strm,
    hls::stream<ap_uint<KEYW> >& o_key_strm,
    hls::stream<ap_uint<PW> >& o_pld_strm,
    hls::stream<ap_uint<ARW> >& o_nm_strm,
    hls::stream<bool>& o_e_strm) {
#pragma HLS dataflow
    imux_read_bf<PW, B_PW, ARW>(i_base_pld_strm, i_base_addr_strm, i_base_nm_strm, i_base_e_strm, //
                                o_base_pld_strm, o_base_addr_strm, o_base_nm_strm, o_base_e_strm);
    imux_read_bf<PW, B_PW, ARW>(i_overflow_pld_strm, i_overflow_addr_strm, i_overflow_nm_strm, i_overflow_e_strm, //
                                o_overflow_pld_strm, o_overflow_addr_strm, o_overflow_nm_strm, o_overflow_e_strm);
    imux_row<KEYW, B_PW, ARW>(i_t_key_strm, i_t_pld_strm, i_t_nm_strm, i_t_e_strm, //
                              o_key_strm, o_pld_strm, o_nm_strm, o_e_strm);
}

template <int HASHWL, int KEYW, int PW, int B_PW, int ARW>
void imux_for_part(
    // input
    hls::stream<ap_uint<HASHWL> >& i_hash_strm,
    hls::stream<ap_uint<KEYW> >& i_key_strm,
    hls::stream<ap_uint<PW> >& i_pld_strm,
    hls::stream<ap_uint<PW> >& i_bf_info_strm,
    hls::stream<bool>& i_to_strm,
    hls::stream<bool>& i_e_strm,
    // output
    hls::stream<ap_uint<B_PW> >& o_base_pld_strm,
    hls::stream<ap_uint<PW> >& o_base_addr_strm,
    hls::stream<ap_uint<ARW> >& o_base_nm_strm,
    hls::stream<bool>& o_base_e_strm,
    hls::stream<ap_uint<B_PW> >& o_overflow_pld_strm,
    hls::stream<ap_uint<PW> >& o_overflow_addr_strm,
    hls::stream<ap_uint<ARW> >& o_overflow_nm_strm,
    hls::stream<bool>& o_overflow_e_strm,
    hls::stream<ap_uint<HASHWL> >& o_hash_strm,
    hls::stream<ap_uint<KEYW> >& o_key_strm,
    hls::stream<ap_uint<PW> >& o_pld_strm,
    hls::stream<ap_uint<ARW> >& o_nm_strm,
    hls::stream<bool>& o_e_strm) {
    bool last = i_e_strm.read();
    while (!last) {
#pragma HLS pipeline II = 1
        ap_uint<HASHWL> hash = i_hash_strm.read();
        ap_uint<KEYW> key = i_key_strm.read();
        ap_uint<PW> pld = i_pld_strm.read();
        ap_uint<PW> bf_info = i_bf_info_strm.read();
        bool to_overflow = i_to_strm.read();
        last = i_e_strm.read();

        if (to_overflow) {
            o_overflow_pld_strm.write(pld);
            o_overflow_addr_strm.write(bf_info);
            o_overflow_nm_strm.write(1);
            o_overflow_e_strm.write(false);
        } else {
            o_base_pld_strm.write(pld);
            o_base_addr_strm.write(bf_info);
            o_base_nm_strm.write(1);
            o_base_e_strm.write(false);
        }
        o_hash_strm.write(hash);
        o_key_strm.write(key);
        o_pld_strm.write(pld);
        ap_uint<ARW> o_nm = 0;
        o_nm[0] = to_overflow;
        o_nm_strm.write(o_nm);
        o_e_strm.write(false);
    }
    o_base_e_strm.write(true);
    o_overflow_e_strm.write(true);
    o_e_strm.write(true);
}

/// @brief mux data from uram handler and input demux and send to hbm handler
template <int HASHWL, int KEYW, int PW, int B_PW, int S_PW, int ARW>
void input_mux(hls::stream<ap_uint<8> >& i_general_cfg_strm,
               // input from input demux
               hls::stream<ap_uint<HASHWL> >& i_hash_strm,
               hls::stream<ap_uint<KEYW> >& i_key_strm,
               hls::stream<ap_uint<PW> >& i_pld_strm,
               hls::stream<ap_uint<PW> >& i_bf_info_strm,
               hls::stream<bool>& i_to_strm,
               hls::stream<bool>& i_e_strm,
               // input from uram handler
               hls::stream<ap_uint<B_PW> >& i_base_pld_strm,
               hls::stream<ap_uint<PW> >& i_base_addr_strm,
               hls::stream<ap_uint<ARW> >& i_base_nm_strm,
               hls::stream<bool>& i_base_e_strm,
               hls::stream<ap_uint<B_PW> >& i_overflow_pld_strm,
               hls::stream<ap_uint<PW> >& i_overflow_addr_strm,
               hls::stream<ap_uint<ARW> >& i_overflow_nm_strm,
               hls::stream<bool>& i_overflow_e_strm,
               hls::stream<ap_uint<KEYW> >& i_t_key_strm,
               hls::stream<ap_uint<B_PW> >& i_t_pld_strm,
               hls::stream<ap_uint<ARW> >& i_t_nm_strm,
               hls::stream<bool>& i_t_e_strm,
               // output to hbm handler
               hls::stream<ap_uint<B_PW> >& o_base_pld_strm,
               hls::stream<ap_uint<PW> >& o_base_addr_strm,
               hls::stream<ap_uint<ARW> >& o_base_nm_strm,
               hls::stream<bool>& o_base_e_strm,
               hls::stream<ap_uint<B_PW> >& o_overflow_pld_strm,
               hls::stream<ap_uint<PW> >& o_overflow_addr_strm,
               hls::stream<ap_uint<ARW> >& o_overflow_nm_strm,
               hls::stream<bool>& o_overflow_e_strm,
               hls::stream<ap_uint<HASHWL> >& o_hash_strm,
               hls::stream<ap_uint<KEYW> >& o_key_strm,
               hls::stream<ap_uint<PW> >& o_pld_strm,
               hls::stream<ap_uint<ARW> >& o_nm_strm,
               hls::stream<bool>& o_e_strm) {
    // kernel switching
    ap_uint<8> general_cfg = i_general_cfg_strm.read();
    switch (general_cfg.range(4, 0)) {
        case gqe::JoinOn:
            imux_for_join<HASHWL, KEYW, PW, B_PW, ARW>(
                i_base_addr_strm, i_base_nm_strm, i_base_e_strm, i_overflow_addr_strm, i_overflow_nm_strm,
                i_overflow_e_strm, i_t_key_strm, i_t_pld_strm, i_t_nm_strm,
                i_t_e_strm, //
                o_base_addr_strm, o_base_nm_strm, o_base_e_strm, o_overflow_addr_strm, o_overflow_nm_strm,
                o_overflow_e_strm, o_key_strm, o_pld_strm, o_nm_strm, o_e_strm);
            break;
        case gqe::BloomFilterOn:
            imux_for_bf<HASHWL, KEYW, PW, B_PW, ARW>(i_base_pld_strm, i_base_addr_strm, i_base_nm_strm, i_base_e_strm,
                                                     i_overflow_pld_strm, i_overflow_addr_strm, i_overflow_nm_strm,
                                                     i_overflow_e_strm, i_t_key_strm, i_t_pld_strm, i_t_nm_strm,
                                                     i_t_e_strm, //
                                                     o_base_pld_strm, o_base_addr_strm, o_base_nm_strm, o_base_e_strm,
                                                     o_overflow_pld_strm, o_overflow_addr_strm, o_overflow_nm_strm,
                                                     o_overflow_e_strm, o_key_strm, o_pld_strm, o_nm_strm, o_e_strm);
            break;
        case gqe::PartOn | gqe::BloomFilterOn:
            imux_for_part<HASHWL, KEYW, PW, B_PW, ARW>(
                i_hash_strm, i_key_strm, i_pld_strm, i_bf_info_strm, i_to_strm, i_e_strm, //
                o_base_pld_strm, o_base_addr_strm, o_base_nm_strm, o_base_e_strm, o_overflow_pld_strm,
                o_overflow_addr_strm, o_overflow_nm_strm, o_overflow_e_strm, o_hash_strm, o_key_strm, o_pld_strm,
                o_nm_strm, o_e_strm);
            break;
#ifndef __SYNTHESIS__
        default:
            std::cerr << "Error: illegal kernel switching combination\n";
            exit(1);
#endif
    }
}

// ************************************ Output ***************************************

template <int KEYW, int PW, int ARW>
void odmux_read_row(ap_uint<32>& join_depth,
                    // input
                    hls::stream<ap_uint<KEYW> >& i_key_strm,
                    hls::stream<ap_uint<PW> >& i_pld_strm,
                    hls::stream<ap_uint<ARW> >& i_nm_strm,
                    hls::stream<bool>& i_e_strm,
                    // output
                    hls::stream<ap_uint<KEYW> >& o_key_strm,
                    hls::stream<ap_uint<PW> >& o_pld_strm,
                    hls::stream<ap_uint<ARW> >& o_nm_strm,
                    hls::stream<bool>& o_e_strm,

                    hls::stream<ap_uint<ARW> >& o_base_nm_strm,
                    hls::stream<bool>& o_base_e_strm,
                    hls::stream<ap_uint<ARW> >& o_overflow_nm_strm,
                    hls::stream<bool>& o_overflow_e_strm) {
    ap_uint<32> depth = join_depth;
    bool last = i_e_strm.read();
    while (!last) {
#pragma HLS pipeline II = 1
        ap_uint<KEYW> key = i_key_strm.read();
        ap_uint<PW> pld = i_pld_strm.read();
        ap_uint<ARW> nm = i_nm_strm.read();
        last = i_e_strm.read();

        o_key_strm.write(key);
        o_pld_strm.write(pld);
        o_nm_strm.write(nm);
        o_e_strm.write(false);

        ap_uint<ARW> base_nm, overflow_nm;
        if (nm > depth) {
            base_nm = depth;
            overflow_nm = nm - depth;
        } else {
            base_nm = nm;
            overflow_nm = 0;
        }
        o_base_nm_strm.write(base_nm);
        o_base_e_strm.write(false);
        o_overflow_nm_strm.write(overflow_nm);
        o_overflow_e_strm.write(false);
    }
    o_e_strm.write(true);
    o_base_e_strm.write(true);
    o_overflow_e_strm.write(true);
}

template <int KEYW, int S_PW, int ARW>
void odmux_read_probe(
    // input
    hls::stream<ap_uint<ARW> >& i_nm_strm,
    hls::stream<bool>& i_e_strm,
    hls::stream<ap_uint<KEYW> >& i_key_strm,
    hls::stream<ap_uint<S_PW> >& i_pld_strm,
    // output
    hls::stream<ap_uint<KEYW> >& o_key_strm,
    hls::stream<ap_uint<S_PW> >& o_pld_strm) {
    bool last = i_e_strm.read();
    while (!last) {
        ap_uint<ARW> nm = i_nm_strm.read();
        last = i_e_strm.read();
        for (int i = 0; i < nm; i++) {
#pragma HLS pipeline II = 1
            ap_uint<KEYW> key = i_key_strm.read();
            ap_uint<S_PW> pld = i_pld_strm.read();
            o_key_strm.write(key);
            o_pld_strm.write(pld);
        }
    }
}

template <int KEYW, int PW, int S_PW, int ARW>
void odmux_for_join(ap_uint<32>& depth,
                    // input
                    hls::stream<ap_uint<KEYW> >& i_key_strm,
                    hls::stream<ap_uint<PW> >& i_pld_strm,
                    hls::stream<ap_uint<ARW> >& i_nm_strm,
                    hls::stream<bool>& i_e_strm,
                    hls::stream<ap_uint<KEYW> >& i_base_key_strm,
                    hls::stream<ap_uint<S_PW> >& i_base_pld_strm,
                    hls::stream<ap_uint<KEYW> >& i_overflow_key_strm,
                    hls::stream<ap_uint<S_PW> >& i_overflow_pld_strm,
                    // output
                    hls::stream<ap_uint<KEYW> >& o_key_strm,
                    hls::stream<ap_uint<PW> >& o_pld_strm,
                    hls::stream<ap_uint<ARW> >& o_nm_strm,
                    hls::stream<bool>& o_e_strm,
                    hls::stream<ap_uint<KEYW> >& o_base_key_strm,
                    hls::stream<ap_uint<S_PW> >& o_base_pld_strm,
                    hls::stream<ap_uint<KEYW> >& o_overflow_key_strm,
                    hls::stream<ap_uint<S_PW> >& o_overflow_pld_strm) {
#pragma HLS dataflow

    hls::stream<ap_uint<ARW> > mid_base_nm_strm;
#pragma HLS stream variable = mid_base_nm_strm depth = 8
#pragma HLS bind_storage variable = mid_base_nm_strm type = fifo impl = srl
    hls::stream<bool> mid_base_e_strm;
#pragma HLS stream variable = mid_base_e_strm depth = 8
#pragma HLS bind_storage variable = mid_base_e_strm type = fifo impl = srl
    hls::stream<ap_uint<ARW> > mid_overflow_nm_strm;
#pragma HLS stream variable = mid_overflow_nm_strm depth = 8
#pragma HLS bind_storage variable = mid_overflow_nm_strm type = fifo impl = srl
    hls::stream<bool> mid_overflow_e_strm;
#pragma HLS stream variable = mid_overflow_e_strm depth = 8
#pragma HLS bind_storage variable = mid_overflow_e_strm type = fifo impl = srl

    odmux_read_row<KEYW, PW, ARW>(depth, i_key_strm, i_pld_strm, i_nm_strm, i_e_strm, //
                                  o_key_strm, o_pld_strm, o_nm_strm, o_e_strm, mid_base_nm_strm, mid_base_e_strm,
                                  mid_overflow_nm_strm, mid_overflow_e_strm);
    odmux_read_probe<KEYW, S_PW, ARW>(mid_base_nm_strm, mid_base_e_strm, i_base_key_strm, i_base_pld_strm, //
                                      o_base_key_strm, o_base_pld_strm);
    odmux_read_probe<KEYW, S_PW, ARW>(mid_overflow_nm_strm, mid_overflow_e_strm, i_overflow_key_strm,
                                      i_overflow_pld_strm, //
                                      o_overflow_key_strm, o_overflow_pld_strm);
}

template <int KEYW, int PW, int S_PW, int ARW>
void odmux_for_bf(
    // input
    hls::stream<ap_uint<KEYW> >& i_key_strm,
    hls::stream<ap_uint<PW> >& i_pld_strm,
    hls::stream<ap_uint<ARW> >& i_nm_strm,
    hls::stream<bool>& i_e_strm,
    hls::stream<ap_uint<KEYW> >& i_base_key_strm,
    hls::stream<ap_uint<S_PW> >& i_base_pld_strm,
    hls::stream<ap_uint<KEYW> >& i_overflow_key_strm,
    hls::stream<ap_uint<S_PW> >& i_overflow_pld_strm,
    // output
    hls::stream<ap_uint<KEYW> >& o_key_strm,
    hls::stream<ap_uint<PW> >& o_pld_strm,
    hls::stream<ap_uint<ARW> >& o_nm_strm,
    hls::stream<bool>& o_e_strm,
    hls::stream<ap_uint<KEYW> >& o_base_key_strm,
    hls::stream<ap_uint<S_PW> >& o_base_pld_strm,
    hls::stream<ap_uint<KEYW> >& o_overflow_key_strm,
    hls::stream<ap_uint<S_PW> >& o_overflow_pld_strm) {
    bool last = i_e_strm.read();
    while (!last) {
#pragma HLS pipeline II = 1
        ap_uint<KEYW> key = i_key_strm.read();
        ap_uint<PW> pld = i_pld_strm.read();
        ap_uint<ARW> nm = i_nm_strm.read();
        last = i_e_strm.read();

        o_key_strm.write(key);
        o_pld_strm.write(pld);
        o_nm_strm.write(nm);
        o_e_strm.write(false);
        if (nm == 1) {
            ap_uint<KEYW> overflow_key = i_overflow_key_strm.read();
            ap_uint<S_PW> overflow_pld = i_overflow_pld_strm.read();
            o_overflow_key_strm.write(overflow_key);
            o_overflow_pld_strm.write(overflow_pld);
        } else if (nm == 2) {
            ap_uint<KEYW> base_key = i_base_key_strm.read();
            ap_uint<S_PW> base_pld = i_base_pld_strm.read();
            o_base_key_strm.write(base_key);
            o_base_pld_strm.write(base_pld);
        }
    }
    o_e_strm.write(true);
}

template <int HASHWL, int KEYW, int PW>
void odmux_for_part(
    // input
    hls::stream<ap_uint<HASHWL> >& i_hash_strm,
    hls::stream<ap_uint<KEYW> >& i_key_strm,
    hls::stream<ap_uint<PW> >& i_pld_strm,
    hls::stream<bool>& i_e_strm,

    // output
    hls::stream<ap_uint<HASHWL> >& o_hash_strm,
    hls::stream<ap_uint<KEYW> >& o_key_strm,
    hls::stream<ap_uint<PW> >& o_pld_strm,
    hls::stream<bool>& o_e_strm) {
    bool last = i_e_strm.read();
    while (!last) {
#pragma HLS pipeline II = 1
        ap_uint<HASHWL> hash = i_hash_strm.read();
        ap_uint<KEYW> key = i_key_strm.read();
        ap_uint<PW> pld = i_pld_strm.read();
        last = i_e_strm.read();

        o_hash_strm.write(hash);
        o_key_strm.write(key);
        o_pld_strm.write(pld);
        o_e_strm.write(false);
    }
    o_e_strm.write(true);
}

/// @brief demux data from hbm handler to hbm handler or output mux
template <int HASHWL, int KEYW, int PW, int B_PW, int S_PW, int ARW>
void output_demux(hls::stream<ap_uint<8> >& i_general_cfg_strm,
                  ap_uint<32>& depth,
                  // input from hbm handler
                  hls::stream<ap_uint<HASHWL> >& i_hash_strm,
                  hls::stream<ap_uint<KEYW> >& i_key_strm,
                  hls::stream<ap_uint<PW> >& i_pld_strm,
                  hls::stream<ap_uint<ARW> >& i_nm_strm,
                  hls::stream<bool>& i_e_strm,
                  hls::stream<ap_uint<KEYW> >& i_base_key_strm,
                  hls::stream<ap_uint<S_PW> >& i_base_pld_strm,
                  hls::stream<ap_uint<KEYW> >& i_overflow_key_strm,
                  hls::stream<ap_uint<S_PW> >& i_overflow_pld_strm,

                  // output to uram handler or output demux
                  hls::stream<ap_uint<HASHWL> >& o_hash_strm,
                  hls::stream<ap_uint<KEYW> > o_key_strms[2],
                  hls::stream<ap_uint<PW> > o_pld_strms[2],
                  hls::stream<ap_uint<ARW> >& o_nm_strm,
                  hls::stream<bool> o_e_strms[2],
                  hls::stream<ap_uint<KEYW> >& o_base_key_strm,
                  hls::stream<ap_uint<S_PW> >& o_base_pld_strm,
                  hls::stream<ap_uint<KEYW> >& o_overflow_key_strm,
                  hls::stream<ap_uint<S_PW> >& o_overflow_pld_strm) {
    // kernel switching
    ap_uint<8> general_cfg = i_general_cfg_strm.read();
    switch (general_cfg.range(4, 0)) {
        case gqe::JoinOn:
            odmux_for_join<KEYW, PW, S_PW, ARW>(depth, i_key_strm, i_pld_strm, i_nm_strm, i_e_strm, i_base_key_strm,
                                                i_base_pld_strm, i_overflow_key_strm, i_overflow_pld_strm, //
                                                o_key_strms[0], o_pld_strms[0], o_nm_strm, o_e_strms[0],
                                                o_base_key_strm, o_base_pld_strm, o_overflow_key_strm,
                                                o_overflow_pld_strm);
            break;
        case gqe::BloomFilterOn:
            odmux_for_bf<KEYW, PW, S_PW, ARW>(i_key_strm, i_pld_strm, i_nm_strm, i_e_strm, i_base_key_strm,
                                              i_base_pld_strm, i_overflow_key_strm, i_overflow_pld_strm, //
                                              o_key_strms[0], o_pld_strms[0], o_nm_strm, o_e_strms[0], o_base_key_strm,
                                              o_base_pld_strm, o_overflow_key_strm, o_overflow_pld_strm);
            break;
        case gqe::PartOn | gqe::BloomFilterOn:
            odmux_for_part<HASHWL, KEYW, PW>(i_hash_strm, i_key_strm, i_pld_strm, i_e_strm, //
                                             o_hash_strm, o_key_strms[1], o_pld_strms[1], o_e_strms[1]);
            break;
#ifndef __SYNTHESIS__
        default:
            std::cerr << "Error: illegal kernel switching combination\n";
            exit(1);
#endif
    }
}

///@ brief split key/pld to individual key and pld as well as combine partition bucket number and number of elements
/// together to output nm stream to reuse the same data path with join/bf
template <int HASHWL, int KEYW, int PW, int ARW>
void omux_for_part(
    // input
    hls::stream<ap_uint<KEYW + PW> >& i_kpld_strm,
    hls::stream<ap_uint<10> >& i_nm_strm,
    hls::stream<ap_uint<10> >& i_bk_strm,
    // output
    hls::stream<ap_uint<KEYW> >& o_key_strm,
    hls::stream<ap_uint<PW> >& o_pld_strm,
    hls::stream<ap_uint<ARW> >& o_nm_strm) {
    ap_uint<KEYW> key;
    ap_uint<PW> pld;
    ap_uint<10> nm = i_nm_strm.read();
    while (nm > 0) {
        ap_uint<10> bk = i_bk_strm.read();
        ap_uint<ARW> o_nm = 0;
        o_nm.range(9, 0) = nm;
        o_nm.range(19, 10) = bk;
        o_nm_strm.write(o_nm);

        for (int i = 0; i < nm; i++) {
#pragma HLS pipeline II = 1
            (pld, key) = i_kpld_strm.read();
            o_key_strm.write(key);
            o_pld_strm.write(pld);
        }
        nm = i_nm_strm.read();
    }
    o_nm_strm.write(0);
}

/// @brief mux data to output from uram handler or output demux
template <int HASHWL, int KEYW, int PW, int B_PW, int S_PW, int ARW>
void output_mux(hls::stream<ap_uint<8> >& i_general_cfg_strm,
                ap_uint<32>& depth,
                // input from output demux
                hls::stream<ap_uint<KEYW> >& i_key_strm,
                hls::stream<ap_uint<PW> >& i_pld_strm,
                hls::stream<ap_uint<ARW> >& i_nm_strm,
                hls::stream<bool>& i_e_strm,
                hls::stream<ap_uint<KEYW> >& i_base_key_strm,
                hls::stream<ap_uint<S_PW> >& i_base_pld_strm,
                hls::stream<ap_uint<KEYW> >& i_overflow_key_strm,
                hls::stream<ap_uint<S_PW> >& i_overflow_pld_strm,
                // input from uram handler
                hls::stream<ap_uint<KEYW + PW> >& i_hp_kpld_strm,
                hls::stream<ap_uint<10> >& i_hp_nm_strm,
                hls::stream<ap_uint<10> >& i_hp_bk_strm,
                // output
                hls::stream<ap_uint<KEYW> >& o_key_strm,
                hls::stream<ap_uint<PW> >& o_pld_strm,
                hls::stream<ap_uint<ARW> >& o_nm_strm,
                hls::stream<bool>& o_e_strm,
                hls::stream<ap_uint<KEYW> >& o_base_key_strm,
                hls::stream<ap_uint<S_PW> >& o_base_pld_strm,
                hls::stream<ap_uint<KEYW> >& o_overflow_key_strm,
                hls::stream<ap_uint<S_PW> >& o_overflow_pld_strm) {
    // kernel switching
    ap_uint<8> general_cfg = i_general_cfg_strm.read();
    switch (general_cfg.range(4, 0)) {
        case gqe::JoinOn:
            odmux_for_join<KEYW, PW, S_PW, ARW>(depth, i_key_strm, i_pld_strm, i_nm_strm, i_e_strm, i_base_key_strm,
                                                i_base_pld_strm, i_overflow_key_strm, i_overflow_pld_strm, //
                                                o_key_strm, o_pld_strm, o_nm_strm, o_e_strm, o_base_key_strm,
                                                o_base_pld_strm, o_overflow_key_strm, o_overflow_pld_strm);
            break;
        case gqe::BloomFilterOn:
            odmux_for_bf<KEYW, PW, S_PW, ARW>(i_key_strm, i_pld_strm, i_nm_strm, i_e_strm, i_base_key_strm,
                                              i_base_pld_strm, i_overflow_key_strm, i_overflow_pld_strm, //
                                              o_key_strm, o_pld_strm, o_nm_strm, o_e_strm, o_base_key_strm,
                                              o_base_pld_strm, o_overflow_key_strm, o_overflow_pld_strm);
            break;
        case gqe::PartOn | gqe::BloomFilterOn:
            omux_for_part<HASHWL, KEYW, PW, ARW>(i_hp_kpld_strm, i_hp_nm_strm, i_hp_bk_strm, //
                                                 o_key_strm, o_pld_strm, o_nm_strm);
            break;
#ifndef __SYNTHESIS__
        default:
            std::cerr << "Error: illegal kernel switching combination\n";
            exit(1);
#endif
    }
}

} // namespace details
} // namespace database
} // namespace xf

#endif // GQE_ISV_DEMUX_AND_MUX_HPP
