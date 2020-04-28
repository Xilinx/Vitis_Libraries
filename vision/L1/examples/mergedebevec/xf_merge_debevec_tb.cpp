/*
 *
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
 
#include <cmath>
#include <fstream>
#include <iostream>
#include <vector>
#include <time.h>

#include "opencv2/opencv.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/photo.hpp"
#include "opencv2/core/hal/hal.hpp"

#include "xf_merge_debevec_config.h"

int main(int argc, char** argv)
{
    std::vector<cv::Mat> images;
    std::vector<cv::Mat> imagesc;
    std::vector<float> times;

    if(argc != 10) {
        std::cout << "Usage: " << argv[0] << "<INPUT IMAGE PATH 1> \
                                              <INPUT IMAGE PATH 2> \
                                              <INPUT IMAGE PATH 3> \
                                              <INPUT IMAGE PATH 4> \
                                              <CALIBRATION INPUT IMAGE PATH 1> \
                                              <CALIBRATION INPUT IMAGE PATH 2> \
                                              <CALIBRATION INPUT IMAGE PATH 3> \
                                              <CALIBRATION INPUT IMAGE PATH 4> \
                                              <TIME RESPONSE DATA FILE PATH>" << std::endl;
        return EXIT_FAILURE;
    }

    for(int i = 1; i <=4; i++)
        images.push_back(cv::imread(argv[i],1));

    for(int i = 5; i <=8; i++)
        imagesc.push_back(cv::imread(argv[i],1));

    std::ifstream ifs(argv[9],std::ifstream::in);
    if(!ifs.is_open()) {
        std::cout << "ERR : Unable to open file " << argv[5] << std::endl;
        return EXIT_FAILURE;
    }

    float n;
    while(ifs >> n) times.push_back(n);

    // Align input images
    cv::Ptr<cv::AlignMTB> alignMTB = cv::createAlignMTB(6,4,false);
    alignMTB->process(images, images);
    alignMTB->process(imagesc, imagesc);

    // Obtain Camera Response Function (CRF)
    cv::Mat responseDebevec;
    cv::Ptr<cv::CalibrateDebevec> calibrateDebevec = cv::createCalibrateDebevec();
    calibrateDebevec->process(imagesc, responseDebevec, times);

    float* times_ln = (float*)malloc(SRC_NUM*sizeof(float));
    cv::hal::log32f(times.data(), times_ln, SRC_NUM);

    float* input_response_vec = (float*)responseDebevec.data;
    float* input_response_vec_ln = (float*)malloc(cv::LDR_SIZE*images[0].channels()*sizeof(float));
    cv::hal::log32f(input_response_vec, input_response_vec_ln, cv::LDR_SIZE*images[0].channels());

    struct timespec t1;
    struct timespec t2;
    clock_gettime(CLOCK_MONOTONIC, &t1);
    cv::Mat hdrDebevec;
    cv::Ptr<cv::MergeDebevec> mergeDebevec = cv::createMergeDebevec();
    mergeDebevec->process(images, hdrDebevec, times, responseDebevec);
    clock_gettime(CLOCK_MONOTONIC, &t2);
    float diff_latency = (t2.tv_nsec - t1.tv_nsec)/1e9 + t2.tv_sec - t1.tv_sec;
    std::cout << "Debevec Merge latency (opencv) : " << (float)(diff_latency*1000.f) << " ms" << std::endl;

    /*
    createTonemapDrago
    (
    float   gamma = 1.0f,
    float   saturation = 1.0f,
    float   bias = 0.85f 
    );
    */
    // Tonemap using Drago's method to obtain 24-bit color image
    cv::Mat ldrDrago;
    cv::Ptr<cv::TonemapDrago> tonemapDrago = cv::createTonemapDrago(1.0, 0.7);
    tonemapDrago->process(hdrDebevec, ldrDrago);
    ldrDrago = 3 * ldrDrago;
    cv::imwrite("ldr-Drago.jpg", ldrDrago * 255);

    //Reference opencv compile done 

    cv::Mat hdrDebevecHLS(HEIGHT,WIDTH,CV_32FC3);

    mergedebevec_accel_4((ap_uint<PTR_IN_WIDTH>*)images[0].data,
                         (ap_uint<PTR_IN_WIDTH>*)images[1].data,
                         (ap_uint<PTR_IN_WIDTH>*)images[2].data,
                         (ap_uint<PTR_IN_WIDTH>*)images[3].data,
                         (ap_uint<PTR_OUT_WIDTH>*)hdrDebevecHLS.data,
                         times_ln,
                         input_response_vec_ln,
                         images[0].rows,
                         images[0].cols
                         );

    cv::Mat diffHDR;
    cv::absdiff(hdrDebevec, hdrDebevecHLS, diffHDR);
    cv::imwrite("diffHDR.hdr", diffHDR);

    float minval = 256.0f, maxval = 0.0f;
    int cnt = 0;
    for(int i = 0; i < diffHDR.rows; i++)
    {
        for(int j = 0; j < diffHDR.cols; j++)
        {
            for(int k = 0; k < diffHDR.channels(); k++)
            {
                float d = diffHDR.at<cv::Vec3f>(i,j)[k];
                if (d > 0.01f) cnt++;
                if (minval > d) minval = d;
                if (maxval < d)  maxval = d;
            }
        }
    }

    float err_per = 100.0*(float)cnt/(hdrDebevec.rows*hdrDebevec.cols*hdrDebevec.channels());
    std::cout << "INFO: Verification results:" << std::endl;
    std::cout << "\tMinimum error in intensity = " << minval << std::endl;
    std::cout << "\tMaximum error in intensity = " << maxval << std::endl;
    std::cout << "\tPercentage of pixels above error threshold = " << err_per << std::endl;

    // Save HDR image.
    cv::imwrite("hdrDebevec.hdr", hdrDebevec);
    cv::imwrite("hdrDebevecHLS.hdr", hdrDebevecHLS);


    cv::Mat ldrDragoHLS;
    cv::Ptr<cv::TonemapDrago> tonemapDragoHLS = cv::createTonemapDrago(1.0, 0.7);
    tonemapDragoHLS->process(hdrDebevecHLS, ldrDragoHLS);
    ldrDragoHLS = 3 * ldrDragoHLS;
    cv::imwrite("ldr-DragoHLS.jpg", ldrDragoHLS * 255);

    if(err_per > 0.0f)
    {
        std::cout << "Test failed" << std::endl;
        return EXIT_FAILURE;
    }
    std::cout << "Test passed" << std::endl;
    return 0;
}
