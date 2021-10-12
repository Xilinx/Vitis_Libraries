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

#ifndef _XF_GRAPH_MST_TOP_HPP_
#define _XF_GRAPH_MST_TOP_HPP_

#include "hw/mst.hpp"

extern "C" void mst_top(unsigned int numVert,
                        unsigned int numEdge,
                        unsigned int source,
                        unsigned int* offset,
                        unsigned int* column,
                        float* weight,
                        unsigned* mst);

#endif
