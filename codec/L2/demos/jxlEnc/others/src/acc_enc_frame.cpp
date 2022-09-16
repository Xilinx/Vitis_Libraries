// Copyright (c) the JPEG XL Project Authors. All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#include "acc_host.hpp"
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
#include "lib/jxl/enc_bit_writer.h"
#include "lib/jxl/enc_cache.h"
#include "acc_enc_chroma_from_luma.hpp"
#include "lib/jxl/enc_coeff_order.h"
#include "lib/jxl/enc_context_map.h"
#include "lib/jxl/enc_entropy_coder.h"
#include "acc_enc_group.hpp"
#include "lib/jxl/enc_modular.h"
#include "lib/jxl/enc_noise.h"
#include "lib/jxl/enc_params.h"
#include "lib/jxl/enc_patch_dictionary.h"
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

namespace jxl {
namespace {

uint64_t FrameFlagsFromParams(const CompressParams& cparams) {
    uint64_t flags = 0;

    const float dist = cparams.butteraugli_distance;

    // We don't add noise at low butteraugli distances because the original
    // noise is stored within the compressed image and adding noise makes things
    // worse.
    if (ApplyOverride(cparams.noise, dist >= kMinButteraugliForNoise) || cparams.photon_noise_iso > 0) {
        flags |= FrameHeader::kNoise;
    }

    if (cparams.progressive_dc > 0 && cparams.modular_mode == false) {
        flags |= FrameHeader::kUseDcFrame;
    }

    return flags;
}

Status LoopFilterFromParams(const CompressParams& cparams, FrameHeader* JXL_RESTRICT frame_header) {
    LoopFilter* loop_filter = &frame_header->loop_filter;

    // Gaborish defaults to enabled in Hare or slower.
    loop_filter->gab = ApplyOverride(cparams.gaborish, cparams.speed_tier <= SpeedTier::kHare &&
                                                           frame_header->encoding == FrameEncoding::kVarDCT &&
                                                           cparams.decoding_speed_tier < 4);

    if (cparams.epf != -1) {
        loop_filter->epf_iters = cparams.epf;
    } else {
        if (frame_header->encoding == FrameEncoding::kModular) {
            loop_filter->epf_iters = 0;
        } else {
            constexpr float kThresholds[3] = {0.7, 1.5, 4.0};
            loop_filter->epf_iters = 0;
            if (cparams.decoding_speed_tier < 3) {
                for (size_t i = cparams.decoding_speed_tier == 2 ? 1 : 0; i < 3; i++) {
                    if (cparams.butteraugli_distance >= kThresholds[i]) {
                        loop_filter->epf_iters++;
                    }
                }
            }
        }
    }
    // Strength of EPF in modular mode.
    if (frame_header->encoding == FrameEncoding::kModular && cparams.quality_pair.first < 100) {
        // TODO(veluca): this formula is nonsense.
        loop_filter->epf_sigma_for_modular = 20.0f * (1.0f - cparams.quality_pair.first / 100);
    }
    if (frame_header->encoding == FrameEncoding::kModular && cparams.lossy_palette) {
        loop_filter->epf_sigma_for_modular = 1.0f;
    }

    return true;
}

Status MakeFrameHeader(const CompressParams& cparams,
                       const ProgressiveSplitter& progressive_splitter,
                       const FrameInfo& frame_info,
                       const ImageBundle& ib,
                       FrameHeader* JXL_RESTRICT frame_header) {
    frame_header->nonserialized_is_preview = frame_info.is_preview;
    frame_header->is_last = frame_info.is_last;
    frame_header->save_before_color_transform = frame_info.save_before_color_transform;
    frame_header->frame_type = frame_info.frame_type;
    frame_header->name = ib.name;

    progressive_splitter.InitPasses(&frame_header->passes);

    if (cparams.modular_mode) {
        frame_header->encoding = FrameEncoding::kModular;
        frame_header->group_size_shift = cparams.modular_group_size_shift;
    }

    frame_header->chroma_subsampling = ib.chroma_subsampling;
    if (ib.IsJPEG()) {
        // we are transcoding a JPEG, so we don't get to choose
        frame_header->encoding = FrameEncoding::kVarDCT;
        frame_header->color_transform = ib.color_transform;
    } else {
        frame_header->color_transform = cparams.color_transform;
        if (!cparams.modular_mode &&
            (frame_header->chroma_subsampling.MaxHShift() != 0 || frame_header->chroma_subsampling.MaxVShift() != 0)) {
            return JXL_FAILURE(
                "Chroma subsampling is not supported in VarDCT mode when not "
                "recompressing JPEGs");
        }
    }

    frame_header->flags = FrameFlagsFromParams(cparams);
    // Noise is not supported in the Modular encoder for now.
    if (frame_header->encoding != FrameEncoding::kVarDCT) {
        frame_header->UpdateFlag(false, FrameHeader::Flags::kNoise);
    }

    JXL_RETURN_IF_ERROR(LoopFilterFromParams(cparams, frame_header));

    frame_header->dc_level = frame_info.dc_level;
    if (frame_header->dc_level > 2) {
        // With 3 or more progressive_dc frames, the implementation does not yet
        // work, see enc_cache.cc.
        return JXL_FAILURE("progressive_dc > 2 is not yet supported");
    }
    if (cparams.progressive_dc > 0 && (cparams.ec_resampling != 1 || cparams.resampling != 1)) {
        return JXL_FAILURE("Resampling not supported with DC frames");
    }
    if (cparams.resampling != 1 && cparams.resampling != 2 && cparams.resampling != 4 && cparams.resampling != 8) {
        return JXL_FAILURE("Invalid resampling factor");
    }
    if (cparams.ec_resampling != 1 && cparams.ec_resampling != 2 && cparams.ec_resampling != 4 &&
        cparams.ec_resampling != 8) {
        return JXL_FAILURE("Invalid ec_resampling factor");
    }
    // Resized frames.
    if (frame_info.frame_type != FrameType::kDCFrame) {
        frame_header->frame_origin = ib.origin;
        size_t ups = 1;
        if (cparams.already_downsampled) ups = cparams.resampling;
        frame_header->frame_size.xsize = ib.xsize() * ups;
        frame_header->frame_size.ysize = ib.ysize() * ups;
        if (ib.origin.x0 != 0 || ib.origin.y0 != 0 || frame_header->frame_size.xsize != frame_header->default_xsize() ||
            frame_header->frame_size.ysize != frame_header->default_ysize()) {
            frame_header->custom_size_or_origin = true;
        }
    }
    // Upsampling.
    frame_header->upsampling = cparams.resampling;
    const std::vector<ExtraChannelInfo>& extra_channels = frame_header->nonserialized_metadata->m.extra_channel_info;
    frame_header->extra_channel_upsampling.clear();
    frame_header->extra_channel_upsampling.resize(extra_channels.size(), cparams.ec_resampling);
    frame_header->save_as_reference = frame_info.save_as_reference;

    // Set blending-related information.
    if (ib.blend || frame_header->custom_size_or_origin) {
        // Set blend_channel to the first alpha channel. These values are only
        // encoded in case a blend mode involving alpha is used and there are more
        // than one extra channels.
        size_t index = 0;
        if (extra_channels.size() > 1) {
            for (size_t i = 0; i < extra_channels.size(); i++) {
                if (extra_channels[i].type == ExtraChannel::kAlpha) {
                    index = i;
                    break;
                }
            }
        }
        frame_header->blending_info.alpha_channel = index;
        frame_header->blending_info.mode = ib.blend ? ib.blendmode : BlendMode::kReplace;
        // previous frames are saved with ID 1.
        frame_header->blending_info.source = 1;
        for (size_t i = 0; i < extra_channels.size(); i++) {
            frame_header->extra_channel_blending_info[i].alpha_channel = index;
            BlendMode default_blend = ib.blendmode;
            if (extra_channels[i].type != ExtraChannel::kBlack && i != index) {
                // K needs to be blended, spot colors and other stuff gets added
                default_blend = BlendMode::kAdd;
            }
            frame_header->extra_channel_blending_info[i].mode = ib.blend ? default_blend : BlendMode::kReplace;
            frame_header->extra_channel_blending_info[i].source = 1;
        }
    }

    frame_header->animation_frame.duration = ib.duration;

    // TODO(veluca): timecode.

    return true;
}

} // namespace

Status EncodeFrame(const CompressParams& cparams_orig,
                   const FrameInfo& frame_info,
                   const CodecMetadata* metadata,
                   const ImageBundle& ib,
                   PassesEncoderState* passes_enc_state,
                   ThreadPool* pool,
                   BitWriter* writer,
                   AuxOut* aux_out,
                   std::string xclbinPath) {
    ib.VerifyMetadata();
    passes_enc_state->special_frames.clear();

    CompressParams cparams = cparams_orig;

    if (cparams.progressive_dc < 0) {
        if (cparams.progressive_dc != -1) {
            return JXL_FAILURE("Invalid progressive DC setting value (%d)", cparams.progressive_dc);
        }
        cparams.progressive_dc = 0;
        // Enable progressive_dc for lower qualities.
        if (cparams.butteraugli_distance >= kMinButteraugliDistanceForProgressiveDc) {
            cparams.progressive_dc = 1;
        }
    }
    if (cparams.ec_resampling < cparams.resampling) {
        cparams.ec_resampling = cparams.resampling;
    }
    if (cparams.resampling > 1) cparams.progressive_dc = 0;

    if (frame_info.dc_level + cparams.progressive_dc > 4) {
        return JXL_FAILURE("Too many levels of progressive DC");
    }

    if (cparams.butteraugli_distance != 0 && cparams.butteraugli_distance < kMinButteraugliDistance) {
        return JXL_FAILURE("Butteraugli distance is too low (%f)", cparams.butteraugli_distance);
    }
    if (cparams.butteraugli_distance > 0.9f && cparams.modular_mode == false && cparams.quality_pair.first == 100) {
        // in case the color image is lossy, make the alpha slightly lossy too
        cparams.quality_pair.first = std::max(90.f, 99.f - 0.3f * cparams.butteraugli_distance);
    }

    if (ib.IsJPEG()) {
        cparams.gaborish = Override::kOff;
        cparams.epf = 0;
        cparams.modular_mode = false;
    }

    if (ib.xsize() == 0 || ib.ysize() == 0) return JXL_FAILURE("Empty image");

    // Assert that this metadata is correctly set up for the compression params,
    // this should have been done by enc_file.cc
    JXL_ASSERT(metadata->m.xyb_encoded == (cparams.color_transform == ColorTransform::kXYB));
    std::unique_ptr<FrameHeader> frame_header = jxl::make_unique<FrameHeader>(metadata);
    JXL_RETURN_IF_ERROR(
        MakeFrameHeader(cparams, passes_enc_state->progressive_splitter, frame_info, ib, frame_header.get()));
    // Check that if the codestream header says xyb_encoded, the color_transform
    // matches the requirement. This is checked from the cparams here, even though
    // optimally we'd be able to check this against what has actually been written
    // in the main codestream header, but since ib is a const object and the data
    // written to the main codestream header is (in modified form) in ib, the
    // encoder cannot indicate this fact in the ib's metadata.
    if (cparams_orig.color_transform == ColorTransform::kXYB) {
        if (frame_header->color_transform != ColorTransform::kXYB) {
            return JXL_FAILURE(
                "The color transform of frames must be xyb if the codestream is xyb "
                "encoded");
        }
    } else {
        if (frame_header->color_transform == ColorTransform::kXYB) {
            return JXL_FAILURE(
                "The color transform of frames cannot be xyb if the codestream is "
                "not xyb encoded");
        }
    }

    FrameDimensions frame_dim = frame_header->ToFrameDimensions();

    const size_t num_groups = frame_dim.num_groups;

    Image3F opsin;
    const ColorEncoding& c_linear = ColorEncoding::LinearSRGB(ib.IsGray());
    std::unique_ptr<ImageMetadata> metadata_linear = jxl::make_unique<ImageMetadata>();
    metadata_linear->xyb_encoded = (cparams.color_transform == ColorTransform::kXYB);
    metadata_linear->color_encoding = c_linear;
    ImageBundle linear_storage(metadata_linear.get());

    std::vector<AuxOut> aux_outs;
    // LossyFrameEncoder stores a reference to a std::function<Status(size_t)>
    // so we need to keep the std::function<Status(size_t)> being referenced
    // alive while lossy_frame_encoder is used. We could make resize_aux_outs a
    // lambda type by making LossyFrameEncoder a template instead, but this is
    // simpler.
    const std::function<Status(size_t)> resize_aux_outs = [&aux_outs, aux_out](size_t num_threads) -> Status {
        if (aux_out != nullptr) {
            size_t old_size = aux_outs.size();
            for (size_t i = num_threads; i < old_size; i++) {
                aux_out->Assimilate(aux_outs[i]);
            }
            aux_outs.resize(num_threads);
            // Each thread needs these INPUTS. Don't copy the entire AuxOut
            // because it may contain stats which would be Assimilated multiple
            // times below.
            for (size_t i = old_size; i < aux_outs.size(); i++) {
                aux_outs[i].dump_image = aux_out->dump_image;
                aux_outs[i].debug_prefix = aux_out->debug_prefix;
            }
        }
        return true;
    };

    LossyFrameEncoder lossy_frame_encoder(cparams, *frame_header, passes_enc_state, pool, aux_out);
    std::unique_ptr<ModularFrameEncoder> modular_frame_encoder =
        jxl::make_unique<ModularFrameEncoder>(*frame_header, cparams);

    const std::vector<ImageF>* extra_channels = &ib.extra_channels();
    std::vector<ImageF> extra_channels_storage;
    const ImageBundle* JXL_RESTRICT ib_or_linear;

    if (ib.IsJPEG()) {
        JXL_RETURN_IF_ERROR(lossy_frame_encoder.ComputeJPEGTranscodingData(*ib.jpeg_data, modular_frame_encoder.get(),
                                                                           frame_header.get()));
    } else if (!lossy_frame_encoder.State()->heuristics->HandlesColorConversion(cparams, ib) ||
               frame_header->encoding != FrameEncoding::kVarDCT) {
        acc_host(xclbinPath, opsin, lossy_frame_encoder, ib_or_linear, pool, modular_frame_encoder, writer, aux_out,
                 frame_header, frame_info, cparams, &ib.extra_channels(), passes_enc_state, frame_dim, num_groups, ib,
                 aux_outs, resize_aux_outs);
        if (frame_header->encoding == FrameEncoding::kVarDCT) {
        } else if (frame_header->upsampling != 1 && !cparams.already_downsampled) {
            // In VarDCT mode, LossyFrameHeuristics takes care of running downsampling
            // after noise, if necessary.
            DownsampleImage(&opsin, frame_header->upsampling);
        }

    } else {
        JXL_RETURN_IF_ERROR(lossy_frame_encoder.ComputeEncodingData(&ib, &opsin, pool, modular_frame_encoder.get(),
                                                                    writer, frame_header.get()));
    }

    if (!ib.IsJPEG() && (!lossy_frame_encoder.State()->heuristics->HandlesColorConversion(cparams, ib) ||
                         frame_header->encoding != FrameEncoding::kVarDCT) &&
        frame_header->encoding == FrameEncoding::kVarDCT) {
    } else {
        if (cparams.ec_resampling != 1 && !cparams.already_downsampled) {
            extra_channels = &extra_channels_storage;
            for (size_t i = 0; i < ib.extra_channels().size(); i++) {
                extra_channels_storage.emplace_back(CopyImage(ib.extra_channels()[i]));
                DownsampleImage(&extra_channels_storage.back(), cparams.ec_resampling);
            }
        }
        // needs to happen *AFTER* VarDCT-ComputeEncodingData.
        JXL_RETURN_IF_ERROR(modular_frame_encoder->ComputeEncodingData(
            *frame_header, *ib.metadata(), &opsin, *extra_channels, lossy_frame_encoder.State(), pool, aux_out,
            /* do_color=*/frame_header->encoding == FrameEncoding::kModular));

        writer->AppendByteAligned(lossy_frame_encoder.State()->special_frames);
        frame_header->UpdateFlag(lossy_frame_encoder.State()->shared.image_features.patches.HasAny(),
                                 FrameHeader::kPatches);
        frame_header->UpdateFlag(lossy_frame_encoder.State()->shared.image_features.splines.HasAny(),
                                 FrameHeader::kSplines);
        JXL_RETURN_IF_ERROR(WriteFrameHeader(*frame_header, writer, aux_out));

        const size_t num_passes = passes_enc_state->progressive_splitter.GetNumPasses();

        // DC global info + DC groups + AC global info + AC groups *
        // num_passes.
        const bool has_ac_global = true;
        std::vector<BitWriter> group_codes(
            NumTocEntries(frame_dim.num_groups, frame_dim.num_dc_groups, num_passes, has_ac_global));
        const size_t global_ac_index = frame_dim.num_dc_groups + 1;
        const bool is_small_image = frame_dim.num_groups == 1 && num_passes == 1;
        const auto get_output = [&](const size_t index) { return &group_codes[is_small_image ? 0 : index]; };
        auto ac_group_code = [&](size_t pass, size_t group) {
            return get_output(AcGroupIndex(pass, group, frame_dim.num_groups, frame_dim.num_dc_groups, has_ac_global));
        };

        if (frame_header->flags & FrameHeader::kPatches) {
            PatchDictionaryEncoder::Encode(lossy_frame_encoder.State()->shared.image_features.patches, get_output(0),
                                           kLayerDictionary, aux_out);
        }

        if (frame_header->flags & FrameHeader::kSplines) {
            EncodeSplines(lossy_frame_encoder.State()->shared.image_features.splines, get_output(0), kLayerSplines,
                          HistogramParams(), aux_out);
        }

        if (frame_header->flags & FrameHeader::kNoise) {
            EncodeNoise(lossy_frame_encoder.State()->shared.image_features.noise_params, get_output(0), kLayerNoise,
                        aux_out);
        }

        JXL_RETURN_IF_ERROR(DequantMatricesEncodeDC(&lossy_frame_encoder.State()->shared.matrices, get_output(0),
                                                    kLayerDequantTables, aux_out));
        if (frame_header->encoding == FrameEncoding::kVarDCT) {
            JXL_RETURN_IF_ERROR(lossy_frame_encoder.EncodeGlobalDCInfo(*frame_header, get_output(0)));
        }
        JXL_RETURN_IF_ERROR(modular_frame_encoder->EncodeGlobalInfo(get_output(0), aux_out));
        JXL_RETURN_IF_ERROR(modular_frame_encoder->EncodeStream(get_output(0), aux_out, kLayerModularGlobal,
                                                                ModularStreamId::Global()));

        const auto process_dc_group = [&](const int group_index, const int thread) {
            AuxOut* my_aux_out = aux_out ? &aux_outs[thread] : nullptr;
            BitWriter* output = get_output(group_index + 1);
            if (frame_header->encoding == FrameEncoding::kVarDCT && !(frame_header->flags & FrameHeader::kUseDcFrame)) {
                BitWriter::Allotment allotment(output, 2);
                output->Write(2, modular_frame_encoder->extra_dc_precision[group_index]);
                ReclaimAndCharge(output, &allotment, kLayerDC, my_aux_out);
                JXL_CHECK(modular_frame_encoder->EncodeStream(output, my_aux_out, kLayerDC,
                                                              ModularStreamId::VarDCTDC(group_index)));
            }
            JXL_CHECK(modular_frame_encoder->EncodeStream(output, my_aux_out, kLayerModularDcGroup,
                                                          ModularStreamId::ModularDC(group_index)));
            if (frame_header->encoding == FrameEncoding::kVarDCT) {
                const Rect& rect = lossy_frame_encoder.State()->shared.DCGroupRect(group_index);
                size_t nb_bits = CeilLog2Nonzero(rect.xsize() * rect.ysize());
                if (nb_bits != 0) {
                    BitWriter::Allotment allotment(output, nb_bits);
                    output->Write(nb_bits, modular_frame_encoder->ac_metadata_size[group_index] - 1);
                    ReclaimAndCharge(output, &allotment, kLayerControlFields, my_aux_out);
                }
                JXL_CHECK(modular_frame_encoder->EncodeStream(output, my_aux_out, kLayerControlFields,
                                                              ModularStreamId::ACMetadata(group_index)));
            }
        };
        RunOnPool(pool, 0, frame_dim.num_dc_groups, resize_aux_outs, process_dc_group, "EncodeDCGroup");

        if (frame_header->encoding == FrameEncoding::kVarDCT) {
            JXL_RETURN_IF_ERROR(
                lossy_frame_encoder.EncodeGlobalACInfo(get_output(global_ac_index), modular_frame_encoder.get()));
        }

        std::atomic<int> num_errors{0};
        const auto process_group = [&](const int group_index, const int thread) {
            AuxOut* my_aux_out = aux_out ? &aux_outs[thread] : nullptr;

            for (size_t i = 0; i < num_passes; i++) {
                if (frame_header->encoding == FrameEncoding::kVarDCT) {
                    if (!lossy_frame_encoder.EncodeACGroup(i, group_index, ac_group_code(i, group_index), my_aux_out)) {
                        num_errors.fetch_add(1, std::memory_order_relaxed);
                        return;
                    }
                }
                // Write all modular encoded data (color?, alpha, depth, extra channels)
                if (!modular_frame_encoder->EncodeStream(ac_group_code(i, group_index), my_aux_out,
                                                         kLayerModularAcGroup,
                                                         ModularStreamId::ModularAC(group_index, i))) {
                    num_errors.fetch_add(1, std::memory_order_relaxed);
                    return;
                }
            }
        };
        RunOnPool(pool, 0, num_groups, resize_aux_outs, process_group, "EncodeGroupCoefficients");

        // Resizing aux_outs to 0 also Assimilates the array.
        static_cast<void>(resize_aux_outs(0));
        JXL_RETURN_IF_ERROR(num_errors.load(std::memory_order_relaxed) == 0);

        for (BitWriter& bw : group_codes) {
            bw.ZeroPadToByte(); // end of group.
        }

        std::vector<coeff_order_t>* permutation_ptr = nullptr;
        std::vector<coeff_order_t> permutation;
        if (cparams.centerfirst && !(num_passes == 1 && num_groups == 1)) {
            permutation_ptr = &permutation;
            // Don't permute global DC/AC or DC.
            permutation.resize(global_ac_index + 1);
            std::iota(permutation.begin(), permutation.end(), 0);
            std::vector<coeff_order_t> ac_group_order(num_groups);
            std::iota(ac_group_order.begin(), ac_group_order.end(), 0);
            size_t group_dim = frame_dim.group_dim;

            // The center of the image is either given by parameters or chosen
            // to be the middle of the image by default if center_x, center_y resp.
            // are not provided.

            int64_t imag_cx;
            if (cparams.center_x != static_cast<size_t>(-1)) {
                JXL_RETURN_IF_ERROR(cparams.center_x < ib.xsize());
                imag_cx = cparams.center_x;
            } else {
                imag_cx = ib.xsize() / 2;
            }

            int64_t imag_cy;
            if (cparams.center_y != static_cast<size_t>(-1)) {
                JXL_RETURN_IF_ERROR(cparams.center_y < ib.ysize());
                imag_cy = cparams.center_y;
            } else {
                imag_cy = ib.ysize() / 2;
            }

            // The center of the group containing the center of the image.
            int64_t cx = (imag_cx / group_dim) * group_dim + group_dim / 2;
            int64_t cy = (imag_cy / group_dim) * group_dim + group_dim / 2;
            // This identifies in what area of the central group the center of the
            // image
            // lies in.
            double direction = -std::atan2(imag_cy - cy, imag_cx - cx);
            // This identifies the side of the central group the center of the image
            // lies closest to. This can take values 0, 1, 2, 3 corresponding to left,
            // bottom, right, top.
            int64_t side = std::fmod((direction + 5 * kPi / 4), 2 * kPi) * 2 / kPi;
            auto get_distance_from_center = [&](size_t gid) {
                Rect r = passes_enc_state->shared.GroupRect(gid);
                int64_t gcx = r.x0() + group_dim / 2;
                int64_t gcy = r.y0() + group_dim / 2;
                int64_t dx = gcx - cx;
                int64_t dy = gcy - cy;
                // The angle is determined by taking atan2 and adding an appropriate
                // starting point depending on the side we want to start on.
                double angle = std::remainder(std::atan2(dy, dx) + kPi / 4 + side * (kPi / 2), 2 * kPi);
                // Concentric squares in clockwise order.
                return std::make_pair(std::max(std::abs(dx), std::abs(dy)), angle);
            };
            std::sort(ac_group_order.begin(), ac_group_order.end(), [&](coeff_order_t a, coeff_order_t b) {
                return get_distance_from_center(a) < get_distance_from_center(b);
            });
            std::vector<coeff_order_t> inv_ac_group_order(ac_group_order.size(), 0);
            for (size_t i = 0; i < ac_group_order.size(); i++) {
                inv_ac_group_order[ac_group_order[i]] = i;
            }
            for (size_t i = 0; i < num_passes; i++) {
                size_t pass_start = permutation.size();
                for (coeff_order_t v : inv_ac_group_order) {
                    permutation.push_back(pass_start + v);
                }
            }
            std::vector<BitWriter> new_group_codes(group_codes.size());
            for (size_t i = 0; i < permutation.size(); i++) {
                new_group_codes[permutation[i]] = std::move(group_codes[i]);
            }
            group_codes = std::move(new_group_codes);
        }

        JXL_RETURN_IF_ERROR(WriteGroupOffsets(group_codes, permutation_ptr, writer, aux_out));
        writer->AppendByteAligned(group_codes);
        writer->ZeroPadToByte(); // end of frame.
    }
    return true;
}

} // namespace jxl
