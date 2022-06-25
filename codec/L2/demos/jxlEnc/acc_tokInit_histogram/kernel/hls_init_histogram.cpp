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

#ifndef HLS_INIT_HISTOGRAM_CPP
#define HLS_INIT_HISTOGRAM_CPP

#include "hls_init_histogram.hpp"
#include "hls_math.h"

#define kBlockDim 8
#define kDCTBlockSize kBlockDim* kBlockDim
#define kNonZeroBuckets 37
#define kZeroDensityContextCount 458
//====================================================================================//
// hls_initHistogram_qc.cpp
//====================================================================================//
void hls_InitHistogram(hls::stream<ap_uint<64> >& token_stream,
#ifndef __SYNTHESIS__
                       std::vector<std::vector<int32_t> >& histograms_uram,
#else
                       int32_t histograms_uram[4096][40],
#endif
                       uint32_t histograms_size[4096],
                       uint32_t total_count[4096],
                       uint32_t& nempty_cnt,
                       uint32_t nonempty_[4096],
                       uint32_t& large_idx) {
#pragma HLS INLINE off

    int32_t histo_reg[4] = {0, 0, 0, 0};
#pragma HLS array_partition variable = histo_reg complete dim = 1
    int32_t histo_ctx[4] = {-1, -1, -1, -1};
#pragma HLS array_partition variable = histo_ctx complete dim = 1
    int32_t histo_tok[4] = {-1, -1, -1, -1};
#pragma HLS array_partition variable = histo_tok complete dim = 1

    uint32_t totalcnt_reg[4] = {0, 0, 0, 0};
#pragma HLS array_partition variable = totalcnt_reg complete dim = 1

    uint32_t histo_size_reg[4] = {0, 0, 0, 0};
#pragma HLS array_partition variable = histo_size_reg complete dim = 1

    ap_uint<64> token_reg = 0;
    token_reg = token_stream.read();

    nempty_cnt = 0;
    uint32_t max_totalcnt = 0;

    int tmp_test = 0; // csim-only

INIT_HISTOGRAM_LOOP:
    while (token_reg[63] != 1) {
#pragma HLS PIPELINE II = 1
#pragma HLS DEPENDENCE variable = total_count inter false
#pragma HLS DEPENDENCE variable = histograms_uram inter false
#pragma HLS DEPENDENCE variable = histograms_size inter false

        // csim-only
        tmp_test++;

        ap_uint<32> value = token_reg.range(31, 0);
        ap_uint<31> context = token_reg.range(62, 32);
        uint32_t tok;

        if (value < 16) {
            tok = value;
        } else {
            uint32_t n = 32 - value.countLeadingZeros() - 1;
            uint32_t m = value - (1 << n);
            tok = 16 + ((n - 4) << 2) + (m >> (n - 2));
        }
        int32_t histo_read;
        int32_t histo_write;
        if (context == histo_ctx[0] && tok == histo_tok[0]) {
            histo_read = histo_reg[0];
        } else if (context == histo_ctx[1] && tok == histo_tok[1]) {
            histo_read = histo_reg[1];
        } else if (context == histo_ctx[2] && tok == histo_tok[2]) {
            histo_read = histo_reg[2];
        } else if (context == histo_ctx[3] && tok == histo_tok[3]) {
            histo_read = histo_reg[3];
        } else {
            histo_read = histograms_uram[context][tok];
        }
        histo_write = histo_read + 1;

        uint32_t tot_cnt_read;
        uint32_t tot_cnt_write;
        uint32_t siz_read;
        uint32_t siz_write;
        if (context == histo_ctx[0]) {
            tot_cnt_read = totalcnt_reg[0];
            siz_read = histo_size_reg[0];
        } else if (context == histo_ctx[1]) {
            tot_cnt_read = totalcnt_reg[1];
            siz_read = histo_size_reg[1];
        } else if (context == histo_ctx[2]) {
            tot_cnt_read = totalcnt_reg[2];
            siz_read = histo_size_reg[2];
        } else if (context == histo_ctx[3]) {
            tot_cnt_read = totalcnt_reg[3];
            siz_read = histo_size_reg[3];
        } else {
            tot_cnt_read = total_count[context];
            siz_read = histograms_size[context];
        }

        tot_cnt_write = tot_cnt_read + 1;

        if (tot_cnt_read == 0) {
            nonempty_[nempty_cnt] = context;
            nempty_cnt++;
        }
        if (tot_cnt_write > max_totalcnt) {
            large_idx = context;
            max_totalcnt = tot_cnt_write;
        }

        if (siz_read <= tok) {
            siz_write = (tok + 8) / 8 * 8;
        } else {
            siz_write = siz_read;
        }

        token_reg = token_stream.read();
        histograms_uram[context][tok] = histo_write; // II=1
        histo_reg[3] = histo_reg[2];
        histo_reg[2] = histo_reg[1];
        histo_reg[1] = histo_reg[0];
        histo_reg[0] = histo_write;
        histo_ctx[3] = histo_ctx[2];
        histo_ctx[2] = histo_ctx[1];
        histo_ctx[1] = histo_ctx[0];
        histo_ctx[0] = context;
        histo_tok[3] = histo_tok[2];
        histo_tok[2] = histo_tok[1];
        histo_tok[1] = histo_tok[0];
        histo_tok[0] = tok;
        total_count[context] = tot_cnt_write; // shoulde be II=1
        totalcnt_reg[3] = totalcnt_reg[2];
        totalcnt_reg[2] = totalcnt_reg[1];
        totalcnt_reg[1] = totalcnt_reg[0];
        totalcnt_reg[0] = tot_cnt_write;
        histograms_size[context] = siz_write;
        histo_size_reg[3] = histo_size_reg[2];
        histo_size_reg[2] = histo_size_reg[1];
        histo_size_reg[1] = histo_size_reg[0];
        histo_size_reg[0] = siz_write;
    }
}

void init_histogram_core(hls::stream<ap_uint<64> >& token_stream,
                         int32_t* histograms_ptr,
                         uint32_t* histograms_size_ptr,
                         uint32_t* total_count_ptr,
                         uint32_t* nonempty_ptr,
                         hls::stream<uint32_t>& strm_nempty_cnt,
                         hls::stream<uint32_t>& strm_largest_idx) {
#pragma HLS INLINE off

#ifndef __SYNTHESIS__
    std::vector<std::vector<int32_t> > histograms_uram(4096, std::vector<int32_t>(40));
#else
    int32_t histograms_uram[4096][40]; // pragma
#pragma HLS BIND_STORAGE impl = URAM variable = histograms_uram type = ram_s2p
#pragma HLS ARRAY_PARTITION variable = histograms_uram complete dim = 2
                                       // uram pargma
#endif

    uint32_t histograms_size[4096];
    uint32_t total_count[4096];
    uint32_t nonempty_[4096];
    uint32_t nempty_cnt;

HISTOGRAM_URAM_INIT_LOOP:
    for (int j = 0; j < 4096; j++) {
#pragma HLS PIPELINE II = 1
        histograms_size[j] = 0;
        total_count[j] = 0;
        for (int k = 0; k < 40; k++) {
#pragma HLS UNROLL
            histograms_uram[j][k] = 0;
        }
    }

    uint32_t largest_idx_tmp = 0;

    hls_InitHistogram(token_stream, histograms_uram, histograms_size, total_count, nempty_cnt, nonempty_,
                      largest_idx_tmp);

    // nempty_cnt_ptr = nempty_cnt;
    strm_nempty_cnt.write(nempty_cnt);
    strm_largest_idx.write(largest_idx_tmp);

    uint32_t nempty_context;
HISTOGRAM_WRITEOUT_LOOP:
    for (int i = 0; i < nempty_cnt; i++) {
        for (int j = 0; j < 40; j++) {
#pragma HLS PIPELINE II = 1
            if (j == 0) nempty_context = nonempty_[i];
            histograms_ptr[nempty_context * 40 + j] = histograms_uram[nempty_context][j];
        }
    }

HISTOGRAM_SIZE_WRITEOUT_LOOP:
    for (int j = 0; j < 4096; j++) {
#pragma HLS PIPELINE II = 1
        histograms_size_ptr[j] = histograms_size[j];
    }

HISTOGRAM_CNT_WRITEOUT_LOOP:
    for (int j = 0; j < 4096; j++) {
#pragma HLS PIPELINE II = 1
        total_count_ptr[j] = total_count[j];
    }

HISTOGRAM_NEMPTY_WRITEOUT_LOOP:
    for (int j = 0; j < 4096; j++) {
#pragma HLS PIPELINE II = 1
        nonempty_ptr[j] = nonempty_[j];
    }
}

