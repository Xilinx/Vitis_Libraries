// Copyright (c) the JPEG XL Project Authors. All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#ifndef HLS_KERNEL2_CPP
#define HLS_KERNEL2_CPP

#include "acc_phase2.hpp"
#include "host_lossy_enc_compute.hpp"

#include <iostream>
#include <queue>
#include <fstream>

namespace jxl {

void collect_dc(PassesEncoderState* enc_state,
                Image3F* dc,
                size_t xsize,
                size_t ysize,
                float* hls_dc8x8,
                float* hls_dc16x16,
                float* hls_dc32x32) {
    for (int i = 0; i < enc_state->shared.frame_dim.num_groups; i++) {
        const Rect block_group_rect = enc_state->shared.BlockGroupRect(i);
        const size_t xsize_blocks = block_group_rect.xsize();
        const size_t ysize_blocks = block_group_rect.ysize();

        const size_t dc_stride = static_cast<size_t>(dc->PixelsPerRow());

        {
            size_t offset = 0;

            for (size_t by = 0; by < ysize_blocks; ++by) {
                size_t ty = by / kColorTileDimInBlocks;
                float* JXL_RESTRICT dc_rows[3] = {
                    block_group_rect.PlaneRow(dc, 0, by), block_group_rect.PlaneRow(dc, 1, by),
                    block_group_rect.PlaneRow(dc, 2, by),
                };
                AcStrategyRow ac_strategy_row = enc_state->shared.ac_strategy.ConstRow(block_group_rect, by);
                for (size_t tx = 0; tx < DivCeil(xsize_blocks, kColorTileDimInBlocks); tx++) {
                    for (size_t bx = tx * kColorTileDimInBlocks;
                         bx < xsize_blocks && bx < (tx + 1) * kColorTileDimInBlocks; ++bx) {
                        const AcStrategy acs = ac_strategy_row[bx];
                        if (!acs.IsFirstBlock()) continue;

                        size_t xblocks = acs.covered_blocks_x();
                        size_t yblocks = acs.covered_blocks_y();

                        size_t size = kDCTBlockSize * xblocks * yblocks;

                        size_t tile_xsize = (xsize + 63) / 64 * 64;
                        size_t tile_ysize = (ysize + 63) / 64 * 64;

                        size_t block_cnt8x8 =
                            (block_group_rect.y0() + by) * (tile_xsize / 8) + block_group_rect.x0() + bx;
                        size_t block_cnt16x16 =
                            (block_group_rect.y0() + by) / 2 * (tile_xsize / 16) + (block_group_rect.x0() + bx) / 2;
                        size_t block_cnt32x32 =
                            (block_group_rect.y0() + by) / 4 * (tile_xsize / 32) + (block_group_rect.x0() + bx) / 4;

                        for (size_t c : {0, 1, 2}) {
                            float* coef_dc = dc_rows[c] + bx;
                            if (acs.RawStrategy() == 0) {
                                coef_dc[0] = hls_dc8x8[c * tile_xsize * tile_ysize + block_cnt8x8];
                            } else if (acs.RawStrategy() == 4) {
                                for (int i = 0; i < 2; i++) {
                                    for (int j = 0; j < 2; j++) {
                                        coef_dc[i * dc_stride + j] =
                                            hls_dc16x16[c * tile_xsize * tile_ysize + 4 * block_cnt16x16 + i * 2 + j];
                                    }
                                }
                            } else if (acs.RawStrategy() == 5) {
                                for (int i = 0; i < 4; i++) {
                                    for (int j = 0; j < 4; j++) {
                                        coef_dc[i * dc_stride + j] =
                                            hls_dc32x32[c * tile_ysize * tile_xsize + 16 * block_cnt32x32 + i * 4 + j];
                                    }
                                }
                            } else {
                                std::cout << "unsupported DCFromLowFREQ" << std::endl;
                            }
                        }
                        offset += size;
                    }
                }
            }
        }
    }
}

Status acc_phase2(std::string xclbinPath,
                  Image3F& opsin,
                  LossyFrameEncoder& lossy_frame_encoder,
                  std::unique_ptr<ModularFrameEncoder>& modular_frame_encoder,
                  CompressParams cparams,
                  std::unique_ptr<FrameHeader>& frame_header,
                  const std::vector<ImageF>* extra_channels,
                  const ImageBundle* JXL_RESTRICT ib_or_linear,
                  const ImageBundle& ib,
                  ThreadPool* pool,
                  AuxOut* aux_out) {
    if (frame_header->encoding == FrameEncoding::kVarDCT) {
        //===================================================================================================//
        // kernel-2 CPU part, pre-processing
        //===================================================================================================//

        // pointer define
        PassesEncoderState* JXL_RESTRICT enc_state_ = lossy_frame_encoder.State();
        PassesSharedState& shared = enc_state_->shared;

        // define sizes
        uint32_t tile_xsize = (opsin.xsize() + 63) / 64 * 64;
        uint32_t tile_ysize = (opsin.ysize() + 63) / 64 * 64;
        uint32_t ysize64 = tile_ysize / 64;
        uint32_t xsize64 = tile_xsize / 64;
        int xsize_blocks = enc_state_->shared.frame_dim.xsize_blocks;
        int ysize_blocks = enc_state_->shared.frame_dim.ysize_blocks;
        int xnum_tile = (opsin.xsize() + 63) / 64;
        int ynum_tile = (opsin.ysize() + 63) / 64;
        unsigned xsize_8alg = (opsin.xsize() + 7) / 8 * 8;
        unsigned ysize_8alg = (opsin.ysize() + 7) / 8 * 8;
        int num_tile = xnum_tile * ynum_tile;

        Image3F* opsin_ = &opsin;
        Quantizer& quantizer = enc_state_->shared.quantizer;
        enc_state_->shared.matrices = DequantMatrices();
        enc_state_->histogram_idx.resize(shared.frame_dim.num_groups);
        enc_state_->x_qm_multiplier = std::pow(1.25f, shared.frame_header.x_qm_scale - 2.0f);
        enc_state_->b_qm_multiplier = std::pow(1.25f, shared.frame_header.b_qm_scale - 2.0f);

        if (enc_state_->coeffs.size() < shared.frame_header.passes.num_passes) {
            enc_state_->coeffs.reserve(shared.frame_header.passes.num_passes);
            for (size_t i = enc_state_->coeffs.size(); i < shared.frame_header.passes.num_passes; i++) {
                // Allocate enough coefficients for each group on every row.
                enc_state_->coeffs.emplace_back(
                    make_unique<ACImageT<int32_t> >(kGroupDim * kGroupDim, shared.frame_dim.num_groups));
            }
        }

        while (enc_state_->coeffs.size() > shared.frame_header.passes.num_passes) {
            enc_state_->coeffs.pop_back();
        }

        Image3F dc(shared.frame_dim.xsize_blocks, shared.frame_dim.ysize_blocks);

        AcStrategyHeuristics acs_heuristics;
        CfLHeuristics cfl_heuristics;
        cfl_heuristics.Init(*opsin_);
        cfl_heuristics.PrepareForThreads(1);
        acs_heuristics.Init(*opsin_, enc_state_);

        //========================================================================//
        // host interface
        //========================================================================//
        int config[MAX_NUM_CONFIG];
        float config_fl[MAX_NUM_CONFIG];
        float* hls_opsin_1 = (float*)malloc(ALL_PIXEL * sizeof(float));
        float* hls_opsin_2 = (float*)malloc(ALL_PIXEL * sizeof(float));
        float* hls_opsin_3 = (float*)malloc(ALL_PIXEL * sizeof(float));
        float* hls_quant_field = (float*)malloc(BLOCK8_H * BLOCK8_W * sizeof(float));
        float* hls_masking_field = (float*)malloc(BLOCK8_H * BLOCK8_W * sizeof(float));
        float* aq_map_f = (float*)malloc(BLOCK8_H * BLOCK8_W * sizeof(float));
        int8_t* cmap_axi = (int8_t*)malloc(TILE_W * TILE_H * 2 * sizeof(int8_t));
        int* ac_coef_axiout = (int*)malloc(ALL_PIXEL * sizeof(int));
        uint8_t* strategy_all = (uint8_t*)malloc(sizeof(uint8_t*) * BLOCK8_H * BLOCK8_W);
        int* raw_quant_field_i = (int*)malloc(BLOCK8_H * BLOCK8_W * sizeof(int));
        uint32_t hls_order[MAX_ORDER];
        float* hls_dc8x8 = (float*)malloc(ALL_PIXEL * sizeof(float));
        float* hls_dc16x16 = (float*)malloc(ALL_PIXEL * sizeof(float));
        float* hls_dc32x32 = (float*)malloc(ALL_PIXEL * sizeof(float));

        float* Image_reorder_dct8 = (float*)malloc(ALL_PIXEL * sizeof(float));
        float* Image_reorder_dct16 = (float*)malloc(ALL_PIXEL * sizeof(float));
        float* Image_reorder_dct32 = (float*)malloc(ALL_PIXEL * sizeof(float));

        config[0] = opsin.ysize();
        config[1] = opsin.xsize();
        config[2] = acs_heuristics.config.masking_field_stride;
        config[3] = acs_heuristics.config.quant_field_stride;
        config_fl[0] = acs_heuristics.enc_state->cparams.butteraugli_distance;
        config_fl[1] = acs_heuristics.config.cost1;
        config_fl[2] = quantizer.InvGlobalScale();

        for (int c = 0; c < 3; c++) {
            for (int y = 0; y < tile_ysize; y++) {
                const float* JXL_RESTRICT row = opsin.ConstPlaneRow(c, y);
                memcpy(&hls_opsin_1[c * tile_xsize * tile_ysize + y * tile_xsize], row, tile_xsize * sizeof(float));
            }
        }

        for (int c = 0; c < 3; c++) {
            for (int y = 0; y < tile_ysize; y++) {
                const float* JXL_RESTRICT row = opsin.ConstPlaneRow(c, y);
                memcpy(&hls_opsin_2[c * tile_xsize * tile_ysize + y * tile_xsize], row, tile_xsize * sizeof(float));
            }
        }

        for (int c = 0; c < 3; c++) {
            for (int y = 0; y < tile_ysize; y++) {
                const float* JXL_RESTRICT row = opsin.ConstPlaneRow(c, y);
                memcpy(&hls_opsin_3[c * tile_xsize * tile_ysize + y * tile_xsize], row, tile_xsize * sizeof(float));
            }
        }

        for (uint32_t y64 = 0; y64 < ysize64; y64++) {
            for (uint32_t x64 = 0; x64 < xsize64; x64++) {
                for (uint32_t y8 = 0; y8 < 8; y8++) {
                    for (uint32_t x8 = 0; x8 < 8; x8++) {
                        for (int c = 0; c < 3; c++) {
                            for (int m = 0; m < 8; m++) {
                                for (int n = 0; n < 8; n++) {
                                    uint32_t c_tmp = 0;
                                    if (c == 0) {
                                        c_tmp = 1;
                                    } else if (c == 1) {
                                        c_tmp = 0;
                                    } else {
                                        c_tmp = 2;
                                    }
                                    uint32_t addr = c_tmp * tile_xsize * tile_ysize + y64 * tile_xsize * 64 + x64 * 64 +
                                                    y8 * tile_xsize * 8 + x8 * 8 + m * tile_xsize + n;

                                    float reg = hls_opsin_1[addr];
                                    Image_reorder_dct8[n + 8 * m + 64 * c + 64 * 3 * x8 + 512 * 3 * y8 +
                                                       4096 * 3 * x64 + 4096 * 3 * xsize64 * y64] = reg;
                                }
                            }
                        }
                    }
                }
            }
        }

        for (uint32_t y64 = 0; y64 < ysize64; y64++) {
            for (uint32_t x64 = 0; x64 < xsize64; x64++) {
                for (uint32_t y16 = 0; y16 < 4; y16++) {
                    for (uint32_t x16 = 0; x16 < 4; x16++) {
                        for (uint32_t c = 0; c < 3; c++) {
                            for (uint32_t m = 0; m < 16; m++) {
                                for (uint32_t n = 0; n < 16; n++) {
                                    uint32_t c_tmp = 0;
                                    if (c == 0) {
                                        c_tmp = 1;
                                    } else if (c == 1) {
                                        c_tmp = 0;
                                    } else {
                                        c_tmp = 2;
                                    }

                                    uint32_t addr = c_tmp * tile_xsize * tile_ysize + y64 * tile_xsize * 64 + x64 * 64 +
                                                    y16 * tile_xsize * 16 + x16 * 16 + m * tile_xsize + n;
                                    float reg = hls_opsin_2[addr];
                                    Image_reorder_dct16[4096 * 3 * xsize64 * y64 + 4096 * 3 * x64 + 1024 * 3 * y16 +
                                                        256 * 3 * x16 + 256 * c + 16 * m + n] = reg;
                                }
                            }
                        }
                    }
                }
            }
        }

        for (uint32_t y64 = 0; y64 < ysize64; y64++) {
            for (uint32_t x64 = 0; x64 < xsize64; x64++) {
                for (uint32_t y32 = 0; y32 < 2; y32++) {
                    for (uint32_t x32 = 0; x32 < 2; x32++) {
                        for (uint32_t c = 0; c < 3; c++) {
                            for (uint32_t m = 0; m < 32; m++) {
                                for (uint32_t n = 0; n < 32; n++) {
                                    uint32_t c_tmp = 0;
                                    if (c == 0) {
                                        c_tmp = 1;
                                    } else if (c == 1) {
                                        c_tmp = 0;
                                    } else {
                                        c_tmp = 2;
                                    }

                                    uint32_t addr = c_tmp * tile_xsize * tile_ysize + y64 * tile_xsize * 64 + x64 * 64 +
                                                    y32 * tile_xsize * 32 + x32 * 32 + m * tile_xsize + n;
                                    float reg = hls_opsin_3[addr];
                                    Image_reorder_dct32[4096 * 3 * xsize64 * y64 + 4096 * 3 * x64 + 2048 * 3 * y32 +
                                                        1024 * 3 * x32 + 1024 * c + 32 * m + n] = reg;
                                }
                            }
                        }
                    }
                }
            }
        }

        // input: rqf
        for (int y = 0; y < ysize_blocks; y++) {
            float* aq_row = enc_state_->initial_quant_field.Row(y);
            for (int x = 0; x < xsize_blocks; x++) {
                aq_map_f[y * xsize_blocks + x] = aq_row[x];
            }
        }

        // input: masking field
        for (int i = 0; i < BLOCK8_H * BLOCK8_W; i++) {
            hls_masking_field[i] = acs_heuristics.config.masking_field_row[i];
        }

        // input: quant_field
        for (int i = 0; i < BLOCK8_H * BLOCK8_W; i++) {
            hls_quant_field[i] = acs_heuristics.config.quant_field_row[i];
        }

        //================================================================//
        // kernel-2 FPGA kernel part, pass HLS test
        // hls_kernel2_top.cpp
        //===============================================================//
        hls_lossy_enc_compute_wrapper(xclbinPath,
                                      // input
                                      config, config_fl, Image_reorder_dct8, Image_reorder_dct16, Image_reorder_dct32,
                                      hls_quant_field, hls_masking_field, aq_map_f,
                                      // output
                                      cmap_axi, ac_coef_axiout, strategy_all, raw_quant_field_i, hls_order, hls_dc8x8,
                                      hls_dc16x16, hls_dc32x32);

        //==============================================================//
        // kernel-2 CPU part, post-processing
        //==============================================================//
        // ac_coef host post-process
        int* ac_coef = (int*)malloc(ALL_PIXEL * sizeof(int));
        {
            bool visit[8][8];
            int i = 0, addr = 0;
            for (int ty = 0; ty < ynum_tile; ty++) {
                for (int tx = 0; tx < xnum_tile; tx++) {
                    for (int by = 0; by < 8; by++) {
                        for (int bx = 0; bx < 8; bx++) {
                            visit[by][bx] = false;
                        }
                    }
                    for (int by = 0; by < 8; by++) {
                        for (int bx = 0; bx < 8; bx++) {
                            if (!visit[by][bx] && (ty * 8 + by) < ysize_8alg / 8 && (tx * 8 + bx) < xsize_8alg / 8) {
                                int idx_acs = (ty * 8 + by) * xsize_8alg / 8 + tx * 8 + bx;
                                char strategy = strategy_all[idx_acs];
                                int b = 0;
                                if (strategy == 4) {
                                    b = 2;
                                } else if (strategy == 5) {
                                    b = 4;
                                } else {
                                    b = 1;
                                }
                                for (int iy = 0; iy < b; iy++) {
                                    for (int ix = 0; ix < b; ix++) {
                                        visit[by + iy][bx + ix] = true;
                                        for (int j = 0; j < 64; j++) {
                                            for (unsigned c = 0; c < 3; ++c) {
                                                if (c == 0 && j == 0) {
                                                    addr = ((ty * 8 + by + iy) * 64 * 3 * xsize_8alg / 8 +
                                                            (tx * 8 + bx + ix) * 64 * 3);
                                                }
                                                ac_coef[addr + j * 3 + c] = ac_coef_axiout[i];
                                                i++;
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        // acs host post-processing
        AcStrategyImage* acs_strategy = &acs_heuristics.enc_state->shared.ac_strategy;
        for (size_t y = 0; y < ysize_blocks; ++y) {
            for (size_t x = 0; x < xsize_blocks; ++x) {
                int index = y * xsize_blocks + x;
                int value = strategy_all[index];
                if (value == 4 && y % 2 == 0 && x % 2 == 0) {
                    acs_strategy->Set(x, y, static_cast<AcStrategy::Type>(value));
                } else if (value == 5 && y % 4 == 0 && x % 4 == 0) {
                    acs_strategy->Set(x, y, static_cast<AcStrategy::Type>(value));
                } else if (value < 4) {
                    acs_strategy->Set(x, y, static_cast<AcStrategy::Type>(value));
                }
            }
        }

        // rqf host post-processing
        ImageI* raw_quant_field = &enc_state_->shared.raw_quant_field;
        for (int y = 0; y < ysize_blocks; y++) {
            float* aq_row = enc_state_->initial_quant_field.Row(y); // quant_field.Row(y);
            int* row_qi = raw_quant_field->Row(y);
            for (int x = 0; x < xsize_blocks; x++) {
                row_qi[x] = raw_quant_field_i[y * xsize_blocks + x];
                aq_row[x] = aq_map_f[y * xsize_blocks + x];
            }
        }

        // epf init
        ImageB* epf_sharpness = &enc_state_->shared.epf_sharpness;
        for (int y = 0; y < enc_state_->shared.frame_dim.ysize_blocks; y++) {
            uint8_t* row = epf_sharpness->Row(y);
            for (int x = 0; x < enc_state_->shared.frame_dim.xsize_blocks; x++) {
                row[x] = 4;
            }
        }

        // dc coeff post-processing
        collect_dc(enc_state_, &dc, opsin.xsize(), opsin.ysize(), hls_dc8x8, hls_dc16x16, hls_dc32x32);

        // cmap host post-processing
        const FrameDimensions frame_dim = enc_state_->shared.frame_dim;
        ImageSB* map_x = &(enc_state_->shared.cmap).ytox_map;
        ImageSB* map_b = &(enc_state_->shared.cmap).ytob_map;

        for (int tid = 0; tid < DivCeil(frame_dim.xsize_blocks, kEncTileDimInBlocks) *
                                    DivCeil(frame_dim.ysize_blocks, kEncTileDimInBlocks);
             tid++) {
            size_t n_enc_tiles = DivCeil(frame_dim.xsize_blocks, kEncTileDimInBlocks);
            size_t tx = tid % n_enc_tiles;
            size_t ty = tid / n_enc_tiles;
            size_t by0 = ty * kEncTileDimInBlocks;
            size_t by1 = std::min((ty + 1) * kEncTileDimInBlocks, frame_dim.ysize_blocks);
            size_t bx0 = tx * kEncTileDimInBlocks;
            size_t bx1 = std::min((tx + 1) * kEncTileDimInBlocks, frame_dim.xsize_blocks);
            Rect r(bx0, by0, bx1 - bx0, by1 - by0);
            static_assert(kEncTileDimInBlocks == kColorTileDimInBlocks, "Invalid color tile dim");

            size_t num_ac = 0;

            int8_t* JXL_RESTRICT row_out_x = map_x->Row(ty);
            int8_t* JXL_RESTRICT row_out_b = map_b->Row(ty);

            row_out_x[tx] = cmap_axi[tid];
            row_out_b[tx] = cmap_axi[num_tile + tid];
        }

        // ac_coeff host post-processing
        for (size_t group_index = 0; group_index < frame_dim.num_groups; group_index++) {
            const size_t gx = group_index % frame_dim.xsize_groups;
            const size_t gy = group_index / frame_dim.xsize_groups;
            const Rect rect(gx * kGroupDimInBlocks, gy * kGroupDimInBlocks, kGroupDimInBlocks, kGroupDimInBlocks,
                            frame_dim.xsize_blocks, frame_dim.ysize_blocks);
            ACPtr rows[3];
            // ACType type = (*enc_state_->coeffs[0]).Type();
            for (size_t c = 0; c < 3; c++) {
                rows[c] = (*enc_state_->coeffs[0]).PlaneRow(c, group_index, 0);
            }
            size_t ac_offset = 0;
            for (size_t by = 0; by < rect.ysize(); ++by) {
                AcStrategyRow acs_row = enc_state_->shared.ac_strategy.ConstRow(rect, by);
                for (size_t bx = 0; bx < rect.xsize(); ++bx) {
                    AcStrategy acs = acs_row[bx];
                    if (!acs.IsFirstBlock()) continue;
                    size_t size = kDCTBlockSize << acs.log2_covered_blocks();
                    size_t cxsize = acs.covered_blocks_x();
                    size_t cysize = acs.covered_blocks_y();

                    int addr = 0;
                    for (int cy = 0; cy < cysize; cy++) {
                        for (int cx = 0; cx < cxsize; cx++) {
                            for (int i = 0; i < 64; i++) {
                                for (size_t c = 0; c < 3; ++c) {
                                    int reorder[3] = {1, 0, 2};
                                    rows[c].ptr32[ac_offset + addr] =
                                        ac_coef[(gy * 32 + by + cy) * 64 * 3 * xsize_8alg / 8 +
                                                (gx * 32 + bx + cx) * 64 * 3 + i * 3 + reorder[c]];
                                }
                                addr++;
                            }
                        }
                    }
                    ac_offset += size;
                }
            }
        }

        // hls_order host-post processing
        enc_state_->used_orders.resize(enc_state_->progressive_splitter.GetNumPasses());
        coeff_order_t* JXL_RESTRICT order = &enc_state_->shared.coeff_orders[0 * enc_state_->shared.coeff_order_size];

        const int32_t offset8x8 = 0;
        const int32_t offset16x16 = 64;

        uint32_t hls_order_reg = hls_order[320 * 3];
        uint32_t mask_0 = 0x00000001;
        uint32_t mask_2 = 0x00000004;
        uint32_t all_used_orders_set[32];
        for (int i = 0; i < 32; i++) {
            if (i == 0) {
                all_used_orders_set[i] = hls_order_reg & mask_0;
            } else if (i == 2) {
                all_used_orders_set[i] = hls_order_reg & mask_2;
            } else if (i == 1 || i == 3) {
                all_used_orders_set[i] = 0;
            } else {
                all_used_orders_set[i] = 0;
            }
        }

        uint32_t computed = 0;
        for (uint8_t o = 0; o < AcStrategy::kNumValidStrategies; ++o) {
            uint8_t ord = kStrategyOrder[o];
            if (computed & (1 << ord)) continue;
            computed |= 1 << ord;
            AcStrategy acs = AcStrategy::FromRawStrategy(o);
            size_t sz = kDCTBlockSize * acs.covered_blocks_x() * acs.covered_blocks_y();
            if (all_used_orders_set[ord] == 0) {
                for (size_t c = 0; c < 3; c++) {
                    size_t offset = CoeffOrderOffset(ord, c);
                    JXL_DASSERT(CoeffOrderOffset(ord, c + 1) - offset == sz);
                    SetDefaultOrder(AcStrategy::FromRawStrategy(o), &order[offset]);
                }
            } else {
                for (size_t c = 0; c < 3; c++) {
                    int reorder[3] = {1, 0, 2};
                    for (int i = 0; i < sz; i++) {
                        size_t offset = CoeffOrderOffset(ord, c);
                        coeff_order_t* JXL_RESTRICT cur_order = &order[offset];
                        if (o == 0) {
                            cur_order[i] = hls_order[reorder[c] * 320 + offset8x8 + i];
                        } else if (o == 4) {
                            cur_order[i] = hls_order[reorder[c] * 320 + offset16x16 + i];
                        }
                    }
                }
            }
        }
        enc_state_->used_orders[0] = hls_order_reg;

        // Choose a context model that depends on the amount of quantization for AC.
        if (cparams.speed_tier < SpeedTier::kFalcon) {
            FindBestBlockEntropyModel(*enc_state_);
        }

        // resize ac_tokens vector
        enc_state_->passes.resize(enc_state_->progressive_splitter.GetNumPasses());
        for (PassesEncoderState::PassData& pass : enc_state_->passes) {
            pass.ac_tokens.resize(shared.frame_dim.num_groups);
        }

        shared.num_histograms = 1;
        *frame_header = shared.frame_header;

        // Modular VarDCTDC
        for (int group_index = 0; group_index < shared.frame_dim.num_dc_groups; group_index++) {
            modular_frame_encoder->AddVarDCTDC(dc, group_index, enc_state_->cparams.butteraugli_distance >= 2.0f &&
                                                                    enc_state_->cparams.speed_tier < SpeedTier::kFalcon,
                                               enc_state_);
        };

        // Modular ACMetadata
        for (int group_index = 0; group_index < shared.frame_dim.num_dc_groups; group_index++) {
            modular_frame_encoder->AddACMetadata(group_index, /*jpeg_transcode=*/false, enc_state_);
        };

        // Modular encode
        JXL_RETURN_IF_ERROR(modular_frame_encoder->ComputeEncodingData(
            *frame_header, *ib.metadata(), &opsin, *extra_channels, lossy_frame_encoder.State(), pool, aux_out,
            /* do_color=*/frame_header->encoding == FrameEncoding::kModular));

        // free host mem
        free(hls_opsin_1);
        free(hls_opsin_2);
        free(hls_opsin_3);
        free(Image_reorder_dct8);
        free(Image_reorder_dct16);
        free(Image_reorder_dct32);
        free(aq_map_f);
        free(hls_masking_field);
        free(hls_quant_field);
        free(cmap_axi);
        free(ac_coef_axiout);
        free(strategy_all);
        free(raw_quant_field_i);
        free(hls_dc8x8);
        free(hls_dc16x16);
        free(hls_dc32x32);
        free(ac_coef);
    }
    return true;
}
} // namespace jxl

#endif
