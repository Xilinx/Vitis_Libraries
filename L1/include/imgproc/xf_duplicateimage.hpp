#ifndef _XF_Duplicate_HPP_
#define _XF_Duplicate_HPP_

#ifndef __cplusplus
#error C++ is needed to include this header
#endif

#include "hls_stream.h"
#include "common/xf_common.h"
#include "common/xf_utility.h"

namespace xf {
namespace cv {
template <int ROWS, int COLS, int SRC_T, int DEPTH, int NPC, int WORDWIDTH>
void xFDuplicate(hls::stream<XF_TNAME(SRC_T, NPC)>& _src_mat,
                 hls::stream<XF_TNAME(SRC_T, NPC)>& _dst1_mat,
                 hls::stream<XF_TNAME(SRC_T, NPC)>& _dst2_mat,
                 uint16_t img_height,
                 uint16_t img_width) {
    img_width = img_width >> XF_BITSHIFT(NPC);

    ap_uint<13> row, col;
Row_Loop:
    for (row = 0; row < img_height; row++) {
        // clang-format off
        #pragma HLS LOOP_TRIPCOUNT min=ROWS max=ROWS
        #pragma HLS LOOP_FLATTEN off
        // clang-format on
    Col_Loop:
        for (col = 0; col < img_width; col++) {
            // clang-format off
            #pragma HLS LOOP_TRIPCOUNT min=240 max=240
            #pragma HLS pipeline
            // clang-format on
            XF_TNAME(SRC_T, NPC) tmp_src;
            tmp_src = _src_mat.read();
            _dst1_mat.write(tmp_src);
            _dst2_mat.write(tmp_src);
        }
    }
}

template <int SRC_T, int ROWS, int COLS, int NPC>
void duplicateMat(xf::cv::Mat<SRC_T, ROWS, COLS, NPC>& _src,
                  xf::cv::Mat<SRC_T, ROWS, COLS, NPC>& _dst1,
                  xf::cv::Mat<SRC_T, ROWS, COLS, NPC>& _dst2) {
    // clang-format off
    #pragma HLS inline off
    // clang-format on

    // clang-format off
    #pragma HLS dataflow
    // clang-format on

    hls::stream<XF_TNAME(SRC_T, NPC)> src;
    hls::stream<XF_TNAME(SRC_T, NPC)> dst;
    hls::stream<XF_TNAME(SRC_T, NPC)> dst1;

    /********************************************************/

Read_yuyv_Loop:
    for (int i = 0; i < _src.rows; i++) {
        // clang-format off
        #pragma HLS LOOP_TRIPCOUNT min=1 max=ROWS
        // clang-format on
        for (int j = 0; j<(_src.cols)>> (XF_BITSHIFT(NPC)); j++) {
            // clang-format off
            #pragma HLS LOOP_TRIPCOUNT min=1 max=COLS/NPC
            #pragma HLS PIPELINE
            #pragma HLS loop_flatten off
            // clang-format on
            src.write(_src.read(i * (_src.cols >> (XF_BITSHIFT(NPC))) + j));
        }
    }

    xFDuplicate<ROWS, COLS, SRC_T, XF_DEPTH(SRC_T, NPC), NPC, XF_WORDWIDTH(SRC_T, NPC)>(src, dst, dst1, _src.rows,
                                                                                        _src.cols);

    for (int i = 0; i < _dst1.rows; i++) {
        // clang-format off
        #pragma HLS LOOP_TRIPCOUNT min=1 max=ROWS
        // clang-format on
        for (int j = 0; j<(_dst1.cols)>> (XF_BITSHIFT(NPC)); j++) {
            // clang-format off
            #pragma HLS LOOP_TRIPCOUNT min=1 max=COLS/NPC
            #pragma HLS PIPELINE
            #pragma HLS loop_flatten off
            // clang-format on
            _dst1.write((i * (_dst1.cols >> (XF_BITSHIFT(NPC))) + j), dst.read());
            _dst2.write((i * (_dst2.cols >> (XF_BITSHIFT(NPC))) + j), dst1.read());
        }
    }
}
} // namespace cv
} // namespace xf
#endif
