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

#include <chrono>
#include <cmath>
#include <common/xf_aie_sw_utils.hpp>
#include <common/xfcvDataMovers.h>
#include <fstream>
#include <sstream>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <xrt/experimental/xrt_aie.h>
#include <xrt/experimental/xrt_graph.h>
#include <xrt/experimental/xrt_kernel.h>

#include "config.h"

template <int FBITS_ALPHA = 0, int FBITS_BETA = 4, int FBITS_OUT = 0>
void normalization_CRefImpl(cv::Mat input, cv::Mat& output, unsigned char alpha[4], char beta[4]) {
    std::cout << (input.rows) << " " << (input.cols) << " " << (input.channels()) << std::endl;
    for (int i = 0; i < (input.rows); i++) {
        for (int j = 0; j < (input.cols); j++) {
            for (int k = 0; k < input.channels(); k++) {
                unsigned char x = input.at<cv::Vec<signed char, 4> >(i, j)[k];
                unsigned char alpha_Q8p0 = alpha[k];
                char beta_Q4p4 = beta[k];
                short tmp_out_Q12p4 = ((short)x - (short)alpha_Q8p0) * beta_Q4p4;
                char out_Q8p0 = (int)lrintf(((float)tmp_out_Q12p4) / (1 << (FBITS_ALPHA + FBITS_BETA - FBITS_OUT)));
                output.at<cv::Vec<signed char, 4> >(i, j)[k] = out_Q8p0;
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
        cv::cvtColor(srcImageR, srcImageR, cv::COLOR_BGR2BGRA);

        int width = srcImageR.cols;
        if (argc >= 4) width = atoi(argv[3]);
        int height = srcImageR.rows;
        if (argc >= 5) height = atoi(argv[4]);

        int iterations = 1;
        if (argc >= 6) iterations = atoi(argv[5]);

        int op_width = width;
        int op_height = height;

        if ((op_width != srcImageR.cols) || (op_height != srcImageR.rows))
            cv::resize(srcImageR, srcImageR, cv::Size(op_width, op_height));

        std::cout << "Image size" << std::endl;
        std::cout << srcImageR.rows << std::endl;
        std::cout << srcImageR.cols << std::endl;
        std::cout << srcImageR.elemSize() << std::endl;
        std::cout << srcImageR.type() << std::endl;
        std::cout << "Image size (end)" << std::endl;

        // alpha and beta values for Normalization
        std::array<float, 4> mean = {104, 107, 123, 0};
        std::array<float, 4> std_deviation = {2, 2, 2, 0};

        unsigned char alpha[4];
        char beta[4];

        get_alpha_beta<0, 4>(mean, std_deviation, alpha, beta);
        std::array<int16_t, 8> coeff({alpha[0], alpha[1], alpha[2], alpha[3], beta[0], beta[1], beta[2], beta[3]});

        //////////////////////////////////////////
        // Run opencv reference test (filter2D design)
        //////////////////////////////////////////
        cv::Mat dstRefImage(op_height, op_width, CV_8SC4);
        normalization_CRefImpl<0, 4, 0>(srcImageR, dstRefImage, alpha, beta);

        // Initializa device
        xF::deviceInit(xclBinName);

        // Load image
        std::vector<uint8_t> srcData;
        srcData.assign(srcImageR.data, (srcImageR.data + (srcImageR.total() * srcImageR.channels())));
        cv::Mat src(srcImageR.rows, srcImageR.cols, CV_8UC4, (void*)srcData.data());

        std::vector<int8_t> dstData;
        dstData.assign(op_height * op_width * srcImageR.elemSize(), 0);
        cv::Mat dst(op_height, op_width, CV_8SC4, (void*)dstData.data());

        xF::xfcvDataMovers<xF::TILER, uint8_t, TILE_HEIGHT_IN, TILE_WIDTH_IN, 16, 1, 0, true> tiler(0, 0, 4);
        xF::xfcvDataMovers<xF::STITCHER, int8_t, TILE_HEIGHT_OUT, TILE_WIDTH_OUT, 16, 1, 0, true> stitcher(4);

        std::cout << "Graph init. This does nothing because CDO in boot PDI "
                     "already configures AIE.\n";

#if !__X86_DEVICE__
        auto gHndl = xrt::graph(xF::gpDhdl, xF::xclbin_uuid, "norm");
        std::cout << "XRT graph opened" << std::endl;
        gHndl.reset();
        std::cout << "Graph reset done" << std::endl;
        gHndl.update("norm.k.in[1]", coeff);
#endif

        START_TIMER
        tiler.compute_metadata(srcImageR.size());
        STOP_TIMER("Meta data compute time")

        std::chrono::microseconds tt(0);
        for (int i = 0; i < iterations; i++) {
            //@{
            std::cout << "Iteration : " << (i + 1) << std::endl;
            START_TIMER
            auto tiles_sz = tiler.host2aie_nb(srcData.data(), srcImageR.size(), {"norm.in1"});

#if !__X86_DEVICE__
            std::cout << "Graph run(" << tiles_sz[0] * tiles_sz[1] << ")\n";
            gHndl.run(tiles_sz[0] * tiles_sz[1]);
#endif
            stitcher.aie2host_nb(dstData.data(), dst.size(), tiles_sz, {"norm.out1"});
            tiler.wait({"norm.in1"});
#if !__X86_DEVICE__
            gHndl.wait();
#endif
            stitcher.wait({"norm.out1"});
            STOP_TIMER("normalize function")
            std::cout << "Data transfer complete (Stitcher)\n";
            tt += tdiff;
            //@}
        }

        // Analyze output
        std::cout << "Analyzing diff\n";
        cv::Mat diff(op_height, op_width, CV_8UC4);

        int ref = 0, aie = 0;
        for (int ii = 0; ii < dst.rows; ii++) {
            for (int jj = 0; jj < dst.cols; jj++) {
                for (int kk = 0; kk < dst.channels(); kk++) {
                    ref = dstRefImage.at<cv::Vec<signed char, 4> >(ii, jj)[kk];
                    aie = dst.at<cv::Vec<signed char, 4> >(ii, jj)[kk];
                    diff.at<cv::Vec4b>(ii, jj)[kk] = abs(ref - aie);
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
        //}
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
