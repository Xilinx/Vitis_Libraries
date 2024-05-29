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
denormResizeGraph denorm_resize;

// initialize and run the dataflow graph
#if defined(__AIESIM__) || defined(__X86SIM__)
int main(int argc, char** argv) {
    // Empty
    std::array<float, 4> mean = {0.2, 0.7, 0.8, 0};
    std::array<float, 4> std_deviation = {0.3, 0.5, 0.6, 0};

    unsigned char alpha[4];
    char beta[4];

    get_alpha_beta<8, 5>(mean, std_deviation, alpha, beta);
    std::vector<int16_t> coeff({alpha[0], alpha[1], alpha[2], alpha[3], beta[0], beta[1], beta[2], beta[3]});

    uint32_t scale_x_fix = compute_scalefactor<16>(IMAGE_WIDTH_IN, IMAGE_WIDTH_OUT);
    uint32_t scale_y_fix = compute_scalefactor<16>(IMAGE_HEIGHT_IN, IMAGE_HEIGHT_OUT);

    denorm_resize.init();
    denorm_resize.update(denorm_resize.scalex, scale_x_fix);
    denorm_resize.update(denorm_resize.scaley, scale_y_fix);
    denorm_resize.update(denorm_resize.coeff, coeff.data(), coeff.size());

    denorm_resize.run(IMAGE_HEIGHT_OUT);
    denorm_resize.wait();
    denorm_resize.end();

    return 0;
}
#endif
