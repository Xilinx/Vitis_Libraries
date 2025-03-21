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
// #include "ccm-params.h"
#include <common/xf_aie_utils.hpp>
using namespace adf;

// instantiate adf dataflow graph
TopPipelineGraph<1> TOP[CORES] = {{24, 0, 0}};
// TopPipelineGraph<1> TOP[CORES] = {{20, 0, 0}, {22, 0, 1}, {24, 0, 2}}; //, {24, 0, 3}};

#define SAT_U8(x) std::max(0, std::min(255, (static_cast<int>(x))))

// initialize and run the dataflow graph
#if defined(__AIESIM__) || defined(__X86SIM__)
#include "ccm-params.h"
int main(int argc, char** argv) {
    uint8_t rgain1 = 128;
    uint8_t bgain1 = 128;
    uint8_t ggain1 = 64;
    uint8_t black_level = 32;
    const int MaxLevel = 255; // 8b input value
    float MulValue1 = (float)((float)MaxLevel / (MaxLevel - black_level));
    uint16_t MulValue = 37470; // Q(1.15)

    int16_t coeffs[25];
    uint16_t* coeffs_awb = (uint16_t*)(coeffs + 16);
    int16_t* coeffs_ccm = coeffs;

    float min[4], max[4];
    min[0] = -0.5;
    min[1] = -0.5;
    min[2] = -0.5;
    min[3] = 0;
    max[0] = 230.5;
    max[1] = 223.5;
    max[2] = 250.5;
    max[3] = 0;
    ccmparams<0>(coeffs_ccm);

    uint8_t* inputData = (uint8_t*)GMIO::malloc(TILE_WINDOW_SIZE * CORES);
    uint8_t* outputData = (uint8_t*)GMIO::malloc(TILE_WINDOW_SIZE_RGBA * CORES);
    uint8_t* outputData2 = (uint8_t*)GMIO::malloc(TILE_WINDOW_SIZE_RGBA * CORES);

    memset(inputData, 0, TILE_WINDOW_SIZE * CORES);
    memset(outputData, 0, TILE_WINDOW_SIZE_RGBA * CORES);
    memset(outputData2, 0, TILE_WINDOW_SIZE_RGBA * CORES);

    xf::cv::aie::xfSetTileWidth(inputData, TILE_WIDTH);
    xf::cv::aie::xfSetTileHeight(inputData, TILE_HEIGHT);

    xf::cv::aie::xfSetTileWidth(outputData, TILE_WIDTH);
    xf::cv::aie::xfSetTileHeight(outputData, TILE_HEIGHT);
    xf::cv::aie::xfSetTileWidth(outputData2, TILE_WIDTH);
    xf::cv::aie::xfSetTileHeight(outputData2, TILE_HEIGHT);

    uint8_t* dataIn = (uint8_t*)xf::cv::aie::xfGetImgDataPtr(inputData);
    for (int i = 0; i < TILE_ELEMENTS; i++) dataIn[i] = rand() % 256;

    for (int i = 0; i < CORES; i++) {
        // Empty
        TOP[i].init();
        TOP[i].update(TOP[i].blk_val, black_level);
        TOP[i].update(TOP[i].mul_val, MulValue);
        TOP[i].update(TOP[i].rgain, rgain1);
        TOP[i].update(TOP[i].bgain, bgain1);
        TOP[i].update(TOP[i].ggain, ggain1);
        TOP[i].update(TOP[i].coeffs, coeffs, 25);
        TOP[i].run(NUM_TILES);

        TOP[i].in1[i].gm2aie_nb(inputData + i * TILE_WINDOW_SIZE, TILE_WINDOW_SIZE);
        TOP[i].out1[i].aie2gm_nb(outputData + i * TILE_WINDOW_SIZE_RGBA, TILE_WINDOW_SIZE_RGBA);
        TOP[i].out2[i].aie2gm_nb(outputData2 + i * TILE_WINDOW_SIZE_RGBA, TILE_WINDOW_SIZE_RGBA);
    }

    for (int i = 0; i < CORES; i++) {
        TOP[i].out1[i].wait();
        TOP[i].out2[i].wait();
    }

    for (int i = 0; i < CORES; i++) {
        TOP[i].end();
    }
    return 0;
}
#endif
