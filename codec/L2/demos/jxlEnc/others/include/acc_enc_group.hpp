// Copyright (c) the JPEG XL Project Authors. All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#ifndef LIB_JXL_ENC_GROUP_H_
#define LIB_JXL_ENC_GROUP_H_

#include <stddef.h>
#include <stdint.h>

#include "lib/jxl/aux_out_fwd.h"
#include "lib/jxl/base/status.h"
#include "lib/jxl/enc_bit_writer.h"
#include "lib/jxl/enc_cache.h"

namespace jxl {

// Fills DC
void ComputeCoefficients(size_t group_idx,
                         PassesEncoderState* enc_state,
                         const Image3F& opsin,
                         Image3F* dc,
                         size_t xsize,
                         size_t ysize,
                         std::vector<std::vector<float> >& dctIDT,
                         std::vector<std::vector<float> >& dct2x2,
                         std::vector<std::vector<float> >& dct4x4,
                         std::vector<std::vector<float> >& dct8x8,
                         std::vector<std::vector<float> >& dct16x16,
                         std::vector<std::vector<float> >& dct32x32,
                         std::vector<std::vector<float> >& dcIDT,
                         std::vector<std::vector<float> >& dc2x2,
                         std::vector<std::vector<float> >& dc4x4,
                         std::vector<std::vector<float> >& dc8x8,
                         std::vector<std::vector<float> >& dc16x16,
                         std::vector<std::vector<float> >& dc32x32);

Status EncodeGroupTokenizedCoefficients(size_t group_idx,
                                        size_t pass_idx,
                                        size_t histogram_idx,
                                        const PassesEncoderState& enc_state,
                                        BitWriter* writer,
                                        AuxOut* aux_out);

} // namespace jxl

#endif // LIB_JXL_ENC_GROUP_H_
