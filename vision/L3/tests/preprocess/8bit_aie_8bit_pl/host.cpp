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

#define PROFILE

#if !__X86_DEVICE__
#include <adf/adf_api/XRTConfig.h>
#endif
#include <chrono>
#include <common/xf_aie_sw_utils.hpp>
#include <common/xfcvDataMovers.h>
#include <fstream>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <experimental/xrt_kernel.h>
#include <experimental/xrt_graph.h>
#include <experimental/xrt_aie.h>

#include "config.h"

template <int FBITS>
uint32_t compute_scalefactor(int M, int N) {
    float x_scale = (float)M / (float)N;
    float scale = x_scale * (1 << FBITS);
    return (uint32_t)(std::roundf(scale));
}
template <int FBITS_ALPHA = 0, int FBITS_BETA = 4>
void get_alpha_beta(std::array<float, 4> mean,
                    std::array<float, 4> std_deviation,
                    unsigned char alpha[4],
                    char beta[4]) {
    for (int i = 0; i < 4; i++) {
        if (i < 3) {
            float a_v = mean[i] * (1 << FBITS_ALPHA);
            float b_v = (1 / std_deviation[i]) * (1 << FBITS_BETA);

            alpha[i] = (unsigned char)a_v;
            beta[i] = (char)b_v;
        } else {
            alpha[i] = 0;
            beta[i] = 0;
        }
    }
}

void cvtColor_OCVImpl(cv::Mat srcImageY, cv::Mat srcImageUV, cv::Mat dstRefImage) {
    cv::cvtColorTwoPlane(srcImageY, srcImageUV, dstRefImage, cv::COLOR_YUV2RGBA_NV12);
}

template <int FBITS_ALPHA = 0, int FBITS_BETA = 4, int FBITS_OUT = 0>
void normalization_CRefImpl(cv::Mat input, cv::Mat& output, unsigned char alpha[4], char beta[4]) {
    std::cout << (input.rows) << " " << (input.cols) << " " << (input.channels()) << std::endl;
    for (int i = 0; i < (input.rows); i++) {
        for (int j = 0; j < (input.cols); j++) {
            for (int k = 0; k < input.channels(); k++) {
                unsigned char x = input.at<cv::Vec4b>(i, j)[k];
                unsigned char alpha_Q8p0 = alpha[k];
                char beta_Q4p4 = beta[k];
                short tmp_out_Q12p4 = ((short)x - (short)alpha_Q8p0) * beta_Q4p4;
                char out_Q8p0 = (int)lrintf(((float)tmp_out_Q12p4) / (1 << (FBITS_ALPHA + FBITS_BETA - FBITS_OUT)));
                output.at<cv::Vec4b>(i, j)[k] = out_Q8p0;
            }
        }
    }
}

/*
 ******************************************************************************
 * Top level executable
 ******************************************************************************
 */

