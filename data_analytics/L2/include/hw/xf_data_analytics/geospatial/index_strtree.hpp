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

#ifndef _INDEX_STRTREE_HPP_
#define _INDEX_STRTREE_HPP_

#include <ap_int.h>
#include <hls_stream.h>
#include <hls_math.h>

namespace xf {
namespace data_analytics {
namespace geospatial {
namespace internal {

inline int ceil(int s, int d) {
    int r = s / d;
    if (s % d) r++;
    return r;
}

template <int N>
inline int calcuSlice(int size) {
    float dev = ceil(size, N);
#ifndef __SYNTHESIS__
    return std::ceil(std::sqrt(dev));
#else
    return hls::ceil(hls::sqrt(dev));
#endif
}

template <typename KT, typename VT>
void strmConv(int i_blk_sz,
              hls::stream<KT>& i_key_strm,
              hls::stream<VT>& i_val_strm,
              hls::stream<bool>& i_end_strm,
              hls::stream<KT>& o_key_strm,
              hls::stream<VT>& o_val_strm,
              hls::stream<bool>& o_end_strm) {
    bool end = i_end_strm.read();
    unsigned cnt = 0;
    bool end_flag = false;
loop_strmConv:
    while (!end) {
#pragma HLS pipeline ii = 1
#pragma HLS loop_tripcount max = 1000 min = 1000
        // if (!end_flag) {
        KT key = i_key_strm.read();
        VT val = i_val_strm.read();
        o_key_strm.write(key);
        o_val_strm.write(val);
        // std::cout << "In Point: i=" << cnt << ", x=" << key(31, 0) << ", key=" << key(63, 32) << ", id=" << val
        //          << std::endl;
        o_end_strm.write(false);
        end = i_end_strm.read();
        //} else {
        //    o_end_strm.write(true);
        //}
        // if (!end_flag && (cnt == i_blk_sz - 1)) {
        //    cnt = 0;
        //    end_flag = 1;
        //} else {
        end_flag = 0;
        cnt += 1;
    }
    //}
    if (cnt > 0) o_end_strm.write(true);
}

template <typename DT>
void readMem2Strm(unsigned sz, unsigned bgn, DT* i_buf, hls::stream<DT>& o_strm) {
    for (unsigned i = 0; i < sz; i++) {
#pragma HLS pipeline ii = 1
#pragma HLS loop_tripcount max = 1000 min = 1000
        DT tmp = i_buf[i + bgn];
        o_strm.write(tmp);
    }
}

template <typename KT, typename VT, typename PT, typename NT>
void mem2StrmConvCore(int flag, // 0: quantify & x sort, 1: y sort, 2: x sort (node)
                      int sz,
                      // int sn,
                      // input when flag = 0
                      hls::stream<double>& i_xs_strm,
                      hls::stream<double>& i_ys_strm,
                      double* i_zone,
                      // input when flag = 1
                      hls::stream<PT>& i_point_strm,
                      // input when flag = 2
                      hls::stream<NT>& i_node_strm,
                      // output
                      unsigned& o_real_sz,
                      hls::stream<KT>& o_key_strm,
                      hls::stream<VT>& o_val_strm,
                      hls::stream<bool>& o_end_strm) {
    bool end_flag = 0;
    double xmin, xmax, ymin, ymax, dx, dy;
    const uint64_t QC = 4294967295; // Quantization coefficient
    if (flag == 0) {
        xmin = i_zone[0];
        xmax = i_zone[1];
        ymin = i_zone[2];
        ymax = i_zone[3];
        dx = xmax - xmin;
        dy = ymax - ymin;
        o_real_sz = 0;
    }

loop_mem2StrmConv:
    for (unsigned i = 0; i < sz; i++) {
#pragma HLS pipeline ii = 1
#pragma HLS loop_tripcount max = 1000 min = 1000
        KT key;
        VT val;
        double x = i_xs_strm.read();
        double y = i_ys_strm.read();
        PT tmp = i_point_strm.read();
        NT tmp2 = i_node_strm.read();
        if (flag == 0) {
            if ((x > xmin) && (x < xmax) && (y > ymin) && (y < ymax)) {
                uint32_t qx = (x - xmin) / dx * QC;
                uint32_t qy = (y - ymin) / dy * QC;
                key(63, 32) = qx;
                key(31, 0) = qy;
                val = i;
                o_key_strm.write(key);
                o_val_strm.write(val);
                o_end_strm.write(false);
                o_real_sz++;
            }
            // i++;
        } else if (flag == 1) {
            // if (!end_flag) {
            key(31, 0) = tmp(63, 32);
            key(63, 32) = tmp(31, 0);
            val = tmp(95, 64);
            // std::cout << "mem2StrmConv1: i=" << i << ", x=" << tmp(63, 32) << ", y=" << tmp(31, 0)
            //          << ", id=" << tmp(95, 64) << std::endl;
            o_key_strm.write(key);
            o_val_strm.write(val);
            o_end_strm.write(false);
        } else {
            // std::cout << "mem2StrmConv2: i=" << i << ", level=" << tmp2(31, 0) << ", xmin=" << tmp2(63, 32)
            //          << ", xmax=" << tmp2(95, 64) << ", ymin=" << tmp2(127, 96) << ", ymax=" << tmp2(159, 128)
            //          << ", addr=" << tmp2(191, 160) << std::endl;
            uint32_t qx = tmp2(63, 33) + tmp2(95, 65);
            uint32_t qy = tmp2(127, 97) + tmp2(159, 129);
            key(63, 32) = qx;
            key(31, 0) = qy;
            val = i;
            o_key_strm.write(key);
            o_val_strm.write(val);
            o_end_strm.write(false);
            // i++;
        }
    }
    o_end_strm.write(true);
    // if (flag == 1 && (sz % sn)) o_end_strm.write(true);
}

template <typename KT, typename VT, typename PT, typename NT>
void mem2StrmConv(int flag, // 0: quantify & x sort, 1: y sort, 2: x sort
                  int sz,
                  int bgn,
                  // int sn,
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
                  hls::stream<KT>& o_key_strm,
                  hls::stream<VT>& o_val_strm,
                  hls::stream<bool>& o_end_strm) {
    hls::stream<double, 32> xs_strm;
    hls::stream<double, 32> ys_strm;
    hls::stream<PT, 32> point_strm;
    hls::stream<NT, 32> node_strm;
#pragma HLS dataflow
    readMem2Strm<double>(sz, 0, i_xs, xs_strm);
    readMem2Strm<double>(sz, 0, i_ys, ys_strm);
    readMem2Strm<PT>(sz, bgn, i_point_buff, point_strm);
    readMem2Strm<NT>(sz, bgn, i_node_buff, node_strm);
    mem2StrmConvCore<KT, VT, PT, NT>(flag, sz, xs_strm, ys_strm, i_zone, point_strm, node_strm, o_real_sz, o_key_strm,
                                     o_val_strm, o_end_strm);
}

} // internal
} // geospatial
} // data_analytics
} // xf
#endif