void init_histogram_top(
    // bool do_once[5],
    hls::stream<int>& strm_do_once,
    hls::stream<ap_uint<64> >& token_stream0,
    hls::stream<ap_uint<64> >& token_stream1,
    hls::stream<ap_uint<64> >& token_stream2,
    hls::stream<ap_uint<64> >& token_stream3,
    hls::stream<ap_uint<64> >& token_stream4,
    hls::stream<uint32_t>& strm_nempty_cnt,
    hls::stream<uint32_t>& strm_largest_idx,

    int32_t* histograms0_ptr,
    uint32_t* histograms_size0_ptr,
    uint32_t* total_count0_ptr,
    uint32_t* nonempty0_ptr,

    int32_t* histograms1_ptr,
    uint32_t* histograms_size1_ptr,
    uint32_t* total_count1_ptr,
    uint32_t* nonempty1_ptr,

    int32_t* histograms2_ptr,
    uint32_t* histograms_size2_ptr,
    uint32_t* total_count2_ptr,
    uint32_t* nonempty2_ptr,

    int32_t* histograms3_ptr,
    uint32_t* histograms_size3_ptr,
    uint32_t* total_count3_ptr,
    uint32_t* nonempty3_ptr,

    int32_t* histograms4_ptr,
    uint32_t* histograms_size4_ptr,
    uint32_t* total_count4_ptr,
    uint32_t* nonempty4_ptr) {
#pragma HLS INLINE off
    int do_once[5];

    do_once[0] = strm_do_once.read();
    do_once[1] = strm_do_once.read();
    do_once[2] = strm_do_once.read();
    do_once[3] = strm_do_once.read();
    do_once[4] = strm_do_once.read();

    if (do_once[0]) {
        init_histogram_core(token_stream0, histograms0_ptr, histograms_size0_ptr, total_count0_ptr, nonempty0_ptr,
                            strm_nempty_cnt, strm_largest_idx);
    } else {
        strm_nempty_cnt.write(0);
        strm_largest_idx.write(0);
    }

    if (do_once[1]) {
        init_histogram_core(token_stream1, histograms1_ptr, histograms_size1_ptr, total_count1_ptr, nonempty1_ptr,
                            strm_nempty_cnt, strm_largest_idx);
    } else {
        strm_nempty_cnt.write(0);
        strm_largest_idx.write(0);
    }

    if (do_once[2]) {
        init_histogram_core(token_stream2, histograms2_ptr, histograms_size2_ptr, total_count2_ptr, nonempty2_ptr,
                            strm_nempty_cnt, strm_largest_idx);
    } else {
        strm_nempty_cnt.write(0);
        strm_largest_idx.write(0);
    }

    if (do_once[3]) {
        init_histogram_core(token_stream3, histograms3_ptr, histograms_size3_ptr, total_count3_ptr, nonempty3_ptr,
                            strm_nempty_cnt, strm_largest_idx);
    } else {
        strm_nempty_cnt.write(0);
        strm_largest_idx.write(0);
    }

    if (do_once[4]) {
        init_histogram_core(token_stream4, histograms4_ptr, histograms_size4_ptr, total_count4_ptr, nonempty4_ptr,
                            strm_nempty_cnt, strm_largest_idx);
    } else {
        strm_nempty_cnt.write(0);
        strm_largest_idx.write(0);
    }
}

void load_token(hls::stream<int>& strm_do_once, ap_uint<64>* tokens_ptr, hls::stream<ap_uint<64> >& token_stream) {
#pragma HLS INLINE off

    int enable = strm_do_once.read();
    if (enable) {
        ap_uint<64> token_reg;
        uint32_t token_size = tokens_ptr[0];
    LOAD_TOKEN_LOOP:
        for (int i = 0; i < (token_size + 1 + 256) / 256; i++) {
            for (int j = 0; j < 256; j++) {
#pragma HLS PIPELINE II = 1
                token_reg.range(62, 0) = tokens_ptr[i * 256 + j];
                token_reg[63] = 0;
                if (i * 256 + j != 0 && i * 256 + j < token_size + 1) token_stream.write(token_reg);
            }
        }
        token_reg[63] = 1;
        token_stream.write(token_reg);
    }
}
//=====================================================================================================//
// hls_enc_entropy_coder.cpp
//=====================================================================================================//
#define MAX_NUM_BLOCK88_JXL (256 / 8)

const uint8_t kNumOrders = 13;

uint8_t covered_blocks_x[] = {1, 1, 1, 1, 2, 4, 1, 2, 1, 4, 2, 4, 1, 1, 1, 1, 1, 1, 8, 4, 8, 16, 8, 16, 32, 16, 32};

uint8_t covered_blocks_y[] = {1, 1, 1, 1, 2, 4, 2, 1, 4, 1, 4, 2, 1, 1, 1, 1, 1, 1, 8, 8, 4, 16, 16, 8, 32, 32, 16};
uint64_t coverd_blocks_lut[] = {0, 0, 1, 0, 2, 0, 1, 0, 3};

uint16_t hls_kCoeffFreqContext[64] = {
    0xBAD, 0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 15, 16, 16, 17, 17,
    18,    18, 19, 19, 20, 20, 21, 21, 22, 22, 23, 23, 23, 23, 24, 24, 24, 24, 25, 25, 25, 25,
    26,    26, 26, 26, 27, 27, 27, 27, 28, 28, 28, 28, 29, 29, 29, 29, 30, 30, 30, 30,
};

uint16_t hls_kCoeffNumNonzeroContext[64] = {
    0xBAD, 0,   31,  62,  62,  93,  93,  93,  93,  123, 123, 123, 123, 152, 152, 152, 152, 152, 152, 152, 152, 180,
    180,   180, 180, 180, 180, 180, 180, 180, 180, 180, 180, 206, 206, 206, 206, 206, 206, 206, 206, 206, 206, 206,
    206,   206, 206, 206, 206, 206, 206, 206, 206, 206, 206, 206, 206, 206, 206, 206, 206, 206, 206, 206,
};

uint64_t hls_kCoeffOrderOffset[] = {
    0,   1,   2,   3,   4,   5,   6,   10,  14,  18,  34,   50,   66,   68,   70,   72,   76,   80,   84,   92,
    100, 108, 172, 236, 300, 332, 364, 396, 652, 908, 1164, 1292, 1420, 1548, 2572, 3596, 4620, 5132, 5644, 6156,
};

uint8_t hls_kStrategyOrder[] = {
    0, 1, 1, 1, 2, 3, 4, 4, 5, 5, 6, 6, 1, 1, 1, 1, 1, 1, 7, 8, 8, 9, 10, 10, 11, 12, 12,
};

uint8_t hls_kDefaultCtxMap[39] = {
    // Default ctx map clusters all the large transforms together.
    0, 1, 2, 2, 3,  3,  4,  5,  6,  6,  6,  6,  6,  //
    7, 8, 9, 9, 10, 11, 12, 13, 14, 14, 14, 14, 14, //
    7, 8, 9, 9, 10, 11, 12, 13, 14, 14, 14, 14, 14, //
};

uint32_t hls_covered_block_lut(int covered_blocks) {
#pragma HLS INLINE
    int log2_covered_blk = 0;
    if (covered_blocks == 4) {
        log2_covered_blk = 2;
    } else if (covered_blocks == 16) {
        log2_covered_blk = 4;
    } else {
        log2_covered_blk = 0;
    }

    return log2_covered_blk;
}

// Non-zero context is based on number of non-zeros and block context.
// For better clustering, contexts with same number of non-zeros are grouped.
uint32_t hls_ZeroDensityContextsOffset(uint64_t num_ctxs, uint32_t block_ctx) {
#pragma HLS INLINE
    return num_ctxs * kNonZeroBuckets + kZeroDensityContextCount * block_ctx;
}

// Specialization for 8x8, where only top-left is LLF/DC.
// About 1% overall speedup vs. NumNonZeroExceptLLF.
int32_t HLS_NumNonZero8x8ExceptDC(hls::stream<int32_t>& strm_ac_coeff_raster, int32_t* nzeros_pos) {
    int sum_zeros = 1;
HLS_COUNT_NZ8X8_INNER_LOOP:
    for (int k = 0; k < kBlockDim * kBlockDim; k++) {
#pragma HLS PIPELINE II = 1
        int32_t ac_coeff = strm_ac_coeff_raster.read();

        // strm_ac_coeff_raster_out.write(ac_coeff);
        if (k == 0) {
            continue;
        } else {
            if (!ac_coeff) {
                sum_zeros++;
            }
        }
    }

    *nzeros_pos = (kDCTBlockSize - sum_zeros);

    return (kDCTBlockSize - sum_zeros);
}

int hls_Is_FirstBlock(int by, int bx, int8_t strategy) {
#pragma HLS INLINE
    int32_t isFirstBlock = 0;

    if (strategy == 4) {
        if ((bx % 2 == 0) && (by % 2 == 0)) {
            isFirstBlock = 1;
        }
    } else if (strategy == 5) {
        if ((bx % 4 == 0) && (by % 4 == 0)) {
            isFirstBlock = 1;
        }
    } else {
        isFirstBlock = 1;
    }

    return isFirstBlock;
}

int32_t hls_PredictFromTopAndLeft(const int32_t* row_top,
                                  const int32_t* row,
                                  int covered_blocks,
                                  int log2_covered_blocks,
                                  int c,
                                  int32_t x,
                                  int32_t y,
                                  int32_t default_val) {
#pragma HLS INLINE
    int32_t predict_nzeros = 0;

    if (x == 0 && y == 0) {
        predict_nzeros = default_val;
    } else if (x == 0) {
        predict_nzeros = row_top[0]; // nzero_row_abv[0];
    } else if (y == 0) {
        predict_nzeros = row[x - 1]; // nzero_row_left[c];
    } else {
        predict_nzeros = (row_top[x] + row[x - 1] + 1) / 2;
    }

    return predict_nzeros;
}

// TODO(user): investigate, why disabling pre-clustering makes entropy code
// less dense. Perhaps we would need to add HQ clustering algorithm that would
// be able to squeeze better by spending more CPU cycles.
uint32_t hls_ZeroDensityContext(
    uint32_t nonzeros_left, uint32_t k, uint32_t covered_blocks, uint32_t log2_covered_blocks, uint32_t prev) {
#pragma HLS INLINE
    nonzeros_left = (nonzeros_left + covered_blocks - 1) >> log2_covered_blocks;
    k >>= log2_covered_blocks;

    return (hls_kCoeffNumNonzeroContext[nonzeros_left] + hls_kCoeffFreqContext[k]) * 2 + prev;
}

