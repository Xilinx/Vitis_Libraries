/*
 * Copyright 2021 Xilinx, Inc.
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

#include "graph.h"
#include <iostream>
//#include <fstream>

myGraph ccm_graph;

#if defined(__AIESIM__) || defined(__X86SIM__)
#include <common/xf_aie_utils.hpp>

int main(int argc, char** argv) {
    int BLOCK_SIZE_in_Bytes = TILE_WINDOW_SIZE_IN;

    T* _data = (T*)malloc(TILE_WINDOW_SIZE_IN);

    T* outputData = (T*)malloc(TILE_WINDOW_SIZE_OUT);

    int16_t* coeffs = (int16_t*)malloc(16 * sizeof(int16_t) + 3 * 3 * sizeof(int16_t));
    uint16_t* coeffs_awb = (uint16_t*)(coeffs + 16);
    int16_t* coeffs_ccm = coeffs;
    memset(coeffs, 0, (16 + 9));
    // awb rtps
    int min[4], max[4];
    min[0] = 36;
    min[1] = 25;
    min[2] = 14;
    min[3] = 0;
    max[0] = 172;
    max[1] = 93;
    max[2] = 234;
    max[3] = 0;
    compute_awb_params(coeffs_awb, min, max);

    memset(_data, 0, TILE_WINDOW_SIZE_IN);

    xf::cv::aie::xfSetTileWidth(_data, TILE_WIDTH);
    xf::cv::aie::xfSetTileHeight(_data, TILE_HEIGHT);

    T* dataIn = (T*)xf::cv::aie::xfGetImgDataPtr(_data);

    for (int i = 0; i < TILE_ELEMENTS_IN / 4; i++) {
        for (int j = 0; j < 4; j++) {
            dataIn[i * 4 + j] = rand() % 256;
        }
    }

    // c-ref
    T ref_out[TILE_ELEMENTS_OUT];
    awbnorm_colorcorrectionmatrix(dataIn, ref_out, coeffs_awb, coeffs_ccm);

    ccm_graph.init();
    for (int j = 0; j < 16 + 9; j++) printf("host_coeff: %d \n", (int)coeffs[j]);

    ccm_graph.update(ccm_graph.coeff, coeffs, (16 + 9));
    ccm_graph.run(1);

    ccm_graph.end();
    return 0;
}

#endif
