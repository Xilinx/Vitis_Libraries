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

#include "xf_DataAnalytics/regression/linearRegressionTrain.hpp"

extern void dut(ap_uint<512> input[302], ap_uint<512> output[302]) {
    xf::data_analytics::regression::linearLeastSquareRegressionSGDTrain<512, 8, 25, 64>(input, output);
}

#ifndef __SYNTHESIS__
int main() {
    // input data
    const int rows = 100;
    const int cols = 23;
    const float fraction = 1.0;
    const bool ifJump = false;
    const int bucketSize = 1;
    const ap_uint<32> seed = 42;
    double stepSize = 1.0;
    double tolerance = 0.001;
    bool withIntercept = false;
    ap_uint<32> maxIter = 100;
    const ap_uint<32> offset = 2;

    // training data generation
    double table[rows * cols + 16];
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols - 1; j++) {
            table[16 + i * cols + j] = (i + j) * 0.01;
        }
        table[16 + i * cols + cols - 1] = (i + i + cols - 2) * (cols - 1) / 2.0 * 0.01 * 0.3;
    }

    *(ap_uint<32>*)(table + 0) = seed;
    *(double*)(table + 1) = stepSize;
    *(double*)(table + 2) = tolerance;
    *(ap_uint<32>*)(table + 3) = withIntercept ? 1 : 0;
    *(ap_uint<32>*)(table + 4) = maxIter;
    *(ap_uint<32>*)(table + 5) = offset;
    *(ap_uint<32>*)(table + 6) = rows;
    *(ap_uint<32>*)(table + 7) = cols;
    *(ap_uint<32>*)(table + 8) = bucketSize;
    *(float*)(table + 9) = fraction;
    *(ap_uint<32>*)(table + 10) = ifJump ? 1 : 0;

    // training golden
    double weightGolden[cols - 1] = {0.2648105355276227,  0.26800086938263273, 0.2711912097095614,  0.27438156857003015,
                                     0.27757189527487824, 0.28076224618176043, 0.28395259783942906, 0.2871429436626949,
                                     0.29033329314770484, 0.2935236422350576,  0.29671397291430046, 0.29990431851379545,
                                     0.30309465138274605, 0.30628501021078175, 0.3094753365314273,  0.3126656902537365,
                                     0.3158560567229295,  0.31904640219484903, 0.3222367284342532,  0.3254270788568063,
                                     0.3286174240856112,  0.3318077533761295};
    // result of training
    double res[200];
    dut((ap_uint<512>*)table, (ap_uint<512>*)res);

    // check result
    bool tested = true;
    for (int i = 0; i < cols - 1; i++) {
        double diff = res[i] - weightGolden[i];
        double rel_err = diff / weightGolden[i];
        if (rel_err < -0.00001 || rel_err > 0.00001) {
            std::cout << i << " th weight: " << res[i] << std::endl;
            tested = false;
        }
    }
    // return test result
    if (tested) {
        return 0;
    } else {
        return 1;
    }
}
#endif
