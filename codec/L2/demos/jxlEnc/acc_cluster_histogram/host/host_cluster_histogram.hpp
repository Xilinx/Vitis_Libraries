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

#ifndef HOST_CLUSTER_HISTOGRAM_HPP
#define HOST_CLUSTER_HISTOGRAM_HPP

#include <iostream>
#include <sys/time.h>
#include "xcl2.hpp"
#include "xf_utils_sw/logger.hpp"

void hls_ANSclusterHistogram_wrapper(std::string xclbinPath,
                                     uint32_t* config,
                                     //====================
                                     int32_t* histograms0_ptr,
                                     uint32_t* histo_totalcnt0_ptr,
                                     uint32_t* histo_size0_ptr,
                                     uint32_t* nonempty_histo0_ptr,
                                     uint8_t* ctx_map0_ptr,
                                     int32_t* histograms_clusd0_ptr,
                                     uint32_t* histo_size_clusd0_ptr,
                                     int32_t* histograms_clusdin0_ptr,
                                     //====================
                                     int32_t* histograms1_ptr,
                                     uint32_t* histo_totalcnt1_ptr,
                                     uint32_t* histo_size1_ptr,
                                     uint32_t* nonempty_histo1_ptr,
                                     uint8_t* ctx_map1_ptr,
                                     int32_t* histograms_clusd1_ptr,
                                     uint32_t* histo_size_clusd1_ptr,
                                     int32_t* histograms_clusdin1_ptr,
                                     //======================
                                     int32_t* histograms2_ptr,
                                     uint32_t* histo_totalcnt2_ptr,
                                     uint32_t* histo_size2_ptr,
                                     uint32_t* nonempty_histo2_ptr,
                                     uint8_t* ctx_map2_ptr,
                                     int32_t* histograms_clusd2_ptr,
                                     uint32_t* histo_size_clusd2_ptr,
                                     int32_t* histograms_clusdin2_ptr,
                                     //======================
                                     int32_t* histograms3_ptr,
                                     uint32_t* histo_totalcnt3_ptr,
                                     uint32_t* histo_size3_ptr,
                                     uint32_t* nonempty_histo3_ptr,
                                     uint8_t* ctx_map3_ptr,
                                     int32_t* histograms_clusd3_ptr,
                                     uint32_t* histo_size_clusd3_ptr,
                                     int32_t* histograms_clusdin3_ptr,
                                     //======================
                                     int32_t* histograms4_ptr,
                                     uint32_t* histo_totalcnt4_ptr,
                                     uint32_t* histo_size4_ptr,
                                     uint32_t* nonempty_histo4_ptr,
                                     uint8_t* ctx_map4_ptr,
                                     int32_t* histograms_clusd4_ptr,
                                     uint32_t* histo_size_clusd4_ptr,
                                     int32_t* histograms_clusdin4_ptr);

#endif
