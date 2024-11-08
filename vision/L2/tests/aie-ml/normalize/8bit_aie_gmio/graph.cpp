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

using namespace adf;
// instantiate adf dataflow graph
NormalizeGraph norm;

// initialize and run the dataflow graph
#if defined(__AIESIM__) || defined(__X86SIM__)
#include <common/xf_aie_utils.hpp>
int main(int argc, char** argv) {
    int BLOCK_SIZE_in_Bytes = TILE_WINDOW_SIZE_IN;

    uint8_t* inputData = (uint8_t*)GMIO::malloc(BLOCK_SIZE_in_Bytes);
    int8_t* outputData = (int8_t*)GMIO::malloc(BLOCK_SIZE_in_Bytes);

    memset(inputData, 0, BLOCK_SIZE_in_Bytes);
    xf::cv::aie::xfSetTileWidth(inputData, TILE_WIDTH_IN);
    xf::cv::aie::xfSetTileOutTWidth(inputData, TILE_WIDTH_OUT);
    xf::cv::aie::xfSetTileHeight(inputData, TILE_HEIGHT_IN);

    uint8_t* dataIn = (uint8_t*)xf::cv::aie::xfGetImgDataPtr(inputData);
    for (int i = 0; i < TILE_ELEMENTS_IN; i++) {
        dataIn[i] = rand() % 256;
    }

    std::array<float, 4> mean = {104, 107, 123, 0};
    std::array<float, 4> std_deviation = {2, 2, 2, 0};

    unsigned char alpha[4];
    char beta[4];

    get_alpha_beta<0, 4>(mean, std_deviation, alpha, beta);
    std::vector<int16_t> coeff({alpha[0], alpha[1], alpha[2], alpha[3], beta[0], beta[1], beta[2], beta[3]});

    norm.init();
    norm.update(norm.coeff, coeff.data(), coeff.size());
    norm.run(1);
    norm.in1.gm2aie_nb(inputData, BLOCK_SIZE_in_Bytes);
    norm.out1.aie2gm_nb(outputData, BLOCK_SIZE_in_Bytes);
    norm.out1.wait();
    norm.end();

    return 0;
}
#endif
