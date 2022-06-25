// Copyright (c) the JPEG XL Project Authors. All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#include "acc_host.hpp"

namespace jxl {
void FindBestDequantMatrices(const CompressParams& cparams,
                             const Image3F& opsin,
                             ModularFrameEncoder* modular_frame_encoder,
                             DequantMatrices* dequant_matrices) {
    // TODO(veluca): quant matrices for no-gaborish.
    // TODO(veluca): heuristics for in-bitstream quant tables.
    *dequant_matrices = DequantMatrices();
    if (cparams.max_error_mode) {
        // Set numerators of all quantization matrices to constant values.
        float weights[3][1] = {
            {1.0f / cparams.max_error[0]}, {1.0f / cparams.max_error[1]}, {1.0f / cparams.max_error[2]}};
        DctQuantWeightParams dct_params(weights);
        std::vector<QuantEncoding> encodings(DequantMatrices::kNum, QuantEncoding::DCT(dct_params));
        DequantMatricesSetCustom(dequant_matrices, encodings, modular_frame_encoder);
        float dc_weights[3] = {1.0f / cparams.max_error[0], 1.0f / cparams.max_error[1], 1.0f / cparams.max_error[2]};
        DequantMatricesSetCustomDC(dequant_matrices, dc_weights);
    }
}

bool DefaultEncoderHeuristics::HandlesColorConversion(const CompressParams& cparams, const ImageBundle& ib) {
    return cparams.noise != Override::kOn && cparams.patches != Override::kOn &&
           cparams.speed_tier >= SpeedTier::kWombat && cparams.resampling == 1 &&
           cparams.color_transform == ColorTransform::kXYB && !cparams.modular_mode && !ib.HasAlpha();
}

Status acc_host(std::string xclbinPath,
                Image3F& opsin,
                LossyFrameEncoder& lossy_frame_encoder,
                const ImageBundle* JXL_RESTRICT ib_or_linear,
                ThreadPool* pool,
                std::unique_ptr<ModularFrameEncoder>& modular_frame_encoder,
                BitWriter* writer,
                AuxOut* aux_out,
                std::unique_ptr<FrameHeader>& frame_header,
                const FrameInfo& frame_info,
                CompressParams cparams,
                const std::vector<ImageF>* extra_channels,
                PassesEncoderState* passes_enc_state,
                FrameDimensions frame_dim,
                const size_t num_groups,
                const ImageBundle& ib,
                std::vector<AuxOut>& aux_outs,
                const std::function<Status(size_t)>& resize_aux_outs) {
    acc_phase1(opsin, lossy_frame_encoder, cparams, frame_header, frame_info, ib_or_linear, ib, aux_out, pool);

    acc_phase2(xclbinPath, opsin, lossy_frame_encoder, modular_frame_encoder, cparams, frame_header, extra_channels,
               ib_or_linear, ib, pool, aux_out);

    acc_phase3(xclbinPath, opsin, lossy_frame_encoder, modular_frame_encoder, cparams, frame_header, passes_enc_state,
               frame_dim, writer, num_groups, aux_out, pool, aux_outs, ib, resize_aux_outs);

    return true;
}

Status DefaultEncoderHeuristics::LossyFrameHeuristics(PassesEncoderState* enc_state,
                                                      ModularFrameEncoder* modular_frame_encoder,
                                                      const ImageBundle* original_pixels,
                                                      Image3F* opsin,
                                                      ThreadPool* pool,
                                                      AuxOut* aux_out) {
    PROFILER_ZONE("JxlLossyFrameHeuristics uninstrumented");

    CompressParams& cparams = enc_state->cparams;
    PassesSharedState& shared = enc_state->shared;

    // Compute parameters for noise synthesis.
    if (shared.frame_header.flags & FrameHeader::kNoise) {
        PROFILER_ZONE("enc GetNoiseParam");
        if (cparams.photon_noise_iso > 0) {
            shared.image_features.noise_params =
                SimulatePhotonNoise(opsin->xsize(), opsin->ysize(), cparams.photon_noise_iso);
        } else {
            // Don't start at zero amplitude since adding noise is expensive -- it
            // significantly slows down decoding, and this is unlikely to
            // completely go away even with advanced optimizations. After the
            // kNoiseModelingRampUpDistanceRange we have reached the full level,
            // i.e. noise is no longer represented by the compressed image, so we
            // can add full noise by the noise modeling itself.
            static const float kNoiseModelingRampUpDistanceRange = 0.6;
            static const float kNoiseLevelAtStartOfRampUp = 0.25;
            static const float kNoiseRampupStart = 1.0;
            // TODO(user) test and properly select quality_coef with smooth
            // filter
            float quality_coef = 1.0f;
            const float rampup = (cparams.butteraugli_distance - kNoiseRampupStart) / kNoiseModelingRampUpDistanceRange;
            if (rampup < 1.0f) {
                quality_coef = kNoiseLevelAtStartOfRampUp + (1.0f - kNoiseLevelAtStartOfRampUp) * rampup;
            }
            if (rampup < 0.0f) {
                quality_coef = kNoiseRampupStart;
            }
            if (!GetNoiseParameter(*opsin, &shared.image_features.noise_params, quality_coef)) {
                shared.frame_header.flags &= ~FrameHeader::kNoise;
            }
        }
    }
    if (enc_state->shared.frame_header.upsampling != 1 && !cparams.already_downsampled) {
        // In VarDCT mode, LossyFrameHeuristics takes care of running downsampling
        // after noise, if necessary.
        DownsampleImage(opsin, cparams.resampling);
        PadImageToBlockMultipleInPlace(opsin);
    }

    const FrameDimensions& frame_dim = enc_state->shared.frame_dim;
    size_t target_size = TargetSize(cparams, frame_dim);
    size_t opsin_target_size = target_size;
    if (cparams.target_size > 0 || cparams.target_bitrate > 0.0) {
        cparams.target_size = opsin_target_size;
    } else if (cparams.butteraugli_distance < 0) {
        return JXL_FAILURE("Expected non-negative distance");
    }

#ifndef XLNX_DISABLE_BLK_DICT
    // Find and subtract splines.
    if (cparams.speed_tier <= SpeedTier::kSquirrel) {
        shared.image_features.splines = FindSplines(*opsin);
        JXL_RETURN_IF_ERROR(shared.image_features.splines.SubtractFrom(opsin, shared.cmap));
    }

    // Find and subtract patches/dots.
    if (ApplyOverride(cparams.patches, cparams.speed_tier <= SpeedTier::kSquirrel)) {
        FindBestPatchDictionary(*opsin, enc_state, pool, aux_out);
        PatchDictionaryEncoder::SubtractFrom(shared.image_features.patches, opsin);
    }
#endif

    static const float kAcQuant = 0.79f;
    const float quant_dc = InitialQuantDC(cparams.butteraugli_distance);
    Quantizer& quantizer = enc_state->shared.quantizer;
    // We don't know the quant field yet, but for computing the global scale
    // assuming that it will be the same as for Falcon mode is good enough.
    quantizer.ComputeGlobalScaleAndQuant(quant_dc, kAcQuant / cparams.butteraugli_distance, 0);

    // TODO(veluca): we can now run all the code from here to FindBestQuantizer
    // (excluded) one rect at a time. Do that.

    // Dependency graph:
    //
    // input: either XYB or input image
    //
    // input image -> XYB [optional]
    // XYB -> initial quant field
    // XYB -> Gaborished XYB
    // Gaborished XYB -> CfL1
    // initial quant field, Gaborished XYB, CfL1 -> ACS
    // initial quant field, ACS, Gaborished XYB -> EPF control field
    // initial quant field -> adjusted initial quant field
    // adjusted initial quant field, ACS -> raw quant field
    // raw quant field, ACS, Gaborished XYB -> CfL2
    //
    // output: Gaborished XYB, CfL, ACS, raw quant field, EPF control field.

    ArControlFieldHeuristics ar_heuristics;
    AcStrategyHeuristics acs_heuristics;
    CfLHeuristics cfl_heuristics;

    if (!opsin->xsize()) {
        JXL_ASSERT(HandlesColorConversion(cparams, *original_pixels));
        *opsin = Image3F(RoundUpToBlockDim(original_pixels->xsize()), RoundUpToBlockDim(original_pixels->ysize()));
        opsin->ShrinkTo(original_pixels->xsize(), original_pixels->ysize());
        ToXYB(*original_pixels, pool, opsin, /*linear=*/nullptr);
        PadImageToBlockMultipleInPlace(opsin);
    }

    // Compute an initial estimate of the quantization field.
    // Call InitialQuantField only in Hare mode or slower. Otherwise, rely
    // on simple heuristics in FindBestAcStrategy, or set a constant for Falcon
    // mode.
    if (cparams.speed_tier > SpeedTier::kHare || cparams.uniform_quant > 0) {
        enc_state->initial_quant_field = ImageF(shared.frame_dim.xsize_blocks, shared.frame_dim.ysize_blocks);
        float q = cparams.uniform_quant > 0 ? cparams.uniform_quant : kAcQuant / cparams.butteraugli_distance;
        FillImage(q, &enc_state->initial_quant_field);
    } else {
        // Call this here, as it relies on pre-gaborish values.
        float butteraugli_distance_for_iqf = cparams.butteraugli_distance;
        if (!shared.frame_header.loop_filter.gab) {
            butteraugli_distance_for_iqf *= 0.73f;
        }
        enc_state->initial_quant_field = InitialQuantField(butteraugli_distance_for_iqf, *opsin, shared.frame_dim, pool,
                                                           1.0f, &enc_state->initial_quant_masking);
    }

    // TODO(veluca): do something about animations.

    // Apply inverse-gaborish.
    if (shared.frame_header.loop_filter.gab) {
        GaborishInverse(opsin, 0.9908511000000001f, pool);
    }

    cfl_heuristics.Init(*opsin);
    acs_heuristics.Init(*opsin, enc_state);
    ar_heuristics.PrepareForThreads(/*num_threads*/ 1);
    cfl_heuristics.PrepareForThreads(/*num_threads*/ 1);

    //  auto process_tile = [&](size_t tid, size_t thread) {
    for (int tid = 0; tid < DivCeil(enc_state->shared.frame_dim.xsize_blocks, kEncTileDimInBlocks) *
                                DivCeil(enc_state->shared.frame_dim.ysize_blocks, kEncTileDimInBlocks);
         tid++) {
        size_t thread = 0;
        size_t n_enc_tiles = DivCeil(enc_state->shared.frame_dim.xsize_blocks, kEncTileDimInBlocks);
        size_t tx = tid % n_enc_tiles;
        size_t ty = tid / n_enc_tiles;
        size_t by0 = ty * kEncTileDimInBlocks;
        size_t by1 = std::min((ty + 1) * kEncTileDimInBlocks, enc_state->shared.frame_dim.ysize_blocks);
        size_t bx0 = tx * kEncTileDimInBlocks;
        size_t bx1 = std::min((tx + 1) * kEncTileDimInBlocks, enc_state->shared.frame_dim.xsize_blocks);
        Rect r(bx0, by0, bx1 - bx0, by1 - by0);

        // For speeds up to Wombat, we only compute the color correlation map
        // once we know the transform type and the quantization map.
        if (cparams.speed_tier <= SpeedTier::kSquirrel) {
            //      cfl_heuristics.ComputeTile(r, *opsin, enc_state->shared.matrices,
            //                                 /*ac_strategy=*/nullptr,
            //                                 /*quantizer=*/nullptr, /*fast=*/false, thread,
            //                                 &enc_state->shared.cmap);
        }

// Choose block sizes.
//    acs_heuristics.ProcessRect(r);

// Choose amount of post-processing smoothing.
// TODO(veluca): should this go *after* AdjustQuantField?
#ifndef XLNX_DISABLE_ARC
        ar_heuristics.RunRect(r, *opsin, enc_state, thread);
#else
        ImageB* JXL_RESTRICT epf_sharpness = &enc_state->shared.epf_sharpness;
        FillPlane(static_cast<uint8_t>(4), epf_sharpness, r);
#endif
        // Always set the initial quant field, so we can compute the CfL map with
        // more accuracy. The initial quant field might change in slower modes, but
        // adjusting the quant field with butteraugli when all the other encoding
        // parameters are fixed is likely a more reliable choice anyway.
        AdjustQuantField(enc_state->shared.ac_strategy, r, &enc_state->initial_quant_field);
        quantizer.SetQuantFieldRect(enc_state->initial_quant_field, r, &enc_state->shared.raw_quant_field);

// Compute a non-default CfL map if we are at Hare speed, or slower.
#ifndef XLNX_DISABLE_2NDCMP
        if (cparams.speed_tier <= SpeedTier::kHare) {
            cfl_heuristics.ComputeTile(
                r, *opsin, enc_state->shared.matrices, &enc_state->shared.ac_strategy, &enc_state->shared.quantizer,
                /*fast=*/cparams.speed_tier >= SpeedTier::kWombat, thread, &enc_state->shared.cmap);
        }
#endif
    };
    /*  RunOnPool(pool, 0, DivCeil(enc_state->shared.frame_dim.xsize_blocks,
                                 kEncTileDimInBlocks) *
                             DivCeil(enc_state->shared.frame_dim.ysize_blocks,
                                     kEncTileDimInBlocks),
                [&](const size_t num_threads) {
                  ar_heuristics.PrepareForThreads(num_threads);
                  cfl_heuristics.PrepareForThreads(num_threads);
                  return true;
                },
                process_tile, "Enc Heuristics");*/

    acs_heuristics.Finalize(aux_out);
    if (cparams.speed_tier <= SpeedTier::kHare) {
        cfl_heuristics.ComputeDC(/*fast=*/cparams.speed_tier >= SpeedTier::kWombat, &enc_state->shared.cmap);
    }

    FindBestDequantMatrices(cparams, *opsin, modular_frame_encoder, &enc_state->shared.matrices);

    // Refine quantization levels.
    FindBestQuantizer(original_pixels, *opsin, enc_state, pool, aux_out);

    // Choose a context model that depends on the amount of quantization for AC.
    if (cparams.speed_tier < SpeedTier::kFalcon) {
        FindBestBlockEntropyModel(*enc_state);
    }

#ifdef XLNX_DEBUG_CMAP
    std::cout << "=========================================" << std::endl;
    std::cout << "ColorMap info: " << std::endl;
    ImageSB* JXL_RESTRICT tmp_map = &enc_state->shared.cmap.ytox_map;
    int32_t dc = enc_state->shared.cmap.GetYToXDC();
    std::cout << "Y to X dc: " << dc << std::endl;
    for (int i = 0; i < tmp_map->ysize(); i++) {
        int8_t* JXL_RESTRICT row_out = tmp_map->Row(i);
        for (int j = 0; j < tmp_map->xsize(); j++) {
            std::cout << (int)row_out[j] << " ";
        }
        std::cout << std::endl;
    }

    tmp_map = &enc_state->shared.cmap.ytox_map;
    dc = enc_state->shared.cmap.GetYToBDC();
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

    return true;
}
} // namespace jxl
