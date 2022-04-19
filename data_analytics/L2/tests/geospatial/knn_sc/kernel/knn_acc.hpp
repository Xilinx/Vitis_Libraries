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

#pragma once
#include "vpp_acc.hpp"
#include <ap_int.h>
#include <stdint.h>

enum { CSV_PU_NM = 2 };
enum { MAX_SORT_NM = 8 };

class knn_acc : public VPP_ACC<knn_acc, /*NCU=*/1> {
    ZERO_COPY(csv_buf);
    ZERO_COPY(schema_buf);
    ZERO_COPY(sorted_dist_buf);
    ZERO_COPY(sorted_idx_buf);

#ifdef USE_U2
    SYS_PORT(csv_buf, bank0);
    SYS_PORT(schema_buf, bank0);
    SYS_PORT(sorted_dist_buf, bank0);
    SYS_PORT(sorted_idx_buf, bank0);
#else
    SYS_PORT(csv_buf, HBM[0]);
    SYS_PORT(schema_buf, HBM[1]);
    SYS_PORT(sorted_dist_buf, HBM[2]);
    SYS_PORT(sorted_idx_buf, HBM[3]);
#endif

   public:
    static void compute(ap_uint<128>* csv_buf,
                        ap_uint<8>* schema_buf,
                        float base_x,
                        float base_y,
                        int k,
                        float* sorted_dist_buf,
                        uint32_t* sorted_idx_buf);

    static void hls_kernel(ap_uint<128>* csv_buf,
                           ap_uint<8>* schema_buf,
                           float base_x,
                           float base_y,
                           int k,
                           float* sorted_dist_buf,
                           uint32_t* sorted_idx_buf);
};
