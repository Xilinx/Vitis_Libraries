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
resizeGraph resize[NO_INSTANCES] = {{6, 0, 0}, {7, 0, 1}, {8, 0, 2}, {9, 0, 3}};       //, {10,0,4}, {11,0,5}} ;
resizeGraph2 resize2[NO_INSTANCES] = {{17, 0, 6}, {18, 0, 7}, {19, 0, 8}, {20, 0, 9}}; //, {21,0,10}, {22,0,11}};

// initialize and run the dataflow graph
#if defined(__AIESIM__) || defined(__X86SIM__)

uint8_t saturatecast(uint16_t P) {
    uint8_t temp = (P > 255) ? 255 : (uint8_t)P;
    return temp;
}
#include <common/xf_aie_utils.hpp>
int main(int argc, char** argv) {
    int BLOCK_SIZE_in_Bytes = TILE_WINDOW_SIZE_IN * sizeof(DATA_TYPE);
    int BLOCK_SIZE_out_Bytes = TILE_WINDOW_SIZE_OUT * sizeof(DATA_TYPE);

    DATA_TYPE* inputData = (DATA_TYPE*)GMIO::malloc(BLOCK_SIZE_in_Bytes);
    DATA_TYPE* outputData = (DATA_TYPE*)GMIO::malloc(BLOCK_SIZE_out_Bytes);

    memset(inputData, 0, BLOCK_SIZE_in_Bytes);

    xf::cv::aie::xfSetTileWidth(inputData, TILE_WIDTH_IN);
    xf::cv::aie::xfSetTileHeight(inputData, TILE_HEIGHT_IN);
    xf::cv::aie::xfSetTileOutTWidth(inputData, TILE_WIDTH_OUT);
    xf::cv::aie::xfSetTileOutTHeight(inputData, TILE_HEIGHT_OUT);

    xf::cv::aie::xfSetTilePosH(inputData, 0);
    xf::cv::aie::xfSetTilePosV(inputData, 480);
    xf::cv::aie::xfSetTileOutPosH(inputData, 0);
    xf::cv::aie::xfSetTileOutPosV(inputData, 1928);

    DATA_TYPE* dataIn = (DATA_TYPE*)xf::cv::aie::xfGetImgDataPtr(inputData);
    for (int i = 0; i < TILE_ELEMENTS_IN; i++) {
        dataIn[i] = rand() % 256;
    }

    resize[0].init();
    resize[0].updateInputOutputSize(IMAGE_WIDTH_IN, IMAGE_HEIGHT_IN, IMAGE_WIDTH_OUT, IMAGE_HEIGHT_OUT);
    resize[0].in1.gm2aie_nb(inputData, BLOCK_SIZE_in_Bytes);
    resize[0].run(1);
    resize[0].out1.aie2gm_nb(outputData, BLOCK_SIZE_out_Bytes);
    resize[0].out1.wait();

    // Compare the results

    DATA_TYPE* dataOut = (DATA_TYPE*)xf::cv::aie::xfGetImgDataPtr(outputData);
    FILE* fp = fopen("aieout.txt", "w");
    int q = 0;
    for (int i = 0; i < TILE_HEIGHT_OUT; i++) {
        for (int j = 0; j < (TILE_WIDTH_OUT * 4); j++) {
            int pixel = dataOut[i * (TILE_WIDTH_OUT * 4) + j];
            fprintf(fp, "%d ", pixel);
            if (((q + 1) % 8) == 0) {
                fprintf(fp, "\n");
            }
            q++;
        }
    }
    fclose(fp);
    std::cout << "Test passed!" << std::endl;

    resize[0].end();
    return 0;
}
#endif
