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

#pragma once
#include "vpp_acc.hpp"
#include "ap_int.h"

class aes256CbcDecrypt : public VPP_ACC<aes256CbcDecrypt, /*NCU=*/1> {
    ZERO_COPY(hb_in, hb_in[hb_in_size]);
    ZERO_COPY(hb_out, hb_out[hb_out_size]);

    SYS_PORT(hb_in, DDR[0]);
    SYS_PORT(hb_out, DDR[0]);

    SYS_PORT_PFM(u50, hb_in, HBM[0]);
    SYS_PORT_PFM(u50, hb_out, HBM[0]);

   public:
    static void compute(int hb_in_size, int hb_out_size, ap_uint<128>* hb_in, ap_uint<128>* hb_out);

    static void hls_kernel(int hb_in_size, int hb_out_size, ap_uint<128>* hb_in, ap_uint<128>* hb_out);
};
