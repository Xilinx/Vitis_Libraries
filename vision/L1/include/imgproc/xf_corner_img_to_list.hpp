/***************************************************************************
Copyright (c) 2016, Xilinx, Inc.
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors
may be used to endorse or promote products derived from this software
without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

 ***************************************************************************/
#ifndef __XF_CORNER_DENSE_TO_SPARSE_HPP__
#define __XF_CORNER_DENSE_TO_SPARSE_HPP__
#include "ap_int.h"
#include "hls_stream.h"
#include "common/xf_common.h"
namespace xf {
namespace cv {
template <unsigned int MAXCORNERSNO, unsigned int TYPE, unsigned int ROWS, unsigned int COLS, unsigned int NPC>
void cornersImgToList(xf::cv::Mat<TYPE, ROWS, COLS, NPC>& _src,
                      unsigned int list[MAXCORNERSNO],
                      unsigned int* ncorners) {
    int cornerCount = 0;
    for (unsigned short i = 0; i < _src.rows; i++) {
        for (unsigned short j = 0; j < _src.cols; j++) {
            // clang-format off
            #pragma HLS PIPELINE
            // clang-format on
            ap_uint<8> tempValue = _src.read(i * _src.cols + j);
            if (tempValue == 255 && cornerCount < MAXCORNERSNO) // value is 255 if there's a corner
            {
                ap_uint<32> point;
                point.range(31, 16) = i;
                point.range(15, 0) = j;

                list[cornerCount] = (unsigned int)point;
                cornerCount++;
            }
        }
    }
    *ncorners = cornerCount;
    for (int i = cornerCount; i < MAXCORNERSNO; i++) {
        // clang-format off
        #pragma HLS PIPELINE
        // clang-format on
        list[i] = 0;
    }
}
} // namespace cv
} // namespace xf
#endif
