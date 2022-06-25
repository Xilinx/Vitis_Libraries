// Copyright (c) the JPEG XL Project Authors. All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#ifndef ACC_CLUSTER_HISTOGRAM_HPP
#define ACC_CLUSTER_HISTOGRAM_HPP

#include "acc_phase3.hpp"

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
                             std::vector<std::vector<uint8_t> >& context_map_in);

} // namespace jxl

#endif
