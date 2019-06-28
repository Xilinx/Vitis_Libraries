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
#ifndef AMIN_TOP_H
#define AMIN_TOP_H

#include "amaxmin.h"
#ifndef BLAS_dataWidth
#define BLAS_dataWidth (sizeof(BLAS_dataType) * 8)
#endif


void amaxmin_top(
  unsigned int p_n,
  hls::stream<ap_uint<BLAS_dataWidth * BLAS_parEntries> > &p_x,
  BLAS_indexType &p_reault 
);

void UUT_Top(
  BLAS_dataType p_in[BLAS_size],
  unsigned int p_n,
  BLAS_indexType &p_result
);

#endif
