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

#ifndef XF_DATA_ANALYTICS_TEXT_STRING_COMPARE_HPP
#define XF_DATA_ANALYTICS_TEXT_STRING_COMPARE_HPP

#include <stdint.h>
#include "ap_int.h"
#include "hls_stream.h"

#ifndef __SYNTHESIS__
#include <iostream>
#endif

/**
 * @file stringCompare.hpp
 * @brief provide a group of functions that compare the value of input string to the given base string character
 * pattern.
 *
 * This file is part of Vitis Data Analytics Library.
 */

namespace xf {
namespace data_analytics {
namespace text {

enum LIKE_OP { FOP_LK_ANY = 0, FOP_LK_BEG, FOP_LK_END };

namespace internal {
template <int _n>
inline bool orTree(bool flag[], int _o = 0) {
#pragma HLS inline
    return orTree<_n / 2>(flag, _o) || orTree<_n / 2>(flag, _o + (_n / 2));
}

template <>
inline bool orTree<2>(bool flag[], int _o) {
#pragma HLS inline
    return flag[_o] || flag[_o + 1];
}

template <>
inline bool orTree<1>(bool flag[], int _o) {
#pragma HLS inline
    return flag[_o];
}

template <int NUM_BASE_STR = 8, int MAX_BASE_STR_LEN = 64>
static void str_cmp(hls::stream<ap_uint<64> >& i_cfg_strm,
                    hls::stream<ap_uint<64> >& i_str_strm,
                    hls::stream<uint32_t>& i_len_strm,
                    hls::stream<bool>& i_e_strm,

                    hls::stream<bool>& o_valid_strm,
                    hls::stream<bool>& o_e_strm) {
    // buffer for base string
    ap_uint<64> base_len[NUM_BASE_STR];
#pragma HLS array_partition variable = base_len dim = 1
    ap_uint<64> base_str[NUM_BASE_STR][MAX_BASE_STR_LEN / 8];
#pragma HLS array_partition variable = base_str dim = 1

    // parsing base string
    ap_uint<64> nm_basestr = i_cfg_strm.read();
    for (int i = 0; i < NUM_BASE_STR; ++i) {
        ap_uint<64> tmp = i_cfg_strm.read();
        if (i < nm_basestr) {
            base_len[i] = tmp;
        } else {
            base_len[i] = 0xffffffff; // indicator for not-use.
        }
    }
    for (int i = 0; i < NUM_BASE_STR; ++i) {
        for (int j = 0; j < MAX_BASE_STR_LEN / 8; ++j) {
            base_str[i][j] = i_cfg_strm.read();
        }
    }

    bool equal[NUM_BASE_STR] = {true};
#pragma HLS array_partition variable = equal dim = 1
    uint32_t str_len = 0;
    uint32_t n_blk = 0;
    bool next_str = true;
    int idx = 0;
    bool valid = true;

    bool end = false;
    bool is_run = true;
    bool out = false;
    while (!end || is_run) {
#pragma HLS pipeline II = 1
        if (next_str) {
            if (i_e_strm.read_nb(end)) {
                if (!end) {
                    str_len = i_len_strm.read();
                    n_blk = str_len / sizeof(ap_uint<64>) + ((str_len % sizeof(ap_uint<64>)) > 0);
                    idx = 0;
                    for (int i = 0; i < NUM_BASE_STR; ++i) {
                        if (str_len == 0 && base_len[i] != 0) {
                            equal[i] = false;
                        } else {
                            equal[i] = true;
                        }
                    }
                    valid = true; // default
                    out = true;
                }
            }
        }

        if (idx < n_blk) {
            next_str = false;
            ap_uint<64> cur_str = i_str_strm.read();
            for (int i = 0; i < NUM_BASE_STR; ++i) {
#pragma HLS unroll
                uint32_t n_base_blk = base_len[i] / 8;
                if (base_len[i] == 0xffffffff) { // str_cmp_module[i] not used
                    equal[i] = false;
                } else if (base_len[i] != str_len) {
                    equal[i] = false;
                } else if (0 < base_len[i] && base_len[i] <= sizeof(ap_uint<64>)) {
                    if (cur_str.range(base_len[i] * 8 - 1, 0) != base_str[i][idx].range(base_len[i] * 8 - 1, 0)) {
                        equal[i] = false;
                    }
                } else if (base_len[i] > sizeof(ap_uint<64>) && idx < n_base_blk) {
                    if (cur_str != base_str[i][idx]) {
                        equal[i] = false;
                    }
                } else if (base_len[i] > sizeof(ap_uint<64>) && idx == n_base_blk) {
                    if (cur_str.range((base_len[i] - sizeof(ap_uint<64>) * n_base_blk) * 8 - 1, 0) !=
                        base_str[i][idx].range((base_len[i] - sizeof(ap_uint<64>) * n_base_blk) * 8 - 1, 0)) {
                        equal[i] = false;
                    }
                }
            }
            idx++;
        } else {
            valid = orTree<NUM_BASE_STR>(equal);

            if (out) {
                o_valid_strm.write(valid);
                o_e_strm.write(false);
                out = false;
            }
            next_str = true;
            if (end) {
                is_run = false;
            }
        }
    }
    o_e_strm.write(true);
}

static void set_flag(hls::stream<bool>& i_flag_strm,
                     hls::stream<bool>& i_e_strm,
                     bool not_flag,

                     hls::stream<bool>& o_flag_strm,
                     hls::stream<bool>& o_e_strm) {
    bool end = i_e_strm.read();
    while (!end) {
#pragma HLS pipeline II = 1
        bool res = i_flag_strm.read();
        if (not_flag) {
            o_flag_strm.write(!res);
        } else {
            o_flag_strm.write(res);
        }

        o_e_strm.write(false);
        end = i_e_strm.read();
    }
    o_e_strm.write(true);
}

template <int MAX_BASE_STR_LEN = 64, int BATCH_SZ = 2>
void preprocess(hls::stream<ap_uint<64> >& i_cfg_strm,
                hls::stream<uint32_t>& i_len_strm,
                hls::stream<bool>& i_e_strm,

                hls::stream<ap_uint<8> >& o_base_len_strm,
                hls::stream<ap_uint<64> >& o_mrg_strm,
                hls::stream<ap_uint<64> >& o_cfg_strm,
                hls::stream<bool>& o_e_strm) {
    ap_uint<16> len_cmp = 0;
    ap_uint<16> n_cmp_blk = 0;
    ap_uint<16> n_shift = 0;
    ap_uint<16> n_read = 0;

    ap_uint<8> len_base = 0; // base_len_strm.read();
    for (int i = 0; i < MAX_BASE_STR_LEN / 8 + 2; i++) {
#pragma HLS pipeline II = 1
        ap_uint<64> cfg = i_cfg_strm.read();
        if (i == 1) {
            len_base = cfg;
        }
        o_cfg_strm.write(cfg);
    }
    o_base_len_strm.write(len_base);

    bool end = false;
LOOP_PREPROCESS:
    while (!end) {
#pragma HLS pipeline II = 1
        if (i_e_strm.read_nb(end)) {
            if (!end) {
                len_cmp = i_len_strm.read();
                n_cmp_blk = len_cmp / 8 + (len_cmp % 8 > 0);
                n_read = (n_cmp_blk <= (ap_uint<16>)(MAX_BASE_STR_LEN / 8)) ? n_cmp_blk
                                                                            : (ap_uint<16>)(MAX_BASE_STR_LEN / 8);

                // num of shift to complete searching one input string
                if (len_cmp < len_base || len_cmp == 0) {
                    n_shift = 0;
                } else {
                    n_shift = (len_cmp - len_base) / BATCH_SZ + 1; // ((len_cmp - len_base) % BATCH_SZ > 0);
                }
                ap_uint<64> out = 0;
                out.range(15, 0) = len_cmp;
                out.range(31, 16) = n_cmp_blk;
                out.range(47, 32) = n_shift;
                out.range(63, 48) = n_read;
                o_mrg_strm.write(out);
                o_e_strm.write(false);
            }
        }
    }
    o_e_strm.write(true);
}

template <int MAX_BASE_STR_LEN = 64, int BATCH_SZ = 2>
void gen_cmpstr(hls::stream<ap_uint<64> >& i_str_strm,
                hls::stream<ap_uint<64> >& i_mrg_strm,
                hls::stream<bool>& i_e_strm,
                hls::stream<ap_uint<8> >& base_len_strm,

                hls::stream<ap_uint<8 * (BATCH_SZ + MAX_BASE_STR_LEN)> >& o_cmpstr_strm,
                hls::stream<int>& o_cmp_len_strm,
                hls::stream<bool>& o_str_end_strm,
                hls::stream<bool>& o_e_strm) {
    ap_uint<8 * (BATCH_SZ + MAX_BASE_STR_LEN)> cmp_str = 0;
    ap_uint<64> tmp_read = 0;

    ap_uint<16> len_cmp = 0;
    ap_uint<16> n_cmp_blk = 0;

    ap_uint<16> n_shift = 0;
    ap_uint<16> curr_shift = 0;

    ap_uint<4> n_batch = 8 / BATCH_SZ;
    ap_uint<4> curr_batch = 0;

    ap_uint<4> n_read = 0;
    ap_uint<4> curr_read = 0;

    bool next_str = true;
    ap_uint<8> len_base = base_len_strm.read();

    bool end = false;
    bool is_run = true; // control signal for running last blk;
    bool out = false;

LOOP_GEN_CMPSTR:
    while (!end || is_run) {
#pragma HLS pipeline II = 1
        // start processing next input and initialize variables
        if (next_str) {
            if (i_e_strm.read_nb(end)) {
                if (!end) {
                    ap_uint<64> mrg_in = i_mrg_strm.read();
                    len_cmp = mrg_in.range(15, 0);
                    n_cmp_blk = mrg_in.range(31, 16);
                    n_read = mrg_in.range(63, 48);
                    n_shift = mrg_in.range(47, 32);
                    curr_read = 0;
                    curr_shift = 0;
                    curr_batch = n_batch;
                    cmp_str = 0;
                    next_str = false;
                    out = true;
                } else {
                    out = false; // seems useless, to be verified.
                }
            }
        }

        if (curr_read < n_read) {
            cmp_str.range(curr_read * 64 + 63, curr_read * 64) = i_str_strm.read();
            curr_read++;
            n_cmp_blk--;
        } else {
            if (curr_shift < n_shift) {
                // fill high BATCH_SZ*8 bits with input string or 0, and shift
                if (n_cmp_blk != 0) {
                    if (curr_batch == n_batch) {
                        curr_batch = 0;
                        tmp_read = i_str_strm.read();
                        n_cmp_blk--;
                    }
                    cmp_str.range(8 * (MAX_BASE_STR_LEN + BATCH_SZ) - 1, 8 * MAX_BASE_STR_LEN) =
                        tmp_read.range(8 * BATCH_SZ * curr_batch + 8 * BATCH_SZ - 1, 8 * BATCH_SZ * curr_batch);
                    curr_batch++;
                } else if (n_cmp_blk == 0 && curr_batch < n_batch) {
                    cmp_str.range(8 * (MAX_BASE_STR_LEN + BATCH_SZ) - 1, 8 * MAX_BASE_STR_LEN) =
                        tmp_read.range(8 * BATCH_SZ * curr_batch + 8 * BATCH_SZ - 1, 8 * BATCH_SZ * curr_batch);
                    curr_batch++;
                } else {
                    cmp_str.range(8 * (MAX_BASE_STR_LEN + BATCH_SZ) - 1, 8 * MAX_BASE_STR_LEN) = 0;
                }
                // write out
                o_cmpstr_strm.write(cmp_str);
                o_cmp_len_strm.write(len_cmp);

                if (curr_shift == (n_shift - 1)) {
                    o_str_end_strm.write(true);
                } else {
                    o_str_end_strm.write(false);
                }
                o_e_strm.write(false);

                // shift
                for (int s = 0; s < MAX_BASE_STR_LEN / BATCH_SZ; s++) {
#pragma HLS unroll
                    cmp_str.range(s * 8 * BATCH_SZ + 8 * BATCH_SZ - 1, s * 8 * BATCH_SZ) =
                        cmp_str.range(s * 8 * BATCH_SZ + 16 * BATCH_SZ - 1, s * 8 * BATCH_SZ + 8 * BATCH_SZ);
                }

                curr_shift++;
            } else {
                if (n_shift == 0 && out) {
                    o_cmpstr_strm.write(0);
                    o_cmp_len_strm.write(len_cmp);
                    o_str_end_strm.write(true);
                    o_e_strm.write(false);
                    out = false;
                }
                next_str = true;
                if (end) {
                    is_run = false;
                }
            }
        }
    }
    o_e_strm.write(true);
}

template <int MAX_BASE_STR_LEN = 64, int BATCH_SZ = 2>
void compare_core(hls::stream<ap_uint<64> >& i_cfg_strm,
                  hls::stream<ap_uint<8 * (BATCH_SZ + MAX_BASE_STR_LEN)> >& i_cmpstr_strm,
                  hls::stream<int>& i_cmp_len_strm,
                  hls::stream<bool>& i_str_end_strm,
                  hls::stream<bool>& i_e_strm,

                  hls::stream<bool>& o_valid_strm,
                  hls::stream<bool>& o_e_strm) {
    bool any_hit = false;
    bool beg_hit = false;
    bool end_hit = false;
    bool str_end = true;

    bool find[BATCH_SZ] = {false};
#pragma HLS array_partition variable = find dim = 0

    ap_uint<8> len_base = 0;
    ap_uint<8 * MAX_BASE_STR_LEN> base_str = 0;
    ap_uint<8> op = 0;
    for (int i = 0; i < MAX_BASE_STR_LEN / 8 + 2; i++) {
#pragma HLS pipeline II = 1
        ap_uint<64> cfg = i_cfg_strm.read();
        if (i == 0) {
            op = cfg.range(7, 0);
        } else if (i == 1) {
            len_base = cfg.range(7, 0);
        } else {
            base_str.range((i - 2) * 64 + 63, (i - 2) * 64) = cfg;
        }
    }

    bool end = false;
LOOP_COMPARE_CORE:
    while (!end) {
#pragma HLS pipeline II = 1
        if (i_e_strm.read_nb(end)) {
            if (!end) {
                ap_uint<8 * (BATCH_SZ + MAX_BASE_STR_LEN)> cmp_str = i_cmpstr_strm.read();
                int len_cmp = i_cmp_len_strm.read();
                if (len_base == 0) {
                    str_end = i_str_end_strm.read();
                    any_hit = true;
                    beg_hit = true;
                    end_hit = true;
                } else {
                    for (int n = 0; n < BATCH_SZ; n++) {
#pragma HLS unroll
                        // if the actual character in base string doesn't match with compare
                        // string, set the flag to false
                        if (base_str.range(8 * len_base - 1, 0) != cmp_str.range(8 * n + 8 * len_base - 1, 8 * n)) {
                            find[n] = false;
                            // otherwise, set them all true
                        } else {
                            find[n] = true;
                        }
                    }
                    // the or-tree for II = 1
                    bool has_hit = orTree<BATCH_SZ>(find);

                    bool is_beg = str_end;
                    str_end = i_str_end_strm.read();
                    bool is_end = str_end;

                    if (has_hit) {
                        any_hit = true;

                        if (is_beg && find[0]) {
                            beg_hit = true;
                        }

                        int end_idx = (len_cmp - len_base) % BATCH_SZ;
                        if (is_end && find[end_idx]) {
                            end_hit = true;
                        }
                    }
                }

                if (str_end) {
                    // write string comparison result
                    if (op == FOP_LK_ANY) {
                        o_valid_strm.write(any_hit);
                    } else if (op == FOP_LK_BEG) {
                        o_valid_strm.write(beg_hit);
                    } else if (op == FOP_LK_END) {
                        o_valid_strm.write(end_hit);
                    } else {
                        o_valid_strm.write(true); // shouldn't enter this
                    }
                    o_e_strm.write(false);

                    // reset next string variables
                    for (int i = 0; i < BATCH_SZ; ++i) {
                        find[i] = false;
                    }
                    any_hit = false;
                    beg_hit = false;
                    end_hit = false;
                }
            }
        }
    }
    o_e_strm.write(true);
}

} // end namespace internal

/**
 * @brief performs string comparison and check if two strings are equal or not.
 *
 * @tparam MAX_BASE_STR_LEN max supported length of base string
 * @param i_cfg_strm base string configuration stream
 * @param i_str_strm input string stream
 * @param i_len_strm input string length stream
 * @param i_e_strm end flag stream for input
 * @param not_flag op: 1 - output true if string are not equal; 0 - output true if strings are equal
 * @param o_valid_strm compare result output stream
 * @param o_e_strm end flag stream for output
 */
template <int MAX_BASE_STR_LEN = 64>
static void string_equal(hls::stream<ap_uint<64> >& i_cfg_strm,
                         hls::stream<ap_uint<64> >& i_str_strm,
                         hls::stream<uint32_t>& i_len_strm,
                         hls::stream<bool>& i_e_strm,
                         bool not_flag,

                         hls::stream<bool>& o_valid_strm,
                         hls::stream<bool>& o_e_strm) {
#pragma HLS dataflow
    hls::stream<bool> valid_strm;
#pragma HLS STREAM variable = valid_strm depth = 4
#pragma HLS BIND_STORAGE variable = valid_strm type = FIFO impl = SRL
    hls::stream<bool> e_strm;
#pragma HLS STREAM variable = e_strm depth = 4
#pragma HLS BIND_STORAGE variable = e_strm type = FIFO impl = SRL
    internal::str_cmp<1, MAX_BASE_STR_LEN>(i_cfg_strm, i_str_strm, i_len_strm, i_e_strm, valid_strm, e_strm);
    internal::set_flag(valid_strm, e_strm, not_flag, o_valid_strm, o_e_strm);
}

/**
 * @brief determines if the input string matches any value in a string list or not
 *
 * @tparam NUM_BASE_STR max supported number of elements in the list
 * @tparam MAX_BASE_STR_LEN max supported length of base string
 * @param i_cfg_strm base string configuration stream
 * @param i_str_strm input string stream
 * @param i_len_strm input string length stream
 * @param i_e_strm end flag stream for input
 * @param not_flag op: 1 - output true if not matched; 0 - output true if matched.
 * @param o_valid_strm compare result output stream
 * @param o_e_strm end flag stream for output
 */
template <int NUM_BASE_STR = 8, int MAX_BASE_STR_LEN = 64>
static void string_in(hls::stream<ap_uint<64> >& i_cfg_strm,
                      hls::stream<ap_uint<64> >& i_str_strm,
                      hls::stream<uint32_t>& i_len_strm,
                      hls::stream<bool>& i_e_strm,
                      bool not_flag,

                      hls::stream<bool>& o_valid_strm,
                      hls::stream<bool>& o_e_strm) {
#pragma HLS dataflow
    hls::stream<bool> valid_strm;
#pragma HLS STREAM variable = valid_strm depth = 4
#pragma HLS BIND_STORAGE variable = valid_strm type = FIFO impl = SRL
    hls::stream<bool> e_strm;
#pragma HLS STREAM variable = e_strm depth = 4
#pragma HLS BIND_STORAGE variable = e_strm type = FIFO impl = SRL
    internal::str_cmp<NUM_BASE_STR, MAX_BASE_STR_LEN>(i_cfg_strm, i_str_strm, i_len_strm, i_e_strm, valid_strm, e_strm);
    internal::set_flag(valid_strm, e_strm, not_flag, o_valid_strm, o_e_strm);
}

/**
 * @brief determines whether the input string matches a specific pattern that includes wildcard charater %.
 * '%abc%' match strings that have 'abc' in any position
 * 'abc%' match strings that start with 'abc'
 * '%abc' match strings that end with 'abc'
 *
 * @tparam MAX_BASE_STR_LEN max supported length of base string
 * @tparam BATCH_SZ number of bytes simutaneously processed
 * @param i_cfg_strm base string configuration stream
 * @param i_str_strm input string stream
 * @param i_len_strm input string length stream
 * @param i_e_strm end flag stream for input
 * @param not_flag op: 1 - output true if not matched; 0 - output true if matched.
 * @param o_valid_strm compare result output stream
 * @param o_e_strm end flag stream for output
 */
template <int MAX_BASE_STR_LEN = 64, int BATCH_SZ = 2>
static void string_like(hls::stream<ap_uint<64> >& i_cfg_strm,
                        hls::stream<ap_uint<64> >& i_str_strm,
                        hls::stream<uint32_t>& i_len_strm,
                        hls::stream<bool>& i_e_strm,
                        bool not_flag,

                        hls::stream<bool>& o_valid_strm,
                        hls::stream<bool>& o_e_strm) {
#pragma HLS dataflow
    hls::stream<ap_uint<64> > pre_mrg_strm;
#pragma HLS stream variable = pre_mrg_strm depth = 8
#pragma HLS bind_storage variable = pre_mrg_strm type = FIFO impl = SRL
    hls::stream<bool> pre_e_strm;
#pragma HLS stream variable = pre_e_strm depth = 8
#pragma HLS bind_storage variable = pre_e_strm type = FIFO impl = SRL
    hls::stream<ap_uint<8> > pre_len_strm;
#pragma HLS stream variable = pre_len_strm depth = 8
#pragma HLS bind_storage variable = pre_len_strm type = FIFO impl = SRL
    hls::stream<ap_uint<64> > o_cfg_strm;
#pragma HLS STREAM variable = o_cfg_strm depth = MAX_BASE_STR_LEN / 8 + 2
#pragma HLS BIND_STORAGE variable = o_cfg_strm type = FIFO impl = SRL

    hls::stream<ap_uint<8 * (BATCH_SZ + MAX_BASE_STR_LEN)> > cmpstr_strm;
#pragma HLS stream variable = cmpstr_strm depth = 8
#pragma HLS bind_storage variable = cmpstr_strm type = FIFO impl = SRL
    hls::stream<int> cmp_len_strm;
#pragma HLS stream variable = cmp_len_strm depth = 8
#pragma HLS bind_storage variable = cmp_len_strm type = FIFO impl = SRL
    hls::stream<bool> str_end_strm;
#pragma HLS stream variable = str_end_strm depth = 8
#pragma HLS bind_storage variable = str_end_strm type = FIFO impl = SRL
    hls::stream<bool> end_strm;
#pragma HLS stream variable = end_strm depth = 8
#pragma HLS bind_storage variable = end_strm type = FIFO impl = SRL

    hls::stream<bool> valid_strm;
#pragma HLS STREAM variable = valid_strm depth = 4
#pragma HLS BIND_STORAGE variable = valid_strm type = FIFO impl = SRL
    hls::stream<bool> e_strm;
#pragma HLS STREAM variable = e_strm depth = 4
#pragma HLS BIND_STORAGE variable = e_strm type = FIFO impl = SRL

    internal::preprocess<MAX_BASE_STR_LEN, BATCH_SZ>(i_cfg_strm, i_len_strm, i_e_strm, pre_len_strm, pre_mrg_strm,
                                                     o_cfg_strm, pre_e_strm);

    internal::gen_cmpstr<MAX_BASE_STR_LEN, BATCH_SZ>(i_str_strm, pre_mrg_strm, pre_e_strm, pre_len_strm, cmpstr_strm,
                                                     cmp_len_strm, str_end_strm, end_strm);

    internal::compare_core<MAX_BASE_STR_LEN, BATCH_SZ>(o_cfg_strm, cmpstr_strm, cmp_len_strm, str_end_strm, end_strm,
                                                       valid_strm, e_strm);

    internal::set_flag(valid_strm, e_strm, not_flag, o_valid_strm, o_e_strm);
}

} // end namespace text
} // end namespace data_analytics
} // end namespace xf

#endif // end XF_DATA_ANALYTICS_TEXT_STRING_COMPARE_HPP