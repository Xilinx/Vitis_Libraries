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
demosaicGraph<1> demo[CORES] = {{20, 0, 0}}; //, {22, 0, 1}, {24, 0, 2}, {26, 0, 3}};

// initialize and run the dataflow graph
#if defined(__AIESIM__) || defined(__X86SIM__)
int main(int argc, char** argv) {
    uint8_t* inputData = (uint8_t*)GMIO::malloc(TILE_WINDOW_SIZE * CORES);
    uint8_t* outputData = (uint8_t*)GMIO::malloc(TILE_WINDOW_SIZE_RGBA * CORES);

    memset(inputData, 0, TILE_WINDOW_SIZE * CORES);
    memset(outputData, 0, TILE_WINDOW_SIZE_RGBA * CORES);
    xf::cv::aie::xfSetTileWidth(inputData, TILE_WIDTH);
    xf::cv::aie::xfSetTileHeight(inputData, TILE_HEIGHT);

    xf::cv::aie::xfSetTileWidth(outputData, TILE_WIDTH);
    xf::cv::aie::xfSetTileHeight(outputData, TILE_HEIGHT);

    uint8_t* dataIn = (uint8_t*)xf::cv::aie::xfGetImgDataPtr(inputData);
    for (int i = 0; i < TILE_ELEMENTS; i++) dataIn[i] = rand() % 256;

    for (int i = 0; i < CORES; i++) {
        // Empty
        demo[i].init();
        demo[i].run(NUM_TILES / CORES);

        demo[i].in1[i].gm2aie_nb(inputData + i * TILE_WINDOW_SIZE, TILE_WINDOW_SIZE);
        demo[i].out1[i].aie2gm_nb(outputData + i * TILE_WINDOW_SIZE_RGBA, TILE_WINDOW_SIZE_RGBA);
    }

    for (int i = 0; i < CORES; i++) {
        demo[i].out1[i].wait();
        demo[i].end();
    }

    return 0;
}
#endif
