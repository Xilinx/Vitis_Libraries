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

#ifndef _DIAMETER_KERNEL_HPP_
#define _DIAMETER_KERNEL_HPP_

#include "hw/diameter.hpp"

#include <ap_int.h>
#include <hls_stream.h>

#define FADD_LTNCY 5
#define PARA_NUM 8        // Degree of paralleism
#define LOOP_NUM 1        // Interator size
#define RANDOM_OFFSET 380 // Start point

extern "C" void diameter_kernel(unsigned numVert,
                                unsigned numEdge,
                                unsigned* offset,
                                unsigned* column,
                                float* weight,

                                float* max_dist,
                                unsigned* src,
                                unsigned* des);

#endif
