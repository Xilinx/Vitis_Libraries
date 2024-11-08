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
#include "config.h"
#include <cmath>
using namespace adf;
// instantiate adf dataflow graph
DenormalizeGraph denorm;

// initialize and run the dataflow graph
#if defined(__AIESIM__) || defined(__X86SIM__)
#include <common/xf_aie_utils.hpp>
int main(int argc, char** argv) {
    int BLOCK_SIZE_in_Bytes = TILE_WINDOW_SIZE_IN;

    uint8_t* inputData = (uint8_t*)GMIO::malloc(BLOCK_SIZE_in_Bytes);
    uint8_t* outputData = (uint8_t*)GMIO::malloc(BLOCK_SIZE_in_Bytes);

    memset(inputData, 0, BLOCK_SIZE_in_Bytes);
    xf::cv::aie::xfSetTileWidth(inputData, TILE_WIDTH_IN);
    xf::cv::aie::xfSetTileOutTWidth(inputData, TILE_WIDTH_OUT);
    xf::cv::aie::xfSetTileHeight(inputData, TILE_HEIGHT_IN);

    uint8_t* dataIn = (uint8_t*)xf::cv::aie::xfGetImgDataPtr(inputData);
    for (int i = 0; i < TILE_ELEMENTS_IN; i++) {
        dataIn[i] = rand() % 256;
    }

    std::array<float, 4> mean = {0.2, 0.7, 0.8, 0};
    std::array<float, 4> std_deviation = {0.3, 0.5, 0.6, 0};

    unsigned char alpha[4];
    char beta[4];

    get_alpha_beta<8, 5>(mean, std_deviation, alpha, beta);
    std::vector<int16_t> coeff({alpha[0], alpha[1], alpha[2], alpha[3], beta[0], beta[1], beta[2], beta[3]});

    denorm.init();
    denorm.update(denorm.coeff, coeff.data(), coeff.size());
    denorm.run(1); // input.txt has data for 224 tiles and assumes metadata for
                   // TILE_HEIGHT 2
    denorm.in1.gm2aie_nb(inputData, BLOCK_SIZE_in_Bytes);
    denorm.out1.aie2gm_nb(outputData, BLOCK_SIZE_in_Bytes);
    denorm.out1.wait();

    denorm.end();

    return 0;
}
#endif
