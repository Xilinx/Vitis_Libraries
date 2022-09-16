// Copyright (c) the JPEG XL Project Authors. All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#ifndef ACC_INIT_HISTOGRAM_CPP
#define ACC_INIT_HISTOGRAM_CPP

#include "acc_init_histogram.hpp"

namespace jxl {
bool acc_InitHistogram(std::vector<Histogram>& histograms, std::vector<std::vector<Token> >& tokens) {
    size_t total_tokens = 0;
    HybridUintConfig uint_config; //  Default config for clustering.

    for (size_t i = 0; i < tokens.size(); ++i) {
        for (size_t j = 0; j < tokens[i].size(); ++j) {
            const Token token = tokens[i][j];
            total_tokens++;
            uint32_t tok, nbits, bits;
            uint_config.Encode(token.value, &tok, &nbits, &bits);
            tok += 0;
            histograms[token.context].Add(tok);
        }
    }
    bool use_prefix_code = total_tokens < 100;
    return false;
}

void acc_ANSinitHistogram(LossyFrameEncoder& lossy_frame_encoder,
                          std::unique_ptr<FrameHeader>& frame_header,

                          std::vector<HistogramParams>& params,
                          bool do_once[5],

                          std::vector<std::vector<Token> >& tokens0,
                          std::vector<std::vector<Token> >& tokens1,
                          std::vector<std::vector<Token> >& tokens2,
                          std::vector<std::vector<Token> >& tokens3,

                          char* do_prefix_out,
                          std::vector<uint32_t>& largest_idx,
                          std::vector<std::vector<uint32_t> >& nonempty_histograms,
                          std::vector<std::vector<Histogram> >& histograms_) {
    PassesEncoderState* JXL_RESTRICT enc_state_ = lossy_frame_encoder.State();
    PassesSharedState& shared = enc_state_->shared;
    std::vector<EncCache>& group_caches_ = lossy_frame_encoder.get_group_cashes();

    group_caches_.resize(1);
    for (int group_index = 0; group_index < shared.frame_dim.num_groups; group_index++) {
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
            group_caches_[0].InitOnce();
            TokenizeCoefficients(&shared.coeff_orders[idx_pass * shared.coeff_order_size], rect, ac_rows,
                                 shared.ac_strategy, frame_header->chroma_subsampling, &group_caches_[0].num_nzeroes,
                                 &enc_state_->passes[idx_pass].ac_tokens[group_index], enc_state_->shared.quant_dc,
                                 enc_state_->shared.raw_quant_field, enc_state_->shared.block_ctx_map);
        }
    };

    for (int i = 0; i < 5; i++) {
        if (!do_once[i]) continue;

        std::vector<std::vector<Token> >& tokens = tokens0;
        if (i == 0) {
            tokens = tokens0;
        } else if (i == 1) {
            tokens = tokens1;
        } else if (i == 2) {
            tokens = tokens2;
        } else if (i == 3) {
            tokens = tokens3;
        } else if (i == 4) {
            tokens = enc_state_->passes[0].ac_tokens;
        }

        bool use_prefix_code = acc_InitHistogram(histograms_[i], tokens);

        do_prefix_out[i] = (char)use_prefix_code;

        int count = 0;
        for (int j = 0; j < histograms_[i].size(); j++) {
            count += histograms_[i][j].data_.size();
        }

        if (histograms_[i].size() > 1) {
            size_t max_histograms = std::min(kClustersLimit, params[i].max_histograms);

            largest_idx[i] = 0;
            nonempty_histograms[i].reserve(histograms_[i].size());
            for (size_t j = 0; j < histograms_[i].size(); j++) {
                if (histograms_[i][j].total_count_ == 0) continue;

                if (histograms_[i][j].total_count_ > histograms_[i][largest_idx[i]].total_count_) {
                    largest_idx[i] = j;
                }
                nonempty_histograms[i].push_back(j);
            }

            largest_idx[i] = std::find(nonempty_histograms[i].begin(), nonempty_histograms[i].end(), largest_idx[i]) -
                             nonempty_histograms[i].begin();
        }
    }
}
} // namespace jxl

#endif
