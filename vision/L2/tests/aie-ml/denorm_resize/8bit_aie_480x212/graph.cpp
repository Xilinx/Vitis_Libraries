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
    float mean[3] = {0.485, 0.456, 0.406};
    float std_deviation[3] = {0.229, 0.224, 0.225};

    denorm_resize.init();

    denorm_resize.updateInputOutputSize(IMAGE_WIDTH_IN, IMAGE_HEIGHT_IN, IMAGE_WIDTH_OUT, IMAGE_HEIGHT_OUT);
    denorm_resize.updateMeanStddev(mean, std_deviation);
    for (int outRowIdx = 0; outRowIdx < IMAGE_HEIGHT_OUT; outRowIdx++) {
        denorm_resize.update(denorm_resize.row, outRowIdx);
        denorm_resize.run(1);
        denorm_resize.wait();
    }
    denorm_resize.end();

    return 0;
}
#endif
