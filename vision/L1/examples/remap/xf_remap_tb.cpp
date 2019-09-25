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
#include "xf_remap_config.h"

using namespace std;

// declarations for the map generation
#define WIDTH_BY_2 (WIDTH >> 1)
#define WIDTH_BY_4 (WIDTH >> 2)
#define WIDTH_BY_8 (WIDTH >> 3)
#define WIDTH_BY_16 (WIDTH >> 4)

/* Numwords used to copy the data */
#define NUMWORDS (WIDTH_BY_8 << 3)
#define NUMWORDS1 (WIDTH_BY_4 << 3)
#define NUMWORDS2 (WIDTH_BY_2 << 3)
#define NUMWORDS3 (WIDTH_BY_16 << 3)

// For reading mapx and mapy from file, enable this flag and input the maps in the format,
//	p00 p01 p02 p03 ....
//      p10
//      p20
//      p30
//      .
//      .
//  Note that the number of rows and cols in the Maps must be same as the input image, and must be in float

#define READ_MAPS_FROM_FILE 0

int main(int argc, char** argv) {
#if READ_MAPS_FROM_FILE
    if (argc != 4) {
        std::cout << "Usage: <executable> <input image path> <mapx file> <mapy file>" << std::endl;
        return -1;
    }
#else
    if (argc != 2) {
        std::cout << "Usage: <executable> <input image path>" << std::endl;
        return -1;
    }
#endif

    std::cout << "INFO: Allocate memory for input and output data." << std::endl;
    cv::Mat src, ocv_remapped, hls_remapped;
    cv::Mat map_x, map_y, diff;
#if GRAY
    src = cv::imread(argv[1], 0); // read image Grayscale
#else                             // RGB
    src = cv::imread(argv[1], 1); // read image RGB

#endif
    ocv_remapped.create(src.rows, src.cols, src.type()); // opencv result
    map_x.create(src.rows, src.cols, CV_32FC1);          // Mapx for opencv remap function
    map_y.create(src.rows, src.cols, CV_32FC1);          // Mapy for opencv remap function
    hls_remapped.create(src.rows, src.cols, src.type()); // create memory for output images
    diff.create(src.rows, src.cols, src.type());
    if (!src.data) {
        std::cout << "Failed to load the input image ... !!!" << std::endl;
        return -1;
    }

    std::cout << "INFO: Initialize maps." << std::endl;
#if READ_MAPS_FROM_FILE
    // read the float map data from the file (code could be alternated for reading from image)
    FILE *fp_mx, *fp_my;
    fp_mx = fopen(argv[2], "r");
    fp_my = fopen(argv[3], "r");
    for (int i = 0; i < src.rows; i++) {
        for (int j = 0; j < src.cols; j++) {
            float valx, valy;
            if (fscanf(fp_mx, "%f", &valx) != 1) {
                printf("Not enough data in the provided map_x file ... !!!\n");
            }
            if (fscanf(fp_my, "%f", &valy) != 1) {
                printf("Not enough data in the provided map_y file ... !!!\n");
            }
            map_x.at<float>(i, j) = valx;
            map_y.at<float>(i, j) = valy;
        }
    }
#else // example map generation, flips the image horizontally
    for (int i = 0; i < src.rows; i++) {
        for (int j = 0; j < src.cols; j++) {
            float valx = (float)(src.cols - j - 1), valy = (float)i;
            map_x.at<float>(i, j) = valx;
            map_y.at<float>(i, j) = valy;
        }
    }
#endif

    /* Opencv remap function */
    std::cout << "INFO: Starting Opencv Reference..." << std::endl;
    cv::remap(src, ocv_remapped, map_x, map_y, CV_INTER_LINEAR, cv::BORDER_CONSTANT, cv::Scalar(0, 0, 0));
    std::cout << "INFO: End of Opencv Reference..." << std::endl;

    //////////////////	HLS Function Call  ////////////////////////
    static xf::cv::Mat<TYPE, XF_HEIGHT, XF_WIDTH, XF_NPPC1> inMat(src.rows, src.cols);
    static xf::cv::Mat<XF_32FC1, XF_HEIGHT, XF_WIDTH, XF_NPPC1> mapxMat(src.rows, src.cols);
    static xf::cv::Mat<XF_32FC1, XF_HEIGHT, XF_WIDTH, XF_NPPC1> mapyMat(src.rows, src.cols);
    static xf::cv::Mat<TYPE, XF_HEIGHT, XF_WIDTH, XF_NPPC1> remappedMat(src.rows, src.cols);

    mapxMat.copyTo(map_x.data);
    mapyMat.copyTo(map_y.data);

    inMat.copyTo(src.data);

    std::cout << "INFO: Starting xfcv Kernel..." << std::endl;
    remap_accel(inMat, remappedMat, mapxMat, mapyMat);
    std::cout << "INFO: End of kernel execution..." << std::endl;

    /// Save the results
    imwrite("ocv_reference_out.png", ocv_remapped); // Opencv Result
    xf::cv::imwrite("kernel_out.jpg", remappedMat);

    xf::cv::absDiff(ocv_remapped, remappedMat, diff);
    imwrite("diff.png", diff);

    float err_per;
    xf::cv::analyzeDiff(diff, 0, err_per);

    if (err_per > 0.0f) {
        std::cout << "ERROR: Test Failed." << std::endl;
        return EXIT_FAILURE;
    }

    ocv_remapped.~Mat();
    map_x.~Mat();
    map_y.~Mat();

    hls_remapped.~Mat();
    diff.~Mat();
    src.~Mat();

    return 0;
}
