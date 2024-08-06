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
#include <fstream>

template <typename T>
T saturate(T val, T min, T max) {
    return std::min(std::max(val, min), max);
}

// instantiate adf dataflow graph
RGBA2GreyGraph bl;

// initialize and run the dataflow graph
#if defined(__AIESIM__) || defined(__X86SIM__)

bool rgba2grey_ref(uint8_t* in, uint8_t* out, int height, int width) {
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            int posn = i * width + j;
            uint8_t grey = (in[posn * 4] * 77 + in[posn * 4 + 1] * 150 + in[posn * 4 + 2] * 29) >> 8;
            if (out[posn] != grey) {
                std::cout << i << "," << j << " :posn in wrong" << std::endl;
                return false;
            }
        }
    }
    return true;
}

#include <common/xf_aie_utils.hpp>
int main(int argc, char** argv) {
    uint8_t* inputData = (uint8_t*)GMIO::malloc(ELEM_WITH_METADATA_IN);
    uint8_t* outputData = (uint8_t*)GMIO::malloc(ELEM_WITH_METADATA_OUT);

    memset(inputData, 0, ELEM_WITH_METADATA_IN);
    memset(outputData, 0, ELEM_WITH_METADATA_OUT);
    xf::cv::aie::xfSetTileWidth(inputData, TILE_WIDTH);
    xf::cv::aie::xfSetTileHeight(inputData, TILE_HEIGHT);

    xf::cv::aie::xfSetTileWidth(outputData, TILE_WIDTH);
    xf::cv::aie::xfSetTileHeight(outputData, TILE_HEIGHT);

    uint8_t* dataIn = (uint8_t*)xf::cv::aie::xfGetImgDataPtr(inputData);
    for (int i = 0; i < TILE_ELEMENTS * 4; i++) dataIn[i] = rand() % 256;

    bl.init();
    bl.run(1);

    bl.in.gm2aie_nb(inputData, ELEM_WITH_METADATA_IN);
    bl.out.aie2gm_nb(outputData, ELEM_WITH_METADATA_OUT);
    bl.out.wait();

    // std::ofstream myfile ("output_gray.txt");
    // if (myfile.is_open())
    // {
    //     for(int count = 1; count <= ELEM_WITH_METADATA_OUT; count ++){
    //         myfile << (unsigned)outputData[count-1] << " " ;
    //         std::cout << (unsigned)outputData[count-1] << " " ;
    //         if(count%4==0) {myfile << "\n"; std::cout << "\n" ;}
    //     }
    //     myfile.close();
    // }
    // else std::cout << "Unable to open file";

    // TODO: Compare the results
    if (rgba2grey_ref((uint8_t*)xf::cv::aie::xfGetImgDataPtr(inputData),
                      (uint8_t*)xf::cv::aie::xfGetImgDataPtr(outputData), TILE_HEIGHT, TILE_WIDTH))
        std::cout << "Test passed!" << std::endl;
    else
        std::cout << "Test failed!" << std::endl;

    bl.end();
    return 0;
}
#endif
