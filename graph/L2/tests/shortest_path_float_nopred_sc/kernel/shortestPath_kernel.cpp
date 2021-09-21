/*
 * Copyright 2019-2021 Xilinx, Inc.
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

#include "shortestPath_acc.hpp"

#ifndef __SYNTHESIS__
#include <iostream>
#endif

void shortestPath_acc::hls_kernel(int numVertices,
                                  int numEdges,
                                  ap_uint<32>* config32,
                                  ap_uint<512>* offset512,
                                  ap_uint<512>* column512,
                                  ap_uint<512>* weight512,

                                  ap_uint<512>* queue512,
                                  ap_uint<32>* queue32,

                                  ap_uint<512>* result512,
                                  ap_uint<32>* result32,

                                  ap_uint<8>* info8) {
#ifndef __SYNTHESIS__
    std::cout << "kernel call success" << std::endl;
#endif
    xf::graph::singleSourceShortestPath<32, MAXOUTDEGREE>(config32, offset512, column512, weight512, queue512, queue32,
                                                          result512, result32, info8);
#ifndef __SYNTHESIS__
    std::cout << "info[0:1]=" << info8[0] << " : " << info8[1] << std::endl;
    std::cout << "kernel call finish" << std::endl;
#endif
}