// Non-zero context is based on number of non-zeros and block context.
// For better clustering, contexts with same number of non-zeros are grouped.
uint32_t hls_NonZeroContext(uint64_t num_ctxs, uint32_t non_zeros, uint32_t block_ctx) {
#pragma HLS INLINE
    uint32_t ctx;
    if (non_zeros >= 64) non_zeros = 64;
    if (non_zeros < 8) {
        ctx = non_zeros;
    } else {
        ctx = 4 + non_zeros / 2;
    }
    return ctx * num_ctxs + block_ctx;
}

// Encodes non-negative (X) into (2 * X), negative (-X) into (2 * X - 1)
uint32_t hls_PackSigned(int32_t value) {
#pragma HLS INLINE
    // JXL_NO_SANITIZE("unsigned-integer-overflow") {
    return (static_cast<uint32_t>(value) << 1) ^ ((static_cast<uint32_t>(~value) >> 31) - 1);
}

int hls_dim_sanf_order(int i) {
#pragma HLS INLINE
    int c = 0;
    if (i == 0)
        c = 1;
    else if (i == 1)
        c = 0;
    else
        c = 2;

    return c;
}

uint64_t hls_Context(uint8_t* ctx_map,
                     uint32_t* qf_thresholds,
                     int qf_thresholds_size,
                     int kNumOrders,
                     int num_dc_ctxs,
                     int dc_idx,
                     int32_t qf,
                     uint64_t ord,
                     uint32_t c) {
#pragma HLS INLINE
    uint32_t qf_idx = 0;
    for (uint32_t i = 0; i < qf_thresholds_size; i++) {
#pragma HLS UNROLL
        if (qf > qf_thresholds[i]) qf_idx++;
    }

    uint32_t idx = c < 2 ? (c ^ 1) : 2;
    idx = idx * kNumOrders + ord;
    idx = idx * (qf_thresholds_size + 1) + qf_idx;
    idx = idx * num_dc_ctxs + dc_idx;
    return ctx_map[idx];
}

//==================== dataflow stage-1 ===========================//
void hls_count_nz(int ysize_blocks,
                  int xsize_blocks,
                  int nzeros_stride,
                  hls::stream<int32_t>& strm_ac_coeff_in,
                  hls::stream<int32_t>& strm_strategy_in,

                  hls::stream<int32_t>& strm_ac_coeff_out,
                  hls::stream<int32_t>& strm_strategy_out,
                  hls::stream<int32_t>& strm_nzeros,
                  hls::stream<int32_t>& strm_predict_nzeros) {
#pragma HLS INLINE off
    // bram
    int32_t nzero_row_left[3] = {0, 0, 0};
    int32_t nzero_row_0[MAX_NUM_BLOCK88_JXL * MAX_NUM_BLOCK88_JXL];
    int32_t nzero_row_1[MAX_NUM_BLOCK88_JXL * MAX_NUM_BLOCK88_JXL];
    int32_t nzero_row_2[MAX_NUM_BLOCK88_JXL * MAX_NUM_BLOCK88_JXL];

    // global config
    int32_t hls_strategy;

HLS_COUNT_NZ_OUTTER_LOOP:
    for (uint32_t by = 0; by < ysize_blocks; ++by) {
        for (uint32_t bx = 0; bx < xsize_blocks; ++bx) {
            for (int i = 0; i < 3; i++) {
                int c = hls_dim_sanf_order(i);
                int32_t* row_nzeros;
                int32_t* row_nzeros_top;

                if (c == 0) {
                    row_nzeros = &nzero_row_0[by * nzeros_stride + 0];
                    row_nzeros_top = &nzero_row_0[(by - 1) * nzeros_stride + 0];
                } else if (c == 1) {
                    row_nzeros = &nzero_row_1[by * nzeros_stride + 0];
                    row_nzeros_top = &nzero_row_1[(by - 1) * nzeros_stride + 0];

                } else {
                    row_nzeros = &nzero_row_2[by * nzeros_stride + 0];
                    row_nzeros_top = &nzero_row_2[(by - 1) * nzeros_stride + 0];
                }

                if (i == 0) {
                    hls_strategy = strm_strategy_in.read();
                    strm_strategy_out.write(hls_strategy);
                }

                bool hls_isFirstBlock = hls_Is_FirstBlock(by, bx, hls_strategy);
                uint32_t cx = covered_blocks_x[hls_strategy]; // lut
                uint32_t cy = covered_blocks_y[hls_strategy];
                const uint32_t covered_blocks = cx * cy; // = #LLF coefficients
                uint32_t log2_covered_blocks = hls_covered_block_lut(covered_blocks);
                uint32_t size = covered_blocks * kDCTBlockSize;

                if (hls_isFirstBlock) {
                    int32_t* nzeros_pos = row_nzeros + bx;
                    int num_zeros = 0;
                    for (int y = 0; y < cy * kBlockDim; y++) {
                        for (int x = 0; x < cx * kBlockDim; x++) {
#pragma HLS PIPELINE II = 1
                            int32_t ac_coeff = strm_ac_coeff_in.read();
                            strm_ac_coeff_out.write(ac_coeff);
                            if (!ac_coeff) {
                                num_zeros++;
                            }
                        }
                    }

                    //=============Move to an independent process, linked with hls::stream<int> num_zeros=======
                    int nzeros = int(cx * cy * kDCTBlockSize) - num_zeros;
                    const int32_t shifted_nzeros =
                        static_cast<int32_t>((nzeros + covered_blocks - 1) >> log2_covered_blocks);
                NZ_EXCEPT_LLF_INNER_LOOP3:
                    for (int32_t y = 0; y < cy; y++) {
                        for (int32_t x = 0; x < cx; x++) {
#pragma HLS PIPELINE II = 1
                            nzeros_pos[x + y * nzeros_stride] = shifted_nzeros;
                        }
                    }

                    int32_t predicted_nzeros = hls_PredictFromTopAndLeft(row_nzeros_top, row_nzeros, covered_blocks,
                                                                         log2_covered_blocks, c, bx, by, 32);

                    strm_nzeros.write(nzeros);
                    strm_predict_nzeros.write(predicted_nzeros);
                    //===============================
                }
            }
        }
    }
}

// void hls_block_context(
//     // config
//     int rect_x0,
//     int rect_y0,
//     int ysize_blocks,
//     int xsize_blocks,
//     int num_ctxs,
//     int num_dc_ctxs,
//     int qf_thresholds_size,
//     // bram
//     uint8_t ctx_map[MAX_CTX_MAP_SIZE],
//     uint32_t qf_thresholds[MAX_QF_THRESH_SIZE],
//     // strm
//     hls::stream<int32_t>& strm_qf,
//     hls::stream<uint8_t>& strm_qdc,
//     hls::stream<int32_t>& strm_strategy_in,
//     hls::stream<int32_t>& strm_strategy_out,
//     hls::stream<uint32_t>& strm_block_ctx) {
// #pragma HLS INLINE off

//     // global config
//     int hls_strategy;
//     uint8_t dc_idx;
//     int32_t hls_qf;

// Block_CTX_LOOP:
//     for (uint32_t by = 0; by < ysize_blocks; ++by) {
//         for (uint32_t bx = 0; bx < xsize_blocks; ++bx) {
//             for (int i = 0; i < 3; i++) {
// #pragma HLS PIPELINE II = 1

//                 if (i == 0) {
//                     hls_strategy = strm_strategy_in.read();
//                     strm_strategy_out.write(hls_strategy);

//                     dc_idx = strm_qdc.read();
//                     hls_qf = strm_qf.read();
//                 }
//                 int ord = hls_kStrategyOrder[hls_strategy];
//                 int c = hls_dim_sanf_order(i);
//                 uint32_t block_ctx = hls_Context(ctx_map, qf_thresholds, qf_thresholds_size, kNumOrders, num_dc_ctxs,
//                                                  dc_idx, hls_qf, ord, c);
//                 strm_block_ctx.write(block_ctx);
//             }
//         }
//     }
// }

//===================================================================================//
void hls_collect_syn(int xsize_blocks,
                     int ysize_blocks,
                     hls::stream<int32_t>& strm_strategy_in,
                     hls::stream<ap_uint<65> >& strm_token_nz,
                     hls::stream<ap_uint<65> >& strm_token_ac,
                     hls::stream<ap_uint<64> >& strm_token_out,
                     hls::stream<ap_uint<64> >& strm_token_internal) {
#pragma HLS INLINE off
    int hls_strategy;

COLLECT_SYN_OUTTER_LOOP:
    for (uint32_t by = 0; by < ysize_blocks; ++by) {
        for (uint32_t bx = 0; bx < xsize_blocks; ++bx) {
            for (int i = 0; i < 3; i++) {
                int c = hls_dim_sanf_order(i);
                if (i == 0) {
                    hls_strategy = strm_strategy_in.read();
                }
                bool hls_isFirstBlock = hls_Is_FirstBlock(by, bx, hls_strategy);
                // covered block size
                uint32_t cx = covered_blocks_x[hls_strategy]; // lut
                uint32_t cy = covered_blocks_y[hls_strategy];
                uint32_t covered_blocks = cx * cy; // = #LLF coefficients
                uint32_t size = covered_blocks * kDCTBlockSize;
                // loop in block
                if (hls_isFirstBlock) {
                COLLECT_SYN_INNER_LOOP:
                    for (int k = 0; k < size + 1; k++) {
#pragma HLS PIPELINE II = 1
                        if (k == 0) {
                            ap_uint<65> token_nz_reg = strm_token_nz.read();
                            ap_uint<64> token_out_reg = token_nz_reg.range(63, 0);
                            strm_token_out.write(token_out_reg);
                            strm_token_internal.write(token_out_reg);
                        } else {
                            ap_uint<65> token_ac_reg = strm_token_ac.read();
                            bool blk_end = token_ac_reg[64];

                            if (blk_end) {
                                break;
                            } else {
                                ap_uint<64> token_out_reg = token_ac_reg.range(63, 0);
                                strm_token_out.write(token_out_reg);
                                strm_token_internal.write(token_out_reg);
                            }
                        }
                    }
                }
            }
        }
    }
}

