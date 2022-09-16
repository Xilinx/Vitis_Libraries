// Copyright (c) the JPEG XL Project Authors. All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#ifndef ACC_PHASE2_CPP
#define ACC_PHASE2_CPP

#include "acc_phase2.hpp"

namespace jxl {

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
        std::vector<EncCache>& group_caches_ = lossy_frame_encoder.get_group_cashes();
        PassesEncoderState* JXL_RESTRICT enc_state_ = lossy_frame_encoder.State();
        PassesSharedState& shared = enc_state_->shared;
        Image3F* opsin_ = &opsin;
        Quantizer& quantizer = enc_state_->shared.quantizer;

        size_t tile_xsize = (opsin.xsize() + 63) / 64 * 64;
        size_t tile_ysize = (opsin.ysize() + 63) / 64 * 64;
#ifdef XLNX_QC_DEBUG_DCT
/*std::cout << std::endl
          << "======================================== full origin pixel "
             "=============================================="
          << std::endl;
for (int c = 0; c < 3; c++) {
  if (c == 0) {
    std::cout << std::setw(15) << 0 << " ";
    for (int m = 0; m < tile_xsize; m++) {
      std::cout << std::setw(15) << m << " ";
    }
    std::cout << std::endl << std::endl;

    for (int y = 0; y < tile_ysize; y++) {
      std::cout << std::setw(15) << y << " ";
      const float* JXL_RESTRICT row_y = opsin.ConstPlaneRow(c, y);
      for (int x = 0; x < tile_xsize; x++) {
        std::cout << std::setw(15) << row_y[x] << " ";
      }
      std::cout << std::endl;
    }
    std::cout << std::endl;
  }
}*/
#endif

        std::vector<std::vector<float> > dctIDT(3, std::vector<float>(tile_xsize * tile_ysize));
        std::vector<std::vector<float> > dct2x2(3, std::vector<float>(tile_xsize * tile_ysize));
        std::vector<std::vector<float> > dct4x4(3, std::vector<float>(tile_xsize * tile_ysize));
        std::vector<std::vector<float> > dct8x8(3, std::vector<float>(tile_xsize * tile_ysize));
        std::vector<std::vector<float> > dct16x16(3, std::vector<float>(tile_xsize * tile_ysize));
        std::vector<std::vector<float> > dct32x32(3, std::vector<float>(tile_xsize * tile_ysize));

        std::vector<std::vector<float> > dcIDT(3, std::vector<float>((tile_xsize * tile_ysize + 63) / 64 * 64));
        std::vector<std::vector<float> > dc2x2(3, std::vector<float>((tile_xsize * tile_ysize + 63) / 64 * 64));
        std::vector<std::vector<float> > dc4x4(3, std::vector<float>((tile_xsize * tile_ysize + 63) / 64 * 64));
        std::vector<std::vector<float> > dc8x8(3, std::vector<float>((tile_xsize * tile_ysize + 63) / 64 * 64));
        std::vector<std::vector<float> > dc16x16(3, std::vector<float>((tile_xsize * tile_ysize + 63) / 64 * 64));
        std::vector<std::vector<float> > dc32x32(3, std::vector<float>((tile_xsize * tile_ysize + 63) / 64 * 64));

        for (int c = 0; c < 3; c++) {
            for (size_t y = 0; y < tile_ysize; y = y + 8) {
                const float* JXL_RESTRICT row = opsin.ConstPlaneRow(c, y);
                size_t stride = opsin.PixelsPerRow();

                for (size_t x = 0; x < tile_xsize; x = x + 8) {
                    float* mem = (float*)calloc(8UL * 8UL, sizeof(float));
                    float* dc_mem =
                        (float*)calloc(AcStrategy::kMaxCoeffBlocks * AcStrategy::kMaxCoeffBlocks, sizeof(float));
                    float* scratch_space = (float*)calloc(2048UL, sizeof(float));
                    AcStrategy acs = AcStrategy::FromRawStrategy(AcStrategy::Type::IDENTITY);
                    size_t xs = acs.covered_blocks_x();
                    N_SCALAR::TransformFromPixels(acs.Strategy(), row + x, stride, mem, scratch_space);
                    N_SCALAR::DCFromLowestFrequencies(acs.Strategy(), mem, dc_mem, xs);
                    for (int m = 0; m < 64; m++) {
                        dctIDT[c][64 * (y / 8 * (tile_xsize / 8) + x / 8) + m] = mem[m];
                    }
                    dcIDT[c][y / 8 * tile_xsize / 8 + x / 8] = dc_mem[0];
                    free(mem);
                    free(dc_mem);
                    free(scratch_space);
                }
            }
        }

