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
resizeGraph resize;
resizeGraph2 resize2;

// initialize and run the dataflow graph
#if defined(__AIESIM__) || defined(__X86SIM__)
int main(int argc, char** argv) {
    resize.init();
    resize.updateInputOutputSize(IMAGE_WIDTH_IN, IMAGE_HEIGHT_IN, IMAGE_WIDTH_OUT, IMAGE_HEIGHT_OUT);
    resize.update(resize.outputStride, IMAGE_HEIGHT_OUT);
    resize.run(1);
    resize.wait();
    resize.end();

    return 0;
}
#endif
