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
#include <iostream>
#include <vector>
#include "opencv2/opencv.hpp"
#include "common/xf_headers.hpp"
#include <stdlib.h>
#include <ap_int.h>
#include <math.h>
#include "common/xf_types.hpp"
#include "common/xf_infra.hpp"

#define MAXVALUE MAXREPRESENTEDVALUE

cv::Mat floyd_steinberg_dithering(cv::Mat image, int scale) {
    cv::Mat new_image, new_image2;
    new_image2.create(cv::Size(image.cols, image.rows), CV_GTM_TYPE);
#if GRAY
    new_image.create(cv::Size(image.cols, image.rows), CV_32FC1);
#else
    new_image.create(cv::Size(image.cols, image.rows), CV_32FC3);
#endif
    // new_image = image;
    for (int rowID = 0; rowID < image.rows; rowID++) {
        for (int colID = 0; colID < image.cols; colID++) {
            if (image.channels() == 1) {
#if T_16U
                new_image.at<float>(rowID, colID) = (float)image.at<unsigned short>(rowID, colID);
#else
                new_image.at<float>(rowID, colID) = (float)image.at<unsigned char>(rowID, colID);
#endif
            } else if (image.channels() == 3) {
                for (int c = 0; c < 3; c++)
#if T_16U
                    new_image.at<cv::Vec3f>(rowID, colID)[c] = (float)image.at<cv::Vec3w>(rowID, colID)[c];
#else
                    new_image.at<cv::Vec3f>(rowID, colID)[c] = (float)image.at<cv::Vec3b>(rowID, colID)[c];
#endif
            }
        }
    }

    int width = image.cols;
    int height = image.rows;
    float old_pix, new_pix;
    float max = MAXVALUE, err_pix;

    for (int rowID = 0; rowID < image.rows; rowID++) {
        for (int colID = 0; colID < image.cols; colID++) {
            if (image.channels() == 1) {
                old_pix = (float)new_image.at<float>(rowID, colID);
                new_pix = round(old_pix * scale / max) * round(max / scale);
                // float orn_pix = (float)image.at<float>(rowID, colID);
                err_pix = old_pix - new_pix;

                if (new_pix >= max) new_pix = (scale - 1) * round(max / scale);
                if (new_pix < 0) new_pix = 0;

                new_image.at<float>(rowID, colID) = round(new_pix / (MAXVALUE / SCALEFACTOR));

                if (colID + 1 < image.cols) {
                    new_image.at<float>(rowID, colID + 1) =
                        ((float)(new_image.at<float>(rowID, colID + 1) + (err_pix * 7 / 16)));
                }
                if ((colID - 1 >= 0) && (rowID + 1 < image.rows)) {
                    new_image.at<float>(rowID + 1, colID - 1) =
                        ((float)(new_image.at<float>(rowID + 1, colID - 1) + (err_pix * 3 / 16)));
                }
                if (rowID + 1 < image.rows) {
                    new_image.at<float>(rowID + 1, colID) =
                        ((float)(new_image.at<float>(rowID + 1, colID) + (err_pix * 5 / 16)));
                }
                if ((colID + 1 < image.cols) && (rowID + 1 < image.rows)) {
                    new_image.at<float>(rowID + 1, colID + 1) =
                        ((float)(new_image.at<float>(rowID + 1, colID + 1) + (err_pix * 1 / 16)));
                }

            } else if (image.channels() == 3) {
                for (int c = 0; c < 3; c++) {
                    old_pix = (float)new_image.at<cv::Vec3f>(rowID, colID)[c];
                    new_pix = round(old_pix * scale / max) * round(max / scale);
                    // float orn_pix = (float)image.at<float>(rowID, colID);
                    err_pix = old_pix - new_pix;

                    if (new_pix >= max) new_pix = (scale - 1) * round(max / scale);
                    if (new_pix < 0) new_pix = 0;

                    new_image.at<cv::Vec3f>(rowID, colID)[c] = round(new_pix / (MAXVALUE / SCALEFACTOR));

                    if (colID + 1 < image.cols) {
                        new_image.at<cv::Vec3f>(rowID, colID + 1)[c] =
                            ((float)(new_image.at<cv::Vec3f>(rowID, colID + 1)[c] + (err_pix * 7 / 16)));
                    }
                    if ((colID - 1 >= 0) && (rowID + 1 < image.rows)) {
                        new_image.at<cv::Vec3f>(rowID + 1, colID - 1)[c] =
                            ((float)(new_image.at<cv::Vec3f>(rowID + 1, colID - 1)[c] + (err_pix * 3 / 16)));
                    }
                    if (rowID + 1 < image.rows) {
                        new_image.at<cv::Vec3f>(rowID + 1, colID)[c] =
                            ((float)(new_image.at<cv::Vec3f>(rowID + 1, colID)[c] + (err_pix * 5 / 16)));
                    }
                    if ((colID + 1 < image.cols) && (rowID + 1 < image.rows)) {
                        new_image.at<cv::Vec3f>(rowID + 1, colID + 1)[c] =
                            ((float)(new_image.at<cv::Vec3f>(rowID + 1, colID + 1)[c] + (err_pix * 1 / 16)));
                    }
                }
            }
        }
    }
    for (int rowID = 0; rowID < image.rows; rowID++) {
        for (int colID = 0; colID < image.cols; colID++) {
            if (image.channels() == 1) {
                new_image2.at<unsigned char>(rowID, colID) = (unsigned char)new_image.at<float>(rowID, colID);
            } else if (image.channels() == 3) {
                for (int c = 0; c < 3; c++)
                    new_image2.at<cv::Vec3b>(rowID, colID)[c] = (unsigned char)new_image.at<cv::Vec3f>(rowID, colID)[c];
            }
        }
    }
    return new_image2;
}
