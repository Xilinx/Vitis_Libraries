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

#include <fstream>
#include <adf/adf_api/XRTConfig.h>
#include <chrono>
#include <common/xf_aie_sw_utils.hpp>
#include <common/xfcvDataMovers.h>
#include <sstream>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <xaiengine.h>
#include <xrt/experimental/xrt_kernel.h>
#include <cmath>
#include <string.h>
#include <vector>

#include "graph.cpp"
#include "config.h"

template <int FBITS_ALPHA = 0, int FBITS_BETA = 4>
void get_alpha_beta(float mean[3], float std[3], int alpha[3], int beta[3]) {
    for (int i = 0; i < 3; i++) {
        if (i < 3) {
            float a_v = mean[i] * (1 << FBITS_ALPHA);
            float b_v = (1 / std[i]) * (1 << FBITS_BETA);

            alpha[i] = (unsigned char)a_v;
            beta[i] = (char)b_v;

            assert((a_v < (1 << 8)) && "alpha values exceeds 8 bit precison");
            assert((b_v < (1 << 8)) && "beta values exceeds 8 bit precison");
        } else {
            alpha[i] = 0;
            beta[i] = 0;
        }
    }
}

template <int FBITS_ALPHA, int FBITS_BETA, int FBITS_OUT>
void normalization_CRefImpl(cv::Mat input, cv::Mat& output, int alpha[3], int beta[3]) {
    std::cout << (input.rows) << " " << (input.cols) << " " << (input.channels()) << std::endl;
    for (int i = 0; i < (input.rows); i++) {
        for (int j = 0; j < (input.cols); j++) {
            for (int k = 0; k < input.channels(); k++) {
                // unsigned char x = input.at<cv::Vec4b>(i, j)[k];
                unsigned char x = input.at<cv::Vec3b>(i, j)[k];
                unsigned char alpha_Q8p0 = alpha[k];
                char beta_Q4p4 = beta[k];
                short tmp_out_Q12p4 = ((short)x - (short)alpha_Q8p0) * beta_Q4p4;
                char out_Q8p0 = (int)lrintf(((float)tmp_out_Q12p4) / (1 << (FBITS_ALPHA + FBITS_BETA - FBITS_OUT)));
                output.at<cv::Vec3b>(i, j)[k] = out_Q8p0;
                // output.at<cv::Vec4b>(i, j)[k] = out_Q8p0;
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
        if (argc < 3) {
            std::stringstream errorMessage;
            errorMessage << argv[0] << " <xclbin> <inputImage> [width] [height] [iterations]";
            std::cerr << errorMessage.str();
            throw std::invalid_argument(errorMessage.str());
        }
        const char* xclBinName = argv[1];
        //////////////////////////////////////////
        // Read image from file and resize
        //////////////////////////////////////////
        cv::Mat srcImageR;
        srcImageR = cv::imread(argv[2], 1);

        int width = srcImageR.cols;
        if (argc >= 4) width = atoi(argv[3]);
        int height = srcImageR.rows;
        if (argc >= 5) height = atoi(argv[4]);

        int iterations = 1;
        if (argc >= 6) iterations = atoi(argv[5]);

        if ((width != srcImageR.cols) || (height != srcImageR.rows))
            cv::resize(srcImageR, srcImageR, cv::Size(width, height));

        std::cout << "Image size" << std::endl;
        std::cout << srcImageR.rows << std::endl;
        std::cout << srcImageR.cols << std::endl;
        std::cout << srcImageR.elemSize() << std::endl;
        std::cout << srcImageR.type() << std::endl;
        std::cout << "Image size (end)" << std::endl;
        int op_width = IMAGE_WIDTH_OUT;
        int op_height = IMAGE_HEIGHT_OUT;

        // alpha and beta values for Normalization
        const int FBITS_ALPHA = 0;
        const int FBITS_BETA = 4;
        const int FBITS_OUT = 0;

        int alpha[3];
        int beta[3];
        float mean[3] = {104, 107, 123};
        float std_deviation[3] = {2, 2, 2};
        get_alpha_beta<FBITS_ALPHA, FBITS_BETA>(mean, std_deviation, alpha, beta);
        //////////////////////////////////////////
        // Run opencv reference test (filter2D design)
        //////////////////////////////////////////
        cv::Mat srcImageRresize, srcImageRalpha;
        cv::resize(srcImageR, srcImageRresize, cv::Size(224, 224));
        cv::Mat dstRefImage(op_height, op_width, CV_8SC3);
        normalization_CRefImpl<FBITS_ALPHA, FBITS_BETA, FBITS_OUT>(srcImageRresize, dstRefImage, alpha, beta);

        // Initializa device
        xF::deviceInit(xclBinName);

        // Load image
        void* srcData = nullptr;
        xrtBufferHandle src_hndl = xrtBOAlloc(xF::gpDhdl, (srcImageR.total() * srcImageR.elemSize()), 0, 0);
        srcData = xrtBOMap(src_hndl);
        memcpy(srcData, srcImageR.data, (srcImageR.total() * srcImageR.elemSize()));

        // Allocate output buffer
        void* dstData = nullptr;
        xrtBufferHandle dst_hndl = xrtBOAlloc(xF::gpDhdl, (op_height * op_width * srcImageR.elemSize()), 0, 0);
        dstData = xrtBOMap(dst_hndl);
        cv::Mat dst(op_height, op_width, dstRefImage.type(), dstData);

        xF::xfcvDataMoverParams params(srcImageR.size(), cv::Size(op_height, op_width));
        xF::xfcvDataMovers<xF::TILER, uint8_t, TILE_HEIGHT_IN, TILE_WIDTH_IN, 16> tiler(0, 0, true);
        xF::xfcvDataMovers<xF::STITCHER, int8_t, TILE_HEIGHT_OUT, TILE_WIDTH_OUT, 16> stitcher(true);

        std::cout << "Graph init. This does nothing because CDO in boot PDI "
                     "already configures AIE.\n";

        resize_norm.init();
        resize_norm.update(resize_norm.a0, alpha[0]);
        resize_norm.update(resize_norm.a1, alpha[1]);
        resize_norm.update(resize_norm.a2, alpha[2]);
        resize_norm.update(resize_norm.a3, 0);
        resize_norm.update(resize_norm.b0, beta[0]);
        resize_norm.update(resize_norm.b1, beta[1]);
        resize_norm.update(resize_norm.b2, beta[2]);
        resize_norm.update(resize_norm.b3, 0);

        START_TIMER
        tiler.compute_metadata(srcImageR.size(), cv::Size(op_height, op_width));
        STOP_TIMER("Meta data compute time")

        std::chrono::microseconds tt(0);
        for (int i = 0; i < iterations; i++) {
            //@{
            std::cout << "Iteration : " << (i + 1) << std::endl;
            START_TIMER
            auto tiles_sz = tiler.host2aie_nb(src_hndl, srcImageR.size(), params);
            stitcher.aie2host_nb(dst_hndl, dst.size(), tiles_sz);

            std::cout << "Graph run(" << tiles_sz[0] * tiles_sz[1] << ")\n";
            resize_norm.run(tiles_sz[0] * tiles_sz[1]);

            resize_norm.wait();
            tiler.wait();
            stitcher.wait();

            STOP_TIMER("resize function")
            std::cout << "Data transfer complete (Stitcher)\n";
            tt += tdiff;
            //@}

            // Analyze output {
            std::cout << "Analyzing diff\n";
            cv::Mat diff(op_height, op_width, CV_8UC3);
            signed char ref[dst.rows * dst.cols * dst.channels()];
            signed char aie[dst.rows * dst.cols * dst.channels()];

            for (int ii = 0; ii < dst.rows; ii++) {
                for (int jj = 0; jj < dst.cols; jj++) {
                    for (int kk = 0; kk < dst.channels(); kk++) {
                        ref[ii + jj + kk] = dstRefImage.data[(ii * dstRefImage.cols * dstRefImage.channels()) +
                                                             (jj * dstRefImage.channels()) + kk];
                        aie[ii + jj + kk] = dst.data[(ii * dst.cols * dst.channels()) + (jj * dst.channels()) + kk];
                        diff.at<cv::Vec3b>(ii, jj)[kk] = abs(ref[ii + jj + kk] - aie[ii + jj + kk]);
                    }
                }
            }

            cv::imwrite("ref.png", dstRefImage);
            cv::imwrite("aie.png", dst);
            cv::imwrite("diff.png", diff);

            float err_per;
            analyzeDiff(diff, 4, err_per);
            if (err_per > 0) {
                std::cerr << "Test failed" << std::endl;
                exit(-1);
            }
        }
        //}
        std::cout << "Test passed" << std::endl;
        std::cout << "Average time to process frame : " << (((float)tt.count() * 0.001) / (float)iterations) << " ms"
                  << std::endl;
        std::cout << "Average frames per second : " << (((float)1000000 / (float)tt.count()) * (float)iterations)
                  << " fps" << std::endl;
        return 0;
    } catch (std::exception& e) {
        const char* errorMessage = e.what();
        std::cerr << "Exception caught: " << errorMessage << std::endl;
        exit(-1);
    }
}
