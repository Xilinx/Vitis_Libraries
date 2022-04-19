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

#ifndef _STRTREE_WRAP_HPP_
#define _STRTREE_WRAP_HPP_

#include "block_sort.hpp"
#include "block_merge_tree.hpp"
#include "index_strtree.hpp"

namespace xf {
namespace data_analytics {
namespace geospatial {
namespace internal {

template <typename KT, typename VT, typename PT, int BSN, int MTCN, int MSN>
void mergeSortTreeDDR(int& ch_cnt, int real_sz, bool i_order, PT* ext_point_buf0, PT* ext_point_buf1) {
#pragma HLS inline off
#ifndef __SYNTHESIS__
    std::cout << "mergeSortTreeDDR: real_sz=" << real_sz << ", BSN=" << BSN << ", MTCN=" << MTCN << std::endl;
#endif
    if (real_sz > BSN) {
        internal::mergeSortTreeMem<KT, VT, PT, BSN, BSN * MTCN>(i_order, real_sz, ext_point_buf0, ext_point_buf1 + MSN);
        ch_cnt++;
    }
    if (real_sz > BSN * MTCN) {
        internal::mergeSortTreeMem<KT, VT, PT, BSN * MTCN, BSN * MTCN * MTCN>(i_order, real_sz, ext_point_buf0 + MSN,
                                                                              ext_point_buf1);
        ch_cnt++;
    }
    // if (real_sz > BSN * MTCN * MTCN) {
    //    internal::mergeSortTreeMem<KT, VT, PT, BSN * MTCN * MTCN, BSN * MTCN * MTCN * MTCN>(
    //        i_order, real_sz, ext_point_buf0, ext_point_buf1 + MSN);
    //    ch_cnt++;
    //}
}

template <typename KT, typename VT, typename PT, typename NT, int ISN, int BSN, int MSN, int NC>
void blockSortWrap(int flag, // 0: quantify & x sort, 1: y sort, 2: x sort
                   int sz,
                   int bgn,
                   bool i_order,
                   // input when flag = 0
                   double* i_xs,
                   double* i_ys,
                   double* i_zone,
                   // input when flag = 1
                   PT* i_point_buff,
                   // input when flag = 2
                   NT* i_node_buff,
                   // output
                   unsigned& o_real_sz,
                   PT* o_point_buff,
                   NT* o_node_buff) {
#pragma HLS inline off
#pragma HLS dataflow
    hls::stream<KT, 32> key_strm;
    hls::stream<VT, 32> val_strm;
    hls::stream<bool, 32> end_strm;
#ifndef __SYNTHESIS__
// std::cout << "mem2StrmConv\n";
#endif
    xf::data_analytics::geospatial::internal::mem2StrmConv<KT, VT, PT, NT>(
        flag, sz, bgn, i_xs, i_ys, i_zone, i_point_buff, i_node_buff, o_real_sz, key_strm, val_strm, end_strm);
    hls::stream<KT, 32> key_strm2;
    hls::stream<VT, 32> val_strm2;
    hls::stream<bool, 32> end_strm2;
#ifndef __SYNTHESIS__
// std::cout << "blockSort\n";
#endif
    internal::blockSort<KT, VT, PT, ISN, BSN, MSN>(i_order, key_strm, val_strm, end_strm, key_strm2, val_strm2,
                                                   end_strm2);
#ifndef __SYNTHESIS__
// std::cout << "strm2Mem\n";
#endif
    internal::strm2MemPointNode<KT, VT, PT, NT, NC>(sz, bgn, key_strm2, val_strm2, end_strm2, o_point_buff,
                                                    o_node_buff);
}
} // internal

/**
 * @brief strtreeTop strtree (a geospatial index) uses bottom-up way to build an R tree for two-dimensional points
 *
 * @tparam KT key type
 * @tparam VT value type
 * @tparam PT point type
 * @tparam NT node type
 * @tparam NC node capacity
 * @tparam ISN insert sort length
 * @tparam BSN block sort length
 * @tparam MTCN merge tree channel number
 * @tparam NSN max sort length
 *
 * @param sz real size of inX or inY
 * @param inX all x value
 * @param inX all y value
 * @param inZone points (x, y) limit zone
 * @param extPointBuf0 output ordered points
 * @param extPointBuf1 points buffer
 * @param extNodeBuf0 output ordered nodes
 * @param extNodeBuf1 nodes buffer
 * @param extNodeBuf2 nodes buffer
 */
template <typename KT, typename VT, typename PT, typename NT, int NC, int ISN, int BSN, int MTCN, int MSN>
void strtreeTop(int sz,
                double* inX,
                double* inY,
                double* inZone,
                PT* extPointBuf0,
                PT* extPointBuf1,
                NT* extNodeBuf0,
                NT* extNodeBuf1,
                NT* extNodeBuf2) {
#pragma HLS inline off
#ifndef __SYNTHESIS__
    std::cout << "Start STRTree_Kernel...\n";
    std::cout << "\n\n===================================== Stage 1 ==========================================\n";
#endif

// clang-format off
#pragma HLS allocation function instances=internal::blockSortWrap<KT, VT, PT, NT, ISN, BSN, MSN, NC> limit=1
    // clang-format on
    double zone[4];
    for (int i = 0; i < 4; i++) zone[i] = inZone[i];
    bool order = 1;
    unsigned real_sz = 0;
    int read_flag = 0;
    int blk_bgn = 0;
    int real_bgn = 0;
    internal::blockSortWrap<KT, VT, PT, NT, ISN, BSN, MSN, NC>(
        read_flag, sz, blk_bgn, order, inX, inY, zone, extPointBuf0, extNodeBuf1, real_sz, extPointBuf1, extNodeBuf2);
    int ch_cnt = 0;
    internal::mergeSortTreeDDR<KT, VT, PT, BSN, MTCN, MSN>(ch_cnt, real_sz, order, extPointBuf0, extPointBuf1);

    int s = xf::data_analytics::geospatial::internal::calcuSlice<NC>(real_sz);
    int sn = s * NC;
    int slices_num = xf::data_analytics::geospatial::internal::ceil(real_sz, sn);
#ifndef __SYNTHESIS__
    std::cout << "\n\n===================================== Stage 2 ==========================================\n";
    std::cout << "sn=" << sn << ", BSN=" << BSN << ", slices_num=" << slices_num << std::endl;
    if (sn > BSN) std::cout << "ERROR\n";
#endif
    unsigned tmp = 0;
    read_flag = 1;
    for (int i = 0; i < slices_num; i++) {
#pragma HLS loop_tripcount max = 10 min = 10
        int blk_sz = sn;
        blk_bgn = i * sn;
        if (slices_num - 1 == i) blk_sz = real_sz - i * sn;
        if (ch_cnt % 2)
            internal::blockSortWrap<KT, VT, PT, NT, ISN, BSN, MSN, NC>(read_flag, blk_sz, blk_bgn, order, inX, inY,
                                                                       zone, extPointBuf0 + MSN, extNodeBuf1, tmp,
                                                                       extPointBuf1, extNodeBuf2);
        else
            internal::blockSortWrap<KT, VT, PT, NT, ISN, BSN, MSN, NC>(read_flag, blk_sz, blk_bgn, order, inX, inY,
                                                                       zone, extPointBuf0, extNodeBuf1, tmp,
                                                                       extPointBuf1, extNodeBuf2);
    }
#ifndef __SYNTHESIS__
    std::cout << "\n\n===================================== Stage 3 ==========================================\n";
#endif
    int node_ch_cnt = 0;
    int node_real_sz = (real_sz + NC - 1) / NC;
    int bgn_buf[16];
    bgn_buf[0] = 0;
    while (node_real_sz > 1) {
#pragma HLS loop_tripcount max = 10 min = 10
#ifndef __SYNTHESIS__
#endif
        // Node -> point (sort X)
        read_flag = 2;
        blk_bgn = real_bgn;
        internal::blockSortWrap<KT, VT, PT, NT, ISN, BSN, MSN, NC>(read_flag, node_real_sz, blk_bgn, order, inX, inY,
                                                                   zone, extPointBuf0, extNodeBuf1, tmp,
                                                                   extPointBuf1 + MSN, extNodeBuf2 + MSN);
        // point -> point (sort x)
        ch_cnt = 0;
        internal::mergeSortTreeDDR<KT, VT, PT, BSN, MTCN, MSN>(
            ch_cnt, node_real_sz, order, extPointBuf0 + MSN + real_bgn, extPointBuf1 + MSN + real_bgn);

        s = xf::data_analytics::geospatial::internal::calcuSlice<NC>(node_real_sz);
        sn = s * NC;
        slices_num = xf::data_analytics::geospatial::internal::ceil(node_real_sz, sn);
#ifndef __SYNTHESIS__
        std::cout << "node_real_sz=" << node_real_sz << ", real_bgn=" << real_bgn << ", s=" << s << ", sn=" << sn
                  << ", slices_num=" << slices_num << std::endl;
#endif
        read_flag = 1;
        for (int i = 0; i < slices_num; i++) {
#pragma HLS loop_tripcount max = 10 min = 10
            int blk_sz = sn;
            blk_bgn = i * sn + real_bgn;
            if (slices_num - 1 == i) blk_sz = node_real_sz - i * sn;
            // point -> point (sort y)
            if (ch_cnt % 2)
                internal::blockSortWrap<KT, VT, PT, NT, ISN, BSN, MSN, NC>(read_flag, blk_sz, blk_bgn, order, inX, inY,
                                                                           zone, extPointBuf0 + 2 * MSN, extNodeBuf1,
                                                                           tmp, extPointBuf1 + MSN, extNodeBuf2 + MSN);
            else
                internal::blockSortWrap<KT, VT, PT, NT, ISN, BSN, MSN, NC>(read_flag, blk_sz, blk_bgn, order, inX, inY,
                                                                           zone, extPointBuf0 + MSN, extNodeBuf1, tmp,
                                                                           extPointBuf1 + MSN, extNodeBuf2 + MSN);
        }
        internal::dataMove<KT, VT, PT, NT, NC>(node_real_sz, real_bgn, extPointBuf0 + MSN, extNodeBuf1, extNodeBuf0,
                                               extNodeBuf2);
        real_bgn += node_real_sz;
        bgn_buf[++node_ch_cnt] = real_bgn;
        node_real_sz = (node_real_sz + NC - 1) / NC;
    }
    if (node_ch_cnt % 2)
        extNodeBuf0[real_bgn] = extNodeBuf2[real_bgn];
    else
        extNodeBuf0[real_bgn] = extNodeBuf1[real_bgn];
    real_bgn++;
    extPointBuf1[sz] = real_sz;      // point size
    extPointBuf1[sz + 1] = real_bgn; // node size
#ifndef __SYNTHESIS__
    std::cout << "End STRTree_Kernel\n";
#endif
}
} // geospatial
} // data_analytics
} // xf
#endif
