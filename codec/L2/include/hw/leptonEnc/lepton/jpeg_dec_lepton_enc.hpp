/*
 * Copyright 2021 Xilinx, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef _JPEG_DEC_LEPTON_ENC_HPP_
#define _JPEG_DEC_LEPTON_ENC_HPP_

/**
 * @file jpeg_dec_lepton_enc.hpp
 * @brief interface of IMAGE Jpeg Decoder Lepton Encoder internal kernel.
 */

#include "XAcc_common.hpp"
#include "XAcc_model.hpp"
#include "XAcc_lepjpegdecoder.hpp"
#include "XAcc_lepjfifparser.hpp"
#include "XAcc_arith.hpp"
#include "XAcc_77.hpp"
#include "XAcc_edges.hpp"
#include "XAcc_dc.hpp"
#include "stream_to_axi.hpp"

namespace xf {
namespace codec {
// ------------------------------------------------------------
/**
 * @brief IMGAE Jpeg Decoder Lepton Encoder internal kernel
 * \rst
 * For detailed document, see :ref:`JpegD_LeptonE_kernel_design`.
 * \endrst
 * @param datainDDR input image buffer.
 * @param jpgSize size of input image buffer.
 * @param arithInfo meta information of output buffer.
 * @param res output lepton format data buffer.
 */

void jpegDecLeptonEnc(ap_uint<AXI_WIDTH>* datainDDR, int jpgSize, int* arithInfo, ap_uint<8>* res);

} // namespace codec
} // namespace xf

#endif
