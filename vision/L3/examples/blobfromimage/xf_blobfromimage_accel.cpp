/*
 * Copyright 2020 Xilinx, Inc.
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

#include "xf_blobfromimage_accel_config.h"

static constexpr int __XF_DEPTH = (HEIGHT * WIDTH * (XF_PIXELWIDTH(IN_TYPE, NPPCX)) / 8) / (INPUT_PTR_WIDTH / 8);
static constexpr int __XF_DEPTH_MS = (2 * XF_CHANNELS(IN_TYPE, NPC));
static constexpr int __XF_DEPTH_OUT =
    (NEWHEIGHT * NEWWIDTH * (XF_PIXELWIDTH(IN_TYPE, NPPCX)) / 8) / (OUTPUT_PTR_WIDTH / 8);

extern "C" {
#if YUV_420
void blobfromimage_yuv420(ap_uint<8 * NPC1>* imgInput0,
                          ap_uint<16 * NPC2>* imgInput1,
                          ap_uint<OUTPUT_PTR_WIDTH>* _dst,
                          float params[2 * XF_CHANNELS(IN_TYPE, NPC)],
                          int in_img_width,
                          int in_img_height,
                          int in_img_linestride,
                          int resize_width,
                          int resize_height,
                          int out_img_width,      // Final Output image width
                          int out_img_height,     // Final Output image height
                          int out_img_linestride, // Final Output image line stride
                          int roi_posx,
                          int roi_posy) {
    static constexpr int __XF_DEPTH_INP_0 = ((HEIGHT) * (WIDTH) * (XF_PIXELWIDTH(XF_8UC1, NPC1))) / (8 * NPC1);
    static constexpr int __XF_DEPTH_INP_1 =
        (((HEIGHT / 2)) * ((WIDTH / 2)) * (XF_PIXELWIDTH(XF_8UC2, NPC2))) / (16 * NPC2);
// static constexpr int __XF_DEPTH_OUT_0 = ((HEIGHT) * (WIDTH) * (XF_PIXELWIDTH(XF_8UC3, NPC1))) / (32 * NPC1);

// clang-format off
#pragma HLS INTERFACE m_axi      port=imgInput0   offset=slave  bundle=gmem_in0  depth=__XF_DEPTH_INP_0
#pragma HLS INTERFACE m_axi      port=imgInput1   offset=slave  bundle=gmem_in1  depth=__XF_DEPTH_INP_1
#pragma HLS INTERFACE m_axi      port=_dst   offset=slave  bundle=gmem_out0 	 depth=__XF_DEPTH_OUT
#pragma HLS INTERFACE m_axi      port=params  offset=slave bundle=gmem3		 depth=__XF_DEPTH_MS

#pragma HLS INTERFACE s_axilite port=in_img_width     
#pragma HLS INTERFACE s_axilite port=in_img_height     
#pragma HLS INTERFACE s_axilite port=in_img_linestride     
#pragma HLS INTERFACE s_axilite port=out_img_width     
#pragma HLS INTERFACE s_axilite port=out_img_height     
#pragma HLS INTERFACE s_axilite port=out_img_linestride     
#pragma HLS INTERFACE s_axilite port=roi_posx     
#pragma HLS INTERFACE s_axilite port=roi_posy     

#pragma HLS INTERFACE s_axilite port=return

    // clang-format on
    // xf::cv::Mat<IN_TYPE, HEIGHT, WIDTH, NPC, XF_CV_DEPTH_IN> imgInput(in_img_height, in_img_width);

    xf::cv::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1, XF_CV_DEPTH_IN_0> _imgInput0(HEIGHT, WIDTH);
    xf::cv::Mat<XF_8UC2, (HEIGHT / 2), (WIDTH / 2), NPC2, XF_CV_DEPTH_IN_1> _imgInput1((HEIGHT / 2), (WIDTH / 2));
    xf::cv::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1, XF_CV_DEPTH_OUT> _imgOutput(HEIGHT, WIDTH);
//   xf::cv::Mat<XF_8UC3, NEWHEIGHT, NEWWIDTH, NPPCX, __XF_DEPTH_OUT> out_mat(NEWHEIGHT, NEWWIDTH);

#if BGR2RGB
    xf::cv::Mat<OUT_TYPE, HEIGHT, WIDTH, NPC, XF_CV_DEPTH_CH_SWAP> ch_swap_mat(in_img_height, in_img_width);
#endif
    xf::cv::Mat<OUT_TYPE, NEWHEIGHT, NEWWIDTH, NPC, XF_CV_DEPTH_RESIZE_OUT> resize_out_mat(resize_height, resize_width);

#if CROP
    xf::cv::Rect_<unsigned int> roi;
    roi.x = roi_posx;
    roi.y = roi_posy;
    roi.height = out_img_height;
    roi.width = out_img_width;

    xf::cv::Mat<OUT_TYPE, NEWHEIGHT, NEWWIDTH, NPC, XF_CV_DEPTH_CROP> crop_mat(out_img_height, out_img_width);
#endif
    xf::cv::Mat<OUT_TYPE, NEWHEIGHT, NEWWIDTH, NPC, XF_CV_DEPTH_OUT> out_mat(out_img_height, out_img_width);

// clang-format off
#pragma HLS DATAFLOW
    // clang-format on
    xf::cv::Array2xfMat<8 * NPC1, XF_8UC1, HEIGHT, WIDTH, NPC1, XF_CV_DEPTH_IN_0>(imgInput0, _imgInput0);
    xf::cv::Array2xfMat<16 * NPC2, XF_8UC2, (HEIGHT / 2), (WIDTH / 2), NPC2, XF_CV_DEPTH_IN_1>(imgInput1, _imgInput1);

    xf::cv::nv122rgb<XF_8UC1, XF_8UC2, XF_8UC3, HEIGHT, WIDTH, NPC1, NPC2, XF_CV_DEPTH_IN_0, XF_CV_DEPTH_IN_1,
                     XF_CV_DEPTH_OUT>(_imgInput0, _imgInput1, _imgOutput);
#if BGR2RGB
    xf::cv::bgr2rgb<IN_TYPE, OUT_TYPE, HEIGHT, WIDTH, NPC, XF_CV_DEPTH_IN, XF_CV_DEPTH_CH_SWAP>(imgInput, ch_swap_mat);
    xf::cv::resize<INTERPOLATION, IN_TYPE, HEIGHT, WIDTH, NEWHEIGHT, NEWWIDTH, NPC, XF_USE_URAM, MAXDOWNSCALE,
                   XF_CV_DEPTH_CH_SWAP, XF_CV_DEPTH_RESIZE_OUT>(ch_swap_mat, resize_out_mat);
#else

    xf::cv::resize<INTERPOLATION, IN_TYPE, HEIGHT, WIDTH, NEWHEIGHT, NEWWIDTH, NPC, XF_USE_URAM, MAXDOWNSCALE,
                   XF_CV_DEPTH_IN, XF_CV_DEPTH_RESIZE_OUT>(_imgOutput, resize_out_mat);
#endif

#if CROP
    xf::cv::crop<OUT_TYPE, NEWHEIGHT, NEWWIDTH, 0, NPC, XF_CV_DEPTH_RESIZE_OUT, XF_CV_DEPTH_CROP>(resize_out_mat,
                                                                                                  crop_mat, roi);
    xf::cv::preProcess<IN_TYPE, OUT_TYPE, NEWHEIGHT, NEWWIDTH, NPC, WIDTH_A, IBITS_A, WIDTH_B, IBITS_B, WIDTH_OUT,
                       IBITS_OUT, XF_CV_DEPTH_CROP, XF_CV_DEPTH_OUT>(crop_mat, out_mat, params);
#else

    xf::cv::preProcess<IN_TYPE, OUT_TYPE, NEWHEIGHT, NEWWIDTH, NPC, WIDTH_A, IBITS_A, WIDTH_B, IBITS_B, WIDTH_OUT,
                       IBITS_OUT, XF_CV_DEPTH_RESIZE_OUT, XF_CV_DEPTH_OUT>(resize_out_mat, out_mat, params);

#endif
    xf::cv::xfMat2Array<OUTPUT_PTR_WIDTH, OUT_TYPE, NEWHEIGHT, NEWWIDTH, NPC, XF_CV_DEPTH_OUT>(out_mat, _dst);
}
#endif
#if YUV_422
void blobfromimage_yuv422(ap_uint<16 * NPC1>* imgInput0,
                          ap_uint<OUTPUT_PTR_WIDTH>* _dst,
                          float params[2 * XF_CHANNELS(IN_TYPE, NPC)],
                          int in_img_width,
                          int in_img_height,
                          int in_img_linestride,
                          int resize_width,
                          int resize_height,
                          int out_img_width,      // Final Output image width
                          int out_img_height,     // Final Output image height
                          int out_img_linestride, // Final Output image line stride
                          int roi_posx,
                          int roi_posy) {
    static constexpr int __XF_DEPTH_INP_0 = ((HEIGHT) * (WIDTH) * (XF_PIXELWIDTH(XF_16UC1, NPC1))) / (16 * NPC1);
    static constexpr int __XF_DEPTH_OUT_0 = ((HEIGHT) * (WIDTH) * (XF_PIXELWIDTH(XF_8UC3, NPC1))) / (32 * NPC1);

// clang-format off
#pragma HLS INTERFACE m_axi      port=imgInput0   offset=slave  bundle=gmem_in0  depth=__XF_DEPTH_INP_0
#pragma HLS INTERFACE m_axi      port=_dst   offset=slave  bundle=gmem_out0 	 depth=__XF_DEPTH_OUT
#pragma HLS INTERFACE m_axi      port=params  offset=slave bundle=gmem3		 depth=__XF_DEPTH_MS

#pragma HLS INTERFACE s_axilite port=in_img_width     
#pragma HLS INTERFACE s_axilite port=in_img_height     
#pragma HLS INTERFACE s_axilite port=in_img_linestride     
#pragma HLS INTERFACE s_axilite port=out_img_width     
#pragma HLS INTERFACE s_axilite port=out_img_height     
#pragma HLS INTERFACE s_axilite port=out_img_linestride     
#pragma HLS INTERFACE s_axilite port=roi_posx     
#pragma HLS INTERFACE s_axilite port=roi_posy     

#pragma HLS INTERFACE s_axilite port=return

    // clang-format on
    xf::cv::Mat<XF_16UC1, HEIGHT, WIDTH, NPC1, XF_CV_DEPTH_IN> _imgInput(HEIGHT, WIDTH);
    xf::cv::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1, XF_CV_DEPTH_OUT> _imgOutput(HEIGHT, WIDTH);

#if BGR2RGB
    xf::cv::Mat<OUT_TYPE, HEIGHT, WIDTH, NPC, XF_CV_DEPTH_CH_SWAP> ch_swap_mat(in_img_height, in_img_width);
#endif
    xf::cv::Mat<OUT_TYPE, NEWHEIGHT, NEWWIDTH, NPC, XF_CV_DEPTH_RESIZE_OUT> resize_out_mat(resize_height, resize_width);

#if CROP
    xf::cv::Rect_<unsigned int> roi;
    roi.x = roi_posx;
    roi.y = roi_posy;
    roi.height = out_img_height;
    roi.width = out_img_width;

    xf::cv::Mat<OUT_TYPE, NEWHEIGHT, NEWWIDTH, NPC, XF_CV_DEPTH_CROP> crop_mat(out_img_height, out_img_width);
#endif
    xf::cv::Mat<OUT_TYPE, NEWHEIGHT, NEWWIDTH, NPC, XF_CV_DEPTH_OUT> out_mat(out_img_height, out_img_width);

// clang-format off
#pragma HLS DATAFLOW
    // clang-format on
    xf::cv::Array2xfMat<16 * NPC1, XF_16UC1, HEIGHT, WIDTH, NPC1, XF_CV_DEPTH_IN>(imgInput0, _imgInput);
    xf::cv::yuyv2rgb<XF_16UC1, XF_8UC3, HEIGHT, WIDTH, NPC1, XF_CV_DEPTH_IN, XF_CV_DEPTH_OUT>(_imgInput, _imgOutput);

#if BGR2RGB
    xf::cv::bgr2rgb<IN_TYPE, OUT_TYPE, HEIGHT, WIDTH, NPC, XF_CV_DEPTH_IN, XF_CV_DEPTH_CH_SWAP>(imgInput, ch_swap_mat);
    xf::cv::resize<INTERPOLATION, IN_TYPE, HEIGHT, WIDTH, NEWHEIGHT, NEWWIDTH, NPC, XF_USE_URAM, MAXDOWNSCALE,
                   XF_CV_DEPTH_CH_SWAP, XF_CV_DEPTH_RESIZE_OUT>(ch_swap_mat, resize_out_mat);
#else

    xf::cv::resize<INTERPOLATION, IN_TYPE, HEIGHT, WIDTH, NEWHEIGHT, NEWWIDTH, NPC, XF_USE_URAM, MAXDOWNSCALE,
                   XF_CV_DEPTH_IN, XF_CV_DEPTH_RESIZE_OUT>(_imgOutput, resize_out_mat);
#endif

#if CROP
    xf::cv::crop<OUT_TYPE, NEWHEIGHT, NEWWIDTH, 0, NPC, XF_CV_DEPTH_RESIZE_OUT, XF_CV_DEPTH_CROP>(resize_out_mat,
                                                                                                  crop_mat, roi);
    xf::cv::preProcess<IN_TYPE, OUT_TYPE, NEWHEIGHT, NEWWIDTH, NPC, WIDTH_A, IBITS_A, WIDTH_B, IBITS_B, WIDTH_OUT,
                       IBITS_OUT, XF_CV_DEPTH_CROP, XF_CV_DEPTH_OUT>(crop_mat, out_mat, params);
#else

    xf::cv::preProcess<IN_TYPE, OUT_TYPE, NEWHEIGHT, NEWWIDTH, NPC, WIDTH_A, IBITS_A, WIDTH_B, IBITS_B, WIDTH_OUT,
                       IBITS_OUT, XF_CV_DEPTH_RESIZE_OUT, XF_CV_DEPTH_OUT>(resize_out_mat, out_mat, params);

#endif
    xf::cv::xfMat2Array<OUTPUT_PTR_WIDTH, OUT_TYPE, NEWHEIGHT, NEWWIDTH, NPC, XF_CV_DEPTH_OUT>(out_mat, _dst);
}
#endif
#if YUV_400
void blobfromimage_yuv400(ap_uint<INPUT_PTR_WIDTH>* imgInput0,
                          ap_uint<OUTPUT_PTR_WIDTH>* _dst,
                          float params[2 * XF_CHANNELS(IN_TYPE, NPC)],
                          int in_img_width,
                          int in_img_height,
                          int in_img_linestride,
                          int resize_width,
                          int resize_height,
                          int out_img_width,      // Final Output image width
                          int out_img_height,     // Final Output image height
                          int out_img_linestride, // Final Output image line stride
                          int roi_posx,
                          int roi_posy) {
// clang-format off
#pragma HLS INTERFACE m_axi      port=imgInput0   offset=slave  bundle=gmem_in0  depth=__XF_DEPTH
#pragma HLS INTERFACE m_axi      port=_dst   offset=slave  bundle=gmem_out0 	 depth=__XF_DEPTH_OUT
#pragma HLS INTERFACE m_axi      port=params  offset=slave bundle=gmem3		 depth=__XF_DEPTH_MS

#pragma HLS INTERFACE s_axilite port=in_img_width     
#pragma HLS INTERFACE s_axilite port=in_img_height     
#pragma HLS INTERFACE s_axilite port=in_img_linestride     
#pragma HLS INTERFACE s_axilite port=out_img_width     
#pragma HLS INTERFACE s_axilite port=out_img_height     
#pragma HLS INTERFACE s_axilite port=out_img_linestride     
#pragma HLS INTERFACE s_axilite port=roi_posx     
#pragma HLS INTERFACE s_axilite port=roi_posy     

#pragma HLS INTERFACE s_axilite port=return

    // clang-format on
    xf::cv::Mat<IN_TYPE, HEIGHT, WIDTH, NPC1, XF_CV_DEPTH_IN> imgInput(HEIGHT, WIDTH);
    xf::cv::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1, XF_CV_DEPTH_OUT> _imgOutput(HEIGHT, WIDTH);

#if BGR2RGB
    xf::cv::Mat<OUT_TYPE, HEIGHT, WIDTH, NPC, XF_CV_DEPTH_CH_SWAP> ch_swap_mat(in_img_height, in_img_width);
#endif
    xf::cv::Mat<OUT_TYPE, NEWHEIGHT, NEWWIDTH, NPC, XF_CV_DEPTH_RESIZE_OUT> resize_out_mat(resize_height, resize_width);

#if CROP
    xf::cv::Rect_<unsigned int> roi;
    roi.x = roi_posx;
    roi.y = roi_posy;
    roi.height = out_img_height;
    roi.width = out_img_width;

    xf::cv::Mat<OUT_TYPE, NEWHEIGHT, NEWWIDTH, NPC, XF_CV_DEPTH_CROP> crop_mat(out_img_height, out_img_width);
#endif
    xf::cv::Mat<OUT_TYPE, NEWHEIGHT, NEWWIDTH, NPC, XF_CV_DEPTH_OUT> out_mat(out_img_height, out_img_width);

// clang-format off
#pragma HLS DATAFLOW
    // clang-format on
    xf::cv::Array2xfMat<INPUT_PTR_WIDTH, IN_TYPE, HEIGHT, WIDTH, NPPCX, XF_CV_DEPTH_IN>(imgInput0, imgInput);

#if BGR2RGB
    xf::cv::bgr2rgb<IN_TYPE, OUT_TYPE, HEIGHT, WIDTH, NPC, XF_CV_DEPTH_IN, XF_CV_DEPTH_CH_SWAP>(imgInput, ch_swap_mat);
    xf::cv::resize<INTERPOLATION, IN_TYPE, HEIGHT, WIDTH, NEWHEIGHT, NEWWIDTH, NPC, XF_USE_URAM, MAXDOWNSCALE,
                   XF_CV_DEPTH_CH_SWAP, XF_CV_DEPTH_RESIZE_OUT>(ch_swap_mat, resize_out_mat);
#else

    xf::cv::resize<INTERPOLATION, IN_TYPE, HEIGHT, WIDTH, NEWHEIGHT, NEWWIDTH, NPC, XF_USE_URAM, MAXDOWNSCALE,
                   XF_CV_DEPTH_IN, XF_CV_DEPTH_RESIZE_OUT>(imgInput, resize_out_mat);
#endif

#if CROP
    xf::cv::crop<OUT_TYPE, NEWHEIGHT, NEWWIDTH, 0, NPC, XF_CV_DEPTH_RESIZE_OUT, XF_CV_DEPTH_CROP>(resize_out_mat,
                                                                                                  crop_mat, roi);
    xf::cv::preProcess<IN_TYPE, OUT_TYPE, NEWHEIGHT, NEWWIDTH, NPC, WIDTH_A, IBITS_A, WIDTH_B, IBITS_B, WIDTH_OUT,
                       IBITS_OUT, XF_CV_DEPTH_CROP, XF_CV_DEPTH_OUT>(crop_mat, out_mat, params);
#else

    xf::cv::preProcess<IN_TYPE, OUT_TYPE, NEWHEIGHT, NEWWIDTH, NPC, WIDTH_A, IBITS_A, WIDTH_B, IBITS_B, WIDTH_OUT,
                       IBITS_OUT, XF_CV_DEPTH_RESIZE_OUT, XF_CV_DEPTH_OUT>(resize_out_mat, out_mat, params);

#endif
    xf::cv::xfMat2Array<OUTPUT_PTR_WIDTH, OUT_TYPE, NEWHEIGHT, NEWWIDTH, NPC, XF_CV_DEPTH_OUT>(out_mat, _dst);
}
#endif
#if YUV_444
void blobfromimage_yuv444(ap_uint<32 * NPC1>* imgInput0,
                          ap_uint<OUTPUT_PTR_WIDTH>* _dst,
                          float params[2 * XF_CHANNELS(IN_TYPE, NPC)],
                          int in_img_width,
                          int in_img_height,
                          int in_img_linestride,
                          int resize_width,
                          int resize_height,
                          int out_img_width,      // Final Output image width
                          int out_img_height,     // Final Output image height
                          int out_img_linestride, // Final Output image line stride
                          int roi_posx,
                          int roi_posy) {
    static constexpr int __XF_DEPTH_INP_0 = ((HEIGHT) * (WIDTH) * (XF_PIXELWIDTH(XF_8UC3, NPC1))) / (32 * NPC1);

// clang-format off
#pragma HLS INTERFACE m_axi      port=imgInput0   offset=slave  bundle=gmem_in0  depth=__XF_DEPTH_INP_0
#pragma HLS INTERFACE m_axi      port=_dst   offset=slave  bundle=gmem_out0 	 depth=__XF_DEPTH_OUT
#pragma HLS INTERFACE m_axi      port=params  offset=slave bundle=gmem3		 depth=__XF_DEPTH_MS

#pragma HLS INTERFACE s_axilite port=in_img_width     
#pragma HLS INTERFACE s_axilite port=in_img_height     
#pragma HLS INTERFACE s_axilite port=in_img_linestride     
#pragma HLS INTERFACE s_axilite port=out_img_width     
#pragma HLS INTERFACE s_axilite port=out_img_height     
#pragma HLS INTERFACE s_axilite port=out_img_linestride     
#pragma HLS INTERFACE s_axilite port=roi_posx     
#pragma HLS INTERFACE s_axilite port=roi_posy     

#pragma HLS INTERFACE s_axilite port=return

    // clang-format on
    xf::cv::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1, XF_CV_DEPTH_IN> imgInput(HEIGHT, WIDTH);
    xf::cv::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1, XF_CV_DEPTH_OUT> _imgOutput(HEIGHT, WIDTH);

#if BGR2RGB
    xf::cv::Mat<OUT_TYPE, HEIGHT, WIDTH, NPC, XF_CV_DEPTH_CH_SWAP> ch_swap_mat(in_img_height, in_img_width);
#endif
    xf::cv::Mat<OUT_TYPE, NEWHEIGHT, NEWWIDTH, NPC, XF_CV_DEPTH_RESIZE_OUT> resize_out_mat(resize_height, resize_width);

#if CROP
    xf::cv::Rect_<unsigned int> roi;
    roi.x = roi_posx;
    roi.y = roi_posy;
    roi.height = out_img_height;
    roi.width = out_img_width;

    xf::cv::Mat<OUT_TYPE, NEWHEIGHT, NEWWIDTH, NPC, XF_CV_DEPTH_CROP> crop_mat(out_img_height, out_img_width);
#endif
    xf::cv::Mat<OUT_TYPE, NEWHEIGHT, NEWWIDTH, NPC, XF_CV_DEPTH_OUT> out_mat(out_img_height, out_img_width);

// clang-format off
#pragma HLS DATAFLOW
    // clang-format on
    xf::cv::Array2xfMat<32 * NPC1, IN_TYPE, HEIGHT, WIDTH, NPPCX, XF_CV_DEPTH_IN>(imgInput0, imgInput);
    xf::cv::yuv4442rgb<XF_8UC3, XF_8UC3, HEIGHT, WIDTH, NPC1, XF_CV_DEPTH_IN, XF_CV_DEPTH_OUT>(imgInput, _imgOutput);
#if BGR2RGB
    xf::cv::bgr2rgb<IN_TYPE, OUT_TYPE, HEIGHT, WIDTH, NPC, XF_CV_DEPTH_IN, XF_CV_DEPTH_CH_SWAP>(imgInput, ch_swap_mat);
    xf::cv::resize<INTERPOLATION, IN_TYPE, HEIGHT, WIDTH, NEWHEIGHT, NEWWIDTH, NPC, XF_USE_URAM, MAXDOWNSCALE,
                   XF_CV_DEPTH_CH_SWAP, XF_CV_DEPTH_RESIZE_OUT>(ch_swap_mat, resize_out_mat);
#else

    xf::cv::resize<INTERPOLATION, IN_TYPE, HEIGHT, WIDTH, NEWHEIGHT, NEWWIDTH, NPC, XF_USE_URAM, MAXDOWNSCALE,
                   XF_CV_DEPTH_IN, XF_CV_DEPTH_RESIZE_OUT>(_imgOutput, resize_out_mat);
#endif

#if CROP
    xf::cv::crop<OUT_TYPE, NEWHEIGHT, NEWWIDTH, 0, NPC, XF_CV_DEPTH_RESIZE_OUT, XF_CV_DEPTH_CROP>(resize_out_mat,
                                                                                                  crop_mat, roi);
    xf::cv::preProcess<IN_TYPE, OUT_TYPE, NEWHEIGHT, NEWWIDTH, NPC, WIDTH_A, IBITS_A, WIDTH_B, IBITS_B, WIDTH_OUT,
                       IBITS_OUT, XF_CV_DEPTH_CROP, XF_CV_DEPTH_OUT>(crop_mat, out_mat, params);
#else

    xf::cv::preProcess<IN_TYPE, OUT_TYPE, NEWHEIGHT, NEWWIDTH, NPC, WIDTH_A, IBITS_A, WIDTH_B, IBITS_B, WIDTH_OUT,
                       IBITS_OUT, XF_CV_DEPTH_RESIZE_OUT, XF_CV_DEPTH_OUT>(resize_out_mat, out_mat, params);

#endif
    xf::cv::xfMat2Array<OUTPUT_PTR_WIDTH, OUT_TYPE, NEWHEIGHT, NEWWIDTH, NPC, XF_CV_DEPTH_OUT>(out_mat, _dst);
}
#endif
#if RGB
void blobfromimage_rgb(ap_uint<32 * NPC1>* imgInput0,
                       ap_uint<OUTPUT_PTR_WIDTH>* _dst,
                       float params[2 * XF_CHANNELS(IN_TYPE, NPC)],
                       int in_img_width,
                       int in_img_height,
                       int in_img_linestride,
                       int resize_width,
                       int resize_height,
                       int out_img_width,      // Final Output image width
                       int out_img_height,     // Final Output image height
                       int out_img_linestride, // Final Output image line stride
                       int roi_posx,
                       int roi_posy) {
    static constexpr int __XF_DEPTH_INP_0 = ((HEIGHT) * (WIDTH) * (XF_PIXELWIDTH(XF_8UC3, NPC1))) / (32 * NPC1);

// clang-format off
#pragma HLS INTERFACE m_axi      port=imgInput0   offset=slave  bundle=gmem_in0  depth=__XF_DEPTH_INP_0
#pragma HLS INTERFACE m_axi      port=_dst   offset=slave  bundle=gmem_out0 	 depth=__XF_DEPTH_OUT
#pragma HLS INTERFACE m_axi      port=params  offset=slave bundle=gmem3		 depth=__XF_DEPTH_MS

#pragma HLS INTERFACE s_axilite port=in_img_width     
#pragma HLS INTERFACE s_axilite port=in_img_height     
#pragma HLS INTERFACE s_axilite port=in_img_linestride     
#pragma HLS INTERFACE s_axilite port=out_img_width     
#pragma HLS INTERFACE s_axilite port=out_img_height     
#pragma HLS INTERFACE s_axilite port=out_img_linestride     
#pragma HLS INTERFACE s_axilite port=roi_posx     
#pragma HLS INTERFACE s_axilite port=roi_posy     

#pragma HLS INTERFACE s_axilite port=return

    // clang-format on
    xf::cv::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1, XF_CV_DEPTH_IN> imgInput(HEIGHT, WIDTH);
    xf::cv::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1, XF_CV_DEPTH_OUT> _imgOutput(HEIGHT, WIDTH);

#if BGR2RGB
    xf::cv::Mat<OUT_TYPE, HEIGHT, WIDTH, NPC, XF_CV_DEPTH_CH_SWAP> ch_swap_mat(in_img_height, in_img_width);
#endif
    xf::cv::Mat<OUT_TYPE, NEWHEIGHT, NEWWIDTH, NPC, XF_CV_DEPTH_RESIZE_OUT> resize_out_mat(resize_height, resize_width);

#if CROP
    xf::cv::Rect_<unsigned int> roi;
    roi.x = roi_posx;
    roi.y = roi_posy;
    roi.height = out_img_height;
    roi.width = out_img_width;

    xf::cv::Mat<OUT_TYPE, NEWHEIGHT, NEWWIDTH, NPC, XF_CV_DEPTH_CROP> crop_mat(out_img_height, out_img_width);
#endif
    xf::cv::Mat<OUT_TYPE, NEWHEIGHT, NEWWIDTH, NPC, XF_CV_DEPTH_OUT> out_mat(out_img_height, out_img_width);

// clang-format off
#pragma HLS DATAFLOW
    // clang-format on
    xf::cv::Array2xfMat<32 * NPC1, IN_TYPE, HEIGHT, WIDTH, NPPCX, XF_CV_DEPTH_IN>(imgInput0, imgInput);
#if BGR2RGB
    xf::cv::bgr2rgb<IN_TYPE, OUT_TYPE, HEIGHT, WIDTH, NPC, XF_CV_DEPTH_IN, XF_CV_DEPTH_CH_SWAP>(imgInput, ch_swap_mat);
    xf::cv::resize<INTERPOLATION, IN_TYPE, HEIGHT, WIDTH, NEWHEIGHT, NEWWIDTH, NPC, XF_USE_URAM, MAXDOWNSCALE,
                   XF_CV_DEPTH_CH_SWAP, XF_CV_DEPTH_RESIZE_OUT>(ch_swap_mat, resize_out_mat);
#else

    xf::cv::resize<INTERPOLATION, IN_TYPE, HEIGHT, WIDTH, NEWHEIGHT, NEWWIDTH, NPC, XF_USE_URAM, MAXDOWNSCALE,
                   XF_CV_DEPTH_IN, XF_CV_DEPTH_RESIZE_OUT>(imgInput, resize_out_mat);
#endif

#if CROP
    xf::cv::crop<OUT_TYPE, NEWHEIGHT, NEWWIDTH, 0, NPC, XF_CV_DEPTH_RESIZE_OUT, XF_CV_DEPTH_CROP>(resize_out_mat,
                                                                                                  crop_mat, roi);
    xf::cv::preProcess<IN_TYPE, OUT_TYPE, NEWHEIGHT, NEWWIDTH, NPC, WIDTH_A, IBITS_A, WIDTH_B, IBITS_B, WIDTH_OUT,
                       IBITS_OUT, XF_CV_DEPTH_CROP, XF_CV_DEPTH_OUT>(crop_mat, out_mat, params);
#else

    xf::cv::preProcess<IN_TYPE, OUT_TYPE, NEWHEIGHT, NEWWIDTH, NPC, WIDTH_A, IBITS_A, WIDTH_B, IBITS_B, WIDTH_OUT,
                       IBITS_OUT, XF_CV_DEPTH_RESIZE_OUT, XF_CV_DEPTH_OUT>(resize_out_mat, out_mat, params);

#endif
    xf::cv::xfMat2Array<OUTPUT_PTR_WIDTH, OUT_TYPE, NEWHEIGHT, NEWWIDTH, NPC, XF_CV_DEPTH_OUT>(out_mat, _dst);
}
#endif

} // extern
