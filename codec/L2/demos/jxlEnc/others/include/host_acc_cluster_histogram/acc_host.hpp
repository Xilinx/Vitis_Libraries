// Copyright (c) the JPEG XL Project Authors. All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#ifndef ACC_HOST_HPP
#define ACC_HOST_HPP

#include "acc_common.hpp"
#include "acc_phase1.hpp"
#include "acc_phase2.hpp"
#include "acc_phase3.hpp"

namespace jxl {

Status acc_host(std::string xclbinPath,
                Image3F& opsin,
                LossyFrameEncoder& lossy_frame_encoder,
                const ImageBundle* JXL_RESTRICT ib_or_linear,
                ThreadPool* pool,
                std::unique_ptr<ModularFrameEncoder>& modular_frame_encoder,
                BitWriter* writer,
                AuxOut* aux_out,
                std::unique_ptr<FrameHeader>& frame_header,
                const FrameInfo& frame_info,
                CompressParams cparams,
                const std::vector<ImageF>* extra_channels,
                PassesEncoderState* passes_enc_state,
                FrameDimensions frame_dim,
                const size_t num_groups,
                const ImageBundle& ib,
                std::vector<AuxOut>& aux_outs,
                const std::function<Status(size_t)>& resize_aux_outs);
}
#endif