void hls_tokenize_nz(
    // config
    int rect_x0,
    int rect_y0,
    int ysize_blocks,
    int xsize_blocks,
    int num_ctxs,
    int num_dc_ctxs,
    int qf_thresholds_size,
    // bram
    uint8_t ctx_map[MAX_CTX_MAP_SIZE],
    uint32_t qf_thresholds[MAX_QF_THRESH_SIZE],
    // stream
    hls::stream<int32_t>& strm_qf,
    hls::stream<uint8_t>& strm_qdc,
    hls::stream<int32_t>& strm_strategy_in,
    hls::stream<int32_t>& strm_nzeros,
    hls::stream<int32_t>& strm_predict_nzeros,
    hls::stream<int32_t>& strm_strategy_out,
    hls::stream<int32_t>& strm_strategy_out2,
    hls::stream<uint32_t>& strm_histo_offset,
    hls::stream<int32_t>& strm_nzero_out,
    hls::stream<ap_uint<65> >& strm_token_nz) {
#pragma HLS INLINE off

    // global config
    int hls_strategy;
    uint8_t dc_idx;
    int32_t hls_qf;

TOKENIZE_NZ_LOOP:
    for (uint32_t by = 0; by < ysize_blocks; ++by) {
        for (uint32_t bx = 0; bx < xsize_blocks; ++bx) {
            for (int i = 0; i < 3; i++) {
#pragma HLS PIPELINE II = 1

                // only read 1 strategy per block
                if (i == 0) {
                    // strategy
                    hls_strategy = strm_strategy_in.read();
                    strm_strategy_out.write(hls_strategy);
                    strm_strategy_out2.write(hls_strategy);

                    // qdc & qf
                    dc_idx = strm_qdc.read();
                    hls_qf = strm_qf.read();
                }

                bool hls_isFirstBlock = hls_Is_FirstBlock(by, bx, hls_strategy);

                if (hls_isFirstBlock) {
                    int32_t nzeros = strm_nzeros.read();

                    strm_nzero_out.write(nzeros);

                    int32_t predicted_nzeros = strm_predict_nzeros.read();

                    //=================Move this block_ctx calculation into an independent process======
                    int ord = hls_kStrategyOrder[hls_strategy];
                    int c = hls_dim_sanf_order(i);
                    uint32_t block_ctx = hls_Context(ctx_map, qf_thresholds, qf_thresholds_size, kNumOrders,
                                                     num_dc_ctxs, dc_idx, hls_qf, ord, c);
                    //==================================================================================

                    int32_t nzero_ctx = hls_NonZeroContext(num_ctxs, predicted_nzeros, block_ctx);

                    uint32_t histo_offset = hls_ZeroDensityContextsOffset(num_ctxs, block_ctx);
                    strm_histo_offset.write(histo_offset);

                    ap_uint<65> token_nz_reg;
                    token_nz_reg.range(31, 0) = (uint32_t)nzeros;
                    token_nz_reg.range(63, 32) = (uint32_t)nzero_ctx;
                    token_nz_reg[64] = 0;
                    strm_token_nz.write(token_nz_reg);
                }
            }
        }
    }
}

void hls_tokenize_ac(int xsize_blocks,
                     int ysize_blocks,
                     hls::stream<int32_t>& strm_coeff_ordered,
                     hls::stream<int32_t>& strm_strategy_in,
                     hls::stream<uint32_t>& strm_histo_offset,
                     hls::stream<int32_t>& strm_nzeros_tokenAc,
                     hls::stream<ap_uint<65> >& strm_token_ac) {
#pragma HLS INLINE off
    // global variable
    int hls_block_offset = 0;
    ap_uint<64> token_reg;
    ap_uint<64> token_reg_out;
    uint32_t offset[3] = {};
    int hls_strategy;

TOKENIZE_AC_OUTTER_LOOP:
    for (uint32_t by = 0; by < ysize_blocks; ++by) {
        for (uint32_t bx = 0; bx < xsize_blocks; ++bx) {
            for (int i = 0; i < 3; i++) {
                if (i == 0) {
                    hls_strategy = strm_strategy_in.read();
                }

                int c = hls_dim_sanf_order(i);

                bool hls_isFirstBlock = hls_Is_FirstBlock(by, bx, hls_strategy);

                if (hls_isFirstBlock) {
                    uint32_t cx = covered_blocks_x[hls_strategy]; // lut
                    uint32_t cy = covered_blocks_y[hls_strategy];
                    const uint32_t covered_blocks = cx * cy; // = #LLF coefficients
                    uint32_t log2_covered_blocks = hls_covered_block_lut(covered_blocks);
                    uint32_t size = covered_blocks * kDCTBlockSize;
                    uint32_t histo_offset = strm_histo_offset.read();
                    int32_t nzeros = strm_nzeros_tokenAc.read();

                    // Skip LLF.
                    int32_t prev = (nzeros > (int32_t)(size / 16) ? 0 : 1);
                TOKENIZE_AC_INNER_LOOP:
                    for (int32_t k = 0; k < size; ++k) {
#pragma HLS PIPELINE II = 1
                        int32_t coeff = strm_coeff_ordered.read();
                        if (k >= covered_blocks) {
                            uint32_t ctx = histo_offset +
                                           hls_ZeroDensityContext(nzeros, k, covered_blocks, log2_covered_blocks, prev);

                            uint32_t u_coeff = hls_PackSigned(coeff);

                            if (nzeros > 0) {
                                ap_uint<65> token_ac_reg;
                                token_ac_reg.range(31, 0) = (uint32_t)u_coeff;
                                token_ac_reg.range(63, 32) = (uint32_t)ctx;
                                token_ac_reg[64] = 0; // block_end
                                strm_token_ac.write(token_ac_reg);

                                prev = coeff != 0;
                                nzeros -= prev;
                            }
                        }
                    }

                    // end of a block
                    ap_uint<65> token_ac_reg;
                    token_ac_reg.range(63, 0) = 0;
                    token_ac_reg[64] = 1; // block_end
                    strm_token_ac.write(token_ac_reg);

                    // offset
                    offset[c] += size;
                }
            }
        }
    }
}

void hls_ac_tokenize_core(int rect_x0,
                          int rect_y0,
                          int xsize_blocks,
                          int ysize_blocks,
                          int num_ctxs,
                          int num_dc_ctxs,
                          int qf_thresholds_size,
                          int nzeros_stride,
                          uint8_t ctx_map[MAX_QF_THRESH_SIZE],
                          uint32_t qf_thresholds[MAX_CTX_MAP_SIZE],
                          hls::stream<int32_t>& strm_ac_coeff,
                          hls::stream<int32_t>& strm_strategy,
                          hls::stream<int32_t>& strm_qf,
                          hls::stream<uint8_t>& strm_qdc,
                          hls::stream<ap_uint<64> >& strm_token_internal,
                          hls::stream<ap_uint<64> >& strm_token_out) {
#pragma HLS DATAFLOW
    hls::stream<int32_t, 1024> strm_ac_coeff_nz;
    hls::stream<int32_t, 16> strm_nzeros;
    hls::stream<int32_t, 16> strm_predict_nzeros;
    hls::stream<int32_t, 16> strm_strategy_0_1;
    hls::stream<int32_t, 16> strm_ac_ordered_0;
    hls_count_nz(
        // config
        ysize_blocks, xsize_blocks, nzeros_stride,
        // stream_in
        strm_ac_coeff, strm_strategy, strm_ac_coeff_nz,
        // stream_out
        strm_strategy_0_1, strm_nzeros, strm_predict_nzeros);

    // hls::stream<int32_t, 16> strm_block_ctx;
    // hls_block_context(rect_x0, rect_y0, ysize_blocks, xsize_blocks, num_ctxs, num_dc_ctxs, qf_thresholds_size,
    // ctx_map,
    //                   qf_thresholds, strm_qf, strm_qdc, strm_strategy_in, strm_strategy_out, strm_block_ctx);

    hls::stream<int32_t, 16> strm_strategy_1;
    hls::stream<int32_t, 16> strm_strategy_1_2;
    hls::stream<ap_uint<65>, 16> strm_token_nz;
    hls::stream<uint32_t, 16> strm_histo_offset;
    hls::stream<int32_t, 16> strm_nzeros2;
    hls_tokenize_nz(
        // config
        rect_x0, rect_y0, ysize_blocks, xsize_blocks, num_ctxs, num_dc_ctxs, qf_thresholds_size, ctx_map, qf_thresholds,
        // stream_in
        strm_qf, strm_qdc, strm_strategy_0_1, strm_nzeros, strm_predict_nzeros,
        // stream_out
        strm_strategy_1, strm_strategy_1_2, strm_histo_offset, strm_nzeros2, strm_token_nz);

    hls::stream<ap_uint<65>, 16> strm_token_ac;
    hls_tokenize_ac(
        // config
        xsize_blocks, ysize_blocks,
        // stream_in
        strm_ac_coeff_nz, strm_strategy_1_2, strm_histo_offset, strm_nzeros2,
        // sgream_out
        strm_token_ac);

    hls_collect_syn(
        // config
        xsize_blocks, ysize_blocks,
        // stream_in
        strm_strategy_1, strm_token_nz, strm_token_ac,
        // stream_out
        strm_token_out, strm_token_internal);
}

