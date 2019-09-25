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

#include "common/xf_headers.hpp"
#include "xf_stereo_pipeline_config.h"
#include "cameraParameters.h"

using namespace std;

int main(int argc, char** argv) {
    cv::setUseOptimized(false);

    if (argc != 3) {
        fprintf(stderr, "Invalid Number of Arguments!\nUsage: <executable> <left image> <right image>\n");
        return -1;
    }

    cv::Mat left_img, right_img;
    left_img = cv::imread(argv[1], 0);
    right_img = cv::imread(argv[2], 0);

    //////////////////	HLS TOP Function Call  ////////////////////////
    xf::cv::Mat<XF_8UC1, XF_HEIGHT, XF_WIDTH, XF_NPPC1> leftMat(left_img.rows, left_img.cols);
    xf::cv::Mat<XF_8UC1, XF_HEIGHT, XF_WIDTH, XF_NPPC1> rightMat(right_img.rows, right_img.cols);

    int rows = left_img.rows;
    int cols = left_img.cols;

    static xf::cv::Mat<XF_32FC1, XF_HEIGHT, XF_WIDTH, XF_NPPC1> mapxLMat(rows, cols);
    static xf::cv::Mat<XF_32FC1, XF_HEIGHT, XF_WIDTH, XF_NPPC1> mapyLMat(rows, cols);
    static xf::cv::Mat<XF_32FC1, XF_HEIGHT, XF_WIDTH, XF_NPPC1> mapxRMat(rows, cols);
    static xf::cv::Mat<XF_32FC1, XF_HEIGHT, XF_WIDTH, XF_NPPC1> mapyRMat(rows, cols);

    static xf::cv::Mat<XF_8UC1, XF_HEIGHT, XF_WIDTH, XF_NPPC1> leftRemappedMat(rows, cols);
    static xf::cv::Mat<XF_8UC1, XF_HEIGHT, XF_WIDTH, XF_NPPC1> rightRemappedMat(rows, cols);

    static xf::cv::Mat<XF_16UC1, XF_HEIGHT, XF_WIDTH, XF_NPPC1> dispMat(rows, cols);

    // camera parameters for rectification

    ap_fixed<32, 12>* cameraMA_l_fix = (ap_fixed<32, 12>*)malloc(XF_CAMERA_MATRIX_SIZE * sizeof(ap_fixed<32, 12>));
    ap_fixed<32, 12>* cameraMA_r_fix = (ap_fixed<32, 12>*)malloc(XF_CAMERA_MATRIX_SIZE * sizeof(ap_fixed<32, 12>));
    ap_fixed<32, 12>* irA_l_fix = (ap_fixed<32, 12>*)malloc(XF_CAMERA_MATRIX_SIZE * sizeof(ap_fixed<32, 12>));
    ap_fixed<32, 12>* irA_r_fix = (ap_fixed<32, 12>*)malloc(XF_CAMERA_MATRIX_SIZE * sizeof(ap_fixed<32, 12>));
    ap_fixed<32, 12>* distC_l_fix = (ap_fixed<32, 12>*)malloc(XF_DIST_COEFF_SIZE * sizeof(ap_fixed<32, 12>));
    ap_fixed<32, 12>* distC_r_fix = (ap_fixed<32, 12>*)malloc(XF_DIST_COEFF_SIZE * sizeof(ap_fixed<32, 12>));

    //	leftMat.copyTo(left_img.data);
    //	rightMat.copyTo(right_img.data);
    leftMat = xf::cv::imread<XF_8UC1, XF_HEIGHT, XF_WIDTH, XF_NPPC1>(argv[1], 0);
    rightMat = xf::cv::imread<XF_8UC1, XF_HEIGHT, XF_WIDTH, XF_NPPC1>(argv[2], 0);

    xf::cv::xFSBMState<SAD_WINDOW_SIZE, NO_OF_DISPARITIES, PARALLEL_UNITS> bm_state;
    bm_state.preFilterCap = 31;
    bm_state.uniquenessRatio = 15;
    bm_state.textureThreshold = 20;
    bm_state.minDisparity = 0;

    // copy camera params
    for (int i = 0; i < XF_CAMERA_MATRIX_SIZE; i++) {
        cameraMA_l_fix[i] = (ap_fixed<32, 12>)cameraMA_l[i];
        cameraMA_r_fix[i] = (ap_fixed<32, 12>)cameraMA_r[i];
        irA_l_fix[i] = (ap_fixed<32, 12>)irA_l[i];
        irA_r_fix[i] = (ap_fixed<32, 12>)irA_r[i];
    }

    // copy distortion coefficients
    for (int i = 0; i < XF_DIST_COEFF_SIZE; i++) {
        distC_l_fix[i] = (ap_fixed<32, 12>)distC_l[i];
        distC_r_fix[i] = (ap_fixed<32, 12>)distC_r[i];
    }

    printf("starting the kernel...\n");
    stereopipeline_accel(leftMat, rightMat, dispMat, mapxLMat, mapyLMat, mapxRMat, mapyRMat, leftRemappedMat,
                         rightRemappedMat, bm_state, cameraMA_l_fix, cameraMA_r_fix, distC_l_fix, distC_r_fix,
                         irA_l_fix, irA_r_fix, 9, 5);
    printf("end of kernel...\n");

    cv::Mat out_disp_16(rows, cols, CV_16UC1);
    cv::Mat out_disp_img(rows, cols, CV_8UC1);

    out_disp_16.data = dispMat.copyFrom();

    out_disp_16.convertTo(out_disp_img, CV_8U, (256.0 / NO_OF_DISPARITIES) / (16.));
    imwrite("hls_output.png", out_disp_img);
    printf("run complete !\n\n");

    return 0;
}
