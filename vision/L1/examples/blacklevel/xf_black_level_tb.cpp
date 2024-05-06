/*
 * Copyright (C) 2019-2022, Xilinx, Inc.
 * Copyright (C) 2022-2023, Advanced Micro Devices, Inc.
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

#ifndef __XF_BLACK_LEVEL_TB_CPP__
#define __XF_BLACK_LEVEL_TB_CPP__

#include "common/xf_headers.hpp"
#include "xf_vision_message.h"
#define XF_HLS_MODE 1
#include "xf_black_level_tb_config.h"
#define BLACK_LEVEL 32

#if XF_HLS_MODE
void blackLevelCorrection_accel(ap_uint<IMAGE_PTR_WIDTH>* in_img_ptr,
                                ap_uint<IMAGE_PTR_WIDTH>* out_img_ptr,
                                uint32_t black_level,
                                int mul_value,
                                int height,
                                int width);
#endif

int main(int argc, char** argv) {
    XfVisionMessage msg;

    int height, width;

    cv::Mat InImg, OutImg, RefImg;

    int BlackLevel = 32;
#if T_8U
    const int MaxLevel = 255; // Or White Level
#else
    const int MaxLevel = 65535; // Or White Level
#endif
    typedef uint8_t Pixel_t;

    float MulValue = (float)(MaxLevel / (MaxLevel - BlackLevel));
    ; // int((MaxLevel*(1<<IMAGE_MUL_FL_BITS))/(MaxLevel - BlackLevel));
    unsigned int blc_config_1 = (int)(MulValue * 65536); // mul_fact int Q16_16 format
    unsigned int blc_config_2 = BlackLevel;
#if T_8U
    InImg = cv::imread(argv[1], 0);
#else
    InImg = cv::imread(argv[1], -1);
#endif
    height = InImg.rows;
    width = InImg.cols;

    printf("%d %d\n", height, width);

    if (argc != 2) return msg.error("Incorrect Usage. Usage: <app.exe> <Input image>");

    // OutImg.create(cv::Size(img.cols, img.rows), img.type());

    RefImg.create(InImg.size(), InImg.type());
    OutImg.create(InImg.size(), InImg.type());

    msg.info("Creating reference for comparision ...");

    for (int r = 0; r < height; r++) {
        for (int c = 0; c < width; c++) {
            Pixel_t Pixel = InImg.at<Pixel_t>(r, c);
            RefImg.at<Pixel_t>(r, c) = cv::saturate_cast<unsigned char>((Pixel - BlackLevel) * MulValue);
        }
    }

#if XF_HLS_MODE

    blackLevelCorrection_accel((ap_uint<IMAGE_PTR_WIDTH>*)InImg.data, (ap_uint<IMAGE_PTR_WIDTH>*)OutImg.data,
                               blc_config_2, blc_config_1, height, width);

#else

    msg.info("Registering Kernel ...");

    (void)cl_kernel_mgr::registerKernel("blackLevelCorrection_accel", "krnl_black_level", XCLIN(InImg), XCLOUT(OutImg),
                                        XCLIN(blc_config_2), XCLIN(blc_config_1), XCLIN(height), XCLIN(width));

    msg.info("Executing HW Kernel ...");
    cl_kernel_mgr::exec_all();

#endif

    msg.info("Dumping reference output ...");
    cv::imwrite("sw_ref_output.png", RefImg);

    msg.info("Dumping Kernel ouptut ...");
    cv::imwrite("hw_output.png", OutImg);
    FILE* fp3 = fopen("kernel_out.csv", "w");
    for (int i = 0; i < InImg.rows; ++i) {
        for (int j = 0; j < InImg.cols; ++j) {
            Pixel_t val = OutImg.at<Pixel_t>(i, j);
            fprintf(fp3, "%d,", val);
        }
        fprintf(fp3, "");
    }
    fclose(fp3);

    msg.info("Error checking ...");

    cv::Mat DiffImg;
    float ErrorPercent;

    DiffImg.create(InImg.size(), InImg.type());

    cv::absdiff(RefImg, OutImg, DiffImg);

    msg.info("Dumping difference ...");
    cv::imwrite("sw_ref_hw_diff.png", DiffImg);

    xf::cv::analyzeDiff(DiffImg, 1, ErrorPercent);

    if (ErrorPercent > 10) {
        fprintf(stderr, "ERROR: Test Failed.\n ");
        return 1;
    } else
        std::cout << "Test Passed " << std::endl;

    // return XF_APP_SUCCESS;
}

#endif //__XF_BLACK_LEVEL_TB_CPP__