        for (int c = 0; c < 3; c++) {
            for (size_t y = 0; y < tile_ysize; y = y + 8) {
                const float* JXL_RESTRICT row = opsin.ConstPlaneRow(c, y);
                size_t stride = opsin.PixelsPerRow();

                for (size_t x = 0; x < tile_xsize; x = x + 8) {
                    float* mem = (float*)calloc(8UL * 8UL, sizeof(float));
                    float* dc_mem =
                        (float*)calloc(AcStrategy::kMaxCoeffBlocks * AcStrategy::kMaxCoeffBlocks, sizeof(float));
                    float* scratch_space = (float*)calloc(2048UL, sizeof(float));
                    AcStrategy acs = AcStrategy::FromRawStrategy(AcStrategy::Type::DCT2X2);
                    size_t xs = acs.covered_blocks_x();
                    N_SCALAR::TransformFromPixels(acs.Strategy(), row + x, stride, mem, scratch_space);
                    N_SCALAR::DCFromLowestFrequencies(acs.Strategy(), mem, dc_mem, xs);
                    for (int m = 0; m < 64; m++) {
                        dct2x2[c][64 * (y / 8 * (tile_xsize / 8) + x / 8) + m] = mem[m];
                    }
                    dc2x2[c][y / 8 * tile_xsize / 8 + x / 8] = dc_mem[0];
                    free(mem);
                    free(dc_mem);
                    free(scratch_space);
                }
            }
        }

        for (int c = 0; c < 3; c++) {
            for (size_t y = 0; y < tile_ysize; y = y + 8) {
                const float* JXL_RESTRICT row = opsin.ConstPlaneRow(c, y);
                size_t stride = opsin.PixelsPerRow();

                for (size_t x = 0; x < tile_xsize; x = x + 8) {
                    float* mem = (float*)calloc(8UL * 8UL, sizeof(float));
                    float* dc_mem =
                        (float*)calloc(AcStrategy::kMaxCoeffBlocks * AcStrategy::kMaxCoeffBlocks, sizeof(float));
                    float* scratch_space = (float*)calloc(2048UL, sizeof(float));
                    AcStrategy acs = AcStrategy::FromRawStrategy(AcStrategy::Type::DCT4X4);
                    size_t xs = acs.covered_blocks_x();
                    N_SCALAR::TransformFromPixels(acs.Strategy(), row + x, stride, mem, scratch_space);
                    N_SCALAR::DCFromLowestFrequencies(acs.Strategy(), mem, dc_mem, xs);
                    for (int m = 0; m < 64; m++) {
                        dct4x4[c][64 * (y / 8 * (tile_xsize / 8) + x / 8) + m] = mem[m];
                    }
                    dc4x4[c][y / 8 * tile_xsize / 8 + x / 8] = dc_mem[0];
                    free(mem);
                    free(dc_mem);
                    free(scratch_space);
                }
            }
        }

        for (int c = 0; c < 3; c++) {
            for (size_t y = 0; y < tile_ysize; y = y + 8) {
                const float* JXL_RESTRICT row = opsin.ConstPlaneRow(c, y);
                size_t stride = opsin.PixelsPerRow();

                for (size_t x = 0; x < tile_xsize; x = x + 8) {
                    float* mem = (float*)calloc(8UL * 8UL, sizeof(float));
                    float* dc_mem =
                        (float*)calloc(AcStrategy::kMaxCoeffBlocks * AcStrategy::kMaxCoeffBlocks, sizeof(float));
                    float* scratch_space = (float*)calloc(2048UL, sizeof(float));
                    AcStrategy acs = AcStrategy::FromRawStrategy(AcStrategy::Type::DCT);
                    size_t xs = acs.covered_blocks_x();
                    N_SCALAR::TransformFromPixels(acs.Strategy(), row + x, stride, mem, scratch_space);
                    N_SCALAR::DCFromLowestFrequencies(acs.Strategy(), mem, dc_mem, xs);
                    for (int m = 0; m < 64; m++) {
                        dct8x8[c][64 * (y / 8 * (tile_xsize / 8) + x / 8) + m] = mem[m];
                    }
                    dc8x8[c][y / 8 * (tile_xsize / 8) + x / 8] = dc_mem[0];
                    free(mem);
                    free(dc_mem);
                    free(scratch_space);
                }
            }
        }

