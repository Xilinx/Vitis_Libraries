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

// instantiate adf dataflow graph
topkGraph topk;

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
    int BLOCK_SIZE_in_Bytes = (TILE_WIDTH_IN * TILE_HEIGHT * NUM_TILES * sizeof(bfloat16));
    int BLOCK_SIZE_fl_in_Bytes = (TILE_WIDTH_IN * TILE_HEIGHT * NUM_TILES * sizeof(float));
    int BLOCK_SIZE_uint16_in_Bytes = (TILE_WIDTH_IN * TILE_HEIGHT * NUM_TILES * sizeof(uint16_t));

    float* inputData_fl = (float*)malloc(BLOCK_SIZE_fl_in_Bytes);
    uint16_t* inputData_uint16 = (uint16_t*)malloc(BLOCK_SIZE_uint16_in_Bytes);
    float* inputData = (float*)malloc(BLOCK_SIZE_fl_in_Bytes / NUM_TILES);

    uint64_t accu = 0;

    for (int i = 0; i < (TILE_HEIGHT * NUM_TILES); i++) {
        for (int j = 0; j < TILE_WIDTH_IN; j++) {
            float value;
            value = rand();

            inputData_fl[(i * TILE_WIDTH_IN) + j] = value;
            float temp_f = f_to_bf(value);
            int* temp_int = reinterpret_cast<int*>(&temp_f);
            inputData_uint16[(i * TILE_WIDTH_IN) + j] = (uint16_t)((*temp_int) >> 16);
        }
    }

    int l = 0;
    std::ofstream inData("data/input_score_1024x1.txt");
    for (int i = 0; i < (TILE_HEIGHT * NUM_TILES); i++) {
        l = 0;
        inData << 0 << " " << 0 << " " << 1024 << " " << 1 << std::endl;
        inData << 0 << " " << 0 << " " << 0 << " " << 0 << std::endl;
        inData << 0 << " " << 0 << " " << 1024 << " " << 1 << std::endl;
        inData << 0 << " " << 0 << " " << i << " " << 0 << std::endl;
        inData << i << " " << 0 << " " << 0 << " " << 0 << std::endl;
        inData << 0 << " " << 0 << " " << 0 << " " << 0 << std::endl;
        inData << 0 << " " << 0 << " " << 0 << " " << 0 << std::endl;
        inData << 0 << " " << 0 << " " << 0 << " " << 0 << std::endl;

        for (int j = 0; j < TILE_WIDTH_IN; j++) {
            inData << inputData_uint16[(i * TILE_WIDTH_IN) + j] << " ";
            l++;
            if (l == 4) {
                inData << std::endl;
                l = 0;
            }
        }
    }

    std::ofstream inData1("data/input_score_fl_1024x1.txt");
    for (int i = 0; i < (TILE_HEIGHT * NUM_TILES); i++) {
        l = 0;
        for (int j = 0; j < TILE_WIDTH_IN; j++) {
            inData1 << inputData_fl[(i * TILE_WIDTH_IN) + j] << " ";
            l++;
            if (l == 4) {
                inData1 << std::endl;
                l = 0;
            }
        }
    }

    int indices[TILE_WIDTH_IN * TILE_HEIGHT * NUM_TILES];
    for (int i = 0; i < TILE_HEIGHT * NUM_TILES; i++) {
        for (int j = 0; j < TILE_WIDTH_IN; j++) {
            inputData[j] = inputData_fl[(i * TILE_WIDTH_IN) + j];
        }
        topKIndices(inputData, indices + (i * TILE_WIDTH_IN), (TILE_WIDTH_IN), (TILE_WIDTH_OUT));
    }

    std::ofstream out_indices("data/out_indices_ref.txt");

    for (int i = 0; i < TILE_HEIGHT * NUM_TILES; i++) {
        for (int j = 0; j < TILE_WIDTH_OUT; j++) {
            out_indices << "indices = " << indices[(i * TILE_WIDTH_IN) + j] << std::endl;
        }
    }

    topk.init();
    topk.update(topk.num_elem, TILE_ELEMENTS_IN);
    topk.update(topk.ktop, TILE_ELEMENTS_OUT);
    topk.update(topk.start_idx, 0);
    topk.run(NUM_TILES);
    topk.wait();
    topk.end();

    return 0;
}
#endif
