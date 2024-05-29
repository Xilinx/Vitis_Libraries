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
#include <cmath>
#include "config.h"
using namespace adf;
// instantiate adf dataflow graph
DenormalizeGraph denorm;

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

    denorm.init();
    denorm.update(denorm.coeff, coeff.data(), coeff.size());
    denorm.run(1); // input.txt has data for 224 tiles and assumes metadata for TILE_HEIGHT 2
    denorm.wait();
    denorm.end();

    return 0;
}
#endif
