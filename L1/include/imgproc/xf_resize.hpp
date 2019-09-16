/***************************************************************************
Copyright (c) 2019, Xilinx, Inc.
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

#ifndef _XF_RESIZE_
#define _XF_RESIZE_

#include "xf_resize_headers.h"

/**
 * Image resizing function.
 */
namespace xf {
namespace cv {

template <int INTERPOLATION_TYPE,
          int TYPE,
          int SRC_ROWS,
          int SRC_COLS,
          int DST_ROWS,
          int DST_COLS,
          int NPC,
          int MAX_DOWN_SCALE>
void resize(xf::cv::Mat<TYPE, SRC_ROWS, SRC_COLS, NPC>& _src, xf::cv::Mat<TYPE, DST_ROWS, DST_COLS, NPC>& _dst) {
    // clang-format off
    #pragma HLS INLINE OFF
    // clang-format on

    assert(((INTERPOLATION_TYPE == XF_INTERPOLATION_NN) || (INTERPOLATION_TYPE == XF_INTERPOLATION_BILINEAR) ||
            (INTERPOLATION_TYPE == XF_INTERPOLATION_AREA)) &&
           "Incorrect parameters interpolation type");

    if (INTERPOLATION_TYPE == XF_INTERPOLATION_AREA) {
        assert(((_src.rows <= SRC_ROWS) && (_src.cols <= SRC_COLS)) &&
               "SRC_ROWS and SRC_COLS should be greater than input image");
        assert(((_dst.rows <= DST_ROWS) && (_dst.cols <= DST_COLS)) &&
               "DST_ROWS and DST_COLS should be greater than output image");

        if ((SRC_ROWS < DST_ROWS) || (SRC_COLS < DST_COLS)) {
            xFResizeAreaUpScale<SRC_ROWS, SRC_COLS, XF_CHANNELS(TYPE, NPC), TYPE, NPC, XF_WORDWIDTH(TYPE, NPC),
                                DST_ROWS, DST_COLS, (SRC_COLS >> XF_BITSHIFT(NPC)), (DST_COLS >> XF_BITSHIFT(NPC))>(
                _src, _dst);
        } else if ((SRC_ROWS >= DST_ROWS) || (SRC_COLS >= DST_COLS)) {
            xFResizeAreaDownScale<SRC_ROWS, SRC_COLS, XF_CHANNELS(TYPE, NPC), TYPE, NPC, XF_WORDWIDTH(TYPE, NPC),
                                  DST_ROWS, DST_COLS, (SRC_COLS >> XF_BITSHIFT(NPC)), (DST_COLS >> XF_BITSHIFT(NPC))>(
                _src, _dst);
        }

        return;
    } else {
        resizeNNBilinear<TYPE, SRC_ROWS, SRC_COLS, NPC, DST_ROWS, DST_COLS, INTERPOLATION_TYPE, MAX_DOWN_SCALE>(_src,
                                                                                                                _dst);
    }
}
} // namespace cv
} // namespace xf
#endif
