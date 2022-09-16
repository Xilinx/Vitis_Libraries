// Copyright (c) the JPEG XL Project Authors. All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#ifndef ACC_STORE_ENCODE_DATA_HPP
#define ACC_STORE_ENCODE_DATA_HPP

#include "acc_phase3.hpp"

namespace jxl {

bool ans_fuzzer_friendly_ = false;
static const int kMaxNumSymbolsForSmallCode = 4;

struct SizeWriterNew {
    size_t size = 0;
    void Write(size_t num, size_t bits) { size += num; }
};

template <typename Writer>
void StoreVarLenUint8New(size_t n, Writer* writer) {
    JXL_DASSERT(n <= 255);
    if (n == 0) {
        writer->Write(1, 0);
    } else {
        writer->Write(1, 1);
        size_t nbits = FloorLog2Nonzero(n);
        writer->Write(3, nbits);
        writer->Write(nbits, n - (1ULL << nbits));
    }
}

template <typename Writer>
void StoreVarLenUint16New(size_t n, Writer* writer) {
    JXL_DASSERT(n <= 65535);
    if (n == 0) {
        writer->Write(1, 0);
    } else {
        writer->Write(1, 1);
        size_t nbits = FloorLog2Nonzero(n);
        writer->Write(4, nbits);
        writer->Write(nbits, n - (1ULL << nbits));
    }
}

template <typename Writer>
void EncodeUintConfig(const HybridUintConfig uint_config, Writer* writer, size_t log_alpha_size) {
    writer->Write(CeilLog2Nonzero(log_alpha_size + 1), uint_config.split_exponent);
    if (uint_config.split_exponent == log_alpha_size) {
        return; // msb/lsb don't matter.
    }
    size_t nbits = CeilLog2Nonzero(uint_config.split_exponent + 1);
    writer->Write(nbits, uint_config.msb_in_token);
    nbits = CeilLog2Nonzero(uint_config.split_exponent - uint_config.msb_in_token + 1);
    writer->Write(nbits, uint_config.lsb_in_token);
}
template <typename Writer>
void EncodeUintConfigsNew(const std::vector<HybridUintConfig>& uint_config, Writer* writer, size_t log_alpha_size) {
    // TODO(veluca): RLE?
    for (size_t i = 0; i < uint_config.size(); i++) {
        EncodeUintConfig(uint_config[i], writer, log_alpha_size);
    }
}

void ANSBuildInfoTableNew(const ANSHistBin* counts,
                          const AliasTable::Entry* table,
                          size_t alphabet_size,
                          size_t log_alpha_size,
                          ANSEncSymbolInfo* info) {
    size_t log_entry_size = ANS_LOG_TAB_SIZE - log_alpha_size;
    size_t entry_size_minus_1 = (1 << log_entry_size) - 1;
    // create valid alias table for empty streams.
    for (size_t s = 0; s < std::max<size_t>(1, alphabet_size); ++s) {
        const ANSHistBin freq = s == alphabet_size ? ANS_TAB_SIZE : counts[s];
        info[s].freq_ = static_cast<uint16_t>(freq);
#ifdef USE_MULT_BY_RECIPROCAL
        if (freq != 0) {
            info[s].ifreq_ = ((1ull << RECIPROCAL_PRECISION) + info[s].freq_ - 1) / info[s].freq_;
        } else {
            info[s].ifreq_ = 1; // shouldn't matter (symbol shouldn't occur), but...
        }
#endif
        info[s].reverse_map_.resize(freq);
    }
    for (int i = 0; i < ANS_TAB_SIZE; i++) {
        AliasTable::Symbol s = AliasTable::Lookup(table, i, log_entry_size, entry_size_minus_1);
        info[s.value].reverse_map_[s.offset] = i;
    }
}

float EstimateDataBitsNew(const ANSHistBin* histogram, const ANSHistBin* counts, size_t len) {
    float sum = 0.0f;
    int total_histogram = 0;
    int total_counts = 0;
    for (size_t i = 0; i < len; ++i) {
        total_histogram += histogram[i];
        total_counts += counts[i];
        if (histogram[i] > 0) {
            JXL_ASSERT(counts[i] > 0);
            // += histogram[i] * -log(counts[i]/total_counts)
            sum += histogram[i] * std::max(0.0f, ANS_LOG_TAB_SIZE - FastLog2f(counts[i]));
        }
    }
    if (total_histogram > 0) {
        JXL_ASSERT(total_counts == ANS_TAB_SIZE);
    }
    return sum;
}

float EstimateDataBitsFlatNew(const ANSHistBin* histogram, size_t len) {
    const float flat_bits = std::max(FastLog2f(len), 0.0f);
    int total_histogram = 0;
    for (size_t i = 0; i < len; ++i) {
        total_histogram += histogram[i];
    }
    return total_histogram * flat_bits;
}

// Static Huffman code for encoding logcounts. The last symbol is used as RLE
// sequence.
static const uint8_t kLogCountBitLengths[ANS_LOG_TAB_SIZE + 2] = {
    5, 4, 4, 4, 4, 4, 3, 3, 3, 3, 3, 6, 7, 7,
};
static const uint8_t kLogCountSymbols[ANS_LOG_TAB_SIZE + 2] = {
    17, 11, 15, 3, 9, 7, 4, 2, 5, 6, 0, 33, 1, 65,
};

// Returns the difference between largest count that can be represented and is
// smaller than "count" and smallest representable count larger than "count".
static int SmallestIncrement(uint32_t count, uint32_t shift) {
    int bits = count == 0 ? -1 : FloorLog2Nonzero(count);
    int drop_bits = bits - GetPopulationCountPrecision(bits, shift);
    return drop_bits < 0 ? 1 : (1 << drop_bits);
}

template <bool minimize_error_of_sum>
bool RebalanceHistogramNew(
    const float* targets, int max_symbol, int table_size, uint32_t shift, int* omit_pos, ANSHistBin* counts) {
    int sum = 0;
    float sum_nonrounded = 0.0;
    int remainder_pos = 0; // if all of them are handled in first loop
    int remainder_log = -1;
    for (int n = 0; n < max_symbol; ++n) {
        if (targets[n] > 0 && targets[n] < 1.0f) {
            counts[n] = 1;
            sum_nonrounded += targets[n];
            sum += counts[n];
        }
    }
    const float discount_ratio = (table_size - sum) / (table_size - sum_nonrounded);
    JXL_ASSERT(discount_ratio > 0);
    JXL_ASSERT(discount_ratio <= 1.0f);
    // Invariant for minimize_error_of_sum == true:
    // abs(sum - sum_nonrounded)
    //   <= SmallestIncrement(max(targets[])) + max_symbol
    for (int n = 0; n < max_symbol; ++n) {
        if (targets[n] >= 1.0f) {
            sum_nonrounded += targets[n];
            counts[n] = static_cast<ANSHistBin>(targets[n] * discount_ratio); // truncate
            if (counts[n] == 0) counts[n] = 1;
            if (counts[n] == table_size) counts[n] = table_size - 1;
            // Round the count to the closest nonzero multiple of SmallestIncrement
            // (when minimize_error_of_sum is false) or one of two closest so as to
            // keep the sum as close as possible to sum_nonrounded.
            int inc = SmallestIncrement(counts[n], shift);
            counts[n] -= counts[n] & (inc - 1);
            // TODO(robryk): Should we rescale targets[n]?
            const float target = minimize_error_of_sum ? (sum_nonrounded - sum) : targets[n];
            if (counts[n] == 0 || (target > counts[n] + inc / 2 && counts[n] + inc < table_size)) {
                counts[n] += inc;
            }
            sum += counts[n];
            const int count_log = FloorLog2Nonzero(static_cast<uint32_t>(counts[n]));
            if (count_log > remainder_log) {
                remainder_pos = n;
                remainder_log = count_log;
            }
        }
    }
    JXL_ASSERT(remainder_pos != -1);
    // NOTE: This is the only place where counts could go negative. We could
    // detect that, return false and make ANSHistBin uint32_t.
    counts[remainder_pos] -= sum - table_size;
    *omit_pos = remainder_pos;
    return counts[remainder_pos] > 0;
}

Status NormalizeCountsNew(ANSHistBin* counts,
                          int* omit_pos,
                          const int length,
                          const int precision_bits,
                          uint32_t shift,
                          int* num_symbols,
                          int* symbols) {
    const int32_t table_size = 1 << precision_bits; // target sum / table size
    uint64_t total = 0;
    int max_symbol = 0;
    int symbol_count = 0;
    for (int n = 0; n < length; ++n) {
        total += counts[n];
        if (counts[n] > 0) {
            if (symbol_count < kMaxNumSymbolsForSmallCode) {
                symbols[symbol_count] = n;
            }
            ++symbol_count;
            max_symbol = n + 1;
        }
    }
    *num_symbols = symbol_count;
    if (symbol_count == 0) {
        return true;
    }
    if (symbol_count == 1) {
        counts[symbols[0]] = table_size;
        return true;
    }
    if (symbol_count > table_size) return JXL_FAILURE("Too many entries in an ANS histogram");

    // printf("%s: %s: %d, max_symbol=%d\n", __FILE__, __FUNCTION__, __LINE__, max_symbol);
    const float norm = 1.f * table_size / total;
    std::vector<float> targets(max_symbol);
    for (size_t n = 0; n < targets.size(); ++n) {
        targets[n] = norm * counts[n];
    }
    if (!RebalanceHistogramNew<false>(&targets[0], max_symbol, table_size, shift, omit_pos, counts)) {
        // Use an alternative rebalancing mechanism if the one above failed
        // to create a histogram that is positive wherever the original one was.
        if (!RebalanceHistogramNew<true>(&targets[0], max_symbol, table_size, shift, omit_pos, counts)) {
            return JXL_FAILURE("Logic error: couldn't rebalance a histogram");
        }
    }
    return true;
}

template <typename Writer>
bool EncodeCountsNew(const ANSHistBin* counts,
                     const int alphabet_size,
                     const int omit_pos,
                     const int num_symbols,
                     uint32_t shift,
                     const int* symbols,
                     Writer* writer) {
    bool ok = true;
    if (num_symbols <= 2) {
        // Small tree marker to encode 1-2 symbols.
        writer->Write(1, 1);
        if (num_symbols == 0) {
            writer->Write(1, 0);
            StoreVarLenUint8New(0, writer);
        } else {
            writer->Write(1, num_symbols - 1);
            for (int i = 0; i < num_symbols; ++i) {
                StoreVarLenUint8New(symbols[i], writer);
            }
        }
        if (num_symbols == 2) {
            writer->Write(ANS_LOG_TAB_SIZE, counts[symbols[0]]);
        }
    } else {
        // Mark non-small tree.
        writer->Write(1, 0);
        // Mark non-flat histogram.
        writer->Write(1, 0);

        // Precompute sequences for RLE encoding. Contains the number of identical
        // values starting at a given index. Only contains the value at the first
        // element of the series.
        std::vector<uint32_t> same(alphabet_size, 0);
        int last = 0;
        for (int i = 1; i < alphabet_size; i++) {
            // Store the sequence length once different symbol reached, or we're at
            // the end, or the length is longer than we can encode, or we are at
            // the omit_pos. We don't support including the omit_pos in an RLE
            // sequence because this value may use a different amount of log2 bits
            // than standard, it is too complex to handle in the decoder.
            if (counts[i] != counts[last] || i + 1 == alphabet_size || (i - last) >= 255 || i == omit_pos ||
                i == omit_pos + 1) {
                same[last] = (i - last);
                last = i + 1;
            }
        }

        int length = 0;
        std::vector<int> logcounts(alphabet_size);
        int omit_log = 0;
        for (int i = 0; i < alphabet_size; ++i) {
            JXL_ASSERT(counts[i] <= ANS_TAB_SIZE);
            JXL_ASSERT(counts[i] >= 0);
            if (i == omit_pos) {
                length = i + 1;
            } else if (counts[i] > 0) {
                logcounts[i] = FloorLog2Nonzero(static_cast<uint32_t>(counts[i])) + 1;
                length = i + 1;
                if (i < omit_pos) {
                    omit_log = std::max(omit_log, logcounts[i] + 1);
                } else {
                    omit_log = std::max(omit_log, logcounts[i]);
                }
            }
        }
        logcounts[omit_pos] = omit_log;

        // Elias gamma-like code for shift. Only difference is that if the number
        // of bits to be encoded is equal to FloorLog2(ANS_LOG_TAB_SIZE+1), we skip
        // the terminating 0 in unary coding.
        int upper_bound_log = FloorLog2Nonzero(ANS_LOG_TAB_SIZE + 1);
        int log = FloorLog2Nonzero(shift + 1);
        writer->Write(log, (1 << log) - 1);
        if (log != upper_bound_log) writer->Write(1, 0);
        writer->Write(log, ((1 << log) - 1) & (shift + 1));

        // Since num_symbols >= 3, we know that length >= 3, therefore we encode
        // length - 3.
        if (length - 3 > 255) {
            // Pretend that everything is OK, but complain about correctness later.
            StoreVarLenUint8New(255, writer);
            ok = false;
        } else {
            StoreVarLenUint8New(length - 3, writer);
        }

        // The logcount values are encoded with a static Huffman code.
        static const size_t kMinReps = 4;
        size_t rep = ANS_LOG_TAB_SIZE + 1;
        // printf("%s: %s: %d, length=%d\n", __FILE__, __FUNCTION__, __LINE__, length);
        for (int i = 0; i < length; ++i) {
            if (i > 0 && same[i - 1] > kMinReps) {
                // Encode the RLE symbol and skip the repeated ones.
                writer->Write(kLogCountBitLengths[rep], kLogCountSymbols[rep]);
                StoreVarLenUint8New(same[i - 1] - kMinReps - 1, writer);
                i += same[i - 1] - 2;
                continue;
            }
            writer->Write(kLogCountBitLengths[logcounts[i]], kLogCountSymbols[logcounts[i]]);
        }
        for (int i = 0; i < length; ++i) {
            if (i > 0 && same[i - 1] > kMinReps) {
                // Skip symbols encoded by RLE.
                i += same[i - 1] - 2;
                continue;
            }
            if (logcounts[i] > 1 && i != omit_pos) {
                int bitcount = GetPopulationCountPrecision(logcounts[i] - 1, shift);
                int drop_bits = logcounts[i] - 1 - bitcount;
                JXL_CHECK((counts[i] & ((1 << drop_bits) - 1)) == 0);
                writer->Write(bitcount, (counts[i] >> drop_bits) - (1 << bitcount));
            }
        }
    }
    return ok;
}

void EncodeFlatHistogramNew(const int alphabet_size, BitWriter* writer) {
    // Mark non-small tree.
    writer->Write(1, 0);
    // Mark uniform histogram.
    writer->Write(1, 1);
    JXL_ASSERT(alphabet_size > 0);
    // Encode alphabet size.
    StoreVarLenUint8New(alphabet_size - 1, writer);
}

float ComputeHistoAndDataCostNew(const ANSHistBin* histogram, size_t alphabet_size, uint32_t method) {
    if (method == 0) { // Flat code
        return ANS_LOG_TAB_SIZE + 2 + EstimateDataBitsFlatNew(histogram, alphabet_size);
    }
    // Non-flat: shift = method-1.
    uint32_t shift = method - 1;
    std::vector<ANSHistBin> counts(histogram, histogram + alphabet_size);
    int omit_pos = 0;
    int num_symbols;
    int symbols[kMaxNumSymbolsForSmallCode] = {};
    JXL_CHECK(
        NormalizeCountsNew(counts.data(), &omit_pos, alphabet_size, ANS_LOG_TAB_SIZE, shift, &num_symbols, symbols));
    SizeWriterNew writer;
    // Ignore the correctness, no real encoding happens at this stage.
    (void)EncodeCountsNew(counts.data(), alphabet_size, omit_pos, num_symbols, shift, symbols, &writer);
    return writer.size + EstimateDataBitsNew(histogram, counts.data(), alphabet_size);
}

uint32_t ComputeBestMethodNew(const ANSHistBin* histogram,
                              size_t alphabet_size,
                              float* cost,
                              HistogramParams::ANSHistogramStrategy ans_histogram_strategy) {
    size_t method = 0;
    float fcost = ComputeHistoAndDataCostNew(histogram, alphabet_size, 0);
    // printf("%s: %s: %d, ANS_LOG_TAB_SIZE=%d, ans_histogram_strategy=%d\n", __FILE__, __FUNCTION__, __LINE__,
    //  ANS_LOG_TAB_SIZE, ans_histogram_strategy != HistogramParams::ANSHistogramStrategy::kPrecise);
    for (uint32_t shift = 0; shift <= ANS_LOG_TAB_SIZE;
         ans_histogram_strategy != HistogramParams::ANSHistogramStrategy::kPrecise ? shift += 2 : shift++) {
        float c = ComputeHistoAndDataCostNew(histogram, alphabet_size, shift + 1);
        if (c < fcost) {
            method = shift + 1;
            fcost = c;
        } else if (ans_histogram_strategy == HistogramParams::ANSHistogramStrategy::kFast) {
            // do not be as precise if estimating cost.
            break;
        }
    }
    // printf("%s: %s: %d, alphabet_size=%zu, method=%zu, fcost=%f, ANS_TAB_SIZE=%d\n",
    //  __FILE__, __FUNCTION__, __LINE__,
    //  alphabet_size, method, fcost);
    *cost = fcost;
    return method;
}

size_t BuildAndStoreANSEncodingDataNew(HistogramParams::ANSHistogramStrategy ans_histogram_strategy,
                                       const ANSHistBin* histogram,
                                       size_t alphabet_size,
                                       size_t log_alpha_size,
                                       bool use_prefix_code,
                                       ANSEncSymbolInfo* info,
                                       BitWriter* writer) {
    // printf("%s: %s: %d, ans_histogram_strategy=%d, alphabet_size=%zu, log_alpha_size=%zu, ANS_TAB_SIZE=%d,
    // ANS_MAX_ALPHABET_SIZE=%d, ANS_LOG_TAB_SIZE=%d\n",
    //  __FILE__, __FUNCTION__, __LINE__,
    //  ans_histogram_strategy, alphabet_size, log_alpha_size, ANS_TAB_SIZE, ANS_MAX_ALPHABET_SIZE, ANS_LOG_TAB_SIZE);
    if (use_prefix_code) {
        if (alphabet_size <= 1) return 0;
        std::vector<uint32_t> histo(alphabet_size);
        for (size_t i = 0; i < alphabet_size; i++) {
            histo[i] = histogram[i];
            JXL_CHECK(histogram[i] >= 0);
        }
        size_t cost = 0;
        {
            std::vector<uint8_t> depths(alphabet_size);
            std::vector<uint16_t> bits(alphabet_size);
            BitWriter tmp_writer;
            BitWriter* w = writer ? writer : &tmp_writer;
            size_t start = w->BitsWritten();
            BitWriter::Allotment allotment(w, 8 * alphabet_size + 8); // safe upper bound
            BuildAndStoreHuffmanTree(histo.data(), alphabet_size, depths.data(), bits.data(), w);
            ReclaimAndCharge(w, &allotment, 0, /*aux_out=*/nullptr);

            for (size_t i = 0; i < alphabet_size; i++) {
                info[i].bits = depths[i] == 0 ? 0 : bits[i];
                info[i].depth = depths[i];
            }
            cost = w->BitsWritten() - start;
        }
        // Estimate data cost.
        for (size_t i = 0; i < alphabet_size; i++) {
            cost += histogram[i] * info[i].depth;
        }
        return cost;
    }
    JXL_ASSERT(alphabet_size <= ANS_TAB_SIZE);
    // Ensure we ignore trailing zeros in the histogram.
    if (alphabet_size != 0) {
        size_t largest_symbol = 0;
        for (size_t i = 0; i < alphabet_size; i++) {
            if (histogram[i] != 0) largest_symbol = i;
        }
        alphabet_size = largest_symbol + 1;
    }
    // printf("%s: %s: %d, updated alphabet_size=%zu\n", __FILE__, __FUNCTION__, __LINE__, alphabet_size);
    float cost;
    uint32_t method = ComputeBestMethodNew(histogram, alphabet_size, &cost, ans_histogram_strategy);
    JXL_ASSERT(cost >= 0);
    int num_symbols;
    int symbols[kMaxNumSymbolsForSmallCode] = {};
    std::vector<ANSHistBin> counts(histogram, histogram + alphabet_size);
    if (!counts.empty()) {
        size_t sum = 0;
        for (size_t i = 0; i < counts.size(); i++) {
            sum += counts[i];
        }
        if (sum == 0) {
            counts[0] = ANS_TAB_SIZE;
        }
    }
    if (method == 0) {
        counts = CreateFlatHistogram(alphabet_size, ANS_TAB_SIZE);
        AliasTable::Entry a[ANS_MAX_ALPHABET_SIZE];
        InitAliasTable(counts, ANS_TAB_SIZE, log_alpha_size, a);
        ANSBuildInfoTableNew(counts.data(), a, alphabet_size, log_alpha_size, info);
        if (writer != nullptr) {
            EncodeFlatHistogramNew(alphabet_size, writer);
        }
        return cost;
    }
    int omit_pos = 0;
    uint32_t shift = method - 1;
    JXL_CHECK(
        NormalizeCountsNew(counts.data(), &omit_pos, alphabet_size, ANS_LOG_TAB_SIZE, shift, &num_symbols, symbols));
    AliasTable::Entry a[ANS_MAX_ALPHABET_SIZE];
    InitAliasTable(counts, ANS_TAB_SIZE, log_alpha_size, a);
    ANSBuildInfoTableNew(counts.data(), a, alphabet_size, log_alpha_size, info);
    if (writer != nullptr) {
        bool ok = EncodeCountsNew(counts.data(), alphabet_size, omit_pos, num_symbols, shift, symbols, writer);
        (void)ok;
        JXL_DASSERT(ok);
    }
    return cost;
}

float ANSPopulationCostNew(const ANSHistBin* data, size_t alphabet_size) {
    float c;
    ComputeBestMethodNew(data, alphabet_size, &c, HistogramParams::ANSHistogramStrategy::kFast);
    return c;
}

size_t StoreEntropyCodesNew(const HistogramParams& params,
                            const std::vector<std::vector<Token> >& tokens,
                            EntropyEncodingData* codes,
                            bool use_prefix_code,
                            BitWriter* writer,
                            size_t layer,
                            AuxOut* aux_out,
                            std::vector<Histogram> clustered_histograms) {
    size_t cost = 0;
    codes->use_prefix_code = use_prefix_code;
    size_t log_alpha_size = codes->lz77.enabled ? 8 : 7; // Sane default.
    if (ans_fuzzer_friendly_) {
        codes->uint_config.clear();
        codes->uint_config.resize(1, HybridUintConfig(7, 0, 0));
    } else {
        codes->uint_config.resize(clustered_histograms.size());
        if (params.uint_method == HistogramParams::HybridUintMethod::kContextMap) {
            codes->uint_config.clear();
            codes->uint_config.resize(clustered_histograms.size(), HybridUintConfig(2, 0, 1));
        }
    }
    if (log_alpha_size < 5) log_alpha_size = 5;
    SizeWriterNew size_writer; // Used if writer == nullptr to estimate costs.
    cost += 1;
    if (writer) writer->Write(1, use_prefix_code);
    if (use_prefix_code) {
        log_alpha_size = PREFIX_MAX_BITS;
    } else {
        cost += 2;
    }
    if (writer == nullptr) {
        EncodeUintConfigsNew(codes->uint_config, &size_writer, log_alpha_size);
    } else {
        if (!use_prefix_code) writer->Write(2, log_alpha_size - 5);
        EncodeUintConfigsNew(codes->uint_config, writer, log_alpha_size);
    }
    if (use_prefix_code) {
        for (size_t c = 0; c < clustered_histograms.size(); ++c) {
            size_t num_symbol = 1;
            for (size_t i = 0; i < clustered_histograms[c].data_.size(); i++) {
                if (clustered_histograms[c].data_[i]) num_symbol = i + 1;
            }
            if (writer) {
                StoreVarLenUint16New(num_symbol - 1, writer);
            } else {
                StoreVarLenUint16New(num_symbol - 1, &size_writer);
            }
        }
    }
    cost += size_writer.size;
    // printf("%s: %s: %d, final clustered_histograms size=%zu\n", __FILE__, __FUNCTION__, __LINE__,
    // clustered_histograms.size());
    for (size_t c = 0; c < clustered_histograms.size(); ++c) {
        size_t num_symbol = 1;
        // printf("%s: %s: %d, final clustered_histograms data size=%zu\n", __FILE__, __FUNCTION__, __LINE__,
        // clustered_histograms[c].data_.size());
        for (size_t i = 0; i < clustered_histograms[c].data_.size(); i++) {
            if (clustered_histograms[c].data_[i]) num_symbol = i + 1;
        }
        codes->encoding_info.emplace_back();
        codes->encoding_info.back().resize(std::max<size_t>(1, num_symbol));
        // printf("%s: %s: %d, encoding_info size=%zu, adder=%zu\n", __FILE__, __FUNCTION__, __LINE__,
        // codes->encoding_info.size(), num_symbol);
        BitWriter::Allotment allotment(writer, 256 + num_symbol * 24);
        cost += BuildAndStoreANSEncodingDataNew(params.ans_histogram_strategy, clustered_histograms[c].data_.data(),
                                                num_symbol, log_alpha_size, use_prefix_code,
                                                codes->encoding_info.back().data(), writer);
        allotment.FinishedHistogram(writer);
        ReclaimAndCharge(writer, &allotment, layer, aux_out);
    }
    return cost;
}
} // namespace jxl

#endif
