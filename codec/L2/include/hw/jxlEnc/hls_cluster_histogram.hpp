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

#ifndef HLS_CLUSTER_HISTOGRAM_HPP
#define HLS_CLUSTER_HISTOGRAM_HPP

#include "ap_int.h"
#include "hls_stream.h"
#include "hls_math.h"

namespace xf {
namespace codec {

/**
 * @brief JXL ANS cluster Histogram kernel
 *
 * @param config                    configuration for the kernel.
 * @param histograms0_ptr           histograms for Block Context Map.
 * @param histo_totalcnt0_ptr       Count of context for histograms for Block Context Map.
 * @param histo_size0_ptr           size for each context
 * @param nonempty_histo0_ptr       indicate which context is empty
 * @param ctx_map0_ptr              the input context map
 * @param histograms_clusd0_ptr     the clustered histogram
 * @param histograms_clusdin0_ptr   the context for the clustered histogram
 * @param histograms1_ptr           histograms for Modular frame tree.
 * @param histo_totalcnt1_ptr       Count of context for histograms for Modular frame tree.
 * @param histo_size1_ptr           size for each context
 * @param nonempty_histo1_ptr       indicate which context is empty
 * @param ctx_map1_ptr              the input context map
 * @param histograms_clusd1_ptr     the clustered histogram
 * @param histograms_clusdin1_ptr   the context for the clustered histogram
 * @param histograms2_ptr           histograms for code from Modular frame.
 * @param histo_totalcnt2_ptr       Count of context for histograms for Modular frame.
 * @param histo_size2_ptr           size for each context
 * @param nonempty_histo2_ptr       indicate which context is empty
 * @param ctx_map2_ptr              the input context map
 * @param histograms_clusd2_ptr     the clustered histogram
 * @param histograms_clusdin2_ptr   the context for the clustered histogram
 * @param histograms3_ptr           histograms for coef orders.
 * @param histo_totalcnt3_ptr       Count of context for histograms for coef orders.
 * @param histo_size3_ptr           size for each context
 * @param nonempty_histo3_ptr       indicate which context is empty
 * @param ctx_map3_ptr              the input context map
 * @param histograms_clusd3_ptr     the clustered histogram
 * @param histograms_clusdin3_ptr   the context for the clustered histogram
 * @param histograms4_ptr           histograms for ac coefficients.
 * @param histo_totalcnt4_ptr       Count of context for histograms for ac coefficients.
 * @param histo_size4_ptr           size for each context
 * @param nonempty_histo4_ptr       indicate which context is empty
 * @param ctx_map4_ptr              the input context map
 * @param histograms_clusd4_ptr     the clustered histogram
 * @param histograms_clusdin4_ptr   the context for the clustered histogram
 */

extern "C" void JxlEnc_ans_clusterHistogram(uint32_t* config,
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
} // namespace codec
} // namespace xf
#endif
