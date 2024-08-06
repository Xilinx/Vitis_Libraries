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

// adf::GMIO gmioIn[2] = {adf::GMIO("gmioIn1", 256, 1000), adf::GMIO("gmioIn2", 256, 1000)};
// adf::GMIO gmioOut[1] = {adf::GMIO("gmioOut1", 256, 1000)};

// connect dataflow graph to simulation platform
// adf::simulation::platform<2, 1> platform(&gmioIn[0], &gmioIn[1], &gmioOut[0]);

yuv2rgbaGraph mygraph;

// adf::connect<> net0(platform.src[0], mygraph.in1);
// adf::connect<> net1(platform.src[1], mygraph.in2);

// adf::connect<> net2(mygraph.out, platform.sink[0]);

#if defined(__AIESIM__) || defined(__X86SIM__)
#include <common/xf_aie_utils.hpp>
int main(int argc, char** argv) {
    uint8_t* inputData = (uint8_t*)adf::GMIO::malloc(TILE_WINDOW_SIZE + xf::cv::aie::METADATA_SIZE);
    uint8_t* inputData1 = (uint8_t*)adf::GMIO::malloc(TILE_WINDOW_SIZE_UV + xf::cv::aie::METADATA_SIZE);
    uint8_t* outputData = (uint8_t*)adf::GMIO::malloc(TILE_WINDOW_SIZE_RGBA + xf::cv::aie::METADATA_SIZE);

    memset(inputData, 0, TILE_WINDOW_SIZE + xf::cv::aie::METADATA_SIZE);
    memset(inputData1, 0, TILE_WINDOW_SIZE_UV + xf::cv::aie::METADATA_SIZE);
    xf::cv::aie::xfSetTileWidth(inputData, TILE_WIDTH);
    xf::cv::aie::xfSetTileHeight(inputData, TILE_HEIGHT);
    xf::cv::aie::xfSetTileWidth(inputData1, TILE_WIDTH / 2);
    xf::cv::aie::xfSetTileHeight(inputData1, TILE_HEIGHT / 2);

    uint8_t* dataIn = (uint8_t*)xf::cv::aie::xfGetImgDataPtr(inputData);
    uint8_t* dataIn1 = (uint8_t*)xf::cv::aie::xfGetImgDataPtr(inputData1);
    for (int i = 0; i < TILE_ELEMENTS; i++) {
        dataIn[i] = rand() % 256;
    }
    for (int i = 0; i < TILE_ELEMENTS / 2; i++) {
        dataIn1[i] = rand() % 256;
    }

    mygraph.init();
    uint16 tile_width = TILE_WIDTH;
    uint16 tile_height = TILE_HEIGHT;

    mygraph.update(mygraph.tile_width, tile_width);
    mygraph.update(mygraph.tile_height, tile_height);
    mygraph.run(1);

    mygraph.in1.gm2aie_nb(inputData, TILE_WINDOW_SIZE + xf::cv::aie::METADATA_SIZE);
    mygraph.in2.gm2aie_nb(inputData1, TILE_WINDOW_SIZE_UV + xf::cv::aie::METADATA_SIZE);
    mygraph.out.aie2gm_nb(outputData, TILE_WINDOW_SIZE_RGBA + xf::cv::aie::METADATA_SIZE);
    mygraph.out.wait();

    // Compare the results
    // int acceptableError = 0;
    // int errCount = 0;
    // uint8_t* dataOut = (uint8_t*)xf::cv::aie::xfGetImgDataPtr(outputData);

    // for (int i = 0; i < TILE_ELEMENTS*4+64; i++) {
    // int cValue = abs(dataIn[i] - dataIn1[i]);
    //  if (i%16==15) std::cout << (int)outputData[i] << "\n";
    //	else std::cout << (int)outputData[i] <<" ";
    //}
    /*if (errCount) {
        std::cout << "Test failed!" << std::endl;
        exit(-1);
    }*/
    // std::cout << "Test passed" << std::endl;

    mygraph.end();
    return 0;
}
#endif
