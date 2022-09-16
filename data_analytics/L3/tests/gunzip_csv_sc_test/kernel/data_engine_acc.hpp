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

class data_engine_acc : public VPP_ACC<data_engine_acc, /*NCU=*/1> {
    // binding information
    ZERO_COPY(csvBuf);
    ZERO_COPY(szBuf);
    ZERO_COPY(cfgBuf);
    ZERO_COPY(firValue);

    SYS_PORT(csvBuf, bank0);
    SYS_PORT(szBuf, bank0);
    SYS_PORT(cfgBuf, bank0);
    SYS_PORT(firValue, bank0);

    SYS_PORT_PFM(u50, csvBuf, HBM[0]);
    SYS_PORT_PFM(u50, szBuf, HBM[1]);
    SYS_PORT_PFM(u50, cfgBuf, HBM[2]);
    SYS_PORT_PFM(u50, firValue, HBM[3]);

   public:
    /**
     * @brief top of kernel
     *
     * @param csvBuf input CSV files
     * @param firValue result buffer
     * @param szBuf size of each files
     * @param cfgBuf kernel configurations
     */
    static void compute(ap_uint<128>* csvBuf, ap_uint<256>* firValue, ap_uint<64>* szBuf, ap_uint<64>* cfgBuf);

    /**
     * @brief top of kernel
     *
     * @param csvBuf input CSV files
     * @param firValue result buffer
     * @param szBuf size of each files
     * @param cfgBuf kernel configurations
     */
    static void hls_kernel(ap_uint<128>* csvBuf, ap_uint<256>* firValue, ap_uint<64>* szBuf, ap_uint<64>* cfgBuf);
};
