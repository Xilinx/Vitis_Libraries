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

#include "xf_crop_accel_config.h"
extern "C" {
void crop_accel(ap_uint<INPUT_PTR_WIDTH>* img_in,
                ap_uint<OUTPUT_PTR_WIDTH>* _dst,
                ap_uint<OUTPUT_PTR_WIDTH>* _dst1,
                ap_uint<OUTPUT_PTR_WIDTH>* _dst2,
                int* roi,
                int height,
                int width)
//	void crop_accel(ap_uint<INPUT_PTR_WIDTH> *img_in, ap_uint<OUTPUT_PTR_WIDTH> *_dst,int *roi, int height, int
// width)
{
// clang-format off
    #pragma HLS INTERFACE m_axi     port=img_in  offset=slave bundle=gmem0
    #pragma HLS INTERFACE m_axi     port=_dst  offset=slave bundle=gmem1
    #pragma HLS INTERFACE m_axi     port=_dst1  offset=slave bundle=gmem2
    #pragma HLS INTERFACE m_axi     port=_dst2  offset=slave bundle=gmem3
    #pragma HLS INTERFACE m_axi     port=roi   offset=slave bundle=gmem4
    #pragma HLS INTERFACE s_axilite port=height     
    #pragma HLS INTERFACE s_axilite port=width     
    #pragma HLS INTERFACE s_axilite port=return
    // clang-format on

    printf("started loading rect execution\n");
    xf::cv::Rect_<unsigned int> _roi[NUM_ROI];
    for (int i = 0, j = 0; j < (NUM_ROI * 4); i++, j += 4) {
        _roi[i].x = roi[j];
        _roi[i].y = roi[j + 1];
        _roi[i].height = roi[j + 2];
        _roi[i].width = roi[j + 3];
    }

#if MEMORYMAPPED_ARCH
    xf::cv::Mat<IN_TYPE, HEIGHT, WIDTH, NPPCX, XF_CV_DEPTH_IN> in_mat(height, width, img_in);
    xf::cv::Mat<OUT_TYPE, HEIGHT, WIDTH, NPPCX, XF_CV_DEPTH_OUT> out_mat(_roi[0].height, _roi[0].width, _dst);
    xf::cv::Mat<OUT_TYPE, HEIGHT, WIDTH, NPPCX, XF_CV_DEPTH_OUT_1> out_mat1(_roi[1].height, _roi[1].width, _dst1);
    xf::cv::Mat<OUT_TYPE, HEIGHT, WIDTH, NPPCX, XF_CV_DEPTH_OUT_2> out_mat2(_roi[2].height, _roi[2].width, _dst2);

    xf::cv::crop<OUT_TYPE, HEIGHT, WIDTH, MEMORYMAPPED_ARCH, NPPCX, XF_CV_DEPTH_IN, XF_CV_DEPTH_OUT>(in_mat, out_mat,
                                                                                                     _roi[0]);

    xf::cv::crop<OUT_TYPE, HEIGHT, WIDTH, MEMORYMAPPED_ARCH, NPPCX, XF_CV_DEPTH_IN, XF_CV_DEPTH_OUT_1>(in_mat, out_mat1,
                                                                                                       _roi[1]);

    xf::cv::crop<OUT_TYPE, HEIGHT, WIDTH, MEMORYMAPPED_ARCH, NPPCX, XF_CV_DEPTH_IN, XF_CV_DEPTH_OUT_2>(in_mat, out_mat2,
                                                                                                       _roi[2]);
#else

    xf::cv::Mat<IN_TYPE, HEIGHT, WIDTH, NPPCX, XF_CV_DEPTH_IN> in_mat(height, width);
    xf::cv::Mat<IN_TYPE, HEIGHT, WIDTH, NPPCX, XF_CV_DEPTH_IN> in_mat1(height, width);
    xf::cv::Mat<IN_TYPE, HEIGHT, WIDTH, NPPCX, XF_CV_DEPTH_IN> in_mat2(height, width);
    xf::cv::Mat<IN_TYPE, HEIGHT, WIDTH, NPPCX, XF_CV_DEPTH_IN> in_mat3(height, width);
    xf::cv::Mat<OUT_TYPE, HEIGHT, WIDTH, NPPCX, XF_CV_DEPTH_OUT> out_mat(_roi[0].height, _roi[0].width);
    xf::cv::Mat<OUT_TYPE, HEIGHT, WIDTH, NPPCX, XF_CV_DEPTH_OUT_1> out_mat1(_roi[1].height, _roi[1].width);
    xf::cv::Mat<OUT_TYPE, HEIGHT, WIDTH, NPPCX, XF_CV_DEPTH_OUT_2> out_mat2(_roi[2].height, _roi[2].width);

    xf::cv::Array2xfMat<INPUT_PTR_WIDTH, IN_TYPE, HEIGHT, WIDTH, NPPCX, XF_CV_DEPTH_IN>(img_in, in_mat);

    xf::cv::duplicateimages<IN_TYPE, HEIGHT, WIDTH, NPPCX, XF_CV_DEPTH_IN, XF_CV_DEPTH_IN, XF_CV_DEPTH_IN,
                            XF_CV_DEPTH_IN>(in_mat, in_mat1, in_mat2, in_mat3);

    xf::cv::crop<OUT_TYPE, HEIGHT, WIDTH, MEMORYMAPPED_ARCH, NPPCX, XF_CV_DEPTH_IN, XF_CV_DEPTH_OUT>(in_mat1, out_mat,
                                                                                                     _roi[0]);

    xf::cv::crop<OUT_TYPE, HEIGHT, WIDTH, MEMORYMAPPED_ARCH, NPPCX, XF_CV_DEPTH_IN, XF_CV_DEPTH_OUT_1>(
        in_mat2, out_mat1, _roi[1]);

    xf::cv::crop<OUT_TYPE, HEIGHT, WIDTH, MEMORYMAPPED_ARCH, NPPCX, XF_CV_DEPTH_IN, XF_CV_DEPTH_OUT_2>(
        in_mat3, out_mat2, _roi[2]);
    xf::cv::xfMat2Array<OUTPUT_PTR_WIDTH, OUT_TYPE, HEIGHT, WIDTH, NPPCX, XF_CV_DEPTH_OUT>(out_mat, _dst);
    xf::cv::xfMat2Array<OUTPUT_PTR_WIDTH, OUT_TYPE, HEIGHT, WIDTH, NPPCX, XF_CV_DEPTH_OUT_1>(out_mat1, _dst1);
    xf::cv::xfMat2Array<OUTPUT_PTR_WIDTH, OUT_TYPE, HEIGHT, WIDTH, NPPCX, XF_CV_DEPTH_OUT_1>(out_mat2, _dst2);
#endif
}
}
