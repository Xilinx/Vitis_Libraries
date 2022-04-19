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

#include "knn_acc.hpp"
#include "xf_data_analytics/geospatial/knn.hpp"

void knn_acc::compute(ap_uint<128>* csv_buf,
                      ap_uint<8>* schema_buf,
                      float base_x,
                      float base_y,
                      int k,
                      float* sorted_dist_buf,
                      uint32_t* sorted_idx_buf) {
    hls_kernel(csv_buf, schema_buf, base_x, base_y, k, sorted_dist_buf, sorted_idx_buf);
}

void knn_acc::hls_kernel(ap_uint<128>* csv_buf,
                         ap_uint<8>* schema_buf,
                         float base_x,
                         float base_y,
                         int k,
                         float* sorted_dist_buf,
                         uint32_t* sorted_idx_buf) {
#pragma HLS INTERFACE m_axi offset = direct latency = 32 num_read_outstanding = 16 num_write_outstanding = \
    16 max_read_burst_length = 64 max_write_burst_length = 64 bundle = gmem0 port = csv_buf
#pragma HLS INTERFACE m_axi offset = direct latency = 32 num_read_outstanding = 16 num_write_outstanding = \
    16 max_read_burst_length = 64 max_write_burst_length = 64 bundle = gmem1 port = schema_buf
#pragma HLS INTERFACE m_axi offset = direct latency = 32 num_read_outstanding = 16 num_write_outstanding = \
    16 max_read_burst_length = 64 max_write_burst_length = 64 bundle = gmem2 port = sorted_dist_buf
#pragma HLS INTERFACE m_axi offset = direct latency = 32 num_read_outstanding = 16 num_write_outstanding = \
    16 max_read_burst_length = 64 max_write_burst_length = 64 bundle = gmem3 port = sorted_idx_buf

    xf::data_analytics::geospatial::knn<CSV_PU_NM, MAX_SORT_NM>(csv_buf, schema_buf, base_x, base_y, k, sorted_dist_buf,
                                                                sorted_idx_buf);
}
