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
#include <stdio.h>
#include <cstdio>
// Graph object
myGraph filter_graph;

#define SRS_SHIFT 10
// float kData[9] = {0,0,0,0,1,0,0,0,0};
// float kData[9] = {1,1,1,1,1,1,1,1,1};
float kData[9] = {0.0625, 0.1250, 0.0625, 0.125, 0.25, 0.125, 0.0625, 0.125, 0.0625};

void run_ref(int16_t* srcImageR, int16_t* dstRefImage, float coeff[9], int16_t height, int16_t width) {
    float window[9];

    typedef uint16_t Pixel_t;
    width *= 2; // inclusive of channels

    for (int i = 0; i < height * width; i++) {
        int row = i / width;
        int col = i % width;

        if (col % 2) {
            dstRefImage[i] = srcImageR[i];
            continue;
        }

        int w = 0;
        for (int j = -1; j <= 1; j++) {
            for (int k = -2; k <= 2; k += 2) {
                int r = std::max(row + j, 0);
                int c = std::max(col + k, 0);
                r = std::min(r, height - 1);
                c = std::min(c, width - 2);
                window[w++] = srcImageR[r * width + c];
            }
        }

        float s = 0;
        for (int j = 0; j < 9; j++) s += window[j] * coeff[j];
        dstRefImage[i] = s;
    }
    return;
}

template <int SHIFT, int VECTOR_SIZE>
auto float2fixed_coeff(float data[9]) {
    // 3x3 kernel positions
    //
    // k0 k1 0 k2 0
    // k3 k4 0 k5 0
    // k6 k7 0 k8 0
    std::array<int16_t, VECTOR_SIZE> ret;
    ret.fill(0);
    for (int i = 0, j = 0; i < 3; i++, j += 3) {
        ret[4 * i + 0] = data[j] * (1 << SHIFT);
        ret[4 * i + 1] = data[j + 1] * (1 << SHIFT);
        ret[4 * i + 2] = data[j + 2] * (1 << SHIFT);
        ret[4 * i + 3] = 0;
    }
    return ret;
}

#if defined(__AIESIM__) || defined(__X86SIM__)
#include <common/xf_aie_utils.hpp>
int main(int argc, char** argv) {
    int16_t* inputData = (int16_t*)GMIO::malloc(ELEM_WITH_METADATA * sizeof(int16_t));
    int16_t* outputData = (int16_t*)GMIO::malloc(ELEM_WITH_METADATA * sizeof(int16_t));
    int16_t* outputRefData = (int16_t*)GMIO::malloc(ELEM_WITH_METADATA * sizeof(int16_t));

    memset(inputData, 0, ELEM_WITH_METADATA * sizeof(int16_t));
    xf::cv::aie::xfSetTileWidth(inputData, TILE_WIDTH);
    xf::cv::aie::xfSetTileHeight(inputData, TILE_HEIGHT);

    int16_t* dataIn = (int16_t*)xf::cv::aie::xfGetImgDataPtr(inputData);
    int16_t* dataRefOut = (int16_t*)xf::cv::aie::xfGetImgDataPtr(outputRefData);
    int16_t* dataOut = (int16_t*)xf::cv::aie::xfGetImgDataPtr(outputData);

    for (int i = 0; i < TILE_ELEMENTS; i++) {
        dataIn[i] = rand() % 256;
    }

    filter_graph.init();
    filter_graph.update(filter_graph.KC, float2fixed_coeff<10, 16>(kData).data(), 16);

    filter_graph.run(1);

    filter_graph.inptr.gm2aie_nb(inputData, ELEM_WITH_METADATA * sizeof(int16_t));
    filter_graph.outptr.aie2gm_nb(outputData, ELEM_WITH_METADATA * sizeof(int16_t));
    filter_graph.outptr.wait();
    filter_graph.end();
    printf("after graph wait\n");

    // compare the results
    run_ref(dataIn, dataRefOut, kData, TILE_HEIGHT, TILE_WIDTH);
    int acceptableError = 1;
    int errCount = 0;
    for (int i = 0; i < TILE_ELEMENTS; i++) {
        if (abs(dataRefOut[i] - dataOut[i]) > acceptableError) {
            // std::cout<<"err at : i="<<i<<" err="<<abs(dataRefOut[i] -
            // dataOut[i])<<"="<<dataRefOut[i]<<"-"<<dataOut[i]<<std::endl;
            errCount++;
        }
    }
    if (errCount) {
        std::cerr << "Test failed." << std::endl;
        exit(-1);
    }

    std::cout << "Test passed!" << std::endl;

    return 0;
}
#endif
