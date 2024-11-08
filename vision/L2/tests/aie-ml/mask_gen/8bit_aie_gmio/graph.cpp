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
#include "maskgen_util.h"

// instantiate adf dataflow graph
maskGenGraph maskGen;

// initialize and run the dataflow graph
#if defined(__AIESIM__) || defined(__X86SIM__)
#include "maskgen_ref.hpp"
#include <common/xf_aie_utils.hpp>
int main(int argc, char** argv) {
    int BLOCK_SIZE_in_Bytes = TILE_WINDOW_SIZE;

    uint8_t* inputData = (uint8_t*)GMIO::malloc(BLOCK_SIZE_in_Bytes);
    uint8_t* outputData = (uint8_t*)GMIO::malloc(BLOCK_SIZE_in_Bytes);
    uint8_t* refOutputData = (uint8_t*)malloc(TILE_WIDTH * TILE_HEIGHT * sizeof(uint8_t));

    memset(inputData, 0, BLOCK_SIZE_in_Bytes);
    xf::cv::aie::xfSetTileWidth(inputData, TILE_WIDTH);
    xf::cv::aie::xfSetTileHeight(inputData, TILE_HEIGHT);

    uint8_t* dataIn = (uint8_t*)xf::cv::aie::xfGetImgDataPtr(inputData);
    for (int i = 0; i < TILE_ELEMENTS; i++) {
        dataIn[i] = rand() % 256;
    }
    uint16_t fg_thresh, bg_thresh;
    uint8_t fg_thresh_track_r, bg_thresh_track_r;

    scalar_comp_utility((uint8_t)_FGTH, (uint8_t)_BGTH, (uint8_t)_MIN, (uint8_t)_MAX, fg_thresh, bg_thresh);

    maskGen.init();
    maskGen.update(maskGen.depth_min, (uint8_t)_MIN);
    maskGen.update(maskGen.depth_max, (uint8_t)_MAX);
    maskGen.update(maskGen.thres_f_new, fg_thresh);
    maskGen.update(maskGen.thres_b_new, bg_thresh);
    maskGen.run(1);
    maskGen.in1.gm2aie_nb(inputData, BLOCK_SIZE_in_Bytes);
    maskGen.out1.aie2gm_nb(outputData, BLOCK_SIZE_in_Bytes);
    maskGen.out1.wait();

    // reference
    maskgen_ref(dataIn, refOutputData, (bool)MASKGEN_TRACKING, (uint8_t)_MIN, (uint8_t)_MAX, (uint8_t)_FGTH,
                (uint8_t)_BGTH, fg_thresh_track_r, bg_thresh_track_r, (int)TILE_HEIGHT, (int)TILE_WIDTH);

    // Compare the results
    int acceptableError = 0;
    int errCount = 0;
    uint8_t* dataOut = (uint8_t*)xf::cv::aie::xfGetImgDataPtr(outputData);
    for (int i = 0; i < TILE_ELEMENTS; i++) {
        if (abs(dataOut[i] - refOutputData[i]) > acceptableError) errCount++;
    }

    if (errCount) {
        std::cout << "Test failed!" << std::endl;
        exit(-1);
    }
    std::cout << "Test passed!" << std::endl;

    maskGen.end();

    return 0;
}
#endif