        for (int c = 0; c < 3; c++) {
            for (size_t y = 0; y < tile_ysize; y = y + 16) {
                const float* JXL_RESTRICT row = opsin.ConstPlaneRow(c, y);
                size_t stride = opsin.PixelsPerRow();

                for (size_t x = 0; x < tile_xsize; x = x + 16) {
                    float* mem = (float*)calloc(16UL * 16UL, sizeof(float));
                    float* dc_mem =
                        (float*)calloc(AcStrategy::kMaxCoeffBlocks * AcStrategy::kMaxCoeffBlocks, sizeof(float));
                    float* scratch_space = (float*)calloc(2048UL, sizeof(float));
                    AcStrategy acs = AcStrategy::FromRawStrategy(AcStrategy::Type::DCT16X16);
                    size_t xs = acs.covered_blocks_x();
                    N_SCALAR::TransformFromPixels(acs.Strategy(), row + x, stride, mem, scratch_space);
                    N_SCALAR::DCFromLowestFrequencies(acs.Strategy(), mem, dc_mem, xs);
                    for (int m = 0; m < 16 * 16; m++) {
                        dct16x16[c][16 * 16 * (y / 16 * (tile_xsize / 16) + x / 16) + m] = mem[m];
                    }
                    for (int m = 0; m < 4; m++) {
                        dc16x16[c][4 * (y / 16 * (tile_xsize / 16) + x / 16) + m] = dc_mem[m];
                    }
                    free(mem);
                    free(dc_mem);
                    free(scratch_space);
                }
            }
        }

        for (int c = 0; c < 3; c++) {
            for (size_t y = 0; y < tile_ysize; y = y + 32) {
                const float* JXL_RESTRICT row = opsin.ConstPlaneRow(c, y);
                size_t stride = opsin.PixelsPerRow();

                for (size_t x = 0; x < tile_xsize; x = x + 32) {
                    float* mem = (float*)calloc(32UL * 32UL, sizeof(float));
                    float* dc_mem =
                        (float*)calloc(AcStrategy::kMaxCoeffBlocks * AcStrategy::kMaxCoeffBlocks, sizeof(float));
                    float* scratch_space = (float*)calloc(2048UL, sizeof(float));
                    AcStrategy acs = AcStrategy::FromRawStrategy(AcStrategy::Type::DCT32X32);
                    size_t xs = acs.covered_blocks_x();
                    N_SCALAR::TransformFromPixels(acs.Strategy(), row + x, stride, mem, scratch_space);
                    N_SCALAR::DCFromLowestFrequencies(acs.Strategy(), mem, dc_mem, xs);
                    for (int m = 0; m < 32 * 32; m++) {
                        dct32x32[c][32 * 32 * (y / 32 * (tile_xsize / 32) + x / 32) + m] = mem[m];
                    }
                    for (int m = 0; m < 16; m++) {
                        dc32x32[c][16 * (y / 32 * (tile_xsize / 32) + x / 32) + m] = dc_mem[m];
                    }
                    free(mem);
                    free(dc_mem);
                    free(scratch_space);
                }
            }
        }

#ifdef XLNX_QC_DEBUG_DCT
        std::cout << std::endl
                  << "======================================== full coef "
                     "=============================================="
                  << std::endl;
        for (int c = 0; c < 3; c++) {
            if (c == 1) {
                std::cout << std::setw(15) << 0 << " ";
                for (int m = 0; m < tile_xsize; m++) {
                    std::cout << std::setw(15) << m << " ";
                }
                std::cout << std::endl << std::endl;
                for (int y = 0; y < tile_ysize; y++) {
                    std::cout << std::setw(15) << y << " ";
                    for (int x = 0; x < tile_xsize; x++) {
                        std::cout << std::setw(15) << dct8x8[c][y * tile_xsize + x] << " ";
                    }
                    std::cout << std::endl;
                }
            }
        }
#endif

#ifdef XLNX_QC_DEBUG_DC
        std::cout << std::endl
                  << "======================================== full DC "
                     "=============================================="
                  << std::endl;
        for (int c = 0; c < 3; c++) {
            if (c == 1) {
                std::cout << std::setw(15) << 0 << " ";
                for (int m = 0; m < tile_xsize / 8; m++) {
                    std::cout << std::setw(15) << m << " ";
                }
                std::cout << std::endl << std::endl;
                for (int y = 0; y < tile_ysize / 8; y++) {
                    std::cout << std::setw(15) << y << " ";
                    for (int x = 0; x < tile_xsize / 8; x++) {
                        std::cout << std::setw(15) << dc32x32[c][y * tile_xsize / 8 + x] << " ";
                    }
                    std::cout << std::endl;
                }
            }
        }
#endif

