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

// initialize and run the dataflow graph
#if defined(__AIESIM__) || defined(__X86SIM__)
int main(int argc, char** argv) {
    // Empty
    resize_norm.init();
    resize_norm.update(resize_norm.a0, 104);
    resize_norm.update(resize_norm.a1, 107);
    resize_norm.update(resize_norm.a2, 123);
    resize_norm.update(resize_norm.a3, 0);
    resize_norm.update(resize_norm.b0, 8);
    resize_norm.update(resize_norm.b1, 8);
    resize_norm.update(resize_norm.b2, 8);
    resize_norm.update(resize_norm.b3, 0);

    resize_norm.run(IMAGE_HEIGHT_OUT);
    resize_norm.wait();
    resize_norm.end();

    return 0;
}
#endif
