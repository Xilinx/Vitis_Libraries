// Copyright (c) the JPEG XL Project Authors. All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#ifndef ACC_COMMON_HPP
#define ACC_COMMON_HPP

#include "xlnx_cfg.h"

#include <ap_int.h>
#include "lib/jxl/enc_frame.h"

#include <stddef.h>
#include <stdint.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <limits>
#include <numeric>
#include <vector>

#include "lib/jxl/ac_context.h"
#include "lib/jxl/ac_strategy.h"
#include "lib/jxl/ans_params.h"
#include "lib/jxl/aux_out.h"
#include "lib/jxl/aux_out_fwd.h"
#include "lib/jxl/base/bits.h"
#include "lib/jxl/base/compiler_specific.h"
#include "lib/jxl/base/data_parallel.h"
#include "lib/jxl/base/override.h"
#include "lib/jxl/base/padded_bytes.h"
#include "lib/jxl/base/profiler.h"
#include "lib/jxl/base/status.h"
#include "lib/jxl/chroma_from_luma.h"
#include "lib/jxl/coeff_order.h"
#include "lib/jxl/coeff_order_fwd.h"
#include "lib/jxl/color_encoding_internal.h"
#include "lib/jxl/color_management.h"
#include "lib/jxl/common.h"
#include "lib/jxl/compressed_dc.h"
#include "lib/jxl/dct_util.h"
#include "lib/jxl/enc_adaptive_quantization.h"
#include "lib/jxl/enc_ans.h"
#include "lib/jxl/enc_ar_control_field.h"
#include "lib/jxl/enc_bit_writer.h"
#include "lib/jxl/enc_cache.h"
#include "lib/jxl/enc_coeff_order.h"
#include "lib/jxl/enc_context_map.h"
#include "lib/jxl/enc_entropy_coder.h"
#include "lib/jxl/enc_modular.h"
#include "lib/jxl/enc_noise.h"
#include "lib/jxl/enc_params.h"
#include "lib/jxl/enc_patch_dictionary.h"
#include "lib/jxl/enc_photon_noise.h"
#include "lib/jxl/enc_quant_weights.h"
#include "lib/jxl/enc_splines.h"
#include "lib/jxl/enc_toc.h"
#include "lib/jxl/enc_xyb.h"
#include "lib/jxl/fields.h"
#include "lib/jxl/frame_header.h"
#include "lib/jxl/gaborish.h"
#include "lib/jxl/image.h"
#include "lib/jxl/image_bundle.h"
#include "lib/jxl/image_ops.h"
#include "lib/jxl/loop_filter.h"
#include "lib/jxl/quant_weights.h"
#include "lib/jxl/quantizer.h"
#include "lib/jxl/splines.h"
#include "lib/jxl/toc.h"
#include "acc_enc_ac_strategy.hpp"
#include "acc_enc_chroma_from_luma.hpp"
#include "acc_enc_group.hpp"

