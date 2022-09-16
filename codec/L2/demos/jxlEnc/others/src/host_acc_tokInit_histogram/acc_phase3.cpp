// Copyright (c) the JPEG XL Project Authors. All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#ifndef ACC_PHASE3_CPP
#define ACC_PHASE3_CPP

#include "acc_phase3.hpp"

#include <sys/time.h>

#include "acc_cluster_histogram.hpp"
#include "acc_store_encode_data.hpp"
#include "lib/jxl/lehmer_code.h"
#include "host_tokinit_histogram.hpp"

void test(int* in, int* out);

inline int tvdiff(struct timeval* tv0, struct timeval* tv1) {
    return (tv1->tv_sec - tv0->tv_sec) * 1000000 + (tv1->tv_usec - tv0->tv_usec);
}

namespace jxl {
namespace {
size_t IndexOf(const std::vector<uint8_t>& v, uint8_t value) {
    size_t i = 0;
    for (; i < v.size(); ++i) {
        if (v[i] == value) return i;
    }
    return i;
}

void MoveToFront(std::vector<uint8_t>* v, size_t index) {
    uint8_t value = (*v)[index];
    for (size_t i = index; i != 0; --i) {
        (*v)[i] = (*v)[i - 1];
    }
    (*v)[0] = value;
}

std::vector<uint8_t> MoveToFrontTransform(const std::vector<uint8_t>& v) {
    if (v.empty()) return v;
    uint8_t max_value = *std::max_element(v.begin(), v.end());
    std::vector<uint8_t> mtf(max_value + 1);
    for (size_t i = 0; i <= max_value; ++i) mtf[i] = i;
    std::vector<uint8_t> result(v.size());
    for (size_t i = 0; i < v.size(); ++i) {
        size_t index = IndexOf(mtf, v[i]);
        JXL_ASSERT(index < mtf.size());
        result[i] = static_cast<uint8_t>(index);
        MoveToFront(&mtf, index);
    }
    return result;
}
} // namespace

namespace {

void acc_TokenizePermutation(const coeff_order_t* JXL_RESTRICT order,
                             size_t skip,
                             size_t size,
                             std::vector<Token>* tokens) {
    std::vector<LehmerT> lehmer(size);
    std::vector<uint32_t> temp(size + 1);
    ComputeLehmerCode(order, temp.data(), size, lehmer.data());
    size_t end = size;
    while (end > skip && lehmer[end - 1] == 0) {
        --end;
    }
    tokens->emplace_back(CoeffOrderContext(size), end - skip);
    uint32_t last = 0;
    for (size_t i = skip; i < end; ++i) {
        tokens->emplace_back(CoeffOrderContext(last), lehmer[i]);
        last = lehmer[i];
    }
}

} // namespace

namespace {
void acc_EncodeCoeffOrder(const coeff_order_t* JXL_RESTRICT order,
                          AcStrategy acs,
                          std::vector<Token>* tokens,
                          coeff_order_t* order_zigzag) {
    const size_t llf = acs.covered_blocks_x() * acs.covered_blocks_y();
    const size_t size = kDCTBlockSize * llf;
    const coeff_order_t* natural_coeff_order_lut = acs.NaturalCoeffOrderLut();
    for (size_t i = 0; i < size; ++i) {
        order_zigzag[i] = natural_coeff_order_lut[order[i]];
    }
    acc_TokenizePermutation(order_zigzag, llf, size, tokens);
}
} // namespace

Status acc_predictAndtoken(LossyFrameEncoder& lossy_frame_encoder,
                           std::unique_ptr<FrameHeader>& frame_header,
                           std::vector<std::vector<Token> >& coefOrders_tokens,
                           ThreadPool* pool) {
    std::vector<EncCache>& group_caches_ = lossy_frame_encoder.get_group_cashes();
    PassesEncoderState* JXL_RESTRICT enc_state_ = lossy_frame_encoder.State();
    PassesSharedState& shared = enc_state_->shared;

    //------------------------------------------------------------------------//
    // Tokenize coefficients.
    //------------------------------------------------------------------------//
    group_caches_.resize(1); // multi-thread settings, would be further removed
    int thread = 0;
    for (int group_index = 0; group_index < shared.frame_dim.num_groups; group_index++) {
        const Rect rect = shared.BlockGroupRect(group_index);
        for (size_t idx_pass = 0; idx_pass < enc_state_->passes.size(); idx_pass++) {
            const int32_t* JXL_RESTRICT ac_rows[3] = {
                enc_state_->coeffs[idx_pass]->PlaneRow(0, group_index, 0).ptr32,
                enc_state_->coeffs[idx_pass]->PlaneRow(1, group_index, 0).ptr32,
                enc_state_->coeffs[idx_pass]->PlaneRow(2, group_index, 0).ptr32,
            };

            // Ensure group cache is initialized.
            group_caches_[thread].InitOnce();
            const coeff_order_t* acc_coeff_orders = &shared.coeff_orders[idx_pass * shared.coeff_order_size];
            TokenizeCoefficients(&shared.coeff_orders[idx_pass * shared.coeff_order_size], rect, ac_rows,
                                 shared.ac_strategy, frame_header->chroma_subsampling,
                                 &group_caches_[thread].num_nzeroes,
                                 &enc_state_->passes[idx_pass].ac_tokens[group_index], enc_state_->shared.quant_dc,
                                 enc_state_->shared.raw_quant_field, enc_state_->shared.block_ctx_map);
        }
    };

    const coeff_order_t* JXL_RESTRICT order = &enc_state_->shared.coeff_orders[0 * enc_state_->shared.coeff_order_size];
    auto mem = hwy::AllocateAligned<coeff_order_t>(AcStrategy::kMaxCoeffArea);
    uint16_t computed = 0;
    uint16_t used_orders = enc_state_->used_orders[0];

    if (frame_header->encoding == FrameEncoding::kVarDCT) {
        for (uint8_t o = 0; o < AcStrategy::kNumValidStrategies; ++o) {
            uint8_t ord = kStrategyOrder[o];
            if (computed & (1 << ord)) continue;
            computed |= 1 << ord;
            if ((used_orders & (1 << ord)) == 0) continue;
            AcStrategy acs = AcStrategy::FromRawStrategy(o);
            for (size_t c = 0; c < 3; c++) {
                acc_EncodeCoeffOrder(&order[CoeffOrderOffset(ord, c)], acs, &coefOrders_tokens[0], mem.get());
            }
        }
    }
    return true;
}

BitWriter* get_output(const size_t index, std::vector<BitWriter>& group_codes, bool is_small_image) {
    return &group_codes[is_small_image ? 0 : index];
}

Status acc_histogram(LossyFrameEncoder& lossy_frame_encoder,
                     std::unique_ptr<ModularFrameEncoder>& modular_frame_encoder,
                     PassesEncoderState* passes_enc_state,
                     FrameDimensions frame_dim,
                     std::unique_ptr<FrameHeader>& frame_header,
                     CompressParams cparams,
                     std::vector<std::vector<Token> >& coefOrders_tokens,
                     BitWriter* group_codes_writer,
                     BitWriter* acInfo_writer,
                     size_t& ans_cost,
                     size_t& mtf_cost,
                     std::vector<std::vector<Token> >& bcm_tokens,
                     std::vector<std::vector<Token> >& bcm_mtf_tokens,
                     EntropyEncodingData& bcm_codes,
                     std::vector<uint8_t>& bcm_dummy_context_map,

                     EntropyEncodingData& modularFramTree_code,
                     std::vector<uint8_t>& modularFramTree_ctxmap,

                     EntropyEncodingData& coefOrders_codes,
                     std::vector<uint8_t>& coefOrders_context_map,

                     std::vector<AuxOut>& aux_outs,
                     AuxOut* aux_out,
                     std::string xclbinPath) {
    std::vector<EncCache>& group_caches_ = lossy_frame_encoder.get_group_cashes();
    PassesEncoderState* JXL_RESTRICT enc_state_ = lossy_frame_encoder.State();
    PassesSharedState& shared = enc_state_->shared;

    const coeff_order_t* JXL_RESTRICT order = &enc_state_->shared.coeff_orders[0 * enc_state_->shared.coeff_order_size];
    auto mem = hwy::AllocateAligned<coeff_order_t>(AcStrategy::kMaxCoeffArea);
    uint16_t computed = 0;
    uint16_t used_orders = enc_state_->used_orders[0];

    if (frame_header->encoding == FrameEncoding::kVarDCT) {
        for (uint8_t o = 0; o < AcStrategy::kNumValidStrategies; ++o) {
            uint8_t ord = kStrategyOrder[o];
            if (computed & (1 << ord)) continue;
            computed |= 1 << ord;
            if ((used_orders & (1 << ord)) == 0) continue;
            AcStrategy acs = AcStrategy::FromRawStrategy(o);
            for (size_t c = 0; c < 3; c++) {
                acc_EncodeCoeffOrder(&order[CoeffOrderOffset(ord, c)], acs, &coefOrders_tokens[0], mem.get());
            }
        }
    }

    HistogramParams params0;
    params0.clustering = HistogramParams::ClusteringType::kFast;
    params0.uint_method = HistogramParams::HybridUintMethod::kNone;
    params0.lz77_method = HistogramParams::LZ77Method::kNone;
    HistogramParams params1;
    params1.clustering = HistogramParams::ClusteringType::kFast;
    params1.uint_method = HistogramParams::HybridUintMethod::kNone;
    params1.lz77_method = HistogramParams::LZ77Method::kNone;
    HistogramParams params2;
    params2.clustering = HistogramParams::ClusteringType::kFast;
    params2.uint_method = HistogramParams::HybridUintMethod::kNone;
    params2.lz77_method = HistogramParams::LZ77Method::kNone;
    HistogramParams params3;
    params3.clustering = HistogramParams::ClusteringType::kFast;
    params3.uint_method = HistogramParams::HybridUintMethod::kNone;
    params3.lz77_method = HistogramParams::LZ77Method::kNone;
    HistogramParams params4(enc_state_->cparams.speed_tier, enc_state_->shared.block_ctx_map.NumACContexts());
    if (enc_state_->cparams.decoding_speed_tier >= 1) {
        params4.max_histograms = 6;
    }

    params4.clustering = HistogramParams::ClusteringType::kFast;
    params4.uint_method = HistogramParams::HybridUintMethod::kNone;
    params4.lz77_method = HistogramParams::LZ77Method::kNone;
    std::vector<uint8_t> context_map0;
    std::vector<uint8_t> context_map1;
    std::vector<uint8_t> context_map2;
    std::vector<uint8_t> context_map3;
    std::vector<uint8_t> context_map4;
    std::vector<uint8_t> context_map_c0;
    std::vector<uint8_t> context_map_c1;
    std::vector<uint8_t> context_map_c2;
    std::vector<uint8_t> context_map_c3;
    std::vector<uint8_t> context_map_c4;
    std::vector<std::vector<Token> > tokens0(1);
    std::vector<std::vector<Token> > tokens1(1);
    std::vector<std::vector<Token> > tokens2(1);
    std::vector<std::vector<Token> > tokens3(1);
    std::vector<std::vector<Token> > tokens4(1);
    std::vector<std::vector<Token> > tokens_c0(1);
    std::vector<std::vector<Token> > tokens_c1(1);
    std::vector<std::vector<Token> > tokens_c2(1);
    std::vector<std::vector<Token> > tokens_c3(1);
    std::vector<std::vector<Token> > tokens_c4(1);
    EntropyEncodingData codes0;
    EntropyEncodingData codes1;
    EntropyEncodingData codes2;
    EntropyEncodingData codes3;
    EntropyEncodingData codes4;
    EntropyEncodingData codes_c0;
    EntropyEncodingData codes_c1;
    EntropyEncodingData codes_c2;
    EntropyEncodingData codes_c3;
    EntropyEncodingData codes_c4;
    std::vector<Histogram> clustered_histograms0;
    std::vector<Histogram> clustered_histograms1;
    std::vector<Histogram> clustered_histograms2;
    std::vector<Histogram> clustered_histograms3;
    std::vector<Histogram> clustered_histograms4;
    std::vector<Histogram> clustered_histograms_c0;
    std::vector<Histogram> clustered_histograms_c1;
    std::vector<Histogram> clustered_histograms_c2;
    std::vector<Histogram> clustered_histograms_c3;
    std::vector<Histogram> clustered_histograms_c4;
    BitWriter* writer0 = nullptr;
    BitWriter* writer1 = nullptr;
    BitWriter* writer2 = nullptr;
    BitWriter* writer3 = nullptr;
    BitWriter* writer4 = nullptr;
    size_t layer0 = 0;
    size_t layer1 = 0;
    size_t layer2 = 0;
    size_t layer3 = 0;
    size_t layer4 = 0;
    size_t num_contexts0 = 1;
    size_t num_contexts1 = 1;
    size_t num_contexts2 = 1;
    size_t num_contexts3 = 1;
    size_t num_contexts4 = 1;
    bool do_once[5] = {0, 0, 0, 0, 0};
    char* do_inner = (char*)malloc(sizeof(char) * 8);
    for (int i = 0; i < 5; i++) do_inner[i] = 0;
    char* do_prefix_in = (char*)malloc(sizeof(char) * 8);
    for (int i = 0; i < 5; i++) do_prefix_in[i] = 0;
    char* do_prefix_out = (char*)malloc(sizeof(char) * 8);
    for (int i = 0; i < 5; i++) do_prefix_out[i] = 0;

    const size_t num_passes = passes_enc_state->progressive_splitter.GetNumPasses();
    const bool is_small_image = frame_dim.num_groups == 1 && num_passes == 1;

    if (!is_small_image) {
        group_codes_writer->init(200);
        group_codes_writer->update_part(0);
    } else {
        group_codes_writer->init(200);
        group_codes_writer->update_part(0);
    }

    bool all_default = true;
    const float* dc_quant = (lossy_frame_encoder.State()->shared.matrices).DCQuants();
    for (size_t c = 0; c < 3; c++) {
        if (dc_quant[c] != kDCQuant[c]) {
            all_default = false;
        }
    }
    BitWriter::Allotment allotment(group_codes_writer, 1 + sizeof(float) * kBitsPerByte * 3);
    group_codes_writer->Write(1, all_default);
    if (!all_default) {
        for (size_t c = 0; c < 3; c++) {
            JXL_RETURN_IF_ERROR(F16Coder::Write(dc_quant[c] * 128.0f, group_codes_writer));
        }
    }
    ReclaimAndCharge(group_codes_writer, &allotment, kLayerDequantTables, aux_out);

    auto& dct = enc_state_->shared.block_ctx_map.dc_thresholds;
    auto& qft = enc_state_->shared.block_ctx_map.qf_thresholds;
    auto& ctx_map = enc_state_->shared.block_ctx_map.ctx_map;
    if (frame_header->encoding == FrameEncoding::kVarDCT) {
        JXL_RETURN_IF_ERROR(enc_state_->shared.quantizer.Encode(group_codes_writer, kLayerQuant, aux_out));
        //============Encode GlobalDCInfo: Block Context Map=========
        if (dct[0].empty() && dct[1].empty() && dct[2].empty() && qft.empty() && ctx_map.size() == 21 &&
            std::equal(ctx_map.begin(), ctx_map.end(), jxl::kDefaultCtxMap)) {
            group_codes_writer->Write(1, 1); // default
        } else {
            group_codes_writer->Write(1, 0);
            for (int j : {0, 1, 2}) {
                group_codes_writer->Write(4, dct[j].size());
                for (int i : dct[j]) {
                    JXL_CHECK(U32Coder::Write(kDCThresholdDist, PackSigned(i), group_codes_writer));
                }
            }
            group_codes_writer->Write(4, qft.size());
            for (uint32_t i : qft) {
                JXL_CHECK(U32Coder::Write(kQFThresholdDist, i - 1, group_codes_writer));
            }
            for (size_t i = 0; i < ctx_map.size(); i++) {
                bcm_tokens[0].emplace_back(0, ctx_map[i]);
            }

            {
                std::vector<uint8_t> context_map = ctx_map;
                BitWriter* writer = group_codes_writer;
                writer0 = group_codes_writer;
                size_t num_histograms = enc_state_->shared.block_ctx_map.num_ctxs;
                if (num_histograms == 1) {
                    // Simple code
                    writer->Write(1, 1);
                    // 0 bits per entry.
                    writer->Write(2, 0);
                } else {
                    std::vector<std::vector<Token> > tokens(1);
                    for (size_t i = 0; i < context_map.size(); i++) {
                        tokens[0].emplace_back(0, context_map[i]);
                    }

                    size_t entry_bits = CeilLog2Nonzero(num_histograms);
                    size_t simple_cost = entry_bits * context_map.size();
                    if (entry_bits < 4) {
                        writer->Write(1, 1);
                        writer->Write(2, entry_bits);
                        for (size_t i = 0; i < context_map.size(); i++) {
                            writer->Write(entry_bits, context_map[i]);
                        }
                    } else {
                        writer->Write(1, 0);
                        writer->Write(1, 0);
                        EntropyEncodingData context_codes0;
                        std::vector<std::vector<Token> > context_tokens0(1);
                        do_once[0] = true;
                        num_contexts0 = 1;
                        tokens0 = tokens;
                        codes0 = bcm_codes;
                        context_map0 = bcm_dummy_context_map;
                        // codes_c0 = context_codes0;
                        // writer0 = writer;
                        layer0 = 0;

                        // BuildAndEncodeHistogramsNew0
                        // =========================================================
                    }
                }
            }
        }
        //=============================
        //============Encode GlobalDCInfo: Color Correlation Map=========
        if (!is_small_image) {
            group_codes_writer->update_part(20);
        } else {
            group_codes_writer->update_part(20);
        }
        ColorCorrelationMapEncodeDC(&enc_state_->shared.cmap, group_codes_writer, kLayerDC, aux_out);
        //=============================
    }

    if (!is_small_image) {
        group_codes_writer->update_part(30);
    } else {
        group_codes_writer->update_part(30);
    }

    writer1 = group_codes_writer;
    writer2 = group_codes_writer;
    BitWriter::Allotment allotmentGlobalInfo(group_codes_writer, 1);
    // If we are using brotli, or not using modular mode.
    if (modular_frame_encoder->tree_tokens.empty() || modular_frame_encoder->tree_tokens[0].empty()) {
        group_codes_writer->Write(1, 0);
        ReclaimAndCharge(group_codes_writer, &allotmentGlobalInfo, kLayerModularTree, aux_out);
    } else {
        group_codes_writer->Write(1, 1);
        ReclaimAndCharge(group_codes_writer, &allotmentGlobalInfo, kLayerModularTree, aux_out);
        // Write tree
        if (cparams.speed_tier > SpeedTier::kKitten) {
            params1.ans_histogram_strategy = HistogramParams::ANSHistogramStrategy::kApproximate;
            params2.ans_histogram_strategy = HistogramParams::ANSHistogramStrategy::kApproximate;
        }

        if (cparams.decoding_speed_tier >= 1) {
            params1.max_histograms = 12;
            params2.max_histograms = 12;
        }

        EntropyEncodingData context_codes1;
        std::vector<std::vector<Token> > context_tokens1(1);
        std::vector<uint8_t> dummy_context_map1;

        do_once[1] = true;
        num_contexts1 = kNumTreeContexts;
        tokens1 = modular_frame_encoder->tree_tokens;
        codes1 = modularFramTree_code;
        context_map1 = modularFramTree_ctxmap;
        ////codes_c0 = context_codes0;
        ////writer0 = writer;
        layer1 = kLayerModularTree;

        // BuildAndEncodeHistogramsNew1

        if (!is_small_image) {
            group_codes_writer->update_part(50);
        } else {
            group_codes_writer->update_part(50);
        }
        params2.image_widths = modular_frame_encoder->image_widths;
        // Write histograms.
        EntropyEncodingData context_codes2;
        std::vector<std::vector<Token> > context_tokens2(1);
        std::vector<uint8_t> dummy_context_map2;

        do_once[2] = true;
        num_contexts2 = (modular_frame_encoder->tree.size() + 1) / 2;
        tokens2 = modular_frame_encoder->tokens;
        codes2 = modular_frame_encoder->code;
        context_map2 = modular_frame_encoder->context_map;
        ////codes_c0 = context_codes0;
        ////writer0 = writer;
        layer2 = kLayerModularGlobal;

        // BuildAndEncodeHistogramsNew2
    }

    //============================= Encode Global ACInfo =============
    if (!is_small_image) {
        acInfo_writer->init(200);
        acInfo_writer->update_part(0);
    } else {
        acInfo_writer->update_part(80);
    }
    writer3 = acInfo_writer;
    writer4 = acInfo_writer;

    if (frame_header->encoding == FrameEncoding::kVarDCT) {
        bool all_default = true;
        const std::vector<QuantEncoding>& encodings = (enc_state_->shared.matrices).encodings();

        for (size_t i = 0; i < encodings.size(); i++) {
            if (encodings[i].mode != QuantEncoding::kQuantModeLibrary || encodings[i].predefined != 0) {
                all_default = false;
            }
        }
        // TODO(janwas): better bound
        BitWriter::Allotment allotment(acInfo_writer, 512 * 1024);
        acInfo_writer->Write(1, all_default);
        ReclaimAndCharge(acInfo_writer, &allotment, kLayerDequantTables, aux_out);

        size_t num_histo_bits = CeilLog2Nonzero(enc_state_->shared.frame_dim.num_groups);
        if (num_histo_bits != 0) {
            BitWriter::Allotment allotment(acInfo_writer, num_histo_bits);
            acInfo_writer->Write(num_histo_bits, enc_state_->shared.num_histograms - 1);
            ReclaimAndCharge(acInfo_writer, &allotment, kLayerAC, aux_out);
        }

        //============= encode coef orders========
        // Encode coefficient orders.
        uint16_t used_orders = enc_state_->used_orders[0];
        size_t order_bits = 0;
        JXL_RETURN_IF_ERROR(U32Coder::CanEncode(kOrderEnc, enc_state_->used_orders[0], &order_bits));
        BitWriter::Allotment allotmentCoef(acInfo_writer, order_bits);
        JXL_CHECK(U32Coder::Write(kOrderEnc, enc_state_->used_orders[0], acInfo_writer));
        ReclaimAndCharge(acInfo_writer, &allotmentCoef, kLayerOrder, aux_out);

        // Do not write anything if no order is used.
        EntropyEncodingData context_codes3;
        std::vector<std::vector<Token> > context_tokens3(1);
        std::vector<uint8_t> dummy_context_map3;
        do_once[3] = true;
        num_contexts3 = kPermutationContexts;
        tokens3 = coefOrders_tokens;
        codes3 = coefOrders_codes;
        context_map3 = coefOrders_context_map;
        ////codes_c0 = context_codes0;
        ////writer0 = writer;
        layer3 = kLayerOrder;
        // BuildAndEncodeHistogramsNew3

        if (!is_small_image) {
            acInfo_writer->update_part(20);
        } else {
            acInfo_writer->update_part(100);
        }
    }

    std::vector<std::vector<Histogram> > histograms_(5);
    histograms_[0].resize(num_contexts0);
    histograms_[1].resize(num_contexts1);
    histograms_[2].resize(num_contexts2);
    histograms_[3].resize(num_contexts3);
    histograms_[4].resize(enc_state_->shared.num_histograms * enc_state_->shared.block_ctx_map.NumACContexts());

    std::vector<HistogramParams> params(5);
    std::vector<size_t> num_contexts(5);
    std::vector<size_t> layer(5);
    std::vector<EntropyEncodingData*> codes(5);
    std::vector<std::vector<uint8_t>*> context_map(5);
    std::vector<EntropyEncodingData*> codes_c(5);
    std::vector<BitWriter*> writer(5);
    writer[0] = writer0;
    writer[1] = writer1;
    writer[2] = writer2;
    writer[3] = writer3;
    writer[4] = writer4;

    std::vector<std::vector<uint32_t> > nonempty_histograms(5);
    std::vector<uint32_t> largest_idx(5);

    std::vector<std::vector<Histogram> > clustered_histograms(5);
    std::vector<std::vector<uint32_t> > histogram_symbols(5);

    std::vector<std::vector<Histogram> > clustered_histogramsin(5);
    std::vector<std::vector<std::vector<Token> > > tokensin(5, std::vector<std::vector<Token> >(1));
    std::vector<EntropyEncodingData> codesin(5);
    std::vector<std::vector<uint8_t> > context_map_in(5);

    constexpr float kMinDistanceForDistinctFast = 64.0f;
    constexpr float kMinDistanceForDistinctBest = 16.0f;

    if (frame_header->encoding == FrameEncoding::kVarDCT) {
        do_once[4] = true;
    }

    // Build histograms.
    for (int i = 0; i < 5; i++) {
        if (!do_once[i]) continue;
        if (i == 0) {
            params[0] = params0;
            num_contexts[0] = num_contexts0;
            layer[0] = layer0;
            codes[0] = &codes0;
            context_map[0] = &context_map0;
            codes_c[0] = &codes_c0;
        } else if (i == 1) {
            params[1] = params1;
            num_contexts[1] = num_contexts1;
            layer[1] = layer1;
            codes[1] = &codes1;
            context_map[1] = &context_map1;
            codes_c[1] = &codes_c1;
        } else if (i == 2) {
            params[2] = params2;
            num_contexts[2] = num_contexts2;
            layer[2] = layer2;
            codes[2] = &codes2;
            context_map[2] = &context_map2;
            codes_c[2] = &codes_c2;
        } else if (i == 3) {
            params[3] = params3;
            num_contexts[3] = num_contexts3;
            layer[3] = layer3;
            codes[3] = &codes3;
            context_map[3] = &context_map3;
            codes_c[3] = &codes_c3;
        } else if (i == 4) {
            params[4] = params4;
            num_contexts[4] = num_contexts4;
            layer[4] = kLayerAC;
            codes[4] = &enc_state_->passes[0].codes;
            context_map[4] = &enc_state_->passes[0].context_map;
            codes_c[4] = &codes_c4;
        }
    }

    // lossy_frame  ---- > mem

    int config[32];

    // config init
    int pixel_xsize = shared.frame_dim.xsize;
    int pixel_ysize = shared.frame_dim.ysize;
    int group_dim = shared.frame_dim.group_dim;
    int xsize_blocks = shared.frame_dim.xsize_blocks;
    int ysize_blocks = shared.frame_dim.ysize_blocks;

    config[0] = enc_state_->shared.quant_dc.bytes_per_row();
    config[1] = enc_state_->shared.raw_quant_field.bytes_per_row();
    config[4] = group_dim;
    config[5] = pixel_xsize;
    config[6] = pixel_ysize;
    config[7] = enc_state_->shared.block_ctx_map.num_ctxs;
    config[8] = enc_state_->shared.block_ctx_map.num_dc_ctxs;
    config[9] = enc_state_->shared.block_ctx_map.qf_thresholds.size();
    config[10] = 32; // nzero_stride
    config[11] = enc_state_->shared.block_ctx_map.ctx_map.size();
    config[12] = do_once[0];
    config[13] = do_once[1];
    config[14] = do_once[2];
    config[15] = do_once[3];
    config[16] = do_once[4];
    /*
     uint32_t nempty_cnt_ptr[5];
     uint32_t largest_idx_ptr[5];
     */

    // orders
    uint32_t* coeff_orders_ddr = new uint32_t[MAX_ORDERS_SIZE];
    const coeff_order_t* JXL_RESTRICT orders = &shared.coeff_orders[0];
    for (int i = 0; i < MAX_ORDERS_SIZE; i++) {
        coeff_orders_ddr[i] = orders[i];
    }

    // ac_coef
    int32_t* ac_coeff_ordered_ddr = new int32_t[ALL_PIXEL];
    uint64_t group_offset_1 = 0;
    for (int group_index = 0; group_index < shared.frame_dim.num_groups; group_index++) {
        const Rect rect = shared.BlockGroupRect(group_index);
        const int32_t* ac_rows[3] = {
            enc_state_->coeffs[0]->PlaneRow(0, group_index, 0).ptr32,
            enc_state_->coeffs[0]->PlaneRow(1, group_index, 0).ptr32,
            enc_state_->coeffs[0]->PlaneRow(2, group_index, 0).ptr32,
        };
        int rect_xsize_block = rect.xsize();
        int rect_ysize_block = rect.ysize();

        uint32_t offset[3] = {0, 0, 0};
        int acc_block_offset = 0;
        for (int by = 0; by < rect_ysize_block; by++) {
            int by_offset = by * rect_xsize_block * kDCTBlockSize * 3;
            for (int bx = 0; bx < rect_xsize_block; bx++) {
                int bx_offset = bx * kDCTBlockSize * 3;
                for (int c : {1, 0, 2}) {
                    // strategy
                    AcStrategyRow acs_row = shared.ac_strategy.ConstRow(rect, by);
                    AcStrategy acs = acs_row[bx];

                    // cx & cy
                    uint32_t cx = acs.covered_blocks_x();
                    uint32_t cy = acs.covered_blocks_y();

                    // covered blocks
                    const uint32_t covered_blocks = cx * cy; // = #LLF coefficients

                    // size
                    const uint32_t size = covered_blocks * kDCTBlockSize;

                    // ordered_coef
                    int ord = kStrategyOrder[acs.RawStrategy()];
                    const coeff_order_t* JXL_RESTRICT order = &orders[CoeffOrderOffset(ord, c)];

                    // block address
                    if (acs.IsFirstBlock()) {
                        const int32_t* block = ac_rows[c] + offset[c];
                        for (int k = 0; k < covered_blocks * kDCTBlockSize; k++) {
                            ac_coeff_ordered_ddr[k + acc_block_offset + group_offset_1] = block[order[k]];
                        }
                        acc_block_offset += size;
                        offset[c] += size;
                    }
                }
            }
        }
        group_offset_1 += rect_ysize_block * rect_xsize_block * kDCTBlockSize * 3;
    }

    // ac_strategy
    int32_t* strategy_ddr = new int32_t[MAX_NUM_BLK88];
    int group_offset_0 = 0;
    for (int group_index = 0; group_index < shared.frame_dim.num_groups; group_index++) {
        const Rect rect = shared.BlockGroupRect(group_index);
        int rect_xsize_block = rect.xsize();
        int rect_ysize_block = rect.ysize();
        for (int by = 0; by < rect_ysize_block; by++) {
            for (int bx = 0; bx < rect_xsize_block; bx++) {
                AcStrategyRow acs_row = shared.ac_strategy.ConstRow(rect, by);
                AcStrategy acs = acs_row[bx];
                strategy_ddr[group_offset_0 + by * rect_xsize_block + bx] = acs.Strategy();
            }
        }

        group_offset_0 += rect_ysize_block * rect_xsize_block;
    }

    // quant field & quant dc
    const int32_t* qf_rows = enc_state_->shared.raw_quant_field.ConstRow(0);
    const uint8_t* qdc_rows = enc_state_->shared.quant_dc.ConstRow(0);

    int32_t* qf_ddr = new int32_t[MAX_NUM_BLK88];
    uint8_t* qdc_ddr = new uint8_t[MAX_NUM_BLK88];

    int qdc_bytes_per_row = enc_state_->shared.quant_dc.bytes_per_row();
    int qf_bytes_per_row = enc_state_->shared.raw_quant_field.bytes_per_row();

    int group_offset_qdc_qf = 0;
    for (int group_index = 0; group_index < shared.frame_dim.num_groups; group_index++) {
        const Rect rect = shared.BlockGroupRect(group_index);
        int rect_xsize_block = rect.xsize();
        int rect_ysize_block = rect.ysize();
        for (int by = 0; by < rect_ysize_block; by++) {
            for (int bx = 0; bx < rect_xsize_block; bx++) {
                // qdc & qf
                int qdc_y_offset = (rect.y0() + by) * qdc_bytes_per_row + rect.x0();
                uint8_t dc_idx = qdc_rows[bx + qdc_y_offset];
                qdc_ddr[bx + by * rect_xsize_block + group_offset_qdc_qf] = dc_idx;

                int qf_y_offset = (rect.y0() + by) * (qf_bytes_per_row >> 2) + rect.x0();
                int32_t acc_qf = qf_rows[bx + qf_y_offset];
                qf_ddr[bx + by * rect_xsize_block + group_offset_qdc_qf] = acc_qf;
            }
        }

        group_offset_qdc_qf += rect_ysize_block * rect_xsize_block;
    }

    // block_ctx_map & qf_thresholds
    uint8_t* acc_ctx_map = new uint8_t[MAX_CTX_MAP_SIZE];
    uint32_t* qf_thresholds = new uint32_t[MAX_QF_THRESH_SIZE];

    int ctx_map_size = enc_state_->shared.block_ctx_map.ctx_map.size();
    int qf_thresholds_size = enc_state_->shared.block_ctx_map.qf_thresholds.size();

    for (int i = 0; i < ctx_map_size; i++) {
        acc_ctx_map[i] = enc_state_->shared.block_ctx_map.ctx_map[i];
    }

    for (int i = 0; i < qf_thresholds_size; i++) {
        qf_thresholds[i] = enc_state_->shared.block_ctx_map.qf_thresholds[i];
    }

    // ac_token
    uint64_t* ac_tokens_ddr = new uint64_t[MAX_AC_TOKEN_SIZE];

    // tokenize
    uint32_t token0_size = 0;
    for (int i = 0; i < tokens0.size(); i++) {
        token0_size += tokens0[i].size();
    }

    // ap_uint<64>* tokens0_ptr = (ap_uint<64>*)malloc(MAX_AC_TOKEN_SIZE * sizeof(ap_uint<64>));
    uint64_t* tokens0_ptr = (uint64_t*)malloc(MAX_AC_TOKEN_SIZE * sizeof(uint64_t));
    tokens0_ptr[0] = token0_size;
    uint32_t cnt = 1;
    for (int i = 0; i < tokens0.size(); i++) {
        for (int j = 0; j < tokens0[i].size(); j++) {
            ap_uint<64> reg;
            reg.range(31, 0) = tokens0[i][j].value;
            reg.range(62, 32) = tokens0[i][j].context;
            reg[63] = tokens0[i][j].is_lz77_length;
            tokens0_ptr[cnt] = (uint64_t)reg;
            cnt++;
        }
    }

    uint32_t token1_size = 0;
    for (int i = 0; i < tokens1.size(); i++) {
        token1_size += tokens1[i].size();
    }

    // for acc cosim
    // token1_size = MAX_AC_TOKEN_SIZE;

    // ap_uint<64>* tokens1_ptr = (ap_uint<64>*)malloc(MAX_AC_TOKEN_SIZE * sizeof(ap_uint<64>));
    uint64_t* tokens1_ptr = (uint64_t*)malloc(MAX_AC_TOKEN_SIZE * sizeof(uint64_t));

    tokens1_ptr[0] = token1_size;
    cnt = 1;
    for (int i = 0; i < tokens1.size(); i++) {
        for (int j = 0; j < tokens1[i].size(); j++) {
            ap_uint<64> reg;
            reg.range(31, 0) = tokens1[i][j].value;
            reg.range(62, 32) = tokens1[i][j].context;
            reg[63] = tokens1[i][j].is_lz77_length;
            tokens1_ptr[cnt] = (uint64_t)reg;
            cnt++;
        }
    }

    uint32_t token2_size = 0;
    for (int i = 0; i < tokens2.size(); i++) {
        token2_size += tokens2[i].size();
    }
    // ap_uint<64>* tokens2_ptr = (ap_uint<64>*)malloc(MAX_AC_TOKEN_SIZE * sizeof(ap_uint<64>));
    uint64_t* tokens2_ptr = (uint64_t*)malloc(MAX_AC_TOKEN_SIZE * sizeof(uint64_t));
    tokens2_ptr[0] = token2_size;
    cnt = 1;
    for (int i = 0; i < tokens2.size(); i++) {
        for (int j = 0; j < tokens2[i].size(); j++) {
            ap_uint<64> reg;
            reg.range(31, 0) = tokens2[i][j].value;
            reg.range(62, 32) = tokens2[i][j].context;
            reg[63] = tokens2[i][j].is_lz77_length;
            tokens2_ptr[cnt] = (uint64_t)reg;
            cnt++;
        }
    }

    uint32_t token3_size = 0;
    for (int i = 0; i < tokens3.size(); i++) {
        token3_size += tokens3[i].size();
    }
    // ap_uint<64>* tokens3_ptr = (ap_uint<64>*)malloc(MAX_AC_TOKEN_SIZE * sizeof(ap_uint<64>));
    uint64_t* tokens3_ptr = (uint64_t*)malloc(MAX_AC_TOKEN_SIZE * sizeof(uint64_t));
    tokens3_ptr[0] = token3_size;
    cnt = 1;
    for (int i = 0; i < tokens3.size(); i++) {
        for (int j = 0; j < tokens3[i].size(); j++) {
            ap_uint<64> reg;
            reg.range(31, 0) = tokens3[i][j].value;
            reg.range(62, 32) = tokens3[i][j].context;
            reg[63] = tokens3[i][j].is_lz77_length;
            tokens3_ptr[cnt] = (uint64_t)reg;
            cnt++;
        }
    }

    printf("==== token_size:%d, %d, %d, %d =====\n ", token0_size, token1_size, token2_size, token3_size);

    std::vector<int32_t*> histograms_ptr(5);
    std::vector<uint32_t*> histograms_size_ptr(5);
    std::vector<uint32_t*> total_count_ptr(5);
    std::vector<uint32_t*> nonempty_ptr(5);
    uint32_t nempty_cnt_ptr[5];
    uint32_t largest_idx_ptr[5];

    for (int i = 0; i < 5; i++) {
        histograms_ptr[i] = (int32_t*)malloc(4096 * 40 * sizeof(int32_t));
        histograms_size_ptr[i] = (uint32_t*)malloc(4096 * sizeof(uint32_t));
        total_count_ptr[i] = (uint32_t*)malloc(4096 * sizeof(uint32_t));
        nonempty_ptr[i] = (uint32_t*)malloc(4096 * sizeof(uint32_t));
    }

    //=====================================s
    // kernel_code
    //=====================================
    // std::string xclbinPath =
    //     "/wrk/xsjhdnobkup3/tianminr/jxl_debug/xf_codec/L2/demos/jxlEnc/tokInit_histogram/"
    //     "build_dir.sw_emu.xilinx_u50_gen3x16_xdma_201920_3/jxlEnc.xclbin";
    // std::string xclbinPath =
    //     "/wrk/xsjhdnobkup3/tianminr/jxl_debug/xf_codec/L2/demos/jxlEnc/tokInit_histogram/"
    //     "build_dir.hw_emu.xilinx_u50_gen3x16_xdma_201920_3/jxlEnc.xclbin";
    // std::string xclbinPath =
    // "/wrk/xsjhdnobkup3/tianminr/jxl_hw/xf_codec/L2/demos/jxlEnc/tokInit_histogram/build_dir.hw.xilinx_u50_gen3x16_xdma_201920_3/jxlEnc.xclbin";

    hls_ANSinitHistogram_wrapper(xclbinPath, config,
                                 //====================
                                 ac_coeff_ordered_ddr, strategy_ddr, qf_ddr, qdc_ddr, acc_ctx_map, qf_thresholds,
                                 ac_tokens_ddr,
                                 //====================
                                 tokens0_ptr, tokens1_ptr, tokens2_ptr, tokens3_ptr,
                                 //====================
                                 histograms_ptr[0], histograms_size_ptr[0], total_count_ptr[0], nonempty_ptr[0],
                                 //====================
                                 histograms_ptr[1], histograms_size_ptr[1], total_count_ptr[1], nonempty_ptr[1],
                                 //====================
                                 histograms_ptr[2], histograms_size_ptr[2], total_count_ptr[2], nonempty_ptr[2],
                                 //====================
                                 histograms_ptr[3], histograms_size_ptr[3], total_count_ptr[3], nonempty_ptr[3],
                                 //=====================
                                 histograms_ptr[4], histograms_size_ptr[4], total_count_ptr[4], nonempty_ptr[4]);

    // post-largeset & nempty
    largest_idx_ptr[0] = config[17];
    largest_idx_ptr[1] = config[18];
    largest_idx_ptr[2] = config[19];
    largest_idx_ptr[3] = config[20];
    largest_idx_ptr[4] = config[21];

    nempty_cnt_ptr[0] = config[22];
    nempty_cnt_ptr[1] = config[23];
    nempty_cnt_ptr[2] = config[24];
    nempty_cnt_ptr[3] = config[25];
    nempty_cnt_ptr[4] = config[26];

    int all_end = 0;
    int group_end = 0;
    int idx = 0, vec_idx = 0;
    int group_index = 0;
    std::vector<std::vector<Token> >& acc_ac_tokens = enc_state_->passes[0].ac_tokens;
    while (!all_end) {
        uint64_t token_tmp = ac_tokens_ddr[idx];
        all_end = token_tmp >> 63;
        group_end = token_tmp >> 62;
        if (group_end == 1) {
            group_index++;
            vec_idx = 0;
        } else if (all_end) {
            continue;
        } else {
            uint32_t value = token_tmp % 0xffffffff00000000;
            uint32_t context = (token_tmp >> 32) % 0xfffffffff000000;
            acc_ac_tokens[group_index].emplace_back(context, value);
        }
        idx++;
    }

    largest_idx[0] = largest_idx_ptr[0];
    largest_idx[1] = largest_idx_ptr[1];
    largest_idx[2] = largest_idx_ptr[2];
    largest_idx[3] = largest_idx_ptr[3];
    largest_idx[4] = largest_idx_ptr[4];

    delete[] strategy_ddr;
    delete[] ac_coeff_ordered_ddr;
    delete[] coeff_orders_ddr;
    delete[] ac_tokens_ddr;
    delete[] acc_ctx_map;
    delete[] qf_thresholds;

    for (int i = 0; i < 5; i++) {
        if (do_once[i]) {
            for (int j = 0; j < histograms_[i].size(); j++) {
                histograms_[i][j].data_.resize(histograms_size_ptr[i][j]);
                histograms_[i][j].total_count_ = total_count_ptr[i][j];
                for (int k = 0; k < histograms_size_ptr[i][j]; k++) {
                    histograms_[i][j].data_[k] = histograms_ptr[i][j * 40 + k];
                }
            }

            nonempty_histograms[i].reserve(histograms_[i].size());
            for (int j = 0; j < nempty_cnt_ptr[i]; j++) {
                nonempty_histograms[i].push_back(nonempty_ptr[i][j]);
            }

            if (histograms_[i].size() > 1) {
                largest_idx[i] =
                    std::find(nonempty_histograms[i].begin(), nonempty_histograms[i].end(), largest_idx[i]) -
                    nonempty_histograms[i].begin();
            }
        }
    }
    do_prefix_out[0] = 0;
    do_prefix_out[1] = 0;
    do_prefix_out[2] = 0;
    do_prefix_out[3] = 0;
    do_prefix_out[4] = 0;

    // kernel: acs_clusterHistogram
    acc_ANSclusterHistogram(is_small_image, do_once, do_inner, do_prefix_in, params, histograms_, num_contexts,
                            context_map, nonempty_histograms, largest_idx, codes, clustered_histograms,
                            histogram_symbols, writer, layer, clustered_histogramsin, tokensin, codesin,
                            context_map_in);

    for (int i = 0; i < 5; i++) {
        if (!do_once[i]) continue;
        if (i == 0) {
            tokens_c0 = tokensin[i];
            codes_c0 = codesin[i];
            context_map_c0 = context_map_in[i];
            clustered_histograms0 = clustered_histograms[i];
            clustered_histograms_c0 = clustered_histogramsin[i];
        } else if (i == 1) {
            tokens_c1 = tokensin[i];
            codes_c1 = codesin[i];
            context_map_c1 = context_map_in[i];
            clustered_histograms1 = clustered_histograms[i];
            clustered_histograms_c1 = clustered_histogramsin[i];
        } else if (i == 2) {
            tokens_c2 = tokensin[i];
            codes_c2 = codesin[i];
            context_map_c2 = context_map_in[i];
            clustered_histograms2 = clustered_histograms[i];
            clustered_histograms_c2 = clustered_histogramsin[i];
        } else if (i == 3) {
            tokens_c3 = tokensin[i];
            codes_c3 = codesin[i];
            context_map_c3 = context_map_in[i];
            clustered_histograms3 = clustered_histograms[i];
            clustered_histograms_c3 = clustered_histogramsin[i];
        } else if (i == 4) {
            tokens_c4 = tokensin[i];
            codes_c4 = codesin[i];
            context_map_c4 = context_map_in[i];
            clustered_histograms4 = clustered_histograms[i];
            clustered_histograms_c4 = clustered_histogramsin[i];
        }
    }

    // ==============================================
    // Do StoreEntropyCodes for outer histogram
    // ==============================================
    // printf("do_prefix_out = %d, %d, %d, %d, %d\n", do_prefix_out[0],
    // do_prefix_out[1], do_prefix_out[2], do_prefix_out[3], do_prefix_out[4]);

    if (do_once[0]) {
        if (!is_small_image) {
            writer0->update_part(4);
        } else {
            writer0->update_part(4);
        }
        StoreEntropyCodesNew(params0, tokens0, &codes0, do_prefix_out[0], writer0, layer0, nullptr,
                             clustered_histograms0);
        bcm_codes = codes0;
        bcm_dummy_context_map = context_map0;
    }
    if (do_once[1]) {
        if (!is_small_image) {
            writer1->update_part(34);
        } else {
            writer1->update_part(34);
        }
        StoreEntropyCodesNew(params1, tokens1, &codes1, do_prefix_out[1], writer1, layer1, nullptr,
                             clustered_histograms1);
        modularFramTree_code = codes1;
        modularFramTree_ctxmap = context_map1;
    }
    if (do_once[2]) {
        if (!is_small_image) {
            writer2->update_part(54);
        } else {
            writer2->update_part(54);
        }
        StoreEntropyCodesNew(params2, tokens2, &codes2, do_prefix_out[2], writer2, layer2, nullptr,
                             clustered_histograms2);
        modular_frame_encoder->code = codes2;
        modular_frame_encoder->context_map = context_map2;
    }
    if (do_once[3]) {
        if (!is_small_image) {
            writer3->update_part(4);
        } else {
            writer3->update_part(84);
        }
        StoreEntropyCodesNew(params3, tokens3, &codes3, do_prefix_out[3], writer3, layer3, nullptr,
                             clustered_histograms3);
        coefOrders_codes = codes3;
        coefOrders_context_map = context_map3;
    }
    if (do_once[4]) {
        if (!is_small_image) {
            writer4->update_part(24);
        } else {
            writer4->update_part(104);
        }
        StoreEntropyCodesNew(params4, tokens4, &codes4, do_prefix_out[4], writer4, layer4, nullptr,
                             clustered_histograms4);
        enc_state_->passes[0].codes = codes4;
        enc_state_->passes[0].context_map = context_map4;
    }

    // ==============================================
    // Do StoreEntropyCodes for inner histogram
    // ==============================================
    // printf("do_prefix_in = %d, %d, %d, %d, %d\n", do_prefix_in[0],
    // do_prefix_in[1], do_prefix_in[2], do_prefix_in[3], do_prefix_in[4]);

    if (do_inner[0]) {
        if (!is_small_image) {
            writer0->update_part(2);
        } else {
            writer0->update_part(2);
        }
        StoreEntropyCodesNew(params0, tokens_c0, &codes_c0, do_prefix_in[0], writer0, 0, nullptr,
                             clustered_histograms_c0);
    }
    if (do_inner[1]) {
        if (!is_small_image) {
            writer1->update_part(32);
        } else {
            writer1->update_part(32);
        }
        StoreEntropyCodesNew(params1, tokens_c1, &codes_c1, do_prefix_in[1], writer1, 0, nullptr,
                             clustered_histograms_c1);
    }
    if (do_inner[2]) {
        if (!is_small_image) {
            writer2->update_part(52);
        } else {
            writer2->update_part(52);
        }
        StoreEntropyCodesNew(params2, tokens_c2, &codes_c2, do_prefix_in[2], writer2, 0, nullptr,
                             clustered_histograms_c2);
    }
    if (do_inner[3]) {
        if (!is_small_image) {
            writer3->update_part(2);
        } else {
            writer3->update_part(82);
        }
        StoreEntropyCodesNew(params3, tokens_c3, &codes_c3, do_prefix_in[3], writer3, 0, nullptr,
                             clustered_histograms_c3);
    }
    if (do_inner[4]) {
        if (!is_small_image) {
            writer4->update_part(22);
        } else {
            writer4->update_part(102);
        }
        StoreEntropyCodesNew(params4, tokens_c4, &codes_c4, do_prefix_in[4], writer4, 0, nullptr,
                             clustered_histograms_c4);
    }

    // ==============================================
    // Do WriteTokens for inner histogram
    // ==============================================
    // printf("do_inner = %d, %d, %d, %d, %d\n", do_inner[0], do_inner[1],
    // do_inner[2], do_inner[3], do_inner[4]);
    if (do_inner[0]) {
        if (!is_small_image) {
            writer0->update_part(3);
        } else {
            writer0->update_part(3);
        }
        // printf("%s: %s: %d, WriteTokens token size out=%zu,
        // codes.encoding_info.size=%zu\n",
        //  __FILE__, __FUNCTION__, __LINE__, tokens_c0[0].size(),
        //  codes_c0.encoding_info.size(), context_map_c0.size());
        WriteTokens(tokens_c0[0], codes_c0, context_map_c0, writer0);
    }
    if (do_inner[1]) {
        if (!is_small_image) {
            writer1->update_part(33);
        } else {
            writer1->update_part(33);
        }
        // printf("%s: %s: %d, WriteTokens token size out=%zu,
        // codes.encoding_info.size=%zu, context_map.size=%d\n",
        //  __FILE__, __FUNCTION__, __LINE__, tokens_c0[0].size(),
        //  codes_c0.encoding_info.size(), context_map_c0.size());
        WriteTokens(tokens_c1[0], codes_c1, context_map_c1, writer1);
    }
    if (do_inner[2]) {
        if (!is_small_image) {
            writer2->update_part(53);
        } else {
            writer2->update_part(53);
        }
        // printf("%s: %s: %d, WriteTokens token size out=%zu,
        // codes.encoding_info.size=%zu, context_map.size=%d\n",
        //  __FILE__, __FUNCTION__, __LINE__, tokens_c0[0].size(),
        //  codes_c0.encoding_info.size(), context_map_c0.size());
        WriteTokens(tokens_c2[0], codes_c2, context_map_c2, writer2);
    }
    if (do_inner[3]) {
        if (!is_small_image) {
            writer3->update_part(3);
        } else {
            writer3->update_part(83);
        }
        // printf("%s: %s: %d, WriteTokens token size out=%zu,
        // codes.encoding_info.size=%zu, context_map.size=%d\n",
        //  __FILE__, __FUNCTION__, __LINE__, tokens_c0[0].size(),
        //  codes_c0.encoding_info.size(), context_map_c0.size());
        WriteTokens(tokens_c3[0], codes_c3, context_map_c3, writer3);
    }
    if (do_inner[4]) {
        if (!is_small_image) {
            writer4->update_part(23);
        } else {
            writer4->update_part(103);
        }
        // printf("%s: %s: %d, WriteTokens token size out=%zu,
        // codes.encoding_info.size=%zu, context_map.size=%d\n",
        //  __FILE__, __FUNCTION__, __LINE__, tokens_c0[0].size(),
        //  codes_c0.encoding_info.size(), context_map_c0.size());
        WriteTokens(tokens_c4[0], codes_c4, context_map_c4, writer4);
    }
    return true;
}

Status acc_ANS_tokens(LossyFrameEncoder& lossy_frame_encoder,
                      std::unique_ptr<ModularFrameEncoder>& modular_frame_encoder,
                      const size_t num_groups,
                      PassesEncoderState* passes_enc_state,
                      FrameDimensions frame_dim,
                      std::unique_ptr<FrameHeader>& frame_header,
                      std::vector<std::vector<Token> >& coefOrders_tokens,
                      std::vector<BitWriter>& group_codes,
                      BitWriter* group_codes_writer,
                      BitWriter* acInfo_writer,
                      std::vector<BitWriter*>& dc_group_writers,
                      std::vector<BitWriter*>& acGroupWriters,
                      size_t& ans_cost,
                      size_t& mtf_cost,
                      std::vector<std::vector<Token> >& bcm_tokens,
                      std::vector<std::vector<Token> >& bcm_mtf_tokens,
                      EntropyEncodingData& bcm_codes,
                      std::vector<uint8_t>& bcm_dummy_context_map,

                      EntropyEncodingData& modularFramTree_code,
                      std::vector<uint8_t>& modularFramTree_ctxmap,

                      EntropyEncodingData& coefOrders_codes,
                      std::vector<uint8_t>& coefOrders_context_map,
                      std::vector<AuxOut>& aux_outs,
                      AuxOut* aux_out) {
    PassesEncoderState* JXL_RESTRICT enc_state_ = lossy_frame_encoder.State();
    PassesSharedState& shared = enc_state_->shared;
    const size_t global_ac_index = frame_dim.num_dc_groups + 1;

    const size_t num_passes = passes_enc_state->progressive_splitter.GetNumPasses();
    const bool is_small_image = frame_dim.num_groups == 1 && num_passes == 1;

    const bool has_ac_global = true;

    auto& dct = enc_state_->shared.block_ctx_map.dc_thresholds;
    auto& qft = enc_state_->shared.block_ctx_map.qf_thresholds;
    auto& ctx_map = enc_state_->shared.block_ctx_map.ctx_map;

    //============ANSWriteTokens Encode GlobalDCInfo: Block Context Map=========
    if (frame_header->encoding == FrameEncoding::kVarDCT) {
        if (dct[0].empty() && dct[1].empty() && dct[2].empty() && qft.empty() && ctx_map.size() == 21 &&
            std::equal(ctx_map.begin(), ctx_map.end(), jxl::kDefaultCtxMap)) {
        } else {
            if (enc_state_->shared.block_ctx_map.num_ctxs == 1) {
            } else {
                size_t entry_bits = CeilLog2Nonzero(enc_state_->shared.block_ctx_map.num_ctxs);
                size_t simple_cost = entry_bits * ctx_map.size();
                if (entry_bits < 4 /* && simple_cost < ans_cost &&
            simple_cost < mtf_cost*/) {
                } else {
                    if (!is_small_image) {
                        group_codes_writer->update_part(10);
                    } else {
                        group_codes_writer->update_part(10);
                    }
                    WriteTokens(bcm_tokens[0], bcm_codes, bcm_dummy_context_map, group_codes_writer);
                }
            }
            BitWriter::Allotment allotmentGlobalDCInfoBCM(
                group_codes_writer, (dct[0].size() + dct[1].size() + dct[2].size() + qft.size()) * 34 + 1 + 4 + 4 +
                                        ctx_map.size() * 10 + 1024);
            ReclaimAndCharge(group_codes_writer, &allotmentGlobalDCInfoBCM, kLayerAC, aux_out);
        }
    }

    //============ANSWriteTokens Encode GlobalDCInfo: modular frame
    // tree=========
    if (modular_frame_encoder->tree_tokens.empty() || modular_frame_encoder->tree_tokens[0].empty()) {
    } else {
        if (!is_small_image) {
            group_codes_writer->update_part(40);
        } else {
            group_codes_writer->update_part(40);
        }
        WriteTokens(modular_frame_encoder->tree_tokens[0], modularFramTree_code, modularFramTree_ctxmap,
                    group_codes_writer, kLayerModularTree, aux_out);
    }

    //============ANSWriteTokens Encode GlobalDCInfo: modular frame
    // token=========
    if (!is_small_image) {
        group_codes_writer->update_part(60);
    } else {
        group_codes_writer->update_part(60);
    }
    size_t stream_id = ModularStreamId::Global().ID(frame_dim);
    if (modular_frame_encoder->stream_images[stream_id].channel.empty()) {
        // Image with no channels, header never gets decoded.
    } else {
        JXL_RETURN_IF_ERROR(Bundle::Write(modular_frame_encoder->stream_headers[stream_id], group_codes_writer,
                                          kLayerModularGlobal, aux_out));
        WriteTokens(modular_frame_encoder->tokens[stream_id], modular_frame_encoder->code,
                    modular_frame_encoder->context_map, group_codes_writer, kLayerModularGlobal, aux_out);
    }

    //=============================

    //============================= ANSWriteTokens DC group=============
    for (int group_index = 0; group_index < frame_dim.num_dc_groups; group_index++) {
        BitWriter* tmp = get_output(group_index + 1, group_codes, is_small_image);
        dc_group_writers.emplace_back(tmp);
        if (!is_small_image) {
            tmp->init(200);
            tmp->update_part(0);
        } else {
            tmp->update_part(70);
        }
    }

    for (int group_index = 0; group_index < frame_dim.num_dc_groups; group_index++) {
        AuxOut* my_aux_out = aux_out ? &aux_outs[0] : nullptr;
        BitWriter* output = dc_group_writers[group_index];
        if (frame_header->encoding == FrameEncoding::kVarDCT && !(frame_header->flags & FrameHeader::kUseDcFrame)) {
            BitWriter::Allotment allotment(output, 2);
            output->Write(2, modular_frame_encoder->extra_dc_precision[group_index]);
            ReclaimAndCharge(output, &allotment, kLayerDC, my_aux_out);
            size_t stream_id = ModularStreamId::VarDCTDC(group_index).ID(frame_dim);
            if (modular_frame_encoder->stream_images[stream_id].channel.empty()) {
                // Image with no channels, header never gets decoded.
            } else {
                Bundle::Write(modular_frame_encoder->stream_headers[stream_id], output, kLayerDC, aux_out);
                WriteTokens(modular_frame_encoder->tokens[stream_id], modular_frame_encoder->code,
                            modular_frame_encoder->context_map, output, kLayerDC, my_aux_out);
            }
        }

        size_t stream_id = ModularStreamId::ModularDC(group_index).ID(frame_dim);
        if (modular_frame_encoder->stream_images[stream_id].channel.empty()) {
            // Image with no channels, header never gets decoded.
        } else {
            Bundle::Write(modular_frame_encoder->stream_headers[stream_id], output, kLayerModularDcGroup, aux_out);
            WriteTokens(modular_frame_encoder->tokens[stream_id], modular_frame_encoder->code,
                        modular_frame_encoder->context_map, output, kLayerModularDcGroup, my_aux_out);
        }

        if (frame_header->encoding == FrameEncoding::kVarDCT) {
            const Rect& rect = lossy_frame_encoder.State()->shared.DCGroupRect(group_index);
            size_t nb_bits = CeilLog2Nonzero(rect.xsize() * rect.ysize());
            if (nb_bits != 0) {
                BitWriter::Allotment allotment(output, nb_bits);
                output->Write(nb_bits, modular_frame_encoder->ac_metadata_size[group_index] - 1);
                ReclaimAndCharge(output, &allotment, kLayerControlFields, my_aux_out);
            }
            size_t stream_id = ModularStreamId::ACMetadata(group_index).ID(frame_dim);
            if (modular_frame_encoder->stream_images[stream_id].channel.empty()) {
                // Image with no channels, header never gets decoded.
            } else {
                Bundle::Write(modular_frame_encoder->stream_headers[stream_id], output, kLayerControlFields, aux_out);
                WriteTokens(modular_frame_encoder->tokens[stream_id], modular_frame_encoder->code,
                            modular_frame_encoder->context_map, output, kLayerControlFields, my_aux_out);
            }
        }
    };

    //============================= ANSWriteTokens AC Info=============
    for (size_t i = 0; i < enc_state_->progressive_splitter.GetNumPasses(); i++) {
        uint16_t used_orders = enc_state_->used_orders[i];
        if (used_orders != 0) {
            if (!is_small_image) {
                acInfo_writer->update_part(19);
            } else {
                acInfo_writer->update_part(90);
            }
            WriteTokens(coefOrders_tokens[0], coefOrders_codes, coefOrders_context_map, acInfo_writer, kLayerOrder,
                        aux_out);
        }
    }

    //==========================================
    if (!is_small_image) {
        acInfo_writer->update_part(29);
    } else {
        acInfo_writer->update_part(109);
    }
    //===============

    //========================Encode AC Group=============
    for (int group_index = 0; group_index < num_groups; group_index++) {
        for (size_t i = 0; i < num_passes; i++) {
            BitWriter* tmp =
                get_output(AcGroupIndex(i, group_index, frame_dim.num_groups, frame_dim.num_dc_groups, has_ac_global),
                           group_codes, is_small_image);
            acGroupWriters.emplace_back(tmp);
        }
    }

    int sum = 0;
    for (int group_index = 0; group_index < num_groups; group_index++) {
        AuxOut* my_aux_out = aux_out ? &aux_outs[0] : nullptr;
        for (size_t i = 0; i < num_passes; i++) {
            BitWriter* acGroupWriter = acGroupWriters[group_index * num_passes + i];
            if (frame_header->encoding == FrameEncoding::kVarDCT) {
                // Select which histogram to use among those of the current pass.
                const size_t num_histograms = enc_state_->shared.num_histograms;
                // num_histograms is 0 only for lossless.
                JXL_ASSERT(num_histograms == 0 || enc_state_->histogram_idx[group_index] < num_histograms);
                size_t histo_selector_bits = CeilLog2Nonzero(num_histograms);

                if (histo_selector_bits != 0) {
                    BitWriter::Allotment allotment(acGroupWriter, histo_selector_bits);
                    acGroupWriter->Write(histo_selector_bits, enc_state_->histogram_idx[group_index]);
                    ReclaimAndCharge(acGroupWriter, &allotment, kLayerAC, aux_out);
                }
                sum = sum + enc_state_->passes[i].ac_tokens[group_index].size();
                WriteTokens(enc_state_->passes[i].ac_tokens[group_index], enc_state_->passes[i].codes,
                            enc_state_->passes[i].context_map, acGroupWriter, kLayerACTokens, aux_out);
            }

            size_t stream_id = ModularStreamId::ModularAC(group_index, i).ID(frame_dim);
            if (modular_frame_encoder->stream_images[stream_id].channel.empty()) {
                // Image with no channels, header never gets decoded.
            } else {
                Bundle::Write(modular_frame_encoder->stream_headers[stream_id], acGroupWriter, kLayerModularAcGroup,
                              aux_out);
                WriteTokens(modular_frame_encoder->tokens[stream_id], modular_frame_encoder->code,
                            modular_frame_encoder->context_map, acGroupWriter, kLayerModularAcGroup, aux_out);
            }
        }
    }
    //=====================

    return true;
}

Status acc_writeout(LossyFrameEncoder& lossy_frame_encoder,
                    const size_t num_groups,
                    PassesEncoderState* passes_enc_state,
                    std::unique_ptr<FrameHeader>& frame_header,
                    FrameDimensions frame_dim,
                    std::vector<BitWriter>& group_codes,
                    BitWriter* writer,
                    BitWriter* group_codes_writer,
                    BitWriter* acInfo_writer,
                    std::vector<BitWriter*>& dc_group_writers,
                    std::vector<BitWriter*>& acGroupWriters,
                    AuxOut* aux_out,
                    const std::function<Status(size_t)>& resize_aux_outs) {
    const size_t num_passes = passes_enc_state->progressive_splitter.GetNumPasses();
    const bool is_small_image = frame_dim.num_groups == 1 && num_passes == 1;

    writer->AppendByteAligned(lossy_frame_encoder.State()->special_frames);
    frame_header->UpdateFlag(lossy_frame_encoder.State()->shared.image_features.patches.HasAny(),
                             FrameHeader::kPatches);
    frame_header->UpdateFlag(lossy_frame_encoder.State()->shared.image_features.splines.HasAny(),
                             FrameHeader::kSplines);
    JXL_RETURN_IF_ERROR(WriteFrameHeader(*frame_header, writer, aux_out));

    // Resizing aux_outs to 0 also Assimilates the array.
    std::atomic<int> num_errors{0};
    static_cast<void>(resize_aux_outs(0));
    JXL_RETURN_IF_ERROR(num_errors.load(std::memory_order_relaxed) == 0);

    for (BitWriter& bw : group_codes) {
        bw.ZeroPadToByte(); // end of group.
    }

    if (is_small_image) {
        std::vector<int> group_codes_seq{0,  1,  2,  3,  4,  10, 19, 20, 29, 30, 31, 32,  33,  34,  40,  50,  51,
                                         52, 53, 54, 60, 70, 80, 81, 82, 83, 84, 90, 100, 101, 102, 103, 104, 109};
        group_codes_writer->Finalize(group_codes_seq);
        //  group_codes_writer->Finalize();
    } else {
        std::cout << "===============Group Codes writer Final==================" << std::endl;
        std::vector<int> group_codes_seq{0, 1, 2, 3, 4, 10, 19, 20, 29, 30, 31, 32, 33, 34, 40, 50, 51, 52, 53, 54, 60};
        group_codes_writer->Finalize(group_codes_seq);
        //    group_codes_writer->Finalize();
        std::cout << "===============DC Group writer Final==================" << std::endl;
        std::vector<int> dc_group_seq{0};
        for (int group_index = 0; group_index < frame_dim.num_dc_groups; group_index++) {
            dc_group_writers[group_index]->Finalize(dc_group_seq);
            //        dc_group_writers[group_index]->Finalize();
        }
        std::cout << "===============AC Info writer Final==================" << std::endl;
        std::vector<int> acInfo_seq{0, 1, 2, 3, 4, 10, 19, 20, 21, 22, 23, 24, 29};
        acInfo_writer->Finalize(acInfo_seq);
        //  acInfo_writer->Finalize();
        std::cout << "===============AC Group writer Final==================" << std::endl;
        std::vector<int> acGroup_seq{0};
        for (int group_index = 0; group_index < num_groups; group_index++) {
            for (size_t i = 0; i < num_passes; i++) {
                acGroupWriters[group_index * num_passes + i]->Finalize(acGroup_seq);
                //           acGroupWriters[group_index * num_passes + i]->Finalize();
            }
        }
    }
    std::cout << "===============Others writer Final==================" << std::endl;
    BitWriter::Allotment allotmentGrpOffset(writer, MaxBits(group_codes.size()));
    writer->Write(1, 0); // no permutation
    std::vector<int> write_seq{0};
    //  writer->Finalize(write_seq);
    writer->Finalize();
    //  }
    writer->ZeroPadToByte(); // before TOC entries

    for (size_t i = 0; i < group_codes.size(); i++) {
        JXL_ASSERT(group_codes[i].BitsWritten() % kBitsPerByte == 0);
        const size_t group_size = group_codes[i].BitsWritten() / kBitsPerByte;
        JXL_RETURN_IF_ERROR(U32Coder::Write(kTocDist, group_size, writer));
    }
    //  writer->Finalize(write_seq);
    writer->Finalize();
    writer->ZeroPadToByte(); // before first group
    ReclaimAndCharge(writer, &allotmentGrpOffset, kLayerTOC, aux_out);

    writer->AppendByteAligned(group_codes);
    writer->ZeroPadToByte(); // end of frame.

    return true;
}

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
    std::cout << "===========acc_kernel3 start================" << std::endl;
    std::vector<std::vector<Token> > coefOrders_tokens(1);

