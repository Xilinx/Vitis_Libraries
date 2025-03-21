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

// GMIO gmioIn[1] = {GMIO("gmioIn1", 256, 1000)};
// GMIO gmioOut[1] = {GMIO("gmioOut1", 256, 1000)};

// connect dataflow graph to simulation platform
// simulation::platform<1, 1> platform(&gmioIn[0], &gmioOut[0]);

// instantiate adf dataflow graph
gaincontrolGraph gc;

//<> net0(platform.src[0], gc.in1);
// connect<> net1(gc.out, platform.sink[0]);

// initialize and run the dataflow graph
#if defined(__AIESIM__) || defined(__X86SIM__)

uint8_t saturatecast(uint16_t P) {
    uint8_t temp = (P > 255) ? 255 : (uint8_t)P;
    return temp;
}
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

    uint8_t rgain = 255;
    uint8_t bgain = 255;
    uint8_t ggain = 200;

    gc.init();
    gc.update(gc.rgain, rgain);
    gc.update(gc.bgain, bgain);
    gc.update(gc.ggain, ggain);

    gc.in1.gm2aie_nb(inputData, BLOCK_SIZE_in_Bytes);
    gc.run(1);
    gc.out.aie2gm_nb(outputData, BLOCK_SIZE_in_Bytes);
    gc.out.wait();

    // Compare the results
    typedef uint8_t realSize;
    typedef uint16_t maxSize;
    maxSize pixel;

    int acceptableError = 1;
    int errCount = 0;
    uint8_t* dataOut = (uint8_t*)xf::cv::aie::xfGetImgDataPtr(outputData);
    for (int i = 0; i < TILE_HEIGHT; i++) {
        for (int j = 0; j < TILE_WIDTH; j++) {
            int pixel = dataIn[i * TILE_WIDTH + j];
            bool cond1, cond2;
            cond1 = (j % 2 == 0);
            cond2 = (j % 2 != 0);
            if (CODE == XF_BAYER_RG) {
                if (i % 2 == 0 && cond1)
                    pixel = (maxSize)((pixel * rgain) >> 7);
                else if (i % 2 != 0 && cond2)
                    pixel = (maxSize)((pixel * bgain) >> 7);
                else
                    pixel = (maxSize)((pixel * ggain) >> 7);
            } else if (CODE == XF_BAYER_GR) {
                if (i % 2 == 0 && cond2)
                    pixel = (maxSize)((pixel * rgain) >> 7);
                else if (i % 2 != 0 && cond1)
                    pixel = (maxSize)((pixel * bgain) >> 7);
                else
                    pixel = (maxSize)((pixel * ggain) >> 7);

            } else if (CODE == XF_BAYER_BG) {
                if (i % 2 == 0 && cond1)
                    pixel = (maxSize)((pixel * bgain) >> 7);
                else if (i % 2 == 0 && cond2)
                    pixel = (maxSize)((pixel * rgain) >> 7);
                else
                    pixel = (maxSize)((pixel * ggain) >> 7);

            } else if (CODE == XF_BAYER_GB) {
                if (i % 2 == 0 && cond2)
                    pixel = (maxSize)((pixel * bgain) >> 7);
                else if (i % 2 != 0 && cond1)
                    pixel = (maxSize)((pixel * rgain) >> 7);
                else
                    pixel = (maxSize)((pixel * ggain) >> 7);
            }
            pixel = saturatecast(pixel);
            if (abs(dataOut[i * TILE_WIDTH + j] - pixel) > acceptableError) {
                printf("dataOut=%d	pixel=%d\n", dataOut[i * TILE_WIDTH + j]);
                errCount++;
            }
        }
    }
    if (errCount) {
        std::cout << "Test failed!" << std::endl;
        exit(-1);
    }
    std::cout << "Test passed!" << std::endl;

    gc.end();
    return 0;
}
#endif