namespace jxl {
namespace {
void ClusterGroups(PassesEncoderState* enc_state) {
    if (enc_state->shared.frame_header.passes.num_passes > 1) {
        // TODO(veluca): implement this for progressive modes.
        return;
    }
    // This only considers pass 0 for now.
    std::vector<uint8_t> context_map;
    EntropyEncodingData codes;
    auto& ac = enc_state->passes[0].ac_tokens;
    size_t limit = std::ceil(std::sqrt(ac.size()));
    if (limit == 1) return;
    size_t num_contexts = enc_state->shared.block_ctx_map.NumACContexts();
    std::vector<float> costs(ac.size());
    HistogramParams params;
    params.uint_method = HistogramParams::HybridUintMethod::kNone;
    params.lz77_method = HistogramParams::LZ77Method::kNone;
    params.ans_histogram_strategy = HistogramParams::ANSHistogramStrategy::kApproximate;
    size_t max = 0;
    auto token_cost = [&](std::vector<std::vector<Token> >& tokens, size_t num_ctx, bool estimate = true) {
        // TODO(veluca): not estimating is very expensive.
        BitWriter writer;
        size_t c = BuildAndEncodeHistograms(params, num_ctx, tokens, &codes, &context_map, estimate ? nullptr : &writer,
                                            0, /*aux_out=*/0);
        if (estimate) return c;
        for (size_t i = 0; i < tokens.size(); i++) {
            WriteTokens(tokens[i], codes, context_map, &writer, 0, nullptr);
        }
        return writer.BitsWritten();
    };
    for (size_t i = 0; i < ac.size(); i++) {
        std::vector<std::vector<Token> > tokens{ac[i]};
        costs[i] = token_cost(tokens, enc_state->shared.block_ctx_map.NumACContexts());
        if (costs[i] > costs[max]) {
            max = i;
        }
    }
    auto dist = [&](int i, int j) {
        std::vector<std::vector<Token> > tokens{ac[i], ac[j]};
        return token_cost(tokens, num_contexts) - costs[i] - costs[j];
    };
    std::vector<size_t> out{max};
    std::vector<size_t> old_map(ac.size());
    std::vector<float> dists(ac.size());
    size_t farthest = 0;
    for (size_t i = 0; i < ac.size(); i++) {
        if (i == max) continue;
        dists[i] = dist(max, i);
        if (dists[i] > dists[farthest]) {
            farthest = i;
        }
    }

    while (dists[farthest] > 0 && out.size() < limit) {
        out.push_back(farthest);
        dists[farthest] = 0;
        enc_state->histogram_idx[farthest] = out.size() - 1;
        for (size_t i = 0; i < ac.size(); i++) {
            float d = dist(out.back(), i);
            if (d < dists[i]) {
                dists[i] = d;
                old_map[i] = enc_state->histogram_idx[i];
                enc_state->histogram_idx[i] = out.size() - 1;
            }
            if (dists[i] > dists[farthest]) {
                farthest = i;
            }
        }
    }

    std::vector<size_t> remap(out.size());
    std::iota(remap.begin(), remap.end(), 0);
    for (size_t i = 0; i < enc_state->histogram_idx.size(); i++) {
        enc_state->histogram_idx[i] = remap[enc_state->histogram_idx[i]];
    }
    auto remap_cost = [&](std::vector<size_t> remap) {
        std::vector<size_t> re_remap(remap.size(), remap.size());
        size_t r = 0;
        for (size_t i = 0; i < remap.size(); i++) {
            if (re_remap[remap[i]] == remap.size()) {
                re_remap[remap[i]] = r++;
            }
            remap[i] = re_remap[remap[i]];
        }
        auto tokens = ac;
        size_t max_hist = 0;
        for (size_t i = 0; i < tokens.size(); i++) {
            for (size_t j = 0; j < tokens[i].size(); j++) {
                size_t hist = remap[enc_state->histogram_idx[i]];
                tokens[i][j].context += hist * num_contexts;
                max_hist = std::max(hist + 1, max_hist);
            }
        }
        return token_cost(tokens, max_hist * num_contexts, /*estimate=*/false);
    };

    for (size_t src = 0; src < out.size(); src++) {
        float cost = remap_cost(remap);
        size_t best = src;
        for (size_t j = src + 1; j < out.size(); j++) {
            if (remap[src] == remap[j]) continue;
            auto remap_c = remap;
            std::replace(remap_c.begin(), remap_c.end(), remap[src], remap[j]);
            float c = remap_cost(remap_c);
            if (c < cost) {
                best = j;
                cost = c;
            }
        }
        if (src != best) {
            std::replace(remap.begin(), remap.end(), remap[src], remap[best]);
        }
    }
    std::vector<size_t> re_remap(remap.size(), remap.size());
    size_t r = 0;
    for (size_t i = 0; i < remap.size(); i++) {
        if (re_remap[remap[i]] == remap.size()) {
            re_remap[remap[i]] = r++;
        }
        remap[i] = re_remap[remap[i]];
    }

    enc_state->shared.num_histograms = *std::max_element(remap.begin(), remap.end()) + 1;
    for (size_t i = 0; i < enc_state->histogram_idx.size(); i++) {
        enc_state->histogram_idx[i] = remap[enc_state->histogram_idx[i]];
    }
    for (size_t i = 0; i < ac.size(); i++) {
        for (size_t j = 0; j < ac[i].size(); j++) {
            ac[i][j].context += enc_state->histogram_idx[i] * num_contexts;
        }
    }
} // ClusterGroups

void FindBestBlockEntropyModel(PassesEncoderState& enc_state) {
    if (enc_state.cparams.decoding_speed_tier >= 1) {
        static constexpr uint8_t kSimpleCtxMap[] = {
            // Cluster all blocks together
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //
            1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, //
            1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, //
        };
        static_assert(3 * kNumOrders == sizeof(kSimpleCtxMap) / sizeof *kSimpleCtxMap, "Update simple context map");

        auto bcm = enc_state.shared.block_ctx_map;
        bcm.ctx_map.assign(std::begin(kSimpleCtxMap), std::end(kSimpleCtxMap));
        bcm.num_ctxs = 2;
        bcm.num_dc_ctxs = 1;
        return;
    }
    if (enc_state.cparams.speed_tier >= SpeedTier::kFalcon) {
        return;
    }
    const ImageI& rqf = enc_state.shared.raw_quant_field;
    // No need to change context modeling for small images.
    size_t tot = rqf.xsize() * rqf.ysize();
    size_t size_for_ctx_model = (1 << 10) * enc_state.cparams.butteraugli_distance;
    //  if (tot < size_for_ctx_model) return;

    struct OccCounters {
        // count the occurrences of each qf value and each strategy type.
        OccCounters(const ImageI& rqf, const AcStrategyImage& ac_strategy) {
            for (size_t y = 0; y < rqf.ysize(); y++) {
                const int32_t* qf_row = rqf.Row(y);
                AcStrategyRow acs_row = ac_strategy.ConstRow(y);
                for (size_t x = 0; x < rqf.xsize(); x++) {
                    int ord = kStrategyOrder[acs_row[x].RawStrategy()];
                    int qf = qf_row[x] - 1;
                    qf_counts[qf]++;
                    qf_ord_counts[ord][qf]++;
                    ord_counts[ord]++;
                }
            }
        }

        size_t qf_counts[256] = {};
        size_t qf_ord_counts[kNumOrders][256] = {};
        size_t ord_counts[kNumOrders] = {};
    };
    // The OccCounters struct is too big to allocate on the stack.
    std::unique_ptr<OccCounters> counters(new OccCounters(rqf, enc_state.shared.ac_strategy));

    // Splitting the context model according to the quantization field seems to
    // mostly benefit only large images.
    size_t size_for_qf_split = (1 << 13) * enc_state.cparams.butteraugli_distance;
    size_t num_qf_segments = tot < size_for_qf_split ? 1 : 2;
    std::vector<uint32_t>& qft = enc_state.shared.block_ctx_map.qf_thresholds;
    qft.clear();
    // Divide the quant field in up to num_qf_segments segments.
    size_t cumsum = 0;
    size_t next = 1;
    size_t last_cut = 256;
    size_t cut = tot * next / num_qf_segments;
    for (uint32_t j = 0; j < 256; j++) {
        cumsum += counters->qf_counts[j];
        if (cumsum > cut) {
            if (j != 0) {
                qft.push_back(j);
            }
            last_cut = j;
            while (cumsum > cut) {
                next++;
                cut = tot * next / num_qf_segments;
            }
        } else if (next > qft.size() + 1) {
            if (j - 1 == last_cut && j != 0) {
                qft.push_back(j);
            }
        }
    }

    // Count the occurrences of each segment.
    std::vector<size_t> counts(kNumOrders * (qft.size() + 1));
    size_t qft_pos = 0;
    for (size_t j = 0; j < 256; j++) {
        if (qft_pos < qft.size() && j == qft[qft_pos]) {
            qft_pos++;
        }
        for (size_t i = 0; i < kNumOrders; i++) {
            counts[qft_pos + i * (qft.size() + 1)] += counters->qf_ord_counts[i][j];
        }
    }

    // Repeatedly merge the lowest-count pair.
    std::vector<uint8_t> remap((qft.size() + 1) * kNumOrders);
    std::iota(remap.begin(), remap.end(), 0);
    std::vector<uint8_t> clusters(remap);
    size_t nb_clusters = 4; // Clamp1((int)(tot / size_for_ctx_model / 2), 4, 8);
    // This is O(n^2 log n), but n <= 14.
    while (clusters.size() > nb_clusters) {
        std::sort(clusters.begin(), clusters.end(), [&](int a, int b) { return counts[a] > counts[b]; });
        counts[clusters[clusters.size() - 2]] += counts[clusters.back()];
        counts[clusters.back()] = 0;
        remap[clusters.back()] = clusters[clusters.size() - 2];
        clusters.pop_back();
    }
    for (size_t i = 0; i < remap.size(); i++) {
        while (remap[remap[i]] != remap[i]) {
            remap[i] = remap[remap[i]];
        }
    }
    // Relabel starting from 0.
    std::vector<uint8_t> remap_remap(remap.size(), remap.size());
    size_t num = 0;
    for (size_t i = 0; i < remap.size(); i++) {
        if (remap_remap[remap[i]] == remap.size()) {
            remap_remap[remap[i]] = num++;
        }
        remap[i] = remap_remap[remap[i]];
    }
    // Write the block context map.
    auto& ctx_map = enc_state.shared.block_ctx_map.ctx_map;
    ctx_map = remap;
    ctx_map.resize(remap.size() * 3);
    for (size_t i = remap.size(); i < remap.size() * 3; i++) {
        ctx_map[i] = remap[i % remap.size()] + num;
    }
    enc_state.shared.block_ctx_map.num_ctxs = *std::max_element(ctx_map.begin(), ctx_map.end()) + 1;
}

// Returns the target size based on whether bitrate or direct targetsize is
// given.
size_t TargetSize(const CompressParams& cparams, const FrameDimensions& frame_dim) {
    if (cparams.target_size > 0) {
        return cparams.target_size;
    }
    if (cparams.target_bitrate > 0.0) {
        return 0.5 + cparams.target_bitrate * frame_dim.xsize * frame_dim.ysize / kBitsPerByte;
    }
    return 0;
}
} // namespace

class LossyFrameEncoder {
   public:
    LossyFrameEncoder(const CompressParams& cparams,
                      const FrameHeader& frame_header,
                      PassesEncoderState* JXL_RESTRICT enc_state,
                      ThreadPool* pool,
                      AuxOut* aux_out)
        : enc_state_(enc_state), pool_(pool), aux_out_(aux_out) {
        JXL_CHECK(InitializePassesSharedState(frame_header, &enc_state_->shared,
                                              /*encoder=*/true));
        enc_state_->cparams = cparams;
        enc_state_->passes.clear();
    }

