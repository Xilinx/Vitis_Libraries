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

#ifndef _BLOCK_MERGE_TREE_HPP_
#define _BLOCK_MERGE_TREE_HPP_

#include "xf_database/merge_sort.hpp"
#include "block_sort.hpp"

namespace xf {
namespace data_analytics {
namespace geospatial {
namespace internal {

/**
 * @brief mem2NStrmBP memory to N stream with back-pressure
 * @tparam DT data type
 * @tparam CH output channel number
 * @tparam BN block length
 * @tparam BL burst length of AXI read
 *
 */
template <typename DT, int CH, int BN, int BL = 1024>
void mem2NStrmBP(
    int sz, DT* i_buf, hls::stream<DT> o_strm[CH], hls::stream<bool> o_bp_strm[CH], hls::stream<bool> o_end_strm[CH]) {
#pragma HLS inline off
    int begin_buff[CH];
    ap_uint<CH> flag = -1;
    for (int i = 0; i < CH; i++) {
        begin_buff[i] = BN * i;
        // if (BN * i > sz) flag[i] = 0;
    }
    DT tmp = 0;
    while (flag) {
#pragma HLS loop_tripcount max = 10 min = 10
        for (int i = 0; i < CH; i++) {
            if (flag[i]) {
                bool wr_flag = o_bp_strm[i].write_nb(0);
                if (wr_flag) {
                    for (int j = 0; j < BL; j++) {
#pragma HLS pipeline ii = 1
                        tmp = i_buf[begin_buff[i] + j];
                        if (begin_buff[i] + j >= (i + 1) * BN || begin_buff[i] + j >= sz) {
                            if (flag[i]) o_end_strm[i].write(true);
                            flag[i] = 0;
                        } else if (flag[i]) {
                            o_strm[i].write(tmp);
                            // std::cout << "offChipMem2Strm: x=" << tmp.range(31, 0) << ", y=" << tmp.range(63, 32)
                            //          << ", id=" << tmp.range(95, 64) << std::endl;
                            o_end_strm[i].write(false);
                        }
                    }
                    if (flag[i] && (begin_buff[i] + BL >= (i + 1) * BN || begin_buff[i] + BL >= sz)) {
                        if (flag[i]) o_end_strm[i].write(true);
                        flag[i] = 0;
                    }
                    begin_buff[i] += BL;
                }
            }
        }
    }
    // for (int i = 0; i < CH; i++) {
    //    o_end_strm[i].write(true);
    //}
}

template <typename KT, typename VT, typename MT, int BL = 1024>
void convKeyValStrm(int l_cnt,
                    hls::stream<MT>& i_strm,
                    hls::stream<bool>& i_bp_strm,
                    hls::stream<bool>& i_end_strm,
                    hls::stream<KT>& o_key_strm,
                    hls::stream<VT>& o_val_strm,
                    hls::stream<bool>& o_end_blk_strm,
                    hls::stream<bool>& o_end_strm) {
    for (int i = 0; i < l_cnt; i++) {
#pragma HLS loop_tripcount max = 10 min = 10
        o_end_strm.write(0);
        int cnt = 0;
        while (!i_end_strm.read()) {
#pragma HLS loop_tripcount max = 1000 min = 1000
            MT tmp = i_strm.read();
            KT key = tmp(sizeof(KT) * 8 - 1, 0);
            VT val = tmp((sizeof(KT) + sizeof(VT)) * 8 - 1, sizeof(KT) * 8);
            o_key_strm.write(key);
            o_val_strm.write(val);
            o_end_blk_strm.write(0);
            cnt++;
            if (cnt % BL == 0) {
                i_bp_strm.read();
            }
        }
        if (cnt == 0 || (cnt % BL)) i_bp_strm.read();
        o_end_blk_strm.write(1);
    }
    o_end_strm.write(1);
}

template <typename MT, int ISN, int OSN, int BL>
void mem2NStrmBPWrap(int sz,
                     MT* i_buf,
                     hls::stream<MT> o_data_strm[OSN / ISN],
                     hls::stream<bool> o_bp_strm[OSN / ISN],
                     hls::stream<bool> o_end_strm[OSN / ISN]) {
    int l_cnt = (sz + OSN - 1) / OSN;
    for (int i = 0; i < l_cnt; i++) {
#pragma HLS loop_tripcount max = 10 min = 10
        int blk_sz = OSN;
        if (i + 1 == l_cnt) blk_sz = sz - i * OSN;
        mem2NStrmBP<MT, OSN / ISN, ISN, BL>(blk_sz, i_buf + OSN * i, o_data_strm, o_bp_strm, o_end_strm);
    }
}

template <typename KT, typename VT, typename MT, int N, int BL>
void convKeyValStrmWrap(int cnt,
                        hls::stream<MT> i_data_strm[N],
                        hls::stream<bool> i_bp_strm[N],
                        hls::stream<bool> i_end_strm[N],
                        hls::stream<KT> o_key_strm[N],
                        hls::stream<VT> o_val_strm[N],
                        hls::stream<bool> o_end_blk_strm[N],
                        hls::stream<bool> o_end_strm[N]) {
#pragma HLS dataflow
    for (int n = 0; n < N; n++) {
#pragma HLS unroll
        convKeyValStrm<KT, VT, MT, BL>(cnt, i_data_strm[n], i_bp_strm[n], i_end_strm[n], o_key_strm[n], o_val_strm[n],
                                       o_end_blk_strm[n], o_end_strm[n]);
    }
}

template <typename KT, typename VT, typename MT>
void strm2Mem(
    int sz, hls::stream<KT>& i_key_strm, hls::stream<VT>& i_val_strm, hls::stream<bool>& i_end_strm, MT* o_buff) {
    MT tmp = 0;
    for (int i = 0; i < sz; i++) {
#pragma HLS pipeline ii = 1
#pragma HLS loop_tripcount max = 100000 min = 100000
        tmp(sizeof(KT) * 8 - 1, 0) = i_key_strm.read();
        tmp((sizeof(KT) + sizeof(VT)) * 8 - 1, sizeof(KT) * 8) = i_val_strm.read();
        i_end_strm.read();
        o_buff[i] = tmp;
        // std::cout << "strm2Mem Point: i=" << i << ", x=" << tmp(63, 32) << ", y=" << tmp(31, 0)
        //          << ", id=" << tmp(95, 64) << std::endl;
    }
    i_end_strm.read();
    // std::cout << "strm2Mem: size=" << i << std::endl;
}

template <typename KT, typename VT, typename MT, int ISN, int OSN>
void mergeSortTreeMem(bool i_order, int sz, MT* i_buf, MT* o_buf) {
#pragma HLS dataflow
    const int BL = 1024; // burst length of AXI read
    const int K = OSN / ISN;
    hls::stream<MT, BL * 4> data_strm[K];
    hls::stream<bool, 4> data_bp_strm[K];
    hls::stream<bool, BL * 4> data_end_strm[K];
    hls::stream<KT, 32> key_strm[K];
    hls::stream<VT, 32> val_strm[K];
    hls::stream<bool, 32> end_blk_strm[K];
    hls::stream<bool, 32> end_strm[K];
    int l_cnt = (sz + OSN - 1) / OSN;
    if (BL > ISN) {
        mem2NStrmBPWrap<MT, ISN, OSN, ISN>(sz, i_buf, data_strm, data_bp_strm, data_end_strm);
        convKeyValStrmWrap<KT, VT, MT, K, ISN>(l_cnt, data_strm, data_bp_strm, data_end_strm, key_strm, val_strm,
                                               end_blk_strm, end_strm);
    } else {
        mem2NStrmBPWrap<MT, ISN, OSN, BL>(sz, i_buf, data_strm, data_bp_strm, data_end_strm);
        convKeyValStrmWrap<KT, VT, MT, K, BL>(l_cnt, data_strm, data_bp_strm, data_end_strm, key_strm, val_strm,
                                              end_blk_strm, end_strm);
    }
    hls::stream<KT, 32> key_strm2;
    hls::stream<VT, 32> val_strm2;
    hls::stream<bool, 32> end_strm2;
    mergeSortTree<KT, VT, K>(i_order, key_strm, val_strm, end_blk_strm, end_strm, key_strm2, val_strm2, end_strm2);
    internal::strm2Mem<KT, VT, MT>(sz, key_strm2, val_strm2, end_strm2, o_buf);
}

template <typename KT, typename VT, typename PT>
void readPointId(int sz, int bgn, PT* i_buf, hls::stream<VT>& o_id_strm) {
    for (int i = 0; i < sz; i++) {
        PT tmp = i_buf[i + bgn];
        VT id = tmp(95, 64);
        o_id_strm.write(id);
    }
}

template <typename KT, typename VT, typename NT>
void readNodeById(int sz, int bgn, hls::stream<VT>& i_id_strm, NT* i_buf, hls::stream<NT>& o_node_strm) {
    for (int i = 0; i < sz; i++) {
        VT id = i_id_strm.read();
        NT tmp = i_buf[id + bgn];
        // std::cout << "readNodeById: id=" << id << ", bgn=" << bgn << std::endl;
        o_node_strm.write(tmp);
    }
}

template <typename NT, int NC>
void writeNowNode(int sz, int bgn, hls::stream<NT>& i_node_strm, hls::stream<NT>& o_node_strm, NT* o_buf) {
    uint32_t xmin, xmax, ymin, ymax;
    NT tmp2 = 0;
    for (int i = 0; i < sz; i++) {
        NT tmp = i_node_strm.read();
        o_buf[bgn + i] = tmp;
        if (i % NC == 0) {
            xmin = tmp(63, 32);
            xmax = tmp(95, 64);
            ymin = tmp(127, 96);
            ymax = tmp(159, 128);
        } else {
            if (xmin > tmp(63, 32)) xmin = tmp(63, 32);
            if (xmax < tmp(95, 64)) xmax = tmp(95, 64);
            if (ymin > tmp(127, 96)) ymin = tmp(127, 96);
            if (ymax < tmp(159, 128)) ymax = tmp(159, 128);
            if ((i + 1) % NC == 0) {
                tmp2(7, 0) = 1;
                tmp2(15, 8) = NC;
                tmp2(31, 16) = 0;
                tmp2(63, 32) = xmin;
                tmp2(95, 64) = xmax;
                tmp2(127, 96) = ymin;
                tmp2(159, 128) = ymax;
                tmp2(191, 160) = bgn + (i / NC) * NC;
                o_node_strm.write(tmp2);
            }
        }
    }
    if (sz % NC) {
        tmp2(7, 0) = 1;
        tmp2(15, 8) = sz % NC;
        tmp2(31, 16) = 0;
        tmp2(63, 32) = xmin;
        tmp2(95, 64) = xmax;
        tmp2(127, 96) = ymin;
        tmp2(159, 128) = ymax;
        tmp2(191, 160) = bgn + (sz / NC) * NC;
        o_node_strm.write(tmp2);
    }
}

template <typename NT>
void writeNextNode(int sz, int bgn, hls::stream<NT>& i_node_strm, NT* o_buf) {
    for (int i = 0; i < sz; i++) {
        NT tmp = i_node_strm.read();
        o_buf[i + bgn] = tmp;
        // std::cout << "writeNextNode: id=" << i << ", bgn=" << bgn << std::endl;
    }
}

template <typename KT, typename VT, typename PT, typename NT, int NC>
void dataMove(int sz, int last_bgn, PT* i_point_buf, NT* i_node_buf, NT* o_node_buf1, NT* o_node_buf2) {
#pragma HLS dataflow
    hls::stream<VT, 32> id_strm;
    readPointId<KT, VT, PT>(sz, last_bgn, i_point_buf, id_strm);
    hls::stream<NT, 32> node_strm1;
    hls::stream<NT, 32> node_strm2;
    readNodeById<KT, VT, NT>(sz, last_bgn, id_strm, i_node_buf, node_strm1);
    writeNowNode<NT, NC>(sz, last_bgn, node_strm1, node_strm2, o_node_buf1);
    writeNextNode<NT>((sz + NC - 1) / NC, last_bgn + sz, node_strm2, o_node_buf2);
}

template <typename DT>
void writeOutput(int bgn, int sz, hls::stream<DT>& i_data_strm, hls::stream<bool>& i_end_strm, DT* o_buf) {
    int i = 0;
    DT tmp = 0;
    bool flag = i_end_strm.read();
    for (int i = 0; i < sz; i++) {
        if (!flag) {
            tmp = i_data_strm.read();
            flag = i_end_strm.read();
        }
        o_buf[i + bgn] = tmp;
        // std::cout << "writeOutput i=" << i + bgn << std::endl;
    }
}

// NT* o_node_buff) {
template <typename KT, typename VT, typename PT, typename NT, int NC>
void calcuPointNode(int bgn,
                    hls::stream<KT>& i_key_strm,
                    hls::stream<VT>& i_val_strm,
                    hls::stream<bool>& i_end_strm,
                    hls::stream<PT>& o_point_strm,
                    hls::stream<bool>& o_point_end_strm,
                    hls::stream<NT>& o_node_strm,
                    hls::stream<bool>& o_node_end_strm) {
    // NT* o_node_buff) {
    PT tmp = 0;
    NT tmp2 = 0;
    int i = 0;
    uint32_t xmin, xmax, ymin, ymax;
    while (!i_end_strm.read()) {
#pragma HLS pipeline ii = 1
#pragma HLS loop_tripcount max = 100000 min = 100000
        KT key = i_key_strm.read();
        tmp(sizeof(KT) * 8 - 1, 0) = key;
        tmp((sizeof(KT) + sizeof(VT)) * 8 - 1, sizeof(KT) * 8) = i_val_strm.read();
        uint32_t x = key(31, 0);
        uint32_t y = key(63, 32);
        if (i % NC == 0) {
            xmin = x;
            xmax = x;
            ymin = y;
            ymax = y;
        } else {
            if (xmin > x) xmin = x;
            if (xmax < x) xmax = x;
            if (ymin > y) ymin = y;
            if (ymax < y) ymax = y;
            if ((i + 1) % NC == 0) {
                tmp2(7, 0) = 0;
                tmp2(15, 8) = NC;
                tmp2(31, 16) = 0;
                tmp2(63, 32) = xmin;
                tmp2(95, 64) = xmax;
                tmp2(127, 96) = ymin;
                tmp2(159, 128) = ymax;
                tmp2(191, 160) = bgn + (i / NC) * NC;
                o_node_strm.write(tmp2);
                o_node_end_strm.write(0);
                // o_node_buff[(bgn + i) / NC] = tmp2;
                // std::cout << "outputStream: i=" << i << ", level=" << tmp2(31, 0) << ", xmin=" << tmp2(63, 32)
                //          << ", xmax=" << tmp2(95, 64) << ", ymin=" << tmp2(127, 96) << ", ymax=" << tmp2(159, 128)
                //          << ", addr=" << tmp2(191, 160) << std::endl;
            }
        }

        // std::cout << "strm2Mem Point: i=" << i << ", x=" << tmp(63, 32) << ", y=" << tmp(31, 0)
        //          << ", id=" << tmp(95, 64) << std::endl;
        o_point_strm.write(tmp);
        o_point_end_strm.write(0);
        i++;
        // o_point_buff[bgn + i++] = tmp;
    }
    if (i % NC) {
        tmp2(7, 0) = 0;
        tmp2(15, 8) = i % NC;
        tmp2(31, 16) = 0;
        tmp2(63, 32) = xmin;
        tmp2(95, 64) = xmax;
        tmp2(127, 96) = ymin;
        tmp2(159, 128) = ymax;
        tmp2(191, 160) = bgn + (i / NC) * NC;
        // o_node_buff[(bgn + i) / NC] = tmp2;
        o_node_strm.write(tmp2);
        o_node_end_strm.write(0);
    }
    o_node_end_strm.write(1);
    o_point_end_strm.write(1);
    // std::cout << "strm2Mem: size=" << i << std::endl;
}

template <typename KT, typename VT, typename PT, typename NT, int NC>
void strm2MemPointNode(int sz,
                       int bgn,
                       hls::stream<KT>& i_key_strm,
                       hls::stream<VT>& i_val_strm,
                       hls::stream<bool>& i_end_strm,
                       PT* o_point_buff,
                       NT* o_node_buff) {
#pragma HLS dataflow
    hls::stream<PT, 32> point_strm;
    hls::stream<bool, 32> point_end_strm;
    hls::stream<NT, 32> node_strm;
    hls::stream<bool, 32> node_end_strm;
    calcuPointNode<KT, VT, PT, NT, NC>(bgn, i_key_strm, i_val_strm, i_end_strm, point_strm, point_end_strm, node_strm,
                                       node_end_strm);
    // std::cout << "\n\nPoint: \n";
    writeOutput<PT>(bgn, sz, point_strm, point_end_strm, o_point_buff);
    // std::cout << "\n\nNode: \n";
    writeOutput<NT>((bgn + NC - 1) / NC, (sz + NC - 1) / NC, node_strm, node_end_strm, o_node_buff);
}
} // internal
} // geospatial
} // data_analytics
} // xf
#endif // _BLOCK_MERGE_TREE_HPP_