int main(int argc, char** argv) {
    try {
        if (argc < 4) {
            std::stringstream errorMessage;
            errorMessage << argv[0]
                         << " <xclbin> <inputImage1> <inputImage2> [width] [height] <crop_x> <crop_y> [iterations]";
            std::cerr << errorMessage.str();
            throw std::invalid_argument(errorMessage.str());
        }

        const char* xclBinName = argv[1];
        //////////////////////////////////////////
        // Read image from file and resize
        //////////////////////////////////////////
        cv::Mat srcImageY = cv::imread(argv[2], 0);
        int width_Y = srcImageY.cols;
        if (argc >= 5) width_Y = atoi(argv[4]);
        int height_Y = srcImageY.rows;
        if (argc >= 6) height_Y = atoi(argv[5]);
        if ((width_Y != srcImageY.cols) || (height_Y != srcImageY.rows))
            cv::resize(srcImageY, srcImageY, cv::Size(width_Y, height_Y));

        cv::Mat srcImageUV = cv::imread(argv[3], -1);
        if ((width_Y / 2 != srcImageUV.cols) || (height_Y / 2 != srcImageUV.rows))
            cv::resize(srcImageUV, srcImageUV, cv::Size(width_Y / 2, height_Y / 2));

        int crop_x = 0;
        if (argc >= 7) crop_x = atoi(argv[6]);

        int crop_y = 0;
        if (argc >= 8) crop_y = atoi(argv[7]);

        int iterations = 1;
        if (argc >= 9) iterations = atoi(argv[8]);

        cv::imwrite("srcImageY.png", srcImageY);
        cv::imwrite("srcImageUV.png", srcImageUV);

        std::cout << "Image size of UV : " << srcImageUV.cols << "x" << srcImageUV.rows
                  << " (element size: " << srcImageUV.elemSize() << ",total: " << srcImageUV.total()
                  << ", size: " << srcImageUV.size() << ")" << std::endl;

        int width = srcImageY.cols;
        int height = srcImageY.rows;

        int op_width = IMAGE_WIDTH_OUT;
        int op_height = IMAGE_HEIGHT_OUT;

        // alpha and beta values for Normalization
        std::array<float, 4> mean = {104, 107, 123, 0};
        std::array<float, 4> std_deviation = {2, 2, 2, 0};

        unsigned char alpha[4];
        char beta[4];

        get_alpha_beta<0, 4>(mean, std_deviation, alpha, beta);
        std::vector<int16_t> coeff({alpha[0], alpha[1], alpha[2], alpha[3], beta[0], beta[1], beta[2], beta[3]});

        uint32_t scale_x_fix = compute_scalefactor<16>(IMAGE_WIDTH_IN, IMAGE_WIDTH_OUT);
        uint32_t scale_y_fix = compute_scalefactor<16>(IMAGE_HEIGHT_IN, IMAGE_HEIGHT_OUT);

        std::cout << "Image size of Y : " << srcImageY.cols << "x" << srcImageY.rows
                  << " (element size: " << srcImageY.elemSize() << ",total: " << srcImageY.total()
                  << ", size: " << srcImageY.size() << ")" << std::endl;

        //////////////////////////////////////////
        // Run opencv reference test (yuv2rgba design)
        //////////////////////////////////////////
        std::cout << "Before Reference function" << std::endl;
        cv::Rect roiY;
        cv::Rect roiUV;
        roiY.x = crop_x;
        roiY.y = crop_y;
        roiY.width = CROP_WT;
        roiY.height = CROP_HT;
        roiUV.x = crop_x / 2;
        roiUV.y = crop_y / 2;
        roiUV.width = CROP_WT / 2;
        roiUV.height = CROP_HT / 2;

        assert(((roiY.height <= srcImageY.rows) && (roiY.width <= srcImageY.cols)) &&
               "Y ROI dimensions should be smaller or equal to the input image");
        assert(((roiY.height > 0) && (roiY.width > 0)) && "Y ROI dimensions should be greater than 0");
        assert(((roiY.height + roiY.y <= srcImageY.rows) && (roiY.width + roiY.x <= srcImageY.cols)) &&
               "Y ROI area exceeds the input image area");

        assert(((roiUV.height <= srcImageUV.rows) && (roiUV.width <= srcImageUV.cols)) &&
               "UV ROI dimensions should be smaller or equal to the input image");
        assert(((roiUV.height > 0) && (roiUV.width > 0)) && "UV ROI dimensions should be greater than 0");
        assert(((roiUV.height + roiUV.y <= srcImageUV.rows) && (roiUV.width + roiUV.x <= srcImageUV.cols)) &&
               "UV ROI area exceeds the input image area");

        cv::Mat cropImageY = srcImageY(roiY);
        cv::Mat cropImageUV = srcImageUV(roiUV);
        cv::imwrite("cropImageY.png", cropImageY);
        cv::imwrite("cropImageUV.png", cropImageUV);
        cv::Mat yuv2rgbaImage(CROP_HT, CROP_WT, CV_8UC4);
        cv::Mat yuv2rgbaImageResized;
        cv::Mat dstRefImage(op_height, op_width, CV_8SC4);

        cvtColor_OCVImpl(cropImageY, cropImageUV, yuv2rgbaImage);
        cv::resize(yuv2rgbaImage, yuv2rgbaImageResized, cv::Size(op_height, op_width));
        normalization_CRefImpl<0, 4, 0>(yuv2rgbaImageResized, dstRefImage, alpha, beta);

        // Initializa device
        xF::deviceInit(xclBinName);

        // Load image
        void* srcData1 = nullptr;
        xrt::bo src_hndl1 = xrt::bo(xF::gpDhdl, (srcImageY.total() * srcImageY.elemSize()), 0, 0);
        srcData1 = src_hndl1.map();
        memcpy(srcData1, srcImageY.data, (srcImageY.total() * srcImageY.elemSize()));

        void* srcData2 = nullptr;
        xrt::bo src_hndl2 = xrt::bo(xF::gpDhdl, (srcImageUV.total() * srcImageUV.elemSize()), 0, 0);
        srcData2 = src_hndl2.map();
        memcpy(srcData2, srcImageUV.data, (srcImageUV.total() * srcImageUV.elemSize()));

        // Allocate output buffer
        void* dstData = nullptr;
        xrt::bo ptr_dstHndl = xrt::bo(xF::gpDhdl, (op_height * op_width * 4), 0, 0);
        dstData = ptr_dstHndl.map();
        cv::Mat dst(op_height, op_width, CV_8SC4, dstData);

        xF::xfcvDataMoverParams params1(cropImageY.size(), cv::Size(op_height, op_width));
        xF::xfcvDataMoverParams params2(cropImageUV.size(), cv::Size(op_height, op_width));

        xF::xfcvDataMovers<xF::TILER, uint8_t, TILE_HEIGHT, TILE_WIDTH, VECTORIZATION_FACTOR> tiler1(0, 0);
        xF::xfcvDataMovers<xF::TILER, uint8_t, TILE_HEIGHT, TILE_WIDTH / 2, VECTORIZATION_FACTOR> tiler2(0, 0);
        xF::xfcvDataMovers<xF::STITCHER, int8_t, TILE_HEIGHT_OUT, TILE_WIDTH_OUT, VECTORIZATION_FACTOR> stitcher1;

#if !__X86_DEVICE__
        std::cout << "Graph init. This does nothing because CDO in boot PDI "
                     "already configures AIE.\n";
        auto gHndl = xrt::graph(xF::gpDhdl, xF::xclbin_uuid, "mygraph");
        std::cout << "XRT graph opened" << std::endl;
        gHndl.reset();
#endif

        START_TIMER
        tiler1.compute_metadata(cropImageY.size(), cv::Size(op_height, op_width), srcImageY.size(), false, crop_x,
                                crop_y);
        tiler2.compute_metadata(cropImageUV.size(), cv::Size(op_height, op_width), srcImageUV.size(), true, crop_x / 2,
                                crop_y / 2);
        STOP_TIMER("Meta data compute time")

        std::chrono::microseconds tt(0);
        for (int i = 0; i < iterations; i++) {
            std::cout << "Iteration : " << (i + 1) << std::endl;
            //@{
            START_TIMER
            auto tiles_sz1 = tiler1.host2aie_nb(&src_hndl1, srcImageY.size(), params1);
            tiler2.host2aie_nb(&src_hndl2, srcImageUV.size(), params2);
            stitcher1.aie2host_nb(&ptr_dstHndl, dst.size(), tiles_sz1);

#if !__X86_DEVICE__
            gHndl.run(tiles_sz1[0] * tiles_sz1[1]);
            gHndl.wait();
            std::cout << "Graph run complete\n";
#endif

            tiler1.wait();
            tiler2.wait();
            stitcher1.wait();
            STOP_TIMER("Total time to process frame")
            tt += tdiff;
        }

        ptr_dstHndl.sync(XCL_BO_SYNC_BO_FROM_DEVICE, op_height * op_width * 4, 0);
        //@}

        // Analyze output {
        std::cout << "Analyzing diff\n";
        cv::Mat diff(op_height, op_width, CV_8UC4);

        signed char ref2[dst.rows * dst.cols * dst.channels()];
        signed char aie2[dst.rows * dst.cols * dst.channels()];

        std::ofstream diff_data("diff_data.txt");

        for (int ii = 0; ii < dst.rows; ii++) {
            for (int jj = 0; jj < dst.cols; jj++) {
                for (int kk = 0; kk < dst.channels(); kk++) {
                    ref2[ii + jj + kk] = dstRefImage.data[(ii * dstRefImage.cols * dstRefImage.channels()) +
                                                          (jj * dstRefImage.channels()) + kk];
                    aie2[ii + jj + kk] = dst.data[(ii * dst.cols * dst.channels()) + (jj * dst.channels()) + kk];
                    diff.at<cv::Vec4b>(ii, jj)[kk] = abs(ref2[ii + jj + kk] - aie2[ii + jj + kk]);
                    diff_data << "row = " << ii << " col = " << jj << " channel = " << kk
                              << " ref = " << (int)ref2[ii + jj + kk] << " aie = " << (int)aie2[ii + jj + kk]
                              << " diff = " << (int)(abs(ref2[ii + jj + kk] - aie2[ii + jj + kk])) << std::endl;
                }
            }
        }

        cv::imwrite("ref.png", dstRefImage);
        cv::imwrite("aie.png", dst);
        cv::imwrite("diff.png", diff);

        float err_per2 = 0.0f;
        analyzeDiff(diff, 2, err_per2);
        if (err_per2 > 0.0f) {
            std::cerr << "Test failed" << std::endl;
            exit(-1);
        }

        std::cout << "Test passed" << std::endl;
        std::cout << "Average time to process frame : " << (((float)tt.count() * 0.001) / (float)iterations) << " ms"
                  << std::endl;
        std::cout << "Average frames per second : " << (((float)1000000 / (float)tt.count()) * (float)iterations)
                  << " fps" << std::endl;

#if !__X86_DEVICE__
        gHndl.end(0);
#endif

        return 0;
    } catch (std::exception& e) {
        const char* errorMessage = e.what();
        std::cerr << "Exception caught: " << errorMessage << std::endl;
        exit(-1);
    }
}
