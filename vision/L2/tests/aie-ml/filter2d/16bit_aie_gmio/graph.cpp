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
#include "config.h"

// Graph object
myGraph filter_graph;

#define SRS_SHIFT 10

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

    filter_graph.init();
    filter_graph.update(filter_graph.kernelCoefficients, float2fixed_coeff<10, 16>(kData).data(), 16);

    filter_graph.run(1);

    filter_graph.inptr.gm2aie_nb(inputData, BLOCK_SIZE_in_Bytes);
    filter_graph.outptr.aie2gm_nb(outputData, BLOCK_SIZE_in_Bytes);
    filter_graph.outptr.wait();

    printf("after grph wait\n");
    filter_graph.end();

    // compare the results
    int window[9];
    int acceptableError = 1;
    int errCount = 0;

    int16_t* dataOut = (int16_t*)xf::cv::aie::xfGetImgDataPtr(outputData);
    FILE* fp = fopen("aie.txt", "w");
    FILE* fp1 = fopen("ref.txt", "w");

    for (int i = 0; i < TILE_ELEMENTS; i++) {
        int row = i / TILE_WIDTH;
        int col = i % TILE_WIDTH;
        for (int j = -1; j <= 1; j++) {
            for (int k = -1; k <= 1; k++) {
                int r = std::max(row + j, 0);
                int c = std::max(col + k, 0);
                r = std::min(r, TILE_HEIGHT - 1);
                c = std::min(c, TILE_WIDTH - 1);
                window[(j + 1) * 3 + (k + 1)] = dataIn[r * TILE_WIDTH + c];
            }
        }
        float cValue = 0;
        for (int j = 0; j < 9; j++) cValue += window[j] * kData[j];

        fprintf(fp, "%d\n", dataOut[i]);
        fprintf(fp1, "%d\n", cValue);

        if (abs(cValue - dataOut[i]) > acceptableError) {
            errCount++;
        }
    }
    fclose(fp);
    fclose(fp1);

    if (errCount) {
        std::cout << "Test failed!" << std::endl;
        exit(-1);
    }
    std::cout << "Test passed!" << std::endl;

    return 0;
}
#endif