    Status ComputeEncodingData(const ImageBundle* linear,
                               Image3F* JXL_RESTRICT opsin,
                               ThreadPool* pool,
                               ModularFrameEncoder* modular_frame_encoder,
                               BitWriter* JXL_RESTRICT writer,
                               FrameHeader* frame_header) {
        PROFILER_ZONE("ComputeEncodingData uninstrumented");
        JXL_ASSERT((opsin->xsize() % kBlockDim) == 0 && (opsin->ysize() % kBlockDim) == 0);
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

        JXL_RETURN_IF_ERROR(enc_state_->heuristics->LossyFrameHeuristics(enc_state_, modular_frame_encoder, linear,
                                                                         opsin, pool_, aux_out_));

        /*    InitializePassesEncoder(*opsin, pool_, enc_state_,
           modular_frame_encoder,
                                    aux_out_);*/

        enc_state_->passes.resize(enc_state_->progressive_splitter.GetNumPasses());
        for (PassesEncoderState::PassData& pass : enc_state_->passes) {
            pass.ac_tokens.resize(shared.frame_dim.num_groups);
        }

        ComputeAllCoeffOrders(shared.frame_dim);
        shared.num_histograms = 1;

        const auto tokenize_group_init = [&](const size_t num_threads) {
            group_caches_.resize(num_threads);
            return true;
        };
        const auto tokenize_group = [&](const int group_index, const int thread) {
            // Tokenize coefficients.
            const Rect rect = shared.BlockGroupRect(group_index);
            for (size_t idx_pass = 0; idx_pass < enc_state_->passes.size(); idx_pass++) {
                JXL_ASSERT(enc_state_->coeffs[idx_pass]->Type() == ACType::k32);
                const int32_t* JXL_RESTRICT ac_rows[3] = {
                    enc_state_->coeffs[idx_pass]->PlaneRow(0, group_index, 0).ptr32,
                    enc_state_->coeffs[idx_pass]->PlaneRow(1, group_index, 0).ptr32,
                    enc_state_->coeffs[idx_pass]->PlaneRow(2, group_index, 0).ptr32,
                };
                // Ensure group cache is initialized.
                group_caches_[thread].InitOnce();
                TokenizeCoefficients(&shared.coeff_orders[idx_pass * shared.coeff_order_size], rect, ac_rows,
                                     shared.ac_strategy, frame_header->chroma_subsampling,
                                     &group_caches_[thread].num_nzeroes,
                                     &enc_state_->passes[idx_pass].ac_tokens[group_index], enc_state_->shared.quant_dc,
                                     enc_state_->shared.raw_quant_field, enc_state_->shared.block_ctx_map);
            }
        };
        RunOnPool(pool_, 0, shared.frame_dim.num_groups, tokenize_group_init, tokenize_group, "TokenizeGroup");

        *frame_header = shared.frame_header;
        return true;
    }

