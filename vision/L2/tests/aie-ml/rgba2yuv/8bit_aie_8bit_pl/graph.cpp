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
using namespace adf;

// instantiate adf dataflow graph to compute weighted moving average
rgba2yuvGraph mygraph;

// initialize and run the dataflow graph
#if defined(__AIESIM__) || defined(__X86SIM__)

int main(int argc, char** argv) {
    mygraph.init();
    uint16 tile_width = TILE_WIDTH;
    uint16 tile_height = TILE_HEIGHT;

    mygraph.update(mygraph.tile_width, tile_width);
    mygraph.update(mygraph.tile_height, tile_height);
    mygraph.run(1);
    mygraph.end();

    return 0;
}
#endif
