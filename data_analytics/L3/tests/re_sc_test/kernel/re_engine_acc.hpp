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

#pragma once
#include "vpp_acc.hpp"
#include <ap_int.h>

class re_engine_acc : public VPP_ACC<re_engine_acc, /*NCU=*/8> {
    ZERO_COPY(msg_buff);
    ZERO_COPY(len_buff);
    ZERO_COPY(out_buff);

    SYS_PORT(msg_buff, (HBM[0] : HBM[1] : HBM[2] : HBM[3] : HBM[4] : HBM[5] : HBM[6] : HBM[7]));
    SYS_PORT(len_buff, (HBM[0] : HBM[1] : HBM[2] : HBM[3] : HBM[4] : HBM[5] : HBM[6] : HBM[7]));
    SYS_PORT(out_buff, (HBM[0] : HBM[1] : HBM[2] : HBM[3] : HBM[4] : HBM[5] : HBM[6] : HBM[7]));

   public:
    static void compute(ap_uint<64>* msg_buff, ap_uint<16>* len_buff, ap_uint<32>* out_buff);

    static void hls_kernel(ap_uint<64>* msg_buff, ap_uint<16>* len_buff, ap_uint<32>* out_buff);
};
