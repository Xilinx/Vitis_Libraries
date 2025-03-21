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
myGraph accumw_graph;

#if defined(__AIESIM__) || defined(__X86SIM__)
int main(int argc, char** argv) {
    uint8_t* inputData1 = (uint8_t*)GMIO::malloc(IN_ELEM_WITH_METADATA);
    uint8_t* inputData2 = (uint8_t*)GMIO::malloc(IN_ELEM_WITH_METADATA);

    uint16_t* outputData = (uint16_t*)GMIO::malloc(ELEM_WITH_METADATA);

    memset(inputData1, 0, IN_ELEM_WITH_METADATA);
    memset(inputData2, 0, IN_ELEM_WITH_METADATA);

    memset(outputData, 0, ELEM_WITH_METADATA);
    xf::cv::aie::xfSetTileWidth(inputData1, TILE_WIDTH);
    xf::cv::aie::xfSetTileHeight(inputData1, TILE_HEIGHT);
    xf::cv::aie::xfSetTileWidth(inputData2, TILE_WIDTH);
    xf::cv::aie::xfSetTileHeight(inputData2, TILE_HEIGHT);

    xf::cv::aie::xfSetTileWidth(outputData, TILE_WIDTH);
    xf::cv::aie::xfSetTileHeight(outputData, TILE_HEIGHT);

    uint8_t* dataIn = (uint8_t*)xf::cv::aie::xfGetImgDataPtr(inputData1);
    uint8_t* dataIn2 = (uint8_t*)xf::cv::aie::xfGetImgDataPtr(inputData2);

    for (int i = 0; i < TILE_ELEMENTS; i++) {
        dataIn[i] = rand() % 256;
        dataIn2[i] = rand() % 256;
    }
    float alpha = 0.5f;
    accumw_graph.init();
    accumw_graph.update(accumw_graph.alpha, alpha);
    accumw_graph.run(1);
    accumw_graph.in1.gm2aie_nb(inputData1, IN_ELEM_WITH_METADATA);
    accumw_graph.in2.gm2aie_nb(inputData2, IN_ELEM_WITH_METADATA);
    accumw_graph.out1.wait();
    accumw_graph.end();
    return 0;
}

#endif
