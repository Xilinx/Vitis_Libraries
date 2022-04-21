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

#ifndef _XF_DATA_ANALYTICS_GEOSPATIAL_KNN_HPP_
#define _XF_DATA_ANALYTICS_GEOSPATIAL_KNN_HPP_

#include "xf_data_analytics/dataframe/csv_parser_v2.hpp"
#include "similarity/sort_top_k.hpp"
#include <cmath>
#include <algorithm>
#include <stdint.h>

#ifndef __SYNTHESIS__
#include <iostream>
#endif

namespace xf {
namespace data_analytics {
namespace geospatial {
namespace internal {

union toFloat {
    uint32_t a;
    float b;
};

/**
 * @brief split object stream of csv parser into seperate streams for distance computation.
 *
 * @param obj_strm stream of csv parser output object
 * @param idx_strm stream of point index
 * @param x_strm stream of point x coordinate
 * @param y_strm stream of point y coordinate
 * @param strm_out_end stream end flags
 */
void split_csv_stream(hls::stream<xf::data_analytics::dataframe::Object>& obj_strm,
                      hls::stream<ap_uint<32> >& idx_strm,
                      hls::stream<float>& x_strm,
                      hls::stream<float>& y_strm,
                      hls::stream<bool>& strm_out_end) {
    xf::data_analytics::dataframe::Object tmp = obj_strm.read();
    toFloat cvt;
    while (tmp.get_type() != 15) {
#pragma HLS loop_tripcount min = 1000 max = 1000 avg = 1000
#pragma HLS pipeline II = 1

        cvt.a = tmp.get_data();
        x_strm.write(cvt.b); // get longtitude

        tmp = obj_strm.read();
        cvt.a = tmp.get_data();
        y_strm.write(cvt.b); // get latitude

        tmp = obj_strm.read(); // get index from FEOL
        idx_strm.write(tmp.get_data().range(31, 0));

        strm_out_end.write(0);

        tmp = obj_strm.read();
    }
    strm_out_end.write(1);
}

/**
 * @brief Compute distance between input point and base point. Euclidean distance is used.
 *
 * @param x_strm stream of input point x coordinate
 * @param y_strm stream of input point y coordinate
 * @param strm_in_end input stream end flag
 * @param base_x base point x coordinate
 * @param base_y base point y coordinate
 * @param distance_strm stream of output distance
 * @param strm_out_end output stream end flag
 */
void compute_distance(hls::stream<float>& x_strm,
                      hls::stream<float>& y_strm,
                      hls::stream<bool>& strm_in_end,
                      float base_x,
                      float base_y,
                      hls::stream<float>& distance_strm,
                      hls::stream<bool>& strm_out_end) {
    bool end = strm_in_end.read();
    while (!end) {
#pragma HLS loop_tripcount min = 1000 max = 1000 avg = 1000
#pragma HLS pipeline II = 1
#pragma HLS pipeline style = flp
        float x = x_strm.read();
        float y = y_strm.read();

        float dist = std::sqrt((x - base_x) * (x - base_x) + (y - base_y) * (y - base_y));

        distance_strm.write(dist);
        strm_out_end.write(0);
        end = strm_in_end.read();
    }
    strm_out_end.write(1);
}

/**
 * @brief Postprocess and write result to DDR
 *
 * @tparam CSV_PU_NUM num of csv parser used to parallel parse csv data
 * @param line_cnt_buf num of csv lines that each pu processes
 * @param sorted_distance_strm input stream of sorted distance
 * @param sorted_index_strm input stream of sorted point index
 * @param strm_in_end input stream end flag
 * @param distance output sorted top-k distance
 * @param index output sorted top-k index
 */
template <int CSV_PU_NUM = 2>
void write2mem(ap_uint<32> line_cnt_buf[CSV_PU_NUM / 2][2],
               hls::stream<float>& sorted_distance_strm,
               hls::stream<ap_uint<32> >& sorted_index_strm,
               hls::stream<bool>& strm_in_end,
               float* distance,
               uint32_t* index) {
    int cnt = 0;
    bool end = strm_in_end.read();
    while (!end) {
#pragma HLS loop_tripcount min = 5 max = 5 avg = 5
#pragma HLS pipeline
        distance[cnt] = sorted_distance_strm.read();

        ap_uint<32> tmp = sorted_index_strm.read();
        ap_uint<3> pu_idx = tmp.range(31, 29);
        uint32_t line_idx = tmp.range(28, 0);
        // #ifndef __SYNTHESIS__
        //         std::cout << "======================================================" << std::endl;
        //         std::cout << "Index stream: " << std::hex << tmp << std::dec << std::endl;
        //         std::cout << "PU index: " << pu_idx << std::endl;
        //         std::cout << "Line index before: " << line_idx << std::endl;
        // #endif

        for (ap_uint<3> i = 0; i < pu_idx; ++i) {
#pragma HLS loop_tripcount min = 0 max = 7 avg = 4
#pragma HLS pipeline II = 1
            line_idx += line_cnt_buf[i.range(2, 1)][i[0]];
            // #ifndef __SYNTHESIS__
            //             std::cout << "Details pu_grp/pu_idx/line_total: " << i.range(2, 1) << " / " << i[0] <<
            //             std::dec << " / "
            //                       << line_cnt_buf[i.range(2, 1)][i[0]] << std::endl;
            //             std::cout << "Line index after: " << line_idx << std::endl;
            // #endif
        }

        index[cnt] = line_idx;
        /*
        #ifndef __SYNTHESIS__
                std::cout << "Top-" << (cnt + 1) << " distance: " << distance[cnt] << "; line index: " << index[cnt] <<
        "."
                          << std::endl;
        #endif
        */
        cnt++;
        end = strm_in_end.read();
    }
}

} // end namespace internal

/**
 * @brief K Nearest Neighbors(KNN): find nearest-K points for a given base point.
 *
 * @tparam CSV_PU_NUM num of csv parser core used to parallel parse csv data, only support 2/4/8
 * @tparam MAX_SORT_NUM the max number of the sequence can be sorted, should be less than 1024
 * @param csv_buf input csv data
 * @param schema input csv schema
 * @param base_x base point x coordinate
 * @param base_y base point y coordinate
 * @param k num of nearest point, k <= MAX_SORT_NUM
 * @param sorted_dist_buf output distance of nearest-K points
 * @param sorted_idx_buf output index of nearest-K points
 */
template <int CSV_PU_NUM = 2, int MAX_SORT_NUM = 8>
void knn(ap_uint<128>* csv_buf,
         ap_uint<8>* schema,
         float base_x,
         float base_y,
         int k,
         float* sorted_dist_buf,
         uint32_t* sorted_idx_buf) {
    // intermediate connections
    hls::stream<xf::data_analytics::dataframe::Object> obj_strm;
    hls::stream<bool> split_strm_out_end, dist_strm_out_end, strm_out_end;
    hls::stream<float> x_strm, y_strm, distance_strm, sorted_distance_strm;
    hls::stream<ap_uint<32> > index_strm, sorted_index_strm;

#pragma HLS stream depth = 32 variable = obj_strm
#pragma HLS stream depth = 32 variable = x_strm
#pragma HLS stream depth = 32 variable = y_strm
#pragma HLS stream depth = 32 variable = index_strm
#pragma HLS stream depth = 32 variable = split_strm_out_end
#pragma HLS stream depth = 32 variable = distance_strm
#pragma HLS stream depth = 32 variable = dist_strm_out_end
#pragma HLS stream depth = 32 variable = sorted_index_strm
#pragma HLS stream depth = 32 variable = sorted_distance_strm
#pragma HLS stream depth = 32 variable = strm_out_end

    ap_uint<32> line_cnt_buf[CSV_PU_NUM / 2][2];
#pragma HLS array_partition variable = line_cnt_buf dim = 0
#pragma HLS dataflow
    // call primitives
    xf::data_analytics::dataframe::csvParser<CSV_PU_NUM>(csv_buf, schema, line_cnt_buf, obj_strm);
    internal::split_csv_stream(obj_strm, index_strm, x_strm, y_strm, split_strm_out_end);
    internal::compute_distance(x_strm, y_strm, split_strm_out_end, base_x, base_y, distance_strm, dist_strm_out_end);

    xf::graph::sortTopK<float, ap_uint<32>, MAX_SORT_NUM>(
        index_strm, distance_strm, dist_strm_out_end, sorted_index_strm, sorted_distance_strm, strm_out_end, k, false);
    internal::write2mem<CSV_PU_NUM>(line_cnt_buf, sorted_distance_strm, sorted_index_strm, strm_out_end,
                                    sorted_dist_buf, sorted_idx_buf);
}
} // end namespace geospatial
} // end namespace data_analytics
} // end namespace xf

#endif