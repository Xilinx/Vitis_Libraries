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

#include <cstdint>
#include "common/xf_headers.hpp"
#include "common/xf_params.hpp"
#include "xf_config_params.h"
#include "xf_layout_formatter_config.h"
#include "common/xf_axi.hpp"

#define ERROR_THRESHOLD 2
#if _XF_RGBA_
#define CHANNELS 4
#else
#define CHANNELS 3
#endif

int main(int argc, char* argv[]) {
    // cv::Mat imgInput0, imgInput1, refOutput0, result_hls, result_hls1, result_hls2, result_hls3, result_ocv, error,
    // refResize;
    cv::Mat imgInput0, imgInput1, refOutput0, result_hls, result_hls4, result_ocv, error, refResize;

    imgInput0 = cv::imread(argv[1], 1);
    if (!imgInput0.data) {
        fprintf(stderr, "Can't open image %s !!.\n ", argv[1]);
        return -1;
    }

    int width, height;

    width = imgInput0.cols;
    height = imgInput0.rows;
    std::cout << "Input image height : " << height << std::endl;
    std::cout << "Input image width  : " << width << std::endl;

#if _XF_RGBA_
    cv::cvtColor(imgInput0, imgInput0, cv::COLOR_BGR2BGRA);
#endif

    if (ENABLE_FP32)
        imgInput0.convertTo(imgInput0, CV_32F);

    else if (ENABLE_FP16 || ENABLE_BF16)
        imgInput0.convertTo(imgInput0, CV_16U);

    FILE* fp1 = fopen("Input.txt", "w");
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            cv::Vec<TB_PIX_TYPE, CHANNELS> pixVal = imgInput0.at<cv::Vec<TB_PIX_TYPE, CHANNELS> >(i, j);
            cv::Vec<TB_PIX_TYPE, CHANNELS> pixVal_Out = pixVal;

            pixVal_Out[0] = pixVal_Out[0] << 0;
            pixVal_Out[1] = pixVal_Out[1] << 0;
            pixVal_Out[2] = pixVal_Out[2] << 0;
#if _XF_RGBA_
            pixVal_Out[3] = pixVal_Out[3] << 0;
#endif

            imgInput0.at<cv::Vec<TB_PIX_TYPE, CHANNELS> >(i, j) = pixVal_Out;

            fprintf(fp1, "%u %u %u ", pixVal[0], pixVal[1], pixVal[2]);
#if _XF_RGBA_
            fprintf(fp1, "%u", pixVal[3]);
#endif
            fprintf(fp1, "\n");
        }
    }
    fclose(fp1);

    InStream _src;

    xf::cv::cvMat2AXIvideoxf<NPPCX>(imgInput0, _src);

    cv::Mat diff, diff1, diff2, diff3, diff4;
    std::vector<cv::Mat> refImgs;

    result_hls.create(cv::Size(width, height), CV_TYPE);
    cv::Mat result_hls1(cv::Size(width, height), CV_TYPE_NCHW);

    cv::Mat result_hls2(cv::Size(width, height), CV_TYPE_NCHW);
    cv::Mat result_hls3(cv::Size(width, height), CV_TYPE_NCHW);
    diff1.create(cv::Size(width, height), CV_TYPE_NCHW);
    diff2.create(cv::Size(width, height), CV_TYPE_NCHW);
    diff3.create(cv::Size(width, height), CV_TYPE_NCHW);
    result_hls4.create(cv::Size(width, height), CV_TYPE_NCHW);
#if _XF_RGBA_
    diff4.create(cv::Size(width, height), CV_TYPE_NCHW);
#endif

    diff.create(cv::Size(width, height), CV_TYPE);

    int layout_fmt = TB_LAYOUT_FORMAT;
    int datatype = TB_DATATYPE;
    int out_channels = TB_OUT_CHANNELS;

    layout_formatter_accel(
        _src, (ap_uint<OUTPUT_PTR_WIDTH>*)result_hls.data, (ap_uint<OUTPUT_PTR_WIDTH_NCHW>*)result_hls1.data,
        (ap_uint<OUTPUT_PTR_WIDTH_NCHW>*)result_hls2.data, (ap_uint<OUTPUT_PTR_WIDTH_NCHW>*)result_hls3.data,
        (ap_uint<OUTPUT_PTR_WIDTH_NCHW>*)result_hls4.data, height, width, layout_fmt, datatype, out_channels);
    float ErrorPercent = 0;

    //#if !_XF_NCHW_
    if ((layout_fmt == layout_format::XF_NHWC) || (layout_fmt == layout_format::XF_HCWNC4) ||
        (layout_fmt == layout_format::XF_HCWNC8)) {
        FILE* fp = fopen("Result_hls.txt", "w");
        for (int i = 0; i < height; i++) {
            for (int j = 0; j < width; j++) {
                if (layout_fmt == layout_format::XF_NHWC) {
                    cv::Vec<TB_PIX_TYPE, CHANNELS> pixVal = result_hls.at<cv::Vec<TB_PIX_TYPE, CHANNELS> >(i, j);
                    fprintf(fp, "%u %u %u ", pixVal[0], pixVal[1], pixVal[2]);
#if _XF_RGBA_
                    if (out_channels == 4) {
                        fprintf(fp, "%u ", pixVal[3]);
                    }
#endif
                } else { // HCWNC4
                    cv::Vec<TB_PIX_TYPE, 4> pixVal2 = result_hls.at<cv::Vec<TB_PIX_TYPE, 4> >(i, j);
                    fprintf(fp, "%u %u %u %u", pixVal2[0], pixVal2[1], pixVal2[2], pixVal2[3]);
                }

                fprintf(fp, "\n");
            }
        }

        fclose(fp);
    }
    //#endif
    //#if _XF_NCHW_
    if (layout_fmt == layout_format::XF_NCHW) {
        FILE* fp2 = fopen("Result_hls.txt", "w");
        for (int i = 0; i < height; i++) {
            for (int j = 0; j < width; j++) {
                TB_PIX_TYPE_NCHW pixVal = result_hls1.at<TB_PIX_TYPE_NCHW>(i, j);
                TB_PIX_TYPE_NCHW pixVal2 = result_hls2.at<TB_PIX_TYPE_NCHW>(i, j);
                TB_PIX_TYPE_NCHW pixVal3 = result_hls3.at<TB_PIX_TYPE_NCHW>(i, j);

                fprintf(fp2, "%u %u %u ", pixVal, pixVal2, pixVal3);
#if _XF_RGBA_
                TB_PIX_TYPE_NCHW pixVal4 = result_hls4.at<TB_PIX_TYPE_NCHW>(i, j);
                fprintf(fp2, "%u ", pixVal4);
#endif
                fprintf(fp2, "\n");
            }
        }

        fclose(fp2);
    }