        ArControlFieldHeuristics ar_heuristics;
        AcStrategyHeuristics acs_heuristics;
        CfLHeuristics cfl_heuristics;

        cfl_heuristics.Init(*opsin_);
        acs_heuristics.Init(*opsin_, enc_state_);
        ar_heuristics.PrepareForThreads(/*num_threads*/ 1);
        cfl_heuristics.PrepareForThreads(/*num_threads*/ 1);

        //  auto process_tile = [&](size_t tid, size_t thread) {
        for (int tid = 0; tid < DivCeil(enc_state_->shared.frame_dim.xsize_blocks, kEncTileDimInBlocks) *
                                    DivCeil(enc_state_->shared.frame_dim.ysize_blocks, kEncTileDimInBlocks);
             tid++) {
            size_t thread = 0;
            size_t n_enc_tiles = DivCeil(enc_state_->shared.frame_dim.xsize_blocks, kEncTileDimInBlocks);
            size_t tx = tid % n_enc_tiles;
            size_t ty = tid / n_enc_tiles;
            size_t by0 = ty * kEncTileDimInBlocks;
            size_t by1 = std::min((ty + 1) * kEncTileDimInBlocks, enc_state_->shared.frame_dim.ysize_blocks);
            size_t bx0 = tx * kEncTileDimInBlocks;
            size_t bx1 = std::min((tx + 1) * kEncTileDimInBlocks, enc_state_->shared.frame_dim.xsize_blocks);
            Rect r(bx0, by0, bx1 - bx0, by1 - by0);

            // For speeds up to Wombat, we only compute the color correlation map
            // once we know the transform type and the quantization map.
            if (cparams.speed_tier <= SpeedTier::kSquirrel) {
                cfl_heuristics.ComputeTile(r, *opsin_, enc_state_->shared.matrices,
                                           /*ac_strategy=*/nullptr,
                                           /*quantizer=*/nullptr, /*fast=*/false, thread, &enc_state_->shared.cmap,
                                           opsin.xsize(), opsin.ysize(), dctIDT, dct2x2, dct4x4, dct8x8, dct16x16,
                                           dct32x32, dcIDT, dc2x2, dc4x4, dc8x8, dc16x16, dc32x32);
            }

            // Choose block sizes.
            acs_heuristics.ProcessRect(r, opsin.xsize(), opsin.ysize(), dctIDT, dct2x2, dct4x4, dct8x8, dct16x16,
                                       dct32x32, dcIDT, dc2x2, dc4x4, dc8x8, dc16x16, dc32x32);

// Choose amount of post-processing smoothing.
// TODO(veluca): should this go *after* AdjustQuantField?
#ifndef XLNX_DISABLE_ARC
            ar_heuristics.RunRect(r, *opsin_, enc_state_, thread);
#else
            ImageB* JXL_RESTRICT epf_sharpness = &enc_state_->shared.epf_sharpness;
            FillPlane(static_cast<uint8_t>(4), epf_sharpness, r);
#endif
            // Always set the initial quant field, so we can compute the CfL map
            // with more accuracy. The initial quant field might change in slower
            // modes, but adjusting the quant field with butteraugli when all the
            // other encoding parameters are fixed is likely a more reliable choice
            // anyway.
            AdjustQuantField(enc_state_->shared.ac_strategy, r, &enc_state_->initial_quant_field);
            quantizer.SetQuantFieldRect(enc_state_->initial_quant_field, r, &enc_state_->shared.raw_quant_field);

// Compute a non-default CfL map if we are at Hare speed, or slower.
#ifndef XLNX_DISABLE_2NDCMP
            if (cparams.speed_tier <= SpeedTier::kHare) {
                cfl_heuristics.ComputeTile(r, *opsin_, enc_state_->shared.matrices, &enc_state_->shared.ac_strategy,
                                           &enc_state_->shared.quantizer,
                                           /*fast=*/cparams.speed_tier >= SpeedTier::kWombat, thread,
                                           &enc_state_->shared.cmap, dctIDT, dct2x2, dct4x4, dct8x8, dct16x16, dct32x32,
                                           dcIDT, dc2x2, dc4x4, dc8x8, dc16x16, dc32x32);
            }
#endif
        };
        /*  RunOnPool(pool, 0, DivCeil(enc_state_->shared.frame_dim.xsize_blocks,
                                     kEncTileDimInBlocks) *
                                 DivCeil(enc_state_->shared.frame_dim.ysize_blocks,
                                         kEncTileDimInBlocks),
                    [&](const size_t num_threads) {
                      ar_heuristics.PrepareForThreads(num_threads);
                      cfl_heuristics.PrepareForThreads(num_threads);
                      return true;
                    },
                    process_tile, "Enc Heuristics");*/

