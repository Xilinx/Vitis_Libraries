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
#include <iostream>
#include <fstream>
#include <common/xf_aie_utils.hpp>

// instantiate adf dataflow graph
topkGraph topk;

void read_file_uint16(FILE* fpout, uint16_t* in_buffer, int n) {
    for (int i = 0; i < n; i++) {
        fscanf(fpout, "%hu \n", (in_buffer));
        in_buffer += 1;
    }
}

typedef union value_convert {
    std::uint32_t u;
    float f;
} value_convert_t;

std::uint32_t f_to_u(float data) {
    value_convert_t vc{};
    vc.f = data;
    return vc.u;
}

float u_to_f(std::uint32_t data) {
    value_convert_t vc{};
    vc.u = data;
    return vc.f;
}

float f_to_bf(float data) {
    std::uint32_t u = f_to_u(data);
    u = (u + 0x7fff) & 0xFFFF0000;
    return u_to_f(u);
}

void topKIndices(const float* arr, int* indices, int size, int k) {
    if (k <= 0 || k > size) {
        // Handle invalid values of k
        std::cerr << "Invalid value of k\n";
        return;
    }

    // Initialize indices array with values 0 to size-1
    for (int i = 0; i < size; ++i) {
        indices[i] = i;
    }

    // Sort the indices based on the values they point to
    std::sort(indices, indices + size, [&arr](int a, int b) { return arr[a] > arr[b]; });
}

// initialize and run the dataflow graph
#if defined(__AIESIM__) || defined(__X86SIM__)
int main(int argc, char** argv) {
    uint16_t* inputData_aie = (uint16_t*)GMIO::malloc(ELEM_WITH_METADATA_IN * sizeof(uint16_t));
    uint16_t* out1 = (uint16_t*)GMIO::malloc(ELEM_WITH_METADATA_OUT * sizeof(uint16_t));
    uint16_t* out2 = (uint16_t*)GMIO::malloc(ELEM_WITH_METADATA_OUT * sizeof(uint16_t));

    memset(inputData_aie, 0, ELEM_WITH_METADATA_IN);

    uint16_t* dataIn = (uint16_t*)xf::cv::aie::xfGetImgDataPtr(inputData_aie);
    for (int i = 0; i < TILE_ELEMENTS_IN; i++) {
        dataIn[i] = i;
    }

    topk.init();
    topk.update(topk.num_elem, TILE_ELEMENTS_IN);
    topk.update(topk.ktop, TILE_ELEMENTS_OUT);
    topk.update(topk.start_idx, 0);
    topk.run(NUM_TILES);
    topk.in1.gm2aie_nb(inputData_aie, ELEM_WITH_METADATA_IN * sizeof(uint16_t));
    topk.out1.aie2gm_nb(out1, ELEM_WITH_METADATA_OUT * sizeof(uint16_t));
    topk.out2.aie2gm_nb(out2, ELEM_WITH_METADATA_OUT * sizeof(uint16_t));
    topk.out1.wait();
    topk.out2.wait();
    topk.wait();
    topk.end();

    // Compare the results
    uint16_t* refData1 = (uint16_t*)GMIO::malloc(TILE_ELEMENTS_OUT);
    FILE* fp_refout1 = NULL;
    fp_refout1 = fopen("data/output1_ref.txt", "r");
    if (fp_refout1 == NULL) {
        printf("Failure opening file %s for reading!!\n", fp_refout1);
        return -1;
    } else {
        read_file_uint16(fp_refout1, refData1, TILE_ELEMENTS_OUT);
    }
    fclose(fp_refout1);

    uint16_t* aieOut1 = (uint16_t*)xf::cv::aie::xfGetImgDataPtr(out1);
    FILE* fp = fopen("aieout1.txt", "w");
    int err = 0;
    uint16_t ref_pixel;
    for (int i = 0; i < TILE_HEIGHT; i++) {
        for (int j = 0; j < TILE_WIDTH_OUT; j++) {
            int aie_pixel = aieOut1[i * TILE_WIDTH_OUT + j];
            fprintf(fp, "%d ", aie_pixel);
            ref_pixel = refData1[i * 2 * TILE_WIDTH_OUT + j];
            if (abs(aie_pixel - ref_pixel) > 1) {
                std::cout << "ind = " << (i * TILE_WIDTH_OUT + j) << " ref_pixel = " << static_cast<int>(ref_pixel)
                          << " aie_pixel = " << aie_pixel << std::endl;
                err++;
            }
        }
    }
    fclose(fp);

    uint16_t* refData2 = (uint16_t*)GMIO::malloc(TILE_ELEMENTS_OUT);
    FILE* fp_refout2 = NULL;
    fp_refout2 = fopen("data/output2_ref.txt", "r");
    if (fp_refout2 == NULL) {
        printf("Failure opening file %s for reading!!\n", fp_refout2);
        return -1;
    } else {
        read_file_uint16(fp_refout2, refData2, TILE_ELEMENTS_OUT);
    }
    fclose(fp_refout2);

    uint16_t* aieOut2 = (uint16_t*)xf::cv::aie::xfGetImgDataPtr(out2);
    FILE* fp2 = fopen("aieout2.txt", "w");

    for (int i = 0; i < TILE_HEIGHT; i++) {
        for (int j = 0; j < TILE_WIDTH_OUT; j++) {
            int aie_pixel = aieOut2[i * TILE_WIDTH_OUT + j];
            fprintf(fp2, "%d ", aie_pixel);
            ref_pixel = refData2[i * 2 * TILE_WIDTH_OUT + j];
            if (abs(aie_pixel - ref_pixel) > 1) {
                std::cout << "ind = " << (i * TILE_WIDTH_OUT + j) << " ref_pixel = " << static_cast<int>(ref_pixel)
                          << " aie_pixel = " << aie_pixel << std::endl;
                err++;
            }
        }
    }
    fclose(fp2);

    if (err == 0) {
        std::cout << "Test passed!" << std::endl;
    } else {
        std::cout << "Test failed!" << std::endl;
    }

    return 0;
}
#endif
