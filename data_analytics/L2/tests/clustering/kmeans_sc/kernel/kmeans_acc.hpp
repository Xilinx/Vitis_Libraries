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

class kmeans_acc : public VPP_ACC<kmeans_acc, /*NCU=*/1> {
    ZERO_COPY(inputData, inputData[bufferSize]);
    ZERO_COPY(centers, centers[NC]);

    SYS_PORT(inputData, DDR[0]);
    SYS_PORT(centers, DDR[0]);

    SYS_PORT_PFM(u50, inputData, HBM[0]);
    SYS_PORT_PFM(u50, centers, HBM[0]);

   public:
    static void compute(int bufferSize, int NC, ap_uint<512>* inputData, ap_uint<512>* centers);

    static void hls_kernel(int bufferSize, int NC, ap_uint<512>* inputData, ap_uint<512>* centers);
};
