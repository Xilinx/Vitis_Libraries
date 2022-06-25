/*
 * Copyright 2019 Xilinx, Inc.
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

/**
 * @file kernelJpegDecoder.hpp
 * @brief kernelJpegDecoder template function implementation and kernel_decoder warpper.
 *
 * This file is part of HLS algorithm library.
 */

#ifndef _XF_CODEC_KERNEL_JPEG_DEC_SC_HPP_
#define _XF_CODEC_KERNEL_JPEG_DEC_SC_HPP_

#include "XAcc_jpegdecoder.hpp"
#include "XAcc_jfifparser.hpp"
#include "XAcc_idct.hpp"

#include "vpp_acc.hpp"

// ------------------------------------------------------------
/**
 * @brief Level 2 : kernel for jfif parser + huffman decoder + iQ_iDCT
 *
 * @tparam CH_W size of data path in dataflow region, in bit.
 *         when CH_W is 16, the decoder could decode one symbol per cycle in about 99% cases.
 *         when CH_W is 8 , the decoder could decode one symbol per cycle in about 80% cases, but use less resource.
 *
 * @param jpeg_pointer the input jpeg to be read from DDR.
 * @param size the total bytes to be read from DDR.
 * @param yuv_mcu_pointer the output yuv to DDR in mcu order. 1 ap_uint<64> has 8 uint8_t pixels after idct.
 * @param info information of the image, maybe use in the recovery image.
 */
// a.input the jpg 420/422/444 baseline file
// b.output the as the 8x8 's Column scan order YUV (0~255), like [Y*allpixels,U*0.5*allpixels, V*0.5*allpixels], and
// image infos
// c.Fault tolerance: If the picture's format is incorrect, error codes will directly end the kernel
// and wait for the input of the next image. Error codes cloud help to position at which decoding stage does the error
// occur
// d.performance: input throughput: 150MB/s~300MB/s(1symbol/clk), output 1~1.6GB/s (max 8B/clk),
// frequency 250MHz for kernel, for only huffman core 286MHz by vivado 2018.3

class jpegDec_acc : public VPP_ACC<jpegDec_acc, 1> {
    // port bindings
    ZERO_COPY(jpeg_pointer);
    // ZERO_COPY(size);
    ZERO_COPY(yuv_mcu_pointer);
    ZERO_COPY(infos);

    SYS_PORT(jpeg_pointer, DDR[0]);
    SYS_PORT(yuv_mcu_pointer, DDR[0]);
    SYS_PORT(infos, DDR[0]);

    SYS_PORT_PFM(u50, jpeg_pointer, HBM[0]);
    SYS_PORT_PFM(u50, yuv_mcu_pointer, HBM[1]);
    SYS_PORT_PFM(u50, infos, HBM[2]);

   public:
    static void compute(ap_uint<AXI_WIDTH>* jpeg_pointer,
                        const int size,
                        ap_uint<64>* yuv_mcu_pointer,
                        ap_uint<32>* infos);
    static void JDK(ap_uint<AXI_WIDTH>* jpeg_pointer, const int size, ap_uint<64>* yuv_mcu_pointer, ap_uint<32>* infos);
};

#endif // _XF_CODEC_KERNEL_JPEG_DEC_SC_HPP_