    Status ComputeJPEGTranscodingData(const jpeg::JPEGData& jpeg_data,
                                      ModularFrameEncoder* modular_frame_encoder,
                                      FrameHeader* frame_header) {
        PROFILER_ZONE("ComputeJPEGTranscodingData uninstrumented");
        PassesSharedState& shared = enc_state_->shared;

        frame_header->x_qm_scale = 2;
        frame_header->b_qm_scale = 2;

        FrameDimensions frame_dim = frame_header->ToFrameDimensions();

        const size_t xsize = frame_dim.xsize_padded;
        const size_t ysize = frame_dim.ysize_padded;
        const size_t xsize_blocks = frame_dim.xsize_blocks;
        const size_t ysize_blocks = frame_dim.ysize_blocks;

        // no-op chroma from luma
        shared.cmap = ColorCorrelationMap(xsize, ysize, false);
        shared.ac_strategy.FillDCT8();
        FillImage(uint8_t(0), &shared.epf_sharpness);

        enc_state_->coeffs.clear();
        enc_state_->coeffs.emplace_back(make_unique<ACImageT<int32_t> >(kGroupDim * kGroupDim, frame_dim.num_groups));

        // convert JPEG quantization table to a Quantizer object
        float dcquantization[3];
        std::vector<QuantEncoding> qe(DequantMatrices::kNum, QuantEncoding::Library(0));

        auto jpeg_c_map = JpegOrder(frame_header->color_transform, jpeg_data.components.size() == 1);

        std::vector<int> qt(192);
        for (size_t c = 0; c < 3; c++) {
            size_t jpeg_c = jpeg_c_map[c];
            const int* quant = jpeg_data.quant[jpeg_data.components[jpeg_c].quant_idx].values.data();

            dcquantization[c] = 255 * 8.0f / quant[0];
            for (size_t y = 0; y < 8; y++) {
                for (size_t x = 0; x < 8; x++) {
                    // JPEG XL transposes the DCT, JPEG doesn't.
                    qt[c * 64 + 8 * x + y] = quant[8 * y + x];
                }
            }
        }
        DequantMatricesSetCustomDC(&shared.matrices, dcquantization);
        float dcquantization_r[3] = {1.0f / dcquantization[0], 1.0f / dcquantization[1], 1.0f / dcquantization[2]};

        qe[AcStrategy::Type::DCT] = QuantEncoding::RAW(qt);
        DequantMatricesSetCustom(&shared.matrices, qe, modular_frame_encoder);

        // Ensure that InvGlobalScale() is 1.
        shared.quantizer = Quantizer(&shared.matrices, 1, kGlobalScaleDenom);
        // Recompute MulDC() and InvMulDC().
        shared.quantizer.RecomputeFromGlobalScale();

        // Per-block dequant scaling should be 1.
        FillImage(static_cast<int>(shared.quantizer.InvGlobalScale()), &shared.raw_quant_field);

        std::vector<int32_t> scaled_qtable(192);
        for (size_t c = 0; c < 3; c++) {
            for (size_t i = 0; i < 64; i++) {
                scaled_qtable[64 * c + i] = (1 << kCFLFixedPointPrecision) * qt[64 + i] / qt[64 * c + i];
            }
        }

        auto jpeg_row = [&](size_t c, size_t y) {
            return jpeg_data.components[jpeg_c_map[c]].coeffs.data() +
                   jpeg_data.components[jpeg_c_map[c]].width_in_blocks * kDCTBlockSize * y;
        };

        Image3F dc = Image3F(xsize_blocks, ysize_blocks);
        bool DCzero = (shared.frame_header.color_transform == ColorTransform::kYCbCr);
        // Compute chroma-from-luma for AC (doesn't seem to be useful for DC)
        if (frame_header->chroma_subsampling.Is444() && enc_state_->cparams.force_cfl_jpeg_recompression &&
            jpeg_data.components.size() == 3) {
            for (size_t c : {0, 2}) {
                ImageSB* map = (c == 0 ? &shared.cmap.ytox_map : &shared.cmap.ytob_map);
                const float kScale = kDefaultColorFactor;
                const int kOffset = 127;
                const float kBase = c == 0 ? shared.cmap.YtoXRatio(0) : shared.cmap.YtoBRatio(0);
                const float kZeroThresh =
                    kScale * kZeroBiasDefault[c] * 0.9999f; // just epsilon less for better rounding

                auto process_row = [&](int task, int thread) {
                    size_t ty = task;
                    int8_t* JXL_RESTRICT row_out = map->Row(ty);
                    for (size_t tx = 0; tx < map->xsize(); ++tx) {
                        const size_t y0 = ty * kColorTileDimInBlocks;
                        const size_t x0 = tx * kColorTileDimInBlocks;
                        const size_t y1 = std::min(frame_dim.ysize_blocks, (ty + 1) * kColorTileDimInBlocks);
                        const size_t x1 = std::min(frame_dim.xsize_blocks, (tx + 1) * kColorTileDimInBlocks);
                        int32_t d_num_zeros[257] = {0};
                        // TODO(veluca): this needs SIMD + fixed point adaptation, and/or
                        // conversion to the new CfL algorithm.
                        for (size_t y = y0; y < y1; ++y) {
                            const int16_t* JXL_RESTRICT row_m = jpeg_row(1, y);
                            const int16_t* JXL_RESTRICT row_s = jpeg_row(c, y);
                            for (size_t x = x0; x < x1; ++x) {
                                for (size_t coeffpos = 1; coeffpos < kDCTBlockSize; coeffpos++) {
                                    const float scaled_m = row_m[x * kDCTBlockSize + coeffpos] *
                                                           scaled_qtable[64 * c + coeffpos] *
                                                           (1.0f / (1 << kCFLFixedPointPrecision));
                                    const float scaled_s = kScale * row_s[x * kDCTBlockSize + coeffpos] +
                                                           (kOffset - kBase * kScale) * scaled_m;
                                    if (std::abs(scaled_m) > 1e-8f) {
                                        float from, to;
                                        if (scaled_m > 0) {
                                            from = (scaled_s - kZeroThresh) / scaled_m;
                                            to = (scaled_s + kZeroThresh) / scaled_m;
                                        } else {
                                            from = (scaled_s + kZeroThresh) / scaled_m;
                                            to = (scaled_s - kZeroThresh) / scaled_m;
                                        }
                                        if (from < 0.0f) {
                                            from = 0.0f;
                                        }
                                        if (to > 255.0f) {
                                            to = 255.0f;
                                        }
                                        // Instead of clamping the both values
                                        // we just check that range is sane.
                                        if (from <= to) {
                                            d_num_zeros[static_cast<int>(std::ceil(from))]++;
                                            d_num_zeros[static_cast<int>(std::floor(to + 1))]--;
                                        }
                                    }
                                }
                            }
                        }
                        int best = 0;
                        int32_t best_sum = 0;
                        FindIndexOfSumMaximum(d_num_zeros, 256, &best, &best_sum);
                        int32_t offset_sum = 0;
                        for (int i = 0; i < 256; ++i) {
                            if (i <= kOffset) {
                                offset_sum += d_num_zeros[i];
                            }
                        }
                        row_out[tx] = 0;
                        if (best_sum > offset_sum + 1) {
                            row_out[tx] = best - kOffset;
                        }
                    }
                };

                RunOnPool(pool_, 0, map->ysize(), ThreadPool::SkipInit(), process_row, "FindCorrelation");
            }
        }
        if (!frame_header->chroma_subsampling.Is444()) {
            ZeroFillImage(&dc);
            enc_state_->coeffs[0]->ZeroFill();
        }
        // JPEG DC is from -1024 to 1023.
        std::vector<size_t> dc_counts[3] = {};
        dc_counts[0].resize(2048);
        dc_counts[1].resize(2048);
        dc_counts[2].resize(2048);
        size_t total_dc[3] = {};
        for (size_t c : {1, 0, 2}) {
            if (jpeg_data.components.size() == 1 && c != 1) {
                enc_state_->coeffs[0]->ZeroFillPlane(c);
                ZeroFillImage(&dc.Plane(c));
                // Ensure no division by 0.
                dc_counts[c][1024] = 1;
                total_dc[c] = 1;
                continue;
            }
            size_t hshift = frame_header->chroma_subsampling.HShift(c);
            size_t vshift = frame_header->chroma_subsampling.VShift(c);
            ImageSB& map = (c == 0 ? shared.cmap.ytox_map : shared.cmap.ytob_map);
            for (size_t group_index = 0; group_index < frame_dim.num_groups; group_index++) {
                const size_t gx = group_index % frame_dim.xsize_groups;
                const size_t gy = group_index / frame_dim.xsize_groups;
                size_t offset = 0;
                int32_t* JXL_RESTRICT ac = enc_state_->coeffs[0]->PlaneRow(c, group_index, 0).ptr32;
                for (size_t by = gy * kGroupDimInBlocks; by < ysize_blocks && by < (gy + 1) * kGroupDimInBlocks; ++by) {
                    if ((by >> vshift) << vshift != by) continue;
                    const int16_t* JXL_RESTRICT inputjpeg = jpeg_row(c, by >> vshift);
                    const int16_t* JXL_RESTRICT inputjpegY = jpeg_row(1, by);
                    float* JXL_RESTRICT fdc = dc.PlaneRow(c, by >> vshift);
                    const int8_t* JXL_RESTRICT cm = map.ConstRow(by / kColorTileDimInBlocks);
                    for (size_t bx = gx * kGroupDimInBlocks; bx < xsize_blocks && bx < (gx + 1) * kGroupDimInBlocks;
                         ++bx) {
                        if ((bx >> hshift) << hshift != bx) continue;
                        size_t base = (bx >> hshift) * kDCTBlockSize;
                        int idc;
                        if (DCzero) {
                            idc = inputjpeg[base];
                        } else {
                            idc = inputjpeg[base] + 1024 / qt[c * 64];
                        }
                        dc_counts[c][std::min(static_cast<uint32_t>(idc + 1024), uint32_t(2047))]++;
                        total_dc[c]++;
                        fdc[bx >> hshift] = idc * dcquantization_r[c];
                        if (c == 1 || !enc_state_->cparams.force_cfl_jpeg_recompression ||
                            !frame_header->chroma_subsampling.Is444()) {
                            for (size_t y = 0; y < 8; y++) {
                                for (size_t x = 0; x < 8; x++) {
                                    ac[offset + y * 8 + x] = inputjpeg[base + x * 8 + y];
                                }
                            }
                        } else {
                            const int32_t scale = shared.cmap.RatioJPEG(cm[bx / kColorTileDimInBlocks]);

                            for (size_t y = 0; y < 8; y++) {
                                for (size_t x = 0; x < 8; x++) {
                                    int Y = inputjpegY[kDCTBlockSize * bx + x * 8 + y];
                                    int QChroma = inputjpeg[kDCTBlockSize * bx + x * 8 + y];
                                    // Fixed-point multiply of CfL scale with quant table ratio
                                    // first, and Y value second.
                                    int coeff_scale = (scale * scaled_qtable[64 * c + y * 8 + x] +
                                                       (1 << (kCFLFixedPointPrecision - 1))) >>
                                                      kCFLFixedPointPrecision;
                                    int cfl_factor = (Y * coeff_scale + (1 << (kCFLFixedPointPrecision - 1))) >>
                                                     kCFLFixedPointPrecision;
                                    int QCR = QChroma - cfl_factor;
                                    ac[offset + y * 8 + x] = QCR;
                                }
                            }
                        }
                        offset += 64;
                    }
                }
            }
        }

        auto& dct = enc_state_->shared.block_ctx_map.dc_thresholds;
        auto& num_dc_ctxs = enc_state_->shared.block_ctx_map.num_dc_ctxs;
        enc_state_->shared.block_ctx_map.num_dc_ctxs = 1;
        for (size_t i = 0; i < 3; i++) {
            dct[i].clear();
            int num_thresholds = (CeilLog2Nonzero(total_dc[i]) - 10) / 2;
            // up to 3 buckets per channel:
            // dark/medium/bright, yellow/unsat/blue, green/unsat/red
            num_thresholds = std::min(std::max(num_thresholds, 0), 2);
            size_t cumsum = 0;
            size_t cut = total_dc[i] / (num_thresholds + 1);
            for (int j = 0; j < 2048; j++) {
                cumsum += dc_counts[i][j];
                if (cumsum > cut) {
                    dct[i].push_back(j - 1025);
                    cut = total_dc[i] * (dct[i].size() + 1) / (num_thresholds + 1);
                }
            }
            num_dc_ctxs *= dct[i].size() + 1;
        }

        auto& ctx_map = enc_state_->shared.block_ctx_map.ctx_map;
        ctx_map.clear();
        ctx_map.resize(3 * kNumOrders * num_dc_ctxs, 0);

        int lbuckets = (dct[1].size() + 1);
        for (size_t i = 0; i < num_dc_ctxs; i++) {
            // up to 9 contexts for luma
            ctx_map[i] = i / lbuckets;
            // up to 3 contexts for chroma
            ctx_map[kNumOrders * num_dc_ctxs + i] = num_dc_ctxs / lbuckets + (i % lbuckets);
            ctx_map[2 * kNumOrders * num_dc_ctxs + i] = num_dc_ctxs / lbuckets + (i % lbuckets);
        }
        enc_state_->shared.block_ctx_map.num_ctxs = *std::max_element(ctx_map.begin(), ctx_map.end()) + 1;

        enc_state_->histogram_idx.resize(shared.frame_dim.num_groups);

        // disable DC frame for now
        shared.frame_header.UpdateFlag(false, FrameHeader::kUseDcFrame);
        auto compute_dc_coeffs = [&](int group_index, int /* thread */) {
            modular_frame_encoder->AddVarDCTDC(dc, group_index, /*nl_dc=*/false, enc_state_);
            modular_frame_encoder->AddACMetadata(group_index, /*jpeg_transcode=*/true, enc_state_);
        };
        RunOnPool(pool_, 0, shared.frame_dim.num_dc_groups, ThreadPool::SkipInit(), compute_dc_coeffs,
                  "Compute DC coeffs");

        // Must happen before WriteFrameHeader!
        shared.frame_header.UpdateFlag(true, FrameHeader::kSkipAdaptiveDCSmoothing);

        enc_state_->passes.resize(enc_state_->progressive_splitter.GetNumPasses());
        for (PassesEncoderState::PassData& pass : enc_state_->passes) {
            pass.ac_tokens.resize(shared.frame_dim.num_groups);
        }

        JXL_CHECK(enc_state_->passes.size() == 1); // skipping coeff splitting so need to have only one pass

        ComputeAllCoeffOrders(frame_dim);
        shared.num_histograms = 1;

        const auto tokenize_group_init = [&](const size_t num_threads) {
            group_caches_.resize(num_threads);
            return true;
        };
        const auto tokenize_group = [&](const int group_index, const int thread) {
            // Tokenize coefficients.
            const Rect rect = shared.BlockGroupRect(group_index);
            for (size_t idx_pass = 0; idx_pass < enc_state_->passes.size(); idx_pass++) {
                JXL_ASSERT(enc_state_->coeffs[idx_pass]->Type() == ACType::k32);
                const int32_t* JXL_RESTRICT ac_rows[3] = {
                    enc_state_->coeffs[idx_pass]->PlaneRow(0, group_index, 0).ptr32,
                    enc_state_->coeffs[idx_pass]->PlaneRow(1, group_index, 0).ptr32,
                    enc_state_->coeffs[idx_pass]->PlaneRow(2, group_index, 0).ptr32,
                };
                // Ensure group cache is initialized.
                group_caches_[thread].InitOnce();
                TokenizeCoefficients(&shared.coeff_orders[idx_pass * shared.coeff_order_size], rect, ac_rows,
                                     shared.ac_strategy, frame_header->chroma_subsampling,
                                     &group_caches_[thread].num_nzeroes,
                                     &enc_state_->passes[idx_pass].ac_tokens[group_index], enc_state_->shared.quant_dc,
                                     enc_state_->shared.raw_quant_field, enc_state_->shared.block_ctx_map);
            }
        };
        RunOnPool(pool_, 0, shared.frame_dim.num_groups, tokenize_group_init, tokenize_group, "TokenizeGroup");
        *frame_header = shared.frame_header;
        return true;
    }