void load_ac_raster_by_group(hls::stream<int>& strm_config,
                             int32_t* ac_coeff_ddr,
                             hls::stream<int32_t>& strm_ac_coeff_raster) {
#pragma HLS INLINE off

    int group_dim = strm_config.read();
    int pixel_xsize = strm_config.read();
    int pixel_ysize = strm_config.read();

    int xsize_groups = (pixel_xsize + group_dim - 1) / group_dim;
    int ysize_groups = (pixel_ysize + group_dim - 1) / group_dim;
    int num_groups = xsize_groups * ysize_groups;

    uint64_t group_offset = 0;

LOAD_AC_RASTER_OUTTER_LOOP:
    for (int group_index = 0; group_index < num_groups; group_index++) {
        // paras-calculated
        int gx = group_index % xsize_groups;
        int gy = group_index / xsize_groups;
        int hls_x0 = gx * (group_dim >> 3);
        int hls_y0 = gy * (group_dim >> 3);
        // rect xsize_blocks& ysize_blocks
        int size_max = group_dim >> 3;
        int hls_xsize_blocks = (pixel_xsize + kBlockDim - 1) / kBlockDim;
        int hls_ysize_blocks = (pixel_ysize + kBlockDim - 1) / kBlockDim;
        int rect_xsize_blocks = (hls_x0 + size_max <= hls_xsize_blocks)
                                    ? size_max
                                    : (hls_xsize_blocks > hls_x0 ? hls_xsize_blocks - hls_x0 : 0);
        int rect_ysize_blocks = (hls_y0 + size_max <= hls_ysize_blocks)
                                    ? size_max
                                    : (hls_ysize_blocks > hls_y0 ? hls_ysize_blocks - hls_y0 : 0);
        // calculate core-config
        int rect_x0 = hls_x0;
        int rect_y0 = hls_y0;
        int xsize_blocks = rect_xsize_blocks;
        int ysize_blocks = rect_ysize_blocks;

    // loading ac_coeff by group
    LOAD_AC_RASTER_INNER_LOOP:
        for (int k = 0; k < xsize_blocks * ysize_blocks * kDCTBlockSize * 3; k++) {
#pragma HLS PIPELINE II = 1
            int32_t ac_coef_reg = ac_coeff_ddr[k + group_offset];
            strm_ac_coeff_raster.write(ac_coef_reg);
        }

        // move to next group set
        group_offset += ysize_blocks * xsize_blocks * kDCTBlockSize * 3;
    }
}

void load_ac_ordered_by_group(hls::stream<int>& strm_config,
                              int32_t* ac_coeff_ordered_ddr,
                              hls::stream<int32_t>& strm_ac_coeff0) {
#pragma HLS INLINE off

    int group_dim = strm_config.read();
    int pixel_xsize = strm_config.read();
    int pixel_ysize = strm_config.read();

    int xsize_groups = (pixel_xsize + group_dim - 1) / group_dim;
    int ysize_groups = (pixel_ysize + group_dim - 1) / group_dim;
    int num_groups = xsize_groups * ysize_groups;

    uint64_t group_offset = 0;

LOAD_AC_ORDERED_OUTTER_LOOP:
    for (int group_index = 0; group_index < num_groups; group_index++) {
        // paras-calculated
        int gx = group_index % xsize_groups;
        int gy = group_index / xsize_groups;
        int hls_x0 = gx * (group_dim >> 3);
        int hls_y0 = gy * (group_dim >> 3);
        // rect xsize_blocks& ysize_blocks
        int size_max = group_dim >> 3;
        int hls_xsize_blocks = (pixel_xsize + kBlockDim - 1) / kBlockDim;
        int hls_ysize_blocks = (pixel_ysize + kBlockDim - 1) / kBlockDim;
        int rect_xsize_blocks = (hls_x0 + size_max <= hls_xsize_blocks)
                                    ? size_max
                                    : (hls_xsize_blocks > hls_x0 ? hls_xsize_blocks - hls_x0 : 0);
        int rect_ysize_blocks = (hls_y0 + size_max <= hls_ysize_blocks)
                                    ? size_max
                                    : (hls_ysize_blocks > hls_y0 ? hls_ysize_blocks - hls_y0 : 0);
        // calculate core-config
        int rect_x0 = hls_x0;
        int rect_y0 = hls_y0;
        int xsize_blocks = rect_xsize_blocks;
        int ysize_blocks = rect_ysize_blocks;

    // loading ac_coeff by group
    LOAD_AC_ORDERED_INNER_LOOP:
        for (int k = 0; k < xsize_blocks * ysize_blocks * kDCTBlockSize * 3; k++) {
#pragma HLS PIPELINE II = 1
            int32_t ac_coef_ordered_reg = ac_coeff_ordered_ddr[k + group_offset];
            strm_ac_coeff0.write(ac_coef_ordered_reg);
        }

        // move to next group set
        group_offset += ysize_blocks * xsize_blocks * kDCTBlockSize * 3;
    }
}

void load_ac_strategy_by_group(hls::stream<int>& strm_config,
                               int32_t* strategy_ddr,
                               hls::stream<int32_t>& strm_strategy) {
#pragma HLS INLINE off

    // local calculated
    int group_dim = strm_config.read();
    int pixel_xsize = strm_config.read();
    int pixel_ysize = strm_config.read();

    // pre-process
    int xsize_groups = (pixel_xsize + group_dim - 1) / group_dim;
    int ysize_groups = (pixel_ysize + group_dim - 1) / group_dim;
    int num_groups = xsize_groups * ysize_groups;

    uint64_t group_offset = 0;

LOAD_AC_STRATEGY_OUTTER_LOOP:
    for (int group_index = 0; group_index < num_groups; group_index++) {
        // paras-calculated
        int gx = group_index % xsize_groups;
        int gy = group_index / xsize_groups;
        int hls_x0 = gx * (group_dim >> 3);
        int hls_y0 = gy * (group_dim >> 3);
        // rect xsize_blocks& ysize_blocks
        int size_max = group_dim >> 3;
        int hls_xsize_blocks = (pixel_xsize + kBlockDim - 1) / kBlockDim;
        int hls_ysize_blocks = (pixel_ysize + kBlockDim - 1) / kBlockDim;
        int rect_xsize_blocks = (hls_x0 + size_max <= hls_xsize_blocks)
                                    ? size_max
                                    : (hls_xsize_blocks > hls_x0 ? hls_xsize_blocks - hls_x0 : 0);
        int rect_ysize_blocks = (hls_y0 + size_max <= hls_ysize_blocks)
                                    ? size_max
                                    : (hls_ysize_blocks > hls_y0 ? hls_ysize_blocks - hls_y0 : 0);
        // calculate core-config
        int xsize_blocks = rect_xsize_blocks;
        int ysize_blocks = rect_ysize_blocks;

    // loading strategy by group
    LOAD_AC_STRATEGY_INNER_LOOP:
        for (int k = 0; k < xsize_blocks * ysize_blocks; k++) {
#pragma HLS PIPELINE II = 1
            int32_t strategy = strategy_ddr[k + group_offset];
            strm_strategy.write(strategy);
        }

        // move to next group set
        group_offset = group_offset + xsize_blocks * ysize_blocks;
    }
}

void load_qdc_by_group(hls::stream<int>& strm_config, uint8_t* qdc_ddr, hls::stream<uint8_t>& strm_qdc) {
#pragma HLS INLINE off

    int group_dim = strm_config.read();
    int pixel_xsize = strm_config.read();
    int pixel_ysize = strm_config.read();

    int xsize_groups = (pixel_xsize + group_dim - 1) / group_dim;
    int ysize_groups = (pixel_ysize + group_dim - 1) / group_dim;
    int num_groups = xsize_groups * ysize_groups;

    uint64_t group_offset = 0;
LOAD_QDC_OUTTER_LOOP:
    for (int group_index = 0; group_index < num_groups; group_index++) {
        // paras-calculated
        int gx = group_index % xsize_groups;
        int gy = group_index / xsize_groups;
        int hls_x0 = gx * (group_dim >> 3);
        int hls_y0 = gy * (group_dim >> 3);
        // rect xsize_blocks& ysize_blocks
        int size_max = group_dim >> 3;
        int hls_xsize_blocks = (pixel_xsize + kBlockDim - 1) / kBlockDim;
        int hls_ysize_blocks = (pixel_ysize + kBlockDim - 1) / kBlockDim;
        int rect_xsize_blocks = (hls_x0 + size_max <= hls_xsize_blocks)
                                    ? size_max
                                    : (hls_xsize_blocks > hls_x0 ? hls_xsize_blocks - hls_x0 : 0);
        int rect_ysize_blocks = (hls_y0 + size_max <= hls_ysize_blocks)
                                    ? size_max
                                    : (hls_ysize_blocks > hls_y0 ? hls_ysize_blocks - hls_y0 : 0);
        // calculate core-config
        int xsize_blocks = rect_xsize_blocks;
        int ysize_blocks = rect_ysize_blocks;

    LOAD_QDC_INNER_LOOP:
        for (int by = 0; by < ysize_blocks; by++) {
            for (int bx = 0; bx < xsize_blocks; bx++) {
#pragma HLS PIPELINE II = 1
                int32_t dc_idx = qdc_ddr[bx + by * xsize_blocks + group_offset];

                strm_qdc.write(dc_idx);
            }
        }

        group_offset += ysize_blocks * xsize_blocks;
    }
}

