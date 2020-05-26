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

/**
 * @file scan_table.hpp
 * @brief scan raw data to multiple stream channel, template implementation,
 * targeting DDR devices.
 *
 *
 * This file is part of Vitis Database Library
 */

#ifndef GQE_SCAN_TABLE_HPP
#define GQE_SCAN_TABLE_HPP

#ifndef __SYNTHESIS__
#include <iostream>
#endif

#include <ap_int.h>
#include <hls_stream.h>

#include "xf_database/scan_cmp_str_col.hpp"
#include "xf_database/types.hpp"
#include "xf_database/utils.hpp"

namespace xf {
namespace database {
namespace gqe {

/**
 * @brief burst read ddr and transform col data into stream
 *
 * @tparam _NCol number of column in total
 *
 * @param buf_in pointer of DDR memory
 * @param config configuration context
 * @param out_strm[_NCol] stream for non-string column
 * @param cnst_strm constant string stream
 * @param pact_strm stream for string column
 * @param e_strm_o end flag stream for string stream
 * @param nrow_strm_spliter number of row in one column
 * @param nrow_strm_filter number of row in one column
 */
template <int _NCol>
void _read_to_colvec(ap_uint<512>* buf_in,
                     hls::stream<ap_int<64> >& config,

                     hls::stream<ap_uint<512> > out_strm[_NCol],
                     hls::stream<ap_uint<512> > cnst_strm[2],
                     hls::stream<ap_uint<64> > mask_cnst_strm[2],
                     hls::stream<bool> inv_ctr_strm[2],
                     hls::stream<bool> e_cnst_strm[2],
                     hls::stream<ap_uint<512> > pact_strm[2],
                     hls::stream<bool> e_pact_strm[2],
                     hls::stream<ap_uint<4> > num_cnst_str_filter[2],
                     hls::stream<int> nrow_strm_filter[2],
                     hls::stream<int>& nrow_strm_spliter,
                     hls::stream<ap_uint<2> >& num_str_col_strm_spliter) {
    // fixed burst length
    const int burst_len = BURST_LEN;
    // first heading
    ap_uint<512> bw = buf_in[0];

    // number of row in each column
    int nrow = bw.range(31, 0);
    nrow_strm_spliter.write(nrow); // tells spliter

    /***********************Config parser********************************/
    ap_int<64> ccol_id = config.read();
    ap_int<64> scol_id = config.read();
    ap_int<4> scol0 = scol_id.range(3, 0);
    ap_int<4> scol1 = scol_id.range(7, 4);
    int scol0_int = scol0.to_int();
    int scol1_int = scol1.to_int();
    bool is_str0 = (scol0_int != -1);
    bool is_str1 = (scol1_int != -1);
    bool is_str01 = is_str0 || is_str1;

    if (is_str0)
        nrow_strm_filter[0].write(nrow); // tells filter
    else
        nrow_strm_filter[0].write(0);
    if (is_str1)
        nrow_strm_filter[1].write(nrow); // tells filter
    else
        nrow_strm_filter[1].write(0);

    ap_int<2> num_str_col = 0; // number of string column
    if (is_str01) {
        if (is_str0 && is_str1)
            num_str_col = 2;
        else
            num_str_col = 1;
    }
    num_str_col_strm_spliter.write(num_str_col);

    ap_int<64> cnst_cfg = 0;
    if (is_str01) cnst_cfg = config.read();

    ap_int<4> num_cnst_str[2] = {cnst_cfg(3, 0), cnst_cfg(7, 4)}; // number of constant string for each string column

    num_cnst_str_filter[0] << num_cnst_str[0];
    num_cnst_str_filter[1] << num_cnst_str[1];

// fetch and output constant string
config_parser_main_loop:
    for (int i = 0; i < (num_cnst_str[0] + num_cnst_str[1]); i++) {
        const ap_int<8> cnst_len = scol_id(8 * i + 15, 8 * i + 8);
        const ap_int<8> cnst_cfg_item = cnst_cfg(8 * i + 15, 8 * i + 8);

        const int read_config_num = cnst_len(5, 0) / 8 + 1;
        ap_uint<512> cnst_string = 0;
        for (int j = 0; j < read_config_num; j++) {
#pragma HLS PIPELINE II = 1
            ap_uint<64> t = config.read();
            cnst_string.range(511, 448) = t;
            if (j != (read_config_num - 1)) cnst_string >>= 64;
        }

        ap_uint<64> mask_t;
        if (cnst_cfg_item(5, 0) > 0) {
            mask_t = -1;                    // 0-compare, 1-masked
            for (int k = 1; k <= 63; k++) { // check resource usage
#pragma HLS unroll
                if (k <= cnst_cfg_item(5, 0)) mask_t[63 - k] = 0;
            }
        } else
            mask_t = 0;
#if !defined __SYNTHESIS__ && XDEBUG == 1
        std::cout << "Mask-" << i << ":" << std::hex << mask_t << std::dec << std::endl;
#endif // !defined __SYNTHESIS__ && XDEBUG == 1

        mask_cnst_strm[cnst_cfg_item[7]] << mask_t;
        inv_ctr_strm[cnst_cfg_item[7]] << cnst_cfg_item[6];
        cnst_strm[cnst_cfg_item[7]] << cnst_string;
        e_cnst_strm[cnst_cfg_item[7]] << false;
    }
    for (int i = 0; i < 2; i++) {
#pragma HLS PIPELINE II = 1
        e_cnst_strm[i] << true;
    }

    /***********************Config parser end********************************/

    // calculate address and access times
    int c_idx = 0, cc_idx = 0;
    int col_t[_NCol], nrd_t[_NCol];
#pragma HLS ARRAY_PARTITION variable = col_t complete dim = 1
#pragma HLS ARRAY_PARTITION variable = nrd_t complete dim = 1
    col_t[0] = 1;
    nrd_t[0] = bw.range(63, 32).to_int();
    int common_nread = bw.range(63, 32).to_int();
    int str_col_nread = bw.range(63, 32).to_int();
    for (int i = 1; i < _NCol; i++) {
        c_idx += nrd_t[i - 1] + 1;
#if !defined __SYNTHESIS__ && XDEBUG == 1
        std::cout << "c_idx:" << c_idx << std::endl;
#endif // !defined __SYNTHESIS__ && XDEBUG == 1
        // incontinuous single read
        ap_uint<512> buf_t = buf_in[c_idx];
        nrd_t[i] = buf_t.range(63, 32);
#if !defined __SYNTHESIS__ && XDEBUG == 1
        std::cout << "nrd_t[" << i << "]:" << nrd_t[i] << std::endl;
#endif // !defined __SYNTHESIS__ && XDEBUG == 1
        col_t[i] = c_idx + 1;
        ap_int<8> cid_t = ccol_id(8 * i + 7, 8 * i);
        if (cid_t >= 0) {
            if ((i != scol0_int) && (i != scol1_int))
                cc_idx = i;
            else // find larger string column
                str_col_nread = str_col_nread > nrd_t[i] ? str_col_nread : nrd_t[i];
        }
    }

    common_nread = nrd_t[cc_idx];

    // calculate the ratio of one burst read length between string and non-string
    // column
    const int burst_ratio = (str_col_nread + common_nread - 1) / common_nread; // the ratio of burst times, ceiling
    const int str_col_inc = burst_ratio * burst_len;
#if !defined __SYNTHESIS__ && XDEBUG == 1
    std::cout << "str_col_nread/common_nread:" << str_col_nread << "/" << common_nread << " = " << burst_ratio
              << std::endl;
#endif // !defined __SYNTHESIS__ && XDEBUG == 1

    // re-order, ensure that read string column firstly if exists
    ap_uint<8> _cid_0 = ccol_id.range(7, 0);
    int _cid_0_int = _cid_0.to_int();
    if (is_str0 && (_cid_0_int != -1) && (scol0_int != _cid_0_int)) {
        int _c_t = col_t[_cid_0_int];
        int _n_t = nrd_t[_cid_0_int];
        col_t[_cid_0_int] = col_t[scol0_int];
        nrd_t[_cid_0_int] = nrd_t[scol0_int];
        col_t[scol0_int] = _c_t;
        nrd_t[scol0_int] = _n_t;
    }
    // and then the second string column exchange
    ap_uint<8> _cid_1 = is_str0 ? ccol_id.range(15, 8) : ccol_id.range(7, 0);
    int _cid_1_int = _cid_1.to_int();
    if (is_str1 && (_cid_1_int != -1) && (scol1_int != _cid_1_int)) {
        int _c_t = col_t[_cid_1_int];
        int _n_t = nrd_t[_cid_1_int];
        col_t[_cid_1_int] = col_t[scol1_int];
        nrd_t[_cid_1_int] = nrd_t[scol1_int];
        col_t[scol1_int] = _c_t;
        nrd_t[scol1_int] = _n_t;
    }

    // remap to physical addr
    int col_offset[_NCol], nread[_NCol];
#pragma HLS ARRAY_PARTITION variable = col_offset complete dim = 1
#pragma HLS ARRAY_PARTITION variable = nread complete dim = 1
    for (int c = 0; c < _NCol; c++) {
#pragma HLS unroll
        ap_int<8> cid_t = ccol_id.range(8 * c + 7, 8 * c);
        int cid = cid_t.to_int();
        if (cid == -1) {
            col_offset[c] = -1;
            nread[c] = common_nread;
        } else {
            col_offset[c] = col_t[cid];
            nread[c] = nrd_t[cid];
        }
    }

#if !defined __SYNTHESIS__ && XDEBUG == 1
    std::cout << "nrow:" << nrow << std::endl;
    for (int i = 0; i < _NCol; i++) {
        std::cout << "col_offset[" << i << "]=" << col_offset[i] << std::endl;
        std::cout << "nread[" << i << "]=" << nread[i] << std::endl;
    }
    std::cout << "rburst_len:" << burst_len << ", common_nread:" << common_nread << ", str_col0:" << scol0_int
              << ", str_col1:" << scol1_int << std::endl;
#endif // !defined __SYNTHESIS__ && XDEBUG == 1

    // begin DDR read process
    int str_row_cnt0 = 0, str_row_cnt1 = 0;
    int base_addr = 0, len_wrapper = 0;
    // bool pact_end[2] = {true, true};
    for (int i = 0; i < common_nread; i += burst_len) {
        // do a burst read for each col
        for (int c = 0; c < _NCol; c++) {
            bool is_str_col0 = is_str0 && (c == 0);
            bool is_str_col1 = is_str0 ? (is_str1 && (c == 1)) : (is_str1 && (c == 0));
            int offset = col_offset[c];
            int cnread = nread[c];
            if (is_str_col0)
                len_wrapper = ((str_row_cnt0 + str_col_inc) > cnread) ? (cnread - str_row_cnt0) : str_col_inc;
            else if (is_str_col1)
                len_wrapper = ((str_row_cnt1 + str_col_inc) > cnread) ? (cnread - str_row_cnt1) : str_col_inc;
            else
                len_wrapper = ((i + burst_len) > cnread) ? (cnread - i) : burst_len;

            if (offset == -1) {
                // feed dummy data into col
                for (int j = 0; j < len_wrapper; ++j) {
#pragma HLS PIPELINE II = 1
                    ap_uint<512> t = 0;
                    out_strm[c].write(t);
                }
            } else {
                for (int w = 0; w < len_wrapper; w += burst_len) {
                    const int len = ((w + burst_len) > len_wrapper) ? (len_wrapper - w) : burst_len;
                    if (is_str_col0) {
                        base_addr = str_row_cnt0;
                        str_row_cnt0 += len;
                    } else if (is_str_col1) {
                        base_addr = str_row_cnt1;
                        str_row_cnt1 += len;
                    } else {
                        base_addr = i;
                    }
#if !defined __SYNTHESIS__ && XDEBUG == 1
                    std::cout << "c:" << c << ", len:" << len << ", w:" << w << ", len_wrapper:" << len_wrapper
                              << ", base_addr:" << base_addr << std::endl;
#endif // !defined __SYNTHESIS__ && XDEBUG == 1
                    // one burst read for col
                    for (int j = 0; j < len; ++j) {
#pragma HLS PIPELINE II = 1
                        // std::cout << "Read Address:" << offset + base_addr + j <<
                        // std::endl;
                        ap_uint<512> t = buf_in[offset + base_addr + j];
                        if (is_str_col0) {
                            pact_strm[0].write(t.reverse());
                            e_pact_strm[0].write(false);
                        } else if (is_str_col1) {
                            pact_strm[1].write(t.reverse());
                            e_pact_strm[1].write(false);
                        } else {
                            out_strm[c].write(t);
                        }
                    } // end one burst read
                }     // end multiple burst
            }         // end valid column read
        }             // end all column read loop
    }                 // end read

    for (int c = 0; c < 2; c++) e_pact_strm[c].write(true);
}

/**
 * @brief gather string compare for following string filter
 *
 * @tparam _CH number of channel
 *
 * @param nrow_strm number of row
 * @param is_equal result of string compare
 * @param e_equal_strm end flag stream for is_equal
 * @param out_strm vector of compared result for each channel
  */
template <int _CH, int _Nin>
void _gather_str_cmp(hls::stream<int>& nrow_strm,
                     hls::stream<ap_uint<4> >& num_str_strm,
                     hls::stream<bool> is_equal[_Nin],
                     hls::stream<bool> e_equal_strm[_Nin],
                     hls::stream<ap_uint<_CH> >& out_strm,
                     hls::stream<bool>& e_out_strm) {
    ap_uint<_CH> bo = 0;
    int bo_idx = 0;
    int counter = 0;
    int nrow = nrow_strm.read();
    ap_uint<4> max = num_str_strm.read();
    bool is_end[_Nin];
    for (int c = 0; c < _Nin; c++) {
#pragma HLS unroll
        is_end[c] = e_equal_strm[c].read();
    }

    bool cond = (max == 0) ? true : false;
    for (int c = 0; c < _Nin; c++) {
#pragma HLS unroll
        if (c < max) cond |= is_end[c];
    }

gather_main_loop:
    while (!cond) {
#pragma HLS PIPELINE II = 1
        ap_uint<_Nin> res = 0;
        for (int c = 0; c < _Nin; c++) {
#pragma HLS unroll
            if (c < max) {
                res[c] = is_equal[c].read();
                is_end[c] = e_equal_strm[c].read();
                cond |= is_end[c];
            }
        }
        bo[bo_idx] = (res > 0); // OR Rule for SQL 'IN'
#if !defined __SYNTHESIS__ && XDEBUG == 1
        std::cout << std::dec << "counter:" << counter << ",Equal:" << res << ",bo_idx:" << bo_idx << std::endl;
#endif // !defined __SYNTHESIS__ && XDEBUG == 1
        bo_idx++;
        counter++;

        if ((bo_idx == _CH) || (counter == nrow)) {
            out_strm << bo;
            e_out_strm << false;
            bo = 0;
            bo_idx = 0;
        }
    }

    e_out_strm << true;
}

template <int _NDup, typename T>
void _dup_stream(hls::stream<T>& i_strm, hls::stream<T> o_dup_strm[_NDup]) {
    T t = i_strm.read();
    for (int i = 0; i < _NDup; i++) {
#pragma HLS unroll
        o_dup_strm[i] << t;
    }
}

template <int _NDup, typename T>
void _dup_stream_pair(hls::stream<T>& i_strm,
                      hls::stream<bool>& e_i_strm,
                      hls::stream<ap_uint<4> >& max_act_num,
                      hls::stream<T> o_dup_strm[_NDup],
                      hls::stream<bool> e_o_dup_strm[_NDup]) {
    const ap_uint<4> max = max_act_num.read();
    for (int i = 0; i < _NDup; i++) {
#pragma HLS unroll
        if (i >= max) e_o_dup_strm[i] << true;
    }

    bool is_end = e_i_strm.read();
    while (!is_end) {
#pragma HLS PIPELINE II = 1
        T t = i_strm.read();
        e_i_strm >> is_end;
        for (int i = 0; i < _NDup; i++) {
#pragma HLS unroll
            if (i < max) {
                o_dup_strm[i] << t;
                e_o_dup_strm[i] << false;
            }
        }
    }

    for (int i = 0; i < _NDup; i++) {
#pragma HLS unroll
        if (i < max) e_o_dup_strm[i] << true;
    }
}

template <int _NSca, typename T0, typename T1, typename T2>
void _split_three_stream(hls::stream<T0>& i_strm0,
                         hls::stream<T1>& i_strm1,
                         hls::stream<T2>& i_strm2,
                         hls::stream<bool>& e_i_strm,
                         hls::stream<T0> o_scatter_strm0[_NSca],
                         hls::stream<T1> o_scatter_strm1[_NSca],
                         hls::stream<T2> o_scatter_strm2[_NSca]) {
    int index = 0;
    bool is_end = e_i_strm.read();
    while (!is_end) {
#pragma HLS PIPELINE II = 1
        T0 t0 = i_strm0.read();
        T1 t1 = i_strm1.read();
        T2 t2 = i_strm2.read();
        if (index < _NSca) {
            o_scatter_strm0[index] << t0;
            o_scatter_strm1[index] << t1;
            o_scatter_strm2[index] << t2;
        }
        index++;
        e_i_strm >> is_end;
    }

    for (int i = index; i < _NSca; i++) {
#pragma HLS PIPELINE II = 1
        o_scatter_strm0[i] << 0;
        o_scatter_strm1[i] << 0;
        o_scatter_strm2[i] << 0;
    }
}

/**
 * @brief parse input string stream, and then transform compare result into
 * vector for each channel
 *
 * @tparam _CH number of channel
 *
 * @param nstr_row_strm number of row in string column
 * @param pact_stream input stream for non-string column
 * @param e_strm end flag for string column
 * @param cnst_strm constant string stream
 * @param out_strm vector of compared result for each channel
 */
template <int _CH>
void _proc_filter(hls::stream<int> nrow_strm[2],
                  hls::stream<ap_uint<4> > num_cnst_str[2],
                  hls::stream<ap_uint<512> > pact_stream[2],
                  hls::stream<bool> e_pact_strm[2],
                  hls::stream<ap_uint<512> > cnst_strm[2],
                  hls::stream<ap_uint<64> > mask_cnst_strm[2],
                  hls::stream<bool> inv_ctr_strm[2],
                  hls::stream<bool> e_cnst_strm[2],

                  hls::stream<ap_uint<_CH> > out_strm[2],
                  hls::stream<bool> e_out_strm[2]) {
#pragma HLS INLINE off
#pragma HLS DATAFLOW
    enum { _NCnst0 = 1, _NCnst1 = 7 };

    hls::stream<int> dup_row_strm_c0[2];
#pragma HLS STREAM variable = dup_row_strm_c0 depth = 2
#pragma HLS RESOURCE variable = dup_row_strm_c0 core = FIFO_LUTRAM
    hls::stream<int> dup_row_strm_c1[2];
#pragma HLS STREAM variable = dup_row_strm_c1 depth = 2
#pragma HLS RESOURCE variable = dup_row_strm_c1 core = FIFO_LUTRAM

    hls::stream<ap_uint<4> > dup_num_cnst_str_c0[2];
#pragma HLS STREAM variable = dup_num_cnst_str_c0 depth = 2
#pragma HLS RESOURCE variable = dup_num_cnst_str_c0 core = FIFO_LUTRAM
    hls::stream<ap_uint<4> > dup_num_cnst_str_c1[2];
#pragma HLS STREAM variable = dup_num_cnst_str_c1 depth = 2
#pragma HLS RESOURCE variable = dup_num_cnst_str_c1 core = FIFO_LUTRAM

    hls::stream<ap_uint<512> > dup_cnst_str_strm_c0[_NCnst0];
#pragma HLS STREAM variable = dup_cnst_str_strm_c0 depth = 2
#pragma HLS RESOURCE variable = dup_cnst_str_strm_c0 core = FIFO_LUTRAM
    hls::stream<ap_uint<64> > dup_mask_cnst_strm_c0[_NCnst0];
#pragma HLS STREAM variable = dup_mask_cnst_strm_c0 depth = 2
#pragma HLS RESOURCE variable = dup_mask_cnst_strm_c0 core = FIFO_LUTRAM
    hls::stream<bool> dup_inv_ctr_strm_c0[_NCnst0];
#pragma HLS STREAM variable = dup_inv_ctr_strm_c0 depth = 2
#pragma HLS RESOURCE variable = dup_inv_ctr_strm_c0 core = FIFO_LUTRAM
    hls::stream<ap_uint<512> > dup_cnst_str_strm_c1[_NCnst1];
#pragma HLS STREAM variable = dup_cnst_str_strm_c1 depth = 2
#pragma HLS RESOURCE variable = dup_cnst_str_strm_c1 core = FIFO_LUTRAM
    hls::stream<ap_uint<64> > dup_mask_cnst_strm_c1[_NCnst1];
#pragma HLS STREAM variable = dup_mask_cnst_strm_c1 depth = 2
#pragma HLS RESOURCE variable = dup_mask_cnst_strm_c1 core = FIFO_LUTRAM
    hls::stream<bool> dup_inv_ctr_strm_c1[_NCnst1];
#pragma HLS STREAM variable = dup_inv_ctr_strm_c1 depth = 2
#pragma HLS RESOURCE variable = dup_inv_ctr_strm_c1 core = FIFO_LUTRAM

    hls::stream<ap_uint<512> > padding_str_strm_out[2];
#pragma HLS STREAM variable = padding_str_strm_out depth = 8
#pragma HLS RESOURCE variable = padding_str_strm_out core = FIFO_LUTRAM
    hls::stream<bool> e_padding_str_strm_out[2];
#pragma HLS STREAM variable = e_padding_str_strm_out depth = 8
#pragma HLS RESOURCE variable = e_padding_str_strm_out core = FIFO_LUTRAM

    hls::stream<ap_uint<512> > dup_padding_str_c0[_NCnst0];
#pragma HLS STREAM variable = dup_padding_str_c0 depth = 8
#pragma HLS RESOURCE variable = dup_padding_str_c0 core = FIFO_LUTRAM
    hls::stream<bool> dup_e_padding_str_c0[_NCnst0];
#pragma HLS STREAM variable = dup_e_padding_str_c0 depth = 8
#pragma HLS RESOURCE variable = dup_e_padding_str_c0 core = FIFO_LUTRAM
    hls::stream<ap_uint<512> > dup_padding_str_c1[_NCnst1];
#pragma HLS STREAM variable = dup_padding_str_c1 depth = 8
#pragma HLS RESOURCE variable = dup_padding_str_c1 core = FIFO_LUTRAM
    hls::stream<bool> dup_e_padding_str_c1[_NCnst1];
#pragma HLS STREAM variable = dup_e_padding_str_c1 depth = 8
#pragma HLS RESOURCE variable = dup_e_padding_str_c1 core = FIFO_LUTRAM

    hls::stream<bool> is_equal_c0[_NCnst0];
#pragma HLS STREAM variable = is_equal_c0 depth = 8
#pragma HLS RESOURCE variable = is_equal_c0 core = FIFO_LUTRAM
    hls::stream<bool> e_eq_strm_c0[_NCnst0];
#pragma HLS STREAM variable = e_eq_strm_c0 depth = 8
#pragma HLS RESOURCE variable = e_eq_strm_c0 core = FIFO_LUTRAM
    hls::stream<bool> is_equal_c1[_NCnst1];
#pragma HLS STREAM variable = is_equal_c1 depth = 8
#pragma HLS RESOURCE variable = is_equal_c1 core = FIFO_LUTRAM
    hls::stream<bool> e_eq_strm_c1[_NCnst1];
#pragma HLS STREAM variable = e_eq_strm_c1 depth = 8
#pragma HLS RESOURCE variable = e_eq_strm_c1 core = FIFO_LUTRAM

    // duplicate nrow stream
    _dup_stream<2, int>(nrow_strm[0], dup_row_strm_c0);
    _dup_stream<2, int>(nrow_strm[1], dup_row_strm_c1);

    // duplicate number of constant string
    _dup_stream<2, ap_uint<4> >(num_cnst_str[0], dup_num_cnst_str_c0);
    _dup_stream<2, ap_uint<4> >(num_cnst_str[1], dup_num_cnst_str_c1);

    // padding string into 512b
    _split_three_stream<_NCnst0, ap_uint<512>, ap_uint<64>, bool>(cnst_strm[0], mask_cnst_strm[0], inv_ctr_strm[0],
                                                                  e_cnst_strm[0], dup_cnst_str_strm_c0,
                                                                  dup_mask_cnst_strm_c0, dup_inv_ctr_strm_c0);
    xf::database::details::padding_stream_out(pact_stream[0], e_pact_strm[0], dup_row_strm_c0[0],
                                              padding_str_strm_out[0], e_padding_str_strm_out[0]);
    _dup_stream_pair<_NCnst0, ap_uint<512> >(padding_str_strm_out[0], e_padding_str_strm_out[0], dup_num_cnst_str_c0[0],
                                             dup_padding_str_c0, dup_e_padding_str_c0);

    _split_three_stream<_NCnst1, ap_uint<512>, ap_uint<64>, bool>(cnst_strm[1], mask_cnst_strm[1], inv_ctr_strm[1],
                                                                  e_cnst_strm[1], dup_cnst_str_strm_c1,
                                                                  dup_mask_cnst_strm_c1, dup_inv_ctr_strm_c1);
    xf::database::details::padding_stream_out(pact_stream[1], e_pact_strm[1], dup_row_strm_c1[0],
                                              padding_str_strm_out[1], e_padding_str_strm_out[1]);
    _dup_stream_pair<_NCnst1, ap_uint<512> >(padding_str_strm_out[1], e_padding_str_strm_out[1], dup_num_cnst_str_c1[0],
                                             dup_padding_str_c1, dup_e_padding_str_c1);

    // string compare, bool output
    for (int i = 0; i < _NCnst0; i++) {
#pragma HLS unroll
        xf::database::details::str_equal_cond(dup_padding_str_c0[i], dup_e_padding_str_c0[i], dup_cnst_str_strm_c0[i],
                                              dup_mask_cnst_strm_c0[i], dup_inv_ctr_strm_c0[i], is_equal_c0[i],
                                              e_eq_strm_c0[i]);
    }

    for (int i = 0; i < _NCnst1; i++) {
#pragma HLS unroll
        xf::database::details::str_equal_cond(dup_padding_str_c1[i], dup_e_padding_str_c1[i], dup_cnst_str_strm_c1[i],
                                              dup_mask_cnst_strm_c1[i], dup_inv_ctr_strm_c1[i], is_equal_c1[i],
                                              e_eq_strm_c1[i]);
    }

    // gather compare result into multi-channnel
    _gather_str_cmp<_CH, _NCnst0>(dup_row_strm_c0[1], dup_num_cnst_str_c0[1], is_equal_c0, e_eq_strm_c0, out_strm[0],
                                  e_out_strm[0]);

    _gather_str_cmp<_CH, _NCnst1>(dup_row_strm_c1[1], dup_num_cnst_str_c1[1], is_equal_c1, e_eq_strm_c1, out_strm[1],
                                  e_out_strm[1]);
}

/**
 * @brief split input stream into multiple channel
 *
 * @tparam _WData data width of each entry in a row
 * @tparam _CH number of output channel
 * @tparam _NCol number of column
 *
 * @param colvec_strm[col_num] input stream of each column
 * @param nrow_strm number of row in one column
 * @param col_strm[ch_num][col_num] multi-channel ouptut stream
 * @param o_nrow_strm output number of row
 */
template <int _WData, int _CH, int _NCol>
void _split_colvec_to_channel(hls::stream<ap_uint<512> > colvec_strm[_NCol],
                              hls::stream<int>& nrow_strm,
                              hls::stream<ap_uint<2> >& num_str_col,

                              hls::stream<ap_uint<_WData> > col_strm[_CH][_NCol],
                              //    hls::stream<int>& o_nrow_strm,
                              hls::stream<bool> e_col_strm[_CH]) {
    const int vec_len = 512 / _WData;
    enum { per_ch = vec_len / _CH };

    int nrow = nrow_strm.read();
    ap_uint<2> str_col = num_str_col.read();
//  o_nrow_strm.write(nrow);
SPLIT_COL_VEC:
    for (int i = 0; i < nrow; i += vec_len) {
#pragma HLS PIPELINE II = per_ch
        int n = (i + vec_len) > nrow ? (nrow - i) : vec_len;
        ap_uint<512> colvec[_NCol];
        for (int c = 0; c < _NCol; ++c) {
#pragma HLS unroll
            if (((str_col == 1) && (c == 0)) || ((str_col == 2) && (c < 2)))
                colvec[c] = 0;
            else
                colvec[c] = colvec_strm[c].read();
        }
        // j for word in vec
        for (int j = 0; j < per_ch; ++j) {
            // ch for channel
            for (int ch = 0; ch < _CH; ++ch) {
#pragma HLS unroll
                ap_uint<_WData> ct[_NCol];
                for (int c = 0; c < _NCol; ++c) {
#pragma HLS unroll
                    ct[c] = colvec[c].range(_WData * (j * _CH + ch + 1) - 1, _WData * (j * _CH + ch));
                }
                if ((j * _CH + ch) < n) {
                    for (int c = 0; c < _NCol; ++c) {
#pragma HLS unroll
                        col_strm[ch][c].write(ct[c]);
#if !defined __SYNTHESIS__ && XDEBUG == 1
                        std::cout << std::dec << "ch:" << ch << ",c:" << c << ",ctc:" << ct[c] << std::endl;
#endif // !defined __SYNTHESIS__ && XDEBUG == 1
                    }
                    e_col_strm[ch].write(false);
                }
            }
        }
    }
    for (int ch = 0; ch < _CH; ++ch) {
#pragma HLS unroll
        e_col_strm[ch].write(true);
    }
}

/**
 * @brief top function of scan
 *
 * @tparam _WData element width in each row
 * @tparam _CH number of channel
 * @tparam _NCol number of column
 *
 * @param config configuration input stream
 * @param buf_in pointer of DDR memory
 * @param out_strms output multi-channel for each column
 * @param e_out_strms end flag stream for out_strms
 */
template <int _WData, int _CH, int _NCol>
void scan_table(hls::stream<ap_int<64> >& config,
                ap_uint<512>* buf_in,

                hls::stream<ap_uint<_WData> > out_strms[_CH][_NCol],
                hls::stream<bool> e_out_strms[_CH],
                hls::stream<ap_uint<_CH> > filter_out_strms[2],
                hls::stream<bool> e_filter_out_strms[2]) {
#pragma HLS INLINE off
#pragma HLS DATAFLOW
    enum { col_fifo_depth = BURST_LEN, str_fifo_depth = BURST_LEN * 512 / _WData };

    // _read_to_colvec related
    hls::stream<ap_uint<512> > read_out_strm[_NCol];
#pragma HLS stream variable = read_out_strm depth = col_fifo_depth
#pragma HLS RESOURCE variable = read_out_strm core = FIFO_LUTRAM
    hls::stream<ap_uint<512> > read_pact_strm[2];
#pragma HLS stream variable = read_pact_strm depth = str_fifo_depth
#pragma HLS RESOURCE variable = read_pact_strm core = FIFO_BRAM
    hls::stream<bool> read_e_pact_strm[2];
#pragma HLS stream variable = read_e_pact_strm depth = str_fifo_depth
#pragma HLS RESOURCE variable = read_e_pact_strm core = FIFO_LUTRAM

    hls::stream<ap_uint<512> > read_cnst_strm[2];
#pragma HLS stream variable = read_cnst_strm depth = 2
#pragma HLS RESOURCE variable = read_cnst_strm core = FIFO_LUTRAM
    hls::stream<ap_uint<64> > read_mask_cnst_strm[2];
#pragma HLS stream variable = read_mask_cnst_strm depth = 2
#pragma HLS RESOURCE variable = read_mask_cnst_strm core = FIFO_LUTRAM
    hls::stream<bool> read_inv_ctr_strm[2];
#pragma HLS stream variable = read_inv_ctr_strm depth = 2
#pragma HLS RESOURCE variable = read_inv_ctr_strm core = FIFO_LUTRAM
    hls::stream<bool> read_e_cnst_strm[2];
#pragma HLS stream variable = read_e_cnst_strm depth = 2
#pragma HLS RESOURCE variable = read_e_cnst_strm core = FIFO_LUTRAM

    hls::stream<ap_uint<4> > read_num_cnst_str[2];
#pragma HLS stream variable = read_num_cnst_str depth = 2
#pragma HLS RESOURCE variable = read_num_cnst_str core = FIFO_LUTRAM
    hls::stream<int> read_nrow_strm_filter[2];
#pragma HLS stream variable = read_nrow_strm_filter depth = 2
#pragma HLS RESOURCE variable = read_nrow_strm_filter core = FIFO_LUTRAM
    hls::stream<int> read_nrow_strm_spliter("read_nrow_strm_spliter");
#pragma HLS stream variable = read_nrow_strm_spliter depth = 2
#pragma HLS RESOURCE variable = read_nrow_strm_spliter core = FIFO_LUTRAM
    hls::stream<ap_uint<2> > read_num_str_col_strm("read_num_str_col_strm");
#pragma HLS stream variable = read_num_str_col_strm depth = 2
#pragma HLS RESOURCE variable = read_num_str_col_strm core = FIFO_LUTRAM

    _read_to_colvec<_NCol>(buf_in, config, read_out_strm, read_cnst_strm, read_mask_cnst_strm, read_inv_ctr_strm,
                           read_e_cnst_strm, read_pact_strm, read_e_pact_strm, read_num_cnst_str, read_nrow_strm_filter,
                           read_nrow_strm_spliter, read_num_str_col_strm);

    _proc_filter<_CH>(read_nrow_strm_filter, read_num_cnst_str, read_pact_strm, read_e_pact_strm, read_cnst_strm,
                      read_mask_cnst_strm, read_inv_ctr_strm, read_e_cnst_strm, filter_out_strms, e_filter_out_strms);

    _split_colvec_to_channel<_WData, _CH, _NCol>(read_out_strm, read_nrow_strm_spliter, read_num_str_col_strm,
                                                 out_strms, e_out_strms);
}

} // namespace gqe
} // namespace database
} // namespace xf

#endif
