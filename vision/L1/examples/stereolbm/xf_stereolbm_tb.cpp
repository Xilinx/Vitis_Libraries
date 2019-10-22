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
#include "xf_stereolbm_config.h"

using namespace std;

#define _TEXTURE_THRESHOLD_ 20
#define _UNIQUENESS_RATIO_ 15
#define _PRE_FILTER_CAP_ 31
#define _MIN_DISP_ 0

int main(int argc, char** argv) {
    cv::setUseOptimized(false);

    if (argc != 3) {
        fprintf(stderr, "Invalid Number of Arguments!\nUsage:\n");
        return -1;
    }

    cv::Mat left_img, right_img;

    left_img = cv::imread(argv[1], 0);
    right_img = cv::imread(argv[2], 0);

    cv::Mat disp;

    //////////////////      OCV reference Function ////////////////////////

    // OpenCV reference function: enable this for older Opencv version
    cv::StereoBM bm;
    bm.state->preFilterCap = _PRE_FILTER_CAP_;
    bm.state->preFilterType = CV_STEREO_BM_XSOBEL;
    bm.state->SADWindowSize = SAD_WINDOW_SIZE;
    bm.state->minDisparity = _MIN_DISP_;
    bm.state->numberOfDisparities = NO_OF_DISPARITIES;
    bm.state->textureThreshold = _TEXTURE_THRESHOLD_;
    bm.state->uniquenessRatio = _UNIQUENESS_RATIO_;
    bm(left_img, right_img, disp);

    // enable this reference code, based on the version of Opencv
    /*cv::Ptr<cv::StereoBM> stereobm = cv::StereoBM::create(NO_OF_DISPARITIES, SAD_WINDOW_SIZE);
    stereobm-> setPreFilterCap(_PRE_FILTER_CAP_);
    stereobm-> setUniquenessRatio(_UNIQUENESS_RATIO_);
    stereobm-> setTextureThreshold(_TEXTURE_THRESHOLD_);
    stereobm-> compute(left_img,right_img,disp);*/

    cv::Mat disp8;
    disp.convertTo(disp8, CV_8U, (256.0 / NO_OF_DISPARITIES) / (16.));
    imwrite("ocv_output.png", disp8);

    //////////////////	HLS TOP Function Call  ////////////////////////
    static xf::cv::Mat<XF_8UC1, XF_HEIGHT, XF_WIDTH, XF_NPPC1> leftMat(left_img.rows, left_img.cols);
    static xf::cv::Mat<XF_8UC1, XF_HEIGHT, XF_WIDTH, XF_NPPC1> rightMat(left_img.rows, left_img.cols);
    static xf::cv::Mat<XF_16UC1, XF_HEIGHT, XF_WIDTH, XF_NPPC1> dispMat(left_img.rows, left_img.cols);
    static xf::cv::Mat<XF_8UC1, XF_HEIGHT, XF_WIDTH, XF_NPPC1> dispMat_out(left_img.rows, left_img.cols);

    leftMat.copyTo(left_img.data);
    rightMat.copyTo(right_img.data);

    xf::cv::xFSBMState<SAD_WINDOW_SIZE, NO_OF_DISPARITIES, PARALLEL_UNITS> bm_state;
    bm_state.preFilterCap = _PRE_FILTER_CAP_;
    bm_state.uniquenessRatio = _UNIQUENESS_RATIO_;
    bm_state.textureThreshold = _TEXTURE_THRESHOLD_;
    bm_state.minDisparity = _MIN_DISP_;

    stereolbm_accel(leftMat, rightMat, dispMat, bm_state);

    dispMat.convertTo(dispMat_out, XF_CONVERT_16U_TO_8U, (256.0 / NO_OF_DISPARITIES) / (16.));
    xf::cv::imwrite("hls_out.jpg", dispMat_out);

    // changing the invalid value from negative to zero for validating the difference
    for (int i = 0; i < disp.rows; i++) {
        for (int j = 0; j < disp.cols; j++) {
            if (disp.at<short>(i, j) < 0) {
                disp.at<short>(i, j) = 0;
            }
        }
    }

    cv::Mat diff;
    diff.create(left_img.rows, left_img.cols, CV_16UC1);
    xf::cv::absDiff(disp, dispMat, diff);
    cv::imwrite("diff_img.jpg", diff);

    // removing border before diff analysis
    cv::Mat diff_c;
    diff_c.create((diff.rows - SAD_WINDOW_SIZE << 1), diff.cols - (SAD_WINDOW_SIZE << 1), CV_16UC1);
    cv::Rect roi;
    roi.x = SAD_WINDOW_SIZE;
    roi.y = SAD_WINDOW_SIZE;
    roi.width = diff.cols - (SAD_WINDOW_SIZE << 1);
    roi.height = diff.rows - (SAD_WINDOW_SIZE << 1);
    diff_c = diff(roi);

    float err_per;
    xf::cv::analyzeDiff(diff_c, 0, err_per);

    return 0;
}