    const size_t num_passes = passes_enc_state->progressive_splitter.GetNumPasses();

    // DC global info + DC groups + AC global info + AC groups *
    // num_passes.
    const bool has_ac_global = true;
    std::vector<BitWriter> group_codes(
        NumTocEntries(frame_dim.num_groups, frame_dim.num_dc_groups, num_passes, has_ac_global));
    const size_t global_ac_index = frame_dim.num_dc_groups + 1;
    const bool is_small_image = frame_dim.num_groups == 1 && num_passes == 1;

    BitWriter* group_codes_writer = get_output(0, group_codes, is_small_image);
    BitWriter* acInfo_writer = get_output(global_ac_index, group_codes, is_small_image);

    std::vector<std::vector<Token> > bcm_tokens(1), bcm_mtf_tokens(1);
    EntropyEncodingData bcm_codes;
    std::vector<uint8_t> bcm_dummy_context_map;
    size_t ans_cost, mtf_cost;

    EntropyEncodingData modularFramTree_code;
    std::vector<uint8_t> modularFramTree_ctxmap;

    EntropyEncodingData coefOrders_codes;
    std::vector<uint8_t> coefOrders_context_map;

    std::vector<BitWriter*> dc_group_writers;
    std::vector<BitWriter*> acGroupWriters;
    struct timeval start_time, token_time, hist_time, ans_time;
    gettimeofday(&start_time, 0);
    //  acc_predictAndtoken(lossy_frame_encoder, frame_header,
    //  coefOrders_tokens,
    //                      pool);

