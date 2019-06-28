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
#ifndef UUT_TOP_H
#define UUT_TOP_H

int uut_top(
  uint32_t p_n,
  BLAS_dataType p_alpha,
  BLAS_dataType *p_x,
  BLAS_dataType *p_y,
  BLAS_dataType *p_xRes,
  BLAS_dataType *p_yRes,
  BLAS_resDataType p_goldRes
);
#endif
