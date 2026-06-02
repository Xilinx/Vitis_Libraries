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

#include "xf_AVFIRS_4stream_accel_config.h"

template <int APPEND_TYPE, int APPEND_HEIGHT, int APPEND_WIDTH, int APPEND_NPPC, int APPEND_DEPTH>
void append(xf::cv::Mat<APPEND_TYPE, APPEND_HEIGHT, APPEND_WIDTH, APPEND_NPPC, APPEND_DEPTH>& src,
            int row_offset,
            xf::cv::Mat<APPEND_TYPE, APPEND_HEIGHT * 4, APPEND_WIDTH, APPEND_NPPC, XF_COMB_DEPTH>& dst,
            int rows,
            int cols) {
    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
#pragma HLS loop flatten off
            int dst_idx = (row_offset + r) * cols + c;
            int src_idx = r * cols + c;
            dst.write(dst_idx, src.read(src_idx));
        }
    }
}

void AVFIRS_4stream_accel(ap_uint<INPUT_PTR_WIDTH>* img_in1,
                          ap_uint<INPUT_PTR_WIDTH>* img_in2,
                          ap_uint<INPUT_PTR_WIDTH>* img_in3,
                          ap_uint<INPUT_PTR_WIDTH>* img_in4,
                          ap_uint<MAPXY_TYPE_PTR_WIDTH>* map_x1,
                          ap_uint<MAPXY_TYPE_PTR_WIDTH>* map_y1,
                          ap_uint<MAPXY_TYPE_PTR_WIDTH>* map_x2,
                          ap_uint<MAPXY_TYPE_PTR_WIDTH>* map_y2,
                          ap_uint<MAPXY_TYPE_PTR_WIDTH>* map_x3,
                          ap_uint<MAPXY_TYPE_PTR_WIDTH>* map_y3,
                          ap_uint<MAPXY_TYPE_PTR_WIDTH>* map_x4,
                          ap_uint<MAPXY_TYPE_PTR_WIDTH>* map_y4,
                          ap_uint<MASK_INPUT_PTR_WIDTH>* mask_img1,
                          ap_uint<MASK_INPUT_PTR_WIDTH>* mask_img2,
                          ap_uint<MASK_INPUT_PTR_WIDTH>* mask_img3,
                          ap_uint<MASK_INPUT_PTR_WIDTH>* mask_img4,
                          ap_uint<INPUT_PTR_WIDTH>* points_packed,
                          ap_uint<INPUT_PTR_WIDTH>* contour_offsets,
                          ap_uint<INPUT_PTR_WIDTH>* num_contours,
                          float sigma,
                          int shift,
                          unsigned char thresh,
                          unsigned char maxval,
                          int rows,
                          int cols,
                          int img_sizes[8],
                          int mask_corners[8],
                          int dst_height,
                          int dst_width,
                          ap_uint<OUTPUT_PTR_WIDTH>* resizedOutput_duplicate_2_stream) {
// clang-format off
     #pragma HLS INTERFACE m_axi      port=img_in1         offset=slave  bundle=gmem0 
     #pragma HLS INTERFACE m_axi      port=img_in2         offset=slave  bundle=gmem1 
     #pragma HLS INTERFACE m_axi      port=img_in3         offset=slave  bundle=gmem2 
     #pragma HLS INTERFACE m_axi      port=img_in4         offset=slave  bundle=gmem3 
     #pragma HLS INTERFACE m_axi      port=map_x1          offset=slave  bundle=gmem4 
     #pragma HLS INTERFACE m_axi      port=map_y1          offset=slave  bundle=gmem5 
     #pragma HLS INTERFACE m_axi      port=map_x2          offset=slave  bundle=gmem6 
     #pragma HLS INTERFACE m_axi      port=map_y2          offset=slave  bundle=gmem7 
     #pragma HLS INTERFACE m_axi      port=map_x3          offset=slave  bundle=gmem8 
     #pragma HLS INTERFACE m_axi      port=map_y3          offset=slave  bundle=gmem9  
     #pragma HLS INTERFACE m_axi      port=map_x4          offset=slave  bundle=gmem10  
     #pragma HLS INTERFACE m_axi      port=map_y4          offset=slave  bundle=gmem11  
     #pragma HLS INTERFACE m_axi      port=mask_img1       offset=slave  bundle=gmem12
     #pragma HLS INTERFACE m_axi      port=mask_img2       offset=slave  bundle=gmem13
     #pragma HLS INTERFACE m_axi      port=mask_img3       offset=slave  bundle=gmem14
     #pragma HLS INTERFACE m_axi      port=mask_img4       offset=slave  bundle=gmem15
     #pragma HLS INTERFACE m_axi      port=points_packed   offset=slave  bundle=gmem16
     #pragma HLS INTERFACE m_axi      port=contour_offsets offset=slave  bundle=gmem17 
     #pragma HLS INTERFACE m_axi      port=num_contours    offset=slave  bundle=gmem18 
     #pragma HLS INTERFACE m_axi      port=img_sizes    offset=slave  bundle=gmem19
     #pragma HLS INTERFACE m_axi      port=mask_corners    offset=slave  bundle=gmem20             
     #pragma HLS INTERFACE s_axilite  port=sigma     
     #pragma HLS INTERFACE s_axilite  port=shift     
     #pragma HLS INTERFACE s_axilite  port=thresh     
     #pragma HLS INTERFACE s_axilite  port=maxval 			      
     #pragma HLS INTERFACE s_axilite  port=rows 			      
     #pragma HLS INTERFACE s_axilite  port=cols 		
    #pragma HLS INTERFACE s_axilite  port=dst_height 	
    #pragma HLS INTERFACE s_axilite  port=dst_width
    #pragma HLS INTERFACE m_axi      port=resizedOutput_duplicate_2_stream offset=slave  bundle=gmem21


     #pragma HLS INTERFACE s_axilite  port=return
     
    xf::cv::Mat<IN_TYPE, HEIGHT, WIDTH, XF_NPPCX, XF_CV_DEPTH_IN_1> imgInput1(rows, cols);
    xf::cv::Mat<IN_TYPE, HEIGHT, WIDTH, XF_NPPCX, XF_CV_DEPTH_IN_1> imgInput2(rows, cols);
    xf::cv::Mat<IN_TYPE, HEIGHT, WIDTH, XF_NPPCX, XF_CV_DEPTH_IN_1> imgInput3(rows, cols);
    xf::cv::Mat<IN_TYPE, HEIGHT, WIDTH, XF_NPPCX, XF_CV_DEPTH_IN_1> imgInput4(rows, cols);
    xf::cv::Mat<MAPXY_TYPE, HEIGHT_1, WIDTH_1, XF_NPPCX, XF_CV_DEPTH_IN_1> mapX1_stitch(img_sizes[0], img_sizes[1]);
    xf::cv::Mat<MAPXY_TYPE, HEIGHT_1, WIDTH_1, XF_NPPCX, XF_CV_DEPTH_IN_1> mapY1_stitch(img_sizes[0], img_sizes[1]);
    xf::cv::Mat<MAPXY_TYPE, HEIGHT_2, WIDTH_2, XF_NPPCX, XF_CV_DEPTH_IN_1> mapX2_stitch(img_sizes[2], img_sizes[3]);
    xf::cv::Mat<MAPXY_TYPE, HEIGHT_2, WIDTH_2, XF_NPPCX, XF_CV_DEPTH_IN_1> mapY2_stitch(img_sizes[2], img_sizes[3]);
    xf::cv::Mat<MAPXY_TYPE, HEIGHT_3, WIDTH_3, XF_NPPCX, XF_CV_DEPTH_IN_1> mapX3_stitch(img_sizes[4], img_sizes[5]);
    xf::cv::Mat<MAPXY_TYPE, HEIGHT_3, WIDTH_3, XF_NPPCX, XF_CV_DEPTH_IN_1> mapY3_stitch(img_sizes[4], img_sizes[5]);
    xf::cv::Mat<MAPXY_TYPE, HEIGHT_4, WIDTH_4, XF_NPPCX, XF_CV_DEPTH_IN_1> mapX4_stitch(img_sizes[6], img_sizes[7]);
    xf::cv::Mat<MAPXY_TYPE, HEIGHT_4, WIDTH_4, XF_NPPCX, XF_CV_DEPTH_IN_1> mapY4_stitch(img_sizes[6], img_sizes[7]);
    xf::cv::Mat<OUT_TYPE, HEIGHT, WIDTH, XF_NPPCX, XF_CV_DEPTH_OUT_1> resizedOutput(rows, cols);
    xf::cv::Mat<OUT_TYPE, HEIGHT, WIDTH, XF_NPPCX, XF_CV_DEPTH_OUT_1> gaussianOutput(rows, cols);
    xf::cv::Mat<OUT_TYPE_1, HEIGHT, WIDTH, XF_NPPCX, XF_CV_DEPTH_OUT_1> _dstgx(rows, cols);
    xf::cv::Mat<OUT_TYPE_1, HEIGHT, WIDTH, XF_NPPCX, XF_CV_DEPTH_OUT_1> _dstgy(rows, cols);
    xf::cv::Mat<OUT_TYPE_1, HEIGHT, WIDTH, XF_NPPCX, XF_CV_DEPTH_OUT_1> magOutput(rows, cols);
    xf::cv::Mat<OUT_TYPE, HEIGHT, WIDTH, XF_NPPCX, XF_CV_DEPTH_OUT_1> convertOutput(rows, cols);
    xf::cv::Mat<OUT_TYPE, HEIGHT, WIDTH, XF_NPPCX, XF_CV_DEPTH_OUT_1> thresOutput(rows, cols);
    
    
    xf::cv::Mat<OUT_TYPE, HEIGHT_1, WIDTH_1, XF_NPPCX, XF_CV_DEPTH_OUT_1> remapOutput1_stitch(img_sizes[0], img_sizes[1]);
    xf::cv::Mat<OUT_TYPE, HEIGHT_2, WIDTH_2, XF_NPPCX, XF_CV_DEPTH_OUT_1> remapOutput2_stitch(img_sizes[2], img_sizes[3]);
    xf::cv::Mat<OUT_TYPE, HEIGHT_3, WIDTH_3, XF_NPPCX, XF_CV_DEPTH_OUT_1> remapOutput3_stitch(img_sizes[4], img_sizes[5]);
    xf::cv::Mat<OUT_TYPE, HEIGHT_4, WIDTH_4, XF_NPPCX, XF_CV_DEPTH_OUT_1> remapOutput4_stitch(img_sizes[6], img_sizes[7]);
    xf::cv::Mat<IN_MASK_TYPE, HEIGHT_1, WIDTH_1, XF_NPPCX, XF_CV_DEPTH_IN_1> maskInput1(img_sizes[0], img_sizes[1]);
    xf::cv::Mat<IN_MASK_TYPE, HEIGHT_2, WIDTH_2, XF_NPPCX, XF_CV_DEPTH_IN_1> maskInput2(img_sizes[2], img_sizes[3]);
    xf::cv::Mat<IN_MASK_TYPE, HEIGHT_3, WIDTH_3, XF_NPPCX, XF_CV_DEPTH_IN_1> maskInput3(img_sizes[4], img_sizes[5]);
    xf::cv::Mat<IN_MASK_TYPE, HEIGHT_4, WIDTH_4, XF_NPPCX, XF_CV_DEPTH_IN_1> maskInput4(img_sizes[6], img_sizes[7]);

    
    xf::cv::Mat<IN_MASK_TYPE, HEIGHT_1, WIDTH_1, XF_NPPCX, XF_CV_DEPTH_IN_1> maskSrc1(img_sizes[0], img_sizes[1]);
    xf::cv::Mat<IN_MASK_TYPE, HEIGHT_2, WIDTH_2, XF_NPPCX, XF_CV_DEPTH_IN_1> maskSrc2(img_sizes[2], img_sizes[3]);
    xf::cv::Mat<IN_MASK_TYPE, HEIGHT_3, WIDTH_3, XF_NPPCX, XF_CV_DEPTH_IN_1> maskSrc3(img_sizes[4], img_sizes[5]);
    xf::cv::Mat<IN_MASK_TYPE, HEIGHT_4, WIDTH_4, XF_NPPCX, XF_CV_DEPTH_IN_1> maskSrc4(img_sizes[6], img_sizes[7]);
    

    ap_uint<INPUT_PTR_WIDTH> local_num = 0;

// clang-format off
    #pragma HLS DATAFLOW
    // clang-format on
    // Retrieve xf::cv::Mat objects from img_in data:
    xf::cv::Array2xfMat<INPUT_PTR_WIDTH, IN_TYPE, HEIGHT, WIDTH, XF_NPPCX, XF_CV_DEPTH_IN_1>(img_in1, imgInput1);
    xf::cv::Array2xfMat<INPUT_PTR_WIDTH, IN_TYPE, HEIGHT, WIDTH, XF_NPPCX, XF_CV_DEPTH_IN_1>(img_in2, imgInput2);
    xf::cv::Array2xfMat<INPUT_PTR_WIDTH, IN_TYPE, HEIGHT, WIDTH, XF_NPPCX, XF_CV_DEPTH_IN_1>(img_in3, imgInput3);
    xf::cv::Array2xfMat<INPUT_PTR_WIDTH, IN_TYPE, HEIGHT, WIDTH, XF_NPPCX, XF_CV_DEPTH_IN_1>(img_in4, imgInput4);

    xf::cv::Array2xfMat<MAPXY_TYPE_PTR_WIDTH, MAPXY_TYPE, HEIGHT_1, WIDTH_1, XF_NPPCX, XF_CV_DEPTH_IN_1>(map_x1,
                                                                                                         mapX1_stitch);
    xf::cv::Array2xfMat<MAPXY_TYPE_PTR_WIDTH, MAPXY_TYPE, HEIGHT_2, WIDTH_2, XF_NPPCX, XF_CV_DEPTH_IN_1>(map_x2,
                                                                                                         mapX2_stitch);
    xf::cv::Array2xfMat<MAPXY_TYPE_PTR_WIDTH, MAPXY_TYPE, HEIGHT_3, WIDTH_3, XF_NPPCX, XF_CV_DEPTH_IN_1>(map_x3,
                                                                                                         mapX3_stitch);
    xf::cv::Array2xfMat<MAPXY_TYPE_PTR_WIDTH, MAPXY_TYPE, HEIGHT_4, WIDTH_4, XF_NPPCX, XF_CV_DEPTH_IN_1>(map_x4,
                                                                                                         mapX4_stitch);
    xf::cv::Array2xfMat<MAPXY_TYPE_PTR_WIDTH, MAPXY_TYPE, HEIGHT_1, WIDTH_1, XF_NPPCX, XF_CV_DEPTH_IN_1>(map_y1,
                                                                                                         mapY1_stitch);
    xf::cv::Array2xfMat<MAPXY_TYPE_PTR_WIDTH, MAPXY_TYPE, HEIGHT_2, WIDTH_2, XF_NPPCX, XF_CV_DEPTH_IN_1>(map_y2,
                                                                                                         mapY2_stitch);
    xf::cv::Array2xfMat<MAPXY_TYPE_PTR_WIDTH, MAPXY_TYPE, HEIGHT_3, WIDTH_3, XF_NPPCX, XF_CV_DEPTH_IN_1>(map_y3,
                                                                                                         mapY3_stitch);
    xf::cv::Array2xfMat<MAPXY_TYPE_PTR_WIDTH, MAPXY_TYPE, HEIGHT_4, WIDTH_4, XF_NPPCX, XF_CV_DEPTH_IN_1>(map_y4,
                                                                                                         mapY4_stitch);
    xf::cv::Array2xfMat<MASK_INPUT_PTR_WIDTH, IN_MASK_TYPE, HEIGHT_1, WIDTH_1, XF_NPPCX, XF_CV_DEPTH_IN_1>(mask_img1,
                                                                                                           maskSrc1);
    xf::cv::Array2xfMat<MASK_INPUT_PTR_WIDTH, IN_MASK_TYPE, HEIGHT_2, WIDTH_2, XF_NPPCX, XF_CV_DEPTH_IN_1>(mask_img2,
                                                                                                           maskSrc2);
    xf::cv::Array2xfMat<MASK_INPUT_PTR_WIDTH, IN_MASK_TYPE, HEIGHT_3, WIDTH_3, XF_NPPCX, XF_CV_DEPTH_IN_1>(mask_img3,
                                                                                                           maskSrc3);
    xf::cv::Array2xfMat<MASK_INPUT_PTR_WIDTH, IN_MASK_TYPE, HEIGHT_4, WIDTH_4, XF_NPPCX, XF_CV_DEPTH_IN_1>(mask_img4,
                                                                                                           maskSrc4);

    // De-Bayer
    xf::cv::Mat<OUT_TYPE, HEIGHT, WIDTH, XF_NPPCX, XF_CV_DEPTH_OUT_1> RGB2GRAYOutput_1(rows, cols);
    xf::cv::Mat<OUT_TYPE, HEIGHT, WIDTH, XF_NPPCX, XF_CV_DEPTH_OUT_1> RGB2GRAYOutput_2(rows, cols);
    xf::cv::Mat<OUT_TYPE, HEIGHT, WIDTH, XF_NPPCX, XF_CV_DEPTH_OUT_1> RGB2GRAYOutput_3(rows, cols);
    xf::cv::Mat<OUT_TYPE, HEIGHT, WIDTH, XF_NPPCX, XF_CV_DEPTH_OUT_1> RGB2GRAYOutput_4(rows, cols);

    // CvtColor
    xf::cv::rgb2gray<IN_TYPE, OUT_TYPE, HEIGHT, WIDTH, XF_NPPCX, XF_CV_DEPTH_IN_1, XF_CV_DEPTH_OUT_1>(imgInput1,
                                                                                                      RGB2GRAYOutput_1);
    xf::cv::rgb2gray<IN_TYPE, OUT_TYPE, HEIGHT, WIDTH, XF_NPPCX, XF_CV_DEPTH_IN_1, XF_CV_DEPTH_OUT_1>(imgInput2,
                                                                                                      RGB2GRAYOutput_2);
    xf::cv::rgb2gray<IN_TYPE, OUT_TYPE, HEIGHT, WIDTH, XF_NPPCX, XF_CV_DEPTH_IN_1, XF_CV_DEPTH_OUT_1>(imgInput3,
                                                                                                      RGB2GRAYOutput_3);
    xf::cv::rgb2gray<IN_TYPE, OUT_TYPE, HEIGHT, WIDTH, XF_NPPCX, XF_CV_DEPTH_IN_1, XF_CV_DEPTH_OUT_1>(imgInput4,
                                                                                                      RGB2GRAYOutput_4);

    xf::cv::remap<XF_WIN_ROWS, XF_REMAP_INTERPOLATION_TYPE, OUT_TYPE, MAPXY_TYPE, OUT_TYPE, HEIGHT, WIDTH, HEIGHT_1,
                  WIDTH_1, XF_NPPCX, XF_USE_URAM_REMAP_1, XF_CV_DEPTH_IN_1, XF_CV_DEPTH_IN_1, XF_CV_DEPTH_IN_1,
                  XF_CV_DEPTH_OUT_1>(RGB2GRAYOutput_1, remapOutput1_stitch, mapX1_stitch, mapY1_stitch);
    xf::cv::remap<XF_WIN_ROWS, XF_REMAP_INTERPOLATION_TYPE, OUT_TYPE, MAPXY_TYPE, OUT_TYPE, HEIGHT, WIDTH, HEIGHT_2,
                  WIDTH_2, XF_NPPCX, XF_USE_URAM_REMAP_2, XF_CV_DEPTH_IN_1, XF_CV_DEPTH_IN_1, XF_CV_DEPTH_IN_1,
                  XF_CV_DEPTH_OUT_1>(RGB2GRAYOutput_2, remapOutput2_stitch, mapX2_stitch, mapY2_stitch);
    xf::cv::remap<XF_WIN_ROWS, XF_REMAP_INTERPOLATION_TYPE, OUT_TYPE, MAPXY_TYPE, OUT_TYPE, HEIGHT, WIDTH, HEIGHT_3,
                  WIDTH_3, XF_NPPCX, XF_USE_URAM_REMAP_3, XF_CV_DEPTH_IN_1, XF_CV_DEPTH_IN_1, XF_CV_DEPTH_IN_1,
                  XF_CV_DEPTH_OUT_1>(RGB2GRAYOutput_3, remapOutput3_stitch, mapX3_stitch, mapY3_stitch);
    xf::cv::remap<XF_WIN_ROWS, XF_REMAP_INTERPOLATION_TYPE, OUT_TYPE, MAPXY_TYPE, OUT_TYPE, HEIGHT, WIDTH, HEIGHT_4,
                  WIDTH_4, XF_NPPCX, XF_USE_URAM_REMAP_4, XF_CV_DEPTH_IN_1, XF_CV_DEPTH_IN_1, XF_CV_DEPTH_IN_1,
                  XF_CV_DEPTH_OUT_1>(RGB2GRAYOutput_4, remapOutput4_stitch, mapX4_stitch, mapY4_stitch);

    xf::cv::waitMat<HEIGHT_1, WIDTH_1, XF_WIN_ROWS, IN_MASK_TYPE, XF_NPPCX, XF_CV_DEPTH_IN_1>(maskSrc1, maskInput1);
    xf::cv::waitMat<HEIGHT_2, WIDTH_2, XF_WIN_ROWS, IN_MASK_TYPE, XF_NPPCX, XF_CV_DEPTH_IN_1>(maskSrc2, maskInput2);
    xf::cv::waitMat<HEIGHT_3, WIDTH_3, XF_WIN_ROWS, IN_MASK_TYPE, XF_NPPCX, XF_CV_DEPTH_IN_1>(maskSrc3, maskInput3);
    xf::cv::waitMat<HEIGHT_4, WIDTH_4, XF_WIN_ROWS, IN_MASK_TYPE, XF_NPPCX, XF_CV_DEPTH_IN_1>(maskSrc4, maskInput4);

    xf::cv::Mat<OUT_TYPE, HEIGHT_DST, WIDTH_DST, XF_NPPCX, XF_CV_DEPTH_OUT_1> imgOutput(dst_height, dst_width);
    xf::cv::stitch<OUT_TYPE, IN_MASK_TYPE, HEIGHT_1, WIDTH_1, HEIGHT_2, WIDTH_2, HEIGHT_3, WIDTH_3, HEIGHT_4, WIDTH_4,
                   HEIGHT_DST, WIDTH_DST, XF_NPPCX, XF_CV_DEPTH_IN_1, XF_CV_DEPTH_OUT_1>(
        remapOutput1_stitch, remapOutput2_stitch, remapOutput3_stitch, remapOutput4_stitch, maskInput1, maskInput2,
        maskInput3, maskInput4, mask_corners, imgOutput);

    // Resize
    xf::cv::resize<INTERPOLATION, OUT_TYPE, HEIGHT_DST, WIDTH_DST, HEIGHT, WIDTH, XF_NPPCX, XF_USE_URAM_RESIZE,
                   MAXDOWNSCALE, XF_COMB_DEPTH, XF_CV_DEPTH_OUT_1>(imgOutput, resizedOutput);

    xf::cv::Mat<OUT_TYPE, HEIGHT, WIDTH, XF_NPPCX, XF_CV_DEPTH_OUT_1> resizedOutput_duplicate_1(rows, cols);
    xf::cv::Mat<OUT_TYPE, HEIGHT, WIDTH, XF_NPPCX, XF_CV_DEPTH_OUT_1> resizedOutput_duplicate_2(rows, cols);
    xf::cv::duplicateMat(resizedOutput, resizedOutput_duplicate_1, resizedOutput_duplicate_2);

    // Gaussian Blur
    xf::cv::GaussianBlur<GAUSSIAN_FILTER_WIDTH, XF_BORDER_CONSTANT, OUT_TYPE, HEIGHT, WIDTH, XF_NPPCX, XF_CV_DEPTH_IN_1,
                         XF_CV_DEPTH_OUT_1>(resizedOutput_duplicate_1, gaussianOutput, sigma);

    // Sobel Edge
    xf::cv::Sobel<XF_BORDER_CONSTANT, SOBEL_FILTER_WIDTH, OUT_TYPE, OUT_TYPE_1, HEIGHT, WIDTH, XF_NPPCX,
                  XF_USE_URAM_SOBEL, XF_CV_DEPTH_IN_1, XF_CV_DEPTH_OUT_1, XF_CV_DEPTH_OUT_1>(gaussianOutput, _dstgx,
                                                                                             _dstgy);

    // Magnitude
    xf::cv::magnitude<NORM_TYPE, OUT_TYPE_1, OUT_TYPE_1, HEIGHT, WIDTH, XF_NPPCX, XF_CV_DEPTH_IN_1, XF_CV_DEPTH_IN_1,
                      XF_CV_DEPTH_OUT_1>(_dstgx, _dstgy, magOutput);

    // Convert bit depth
    xf::cv::convertTo<OUT_TYPE_1, OUT_TYPE, HEIGHT, WIDTH, XF_NPPCX, XF_CV_DEPTH_IN_1, XF_CV_DEPTH_OUT_1>(
        magOutput, convertOutput, CONVERT_TYPE, shift);

    // Threshold
    xf::cv::Threshold<THRESH_TYPE, OUT_TYPE, HEIGHT, WIDTH, XF_NPPCX, XF_CV_DEPTH_IN_1, XF_CV_DEPTH_OUT_1>(
        convertOutput, thresOutput, thresh, maxval);

    // Contour Detection
    xf::cv::findcontours<OUT_TYPE, OUTPUT_PTR_WIDTH, INPUT_PTR_WIDTH, HEIGHT, WIDTH, MAX_TOTAL_POINTS, MAX_CONTOURS,
                         XF_NPPCX, XF_CV_DEPTH_IN_1>(thresOutput, rows, cols, points_packed, contour_offsets,
                                                     local_num);

    num_contours[0] = local_num;

    xfMat2Array<OUTPUT_PTR_WIDTH, OUT_TYPE, HEIGHT, WIDTH, XF_NPPCX, XF_CV_DEPTH_OUT_1>(
        resizedOutput_duplicate_2, resizedOutput_duplicate_2_stream);

    return;

} // End of kernel