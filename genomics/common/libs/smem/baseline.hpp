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
#ifndef BASELINE_HPP
#define BASELINE_HPP
#include <stdio.h>
#include "host_types.hpp"
#include "ocl.hpp"

#define bwt_set_intv1(c, ik)                                                                               \
    ((ik).x[0] = L2_baseline[(int)(c)] + 1, (ik).x[2] = L2_baseline[(int)(c) + 1] - L2_baseline[(int)(c)], \
     (ik).x[1] = L2_baseline[3 - (c)] + 1, (ik).info = 0)

void mem_collect_intv_new(uint32_t* bwt, int len, const uint8_t* seq, smem_aux_t* a);
int smem_baseline(const uint32_t* bwt,
                  const uint64_t* bwt_para,
                  uint8_t* seq,
                  uint8_t* seq_len,
                  int batch_size,
                  bwtintv_t* mem_output,
                  int* mem_num,
                  double mem_request_size[COMPUTE_UNIT]);

#endif