    Status EncodeGlobalDCInfo(const FrameHeader& frame_header, BitWriter* writer) const {
        // Encode quantizer DC and global scale.
        JXL_RETURN_IF_ERROR(enc_state_->shared.quantizer.Encode(writer, kLayerQuant, aux_out_));
        EncodeBlockCtxMap(enc_state_->shared.block_ctx_map, writer, aux_out_);
        ColorCorrelationMapEncodeDC(&enc_state_->shared.cmap, writer, kLayerDC, aux_out_);
        return true;
    }

    Status EncodeGlobalACInfo(BitWriter* writer, ModularFrameEncoder* modular_frame_encoder) {
        JXL_RETURN_IF_ERROR(DequantMatricesEncode(&enc_state_->shared.matrices, writer, kLayerDequantTables, aux_out_,
                                                  modular_frame_encoder));
        if (enc_state_->cparams.speed_tier <= SpeedTier::kTortoise) {
            ClusterGroups(enc_state_);
        }
        size_t num_histo_bits = CeilLog2Nonzero(enc_state_->shared.frame_dim.num_groups);
        if (num_histo_bits != 0) {
            BitWriter::Allotment allotment(writer, num_histo_bits);
            writer->Write(num_histo_bits, enc_state_->shared.num_histograms - 1);
            ReclaimAndCharge(writer, &allotment, kLayerAC, aux_out_);
        }

        for (size_t i = 0; i < enc_state_->progressive_splitter.GetNumPasses(); i++) {
            // Encode coefficient orders.
            size_t order_bits = 0;
            JXL_RETURN_IF_ERROR(U32Coder::CanEncode(kOrderEnc, enc_state_->used_orders[i], &order_bits));
            BitWriter::Allotment allotment(writer, order_bits);
            JXL_CHECK(U32Coder::Write(kOrderEnc, enc_state_->used_orders[i], writer));
            ReclaimAndCharge(writer, &allotment, kLayerOrder, aux_out_);
            EncodeCoeffOrders(enc_state_->used_orders[i],
                              &enc_state_->shared.coeff_orders[i * enc_state_->shared.coeff_order_size], writer,
                              kLayerOrder, aux_out_);

            // Encode histograms.
            HistogramParams hist_params(enc_state_->cparams.speed_tier,
                                        enc_state_->shared.block_ctx_map.NumACContexts());
            if (enc_state_->cparams.speed_tier > SpeedTier::kTortoise) {
                hist_params.lz77_method = HistogramParams::LZ77Method::kNone;
            }
            if (enc_state_->cparams.decoding_speed_tier >= 1) {
                hist_params.max_histograms = 6;
            }
            BuildAndEncodeHistograms(
                hist_params, enc_state_->shared.num_histograms * enc_state_->shared.block_ctx_map.NumACContexts(),
                enc_state_->passes[i].ac_tokens, &enc_state_->passes[i].codes, &enc_state_->passes[i].context_map,
                writer, kLayerAC, aux_out_);
        }

        return true;
    }

