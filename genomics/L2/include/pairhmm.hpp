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
#ifndef _PAIRHMM_HPP_
#define _PAIRHMM_HPP_

/**
 * @file pairhmm.hpp
 * @brief Header for PairHMM Kernel.
 *
 * This file is part of Vitis Genomics Library.
 */

#include "phmmmodules.hpp"
// Kernel top functions
extern "C" {
/**
 * @brief PairHMM kernel takes read/ref sequence as input and generates the
 * likelihood ratio.
 *
 * @param in input sequence data
 * @param output_data likelihood ratio
 * @param numRead number of read sequences
 * @param numHap number of hap sequences
 */
void pairhmm(ap_uint<GMEM_DWIDTH>* in, float* output_data, int numRead, int numHap);
}
#endif
