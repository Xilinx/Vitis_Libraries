// Copyright (c) the JPEG XL Project Authors. All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#ifndef HLS_KERNEL3_CPP
#define HLS_KERNEL3_CPP

#include "acc_phase3.hpp"

namespace jxl {

Status acc_phase3(std::string xclbinPath,
                  Image3F& opsin,
                  LossyFrameEncoder& lossy_frame_encoder,
                  std::unique_ptr<ModularFrameEncoder>& modular_frame_encoder,
                  CompressParams cparams,
                  std::unique_ptr<FrameHeader>& frame_header,
                  PassesEncoderState* passes_enc_state,
                  FrameDimensions frame_dim,
                  BitWriter* writer,
                  const size_t num_groups,
                  AuxOut* aux_out,
                  ThreadPool* pool,
                  std::vector<AuxOut>& aux_outs,
                  const ImageBundle& ib,
                  const std::function<Status(size_t)>& resize_aux_outs) {
    std::vector<EncCache>& group_caches_ = lossy_frame_encoder.get_group_cashes();
    PassesEncoderState* JXL_RESTRICT enc_state_ = lossy_frame_encoder.State();
    PassesSharedState& shared = enc_state_->shared;

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
    RunOnPool(pool, 0, shared.frame_dim.num_groups, tokenize_group_init, tokenize_group, "TokenizeGroup");

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
    JXL_RETURN_IF_ERROR(
        modular_frame_encoder->EncodeStream(get_output(0), aux_out, kLayerModularGlobal, ModularStreamId::Global()));

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
            if (!modular_frame_encoder->EncodeStream(ac_group_code(i, group_index), my_aux_out, kLayerModularAcGroup,
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
    return true;
}
} // namespace jxl

#endif
