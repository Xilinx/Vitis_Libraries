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
#include "XModified.hpp"

namespace xf {
namespace codec {
namespace details {

// ------------------------------------------------------------
int hls_color_index(int c) {
#pragma HLS INLINE
    return (c == 0) ? 0 : 1;
}

// ------------------------------------------------------------
uint8_t hls_get_num_nonzeros_context(
    bool all_present, bool above_present, bool left_present, uint8_t num_nonzeros_above, uint8_t num_nonzeros_left) {
#pragma HLS INLINE
    uint8_t num_nonzeros_context = 0;
    if ((!all_present) && above_present && !left_present) {
        num_nonzeros_context = (num_nonzeros_above + 1) / 2;
    } else if ((!all_present) && left_present && !above_present) {
        num_nonzeros_context = (num_nonzeros_left + 1) / 2;
    } else if (all_present || (left_present && above_present)) {
        num_nonzeros_context = (num_nonzeros_above + num_nonzeros_left + 2) / 4;
    }
    return num_nonzeros_context;
}

// ------------------------------------------------------------
uint16_t abs16(int16_t din) {
    if (din < 0)
        return -din;
    else
        return din;
}

// ------------------------------------------------------------
uint16_t hls_compute_aavrg(bool all_present,
                           bool left_present,
                           bool above_present,
                           uint16_t abs_coef_left,
                           uint16_t abs_coef_above,     //[64],
                           uint16_t abs_coef_above_left //[64]

                           ) {
#pragma HLS inline
    uint16_t total = 0;
    if (all_present || left_present) {
        total += abs_coef_left; //[hls_raster_to_aligned[coord]]);
    }
    if (all_present || above_present) {
        total += abs_coef_above; //[hls_raster_to_aligned[coord]]);
    }
    if (all_present || (left_present && above_present)) {
        constexpr unsigned int log_weight = 5;
        total *= 13;
        total += 6 * abs_coef_above_left; //[hls_raster_to_aligned[coord]]);
        return ((uint16_t)total) >> 5;
    } else {
        return total;
    }
}

// ------------------------------------------------------------
/*int32_t hls_compute_lak(
    // const ConstBlockContext&context,
    int COLOR,
    unsigned int band,
    bool all_present,
    bool left_present,
    bool above_present,
    int16_t coef_here[64],
    int16_t coef_left[64],
    int16_t coef_above[64]) {
    int coeffs_x[8];
    int coeffs_a[8];
    const int32_t* coef_idct = nullptr;
    if ((band & 7) && (all_present || above_present)) {
        // y == 0: we're the x
        assert(band / 8 == 0); // this function only works for the edge
        // const auto &above = context.above_unchecked();
        for (int i = 0; i < 8; ++i) {
            uint8_t cur_coef = band + i * 8;
            coeffs_x[i] = i ? coef_here[hls_raster_to_aligned[cur_coef]] : 0;
            coeffs_a[i] = coef_above[hls_raster_to_aligned[cur_coef]];
        }
        coef_idct = ProbabilityTablesBase::icos_idct_edge_8192_dequantized_x((int)COLOR) + band * 8;
    } else if ((band & 7) == 0 && left_present) {
        // x == 0: we're the y
        // const auto &left = context.left_unchecked();
        for (int i = 0; i < 8; ++i) {
            uint8_t cur_coef = band + i;
            coeffs_x[i] = i ? coef_here[hls_raster_to_aligned[cur_coef]] : 0;
            coeffs_a[i] = coef_left[hls_raster_to_aligned[cur_coef]];
        }
        coef_idct = ProbabilityTablesBase::icos_idct_edge_8192_dequantized_y((int)COLOR) + band;
    } else {
        return 0;
    }
    int prediction =
        coeffs_a[0] *
        coef_idct[0]; // rounding towards zero before adding coeffs_a[0] helps ratio slightly, but this is cheaper
    for (int i = 1; i < 8; ++i) {
        int sign = (i & 1) ? 1 : -1;
        prediction -= coef_idct[i] * (coeffs_x[i] + sign * coeffs_a[i]);
    }
    prediction /= coef_idct[0];
    return prediction;
}*/

} // namespace details
} // namespace codec
} // namespace xf