        acs_heuristics.Finalize(aux_out);
        if (cparams.speed_tier <= SpeedTier::kHare) {
            cfl_heuristics.ComputeDC(
                /*fast=*/cparams.speed_tier >= SpeedTier::kWombat, &enc_state_->shared.cmap);
        }

        FindBestDequantMatrices(cparams, *opsin_, modular_frame_encoder.get(), &enc_state_->shared.matrices);

        // Refine quantization levels.
        FindBestQuantizer(ib_or_linear, *opsin_, enc_state_, pool, aux_out);

        // Choose a context model that depends on the amount of quantization for
        // AC.
        if (cparams.speed_tier < SpeedTier::kFalcon) {
            FindBestBlockEntropyModel(*enc_state_);
        }

#ifdef XLNX_DEBUG_CMAP
        std::cout << "=========================================" << std::endl;
        std::cout << "ColorMap info: " << std::endl;
        ImageSB* JXL_RESTRICT tmp_map = &enc_state_->shared.cmap.ytox_map;
        int32_t dc = enc_state_->shared.cmap.GetYToXDC();
        std::cout << "Y to X dc: " << dc << std::endl;
        for (int i = 0; i < tmp_map->ysize(); i++) {
            int8_t* JXL_RESTRICT row_out = tmp_map->Row(i);
            for (int j = 0; j < tmp_map->xsize(); j++) {
                std::cout << (int)row_out[j] << " ";
            }
            std::cout << std::endl;
        }

        tmp_map = &enc_state_->shared.cmap.ytox_map;
        dc = enc_state_->shared.cmap.GetYToBDC();
        std::cout << "Y to B dc: " << dc << std::endl;
        for (int i = 0; i < tmp_map->ysize(); i++) {
            int8_t* JXL_RESTRICT row_out = tmp_map->Row(i);
            for (int j = 0; j < tmp_map->xsize(); j++) {
                std::cout << (int)row_out[j] << " ";
            }
            std::cout << std::endl;
        }
        std::cout << std::endl;
#endif

        InitializePassesEncoder(opsin, pool, enc_state_, modular_frame_encoder.get(), aux_out, opsin.xsize(),
                                opsin.ysize(), dctIDT, dct2x2, dct4x4, dct8x8, dct16x16, dct32x32, dcIDT, dc2x2, dc4x4,
                                dc8x8, dc16x16, dc32x32);

        enc_state_->passes.resize(enc_state_->progressive_splitter.GetNumPasses());
        for (PassesEncoderState::PassData& pass : enc_state_->passes) {
            pass.ac_tokens.resize(shared.frame_dim.num_groups);
        }

        lossy_frame_encoder.ComputeAllCoeffOrders(shared.frame_dim);
        shared.num_histograms = 1;

        *frame_header = shared.frame_header;

        // needs to happen *AFTER* VarDCT-ComputeEncodingData.
        JXL_RETURN_IF_ERROR(modular_frame_encoder->ComputeEncodingData(
            *frame_header, *ib.metadata(), &opsin, *extra_channels, lossy_frame_encoder.State(), pool, aux_out,
            /* do_color=*/frame_header->encoding == FrameEncoding::kModular));
    }
    return true;
}
} // namespace jxl

#endif
