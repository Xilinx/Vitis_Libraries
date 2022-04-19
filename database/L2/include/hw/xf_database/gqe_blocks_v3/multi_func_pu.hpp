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
 * @file multi_func_pu.hpp
 * @brief Multi-functional-PU implementation for wrappering the BloomFilter/Join/Part into one
 *
 * This file is part of Vitis Database Library.
 */

#ifndef GQE_ISV_MULTI_FUNC_PU_HPP
#define GQE_ISV_MULTI_FUNC_PU_HPP

#ifndef __cplusplus
#error "Vitis Database Library only works with C++."
#endif

#include <hls_stream.h>
#include <ap_int.h>
#include <hls_burst_maxi.h>

#include "xf_database/hash_multi_join_build_probe.hpp"
#include "xf_database/gqe_blocks_v3/crossbar.hpp"
#include "xf_database/gqe_blocks_v3/build_flow.hpp"
#include "xf_database/gqe_blocks_v3/probe_flow.hpp"
#include "xf_database/gqe_blocks_v3/collect.hpp"

#include "xf_utils_hw/stream_split.hpp"

#include "xf_database/gqe_blocks_v3/gqe_types.hpp"
#include "xf_database/gqe_blocks_v3/gqe_traits.hpp"
#include "xf_database/gqe_blocks_v3/gqe_enums.hpp"
#include "xf_database/gqe_blocks_v3/uram_cache.hpp"

#ifndef __SYNTHESIS__
#include <iostream>
//#define DEBUG_MISS true
//#define USER_DEBUG true
#endif

// FIXME For debug
#ifndef __SYNTHESIS__
#include <iostream>
//#define DEBUG_BLOOMFILTER_JOIN
#endif

