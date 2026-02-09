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
#include <common/xf_aie_utils.hpp>

using namespace adf;
// instantiate adf dataflow graph
blacklevelGraph bl;

// initialize and run the dataflow graph
#if defined(__AIESIM__) || defined(__X86SIM__)

int main(int argc, char** argv) {
    uint8_t* inputData = (uint8_t*)GMIO::malloc(TILE_WINDOW_SIZE);
    uint8_t* outputData = (uint8_t*)GMIO::malloc(TILE_WINDOW_SIZE);

    memset(inputData, 0, TILE_WINDOW_SIZE);
    memset(outputData, 0, TILE_WINDOW_SIZE);
    xf::cv::aie::xfSetTileWidth(inputData, TILE_WIDTH);
    xf::cv::aie::xfSetTileHeight(inputData, TILE_HEIGHT);

    xf::cv::aie::xfSetTileWidth(outputData, TILE_WIDTH);
    xf::cv::aie::xfSetTileHeight(outputData, TILE_HEIGHT);

    uint8_t* dataIn = (uint8_t*)xf::cv::aie::xfGetImgDataPtr(inputData);
    for (int i = 0; i < TILE_ELEMENTS; i++) dataIn[i] = rand() % 256;

    uint8_t BlackLevel = 32;
    const int MaxLevel = 255;
    float MulValue1 = (float)((float)MaxLevel / (MaxLevel - BlackLevel));
    uint16_t MulValue = 37470; // Q(1.15)

    bl.init();
    bl.update(bl.blacklevel, BlackLevel);
    bl.update(bl.mulfact, MulValue);
    bl.run(1);

    bl.in1.gm2aie_nb(inputData, TILE_WINDOW_SIZE);
    bl.out1.aie2gm_nb(outputData, TILE_WINDOW_SIZE);
    bl.out1.wait();
    bl.end();
    return 0;
}
#endif
