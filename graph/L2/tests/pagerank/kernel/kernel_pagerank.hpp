/*
 * Copyright 2019 Xilinx, Inc.
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

#include "xf_graph_L2.hpp"

#define DT double
#define unrollBin 3 // unroll order
#define unrollNm (1 << unrollBin)
#define widthOr 256
#define maxVertex (67108864 / unrollNm)
#define maxEdge (67108864 / unrollNm)

typedef ap_uint<512> buffType;

#define depNrow 1   // offsetCSC buffer depth of 512 bits
#define depNrow0 2  // pagerank, cntValFull, buffPing, buffPong buffers depth of 512 bits
#define depNNZ 2    // indiceCSC buffer depth of 512 bits
#define depDegree 2 // degreeCSR buffer depth of 512 bits
#define depOrder 4  // orderUnroll buffer depth of 512 bits(float) / 256 bits(double)

extern "C" void kernel_pagerank_0(int nrows,
                                  int nnz,
                                  DT alpha,
                                  DT tolerance,
                                  int maxIter,
                                  buffType* pagerank,
                                  buffType* degreeCSR,
                                  buffType* offsetCSC,
                                  buffType* indiceCSC,
                                  buffType* cntValFull,
                                  buffType* buffPing,
                                  buffType* buffPong,
                                  ap_uint<widthOr>* orderUnroll);
