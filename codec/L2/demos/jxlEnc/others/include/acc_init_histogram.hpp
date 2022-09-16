// Copyright (c) the JPEG XL Project Authors. All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#ifndef ACC_INIT_HISTOGRAM_HPP
#define ACCC_INIT_HISTOGRAM_HPP

#include "acc_phase3.hpp"

namespace jxl {
bool acc_InitHistogram(std::vector<Histogram>& histograms, std::vector<std::vector<Token> >& tokens);

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
                          std::vector<std::vector<Histogram> >& histograms_);
} // namespace jxl

#endif
