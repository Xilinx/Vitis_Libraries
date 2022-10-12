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
resizeNormGraph resize_norm;

template <int FBITS_ALPHA = 0, int FBITS_BETA = 4>
void get_alpha_beta(std::array<float, 4> mean,
                    std::array<float, 4> std_deviation,
                    std::array<int, 4>& alpha,
                    std::array<int, 4>& beta) {
    for (int i = 0; i < 4; i++) {
        if (i < 3) {
            float a_v = mean[i] * (1 << FBITS_ALPHA);
            float b_v = (1 / std_deviation[i]) * (1 << FBITS_BETA);

            alpha[i] = (unsigned char)a_v;
            beta[i] = (char)b_v;
        } else {
            alpha[i] = 0;
            beta[i] = 0;
        }
    }
}

// initialize and run the dataflow graph
#if defined(__AIESIM__) || defined(__X86SIM__)
int main(int argc, char** argv) {
    // Empty
    std::array<float, 4> mean = {104, 107, 123, 0};
    std::array<float, 4> std_deviation = {2, 2, 2, 0};

    std::array<int, 4> alpha;
    std::array<int, 4> beta;
    get_alpha_beta<0, 4>(mean, std_deviation, alpha, beta);

    resize_norm.init();
    resize_norm.update(resize_norm.a0, alpha[0]);
    resize_norm.update(resize_norm.a1, alpha[1]);
    resize_norm.update(resize_norm.a2, alpha[2]);
    resize_norm.update(resize_norm.a3, alpha[3]);
    resize_norm.update(resize_norm.b0, beta[0]);
    resize_norm.update(resize_norm.b1, beta[1]);
    resize_norm.update(resize_norm.b2, beta[2]);
    resize_norm.update(resize_norm.b3, beta[3]);

    resize_norm.run(IMAGE_HEIGHT_OUT);
    resize_norm.wait();
    resize_norm.end();

    return 0;
}
#endif
