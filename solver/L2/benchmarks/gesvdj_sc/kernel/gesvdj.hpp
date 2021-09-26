/*
 * Copyright 2019-2021 Xilinx, Inc.
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
#include "vpp_compiler.h"
#include "vpp_acc.hpp"

#define MA 16 // Symmetric Matrix Row size
#define NA MA // Symmetric Matrix Col size

class xgesvdj : public VPP_ACC<xgesvdj, /*NCU=*/1> {
    ACCESS_PATTERN(dataA, RANDOM);
    ACCESS_PATTERN(sigma, RANDOM);
    ACCESS_PATTERN(dataU, RANDOM);
    ACCESS_PATTERN(dataV, RANDOM);
    SYS_PORT(dataA, DDR[0]);
    SYS_PORT(sigma, DDR[0]);
    SYS_PORT(dataU, DDR[0]);
    SYS_PORT(dataV, DDR[0]);

    // SYS_PORT(dataA, HBM[0]);
    // SYS_PORT(sigma, HBM[2]);
    // SYS_PORT(dataU, HBM[4]);
    // SYS_PORT(dataV, HBM[6]);

   public:
    static void compute(
        double dataA[NA * NA], double sigma[NA * NA], double dataU[NA * NA], double dataV[NA * NA], int matrixSize);
    // double* dataA, double* sigma, double* dataU, double* dataV, int matrixSize);

    static void gesvdj(
        double dataA[NA * NA], double sigma[NA * NA], double dataU[NA * NA], double dataV[NA * NA], int matrixSize);
    // double* dataA, double* sigma, double* dataU, double* dataV, int matrixSize);
};

void gesvdj_wrapper(double* dataA, double* sigma, double* dataU, double* dataV, int matrixSize);