    gettimeofday(&token_time, 0);
    acc_histogram(lossy_frame_encoder, modular_frame_encoder, passes_enc_state, frame_dim, frame_header, cparams,
                  coefOrders_tokens, group_codes_writer, acInfo_writer, ans_cost, mtf_cost, bcm_tokens, bcm_mtf_tokens,
                  bcm_codes, bcm_dummy_context_map,

                  modularFramTree_code, modularFramTree_ctxmap,

                  coefOrders_codes, coefOrders_context_map,

                  aux_outs, aux_out, xclbinPath);
    gettimeofday(&hist_time, 0);
    acc_ANS_tokens(lossy_frame_encoder, modular_frame_encoder, num_groups, passes_enc_state, frame_dim, frame_header,
                   coefOrders_tokens, group_codes, group_codes_writer, acInfo_writer, dc_group_writers, acGroupWriters,
                   ans_cost, mtf_cost, bcm_tokens, bcm_mtf_tokens, bcm_codes, bcm_dummy_context_map,

                   modularFramTree_code, modularFramTree_ctxmap,

                   coefOrders_codes, coefOrders_context_map, aux_outs, aux_out);

    acc_writeout(lossy_frame_encoder, num_groups, passes_enc_state, frame_header, frame_dim, group_codes, writer,
                 group_codes_writer, acInfo_writer, dc_group_writers, acGroupWriters, aux_out, resize_aux_outs);
    gettimeofday(&ans_time, 0);

    return true;
}
} // namespace jxl

#endif
