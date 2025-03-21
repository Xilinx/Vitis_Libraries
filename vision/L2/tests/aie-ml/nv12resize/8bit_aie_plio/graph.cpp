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
#include <stdio.h>
#include <cstdio>
// Graph object
myGraph nv12resize_graph;
myGraph1 nv12resize_uvgraph;

#if defined(__AIESIM__) || defined(__X86SIM__)
#include <common/xf_aie_utils.hpp>
int main(int argc, char** argv) {
    nv12resize_graph.init();
    nv12resize_graph.run(1);
    nv12resize_graph.end();
    printf("Y graph run finihed\n");

    nv12resize_uvgraph.init();
    nv12resize_uvgraph.run(1);
    nv12resize_uvgraph.end();
    printf("UV graph run finihed\n");

    return 0;
}
#endif
