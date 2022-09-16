// Copyright (c) the JPEG XL Project Authors. All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#ifndef HLS_KERNEL1_CPP
#define HLS_KERNEL1_CPP

#include "acc_phase1.hpp"

namespace jxl {
namespace {
// Invisible (alpha = 0) pixels tend to be a mess in optimized PNGs.
// Since they have no visual impact whatsoever, we can replace them with
// something that compresses better and reduces artifacts near the edges. This
// does some kind of smooth stuff that seems to work.
// Replace invisible pixels with a weighted average of the pixel to the left,
// the pixel to the topright, and non-invisible neighbours.
// Produces downward-blurry smears, with in the upwards direction only a 1px
// edge duplication but not more. It would probably be better to smear in all
// directions. That requires an alpha-weighed convolution with a large enough
// kernel though, which might be overkill...
void SimplifyInvisible(Image3F* image, const ImageF& alpha, bool lossless) {
    for (size_t c = 0; c < 3; ++c) {
        for (size_t y = 0; y < image->ysize(); ++y) {
            float* JXL_RESTRICT row = image->PlaneRow(c, y);
            const float* JXL_RESTRICT prow = (y > 0 ? image->PlaneRow(c, y - 1) : nullptr);
            const float* JXL_RESTRICT nrow = (y + 1 < image->ysize() ? image->PlaneRow(c, y + 1) : nullptr);
            const float* JXL_RESTRICT a = alpha.Row(y);
            const float* JXL_RESTRICT pa = (y > 0 ? alpha.Row(y - 1) : nullptr);
            const float* JXL_RESTRICT na = (y + 1 < image->ysize() ? alpha.Row(y + 1) : nullptr);
            for (size_t x = 0; x < image->xsize(); ++x) {
                if (a[x] == 0) {
                    if (lossless) {
                        row[x] = 0;
                        continue;
                    }
                    float d = 0.f;
                    row[x] = 0;
                    if (x > 0) {
                        row[x] += row[x - 1];
                        d++;
                        if (a[x - 1] > 0.f) {
                            row[x] += row[x - 1];
                            d++;
                        }
                    }
                    if (x + 1 < image->xsize()) {
                        if (y > 0) {
                            row[x] += prow[x + 1];
                            d++;
                        }
                        if (a[x + 1] > 0.f) {
                            row[x] += 2.f * row[x + 1];
                            d += 2.f;
                        }
                        if (y > 0 && pa[x + 1] > 0.f) {
                            row[x] += 2.f * prow[x + 1];
                            d += 2.f;
                        }
                        if (y + 1 < image->ysize() && na[x + 1] > 0.f) {
                            row[x] += 2.f * nrow[x + 1];
                            d += 2.f;
                        }
                    }
                    if (y > 0 && pa[x] > 0.f) {
                        row[x] += 2.f * prow[x];
                        d += 2.f;
                    }
                    if (y + 1 < image->ysize() && na[x] > 0.f) {
                        row[x] += 2.f * nrow[x];
                        d += 2.f;
                    }
                    if (d > 1.f) row[x] /= d;
                }
            }
        }
    }
}
} // namespace

Status acc_phase1(Image3F& opsin,
                  LossyFrameEncoder& lossy_frame_encoder,
                  CompressParams cparams,
                  std::unique_ptr<FrameHeader>& frame_header,
                  const FrameInfo& frame_info,
                  const ImageBundle* JXL_RESTRICT ib_or_linear,
                  const ImageBundle& ib,
                  AuxOut* aux_out,
                  ThreadPool* pool) {
    const ColorEncoding& c_linear = ColorEncoding::LinearSRGB(ib.IsGray());
    std::unique_ptr<ImageMetadata> metadata_linear = jxl::make_unique<ImageMetadata>();
    metadata_linear->xyb_encoded = (cparams.color_transform == ColorTransform::kXYB);
    metadata_linear->color_encoding = c_linear;
    ImageBundle linear_storage(metadata_linear.get());

    // Allocating a large enough image avoids a copy when padding.
    opsin = Image3F(RoundUpToBlockDim(ib.xsize()), RoundUpToBlockDim(ib.ysize()));
    opsin.ShrinkTo(ib.xsize(), ib.ysize());

    const bool want_linear =
        frame_header->encoding == FrameEncoding::kVarDCT && cparams.speed_tier <= SpeedTier::kKitten;
    ib_or_linear = &ib;

    if (frame_header->color_transform == ColorTransform::kXYB && frame_info.ib_needs_color_transform) {
        // linear_storage would only be used by the Butteraugli loop (passing
        // linear sRGB avoids a color conversion there). Otherwise, don't
        // fill it to reduce memory usage.
        ib_or_linear = ToXYB(ib, pool, &opsin, want_linear ? &linear_storage : nullptr);
    } else { // RGB or YCbCr: don't do anything (forward YCbCr is not
             // implemented, this is only used when the input is already in
             // YCbCr)
             // If encoding a special DC or reference frame, don't do anything:
             // input is already in XYB.
        CopyImageTo(ib.color(), &opsin);
    }
    bool lossless = (frame_header->encoding == FrameEncoding::kModular && cparams.quality_pair.first == 100);
    if (ib.HasAlpha() && !ib.AlphaIsPremultiplied() && !ApplyOverride(cparams.keep_invisible, lossless) &&
        cparams.ec_resampling == cparams.resampling) {
        // simplify invisible pixels
        SimplifyInvisible(&opsin, ib.alpha(), lossless);
        if (want_linear) {
            SimplifyInvisible(const_cast<Image3F*>(&ib_or_linear->color()), ib.alpha(), lossless);
        }
    }
    if (aux_out != nullptr) {
        JXL_RETURN_IF_ERROR(aux_out->InspectImage3F("enc_frame:OpsinDynamicsImage", opsin));
    }
    if (frame_header->encoding == FrameEncoding::kVarDCT) {
        PadImageToBlockMultipleInPlace(&opsin);
        PassesEncoderState* JXL_RESTRICT enc_state_ = lossy_frame_encoder.State();
        //  std::vector<EncCache>& group_caches_ =
        //  lossy_frame_encoder.get_group_cashes();

        JXL_ASSERT((opsin.xsize() % kBlockDim) == 0 && (opsin.ysize() % kBlockDim) == 0);
        PassesSharedState& shared = enc_state_->shared;

        if (!enc_state_->cparams.max_error_mode) {
            float x_qm_scale_steps[3] = {0.65f, 1.25f, 9.0f};
            shared.frame_header.x_qm_scale = 1;
            for (float x_qm_scale_step : x_qm_scale_steps) {
                if (enc_state_->cparams.butteraugli_distance > x_qm_scale_step) {
                    shared.frame_header.x_qm_scale++;
                }
            }
        }

        Image3F* opsin_ = &opsin;
        //  CompressParams& cparams = enc_state->cparams;
        //  PassesSharedState& shared = enc_state->shared;

        // Compute parameters for noise synthesis.
        if (shared.frame_header.flags & FrameHeader::kNoise) {
            PROFILER_ZONE("enc GetNoiseParam");
            if (cparams.photon_noise_iso > 0) {
                shared.image_features.noise_params =
                    SimulatePhotonNoise(opsin_->xsize(), opsin_->ysize(), cparams.photon_noise_iso);
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
                const float rampup =
                    (cparams.butteraugli_distance - kNoiseRampupStart) / kNoiseModelingRampUpDistanceRange;
                if (rampup < 1.0f) {
                    quality_coef = kNoiseLevelAtStartOfRampUp + (1.0f - kNoiseLevelAtStartOfRampUp) * rampup;
                }
                if (rampup < 0.0f) {
                    quality_coef = kNoiseRampupStart;
                }
                if (!GetNoiseParameter(*opsin_, &shared.image_features.noise_params, quality_coef)) {
                    shared.frame_header.flags &= ~FrameHeader::kNoise;
                }
            }
        }
        if (enc_state_->shared.frame_header.upsampling != 1 && !cparams.already_downsampled) {
            // In VarDCT mode, LossyFrameHeuristics takes care of running downsampling
            // after noise, if necessary.
            DownsampleImage(opsin_, cparams.resampling);
            PadImageToBlockMultipleInPlace(opsin_);
        }

        const FrameDimensions& frame_dim_ = enc_state_->shared.frame_dim;
        size_t target_size = TargetSize(cparams, frame_dim_);
        size_t opsin_target_size = target_size;
        if (cparams.target_size > 0 || cparams.target_bitrate > 0.0) {
            cparams.target_size = opsin_target_size;
        } else if (cparams.butteraugli_distance < 0) {
            return JXL_FAILURE("Expected non-negative distance");
        }

#ifndef XLNX_DISABLE_BLK_DICT
        // Find and subtract splines.
        if (cparams.speed_tier <= SpeedTier::kSquirrel) {
            shared.image_features.splines = FindSplines(*opsin_);
            JXL_RETURN_IF_ERROR(shared.image_features.splines.SubtractFrom(opsin_, shared.cmap));
        }

        // Find and subtract patches/dots.
        if (ApplyOverride(cparams.patches, cparams.speed_tier <= SpeedTier::kSquirrel)) {
            FindBestPatchDictionary(*opsin_, enc_state_, pool, aux_out);
            PatchDictionaryEncoder::SubtractFrom(shared.image_features.patches, opsin_);
        }
#endif

        static const float kAcQuant = 0.79f;
        const float quant_dc = InitialQuantDC(cparams.butteraugli_distance);
        Quantizer& quantizer = enc_state_->shared.quantizer;
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

        if (!opsin_->xsize()) {
            JXL_ASSERT(enc_state_->heuristics->HandlesColorConversion(cparams, *ib_or_linear));
            *opsin_ = Image3F(RoundUpToBlockDim(ib_or_linear->xsize()), RoundUpToBlockDim(ib_or_linear->ysize()));
            opsin_->ShrinkTo(ib_or_linear->xsize(), ib_or_linear->ysize());
            ToXYB(*ib_or_linear, pool, opsin_, /*linear=*/nullptr);
            PadImageToBlockMultipleInPlace(opsin_);
        }

        // Compute an initial estimate of the quantization field.
        // Call InitialQuantField only in Hare mode or slower. Otherwise, rely
        // on simple heuristics in FindBestAcStrategy, or set a constant for Falcon
        // mode.
        if (cparams.speed_tier > SpeedTier::kHare || cparams.uniform_quant > 0) {
            enc_state_->initial_quant_field = ImageF(shared.frame_dim.xsize_blocks, shared.frame_dim.ysize_blocks);
            float q = cparams.uniform_quant > 0 ? cparams.uniform_quant : kAcQuant / cparams.butteraugli_distance;
            FillImage(q, &enc_state_->initial_quant_field);
        } else {
            // Call this here, as it relies on pre-gaborish values.
            float butteraugli_distance_for_iqf = cparams.butteraugli_distance;
            if (!shared.frame_header.loop_filter.gab) {
                butteraugli_distance_for_iqf *= 0.73f;
            }
            enc_state_->initial_quant_field = InitialQuantField(butteraugli_distance_for_iqf, *opsin_, shared.frame_dim,
                                                                pool, 1.0f, &enc_state_->initial_quant_masking);
        }

        // TODO(veluca): do something about animations.

        // Apply inverse-gaborish.
        if (shared.frame_header.loop_filter.gab) {
            GaborishInverse(opsin_, 0.9908511000000001f, pool);
        }
    }
    return true;
}
} // namespace jxl
#endif