// Copyright (c) the JPEG XL Project Authors. All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#ifndef ACC_CLUSTER_HISTOGRAM_CPP
#define ACC_CLUSTER_HISTOGRAM_CPP

#include "acc_cluster_histogram.hpp"

namespace jxl {
void acc_ANSclusterHistogram(bool is_small_image,
                             bool do_once[5],
                             char* do_inner,
                             char* do_prefix_in,

                             std::vector<HistogramParams>& params,

                             std::vector<std::vector<Histogram> >& histograms_,
                             std::vector<size_t>& num_contexts,
                             std::vector<std::vector<uint8_t>*> context_map,
                             std::vector<std::vector<uint32_t> >& nonempty_histograms,
                             std::vector<uint32_t>& largest_idx,

                             std::vector<EntropyEncodingData*> codes,
                             std::vector<std::vector<Histogram> >& clustered_histograms,
                             std::vector<std::vector<uint32_t> >& histogram_symbols,

                             std::vector<BitWriter*> writer,
                             std::vector<size_t> layer,
                             std::vector<std::vector<Histogram> >& clustered_histogramsin,
                             std::vector<std::vector<std::vector<Token> > >& tokensin,
                             std::vector<EntropyEncodingData>& codesin,
                             std::vector<std::vector<uint8_t> >& context_map_in) {
    constexpr float kMinDistanceForDistinctFast = 64.0f;
    constexpr float kMinDistanceForDistinctBest = 16.0f;

    for (int i = 0; i < 5; i++) {
        if (!do_once[i]) continue;

        codes[i]->lz77.nonserialized_distance_context = num_contexts[i];
        codes[i]->lz77.enabled = false;
        codes[i]->lz77.min_symbol = 224;
        codes[i]->encoding_info.clear();
        context_map[i]->resize(histograms_[i].size());
        clustered_histograms[i] = histograms_[i];

        if (histograms_[i].size() > 1) {
            size_t max_histograms = std::min(kClustersLimit, params[i].max_histograms);
            acc_FastClusterHistograms(histograms_[i], nonempty_histograms[i], largest_idx[i],
                                      nonempty_histograms[i].size(), max_histograms, kMinDistanceForDistinctFast,
                                      &clustered_histograms[i], &histogram_symbols[i]);
        }
    }

    for (int i = 0; i < 5; i++) {
        if (!do_once[i]) continue;
        if (histograms_[i].size() > 1) {
            // Convert the context map to a canonical form.
            HistogramReindex(&clustered_histograms[i], &histogram_symbols[i]);

            for (size_t c = 0; c < histograms_[i].size(); ++c) {
                (*context_map[i])[c] = static_cast<uint8_t>(histogram_symbols[i][c]);
            }
        }
    }

    for (int i = 0; i < 5; i++) {
        if (!do_once[i]) continue;
        size_t histograms_size = histograms_[i].size();
        if (histograms_size > 1) {
            if (writer[i] != nullptr) {
                size_t num_histograms = clustered_histograms[i].size();
                if (num_histograms == 1) {
                } else {
                    for (size_t j = 0; j < (*context_map[i]).size(); j++) {
                        tokensin[i][0].emplace_back(0, (*context_map[i])[j]);
                    }

                    size_t entry_bits = CeilLog2Nonzero(num_histograms);
                    if (entry_bits < 4) {
                    } else {
                        do_inner[i] = 1;
                    }
                }
            }
        }

        if (do_inner[i]) {
            codesin[i].lz77.nonserialized_distance_context = 1;
            codesin[i].lz77.enabled = false;
            codesin[i].lz77.min_symbol = 224;

            bool use_prefix_code = false;
            do_prefix_in[i] = (char)use_prefix_code;

            std::vector<Histogram> ctxHistograms_(1);
            HybridUintConfig uint_config; //  Default config for clustering.

            for (size_t j = 0; j < tokensin[i].size(); ++j) {
                for (size_t k = 0; k < tokensin[i][j].size(); ++k) {
                    const Token token = tokensin[i][j][k];
                    uint32_t tok, nbits, bits;
                    uint_config.Encode(token.value, &tok, &nbits, &bits);
                    ctxHistograms_[0].Add(tok);
                    clustered_histogramsin[i] = ctxHistograms_;

                    codesin[i].encoding_info.clear();
                    context_map_in[i].resize(clustered_histogramsin[i].size());
                }
            }
        }
    }

    for (int i = 0; i < 5; i++) {
        if (!do_once[i]) continue;

        if (i == 0) {
            if (!is_small_image) {
                writer[0]->update_part(1);
            } else {
                writer[0]->update_part(1);
            }

        } else if (i == 1) {
            if (!is_small_image) {
                writer[1]->update_part(31);
            } else {
                writer[1]->update_part(31);
            }
        } else if (i == 2) {
            if (!is_small_image) {
                writer[2]->update_part(51);
            } else {
                writer[2]->update_part(51);
            }
        } else if (i == 3) {
            if (!is_small_image) {
                writer[3]->update_part(1);
            } else {
                writer[3]->update_part(81);
            }
        } else if (i == 4) {
            if (!is_small_image) {
                writer[4]->update_part(21);
            } else {
                writer[4]->update_part(101);
            }
        }

        size_t histograms_size = histograms_[i].size();

        const size_t max_contexts = std::min(num_contexts[i], kClustersLimit);
        BitWriter::Allotment allotment(writer[i], 128 + num_contexts[i] * 40 + max_contexts * 96);
        if (writer[i]) {
            JXL_CHECK(Bundle::Write(codes[i]->lz77, writer[i], layer[i], nullptr));
        }

        if (histograms_size > 1) {
            size_t num_histograms = clustered_histograms[i].size();
            if (writer[i] != nullptr) {
                // printf("%s: %s: %d, Start EncodeContextMap context size=%zu\n\n",
                // __FILE__, __FUNCTION__, __LINE__, (*context_map).size());
                if (num_histograms == 1) {
                    writer[i]->Write(1, 1);
                    writer[i]->Write(2, 0);
                } else {
                    size_t entry_bits = CeilLog2Nonzero(num_histograms);
                    if (entry_bits < 4) {
                        writer[i]->Write(1, 1);
                        writer[i]->Write(2, entry_bits);
                        for (size_t j = 0; j < (*context_map[i]).size(); j++) {
                            writer[i]->Write(entry_bits, (*context_map[i])[j]);
                        }
                    } else {
                        writer[i]->Write(1, 0);
                        writer[i]->Write(1, 0);
                    }
                }
            }
        }
        // StoreEntropyCodesNew
        allotment.FinishedHistogram(writer[i]);
        ReclaimAndCharge(writer[i], &allotment, layer[i], nullptr);

        if (do_inner[i]) {
            // do inner ontext map = true
            BitWriter::Allotment allotment(writer[i], 128 + 1 * 40 + 96);
            JXL_CHECK(Bundle::Write(codesin[i].lz77, writer[i], 0, nullptr));

            // StoreEntropyCodesNew
            // WriteToken
            allotment.FinishedHistogram(writer[i]);
            ReclaimAndCharge(writer[i], &allotment, 0, nullptr);
        }
    }
}

} // namespace jxl

#endif
