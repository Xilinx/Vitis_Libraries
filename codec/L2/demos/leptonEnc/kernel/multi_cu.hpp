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
#ifndef _MULTIKERNEL_HPP_
#define _MULTIKERNEL_HPP_

/**
 * @file jpeg_decoder_lepton_encoder_kernel.h
 * @brief interface of IMAGE Jpeg Decoder Lepton Encoder kernel.
 */

#include "jpeg_dec_lepton_enc.hpp"

extern "C" void lepEnc(ap_uint<AXI_WIDTH>* datainDDR, int jpgSize, int* arithInfo, ap_uint<8>* res);

/*extern "C" {
void jpegDecLeptonEncKernel_0(ap_uint<AXI_WIDTH>* datainDDR,
                              int jpgSize,
                              // struct_arith& arith,
                              int* arithInfo,
                              uint8_t* res);

void jpegDecLeptonEncKernel_1(ap_uint<AXI_WIDTH>* datainDDR,
                              int jpgSize,
                              // struct_arith& arith,
                              int* arithInfo,
                              uint8_t* res);

void jpegDecLeptonEncKernel_2(ap_uint<AXI_WIDTH>* datainDDR,
                              int jpgSize,
                              // struct_arith& arith,
                              int* arithInfo,
                              uint8_t* res);

void jpegDecLeptonEncKernel_3(ap_uint<AXI_WIDTH>* datainDDR,
                              int jpgSize,
                              // struct_arith& arith,
                              int* arithInfo,
                              uint8_t* res);

void jpegDecLeptonEncKernel_4(ap_uint<AXI_WIDTH>* datainDDR,
                              int jpgSize,
                              // struct_arith& arith,
                              int* arithInfo,
                              uint8_t* res);

void jpegDecLeptonEncKernel_5(ap_uint<AXI_WIDTH>* datainDDR,
                              int jpgSize,
                              // struct_arith& arith,
                              int* arithInfo,
                              uint8_t* res);

void jpegDecLeptonEncKernel_6(ap_uint<AXI_WIDTH>* datainDDR,
                              int jpgSize,
                              // struct_arith& arith,
                              int* arithInfo,
                              uint8_t* res);

void jpegDecLeptonEncKernel_7(ap_uint<AXI_WIDTH>* datainDDR,
                              int jpgSize,
                              // struct_arith& arith,
                              int* arithInfo,
                              uint8_t* res);

void jpegDecLeptonEncKernel_8(ap_uint<AXI_WIDTH>* datainDDR,
                              int jpgSize,
                              // struct_arith& arith,
                              int* arithInfo,
                              uint8_t* res);

void jpegDecLeptonEncKernel_9(ap_uint<AXI_WIDTH>* datainDDR,
                              int jpgSize,
                              // struct_arith& arith,
                              int* arithInfo,
                              uint8_t* res);

void jpegDecLeptonEncKernel_10(ap_uint<AXI_WIDTH>* datainDDR,
                               int jpgSize,
                               // struct_arith& arith,
                               int* arithInfo,
                               uint8_t* res);

void jpegDecLeptonEncKernel_11(ap_uint<AXI_WIDTH>* datainDDR,
                               int jpgSize,
                               // struct_arith& arith,
                               int* arithInfo,
                               uint8_t* res);
}
*/
#endif