    Status EncodeACGroup(size_t pass, size_t group_index, BitWriter* group_code, AuxOut* local_aux_out) {
        return EncodeGroupTokenizedCoefficients(group_index, pass, enc_state_->histogram_idx[group_index], *enc_state_,
                                                group_code, local_aux_out);
    }

    PassesEncoderState* State() { return enc_state_; }

    void ComputeAllCoeffOrders(const FrameDimensions& frame_dim) {
        PROFILER_FUNC;
        enc_state_->used_orders.resize(enc_state_->progressive_splitter.GetNumPasses());
        for (size_t i = 0; i < enc_state_->progressive_splitter.GetNumPasses(); i++) {
            // No coefficient reordering in Falcon or faster.
            if (enc_state_->cparams.speed_tier < SpeedTier::kFalcon) {
                enc_state_->used_orders[i] =
                    ComputeUsedOrders(enc_state_->cparams.speed_tier, enc_state_->shared.ac_strategy,
                                      Rect(enc_state_->shared.raw_quant_field));
            }
            ComputeCoeffOrder(enc_state_->cparams.speed_tier, *enc_state_->coeffs[i], enc_state_->shared.ac_strategy,
                              frame_dim, enc_state_->used_orders[i],
                              &enc_state_->shared.coeff_orders[i * enc_state_->shared.coeff_order_size]);
        }
    }

    std::vector<EncCache>& get_group_cashes() { return group_caches_; }

   private:
    template <typename V, typename R>
    static inline void FindIndexOfSumMaximum(const V* array, const size_t len, R* idx, V* sum) {
        JXL_ASSERT(len > 0);
        V maxval = 0;
        V val = 0;
        R maxidx = 0;
        for (size_t i = 0; i < len; ++i) {
            val += array[i];
            if (val > maxval) {
                maxval = val;
                maxidx = i;
            }
        }
        *idx = maxidx;
        *sum = maxval;
    }

    PassesEncoderState* JXL_RESTRICT enc_state_;
    ThreadPool* pool_;
    AuxOut* aux_out_;
    std::vector<EncCache> group_caches_;
};
} // namespace jxl
#endif
