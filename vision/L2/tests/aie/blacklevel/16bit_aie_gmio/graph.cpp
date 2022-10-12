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

template <typename T>
T saturate(T val, T min, T max) {
    return std::min(std::max(val, min), max);
}

// instantiate adf dataflow graph
blacklevelGraph bl;

// initialize and run the dataflow graph
#if defined(__AIESIM__) || defined(__X86SIM__)

#include <common/xf_aie_utils.hpp>
int main(int argc, char** argv) {
    int BLOCK_SIZE_in_Bytes = TILE_WINDOW_SIZE;

    int16_t* inputData = (int16_t*)GMIO::malloc(BLOCK_SIZE_in_Bytes);
    int16_t* outputData = (int16_t*)GMIO::malloc(BLOCK_SIZE_in_Bytes);

    memset(inputData, 0, BLOCK_SIZE_in_Bytes);
    xf::cv::aie::xfSetTileWidth(inputData, TILE_WIDTH);
    xf::cv::aie::xfSetTileHeight(inputData, TILE_HEIGHT);

    int16_t* dataIn = (int16_t*)xf::cv::aie::xfGetImgDataPtr(inputData);
    for (int i = 0; i < TILE_ELEMENTS; i++) {
        dataIn[i] = rand() % 256;
    }

    int16_t BlackLevel = 32;
    const int MaxLevel = 255;
    float MulValue1 = (float)((float)MaxLevel / (MaxLevel - BlackLevel));
    int32_t MulValue = 37470; // Q(1.15)

    bl.init();
    bl.update(bl.blacklevel, BlackLevel);
    bl.update(bl.mulfact, MulValue);
    bl.run(1);

    bl.in1.gm2aie_nb(inputData, BLOCK_SIZE_in_Bytes);
    bl.out1.aie2gm_nb(outputData, BLOCK_SIZE_in_Bytes);
    bl.out1.wait();

    // Compare the results
    int acceptableError = 1;
    int errCount = 0;
    int16_t* dataOut = (int16_t*)xf::cv::aie::xfGetImgDataPtr(outputData);
    typedef uint8_t Pixel_t;
    Pixel_t out_pix;
    for (int i = 0; i < TILE_HEIGHT; i++) {
        for (int j = 0; j < TILE_WIDTH; j++) {
            Pixel_t Pixel = dataIn[i * TILE_WIDTH + j];
            int temp = ((Pixel - BlackLevel) * MulValue1);
            out_pix = saturate<int>(temp, 0, 255);

            if (abs(dataOut[i * TILE_WIDTH + j] - out_pix) > acceptableError) {
                printf(" at %d Pixel=%d  aie=%d  PL=%d\n", (i * TILE_WIDTH + j), Pixel, out_pix,
                       dataOut[i * TILE_WIDTH + j]);
                errCount++;
            }
        }
    }

    if (errCount) {
        std::cout << "Test failed!" << std::endl;
        exit(-1);
    }
    std::cout << "Test passed!" << std::endl;

    bl.end();
    return 0;
}
#endif
