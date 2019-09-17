/***************************************************************************
Copyright (c) 2019, Xilinx, Inc.
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors
may be used to endorse or promote products derived from this software
without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

***************************************************************************/

#include "common/xf_headers.h"
#include "xf_arithm_config.h"

int main(int argc, char** argv) {
#if ARRAY
    if (argc != 3) {
        std::cout << "Usage: " << argv[0] << " <INPUT IMAGE PATH 1> <INPUT IMAGE PATH 2>" << std::endl;
        return EXIT_FAILURE;
    }
#else
    if (argc != 2) {
        std::cout << "Usage: " << argv[0] << " <INPUT IMAGE PATH 1> " << std::endl;
        return EXIT_FAILURE;
    }

#endif
    cv::Mat in_img1, in_img2, in_gray1, in_gray2, out_img, ocv_ref, diff;

#if GRAY
    // Reading in the image:
    in_gray1 = cv::imread(argv[1], 0);

    if (in_gray1.data == NULL) {
        std::cout << "ERROR: Cannot open image " << argv[1] << std::endl;
        return EXIT_FAILURE;
    }
#else
    in_gray1 = cv::imread(argv[1], 1);

    if (in_gray1.data == NULL) {
        std::cout << "ERROR: Cannot open image " << argv[1] << std::endl;
        return EXIT_FAILURE;
    }
#endif
#if ARRAY
#if GRAY
    in_gray2 = cv::imread(argv[2], 0);

    if (in_gray2.data == NULL) {
        std::cout << "ERROR: Cannot open image " << argv[2] << std::endl;
        return EXIT_FAILURE;
    }
#else
    in_gray2 = cv::imread(argv[2], 1);

    if (in_gray2.data == NULL) {
        std::cout << "ERROR: Cannot open image " << argv[2] << std::endl;
        return EXIT_FAILURE;
    }

#endif
#endif

#if GRAY
#if T_16S
    /*  convert to 16S type  */
    in_gray1.convertTo(in_gray1, CV_16SC1);
    in_gray2.convertTo(in_gray2, CV_16SC1);
    out_img.create(in_gray1.rows, in_gray1.cols, CV_16SC1);
    ocv_ref.create(in_gray2.rows, in_gray1.cols, CV_16SC1);
    diff.create(in_gray1.rows, in_gray1.cols, CV_16SC1);
#else
    out_img.create(in_gray1.rows, in_gray1.cols, CV_8UC1);
    ocv_ref.create(in_gray2.rows, in_gray1.cols, CV_8UC1);
    diff.create(in_gray1.rows, in_gray1.cols, CV_8UC1);
#endif
#else
#if T_16S
    /*  convert to 16S type  */
    in_gray1.convertTo(in_gray1, CV_16SC3);
    in_gray2.convertTo(in_gray2, CV_16SC3);
    out_img.create(in_gray1.rows, in_gray1.cols, CV_16SC3);
    ocv_ref.create(in_gray2.rows, in_gray1.cols, CV_16SC3);
    diff.create(in_gray1.rows, in_gray1.cols, CV_16SC3);
#else
    out_img.create(in_gray1.rows, in_gray1.cols, CV_8UC3);
    ocv_ref.create(in_gray2.rows, in_gray1.cols, CV_8UC3);
    diff.create(in_gray1.rows, in_gray1.cols, CV_8UC3);
#endif
#endif

#if ARRAY
    static xf::cv::Mat<TYPE, HEIGHT, WIDTH, NPC1> imgInput1(in_gray1.rows, in_gray1.cols);
    static xf::cv::Mat<TYPE, HEIGHT, WIDTH, NPC1> imgInput2(in_gray2.rows, in_gray2.cols);
    static xf::cv::Mat<TYPE, HEIGHT, WIDTH, NPC1> imgOutput(in_gray1.rows, in_gray1.cols);

    imgInput1.copyTo(in_gray1.data);
    imgInput2.copyTo(in_gray2.data);

    ///////////// OpenCV reference  /////////////
    cv::CV_FUNCT_NAME(in_gray1, in_gray2, ocv_ref
#ifdef CV_EXTRA_ARG
                      ,
                      CV_EXTRA_ARG
#endif
    );
    //	cv::absdiff		(in_gray1,in_gray2,ocv_ref);
    //	cv::add			(in_gray1,in_gray2,ocv_ref);
    //	cv::subtract	(in_gray1,in_gray2,ocv_ref);
    //	cv::bitwise_and (in_gray1,in_gray2,ocv_ref);
    //	cv::bitwise_or  (in_gray1,in_gray2,ocv_ref);
    //	cv::bitwise_xor (in_gray1,in_gray2,ocv_ref);
    //	cv::multiply	(in_gray1,in_gray2,ocv_ref,0.05);
    //		cv::compare (in_gray1,in_gray2,ocv_ref,CV_CMP_NE);
    //	cv::bitwise_not (in_gray1,ocv_ref);
    //	ocv_ref=cv::Mat::zeros(in_gray1.rows,in_gray1.cols,in_gray1.type());

    arithm_accel(imgInput1, imgInput2, imgOutput);
    out_img.data = imgOutput.copyFrom();

#endif

#if SCALAR
    unsigned char val[XF_CHANNELS(TYPE, NPC1)];

    for (int i = 0; i < XF_CHANNELS(TYPE, NPC1); i++) {
        val[i] = 150;
    }

    ///////////// OpenCV reference  /////////////

    cv::CV_FUNCT_NAME(in_gray1, val[0], ocv_ref
#ifdef CV_EXTRA_ARG
                      ,
                      CV_EXTRA_ARG
#endif
    );

    //	cv::add		(in_gray1,val,ocv_ref);
    //	cv::subtract(in_gray1,val,ocv_ref);	//subs
    //	cv::max		(in_gray1,val,ocv_ref);
    //	cv::min		(in_gray1,val,ocv_ref);
    //	cv::compare(in_gray1,val,ocv_ref,CV_CMP_LE);

    //	cv::subtract(val,in_gray1,ocv_ref);	//subRS
    //	ocv_ref.setTo(val);

    static xf::cv::Mat<TYPE, HEIGHT, WIDTH, NPC1> imgInput1(in_gray1.rows, in_gray1.cols);
    static xf::cv::Mat<TYPE, HEIGHT, WIDTH, NPC1> imgOutput(in_gray1.rows, in_gray1.cols);

    imgInput1.copyTo(in_gray1.data);

    arithm_accel(imgInput1, val, imgOutput);

    out_img.data = imgOutput.copyFrom();
#endif

    cv::imwrite("hls_out.jpg", out_img);
    cv::imwrite("ref_img.jpg", ocv_ref); // save the reference image

    // xf::cv::absDiff(ocv_ref, imgOutput, diff); // Compute absolute difference image
    cv::absdiff(ocv_ref, out_img, diff);
    cv::imwrite("diff_img.jpg", diff); // Save the difference image for debugging purpose

    FILE* fp2 = fopen("diff.txt", "w");
    // Find minimum and maximum differences.
    double minval = 256, maxval = 0;
    int cnt = 0;
    for (int i = 0; i < in_gray1.rows; i++) {
        for (int j = 0; j < in_gray1.cols; j++) {
            uchar v = diff.at<uchar>(i, j);
            fprintf(fp2, "%d\n", v);
            if (v > 2) cnt++;
            if (minval > v) minval = v;
            if (maxval < v) maxval = v;
        }
    }
    fclose(fp2);
    float err_per = 100.0 * (float)cnt / (in_gray1.rows * in_gray1.cols);
    fprintf(stderr,
            "Minimum error in intensity = %f\nMaximum error in intensity = %f\nPercentage of pixels above error "
            "threshold = %f\n",
            minval, maxval, err_per);

    in_img1.~Mat();
    in_img2.~Mat();
    in_gray1.~Mat();
    in_gray2.~Mat();
    out_img.~Mat();
    ocv_ref.~Mat();
    diff.~Mat();

    if (err_per > 0.0f) {
        return 1;
    }

    return 0;
}