namespace xf {
namespace database {
namespace details {

// ---------------------------- Processing Core -----------------------------------

/// @brief initiate uram to zero or read previous stored hash counter.
/// add join_on trigger to avoid out of boundary memory access on HBM
template <int HASHW, int ARW, int NUM_X, int NUM_Y>
void initiate_uram(
    // input
    bool join_on,
    bool id,
    hls::burst_maxi<ap_uint<256> >& htb_buf,
    ap_uint<72> (*uram_buffer0)[NUM_X][NUM_Y][4096],
    ap_uint<72> (*uram_buffer1)[NUM_X][NUM_Y][4096]) {
#pragma HLS INLINE

    const int HASH_DEPTH = (1 << HASHW) / 3 + 1;
    UramCache<72, HASH_DEPTH, 8, NUM_X, NUM_Y> uram_inst0(uram_buffer0);
    UramCache<72, HASH_DEPTH, 8, NUM_X, NUM_Y> uram_inst1(uram_buffer1);

    if (id == 0) {
    // id==0, the first time to build, initialize uram to zero
    INIT_LOOP:
        for (int i = 0; i < HASH_DEPTH; i++) {
#pragma HLS PIPELINE II = 1
#pragma HLS dependence variable = uram_inst0.blocks inter false
#pragma HLS dependence variable = uram_inst1.blocks inter false

            uram_inst0.write(i, 0);
            uram_inst1.write(i, 0);
        }
    }
#ifndef __SYNTHESIS__
    else {
        if (join_on) {
            for (int i = 0; i < HASH_DEPTH; i++) {
                ap_uint<256> tmp_256;

                htb_buf.read_request(8000000 + i, 1);
                tmp_256 = htb_buf.read();
                uram_inst0.write(i, tmp_256.range(71, 0));

                htb_buf.read_request(8000000 + i + HASH_DEPTH, 1);
                tmp_256 = htb_buf.read();
                uram_inst1.write(i, tmp_256.range(71, 0));
            }
        }
    }
#endif
}

/// @brief Top function of hash multi join PU
template <int HASH_MODE, int HASHWH, int HASHWL, int HASHWJ, int KEYW, int S_PW, int T_PW, int ARW, int HBM_OUTSTANDING>
void build_probe_core_wrapper(hls::stream<ap_uint<8> >& i_general_cfg_strm,
                              hls::stream<ap_uint<15> >& i_part_cfg_strm,
                              hls::stream<ap_uint<36> >& i_bf_cfg_strm,
                              // input status
                              ap_uint<32>& depth,

                              // input table
                              hls::stream<ap_uint<HASHWL> >& i_hash_strm,
                              hls::stream<ap_uint<KEYW> >& i_key_strm,
                              hls::stream<ap_uint<T_PW> >& i_pld_strm,
                              hls::stream<bool>& i_e_strm,

                              // output for join
                              hls::stream<ap_uint<KEYW> >& o_t_key_strm,
                              hls::stream<ap_uint<T_PW> >& o_t_pld_strm,
                              hls::stream<ap_uint<ARW> >& o_nm_strm,
                              hls::stream<bool>& o_e_strm,

                              hls::stream<ap_uint<KEYW> >& o_base_s_key_strm,
                              hls::stream<ap_uint<S_PW> >& o_base_s_pld_strm,
                              hls::stream<ap_uint<KEYW> >& o_overflow_s_key_strm,
                              hls::stream<ap_uint<S_PW> >& o_overflow_s_pld_strm,

                              hls::burst_maxi<ap_uint<256> >& htb_buf,
                              hls::burst_maxi<ap_uint<256> >& stb_buf) {
#pragma HLS INLINE off

    const int PW = (S_PW > T_PW) ? S_PW : T_PW;
    // alllocate shared URAM storage
    const int HASH_DEPTH = (1 << HASHWJ) / 3 + 1;
    const int elem_per_line = 72 / 72;
    const int uram_num_x = (HASH_DEPTH + (elem_per_line * 4096) - 1) / (elem_per_line * 4096);
    const int uram_num_y = (72 + 71) / 72;
#ifndef __SYNTHESIS__
    std::unique_ptr<ap_uint<72>[]> buffer0_smart_ptr(new ap_uint<72>[ uram_num_x * uram_num_y * 4096 ]);
    std::unique_ptr<ap_uint<72>[]> buffer1_smart_ptr(new ap_uint<72>[ uram_num_x * uram_num_y * 4096 ]);
    std::unique_ptr<ap_uint<72>[]> buffer2_smart_ptr(new ap_uint<72>[ uram_num_x * uram_num_y * 4096 ]);
    ap_uint<72>* buffer0_raw_ptr = buffer0_smart_ptr.get();
    ap_uint<72>* buffer1_raw_ptr = buffer1_smart_ptr.get();
    ap_uint<72>* buffer2_raw_ptr = buffer2_smart_ptr.get();
    ap_uint<72>(&uram_buffer0)[uram_num_x][uram_num_y][4096] =
        *reinterpret_cast<ap_uint<72>(*)[uram_num_x][uram_num_y][4096]>(buffer0_raw_ptr);
    ap_uint<72>(&uram_buffer1)[uram_num_x][uram_num_y][4096] =
        *reinterpret_cast<ap_uint<72>(*)[uram_num_x][uram_num_y][4096]>(buffer1_raw_ptr);
    ap_uint<72>(&uram_buffer2)[uram_num_x][uram_num_y][4096] =
        *reinterpret_cast<ap_uint<72>(*)[uram_num_x][uram_num_y][4096]>(buffer2_raw_ptr);
#else
    ap_uint<72> uram_buffer0[uram_num_x][uram_num_y][4096];
#pragma HLS bind_storage variable = uram_buffer0 type = ram_2p impl = uram
#pragma HLS array_partition variable = uram_buffer0 complete dim = 1
#pragma HLS array_partition variable = uram_buffer0 complete dim = 2
    ap_uint<72> uram_buffer1[uram_num_x][uram_num_y][4096];
#pragma HLS bind_storage variable = uram_buffer1 type = ram_2p impl = uram
#pragma HLS array_partition variable = uram_buffer1 complete dim = 1
#pragma HLS array_partition variable = uram_buffer1 complete dim = 2
    ap_uint<72> uram_buffer2[uram_num_x][uram_num_y][4096];
#pragma HLS bind_storage variable = uram_buffer2 type = ram_2p impl = uram
#pragma HLS array_partition variable = uram_buffer2 complete dim = 1
#pragma HLS array_partition variable = uram_buffer2 complete dim = 2
#endif

    ap_uint<8> general_cfg = i_general_cfg_strm.read();
    bool join_on = general_cfg[1];
    bool build_probe_flag = general_cfg[5];
    ap_uint<36> bf_cfg = i_bf_cfg_strm.read();
    ap_uint<15> part_cfg = i_part_cfg_strm.read();

    hls::stream<ap_uint<8> > general_cfg_strm;
#pragma HLS stream variable = general_cfg_strm depth = 2
    hls::stream<ap_uint<15> > part_cfg_strm;
#pragma HLS stream variable = part_cfg_strm depth = 2
    hls::stream<ap_uint<36> > bf_cfg_strm;
#pragma HLS stream variable = bf_cfg_strm depth = 2

    ap_uint<32> overflow_length = 0;

    // initilize URAM by previous hash build or probe
    initiate_uram<HASHWJ, ARW, uram_num_x, uram_num_y>(join_on, build_probe_flag, htb_buf, &uram_buffer0,
                                                       &uram_buffer1);

    if (!build_probe_flag) {
#ifndef __SYNTHESIS__
        std::cout << "----------------------build base------------------------" << std::endl;
#endif

        // build the base region, dump the overflow rows to overflow region
        build_wrapper<HASHWL, HASHWJ, KEYW, PW, S_PW, ARW, HBM_OUTSTANDING, uram_num_x, uram_num_y>(
            // input status
            depth, overflow_length,
            // input s-table
            i_hash_strm, i_key_strm, i_pld_strm, i_e_strm,
            // HBM/DDR
            stb_buf, &uram_buffer0, &uram_buffer1);

        if (overflow_length > 0) {
#ifndef __SYNTHESIS__
            std::cout << "----------------------build overflow------------------------" << std::endl;
#endif

            // build the overflow region
            merge_wrapper<HASH_MODE, HASHWH, HASHWJ, KEYW, S_PW, ARW, HBM_OUTSTANDING, uram_num_x, uram_num_y>(
                // input status
                depth, overflow_length,

                htb_buf, stb_buf, &uram_buffer1);
        }

#ifndef __SYNTHESIS__
        for (int i = 0; i < HASH_DEPTH; i++) {
            htb_buf.write_request(8000000 + i, 1);
            htb_buf.write((ap_uint<256>)uram_buffer0[0][i / 4096][i % 4096]);
            htb_buf.write_response();

            htb_buf.write_request(8000000 + i + HASH_DEPTH, 1);
            htb_buf.write((ap_uint<256>)uram_buffer1[0][i / 4096][i % 4096]);
            htb_buf.write_response();
        }
#endif
    } else {
#ifndef __SYNTHESIS__
        std::cout << "-----------------------probe------------------------" << std::endl;
#endif
        general_cfg_strm.write(general_cfg);
        part_cfg_strm.write(part_cfg);
        bf_cfg_strm.write(bf_cfg);

        // probe base + overflow
        multi_probe_wrapper<HASHWH, HASHWL, HASHWJ, KEYW, PW, S_PW, T_PW, ARW, HBM_OUTSTANDING, uram_num_x, uram_num_y>(
            // input configs
            general_cfg_strm, part_cfg_strm, bf_cfg_strm,
            // depth of hash-table
            depth,

            // input large table
            i_hash_strm, i_key_strm, i_pld_strm, i_e_strm,

            // output for join
            o_t_key_strm, o_t_pld_strm, o_nm_strm, o_e_strm, o_base_s_key_strm, o_base_s_pld_strm,
            o_overflow_s_key_strm, o_overflow_s_pld_strm,

            htb_buf, stb_buf, &uram_buffer0, &uram_buffer1, &uram_buffer2);
    }

#ifndef __SYNTHESIS__
// for (int i = 0; i < uram_num_x; i++) {
//    for (int j = 0; j < uram_num_y; j++) {
//        delete[] uram_buffer0[i][j];
//        delete[] uram_buffer1[i][j];
//        delete[] uram_buffer2[i][j];
//    }
//    delete[] uram_buffer0[i];
//    delete[] uram_buffer1[i];
//    delete[] uram_buffer2[i];
//}
// delete[] uram_buffer0;
// delete[] uram_buffer1;
// delete[] uram_buffer2;
#endif
}

/// @brief hash hit branch of t_strm
template <int KEYW, int S_PW, int T_PW, int ARW, int GRP_SZ>
void join_unit_1(bool& build_probe_flag,
                 ap_uint<32>& join_depth,
                 hls::stream<ap_uint<8> >& general_cfg_strm,
                 hls::stream<ap_uint<3> >& join_flag_strm_o,
                 // input large table
                 hls::stream<ap_uint<KEYW> >& i1_t_key_strm,
                 hls::stream<ap_uint<T_PW> >& i1_t_pld_strm,
                 hls::stream<ap_uint<ARW> >& i1_nm_strm,
                 hls::stream<bool>& i1_e0_strm,

                 // input small table
                 hls::stream<ap_uint<KEYW> >& i_base_s_key_strm,
                 hls::stream<ap_uint<S_PW> >& i_base_s_pld_strm,
                 hls::stream<ap_uint<KEYW> >& i_overflow_s_key_strm,
                 hls::stream<ap_uint<S_PW> >& i_overflow_s_pld_strm,

                 // output join result
                 hls::stream<ap_uint<KEYW + S_PW + T_PW> >& o_j_strm,
                 hls::stream<bool>& o_e_strm,
                 hls::stream<ap_uint<20> >& o_nm_bk_strm) {
#pragma HLS INLINE off
    if (build_probe_flag) {
        ap_uint<KEYW> s1_key;
        ap_uint<S_PW> s1_pld;
        ap_uint<KEYW> t1_key;
        ap_uint<T_PW> t1_pld;
        ap_uint<KEYW + S_PW + T_PW> j = 0;
        ap_uint<32> depth = join_depth;

        ap_uint<8> general_cfg = general_cfg_strm.read();
        bool bf_bp_flag = general_cfg[7];
        ap_uint<3> join_flag_t = join_flag_strm_o.read();
        int join_flag_i = join_flag_t;
        xf::database::enums::JoinType join_flag = static_cast<xf::database::enums::JoinType>(join_flag_i);

        bool t1_last;
        ap_uint<ARW> bk_nm;
        ap_uint<10> nm;
        ap_uint<10> bk;
        switch (general_cfg.range(4, 0)) {
            case gqe::JoinOn:
                t1_last = i1_e0_strm.read();
            JOIN_LOOP_1:
                while (!t1_last) {
                    t1_key = i1_t_key_strm.read();
                    t1_pld = i1_t_pld_strm.read();
                    ap_uint<ARW> nm_1 = i1_nm_strm.read();
                    t1_last = i1_e0_strm.read();
                    bool flag = 0;
                    ap_uint<ARW> base1_nm, overflow1_nm;
                    if (nm_1 > depth) {
                        base1_nm = depth;
                        overflow1_nm = nm_1 - depth;
                    } else {
                        base1_nm = nm_1;
                        overflow1_nm = 0;
                    }
                    j(KEYW + S_PW + T_PW - 1, S_PW + T_PW) = t1_key;
                    if (T_PW > 0) j(T_PW - 1, 0) = t1_pld;
                JOIN_COMPARE_LOOP:
                    while (base1_nm > 0 || overflow1_nm > 0) {
#pragma HLS PIPELINE II = 1

                        if (base1_nm > 0) {
                            s1_key = i_base_s_key_strm.read();
                            s1_pld = i_base_s_pld_strm.read();
                            base1_nm--;
                        } else if (overflow1_nm > 0) {
                            s1_key = i_overflow_s_key_strm.read();
                            s1_pld = i_overflow_s_pld_strm.read();
                            overflow1_nm--;
                        }

                        if (S_PW > 0) j(S_PW + T_PW - 1, T_PW) = s1_pld;

                        if (join_flag == xf::database::enums::JT_INNER && s1_key == t1_key) {
                            o_j_strm.write(j);
                            o_e_strm.write(false);
                        }

                        flag = flag ||
                               (join_flag == 3 && s1_key == t1_key && s1_pld.range(31, 0) != t1_pld.range(31, 0)) ||
                               (join_flag != 3 && s1_key == t1_key);
                    }

                    if (join_flag == xf::database::enums::JT_ANTI && !flag) {
                        o_j_strm.write(j);
                        o_e_strm.write(false);
                    } else if ((join_flag == xf::database::enums::JT_SEMI || join_flag == 3) && flag) {
                        o_j_strm.write(j);
                        o_e_strm.write(false);
                    }
                }
                o_j_strm.write(0);
                o_e_strm.write(true);
                break;
            case gqe::BloomFilterOn:
                t1_last = i1_e0_strm.read();
                // dot hash
                while (!t1_last) {
#pragma HLS PIPELINE II = 1
                    t1_key = i1_t_key_strm.read();
                    t1_pld = i1_t_pld_strm.read();
                    ap_uint<ARW> nm = i1_nm_strm.read();
                    t1_last = i1_e0_strm.read();

                    ap_uint<KEYW> drop;
                    ap_uint<S_PW> pld;
                    // from overflow
                    if (nm == 1 && bf_bp_flag) {
                        drop = i_overflow_s_key_strm.read();
                        pld = i_overflow_s_pld_strm.read();
                        // from base
                    } else if (nm == 2 && bf_bp_flag) {
                        drop = i_base_s_key_strm.read();
                        pld = i_base_s_pld_strm.read();
                    }

                    // receive all Fs separator, send out the hash table
                    // as well as clean up the hash table for next group
                    if (t1_pld == ~ap_uint<T_PW>(0) && bf_bp_flag) {
                        o_j_strm.write(j);
                        o_e_strm.write(false);
                        j = 0;
                    } else {
                        bool is_in = pld[0];
                        if (is_in) {
                            ap_uint<T_PW> row_id = t1_pld - 1;
                            j[row_id % GRP_SZ] = 1;
                        }
                    }
                }
                o_j_strm.write(0);
                o_e_strm.write(true);
                break;
            case gqe::PartOn | gqe::BloomFilterOn:
                bk_nm = i1_nm_strm.read();
                nm = bk_nm.range(9, 0);
                while (nm > 0) {
                    o_nm_bk_strm.write(bk_nm.range(19, 0));
                    for (int i = 0; i < nm; i++) {
#pragma HLS pipeline II = 1
                        t1_key = i1_t_key_strm.read();
                        t1_pld = i1_t_pld_strm.read();
                        j.range(KEYW - 1, 0) = t1_key;
                        j.range(KEYW + T_PW - 1, KEYW) = t1_pld;
                        o_j_strm.write(j);
                    }
                    bk_nm = i1_nm_strm.read();
                    nm = bk_nm.range(9, 0);
                }
                o_nm_bk_strm.write(bk_nm.range(19, 0));
                break;
#ifndef __SYNTHESIS__
            default:
                std::cerr << "Error: illegal kernel switching combination\n";
                exit(1);
#endif
        }
    }
}

/// @brief parse config
void parse_cfg(hls::stream<ap_uint<8> >& i_general_cfg_strm,
               hls::stream<ap_uint<8> > o_general_cfg_strms[3],
               bool& build_probe_flag) {
    ap_uint<8> general_cfg = i_general_cfg_strm.read();
    build_probe_flag = general_cfg[5];
    if (build_probe_flag) {
        o_general_cfg_strms[0].write(general_cfg);
        o_general_cfg_strms[1].write(general_cfg);
        o_general_cfg_strms[2].write(general_cfg);
    }
}

/// @brief split t_strm to hash hit branch and hash unhit branch for join
/// pass t_strm to hash hit branch for bf (end flag only) and part (nm only)
template <int KEYW, int S_PW, int T_PW, int ARW>
void split_stream(bool& build_probe_flag,
                  // input
                  hls::stream<ap_uint<8> >& i_general_cfg_strm,
                  hls::stream<ap_uint<3> >& join_flag_strm_o,
                  hls::stream<ap_uint<KEYW> >& i_t_key_strm,
                  hls::stream<ap_uint<T_PW> >& i_t_pld_strm,
                  hls::stream<ap_uint<ARW> >& i_nm_strm,
                  hls::stream<bool>& i_e0_strm,

                  // output
                  hls::stream<ap_uint<3> >& join1_flag_strm,
                  hls::stream<ap_uint<KEYW> >& i1_t_key_strm,
                  hls::stream<ap_uint<T_PW> >& i1_t_pld_strm,
                  hls::stream<ap_uint<ARW> >& i1_nm_strm,
                  hls::stream<bool>& i1_e0_strm,

                  hls::stream<ap_uint<3> >& join2_flag_strm,
                  hls::stream<ap_uint<KEYW> >& i2_t_key_strm,
                  hls::stream<ap_uint<T_PW> >& i2_t_pld_strm,
                  hls::stream<ap_uint<ARW> >& i2_nm_strm,
                  hls::stream<bool>& i2_e0_strm) {
#pragma HLS INLINE off

    if (build_probe_flag) {
        ap_uint<8> general_cfg = i_general_cfg_strm.read();
        ap_uint<KEYW> t_key;
        ap_uint<T_PW> t_pld;
        ap_uint<3> join_flag = join_flag_strm_o.read();

        join1_flag_strm.write(join_flag);
        join2_flag_strm.write(join_flag);

        bool t_last;
        ap_uint<ARW> nm_bk;
        ap_uint<10> nm;
        switch (general_cfg.range(4, 0)) {
            case gqe::JoinOn:
            case gqe::BloomFilterOn:
                t_last = i_e0_strm.read();
            DIVIDE_LOOP:

                while (!t_last) {
#pragma HLS PIPELINE II = 1
                    t_key = i_t_key_strm.read();
                    t_pld = i_t_pld_strm.read();
                    ap_uint<ARW> nm = i_nm_strm.read();
                    t_last = i_e0_strm.read();
                    if (nm > 0) {
                        i1_t_key_strm.write(t_key);
                        i1_t_pld_strm.write(t_pld);
                        i1_nm_strm.write(nm);
                        i1_e0_strm.write(false);

                    } else if (nm == 0) {
                        i2_t_key_strm.write(t_key);
                        i2_t_pld_strm.write(t_pld);
                        i2_nm_strm.write(nm);
                        i2_e0_strm.write(false);
                    }
                }
                i1_e0_strm.write(true);
                i2_e0_strm.write(true);
                break;
            case gqe::PartOn | gqe::BloomFilterOn:
                // only unit 1 path used for PART
                i2_e0_strm.write(true);
                nm_bk = i_nm_strm.read();
                nm = nm_bk.range(9, 0);
                while (nm > 0) {
                    i1_nm_strm.write(nm_bk);
                    for (int i = 0; i < nm; i++) {
#pragma HLS pipeline II = 1
                        t_key = i_t_key_strm.read();
                        t_pld = i_t_pld_strm.read();
                        i1_t_key_strm.write(t_key);
                        i1_t_pld_strm.write(t_pld);
                    }
                    nm_bk = i_nm_strm.read();
                    nm = nm_bk.range(9, 0);
                }
                i1_nm_strm.write(nm_bk);
                break;
#ifndef __SYNTHESIS__
            default:
                std::cerr << "Error: illegal kernel switching combination\n";
                exit(1);
#endif
        }
    }
}
/// @brief combine hash hit and unhit branches as well as transform the end flag stream to nm stream
/// transparent to PART flow
template <int KEYW, int S_PW, int T_PW, int ARW>
void combine_stream(bool& build_probe_flag,
                    // input
                    hls::stream<ap_uint<8> >& i_general_cfg_strm,
                    hls::stream<ap_uint<KEYW + S_PW + T_PW> > i_j_strm[2],
                    hls::stream<bool> i_e_strm[2],
                    hls::stream<ap_uint<20> >& i_nm_bk_strm,

                    // output join result
                    hls::stream<ap_uint<KEYW + S_PW + T_PW> >& o_j_strm,
                    hls::stream<ap_uint<10> >& o_nm_strm,
                    hls::stream<ap_uint<10> >& o_bk_strm) {
#pragma HLS INLINE off
    if (build_probe_flag) {
        ap_uint<8> general_cfg = i_general_cfg_strm.read();
        ap_uint<2> last = 0;
        ap_uint<2> empty_e = 0;
        ap_uint<2> rd_e = 0;
        ap_uint<KEYW + S_PW + T_PW> j_arr[2];
#pragma HLS array_partition variable = j_arr dim = 1
        ap_uint<20> nm_bk;
        ap_uint<10> nm, bk;

        switch (general_cfg.range(4, 0)) {
            case gqe::JoinOn:
            case gqe::BloomFilterOn:
                do {
#pragma HLS pipeline II = 1
                    for (int i = 0; i < 2; i++) {
#pragma HLS unroll
                        empty_e[i] = !i_e_strm[i].empty() && !last[i];
                    }

                    for (int i = 0; i < 2; i++) {
#pragma HLS unroll
                        ap_uint<2> t_e = 0;
                        if (i > 0) t_e = empty_e(i - 1, 0);
                        rd_e[i] = t_e > 0 ? (bool)0 : (bool)empty_e[i];
                    }

                    for (int i = 0; i < 2; i++) {
#pragma HLS unroll
                        if (rd_e[i]) {
                            j_arr[i] = i_j_strm[i].read();
                            last[i] = i_e_strm[i].read();
                        }
                    }

                    ap_uint<3> id = join_v2::mux<2>(rd_e);
                    ap_uint<KEYW + S_PW + T_PW> j = j_arr[id];
                    bool valid_n = last[id];

                    if (!valid_n && rd_e != 0) {
                        o_j_strm.write(j);
                        o_nm_strm.write(1);
                    }
                } while (last != 3);
                o_nm_strm.write(0);
                break;
            case gqe::PartOn | gqe::BloomFilterOn:
                // drop invalid end flag from join unit 2
                last[1] = i_e_strm[1].read();
                j_arr[1] = i_j_strm[1].read();
                // bypassing PART result
                nm_bk = i_nm_bk_strm.read();
                nm = nm_bk.range(9, 0);
                bk = nm_bk.range(19, 10);
                while (nm > 0) {
                    o_nm_strm.write(nm);
                    o_bk_strm.write(bk);
                    for (int i = 0; i < nm; i++) {
#pragma HLS pipeline II = 1
                        j_arr[0] = i_j_strm[0].read();
                        o_j_strm.write(j_arr[0]);
                    }
                    nm_bk = i_nm_bk_strm.read();
                    nm = nm_bk.range(9, 0);
                    bk = nm_bk.range(19, 10);
                }
                o_nm_strm.write(nm);
                o_bk_strm.write(bk);
                break;
#ifndef __SYNTHESIS__
            default:
                std::cerr << "Error: illegal kernel switching combination\n";
                exit(1);
#endif
        }
    }
}

/// @brief top function of multi join
template <int KEYW, int S_PW, int T_PW, int ARW, int GRP_SZ>
void multi_join_unit(
#ifndef __SYNTHESIS__
    int pu_id,
#endif
    hls::stream<ap_uint<8> >& i_general_cfg_strm,
    ap_uint<32>& depth,
    hls::stream<ap_uint<3> >& join_flag_strm,
    // input large table
    hls::stream<ap_uint<KEYW> >& i_t_key_strm,
    hls::stream<ap_uint<T_PW> >& i_t_pld_strm,
    hls::stream<ap_uint<ARW> >& i_nm_strm,
    hls::stream<bool>& i_e0_strm,

    // input small table
    hls::stream<ap_uint<KEYW> >& i_base_s_key_strm,
    hls::stream<ap_uint<S_PW> >& i_base_s_pld_strm,
    hls::stream<ap_uint<KEYW> >& i_overflow_s_key_strm,
    hls::stream<ap_uint<S_PW> >& i_overflow_s_pld_strm,

    // output join result
    hls::stream<ap_uint<KEYW + S_PW + T_PW> >& o_j_strm,
    hls::stream<ap_uint<10> >& o_nm_strm,
    hls::stream<ap_uint<10> >& o_bk_strm) {
#pragma HLS INLINE off
#pragma HLS DATAFLOW

#ifndef __SYNTHESIS__
    unsigned int cnt0 = 0;
    unsigned int cnt1 = 0;
    unsigned int cnt2 = 0;
    unsigned int cnt3 = 0;

    bool hit_failed;
#endif

    hls::stream<ap_uint<8> > general_cfg_strms[3];
#pragma HLS stream variable = general_cfg_strms depth = 2
#pragma HLS bind_storage variable = general_cfg_strms type = fifo impl = srl

    hls::stream<ap_uint<KEYW> > i1_t_key_strm;
#pragma HLS STREAM variable = i1_t_key_strm depth = 1024
#pragma HLS bind_storage variable = i1_t_key_strm type = fifo impl = bram
    hls::stream<ap_uint<T_PW> > i1_t_pld_strm;
#pragma HLS STREAM variable = i1_t_pld_strm depth = 1024
#pragma HLS bind_storage variable = i1_t_pld_strm type = fifo impl = bram
    hls::stream<ap_uint<ARW> > i1_nm_strm;
#pragma HLS STREAM variable = i1_nm_strm depth = 1024
#pragma HLS bind_storage variable = i1_nm_strm type = fifo impl = bram
    hls::stream<ap_uint<3> > join1_flag_strm;
#pragma HLS STREAM variable = join1_flag_strm depth = 16
#pragma HLS bind_storage variable = join1_flag_strm type = fifo impl = srl
    hls::stream<bool> i1_e0_strm;
#pragma HLS STREAM variable = i1_e0_strm depth = 1024
#pragma HLS bind_storage variable = i1_e0_strm type = fifo impl = srl
    hls::stream<ap_uint<KEYW> > i2_t_key_strm;
#pragma HLS STREAM variable = i2_t_key_strm depth = 16
#pragma HLS bind_storage variable = i2_t_key_strm type = fifo impl = srl
    hls::stream<ap_uint<T_PW> > i2_t_pld_strm;
#pragma HLS STREAM variable = i2_t_pld_strm depth = 16
#pragma HLS bind_storage variable = i2_t_pld_strm type = fifo impl = srl
    hls::stream<ap_uint<ARW> > i2_nm_strm;
#pragma HLS STREAM variable = i2_nm_strm depth = 16
#pragma HLS bind_storage variable = i2_nm_strm type = fifo impl = srl
    hls::stream<ap_uint<3> > join2_flag_strm;
#pragma HLS STREAM variable = join2_flag_strm depth = 16
#pragma HLS bind_storage variable = join2_flag_strm type = fifo impl = srl
    hls::stream<bool> i2_e0_strm;
#pragma HLS STREAM variable = i2_e0_strm depth = 16
#pragma HLS bind_storage variable = i2_e0_strm type = fifo impl = srl

    hls::stream<ap_uint<KEYW + S_PW + T_PW> > i_j_strm[2];
#pragma HLS STREAM variable = i_j_strm depth = 16
#pragma HLS bind_storage variable = i_j_strm type = fifo impl = srl
    hls::stream<bool> i_e_strm[2];
#pragma HLS STREAM variable = i_e_strm depth = 16
#pragma HLS bind_storage variable = i_e_strm type = fifo impl = srl
    hls::stream<ap_uint<20> > i_nm_bk_strm;
#pragma HLS STREAM variable = i_nm_bk_strm depth = 4
#pragma HLS bind_storage variable = i_nm_bk_strm type = fifo impl = srl

    bool build_probe_flag;
    parse_cfg(i_general_cfg_strm, general_cfg_strms, build_probe_flag);

    // 1-to-2 demux
    split_stream<KEYW, S_PW, T_PW, ARW>(build_probe_flag, general_cfg_strms[0], join_flag_strm, i_t_key_strm,
                                        i_t_pld_strm, i_nm_strm, i_e0_strm, join1_flag_strm, i1_t_key_strm,
                                        i1_t_pld_strm, i1_nm_strm, i1_e0_strm, join2_flag_strm, i2_t_key_strm,
                                        i2_t_pld_strm, i2_nm_strm, i2_e0_strm);

#ifndef __SYNTHESIS__
#ifdef DEBUG_MISS
    std::cout << std::dec << "i1_t_key_strm output " << i1_t_key_strm.size() << std::endl;
    std::cout << std::dec << "i2_t_key_strm output " << i2_t_key_strm.size() << std::endl;
#endif
#endif

    // join hit
    join_unit_1<KEYW, S_PW, T_PW, ARW, GRP_SZ>(build_probe_flag, depth, general_cfg_strms[1], join1_flag_strm,
                                               i1_t_key_strm, i1_t_pld_strm, i1_nm_strm, i1_e0_strm, i_base_s_key_strm,
                                               i_base_s_pld_strm, i_overflow_s_key_strm, i_overflow_s_pld_strm,
                                               i_j_strm[0], i_e_strm[0], i_nm_bk_strm);

    // join unhit (ANTI-JOIN)
    hash_multi_join_build_probe::join_unit_2<KEYW, S_PW, T_PW, ARW>(build_probe_flag, join2_flag_strm, i2_t_key_strm,
                                                                    i2_t_pld_strm, i2_nm_strm, i2_e0_strm, i_j_strm[1],
                                                                    i_e_strm[1]);

#ifndef __SYNTHESIS__
#ifdef DEBUG_MISS
    std::cout << std::dec << "Join Unit 1 output " << i_j_strm[0].size() << std::endl;
    std::cout << std::dec << "Join Unit 2 output " << i_j_strm[1].size() << std::endl;
#endif
#endif

    // 2-to-1 mux
    combine_stream<KEYW, S_PW, T_PW, ARW>(build_probe_flag, general_cfg_strms[2], i_j_strm, i_e_strm, i_nm_bk_strm,
                                          o_j_strm, o_nm_strm, o_bk_strm);
#ifndef __SYNTHESIS__
#ifdef DEBUG_MISS
    std::cout << std::dec << "Join Unit output " << o_j_strm.size() << std::endl;
#endif
#endif
}

template <int PU_NM, int CH_NM>
void dup_cfg_signals(
    // inputs
    hls::stream<ap_uint<8> >& general_cfg_strm,
    hls::stream<ap_uint<3> >& join_flag_strm,
    hls::stream<ap_uint<15> >& part_cfg_strm,
    hls::stream<ap_uint<36> >& bf_cfg_strm,
    // outpus
    hls::stream<ap_uint<8> > general_cfg2dispatch_strm[CH_NM],
    hls::stream<ap_uint<8> > general_cfg2merge_strm[PU_NM],
    hls::stream<ap_uint<8> > general_cfg2pu_strm[PU_NM],
    hls::stream<ap_uint<8> > general_cfg2join_strm[PU_NM],
    hls::stream<ap_uint<8> > general_cfg2size_strm[PU_NM],
    hls::stream<ap_uint<8> >& general_cfg2collect_strm,
    hls::stream<ap_uint<3> > join_flags[PU_NM],
    hls::stream<ap_uint<15> > part_cfg2pu_strm[PU_NM],
    hls::stream<ap_uint<36> > bf_cfg2pu_strm[PU_NM]) {
    // read in configs
    ap_uint<8> general_cfg = general_cfg_strm.read();
    ap_uint<3> flag = join_flag_strm.read();
    ap_uint<15> part_cfg = part_cfg_strm.read();
    ap_uint<36> bf_cfg = bf_cfg_strm.read();
    bool build_probe_flag = general_cfg[5];

    // send out cfg singlas
    for (int i = 0; i < PU_NM; ++i) {
#pragma HLS unroll
        general_cfg2merge_strm[i].write(general_cfg);
        general_cfg2pu_strm[i].write(general_cfg);
        general_cfg2join_strm[i].write(general_cfg);
        general_cfg2size_strm[i].write(general_cfg);
        bf_cfg2pu_strm[i].write(bf_cfg);
        part_cfg2pu_strm[i].write(part_cfg);
    }
    for (int i = 0; i < CH_NM; i++) {
#pragma HLS unroll
        general_cfg2dispatch_strm[i].write(general_cfg);
    }
    general_cfg2collect_strm.write(general_cfg);

    if (build_probe_flag) {
        for (int i = 0; i < PU_NM; ++i) {
#pragma HLS unroll
            join_flags[i].write(flag);
        }
    }
}

/// @brief size adapter for join/bf/partition, upsizing partition results for higher throughput at collect
template <int PU, int KEYW, int S_PW, int B_PW, int EW, int COL_NM, int GRP_SZ>
void adapt_size(hls::stream<ap_uint<8> >& i_general_cfg_strm,
                // input result streams
                hls::stream<ap_uint<KEYW + S_PW + B_PW> >& i_strm,
                hls::stream<ap_uint<10> >& i_nm_strm,
                hls::stream<ap_uint<10> >& i_hp_bk_strm,

                // output adapted size streams
                hls::stream<ap_uint<EW * VEC_LEN> > o_strm[COL_NM],
                hls::stream<ap_uint<10> >& o_nm_strm,
                hls::stream<ap_uint<10> >& o_hp_bk_strm) {
    // static assertion for assuring output line is wide enough for loading each input row within 1 cycle
    XF_DATABASE_ASSERT((VEC_LEN * EW) >= GRP_SZ);
    XF_DATABASE_ASSERT((VEC_LEN * EW) >= (KEYW + S_PW + B_PW));

    ap_uint<8> general_cfg = i_general_cfg_strm.read();
    bool part_on = general_cfg[3];
    bool build_probe_flag = general_cfg[5];

    ap_uint<EW * VEC_LEN> out[COL_NM];
#pragma HLS array_partition variable = out dim = 1
    ap_uint<KEYW + S_PW + B_PW> din;
    ap_uint<Log2<VEC_LEN>::value> idx = 0;
    ap_uint<10> nm, bk;
    ap_uint<10> nm_r = 0;
    bool rd_e;

    bool last = false;
    if (build_probe_flag) {
        do {
#pragma HLS pipeline II = 1
            switch (general_cfg.range(4, 0)) {
                case gqe::JoinOn:
                case gqe::BloomFilterOn:
                    nm = i_nm_strm.read();
                    if (nm > 0) {
                        din = i_strm.read();
                        out[0].range(KEYW + S_PW + B_PW - 1, 0) = din;
                    }
                    {
#pragma HLS latency min = 1 max = 1
                        for (int c = 0; c < COL_NM; c++) {
#pragma HLS unroll
                            o_strm[c].write(out[c]);
                        }
                        o_nm_strm.write(nm);
                    }
                    last = (nm == 0);
                    break;
                case gqe::PartOn | gqe::BloomFilterOn:
                    rd_e = !i_nm_strm.empty() && !i_hp_bk_strm.empty();
                    if (nm_r == 0 && rd_e) {
                        i_nm_strm.read_nb(nm);
                        i_hp_bk_strm.read_nb(bk);
                        last = (nm == 0);
                        nm_r = (nm + VEC_LEN - 1) / VEC_LEN * VEC_LEN;
                        {
#pragma HLS latency min = 1 max = 1
                            o_nm_strm.write(nm);
                            o_hp_bk_strm.write(bk);
                        }
                    } else if (nm_r > 0) {
                        if (nm > 0)
                            din = i_strm.read();
                        else
                            din = 0;
                        for (int c = 0; c < COL_NM; c++) {
#pragma HLS unroll
                            out[c](EW * (idx + 1) - 1, EW * idx) = din((c + 1) * EW - 1, c * EW);
                        }
                        {
#pragma HLS latency min = 1 max = 1
                            for (int c = 0; c < COL_NM; c++) {
#pragma HLS unroll
                                if (idx == VEC_LEN - 1) o_strm[c].write(out[c]);
                            }
                        }
                        idx++;

                        if (nm > 0) {
                            nm--;
                        }
                        nm_r--;
                    }
                    break;
#ifndef __SYNTHESIS__
                default:
                    std::cerr << "Error: illegal kernel switching combination\n";
                    exit(1);
#endif
            }
        } while (!last);
    }
}
} // namespace details
} // namespace database
} // namespace xf

