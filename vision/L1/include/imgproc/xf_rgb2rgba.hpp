/*
 * Copyright (C) 2019-2022, Xilinx, Inc.
 * Copyright (C) 2022-2026, Advanced Micro Devices, Inc.
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

#ifndef _XF_RGB2RGBA_HPP_
#define _XF_RGB2RGBA_HPP_

#include "ap_int.h"
#include "hls_stream.h"

namespace xf {
namespace cv {

template <int IN_TYPE,
          int OUT_TYPE,
          int HEIGHT,
          int WIDTH,
          int NPC,
          int XFCVDEPTH_IN = _XFCVDEPTH_DEFAULT,
          int XFCVDEPTH_OUT = _XFCVDEPTH_DEFAULT>
void rgb2rgba(xf::cv::Mat<IN_TYPE, HEIGHT, WIDTH, NPC, XFCVDEPTH_IN>& in_mat,
              xf::cv::Mat<OUT_TYPE, HEIGHT, WIDTH, NPC, XFCVDEPTH_OUT>& out_mat) {
    XF_TNAME(OUT_TYPE, NPC) out_pack = 0;
    XF_DTUNAME(OUT_TYPE, NPC) out_val = 0;
    short int width = in_mat.cols >> XF_BITSHIFT(NPC);
    const int bitdepth = XF_DTPIXELDEPTH(IN_TYPE, NPC);
    const int in_channels = XF_CHANNELS(IN_TYPE, NPC);
    const int out_channels = XF_CHANNELS(OUT_TYPE, NPC);
    const int pixeldepth_in = bitdepth * in_channels;
    const int pixeldepth_out = bitdepth * out_channels;

    for (int i = 0; i < in_mat.rows; i++) {
// clang-format off
#pragma HLS PIPELINE II=1
#pragma HLS LOOP_TRIPCOUNT min=1 max=HEIGHT
        // clang-format on

        for (int j = 0; j < width; j++) {
// clang-format off
#pragma HLS UNROLL
#pragma HLS LOOP_TRIPCOUNT min=1 max=WIDTH/NPC
        XF_TNAME(IN_TYPE, NPC) x_pack = in_mat.read(i * width + j);
            // clang-format on
            for (int k = 0; k < XF_NPIXPERCYCLE(NPC); k++) {
// clang-format off
#pragma HLS UNROLL
		out_pack.range((k*bitdepth * out_channels) + (bitdepth * in_channels)-1, (k*bitdepth * out_channels)) = x_pack.range(((k+1)*bitdepth * in_channels)-1, k*bitdepth * in_channels );
		out_pack.range(((k+1)*bitdepth*out_channels)-1, ((k+1)*bitdepth*out_channels)-bitdepth) = 255;
/*        if(i==2 && j==2){
            std::cout << "Range RGB : " << (k*bitdepth * out_channels) + (bitdepth * in_channels)-1 << ", " << (k*bitdepth * out_channels) << std::endl;
            std::cout << "Range Alpha : " << ((k+1)*bitdepth*out_channels)-1 << ", " << ((k+1)*bitdepth*out_channels)-bitdepth << std::endl;
        }*/
		}
        out_mat.write((i*width + j), out_pack);
	}
	}
}
}//cv
}//xf

#endif