//#endif

#if !_XF_HCWNC8_
    cv::imwrite("out_hls.jpg", result_hls);
#if _XF_NCHW_
    cv::imwrite("out_hls1.jpg", result_hls1);
    cv::imwrite("out_hls2.jpg", result_hls2);
    cv::imwrite("out_hls3.jpg", result_hls3);
#if _XF_RGBA_
    cv::imwrite("out_hls4.jpg", result_hls4);
#endif
/*    cv::split(imgInput0, refImgs);
    cv::absdiff(result_hls, refImgs[0], diff);
    cv::absdiff(result_hls1, refImgs[1], diff1);
    cv::absdiff(result_hls2, refImgs[2], diff2);

    xf::cv::analyzeDiff(diff, 1, ErrorPercent);
    xf::cv::analyzeDiff(diff1, 1, ErrorPercent);
    xf::cv::analyzeDiff(diff2, 1, ErrorPercent);
    cv::imwrite("ref.png", refImgs[0]);
    cv::imwrite("ref1.png", refImgs[1]);
    cv::imwrite("ref2.png", refImgs[2]);
    cv::imwrite("diff.png", diff);
    cv::imwrite("diff1.png", diff1);
    cv::imwrite("diff2.png", diff2);
#if _XF_RGBA_
    cv::imwrite("out_hls3.jpg", result_hls3);
    cv::absdiff(result_hls3, refImgs[3], diff3);
    xf::cv::analyzeDiff(diff3, 1, ErrorPercent);
    cv::imwrite("ref3.png", refImgs[3]);
    cv::imwrite("diff3.png", diff3);
#endif*/
#endif
#endif
    std::cout << ErrorPercent << std::endl;

    /*
        layout_format_accel(_src, (ap_uint<OUTPUT_PTR_WIDTH>*)result_hls.data,
    #if _XF_NCHW_
                            (ap_uint<OUTPUT_PTR_WIDTH>*)result_hls1.data,
                            (ap_uint<OUTPUT_PTR_WIDTH>*)result_hls2.data,
    #if _XF_RGBA_
                            (ap_uint<OUTPUT_PTR_WIDTH>*)result_hls3.data,
    #endif
    #endif
                              height, width
                              );
    float ErrorPercent=0;

    #if !_XF_HCWNC8_
        cv::imwrite("out_hls.jpg", result_hls);
    #if _XF_NCHW_
        cv::imwrite("out_hls1.jpg", result_hls1);
        cv::imwrite("out_hls2.jpg", result_hls2);
        cv::split(imgInput0, refImgs);
        cv::absdiff(result_hls, refImgs[0], diff);
        cv::absdiff(result_hls1, refImgs[1], diff1);
        cv::absdiff(result_hls2, refImgs[2], diff2);

        xf::cv::analyzeDiff(diff, 1, ErrorPercent);
        xf::cv::analyzeDiff(diff1, 1, ErrorPercent);
        xf::cv::analyzeDiff(diff2, 1, ErrorPercent);
        cv::imwrite("ref.png", refImgs[0]);
        cv::imwrite("ref1.png", refImgs[1]);
        cv::imwrite("ref2.png", refImgs[2]);
        cv::imwrite("diff.png", diff);
        cv::imwrite("diff1.png", diff1);
        cv::imwrite("diff2.png", diff2);
    #if _XF_RGBA_
        cv::imwrite("out_hls3.jpg", result_hls3);
        cv::absdiff(result_hls3, refImgs[3], diff3);
        xf::cv::analyzeDiff(diff3, 1, ErrorPercent);
        cv::imwrite("ref3.png", refImgs[3]);
        cv::imwrite("diff3.png", diff3);
    #endif
    #endif
    #endif
        std::cout << ErrorPercent << std::endl;*/
}
