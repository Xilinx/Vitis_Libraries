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
static constexpr int CORES = 4;
demosaicGraph<1> demo[CORES] = {{20, 0, 0}, {22, 0, 1}, {24, 0, 2}, {26, 0, 3}};

// initialize and run the dataflow graph
#if defined(__AIESIM__) || defined(__X86SIM__)
int main(int argc, char** argv) {
    for (int i = 0; i < CORES; i++) {
        // Empty
        demo[i].init();
        demo[i].run(NUM_TILES / CORES);
    }

    for (int i = 0; i < CORES; i++) {
        demo[i].wait();
        demo[i].end();
    }

    return 0;
}
#endif
