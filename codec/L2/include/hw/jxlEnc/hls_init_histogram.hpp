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

#ifndef HLS_INIT_HISTOGRAM_HPP
#define HLS_INIT_HISTOGRAM_HPP

#include <ap_int.h>
#include <hls_stream.h>
#include <stdint.h>

const int PIXEL_W = 2048;
const int PIXEL_H = 2048;
const int FRAME_DIM = 3;
const int ALL_PIXEL = PIXEL_W * PIXEL_H * FRAME_DIM;
const int MAX_NUM_BLK88_W = PIXEL_W / 8;
const int MAX_NUM_BLK88_H = PIXEL_H / 8;
const int MAX_NUM_BLK88 = MAX_NUM_BLK88_W * MAX_NUM_BLK88_H;
const int MAX_ORDERS_SIZE = (3 * 64 + 3 * 64 + 3 * 256 + 3 * 1024);
const int MAX_QF_THRESH_SIZE = 256;
const int MAX_CTX_MAP_SIZE = 256;
const int MAX_AC_TOKEN_SIZE = ALL_PIXEL;

namespace xf {
namespace codec {

/**
* @brief JXL ANS init Histogram kernel
*
* @param config                    configuration for the kernel.
* @param ac_coef_ordered_ddr       ac coefficients
* @param strategy_ddr              ac strategy
* @param qf_ddr                    quant field
* @param qdc_ddr                   qdc
* @param ctx_map                   ctx_map ddr
* @param qf_thresholds             quantfield_thresholds
* @param ac_tokens_ddr             the ouput of ac tokens
* @param token0_ptr                tokens for Block Context Map
* @param token1_ptr                tokens for Modular frame tree
* @param token2_ptr                tokens for coef orders
* @param token3_ptr                tokens for Modular frames
* @param histograms0_ptr           histograms for Block Context Map.
* @param histo_totalcnt0_ptr       Count of context for histograms for Block Context Map.
* @param histo_size0_ptr           size for each context
* @param nonempty_histo0_ptr       indicate which context is empty
* @param histograms1_ptr           histograms for Modular frame tree.
* @param histo_totalcnt1_ptr       Count of context for histograms for Modular frame tree.
* @param histo_size1_ptr           size for each context
* @param nonempty_histo1_ptr       indicate which context is empty
* @param histograms2_ptr           histograms for code from Modular frame.
* @param histo_totalcnt2_ptr       Count of context for histograms for Modular frame.
* @param histo_size2_ptr           size for each context
* @param nonempty_histo2_ptr       indicate which context is empty
* @param histograms3_ptr           histograms for coef orders.
* @param histo_totalcnt3_ptr       Count of context for histograms for coef orders.
* @param histo_size3_ptr           size for each context
* @param nonempty_histo3_ptr       indicate which context is empty
* @param histograms4_ptr           histograms for ac coefficients.
* @param histo_totalcnt4_ptr       Count of context for histograms for ac coefficients.
* @param histo_size4_ptr           size for each context
* @param nonempty_histo4_ptr       indicate which context is empty
*/

extern "C" void JxlEnc_ans_initHistogram(
    //===============================================
    int config[32], // HBM-7
    //========================================
    int32_t ac_coeff_ordered_ddr[ALL_PIXEL],   // HBM-2
    int32_t strategy_ddr[MAX_NUM_BLK88],       // HBM-3
    int32_t qf_ddr[MAX_NUM_BLK88],             // HBM-4
    uint8_t qdc_ddr[MAX_NUM_BLK88],            // HBM-5
    uint8_t ctx_map[MAX_QF_THRESH_SIZE],       // HBM-6
    uint32_t qf_thresholds[MAX_CTX_MAP_SIZE],  // HBM-6
    uint64_t ac_tokens_ddr[MAX_AC_TOKEN_SIZE], // HBM-8
    //======================================
    ap_uint<64>* tokens0_ptr, // HBM-9
    ap_uint<64>* tokens1_ptr, // HBM-10
    ap_uint<64>* tokens2_ptr, // HBM-11
    ap_uint<64>* tokens3_ptr, // HBM-12
    //=====================================
    int32_t* histograms0_ptr,       // HBM-10
    uint32_t* histograms_size0_ptr, // HBM-11
    uint32_t* total_count0_ptr,     // HBM-12
    uint32_t* nonempty0_ptr,        // HBM-9
    //=====================================
    int32_t* histograms1_ptr,       // HBM-10
    uint32_t* histograms_size1_ptr, // HBM-11
    uint32_t* total_count1_ptr,     // HBM-12
    uint32_t* nonempty1_ptr,        // HBM-9
    //=====================================
    int32_t* histograms2_ptr,       // HBM-10
    uint32_t* histograms_size2_ptr, // HBM-11
    uint32_t* total_count2_ptr,     // HBM-12
    uint32_t* nonempty2_ptr,        // HBM-9
    //=====================================
    int32_t* histograms3_ptr,       // 24
    uint32_t* histograms_size3_ptr, // 25
    uint32_t* total_count3_ptr,     // 26
    uint32_t* nonempty3_ptr,        // 27
    //=====================================
    int32_t* histograms4_ptr,       // 28
    uint32_t* histograms_size4_ptr, // 29
    uint32_t* total_count4_ptr,     // 30
    uint32_t* nonempty4_ptr         // 31
    );
} // namespace codec
} // namespace xf

#endif
