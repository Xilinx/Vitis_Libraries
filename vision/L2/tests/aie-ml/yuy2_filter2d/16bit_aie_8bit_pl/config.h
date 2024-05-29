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

#ifndef __CONFIG_H_
#define __CONFIG_H_

#include <common/xf_aie_const.hpp>
#include <string>
#include <array>

typedef short int int16_t;
#define SRS_SHIFT 10 // same as one defined in xf_yuy2_filter2d.hpp
float kData[9] = {0.0625, 0.1250, 0.0625, 0.125, 0.25, 0.125, 0.0625, 0.125, 0.0625};

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

// tile dimensions are normally computed by tiler but we need to
// hardcode these values to set the graph window sizes
using DATA_TYPE = int16_t;
static constexpr int TILE_WIDTH = 128;
static constexpr int TILE_HEIGHT = 16;
static constexpr int CHANNELS = 2;
static constexpr int TILE_ELEMENTS = TILE_WIDTH * TILE_HEIGHT * CHANNELS;
static constexpr int TILE_WINDOW_SIZE = ((TILE_ELEMENTS * sizeof(DATA_TYPE)) + xf::cv::aie::METADATA_SIZE);
static constexpr int ELEM_WITH_METADATA = TILE_ELEMENTS + (xf::cv::aie::METADATA_SIZE / sizeof(DATA_TYPE));

/* Graph specific configuration */
static constexpr int VECTORIZATION_FACTOR = 16;

static constexpr int PARAM_DATA = 16; // * sizeof(short int);

static constexpr int __X86_DEVICE__ = 0;

#endif //__CONFIG_H_
