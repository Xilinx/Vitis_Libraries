/*
 * Copyright (C) 2019-2022, Xilinx, Inc.
 * Copyright (C) 2022-2023, Advanced Micro Devices, Inc.
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
 * This file is part of Vitis Data Analytics Library.
 */
#ifndef XF_DATA_ANALYTICS_L1_DATAFRAME_READ_BLOCK_HPP
#define XF_DATA_ANALYTICS_L1_DATAFRAME_READ_BLOCK_HPP

#include "ap_int.h"
#include "hls_stream.h"

#ifndef __SYNTHESIS__
#include <iostream>
#endif

namespace xf {
namespace data_analytics {
namespace dataframe {
namespace internal {
template <int MODE>
static void split(bool last,
                  bool first,
                  hls::stream<ap_uint<128> >& i_strm,
                  hls::stream<bool>& i_e_strm,
                  hls::stream<ap_uint<8> >& o_strm,
                  hls::stream<bool>& o_e_strm) {
    bool e = i_e_strm.read();
    bool fnd_head = first;
    ap_uint<128> i_128 = 0;
    ap_uint<4> vec_len = 0;
    bool is_delimiter;
    while (!e) {
#pragma HLS pipeline II = 1
        if (vec_len == 0) {
            e = i_e_strm.read();
            i_128 = i_strm.read();
        }
        ap_uint<8> i_8 = i_128.range(7, 0);
        i_128.range(119, 0) = i_128.range(127, 8);
        vec_len++;
        if (i_8 == '\n')
            is_delimiter = true;
        else
            is_delimiter = false;

        if (fnd_head) {
            if (MODE == 1 && !is_delimiter) { // JSON, remove '\n'
                o_strm.write(i_8);
                o_e_strm.write(false);
            } else if (MODE == 0) { // CSV, reserve '\n'
                o_strm.write(i_8);
                o_e_strm.write(false);
            }
        }
        if (!fnd_head && is_delimiter) fnd_head = true;
    }

    // for last line, when reach the \n, then stop, the first char is already feteched.
    ap_uint<4> delimiter = 0;
    if (last) {
        bool mask = true;
        for (int i = 0; i < 15; i++) {
#pragma HLS pipeline II = 1
            ap_uint<8> i_8 = i_128.range((i + 1) * 8 - 1, i * 8);
            if (mask && i_8 == '\n') {
                mask = false;
                delimiter = i;
            }
        }
    } else {
        if (is_delimiter)
            delimiter = 0;
        else {
            for (int i = 0; i < 15; ++i) {
#pragma HLS pipeline II = 1
                ap_uint<8> i_8 = i_128.range((i + 1) * 8 - 1, i * 8);
                if (!is_delimiter && i_8 == '\n') {
                    delimiter = i;
                    break;
                }
            }
        }
    }

    for (int i = 0; i <= delimiter && !is_delimiter; ++i) {
#pragma HLS pipeline II = 1
        ap_uint<8> i_8 = i_128.range(7, 0);
        i_128.range(119, 0) = i_128.range(127, 8);
        o_strm.write(i_8);
        o_e_strm.write(false);
    }
    o_e_strm.write(true);
}

template <int NM>
void readBlock(ap_uint<29> base_addr_buff[NM],
               ap_uint<29> nm_buff[NM],
               ap_uint<128>* ddr_buff,
               hls::stream<ap_uint<128> > o_strm[NM],
               hls::stream<bool> e_strm[NM]) {
    const int VBLEN = 512;
    int loop_nm = 0;
    for (int i = 0; i < NM; ++i) {
#pragma HLS pipeline II = 1
        int tmp = (nm_buff[i] + VBLEN - 1) / VBLEN;
        if (tmp > loop_nm) loop_nm = tmp;
    }

    ap_uint<29> left_nm = 0;
    for (int l = 0; l < loop_nm; ++l) {
        for (int i = 0; i < NM; ++i) {
            left_nm = nm_buff[i];
            ap_uint<10> blk_nm = VBLEN;
            if (left_nm > VBLEN)
                blk_nm = VBLEN;
            else
                blk_nm = left_nm;
            nm_buff[i] = left_nm - blk_nm;
            ap_uint<29> base_addr = base_addr_buff[i];
            base_addr_buff[i] += blk_nm;
            for (int j = 0; j < blk_nm; ++j) {
#pragma HLS pipeline II = 1
                ap_uint<128> in_128 = ddr_buff[base_addr + j];
                o_strm[i].write(in_128);
                e_strm[i].write(false);
            }
        }
    }
    for (int i = 0; i < NM; ++i) {
#pragma HLS pipeline II = 1
        e_strm[i].write(true);
    }
}

template <int NM, int MODE>
void readWrapper(ap_uint<29> base_addr_buff[NM],
                 ap_uint<29> nm_buff[NM],
                 ap_uint<128>* ddr_buff,
                 hls::stream<ap_uint<8> > o_strm[NM],
                 hls::stream<bool> e_strm[NM]) {
#pragma HLS dataflow
    hls::stream<ap_uint<128> > d_128_strm[NM];
#pragma HLS stream variable = d_128_strm depth = 1024
#pragma HLS array_partition variable = d_128_strm complete
#pragma HLS bind_storage variable = d_128_strm type = fifo impl = bram

    hls::stream<bool> m_e_strm[NM];
#pragma HLS stream variable = m_e_strm depth = 1024
#pragma HLS array_partition variable = m_e_strm complete
    readBlock<NM>(base_addr_buff, nm_buff, ddr_buff, d_128_strm, m_e_strm);

    for (int i = 0; i < NM; ++i) {
#pragma HLS unroll
        if (i == 0)
            split<MODE>(false, true, d_128_strm[i], m_e_strm[i], o_strm[i], e_strm[i]);
        else if (i == NM - 1)
            split<MODE>(true, false, d_128_strm[i], m_e_strm[i], o_strm[i], e_strm[i]);
        else
            split<MODE>(false, false, d_128_strm[i], m_e_strm[i], o_strm[i], e_strm[i]);
    }
}

template <int NM>
void blockDivide(ap_uint<29>* base_addr_buff, ap_uint<29>* nm_buff, ap_uint<128>* ddr_buff) {
    ap_uint<128> head = ddr_buff[0];
    ap_uint<29> avg_sz = head.range(31, 0);
    ap_uint<29> left_sz = head.range(63, 32);
#ifndef __SYNTHESIS__
// std::cout << "average size: " << avg_sz << ", last size: " << left_sz << std::endl;
#endif
    // initialize
    for (int i = 0; i < NM; ++i) {
#pragma HLS pipeline II = 1
        if (i == NM - 1)
            nm_buff[i] = left_sz;
        else
            nm_buff[i] = avg_sz;
        base_addr_buff[i] = i * avg_sz + 1;
    }

    int cnt = 0;
    // update, start from a complete line
    for (int i = 1; i < NM; ++i) {
        bool find_head = false;
        ap_uint<29> addr = base_addr_buff[i];
        cnt = 0;
        while (!find_head) {
#pragma HLS pipeline II = 2
            ap_uint<128> i_128 = ddr_buff[addr++];
            cnt++;
            // serach \n from head to end
            for (int j = 0; j < 16; ++j) {
#pragma HLS unroll
                ap_uint<8> i_8 = i_128.range((j + 1) * 8 - 1, j * 8);
                if (i_8 == '\n') {
                    find_head = true;
                }
            }
        }
        // update the base address and row number of last block
        base_addr_buff[i] = base_addr_buff[i] + cnt - 1;
        nm_buff[i - 1] = nm_buff[i - 1] + cnt;
        nm_buff[i] = nm_buff[i] + 1 - cnt;
    }
}

/**
 * @brief read CSV line to multiple stream for PUs
 *
 * @tparam NM number of PU
 * @param ddr_buff buffer of CSV file
 * @param o_strm output stream of string
 * @param e_strm output end flag of i_strm
 **/
template <int NM>
void readCSV(ap_uint<128> ddr_buff[2000], hls::stream<ap_uint<8> > o_strm[NM], hls::stream<bool> e_strm[NM]) {
    ap_uint<29> base_addr_buff[NM];
    ap_uint<29> nm_buff[NM];

    blockDivide<NM>(base_addr_buff, nm_buff, ddr_buff);
    readWrapper<NM, 0>(base_addr_buff, nm_buff, ddr_buff, o_strm, e_strm);
}

/**
 * @brief read JSON line to multiple stream for PUs
 *
 * @tparam NM number of PU
 * @param ddr_buff buffer of JSON file
 * @param o_strm output stream of string
 * @param e_strm output end flag of i_strm
 **/
template <int NM>
void readJSON(ap_uint<128> ddr_buff[2000], hls::stream<ap_uint<8> > o_strm[NM], hls::stream<bool> e_strm[NM]) {
    ap_uint<29> base_addr_buff[NM];
    ap_uint<29> nm_buff[NM];

    blockDivide<NM>(base_addr_buff, nm_buff, ddr_buff);
    readWrapper<NM, 1>(base_addr_buff, nm_buff, ddr_buff, o_strm, e_strm);
}
}
}
}
}
#endif
