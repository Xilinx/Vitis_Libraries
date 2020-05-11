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

#include "xf_DataAnalytics/regression/gradient.hpp"
#include "xf_DataAnalytics/regression/linearRegressionTrain.hpp"

extern void dut(ap_uint<512> input[302], ap_uint<512> output[302]) {
    xf::data_analytics::regression::LASSORegressionSGDTrain<512, 8, 25, 64>(input, output);
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
    ap_uint<32> maxIter = 99;
    const ap_uint<32> offset = 2;
    double regVal = 0.1;

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
    *(double*)(table + 11) = regVal;

    // training golden
    double weightGolden[cols - 1] = {0.17378717673636357, 0.18429218206666115, 0.19479719980974586, 0.2053022068754798,
                                     0.21580721874474093, 0.2263122340588962,  0.23681723067533744, 0.24732224644881173,
                                     0.25782726865771066, 0.2683322646505349,  0.2788372865485966,  0.2893422869269156,
                                     0.29984730573249074, 0.3103522922935321,  0.32085731417615854, 0.3313623342311564,
                                     0.34186734339762165, 0.35237236166588226, 0.3628773537166093,  0.3733823629723242,
                                     0.38388738819319396, 0.39439239190673836};
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