namespace xf {
namespace database {
namespace gqe {

/**
 * @brief Multi-functional-PU primitive, using multiple URAM/HBM buffers.
 *
 * This primitive shares most of the structure of ``hashMultiJoinBuildProbe``.
 * The inner table should be fed once, followed by the outer table once.
 * The HBM-based bloom-filter is integrated with the same structure of Join.
 *
 * @tparam HASH_MODE 0 for radix and 1 for Jenkin's Lookup3 hash.
 * @tparam KEYW width of key, in bit.
 * @tparam PW width of max payload, in bit.
 * @tparam S_PW width of payload of small table.
 * @tparam B_PW width of payload of big table.
 * @tparam HASHWH number of hash bits used for PU/buffer selection, 1~3.
 * @tparam HASHWL number of hash bits used for bloom-filtering in PU.
 * @tparam HASHWJ number of hash bits used for hash-table of join in PU.
 * @tparam ARW width of address, larger than 24 is suggested.
 * @tparam CH_NM number of input channels, 1,2,4.
 * @tparam COL_NM Output has 3 columns, partition uses all of them, join and bloom filter uses half of bit-width at
 * column 0.
 * @tparam GRP_SZ number of rows per group (for validation buffer only).
 * @tparam HBM_OUTSTANDING outstanding of HBM adapters.
 *
 * @param general_cfg_strm general configuration.
 * @param join_flag_strm specifies the join type, this flag is only read once.
 * @param part_cfg_strm partition configuration.
 * @param bf_cfg_strm bloom filter configuration.
 *
 * @param k0_strm_arry input of key columns of both tables.
 * @param p0_strm_arry input of payload columns of both tables.
 * @param e0_strm_arry input of end signal of both tables.
 *
 * @param htb0_buf HBM/DDR buffer of hash_table0
 * @param htb1_buf HBM/DDR buffer of hash_table1
 * @param htb2_buf HBM/DDR buffer of hash_table2
 * @param htb3_buf HBM/DDR buffer of hash_table3
 * @param htb4_buf HBM/DDR buffer of hash_table4
 * @param htb5_buf HBM/DDR buffer of hash_table5
 * @param htb6_buf HBM/DDR buffer of hash_table6
 * @param htb7_buf HBM/DDR buffer of hash_table7
 *
 * @param stb0_buf HBM/DDR buffer of PU0
 * @param stb1_buf HBM/DDR buffer of PU1
 * @param stb2_buf HBM/DDR buffer of PU2
 * @param stb3_buf HBM/DDR buffer of PU3
 * @param stb4_buf HBM/DDR buffer of PU4
 * @param stb5_buf HBM/DDR buffer of PU5
 * @param stb6_buf HBM/DDR buffer of PU6
 * @param stb7_buf HBM/DDR buffer of PU7
 *
 * @param pu_begin_status_strms constains depth of hash, row number of join result
 * @param pu_end_status_strms constains depth of hash, row number of join result
 *
 * @param out_strm output results
 * @param e_out_strm end flag of output results
 */
template <int HASH_MODE,
          int KEYW,
          int PW,
          int S_PW,
          int B_PW,
          int HASHWH,
          int HASHWL,
          int HASHWJ,
          int ARW,
          int CH_NM,
          int COL_NM,
          int GRP_SZ,
          int HBM_OUTSTANDING>
void multiFuncPU(hls::stream<ap_uint<8> >& general_cfg_strm,
                 hls::stream<ap_uint<3> >& join_flag_strm, // join type
                 hls::stream<ap_uint<15> >& part_cfg_strm,
                 hls::stream<ap_uint<36> >& bf_cfg_strm,
                 // input
                 hls::stream<ap_uint<KEYW> > k0_strm_arry[CH_NM],
                 hls::stream<ap_uint<PW> > p0_strm_arry[CH_NM],
                 hls::stream<bool> e0_strm_arry[CH_NM],

                 // output hash table
                 hls::burst_maxi<ap_uint<256> >& htb0_buf,
                 hls::burst_maxi<ap_uint<256> >& htb1_buf,
                 hls::burst_maxi<ap_uint<256> >& htb2_buf,
                 hls::burst_maxi<ap_uint<256> >& htb3_buf,
                 hls::burst_maxi<ap_uint<256> >& htb4_buf,
                 hls::burst_maxi<ap_uint<256> >& htb5_buf,
                 hls::burst_maxi<ap_uint<256> >& htb6_buf,
                 hls::burst_maxi<ap_uint<256> >& htb7_buf,

                 // output
                 hls::burst_maxi<ap_uint<256> >& stb0_buf,
                 hls::burst_maxi<ap_uint<256> >& stb1_buf,
                 hls::burst_maxi<ap_uint<256> >& stb2_buf,
                 hls::burst_maxi<ap_uint<256> >& stb3_buf,
                 hls::burst_maxi<ap_uint<256> >& stb4_buf,
                 hls::burst_maxi<ap_uint<256> >& stb5_buf,
                 hls::burst_maxi<ap_uint<256> >& stb6_buf,
                 hls::burst_maxi<ap_uint<256> >& stb7_buf,

                 hls::stream<ap_uint<32> >& pu_begin_status_strms,
                 hls::stream<ap_uint<32> >& pu_end_status_strms,

                 hls::stream<ap_uint<PW * VEC_LEN> > out_strm[COL_NM],
                 hls::stream<ap_uint<10> >& out_nm_strm,
                 hls::stream<ap_uint<10 + HASHWH> >& hp_bkpu_strm) {
#pragma HLS DATAFLOW

    enum { PU = (1 << HASHWH) }; // high hash for distribution.

    // general_cfg from input to dispatch/merge, PU, multi join unit, and collect
    hls::stream<ap_uint<8> > general_cfg2dispatch_strms[CH_NM];
#pragma HLS stream variable = general_cfg2dispatch_strms depth = 2
    hls::stream<ap_uint<8> > general_cfg2merge_strms[PU];
#pragma HLS stream variable = general_cfg2merge_strms depth = 2
    hls::stream<ap_uint<8> > general_cfg2pu_strms[PU];
#pragma HLS stream variable = general_cfg2pu_strms depth = 2
    hls::stream<ap_uint<8> > general_cfg2join_strms[PU];
#pragma HLS stream variable = general_cfg2join_strms depth = 2
    hls::stream<ap_uint<8> > general_cfg2size_strms[PU];
#pragma HLS stream variable = general_cfg2size_strms depth = 2
    hls::stream<ap_uint<8> > general_cfg2collect_strm;
#pragma HLS stream variable = general_cfg2collect_strm depth = 2
    // join_flag from input to multi join unit
    hls::stream<ap_uint<3> > join_flag_strms[PU];
#pragma HLS stream variable = join_flag_strms depth = 2
    // part_cfg from input to each PU
    hls::stream<ap_uint<15> > part_cfg2pu_strms[PU];
#pragma HLS stream variable = part_cfg2pu_strms depth = 2
    // bf_cfg from input to each PU
    hls::stream<ap_uint<36> > bf_cfg2pu_strms[PU];
#pragma HLS stream variable = bf_cfg2pu_strms depth = 2

    details::dup_cfg_signals<PU, CH_NM>(general_cfg_strm, join_flag_strm, part_cfg_strm, bf_cfg_strm,

                                        general_cfg2dispatch_strms, general_cfg2merge_strms, general_cfg2pu_strms,
                                        general_cfg2join_strms, general_cfg2size_strms, general_cfg2collect_strm,
                                        join_flag_strms, part_cfg2pu_strms, bf_cfg2pu_strms);

    ap_uint<32> depth;
    ap_uint<32> join_num;

    // dispatch k0_strm_arry, p0_strm_arry, e0strm_arry to channel1-4
    // Channel1
    hls::stream<ap_uint<KEYW + PW + HASHWL + 1> > pack_strm_arry_c0[PU];
#pragma HLS stream variable = pack_strm_arry_c0 depth = 8
#pragma HLS bind_storage variable = pack_strm_arry_c0 type = fifo impl = srl
    // Channel2
    hls::stream<ap_uint<KEYW + PW + HASHWL + 1> > pack_strm_arry_c1[PU];
#pragma HLS stream variable = pack_strm_arry_c1 depth = 8
#pragma HLS bind_storage variable = pack_strm_arry_c1 type = fifo impl = srl
    // Channel3
    hls::stream<ap_uint<KEYW + PW + HASHWL + 1> > pack_strm_arry_c2[PU];
#pragma HLS stream variable = pack_strm_arry_c2 depth = 8
#pragma HLS bind_storage variable = pack_strm_arry_c2 type = fifo impl = srl
    // Channel4
    hls::stream<ap_uint<KEYW + PW + HASHWL + 1> > pack_strm_arry_c3[PU];
#pragma HLS stream variable = pack_strm_arry_c3 depth = 8
#pragma HLS bind_storage variable = pack_strm_arry_c3 type = fifo impl = srl
    // merge channel1-channel4 to here, then perform build or probe
    hls::stream<ap_uint<KEYW> > k1_strm_arry[PU];
#pragma HLS stream variable = k1_strm_arry depth = 8
#pragma HLS bind_storage variable = k1_strm_arry type = fifo impl = srl
    hls::stream<ap_uint<PW> > p1_strm_arry[PU];
#pragma HLS stream variable = p1_strm_arry depth = 8
#pragma HLS bind_storage variable = p1_strm_arry type = fifo impl = srl
    hls::stream<ap_uint<HASHWL> > hash_strm_arry[PU];
#pragma HLS stream variable = hash_strm_arry depth = 8
#pragma HLS bind_storage variable = hash_strm_arry type = fifo impl = srl
    hls::stream<bool> e1_strm_arry[PU];
#pragma HLS stream variable = e1_strm_arry depth = 8
#pragma HLS bind_storage variable = e1_strm_arry type = fifo impl = srl

    // output of PU for join
    hls::stream<ap_uint<KEYW> > t_key_strm_arry[PU];
#pragma HLS stream variable = t_key_strm_arry depth = 64
#pragma HLS bind_storage variable = t_key_strm_arry type = fifo impl = srl
    hls::stream<ap_uint<B_PW> > t_pld_strm_arry[PU];
#pragma HLS stream variable = t_pld_strm_arry depth = 64
#pragma HLS bind_storage variable = t_pld_strm_arry type = fifo impl = srl
    hls::stream<ap_uint<ARW> > nm_strm_arry[PU];
#pragma HLS stream variable = nm_strm_arry depth = 64
#pragma HLS bind_storage variable = nm_strm_arry type = fifo impl = srl
    hls::stream<bool> e2_strm_arry[PU];
#pragma HLS stream variable = e2_strm_arry depth = 64
#pragma HLS bind_storage variable = e2_strm_arry type = fifo impl = srl

    hls::stream<ap_uint<KEYW> > s_base_key_strm_arry[PU];
#pragma HLS stream variable = s_base_key_strm_arry depth = 32
#pragma HLS bind_storage variable = s_base_key_strm_arry type = fifo impl = srl
    hls::stream<ap_uint<S_PW> > s_base_pld_strm_arry[PU];
#pragma HLS stream variable = s_base_pld_strm_arry depth = 32
#pragma HLS bind_storage variable = s_base_pld_strm_arry type = fifo impl = srl
    hls::stream<ap_uint<KEYW> > s_overflow_key_strm_arry[PU];
#pragma HLS stream variable = s_overflow_key_strm_arry depth = 32
#pragma HLS bind_storage variable = s_overflow_key_strm_arry type = fifo impl = srl
    hls::stream<ap_uint<S_PW> > s_overflow_pld_strm_arry[PU];
#pragma HLS stream variable = s_overflow_pld_strm_arry depth = 32
#pragma HLS bind_storage variable = s_overflow_pld_strm_arry type = fifo impl = srl

    // output of join for adapt size
    hls::stream<ap_uint<KEYW + S_PW + B_PW> > j0_strm_arry[PU];
#pragma HLS stream variable = j0_strm_arry depth = 16
#pragma HLS bind_storage variable = j0_strm_arry type = fifo impl = srl
    hls::stream<ap_uint<10> > j0_nm_strm_arry[PU];
#pragma HLS stream variable = j0_nm_strm_arry depth = 16
#pragma HLS bind_storage variable = j0_nm_strm_arry type = fifo impl = srl
    hls::stream<ap_uint<10> > j0_bk_strm_arry[PU];
#pragma HLS stream variable = j0_bk_strm_arry depth = 8
#pragma HLS bind_storage variable = j0_bk_strm_arry type = fifo impl = srl
    hls::stream<ap_uint<10> > adapt_nm_strm[PU];
#pragma HLS stream variable = adapt_nm_strm depth = 8
#pragma HLS bind_storage variable = adapt_nm_strm type = fifo impl = srl
    hls::stream<ap_uint<10> > adapt_bk_strm[PU];
#pragma HLS stream variable = adapt_bk_strm depth = 8
#pragma HLS bind_storage variable = adapt_bk_strm type = fifo impl = srl

    // output of adapt size for collect
    hls::stream<ap_uint<KEYW / 2 * VEC_LEN> > adapt_strm[PU][COL_NM];
#pragma HLS stream variable = adapt_strm depth = 512
#pragma HLS resource variable = adapt_strm core = FIFO_BRAM

//------------------------------read status-----------------------------------
#ifndef __SYNTHESIS__
#ifdef DEBUG_BLOOMFILTER_JOIN
    std::cout << "------------------------read status------------------------" << std::endl;
#endif
#endif
    details::join_v3::sc::read_status<PU>(pu_begin_status_strms, depth);

//---------------------------------dispatch-------------------------------
#ifndef __SYNTHESIS__
#ifdef DEBUG_BLOOMFILTER_JOIN
    std::cout << "------------------------dispatch PU------------------------" << std::endl;
#endif
#endif

    if (CH_NM >= 1) {
        details::dispatch_unit<HASH_MODE, KEYW, PW, HASHWH, HASHWL, PU, CH_NM>(
            general_cfg2dispatch_strms[0], k0_strm_arry[0], p0_strm_arry[0], e0_strm_arry[0], pack_strm_arry_c0);
    }

    if (CH_NM >= 2) {
        details::dispatch_unit<HASH_MODE, KEYW, PW, HASHWH, HASHWL, PU, CH_NM>(
            general_cfg2dispatch_strms[1], k0_strm_arry[1], p0_strm_arry[1], e0_strm_arry[1], pack_strm_arry_c1);
    }

    if (CH_NM >= 4) {
        details::dispatch_unit<HASH_MODE, KEYW, PW, HASHWH, HASHWL, PU, CH_NM>(
            general_cfg2dispatch_strms[2], k0_strm_arry[2], p0_strm_arry[2], e0_strm_arry[2], pack_strm_arry_c2);

        details::dispatch_unit<HASH_MODE, KEYW, PW, HASHWH, HASHWL, PU, CH_NM>(
            general_cfg2dispatch_strms[3], k0_strm_arry[3], p0_strm_arry[3], e0_strm_arry[3], pack_strm_arry_c3);
    }

//---------------------------------merge---------------------------------
#ifndef __SYNTHESIS__
#ifdef DEBUG_BLOOMFILTER_JOIN
    std::cout << "     ";
    for (int i = 0; i < PU; i++) {
        std::cout << "       PU" << i;
    }
    std::cout << std::endl;
    if (CH_NM >= 1) {
        std::cout << "ch:0 ";
        for (int i = 0; i < PU; i++) {
            std::cout << std::setw(10) << pack_strm_arry_c0[i].size();
        }
        std::cout << std::endl;
    }
    if (CH_NM >= 2) {
        std::cout << "ch:1 ";
        for (int i = 0; i < PU; i++) {
            std::cout << std::setw(10) << pack_strm_arry_c1[i].size();
        }
        std::cout << std::endl;
    }
    if (CH_NM >= 4) {
        std::cout << "ch:2 ";
        for (int i = 0; i < PU; i++) {
            std::cout << std::setw(10) << pack_strm_arry_c2[i].size();
        }
        std::cout << std::endl;
        std::cout << "ch:3 ";
        for (int i = 0; i < PU; i++) {
            std::cout << std::setw(10) << pack_strm_arry_c3[i].size();
        }
        std::cout << std::endl;
    }
    std::cout << "------------------------merge PU------------------------" << std::endl;
#endif
#endif

    if (CH_NM == 1) {
        for (int p = 0; p < PU; ++p) {
#pragma HLS unroll
            details::merge1_1<KEYW, PW, HASHWL>(general_cfg2merge_strms[p], pack_strm_arry_c0[p],

                                                k1_strm_arry[p], p1_strm_arry[p], hash_strm_arry[p], e1_strm_arry[p]);
        }
    } else if (CH_NM == 2) {
        for (int p = 0; p < PU; p++) {
#pragma HLS unroll
            details::merge2_1<KEYW, PW, HASHWL>(general_cfg2merge_strms[p], pack_strm_arry_c0[p], pack_strm_arry_c1[p],

                                                k1_strm_arry[p], p1_strm_arry[p], hash_strm_arry[p], e1_strm_arry[p]);
        }
    } else {
        // CH_NM == 4
        for (int p = 0; p < PU; p++) {
#pragma HLS unroll
#ifndef __SYNTHESIS__
#ifdef DEBUG_BLOOMFILTER_JOIN
            std::cout << "PU" << p << "\n";
#endif
#endif
            details::merge4_1<KEYW, PW, HASHWL>(general_cfg2merge_strms[p], pack_strm_arry_c0[p], pack_strm_arry_c1[p],
                                                pack_strm_arry_c2[p], pack_strm_arry_c3[p],

                                                k1_strm_arry[p], p1_strm_arry[p], hash_strm_arry[p], e1_strm_arry[p]);
        }
    }
#ifndef __SYNTHESIS__
#ifdef DEBUG_BLOOMFILTER_JOIN
    for (int i = 0; i < PU; i++) {
        std::cout << "       PU" << i;
    }
    std::cout << std::endl;
    for (int i = 0; i < PU; i++) {
        std::cout << std::setw(10) << k1_strm_arry[i].size();
    }
    std::cout << std::endl;
#endif
#endif

    //-------------------------------PU----------------------------------------
    if (PU >= 1) {
#ifndef __SYNTHESIS__
#ifdef DEBUG
        std::cout << "------------------------PU0------------------------" << std::endl;
#endif
#endif
        details::build_probe_core_wrapper<HASH_MODE, HASHWH, HASHWL, HASHWJ, KEYW, S_PW, B_PW, ARW, HBM_OUTSTANDING>(
            general_cfg2pu_strms[0], part_cfg2pu_strms[0], bf_cfg2pu_strms[0],
            // input status
            depth,

            // input table
            hash_strm_arry[0], k1_strm_arry[0], p1_strm_arry[0], e1_strm_arry[0],

            // output for join
            t_key_strm_arry[0], t_pld_strm_arry[0], nm_strm_arry[0], e2_strm_arry[0], s_base_key_strm_arry[0],
            s_base_pld_strm_arry[0], s_overflow_key_strm_arry[0], s_overflow_pld_strm_arry[0],

            // HBM/DDR
            htb0_buf, stb0_buf);
    }

    if (PU >= 2) {
#ifndef __SYNTHESIS__
#ifdef DEBUG
        std::cout << "------------------------PU1------------------------" << std::endl;
#endif
#endif
        details::build_probe_core_wrapper<HASH_MODE, HASHWH, HASHWL, HASHWJ, KEYW, S_PW, B_PW, ARW, HBM_OUTSTANDING>(
            general_cfg2pu_strms[1], part_cfg2pu_strms[1], bf_cfg2pu_strms[1],
            // input status
            depth,

            // input t-table
            hash_strm_arry[1], k1_strm_arry[1], p1_strm_arry[1], e1_strm_arry[1],

            // output for join
            t_key_strm_arry[1], t_pld_strm_arry[1], nm_strm_arry[1], e2_strm_arry[1], s_base_key_strm_arry[1],
            s_base_pld_strm_arry[1], s_overflow_key_strm_arry[1], s_overflow_pld_strm_arry[1],

            // HBM/DDR
            htb1_buf, stb1_buf);
    }

    if (PU >= 4) {
#ifndef __SYNTHESIS__
#ifdef DEBUG
        std::cout << "------------------------PU2------------------------" << std::endl;
#endif
#endif
        details::build_probe_core_wrapper<HASH_MODE, HASHWH, HASHWL, HASHWJ, KEYW, S_PW, B_PW, ARW, HBM_OUTSTANDING>(
            general_cfg2pu_strms[2], part_cfg2pu_strms[2], bf_cfg2pu_strms[2],
            // input status
            depth,

            // input t-table
            hash_strm_arry[2], k1_strm_arry[2], p1_strm_arry[2], e1_strm_arry[2],

            // output for join
            t_key_strm_arry[2], t_pld_strm_arry[2], nm_strm_arry[2], e2_strm_arry[2], s_base_key_strm_arry[2],
            s_base_pld_strm_arry[2], s_overflow_key_strm_arry[2], s_overflow_pld_strm_arry[2],

            // HBM/DDR
            htb2_buf, stb2_buf);

#ifndef __SYNTHESIS__
#ifdef DEBUG
        std::cout << "------------------------PU3------------------------" << std::endl;
#endif
#endif
        details::build_probe_core_wrapper<HASH_MODE, HASHWH, HASHWL, HASHWJ, KEYW, S_PW, B_PW, ARW, HBM_OUTSTANDING>(
            general_cfg2pu_strms[3], part_cfg2pu_strms[3], bf_cfg2pu_strms[3],
            // input status
            depth,

            // input t-table
            hash_strm_arry[3], k1_strm_arry[3], p1_strm_arry[3], e1_strm_arry[3],

            // output for join
            t_key_strm_arry[3], t_pld_strm_arry[3], nm_strm_arry[3], e2_strm_arry[3], s_base_key_strm_arry[3],
            s_base_pld_strm_arry[3], s_overflow_key_strm_arry[3], s_overflow_pld_strm_arry[3],

            // HBM/DDR
            htb3_buf, stb3_buf);
    }

    if (PU >= 8) {
#ifndef __SYNTHESIS__
#ifdef DEBUG
        std::cout << "------------------------PU4------------------------" << std::endl;
#endif
#endif
        details::build_probe_core_wrapper<HASH_MODE, HASHWH, HASHWL, HASHWJ, KEYW, S_PW, B_PW, ARW, HBM_OUTSTANDING>(
            general_cfg2pu_strms[4], part_cfg2pu_strms[4], bf_cfg2pu_strms[4],
            // input status
            depth,

            // input t-table
            hash_strm_arry[4], k1_strm_arry[4], p1_strm_arry[4], e1_strm_arry[4],

            // output for join
            t_key_strm_arry[4], t_pld_strm_arry[4], nm_strm_arry[4], e2_strm_arry[4], s_base_key_strm_arry[4],
            s_base_pld_strm_arry[4], s_overflow_key_strm_arry[4], s_overflow_pld_strm_arry[4],

            // HBM/DDR
            htb4_buf, stb4_buf);

#ifndef __SYNTHESIS__
#ifdef DEBUG
        std::cout << "------------------------PU5------------------------" << std::endl;
#endif
#endif
        details::build_probe_core_wrapper<HASH_MODE, HASHWH, HASHWL, HASHWJ, KEYW, S_PW, B_PW, ARW, HBM_OUTSTANDING>(
            general_cfg2pu_strms[5], part_cfg2pu_strms[5], bf_cfg2pu_strms[5],
            // input status
            depth,

            // input t-table
            hash_strm_arry[5], k1_strm_arry[5], p1_strm_arry[5], e1_strm_arry[5],

            // output for join
            t_key_strm_arry[5], t_pld_strm_arry[5], nm_strm_arry[5], e2_strm_arry[5], s_base_key_strm_arry[5],
            s_base_pld_strm_arry[5], s_overflow_key_strm_arry[5], s_overflow_pld_strm_arry[5],

            // HBM/DDR
            htb5_buf, stb5_buf);

#ifndef __SYNTHESIS__
#ifdef DEBUG
        std::cout << "------------------------PU6------------------------" << std::endl;
#endif
#endif
        details::build_probe_core_wrapper<HASH_MODE, HASHWH, HASHWL, HASHWJ, KEYW, S_PW, B_PW, ARW, HBM_OUTSTANDING>(
            general_cfg2pu_strms[6], part_cfg2pu_strms[6], bf_cfg2pu_strms[6],
            // input status
            depth,

            // input t-table
            hash_strm_arry[6], k1_strm_arry[6], p1_strm_arry[6], e1_strm_arry[6],

            // output for join
            t_key_strm_arry[6], t_pld_strm_arry[6], nm_strm_arry[6], e2_strm_arry[6], s_base_key_strm_arry[6],
            s_base_pld_strm_arry[6], s_overflow_key_strm_arry[6], s_overflow_pld_strm_arry[6],

            // HBM/DDR
            htb6_buf, stb6_buf);

#ifndef __SYNTHESIS__
#ifdef DEBUG
        std::cout << "------------------------PU7------------------------" << std::endl;
#endif
#endif
        details::build_probe_core_wrapper<HASH_MODE, HASHWH, HASHWL, HASHWJ, KEYW, S_PW, B_PW, ARW, HBM_OUTSTANDING>(
            general_cfg2pu_strms[7], part_cfg2pu_strms[7], bf_cfg2pu_strms[7],
            // input status
            depth,

            // input t-table
            hash_strm_arry[7], k1_strm_arry[7], p1_strm_arry[7], e1_strm_arry[7],

            // output for join
            t_key_strm_arry[7], t_pld_strm_arry[7], nm_strm_arry[7], e2_strm_arry[7], s_base_key_strm_arry[7],
            s_base_pld_strm_arry[7], s_overflow_key_strm_arry[7], s_overflow_pld_strm_arry[7],

            // HBM/DDR
            htb7_buf, stb7_buf);
    }
#ifndef __SYNTHESIS__
#ifdef DEBUG_MISS
    for (int i = 0; i < PU; i++) {
        std::cout << "------------------after probe PU" << i << "--------------------------\n";
        std::cout << "base has " << s_base_key_strm_arry[i].size() << " rows, overflow has "
                  << s_overflow_key_strm_arry[i].size() << std::endl;
    }
#endif
#endif

    //-----------------------------------join--------------------------------------
    for (int i = 0; i < PU; i++) {
#pragma HLS unroll
        details::multi_join_unit<KEYW, S_PW, B_PW, ARW, GRP_SZ>(
#ifndef __SYNTHESIS__
            i,
#endif
            general_cfg2join_strms[i],

            depth, join_flag_strms[i], t_key_strm_arry[i], t_pld_strm_arry[i], nm_strm_arry[i], e2_strm_arry[i],
            s_base_key_strm_arry[i], s_base_pld_strm_arry[i], s_overflow_key_strm_arry[i], s_overflow_pld_strm_arry[i],
            j0_strm_arry[i], j0_nm_strm_arry[i], j0_bk_strm_arry[i]);
    }

    //------------------------------adapt size-----------------------------------
    for (int i = 0; i < PU; i++) {
#pragma HLS unroll
        details::adapt_size<PU, KEYW, S_PW, B_PW, KEYW / 2, COL_NM, GRP_SZ>(
            general_cfg2size_strms[i], j0_strm_arry[i], j0_nm_strm_arry[i], j0_bk_strm_arry[i], adapt_strm[i],
            adapt_nm_strm[i], adapt_bk_strm[i]);
    }

    //-----------------------------------collect-----------------------------------
    details::collect<PU, KEYW / 2, COL_NM, GRP_SZ>(general_cfg2collect_strm, adapt_strm, adapt_nm_strm, adapt_bk_strm,

                                                   join_num, out_strm, out_nm_strm, hp_bkpu_strm);
#ifndef __SYNTHESIS__
    for (int c = 0; c < COL_NM; c++) {
        std::cout << "out_strm[" << c << "].size() = " << out_strm[c].size() << std::endl;
    }
    std::cout << "out_nm_strm.size() = " << out_nm_strm.size() << std::endl;
#endif

    //------------------------------write status-----------------------------------
    details::join_v3::sc::write_status<PU>(pu_end_status_strms, depth, join_num);
}

// ---------------------------- Processing Wrapper -----------------------------------

/* XXX if dual key, shift the payload,
 * so that for the 3rd col of A table becomes 1st payload
 */
template <int COL_NM, int PLD_NM, int ROUND_NM>
void multi_func_pu_channel_adapter(bool mk_on,
                                   hls::stream<ap_uint<8 * TPCH_INT_SZ> > in_strm[COL_NM],
                                   hls::stream<bool>& e_in_strm,
                                   hls::stream<ap_uint<8 * TPCH_INT_SZ * 2> >& key_strm,
                                   hls::stream<ap_uint<8 * TPCH_INT_SZ * PLD_NM> >& pld_strm,
                                   hls::stream<bool>& e_join_pld_strm) {
#if !defined __SYNTHESIS__ && USER_DEBUG == 1
    for (int i = 0; i < COL_NM; ++i) {
        printf("Hash join input %d data %d\n", i, in_strm[i].size());
    }
    printf("Hash join end flag %d\n", e_in_strm.size());
#endif
    // 1 round only in current implementation
    for (int r = 0; r < ROUND_NM; ++r) {
        bool e = true;
        int cnt = 0;
        while (!(e = e_in_strm.read())) {
#pragma HLS pipeline II = 1

            ap_uint<8 * TPCH_INT_SZ * 2> key_tmp;
            ap_uint<8 * TPCH_INT_SZ * PLD_NM> pld_tmp;

            ap_uint<8 * TPCH_INT_SZ> d_tmp[COL_NM];
#pragma HLS array_partition variable = d_tmp complete
            for (int c = 0; c < COL_NM; ++c) {
#pragma HLS unroll
                d_tmp[c] = in_strm[c].read();
            }
            cnt++;

            key_tmp.range(8 * TPCH_INT_SZ - 1, 0) = d_tmp[0];
            // if dual key off, pad the 2nd key with 0s
            key_tmp.range(8 * TPCH_INT_SZ * 2 - 1, 8 * TPCH_INT_SZ) = mk_on ? d_tmp[1] : ap_uint<8 * TPCH_INT_SZ>(0);

            for (int c = 0; c < PLD_NM; ++c) {
#pragma HLS unroll
                pld_tmp.range(8 * TPCH_INT_SZ * (c + 1) - 1, 8 * TPCH_INT_SZ * c) = d_tmp[2 + c];
            }

            key_strm.write(key_tmp);
            pld_strm.write(pld_tmp);
#ifdef USER_DEBUG
            std::cout << "key_tmp: " << key_tmp << std::endl;
            std::cout << "pld_tmp: " << pld_tmp << std::endl;
#endif
            e_join_pld_strm.write(false);
        }
        e_join_pld_strm.write(true);
    }
}

template <int COL_NM, int CH_NM, int PU_NM, int ROUND_NM, int GRP_SZ, int HBM_OUTSTANDING>
void multi_func_pu_plus_adapter(bool mk_on,
                                hls::stream<ap_uint<8> >& general_cfg_strm,
                                hls::stream<ap_uint<3> >& join_flag_strm,
                                hls::stream<ap_uint<15> >& part_cfg_strm,
                                hls::stream<ap_uint<36> >& bf_cfg_strm,
                                hls::stream<ap_uint<8 * TPCH_INT_SZ> > in_strm[CH_NM][COL_NM],
                                hls::stream<bool> e_in_strm[CH_NM],
                                hls::stream<ap_uint<8 * TPCH_INT_SZ * VEC_LEN> > out_strm[COL_NM],
                                hls::stream<ap_uint<10> >& out_nm_strm,
                                hls::stream<ap_uint<10 + Log2<PU_NM>::value> >& hp_bkpu_strm,
                                hls::stream<ap_uint<32> >& pu_b_status_strm,
                                hls::stream<ap_uint<32> >& pu_e_status_strm,
                                hls::burst_maxi<ap_uint<256> >& htb_buf0,
                                hls::burst_maxi<ap_uint<256> >& htb_buf1,
                                hls::burst_maxi<ap_uint<256> >& htb_buf2,
                                hls::burst_maxi<ap_uint<256> >& htb_buf3,
                                hls::burst_maxi<ap_uint<256> >& htb_buf4,
                                hls::burst_maxi<ap_uint<256> >& htb_buf5,
                                hls::burst_maxi<ap_uint<256> >& htb_buf6,
                                hls::burst_maxi<ap_uint<256> >& htb_buf7,
                                hls::burst_maxi<ap_uint<256> >& stb_buf0,
                                hls::burst_maxi<ap_uint<256> >& stb_buf1,
                                hls::burst_maxi<ap_uint<256> >& stb_buf2,
                                hls::burst_maxi<ap_uint<256> >& stb_buf3,
                                hls::burst_maxi<ap_uint<256> >& stb_buf4,
                                hls::burst_maxi<ap_uint<256> >& stb_buf5,
                                hls::burst_maxi<ap_uint<256> >& stb_buf6,
                                hls::burst_maxi<ap_uint<256> >& stb_buf7) {
#pragma HLS dataflow

    hls::stream<ap_uint<8 * TPCH_INT_SZ * 2> > key_strm[CH_NM];
#pragma HLS stream variable = key_strm depth = 16
    hls::stream<ap_uint<8 * TPCH_INT_SZ> > pld_strm[CH_NM];
#pragma HLS stream variable = pld_strm depth = 64
    hls::stream<bool> e_join_pld_strm[CH_NM];
#pragma HLS stream variable = e_join_pld_strm depth = 16

    hls::stream<ap_uint<8 * TPCH_INT_SZ*(COL_NM + 1)> > joined_strm;
#pragma HLS stream variable = joined_strm depth = 16
    hls::stream<bool> e_joined_strm;
#pragma HLS stream variable = e_joined_strm depth = 16

    const int PLD_NM = 1;
    // let each channel adapt independently
    for (int ch = 0; ch < CH_NM; ++ch) {
#pragma HLS unroll
        multi_func_pu_channel_adapter<COL_NM, PLD_NM, ROUND_NM>(mk_on, in_strm[ch], e_in_strm[ch], key_strm[ch],
                                                                pld_strm[ch], e_join_pld_strm[ch]);
    }

#if !defined __SYNTHESIS__ && USER_DEBUG == 1
    printf("***** after adapt for hash-join\n");
    for (int ch = 0; ch < CH_NM; ++ch) {
        printf("ch:%d nrow=%ld,%ld, nflag=%ld\n", ch, key_strm[ch].size(), pld_strm[ch].size(),
               e_join_pld_strm[ch].size());
    }
#endif

    // <int HASH_MODE, int KEYW, int PW, int S_PW, int B_PW, int HASHWH, int HASHWL, int HASHWJ,int ARW, int CH_NM, int
    // COL_NM, int GRP_SZ, int HBM_OUTSTANDING
    multiFuncPU<1, 128, 64, 64, 64, 3, 61, 17, 24, CH_NM, COL_NM, GRP_SZ, HBM_OUTSTANDING>(
        general_cfg_strm, join_flag_strm, part_cfg_strm, bf_cfg_strm, key_strm, pld_strm, e_join_pld_strm, htb_buf0,
        htb_buf1, htb_buf2, htb_buf3, htb_buf4, htb_buf5, htb_buf6, htb_buf7, stb_buf0, stb_buf1, stb_buf2, stb_buf3,
        stb_buf4, stb_buf5, stb_buf6, stb_buf7, pu_b_status_strm, pu_e_status_strm, out_strm, out_nm_strm,
        hp_bkpu_strm);

#if !defined __SYNTHESIS__ && USER_DEBUG == 1
    printf("Hash joined row %d\n", out_strm[0].size());
#endif
}

template <int COL_NM, int CH_NM, int PU_NM, int ROUND_NM, int GRP_SZ, int HBM_OUTSTANDING>
void multi_func_pu_wrapper(hls::stream<ap_uint<8> >& i_general_cfg_strm,
                           hls::stream<ap_uint<3> >& i_join_cfg_strm,
                           hls::stream<ap_uint<15> >& i_part_cfg_strm,
                           hls::stream<ap_uint<36> >& i_bf_cfg_strm,
                           hls::stream<ap_uint<8 * TPCH_INT_SZ> > in_strm[CH_NM][COL_NM],
                           hls::stream<bool> e_in_strm[CH_NM],
                           hls::stream<ap_uint<8> >& o_general_cfg_strm,
                           hls::stream<ap_uint<8 * TPCH_INT_SZ * VEC_LEN> > out_strm[COL_NM],
                           hls::stream<ap_uint<10> >& out_nm_strm,
                           hls::stream<ap_uint<32> >& nr_row_strm,
                           hls::stream<ap_uint<10 + Log2<PU_NM>::value> >& hp_bkpu_strm,
                           hls::burst_maxi<ap_uint<256> >& htb_buf0,
                           hls::burst_maxi<ap_uint<256> >& htb_buf1,
                           hls::burst_maxi<ap_uint<256> >& htb_buf2,
                           hls::burst_maxi<ap_uint<256> >& htb_buf3,
                           hls::burst_maxi<ap_uint<256> >& htb_buf4,
                           hls::burst_maxi<ap_uint<256> >& htb_buf5,
                           hls::burst_maxi<ap_uint<256> >& htb_buf6,
                           hls::burst_maxi<ap_uint<256> >& htb_buf7,
                           hls::burst_maxi<ap_uint<256> >& stb_buf0,
                           hls::burst_maxi<ap_uint<256> >& stb_buf1,
                           hls::burst_maxi<ap_uint<256> >& stb_buf2,
                           hls::burst_maxi<ap_uint<256> >& stb_buf3,
                           hls::burst_maxi<ap_uint<256> >& stb_buf4,
                           hls::burst_maxi<ap_uint<256> >& stb_buf5,
                           hls::burst_maxi<ap_uint<256> >& stb_buf6,
                           hls::burst_maxi<ap_uint<256> >& stb_buf7) {
    ap_uint<8> general_cfg = i_general_cfg_strm.read();
    o_general_cfg_strm.write(general_cfg);

    bool bypass_on = general_cfg[0];
    bool bp_flag = general_cfg[5];
    bool mk_on = general_cfg[6];

    // bit2: append mode not used here
    ap_uint<3> join_cfg = i_join_cfg_strm.read();
    ap_uint<3> join_type = join_cfg[1, 0];

    // TODO: this signal should follow the data path of gqePart's PU
    ap_uint<15> part_cfg = i_part_cfg_strm.read();

    ap_uint<36> bf_cfg = i_bf_cfg_strm.read();

    hls::stream<ap_uint<8> > general_cfg_strm;
#pragma HLS stream variable = general_cfg_strm depth = 2
    hls::stream<ap_uint<15> > part_cfg_strm;
#pragma HLS stream variable = part_cfg_strm depth = 2
    hls::stream<ap_uint<36> > bf_cfg_strm;
#pragma HLS stream variable = bf_cfg_strm depth = 2

    hls::stream<ap_uint<32> > pu_begin_status_strm;
#pragma HLS stream variable = pu_begin_status_strm depth = 2
    hls::stream<ap_uint<32> > pu_end_status_strm;
#pragma HLS stream variable = pu_end_status_strm depth = 2
    hls::stream<ap_uint<3> > join_flag_strm;
#pragma HLS stream variable = join_flag_strm depth = 2

    // XXX: disable bypass function to avoid host misuse kernel hang
    // if (!bypass_on) {
    // write join type
    join_flag_strm.write(join_type);
    // write depth
    pu_begin_status_strm.write(31);
    // write join number
    pu_begin_status_strm.write(0);
    // write general cfg
    general_cfg_strm.write(general_cfg);
    // write part cfg
    part_cfg_strm.write(part_cfg);
    // write bf cfg
    bf_cfg_strm.write(bf_cfg);
    multi_func_pu_plus_adapter<COL_NM, CH_NM, PU_NM, ROUND_NM, GRP_SZ, HBM_OUTSTANDING>(
        mk_on, general_cfg_strm, join_flag_strm, part_cfg_strm, bf_cfg_strm, in_strm, e_in_strm, out_strm, out_nm_strm,
        hp_bkpu_strm, pu_begin_status_strm, pu_end_status_strm, htb_buf0, htb_buf1, htb_buf2, htb_buf3, htb_buf4,
        htb_buf5, htb_buf6, htb_buf7, stb_buf0, stb_buf1, stb_buf2, stb_buf3, stb_buf4, stb_buf5, stb_buf6, stb_buf7);
    // read depth
    ap_uint<32> st1 = pu_end_status_strm.read();
    // read join number
    ap_uint<32> st2 = pu_end_status_strm.read();
    // only probe stage we need the number of ouput rows
    if (bp_flag) {
        nr_row_strm.write(st2);
    }
#if !defined __SYNTHESIS__ && USER_DEBUG == 1
    printf("Hash join finished. status(%d, %d).\n", st1, st2);
#endif
    //}
}

// ---------------------------- Bypass Core -----------------------------------

// merge CH_NM inputs into 1 output
// work as bypass
// round-robin
template <int COL, int CH_NM>
void bypass_pu_wrapper(hls::stream<ap_uint<8> >& general_cfg_strm,
                       hls::stream<ap_uint<8 * TPCH_INT_SZ> > i_jrow_strm[CH_NM][COL],
                       hls::stream<bool> i_e_strm[CH_NM],
                       hls::stream<ap_uint<8 * TPCH_INT_SZ> > o_jrow_strm[COL],
                       hls::stream<bool>& o_e_strm) {
    ap_uint<8> general_cfg = general_cfg_strm.read();
    const int MAX = (1 << CH_NM) - 1;
    ap_uint<CH_NM> last = 0;
#if !defined __SYNTHESIS__ && USER_DEBUG == 1
    std::cout << "CH_NM=" << CH_NM << std::endl;
#endif
    bool bypass_on = general_cfg[0];
    ap_uint<CH_NM> id = 0;
    if (bypass_on) {
        do {
#pragma HLS pipeline II = 1
            for (int i = 0; i < CH_NM; i++) {
#pragma HLS unroll
                if (id == i && !i_e_strm[i].empty()) {
                    last[i] = i_e_strm[i].read();
                    for (int c = 0; c < COL; ++c) {
#pragma HLS unroll
                        ap_uint<8 * TPCH_INT_SZ> t = i_jrow_strm[i][c].read();
                        if (!last[i]) o_jrow_strm[c].write(t);
                    }
                    if (!last[i]) o_e_strm.write(false);
                }
            }

            if (id == CH_NM - 1)
                id = 0;
            else
                id++;
        } while (last != MAX);

        o_e_strm.write(true);
    }
}

} // namespace gqe
} // namespace database
} // namespace xf

#endif // GQE_ISV_MULTI_FUNC_PU_HPP
