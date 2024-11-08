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
pixelwiseGraph ps;

// initialize and run the dataflow graph
#if defined(__AIESIM__) || defined(__X86SIM__)

void pixelwise_select_ref(uint8* frame, uint8* mask, uint8* bg, uint8* out, int elements) {
    for (int i = 0; i < elements; i++) out[i] = mask[i] ? frame[i] : bg[i];
    return;
}

#include <common/xf_aie_utils.hpp>
int main(int argc, char** argv) {
    uint8_t* f = (uint8_t*)GMIO::malloc(ELEM_WITH_METADATA);
    uint8_t* m = (uint8_t*)GMIO::malloc(ELEM_WITH_METADATA);
    uint8_t* b = (uint8_t*)GMIO::malloc(ELEM_WITH_METADATA);
    uint8_t* o = (uint8_t*)GMIO::malloc(ELEM_WITH_METADATA);

    memset(f, 0, ELEM_WITH_METADATA);
    memset(m, 0, ELEM_WITH_METADATA);
    memset(b, 0, ELEM_WITH_METADATA);

    xf::cv::aie::xfSetTileWidth(f, TILE_WIDTH);
    xf::cv::aie::xfSetTileHeight(f, TILE_HEIGHT);

    xf::cv::aie::xfSetTileWidth(m, TILE_WIDTH);
    xf::cv::aie::xfSetTileHeight(m, TILE_HEIGHT);

    xf::cv::aie::xfSetTileWidth(b, TILE_WIDTH);
    xf::cv::aie::xfSetTileHeight(b, TILE_HEIGHT);

    uint8_t* f_data = (uint8_t*)xf::cv::aie::xfGetImgDataPtr(f);
    uint8_t* m_data = (uint8_t*)xf::cv::aie::xfGetImgDataPtr(m);
    uint8_t* b_data = (uint8_t*)xf::cv::aie::xfGetImgDataPtr(b);
    uint8_t* o_data = (uint8_t*)xf::cv::aie::xfGetImgDataPtr(o);

    std::cout << ">>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<\nInitializing data...";
    for (int i = 0; i < TILE_ELEMENTS; i++) {
        f_data[i] = rand() % 256;
        m_data[i] = rand() % 2;
        b_data[i] = rand() % 256;
    }
    std::cout << "[DONE]" << std::endl;

    ps.init();
    std::cout << "Starting graph run...";
    ps.run(1);

    ps.in_f.gm2aie_nb(f, ELEM_WITH_METADATA);
    ps.in_m.gm2aie_nb(m, ELEM_WITH_METADATA);
    ps.in_b.gm2aie_nb(b, ELEM_WITH_METADATA);
    ps.out.aie2gm_nb(o, ELEM_WITH_METADATA);
    ps.out.wait();

    std::cout << "[DONE]" << std::endl;

    // Compare the results
    uint8_t* ref_o = (uint8_t*)std::malloc(TILE_WIDTH * TILE_HEIGHT);

    // run ref fn
    pixelwise_select_ref(f_data, m_data, b_data, ref_o, TILE_WIDTH * TILE_HEIGHT);

    // Match reference and output
    for (int i = 0; i < TILE_WIDTH * TILE_HEIGHT; i++) {
        if (o_data[i] != ref_o[i]) {
            std::cout << "Functional error at element : " << i << std::endl;
            std::cerr << "Test failed :(" << std::endl;
            exit(-1);
        }
    }
    std::cout << "Test passed!" << std::endl;

    ps.end();
    return 0;
}
#endif
