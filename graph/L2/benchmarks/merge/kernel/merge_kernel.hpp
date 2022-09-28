/*
 * Copyright 2022 Xilinx, Inc.
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
#include "ap_int.h"
#include <hls_stream.h>
#include <hls_burst_maxi.h>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sys/time.h>
#include <vector>
#include <limits>
#include <algorithm>
#include <map>
#include <iterator>
#include <common.hpp>
using namespace std;

void merge_kernel(int num_v,
                  int num_e,
                  int num_c_out,
                  int* num_e_out,
                  DF_V_T* offset_in,
                  DF_E_T* edges_in,
                  // ap_uint<DWIDTHS> * edges_in,
                  DF_W_T* weights_in,
                  DF_V_T* c,
                  DF_V_T* offset_out,
                  DF_E_T* edges_out,
                  DF_W_T* weights_out,
                  DF_V_T* count_c_single,
#ifdef MANUALLY_BURST
                  hls::burst_maxi<DF_V_T> jump,
#else
                  DF_V_T* jump,
#endif
                  DF_V_T* count_c,
                  DF_V_T* index_c);
