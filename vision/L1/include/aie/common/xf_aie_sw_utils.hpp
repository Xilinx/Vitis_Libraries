/*
 * Copyright 2021 Xilinx, Inc.
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

#ifndef _XF_AIE_SW_UTILS_H_
#define _XF_AIE_SW_UTILS_H_

#include <common/xf_aie_const.hpp>
#include <iostream>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#ifdef PROFILE
std::chrono::time_point<std::chrono::high_resolution_clock> start;
std::chrono::time_point<std::chrono::high_resolution_clock> stop;
std::chrono::microseconds tdiff;
#define START_TIMER start = std::chrono::high_resolution_clock::now();
#define STOP_TIMER(name)                                                                                       \
    stop = std::chrono::high_resolution_clock::now();                                                          \
    tdiff = std::chrono::duration_cast<std::chrono::microseconds>(stop - start);                               \
    std::cout << "RUNTIME of " << name << ": " << ((float)tdiff.count() / (float)1000) << " ms " << std::endl; \
    std::cout << "FPS of " << name << ": " << (1000000 / tdiff.count()) << " fps " << std::endl;
#else
#define START_TIMER
#define STOP_TIMER(name)
#endif

extern "C" {
void printConvolutionWindow(cv::Mat& img, int row, int col, int kernel_size_rows, int kernel_size_cols) {
    for (int m = -(kernel_size_rows >> 1); m <= (kernel_size_rows >> 1); m++) {
        for (int n = -(kernel_size_cols >> 1); n <= (kernel_size_cols >> 1); n++) {
            int y = std::min(std::max((row + m), 0), (img.rows - 1));
            int x = std::min(std::max((col + n), 0), (img.cols - 1));
            ;
            int val = (int)img.at<unsigned char>(y, x);
            std::cout << val << " ";
        }
        std::cout << std::endl;
    }
}

void analyzeDiff(::cv::Mat& diff_img, int err_thresh, float& err_per) {
    int cv_bitdepth;
    if (diff_img.depth() == CV_8U) {
        cv_bitdepth = 8;
    } else if (diff_img.depth() == CV_16U || diff_img.depth() == CV_16S) {
        cv_bitdepth = 16;
    } else if (diff_img.depth() == CV_32S || diff_img.depth() == CV_32F) {
        cv_bitdepth = 32;
    } else {
        fprintf(stderr, "OpenCV image's depth not supported for this function\n ");
        return;
    }

    int cnt = 0;
    double minval = std::pow(2.0, cv_bitdepth), maxval = 0;
    int max_fix = (int)(std::pow(2.0, cv_bitdepth) - 1.0);
    for (int i = 0; i < diff_img.rows; i++) {
        for (int j = 0; j < diff_img.cols; j++) {
            int v = 0;
            for (int k = 0; k < diff_img.channels(); k++) {
                int v_tmp = 0;
                if (diff_img.channels() == 1) {
                    if (cv_bitdepth == 8)
                        v_tmp = (int)diff_img.at<unsigned char>(i, j);
                    else if (cv_bitdepth == 16 && diff_img.depth() == CV_16U) // 16 bitdepth
                        v_tmp = (int)diff_img.at<unsigned short>(i, j);
                    else if (cv_bitdepth == 16 && diff_img.depth() == CV_16S) // 16 bitdepth
                        v_tmp = (int)diff_img.at<short>(i, j);
                    else if (cv_bitdepth == 32 && diff_img.depth() == CV_32S)
                        v_tmp = (int)diff_img.at<int>(i, j);
                } else // 3 channels
                    v_tmp = (int)diff_img.at< ::cv::Vec3b>(i, j)[k];

                if (v_tmp > v) v = v_tmp;
            }
            if (v > err_thresh) {
                cnt++;
                if (diff_img.depth() == CV_8U)
                    diff_img.at<unsigned char>(i, j) = max_fix;
                else if (diff_img.depth() == CV_16U)
                    diff_img.at<unsigned short>(i, j) = max_fix;
                else if (diff_img.depth() == CV_16S)
                    diff_img.at<short>(i, j) = max_fix;
                else if (diff_img.depth() == CV_32S)
                    diff_img.at<int>(i, j) = max_fix;
                else
                    diff_img.at<float>(i, j) = (float)max_fix;
            }
            if (minval > v) minval = v;
            if (maxval < v) maxval = v;
        }
    }
    err_per = 100.0 * (float)cnt / (diff_img.rows * diff_img.cols);
    std::cout << "\tMinimum error in intensity = " << minval << std::endl;
    std::cout << "\tMaximum error in intensity = " << maxval << std::endl;
    std::cout << "\tNumber of pixels above error threshold = " << cnt << std::endl;
    std::cout << "\tPercentage of pixels above error threshold = " << err_per << std::endl;
}

// This function computes the peak signal-to-noise ratio between the given images I1, I2.
// It is expressed as a logarithmic quantity using the decibel scale.
// PSNR is defined via the mean squared error (MSE): PSNR = 10 * log10(MAX^2/MSE)
// For 8-bit data typical values for the PSNR are between 30 and 50 dB.
// For 16-bit data typical values for the PSNR are between 60 and 80 dB.
double getPSNR(const cv::Mat& I1, const cv::Mat& I2) {
    int cv_bitdepth;
    if (I1.depth() == CV_8U) {
        cv_bitdepth = 8;
    } else if (I1.depth() == CV_16U || I1.depth() == CV_16S) {
        cv_bitdepth = 16;
    } else if (I1.depth() == CV_32S || I1.depth() == CV_32F) {
        cv_bitdepth = 32;
    } else {
        fprintf(stderr, "OpenCV image's depth not supported for this function\n ");
        return 0;
    }

    cv::Mat s1;
    cv::absdiff(I1, I2, s1); // |I1 - I2|
    s1.convertTo(s1, CV_32F);
    s1 = s1.mul(s1); // |I1 - I2|^2

    cv::Scalar s = cv::sum(s1); // sum elements per channel
    double sse = 0.0;
    if (I1.channels() == 1) {
        sse = s.val[0];
    } else {
        sse = s.val[0] + s.val[1] + s.val[2]; // sum channels
    }

    if (sse <= 1e-10) {
        // for small values return zero
        return 0;
    } else {
        double mse = sse / (double)(I1.channels() * I1.total());

        double psnr = 0;
        double maxval = std::pow(2, cv_bitdepth) - 1.0;
        psnr = 10.0 * std::log10((maxval * maxval) / mse);
        return psnr;
    }
}

// This function will return a similarity index for each channel of the image
// and value is between zero and one, where one corresponds to perfect fit.
cv::Scalar getMSSIM(const cv::Mat& I1, const cv::Mat& I2) {
    int cv_bitdepth;
    if (I1.depth() == CV_8U) {
        cv_bitdepth = 8;
    } else if (I1.depth() == CV_16U || I1.depth() == CV_16S) {
        cv_bitdepth = 16;
    } else if (I1.depth() == CV_32S || I1.depth() == CV_32F) {
        cv_bitdepth = 32;
    } else {
        fprintf(stderr, "OpenCV image's depth not supported for this function\n ");
        return 0;
    }

    double C1 = 0.01 * (std::pow(2, cv_bitdepth) - 1.0);
    C1 = C1 * C1;

    double C2 = 0.03 * (std::pow(2, cv_bitdepth) - 1.0);
    C2 = C2 * C2;

    int d = CV_32F;
    cv::Mat i1, i2;
    I1.convertTo(i1, d); // cannot calculate on one byte large values
    I2.convertTo(i2, d);
    cv::Mat i2_2 = i2.mul(i2);  // i2^2
    cv::Mat i1_2 = i1.mul(i1);  // i1^2
    cv::Mat i1_i2 = i1.mul(i2); // i1 * i2

    cv::Mat mu1, mu2; // PRELIMINARY COMPUTING
    cv::GaussianBlur(i1, mu1, cv::Size(11, 11), 1.5);
    cv::GaussianBlur(i2, mu2, cv::Size(11, 11), 1.5);
    cv::Mat mu1_2 = mu1.mul(mu1);
    cv::Mat mu2_2 = mu2.mul(mu2);
    cv::Mat mu1_mu2 = mu1.mul(mu2);
    cv::Mat sigma1_2, sigma2_2, sigma12;
    cv::GaussianBlur(i1_2, sigma1_2, cv::Size(11, 11), 1.5);
    sigma1_2 -= mu1_2;
    cv::GaussianBlur(i2_2, sigma2_2, cv::Size(11, 11), 1.5);
    sigma2_2 -= mu2_2;
    cv::GaussianBlur(i1_i2, sigma12, cv::Size(11, 11), 1.5);
    sigma12 -= mu1_mu2;
    cv::Mat t1, t2, t3;
    t1 = 2 * mu1_mu2 + C1;
    t2 = 2 * sigma12 + C2;
    t3 = t1.mul(t2); // t3 = ((2*mu1_mu2 + C1).*(2*sigma12 + C2))
    t1 = mu1_2 + mu2_2 + C1;
    t2 = sigma1_2 + sigma2_2 + C2;
    t1 = t1.mul(t2); // t1 =((mu1_2 + mu2_2 + C1).*(sigma1_2 + sigma2_2 + C2))
    cv::Mat ssim_map;
    cv::divide(t3, t1, ssim_map);          // ssim_map =  t3./t1;
    cv::Scalar mssim = cv::mean(ssim_map); // mssim = average of ssim map
    return mssim;
}
}

#endif