void load_qf_by_group(hls::stream<int>& strm_config, int32_t* qf_ddr, hls::stream<int32_t>& strm_qf) {
#pragma HLS INLINE off

    int group_dim = strm_config.read();
    int pixel_xsize = strm_config.read();
    int pixel_ysize = strm_config.read();

    int xsize_groups = (pixel_xsize + group_dim - 1) / group_dim;
    int ysize_groups = (pixel_ysize + group_dim - 1) / group_dim;
    int num_groups = xsize_groups * ysize_groups;

    uint64_t group_offset = 0;
LOAD_QF_OUTTER_LOOP:
    for (int group_index = 0; group_index < num_groups; group_index++) {
        // paras-calculated
        int gx = group_index % xsize_groups;
        int gy = group_index / xsize_groups;
        int hls_x0 = gx * (group_dim >> 3);
        int hls_y0 = gy * (group_dim >> 3);
        // rect xsize_blocks& ysize_blocks
        int size_max = group_dim >> 3;
        int hls_xsize_blocks = (pixel_xsize + kBlockDim - 1) / kBlockDim;
        int hls_ysize_blocks = (pixel_ysize + kBlockDim - 1) / kBlockDim;
        int rect_xsize_blocks = (hls_x0 + size_max <= hls_xsize_blocks)
                                    ? size_max
                                    : (hls_xsize_blocks > hls_x0 ? hls_xsize_blocks - hls_x0 : 0);
        int rect_ysize_blocks = (hls_y0 + size_max <= hls_ysize_blocks)
                                    ? size_max
                                    : (hls_ysize_blocks > hls_y0 ? hls_ysize_blocks - hls_y0 : 0);
        // calculate core-config
        int xsize_blocks = rect_xsize_blocks;
        int ysize_blocks = rect_ysize_blocks;

    LOAD_QF_INNER_LOOP:
        for (int by = 0; by < ysize_blocks; by++) {
            for (int bx = 0; bx < xsize_blocks; bx++) {
#pragma HLS PIPELINE II = 1
                uint8_t hls_qf = qf_ddr[bx + by * xsize_blocks + group_offset];
                strm_qf.write(hls_qf);
            }
        }

        group_offset += ysize_blocks * xsize_blocks;
    }
}

void ac_token_writeout(uint64_t* ac_tokens_ddr, hls::stream<ap_uint<64> >& strm_token_out) {
#pragma HLS INLINE off

    bool token_stream_end = 0;
    uint64_t idx_token = 0;

AC_TOKEN_WRITEOUT_LOOP:
    while (!token_stream_end) {
#pragma HLS PIPELINE II = 1
        ap_uint<64> token_reg = strm_token_out.read();
        ac_tokens_ddr[idx_token] = token_reg;

        token_stream_end = token_reg[63];
        idx_token++;
    }
}

void hls_TokenizeCoefficients(
    // bram
    uint8_t hls_ctx_map[MAX_CTX_MAP_SIZE],
    uint32_t hls_qf_thresholds[MAX_CTX_MAP_SIZE],
    // strm input
    hls::stream<int32_t>& strm_global_config,
    // size of pixel
    hls::stream<int32_t>& strm_ac_coeff_ordered,
    // size of blk_num
    hls::stream<int32_t>& strm_strategy,
    hls::stream<int32_t>& strm_qf,
    hls::stream<uint8_t>& strm_qdc,
    // size of bram
    hls::stream<ap_uint<64> >& strm_token_internal,
    // output
    hls::stream<ap_uint<64> >& strm_token_out

    ) {
#pragma HLS INLINE off

    // global config
    int group_dim = strm_global_config.read();
    int pixel_xsize = strm_global_config.read();
    int pixel_ysize = strm_global_config.read();
    int qf_thresholds_size = strm_global_config.read();
    int num_ctxs = strm_global_config.read();
    int num_dc_ctxs = strm_global_config.read();
    int nzeros_stride = strm_global_config.read();

    int xsize_groups = (pixel_xsize + group_dim - 1) / group_dim;
    int ysize_groups = (pixel_ysize + group_dim - 1) / group_dim;
    int num_groups = xsize_groups * ysize_groups;

    // global variable
    ap_uint<64> token_reg_out;

TOKENIZE_COEFF_LOOP:
    for (int group_index = 0; group_index < num_groups; group_index++) {
        // paras-calculated
        int gx = group_index % xsize_groups;
        int gy = group_index / xsize_groups;
        int hls_x0 = gx * (group_dim >> 3);
        int hls_y0 = gy * (group_dim >> 3);
        int size_max = group_dim >> 3;
        int hls_xsize_blocks = (pixel_xsize + kBlockDim - 1) / kBlockDim;
        int hls_ysize_blocks = (pixel_ysize + kBlockDim - 1) / kBlockDim;
        int xsize_blocks = (hls_x0 + size_max <= hls_xsize_blocks)
                               ? size_max
                               : (hls_xsize_blocks > hls_x0 ? hls_xsize_blocks - hls_x0 : 0);
        int ysize_blocks = (hls_y0 + size_max <= hls_ysize_blocks)
                               ? size_max
                               : (hls_ysize_blocks > hls_y0 ? hls_ysize_blocks - hls_y0 : 0);

        hls_ac_tokenize_core(hls_x0, hls_y0, xsize_blocks, ysize_blocks, num_ctxs, num_dc_ctxs, qf_thresholds_size,
                             nzeros_stride, hls_ctx_map, hls_qf_thresholds, strm_ac_coeff_ordered, strm_strategy,
                             strm_qf, strm_qdc, strm_token_internal, strm_token_out);

        // post-process
        token_reg_out(61, 0) = 0;
        token_reg_out[62] = 1;
        token_reg_out[63] = 0;
        strm_token_out.write(token_reg_out);
    }

    token_reg_out[62] = 0;
    token_reg_out[63] = 1;
    strm_token_out.write(token_reg_out);

    ap_uint<64> token_reg;
    token_reg(62, 0) = 0;
    token_reg[63] = 1;
    strm_token_internal.write(token_reg);
}

//=====================================================================================================//
// hls_init_histogram.cpp
//=====================================================================================================//
void hls_largest_And_empty_write_out(int config[32],
                                     hls::stream<uint32_t>& strm_nempty_cnt,
                                     hls::stream<uint32_t>& strm_largest_idx

                                     ) {
WRITE_LARGEST_IDX_LOOP:
    for (int i = 17; i < 22; i++) {
#pragma HLS PIPELINE II = 1
        uint32_t largest_reg = strm_largest_idx.read();
        config[i] = largest_reg;
    }

WRITE_NEMPTY_CNT_LOOP:
    for (int i = 22; i < 27; i++) {
#pragma HLS PIPELINE II = 1
        uint32_t nempty_cnt = strm_nempty_cnt.read();
        config[i] = nempty_cnt;
    }
}

void load_config(int config[32],
                 hls::stream<int>& strm_config_2,
                 hls::stream<int>& strm_config_3,
                 hls::stream<int>& strm_config_4,
                 hls::stream<int>& strm_config_5,
                 hls::stream<int32_t>& strm_global_config,
                 hls::stream<int>& strm_do_once,
                 hls::stream<int>& strm_do_once_0,
                 hls::stream<int>& strm_do_once_1,
                 hls::stream<int>& strm_do_once_2,
                 hls::stream<int>& strm_do_once_3) {
    strm_config_2.write(config[4]);
    strm_config_2.write(config[5]);
    strm_config_2.write(config[6]);

    strm_config_3.write(config[4]);
    strm_config_3.write(config[5]);
    strm_config_3.write(config[6]);

    strm_config_4.write(config[4]);
    strm_config_4.write(config[5]);
    strm_config_4.write(config[6]);

    strm_config_5.write(config[4]);
    strm_config_5.write(config[5]);
    strm_config_5.write(config[6]);

    strm_global_config.write(config[4]);
    strm_global_config.write(config[5]);
    strm_global_config.write(config[6]);
    strm_global_config.write(config[9]);
    strm_global_config.write(config[7]);
    strm_global_config.write(config[8]);
    strm_global_config.write(config[10]);

    strm_do_once_0.write(config[12]);
    strm_do_once_1.write(config[13]);
    strm_do_once_2.write(config[14]);
    strm_do_once_3.write(config[15]);

    strm_do_once.write(config[12]);
    strm_do_once.write(config[13]);
    strm_do_once.write(config[14]);
    strm_do_once.write(config[15]);
    strm_do_once.write(config[16]);
}

void load_bram(
    // host config
    int config[32],
    uint8_t ctx_map[MAX_QF_THRESH_SIZE],
    uint32_t qf_thresholds[MAX_CTX_MAP_SIZE],
    uint8_t hls_ctx_map[MAX_CTX_MAP_SIZE],
    uint32_t hls_qf_thresholds[MAX_QF_THRESH_SIZE]) {
    // load size config
    int ctx_map_size = config[11];
    int qf_threshold_size = config[9];

// loading into bram
LOAD_CTX_MAP_LOOP:
    for (int i = 0; i < ctx_map_size; i++) {
#pragma HLS PIPELINE II = 1
        hls_ctx_map[i] = ctx_map[i];
    }

LOAD_QF_THRESHOLDS_LOOP:
    for (int i = 0; i < qf_threshold_size; i++) {
#pragma HLS PIPELINE II = 1
        hls_qf_thresholds[i] = qf_thresholds[i];
    }
}

