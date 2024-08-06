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

// instantiate adf dataflow graph
thresholdGraph mygraph;

// initialize and run the dataflow graph
#if defined(__AIESIM__) || defined(__X86SIM__)
#include <common/xf_aie_utils.hpp>

int main(int argc, char** argv) {
    int BLOCK_SIZE_in_Bytes = TILE_WINDOW_SIZE;

    uint8_t* inputData = (uint8_t*)GMIO::malloc(BLOCK_SIZE_in_Bytes);
    uint8_t* outputData = (uint8_t*)GMIO::malloc(BLOCK_SIZE_in_Bytes);

    memset(inputData, 0, BLOCK_SIZE_in_Bytes);
    xf::cv::aie::xfSetTileWidth(inputData, TILE_WIDTH);
    xf::cv::aie::xfSetTileHeight(inputData, TILE_HEIGHT);

    uint8_t* dataIn = (uint8_t*)xf::cv::aie::xfGetImgDataPtr(inputData);
    for (int i = 0; i < TILE_ELEMENTS; i++) {
        dataIn[i] = rand() % 256;
    }

    int16_t thresh_val = 100;
    int16_t max_val = 50;
    mygraph.init();

    mygraph.update(mygraph.threshVal, thresh_val);
    mygraph.update(mygraph.maxVal, max_val);

    mygraph.run(1);
    mygraph.in1.gm2aie_nb(inputData, BLOCK_SIZE_in_Bytes);
    mygraph.out1.aie2gm_nb(outputData, BLOCK_SIZE_in_Bytes);
    mygraph.out1.wait();

    mygraph.end();
    return 0;
}
#endif
