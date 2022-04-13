/*
 * (c) Copyright 2022 Xilinx, Inc. All rights reserved.
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
 *
 */

/**
 * @file sw_maxscore_systolic.hpp
 * @brief Header for Smithwaterman Kernel.
 *
 * This file is part of Vitis Genomics Library.
 */

#include "sw.h"
#include "swmaxscore_compute.hpp"

extern "C" {
/**
 * @brief Systolic implementation of smith-waterman
 *
 * @param input input raw data
 * @param output max score
 * @param size number of iterations
 */
void opencl_sw_maxscore(ap_uint<NUMPACKED * 2>* input, ap_uint<NUMPACKED * 2>* output, int* size);
}