void hls_ANSinitHistogram_core(hls::stream<int32_t>& strm_global_config,
                               hls::stream<int32_t>& strm_config_2,
                               hls::stream<int32_t>& strm_config_3,
                               hls::stream<int32_t>& strm_config_4,
                               hls::stream<int32_t>& strm_config_5,
                               hls::stream<int32_t>& strm_do_once_0,
                               hls::stream<int32_t>& strm_do_once_1,
                               hls::stream<int32_t>& strm_do_once_2,
                               hls::stream<int32_t>& strm_do_once_3,
                               hls::stream<int32_t>& strm_do_once,

                               uint8_t hls_ctx_map[MAX_CTX_MAP_SIZE],
                               uint32_t hls_qf_thresholds[MAX_QF_THRESH_SIZE],
                               // ac_coef_ordered_ddr
                               int32_t ac_coeff_ordered_ddr[ALL_PIXEL],
                               // ac_strategy ddr
                               int32_t strategy_ddr[MAX_NUM_BLK88],
                               // qf ddr
                               int32_t qf_ddr[MAX_NUM_BLK88],
                               // qdc ddr
                               uint8_t qdc_ddr[MAX_NUM_BLK88],
                               // ctx_map ddr
                               uint8_t ctx_map[MAX_QF_THRESH_SIZE], //
                               // quant field threshold
                               uint32_t qf_thresholds[MAX_CTX_MAP_SIZE], //
                               // ac_token_output
                               uint64_t ac_tokens_ddr[MAX_AC_TOKEN_SIZE],

                               ap_uint<64>* tokens0_ptr,
                               ap_uint<64>* tokens1_ptr,
                               ap_uint<64>* tokens2_ptr,
                               ap_uint<64>* tokens3_ptr,
                               hls::stream<uint32_t>& strm_nempty_cnt,
                               hls::stream<uint32_t>& strm_largest_idx,

                               int32_t* histograms0_ptr,
                               uint32_t* histograms_size0_ptr,
                               uint32_t* total_count0_ptr,
                               uint32_t* nonempty0_ptr,

                               int32_t* histograms1_ptr,
                               uint32_t* histograms_size1_ptr,
                               uint32_t* total_count1_ptr,
                               uint32_t* nonempty1_ptr,

                               int32_t* histograms2_ptr,
                               uint32_t* histograms_size2_ptr,
                               uint32_t* total_count2_ptr,
                               uint32_t* nonempty2_ptr,

                               int32_t* histograms3_ptr,
                               uint32_t* histograms_size3_ptr,
                               uint32_t* total_count3_ptr,
                               uint32_t* nonempty3_ptr,

                               int32_t* histograms4_ptr,
                               uint32_t* histograms_size4_ptr,
                               uint32_t* total_count4_ptr,
                               uint32_t* nonempty4_ptr) {
#pragma HLS DATAFLOW

    //================================== core ==========================================//
    hls::stream<int32_t, 40960> strm_ac_coeff0;
    hls::stream<int32_t, 16> strm_ac_coeff1;
    hls::stream<int32_t, 16> strm_strategy;
    hls::stream<int32_t, 16> strm_qf;
    hls::stream<uint8_t, 16> strm_qdc;
    hls::stream<ap_uint<64>, 16> token_stream0;
    hls::stream<ap_uint<64>, 16> token_stream1;
    hls::stream<ap_uint<64>, 16> token_stream2;
    hls::stream<ap_uint<64>, 16> token_stream3;
    load_token(strm_do_once_0, tokens0_ptr, token_stream0);
    load_token(strm_do_once_1, tokens1_ptr, token_stream1);
    load_token(strm_do_once_2, tokens2_ptr, token_stream2);
    load_token(strm_do_once_3, tokens3_ptr, token_stream3);
    load_ac_ordered_by_group(strm_config_2, ac_coeff_ordered_ddr, strm_ac_coeff0);
    load_ac_strategy_by_group(strm_config_3, strategy_ddr, strm_strategy);
    load_qf_by_group(strm_config_4, qf_ddr, strm_qf);
    load_qdc_by_group(strm_config_5, qdc_ddr, strm_qdc);

    hls::stream<ap_uint<64>, 16> token_stream_internal;
    hls::stream<ap_uint<64>, 16> strm_token_out;
    hls_TokenizeCoefficients(hls_ctx_map, hls_qf_thresholds, strm_global_config, strm_ac_coeff0, strm_strategy, strm_qf,
                             strm_qdc, token_stream_internal, strm_token_out);

    init_histogram_top(strm_do_once, token_stream0, token_stream1, token_stream2, token_stream3, token_stream_internal,
                       strm_nempty_cnt, strm_largest_idx, histograms0_ptr, histograms_size0_ptr, total_count0_ptr,
                       nonempty0_ptr,

                       histograms1_ptr, histograms_size1_ptr, total_count1_ptr, nonempty1_ptr,

                       histograms2_ptr, histograms_size2_ptr, total_count2_ptr, nonempty2_ptr,

                       histograms3_ptr, histograms_size3_ptr, total_count3_ptr, nonempty3_ptr,

                       histograms4_ptr, histograms_size4_ptr, total_count4_ptr, nonempty4_ptr);

    ac_token_writeout(ac_tokens_ddr, strm_token_out);
}

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
    // host config
    int config[32],
    // ac_coef_ordered_ddr
    int32_t ac_coeff_ordered_ddr[ALL_PIXEL],
    // ac_strategy ddr
    int32_t strategy_ddr[MAX_NUM_BLK88],
    // qf ddr
    int32_t qf_ddr[MAX_NUM_BLK88],
    // qdc ddr
    uint8_t qdc_ddr[MAX_NUM_BLK88],
    // ctx_map ddr
    uint8_t ctx_map[MAX_QF_THRESH_SIZE],
    // quant field threshold
    uint32_t qf_thresholds[MAX_CTX_MAP_SIZE],
    // ac_token_output
    uint64_t ac_tokens_ddr[MAX_AC_TOKEN_SIZE],

    ap_uint<64>* tokens0_ptr,
    ap_uint<64>* tokens1_ptr,
    ap_uint<64>* tokens2_ptr,
    ap_uint<64>* tokens3_ptr,

    int32_t* histograms0_ptr,
    uint32_t* histograms_size0_ptr,
    uint32_t* total_count0_ptr,
    uint32_t* nonempty0_ptr,

    int32_t* histograms1_ptr,
    uint32_t* histograms_size1_ptr,
    uint32_t* total_count1_ptr,
    uint32_t* nonempty1_ptr,

    int32_t* histograms2_ptr,
    uint32_t* histograms_size2_ptr,
    uint32_t* total_count2_ptr,
    uint32_t* nonempty2_ptr,

    int32_t* histograms3_ptr,
    uint32_t* histograms_size3_ptr,
    uint32_t* total_count3_ptr,
    uint32_t* nonempty3_ptr,

    int32_t* histograms4_ptr,
    uint32_t* histograms_size4_ptr,
    uint32_t* total_count4_ptr,
    uint32_t* nonempty4_ptr) {
// size of pixel
#pragma HLS INTERFACE mode = m_axi bundle = mm2 latency = 32 offset = slave num_write_outstanding =           \
    1 num_read_outstanding = 8 max_write_burst_length = 2 max_read_burst_length = 64 depth = ALL_PIXEL port = \
        ac_coeff_ordered_ddr
// size of num_blk
#pragma HLS INTERFACE mode = m_axi bundle = mm3 latency = 32 offset = slave num_write_outstanding =               \
    1 num_read_outstanding = 8 max_write_burst_length = 2 max_read_burst_length = 64 depth = MAX_NUM_BLK88 port = \
        strategy_ddr
#pragma HLS INTERFACE mode = m_axi bundle = mm4 latency = 32 offset = slave num_write_outstanding =               \
    1 num_read_outstanding = 8 max_write_burst_length = 2 max_read_burst_length = 64 depth = MAX_NUM_BLK88 port = \
        qf_ddr
#pragma HLS INTERFACE mode = m_axi bundle = mm5 latency = 32 offset = slave num_write_outstanding =               \
    1 num_read_outstanding = 8 max_write_burst_length = 2 max_read_burst_length = 64 depth = MAX_NUM_BLK88 port = \
        qdc_ddr
// size of bram
#pragma HLS INTERFACE mode = m_axi bundle = mm6 latency = 32 offset = slave num_write_outstanding =                  \
    1 num_read_outstanding = 8 max_write_burst_length = 2 max_read_burst_length = 16 depth = MAX_CTX_MAP_SIZE port = \
        ctx_map
#pragma HLS INTERFACE mode = m_axi bundle = mm6 latency = 32 offset = slave num_write_outstanding =                    \
    1 num_read_outstanding = 8 max_write_burst_length = 2 max_read_burst_length = 16 depth = MAX_QF_THRESH_SIZE port = \
        qf_thresholds
// config
#pragma HLS INTERFACE mode = m_axi bundle = mm7 latency = 32 offset = slave num_write_outstanding = \
    1 num_read_outstanding = 8 max_write_burst_length = 2 max_read_burst_length = 16 depth = 32 port = config
// output
#pragma HLS INTERFACE mode = m_axi bundle = mm8 latency = 32 offset = slave num_write_outstanding =                   \
    8 num_read_outstanding = 1 max_write_burst_length = 64 max_read_burst_length = 2 depth = MAX_AC_TOKEN_SIZE port = \
        ac_tokens_ddr

#pragma HLS INTERFACE mode = m_axi bundle = mm9 latency = 32 offset = slave num_write_outstanding =                    \
    8 num_read_outstanding = 8 max_write_burst_length = 64 max_read_burst_length = 64 depth = MAX_AC_TOKEN_SIZE port = \
        tokens0_ptr
#pragma HLS INTERFACE mode = m_axi bundle = mm10 latency = 32 offset = slave num_write_outstanding =                   \
    8 num_read_outstanding = 8 max_write_burst_length = 64 max_read_burst_length = 64 depth = MAX_AC_TOKEN_SIZE port = \
        tokens1_ptr
#pragma HLS INTERFACE mode = m_axi bundle = mm11 latency = 32 offset = slave num_write_outstanding =                   \
    8 num_read_outstanding = 8 max_write_burst_length = 64 max_read_burst_length = 64 depth = MAX_AC_TOKEN_SIZE port = \
        tokens2_ptr
#pragma HLS INTERFACE mode = m_axi bundle = mm12 latency = 32 offset = slave num_write_outstanding =                   \
    8 num_read_outstanding = 8 max_write_burst_length = 64 max_read_burst_length = 64 depth = MAX_AC_TOKEN_SIZE port = \
        tokens3_ptr

#pragma HLS INTERFACE mode = m_axi bundle = mm9 latency = 32 offset = slave num_write_outstanding =       \
    8 num_read_outstanding = 8 max_write_burst_length = 64 max_read_burst_length = 64 depth = 4096 port = \
        nonempty0_ptr
#pragma HLS INTERFACE mode = m_axi bundle = mm9 latency = 32 offset = slave num_write_outstanding =       \
    8 num_read_outstanding = 8 max_write_burst_length = 64 max_read_burst_length = 64 depth = 4096 port = \
        nonempty1_ptr
#pragma HLS INTERFACE mode = m_axi bundle = mm9 latency = 32 offset = slave num_write_outstanding =       \
    8 num_read_outstanding = 8 max_write_burst_length = 64 max_read_burst_length = 64 depth = 4096 port = \
        nonempty2_ptr
#pragma HLS INTERFACE mode = m_axi bundle = mm9 latency = 32 offset = slave num_write_outstanding =       \
    8 num_read_outstanding = 8 max_write_burst_length = 64 max_read_burst_length = 64 depth = 4096 port = \
        nonempty3_ptr
#pragma HLS INTERFACE mode = m_axi bundle = mm9 latency = 32 offset = slave num_write_outstanding =       \
    8 num_read_outstanding = 8 max_write_burst_length = 64 max_read_burst_length = 64 depth = 4096 port = \
        nonempty4_ptr

#pragma HLS INTERFACE mode = m_axi bundle = mm10 latency = 32 offset = slave num_write_outstanding =        \
    8 num_read_outstanding = 8 max_write_burst_length = 64 max_read_burst_length = 64 depth = 163840 port = \
        histograms0_ptr
#pragma HLS INTERFACE mode = m_axi bundle = mm10 latency = 32 offset = slave num_write_outstanding =        \
    8 num_read_outstanding = 8 max_write_burst_length = 64 max_read_burst_length = 64 depth = 163840 port = \
        histograms1_ptr
#pragma HLS INTERFACE mode = m_axi bundle = mm10 latency = 32 offset = slave num_write_outstanding =        \
    8 num_read_outstanding = 8 max_write_burst_length = 64 max_read_burst_length = 64 depth = 163840 port = \
        histograms2_ptr
#pragma HLS INTERFACE mode = m_axi bundle = mm10 latency = 32 offset = slave num_write_outstanding =        \
    8 num_read_outstanding = 8 max_write_burst_length = 64 max_read_burst_length = 64 depth = 163840 port = \
        histograms3_ptr
#pragma HLS INTERFACE mode = m_axi bundle = mm10 latency = 32 offset = slave num_write_outstanding =        \
    8 num_read_outstanding = 8 max_write_burst_length = 64 max_read_burst_length = 64 depth = 163840 port = \
        histograms4_ptr

#pragma HLS INTERFACE mode = m_axi bundle = mm11 latency = 32 offset = slave num_write_outstanding =      \
    8 num_read_outstanding = 8 max_write_burst_length = 64 max_read_burst_length = 64 depth = 4096 port = \
        histograms_size0_ptr
#pragma HLS INTERFACE mode = m_axi bundle = mm11 latency = 32 offset = slave num_write_outstanding =      \
    8 num_read_outstanding = 8 max_write_burst_length = 64 max_read_burst_length = 64 depth = 4096 port = \
        histograms_size1_ptr
#pragma HLS INTERFACE mode = m_axi bundle = mm11 latency = 32 offset = slave num_write_outstanding =      \
    8 num_read_outstanding = 8 max_write_burst_length = 64 max_read_burst_length = 64 depth = 4096 port = \
        histograms_size2_ptr
#pragma HLS INTERFACE mode = m_axi bundle = mm11 latency = 32 offset = slave num_write_outstanding =      \
    8 num_read_outstanding = 8 max_write_burst_length = 64 max_read_burst_length = 64 depth = 4096 port = \
        histograms_size3_ptr
#pragma HLS INTERFACE mode = m_axi bundle = mm11 latency = 32 offset = slave num_write_outstanding =      \
    8 num_read_outstanding = 8 max_write_burst_length = 64 max_read_burst_length = 64 depth = 4096 port = \
        histograms_size4_ptr

#pragma HLS INTERFACE mode = m_axi bundle = mm12 latency = 32 offset = slave num_write_outstanding =      \
    8 num_read_outstanding = 8 max_write_burst_length = 64 max_read_burst_length = 64 depth = 4096 port = \
        total_count0_ptr
#pragma HLS INTERFACE mode = m_axi bundle = mm12 latency = 32 offset = slave num_write_outstanding =      \
    8 num_read_outstanding = 8 max_write_burst_length = 64 max_read_burst_length = 64 depth = 4096 port = \
        total_count1_ptr
#pragma HLS INTERFACE mode = m_axi bundle = mm12 latency = 32 offset = slave num_write_outstanding =      \
    8 num_read_outstanding = 8 max_write_burst_length = 64 max_read_burst_length = 64 depth = 4096 port = \
        total_count2_ptr
#pragma HLS INTERFACE mode = m_axi bundle = mm12 latency = 32 offset = slave num_write_outstanding =      \
    8 num_read_outstanding = 8 max_write_burst_length = 64 max_read_burst_length = 64 depth = 4096 port = \
        total_count3_ptr
#pragma HLS INTERFACE mode = m_axi bundle = mm12 latency = 32 offset = slave num_write_outstanding =      \
    8 num_read_outstanding = 8 max_write_burst_length = 64 max_read_burst_length = 64 depth = 4096 port = \
        total_count4_ptr

    //========================== top ==================================================//
    hls::stream<int32_t, 8> strm_global_config;
    hls::stream<int32_t, 8> strm_config_2;
    hls::stream<int32_t, 8> strm_config_3;
    hls::stream<int32_t, 8> strm_config_4;
    hls::stream<int32_t, 8> strm_config_5;
    hls::stream<int32_t, 2> strm_do_once_0("strm_do_once_0");
    hls::stream<int32_t, 2> strm_do_once_1("strm_do_once_1");
    hls::stream<int32_t, 2> strm_do_once_2("strm_do_once_2");
    hls::stream<int32_t, 2> strm_do_once_3("strm_do_once_3");
    hls::stream<int32_t, 8> strm_do_once("strm_do_once");
    load_config(config, strm_config_2, strm_config_3, strm_config_4, strm_config_5, strm_global_config, strm_do_once,
                strm_do_once_0, strm_do_once_1, strm_do_once_2, strm_do_once_3);

    uint8_t hls_ctx_map[MAX_CTX_MAP_SIZE];
#pragma HLS BIND_STORAGE impl = BRAM variable = hls_ctx_map type = ram_s2p
    uint32_t hls_qf_thresholds[MAX_QF_THRESH_SIZE];
#pragma HLS BIND_STORAGE impl = BRAM variable = hls_qf_thresholds type = ram_s2p
    load_bram(config, ctx_map, qf_thresholds, hls_ctx_map, hls_qf_thresholds);

    //=============================== core =====================================//
    hls::stream<uint32_t, 8> strm_nempty_cnt;
    hls::stream<uint32_t, 8> strm_largest_idx;
    hls_ANSinitHistogram_core(
        strm_global_config, strm_config_2, strm_config_3, strm_config_4, strm_config_5, strm_do_once_0, strm_do_once_1,
        strm_do_once_2, strm_do_once_3, strm_do_once, hls_ctx_map, hls_qf_thresholds,

        ac_coeff_ordered_ddr, strategy_ddr, qf_ddr, qdc_ddr, ctx_map, qf_thresholds, ac_tokens_ddr,

        tokens0_ptr, tokens1_ptr, tokens2_ptr, tokens3_ptr, strm_nempty_cnt, strm_largest_idx,

        histograms0_ptr, histograms_size0_ptr, total_count0_ptr, nonempty0_ptr,

        histograms1_ptr, histograms_size1_ptr, total_count1_ptr, nonempty1_ptr,

        histograms2_ptr, histograms_size2_ptr, total_count2_ptr, nonempty2_ptr,

        histograms3_ptr, histograms_size3_ptr, total_count3_ptr, nonempty3_ptr,

        histograms4_ptr, histograms_size4_ptr, total_count4_ptr, nonempty4_ptr);
    //======================= larget_And_empty write out =========================//
    hls_largest_And_empty_write_out(config, strm_nempty_cnt, strm_largest_idx);
    //======================= End of All =========================================//
}

} // namespace codec
} // namespace xf
#endif
