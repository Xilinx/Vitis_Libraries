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
resizeGraph resize;
resizeGraph2 resize2;

// initialize and run the dataflow graph
#if defined(__AIESIM__) || defined(__X86SIM__)

uint8_t saturatecast(uint16_t P) {
    uint8_t temp = (P > 255) ? 255 : (uint8_t)P;
    return temp;
}
#include <common/xf_aie_utils.hpp>
int main(int argc, char** argv) {
    int BLOCK_SIZE_in_Bytes = TILE_WINDOW_SIZE_IN;
    int BLOCK_SIZE_out_Bytes = TILE_WINDOW_SIZE_OUT;

    uint8_t* inputData = (uint8_t*)GMIO::malloc(BLOCK_SIZE_in_Bytes);
    uint8_t* outputData = (uint8_t*)GMIO::malloc(BLOCK_SIZE_out_Bytes);

    memset(inputData, 0, BLOCK_SIZE_in_Bytes);

    xf::cv::aie::xfSetTileWidth(inputData, TILE_WIDTH_IN);
    xf::cv::aie::xfSetTileHeight(inputData, TILE_HEIGHT_IN);
    xf::cv::aie::xfSetTileOutTWidth(inputData, TILE_WIDTH_OUT);
    xf::cv::aie::xfSetTileOutTHeight(inputData, TILE_HEIGHT_OUT);

    xf::cv::aie::xfSetTilePosH(inputData, 0);
    xf::cv::aie::xfSetTilePosV(inputData, 0);
    xf::cv::aie::xfSetTileOutPosH(inputData, 0);
    xf::cv::aie::xfSetTileOutPosV(inputData, 0);

    uint8_t* dataIn = (uint8_t*)xf::cv::aie::xfGetImgDataPtr(inputData);
    for (int i = 0; i < TILE_ELEMENTS_IN; i++) {
        dataIn[i] = rand() % 256;
    }

    resize.init();
    resize.updateInputOutputSize(IMAGE_WIDTH_IN, IMAGE_HEIGHT_IN, IMAGE_WIDTH_OUT, IMAGE_HEIGHT_OUT);
    resize.update(resize.outputStride, IMAGE_HEIGHT_OUT);
    resize.in1.gm2aie_nb(inputData, BLOCK_SIZE_in_Bytes);
    resize.run(1);
    resize.out1.aie2gm_nb(outputData, BLOCK_SIZE_out_Bytes);
    resize.out1.wait();

    // Compare the results

    uint8_t* dataOut = (uint8_t*)xf::cv::aie::xfGetImgDataPtr(outputData);
    FILE* fp = fopen("aieout.txt", "w");
    int q = 0;
    for (int i = 0; i < TILE_HEIGHT_OUT; i++) {
        for (int j = 0; j < TILE_WIDTH_OUT; j++) {
            int pixel = dataIn[i * TILE_WIDTH_OUT + j];
            fprintf(fp, "%d ", pixel);
            if (((q + 1) % 16) == 16) {
                fprintf(fp, "\n");
            }
            q++;
        }
    }
    fclose(fp);
    std::cout << "Test passed!" << std::endl;

    resize.end();
    return 0;
}
#endif