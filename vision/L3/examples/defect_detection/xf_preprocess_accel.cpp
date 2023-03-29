/*
 * Copyright 2022 Xilinx, Inc.
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

#include "xf_threshold_config.h"

extern "C" {
void preprocess_accel(ap_uint<INPUT_PTR_WIDTH>* img_inp,
                      ap_uint<OUTPUT_PTR_WIDTH>* img_out,
                      ap_uint<OUTPUT_PTR_WIDTH>* fw_img_out,
                      int* obj_pix,
                      unsigned char thresh,
                      unsigned char maxval,
                      int rows,
                      int cols,
                      int stride) {
// clang-format off
    #pragma HLS INTERFACE m_axi     port=img_inp  offset=slave bundle=gmem1
    #pragma HLS INTERFACE m_axi     port=img_out  offset=slave bundle=gmem2
    #pragma HLS INTERFACE m_axi     port=fw_img_out  offset=slave bundle=gmem3 
    #pragma HLS INTERFACE m_axi     port=obj_pix offset=slave bundle=gmem4 

    #pragma HLS INTERFACE s_axilite port=thresh     
    #pragma HLS INTERFACE s_axilite port=maxval     
    #pragma HLS INTERFACE s_axilite port=rows     
    #pragma HLS INTERFACE s_axilite port=cols     
    #pragma HLS INTERFACE s_axilite port=stride   
    #pragma HLS INTERFACE s_axilite port=return
    // clang-format on

    const int pROWS = HEIGHT;
    const int pCOLS = WIDTH;
    const int pNPC1 = NPIX;
    int tmp_obj;

    xf::cv::Mat<XF_8UC1, HEIGHT, WIDTH, NPIX, XF_CV_DEPTH_IN> in_mat(rows, cols);
    xf::cv::Mat<TYPE, HEIGHT, WIDTH, NPC1, XF_CV_DEPTH_OUT> imgOutput(rows, cols);

    xf::cv::Mat<XF_8UC1, HEIGHT, WIDTH, NPIX, XF_CV_DEPTH_OUT> out_mat(rows, cols);

    xf::cv::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1, XF_CV_DEPTH_OUT> in_mat_fw(rows, cols);
    xf::cv::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1, XF_CV_DEPTH_OUT> out_mat_fw(rows, cols);
    xf::cv::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1, XF_CV_DEPTH_OUT> out_mat_ret(rows, cols);

// clang-format off
    #pragma HLS DATAFLOW
    // clang-format on

    xf::cv::Array2xfMat<INPUT_PTR_WIDTH, XF_8UC1, HEIGHT, WIDTH, NPIX, XF_CV_DEPTH_IN>(img_inp, in_mat);

    xf::cv::Threshold<THRESH_TYPE, XF_8UC1, HEIGHT, WIDTH, NPIX, XF_CV_DEPTH_IN, XF_CV_DEPTH_OUT>(in_mat, out_mat,
                                                                                                  thresh, maxval);

    // Run xfOpenCV median blur kernel:
    /*    xf::cv::medianBlur<WINDOW_SIZE, XF_BORDER_REPLICATE, TYPE, HEIGHT, WIDTH, NPC1>(out_mat, imgOutput);

        xf::cv::xfMat2Array<OUTPUT_PTR_WIDTH, XF_8UC1, HEIGHT, WIDTH, NPIX>(imgOutput, img_out);*/

    xf::cv::duplicateMat<TYPE, HEIGHT, WIDTH, NPIX, XF_CV_DEPTH_OUT, XF_CV_DEPTH_OUT, XF_CV_DEPTH_OUT>(
        out_mat, in_mat_fw, out_mat_ret);

    xf::cv::fw_cca<TYPE, HEIGHT, WIDTH, NPIX>(in_mat_fw, out_mat_fw, tmp_obj, rows, cols);

    xf::cv::xfMat2Array<OUTPUT_PTR_WIDTH, TYPE, HEIGHT, WIDTH, NPIX, XF_CV_DEPTH_OUT, 1>(out_mat_ret, img_out, stride);

    xf::cv::xfMat2Array<OUTPUT_PTR_WIDTH, TYPE, HEIGHT, WIDTH, NPIX, XF_CV_DEPTH_OUT, 1>(out_mat_fw, fw_img_out,
                                                                                         stride);

    *obj_pix = tmp_obj;
}
}
