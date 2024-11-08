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
#include <common/xf_aie_sw_utils.hpp>
#include <common/xfcvDataMovers.h>
#include <fstream>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <experimental/xrt_kernel.h>
#include <experimental/xrt_graph.h>
#include "config.h"

void blacklevel_ref(cv::Mat& input, cv::Mat& output, uint8_t blacklevel, float mul_fact) {
    int height = input.size().height;
    int width = input.size().width;
    typedef uint8_t Pixel_t;

    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            Pixel_t Pixel = input.at<Pixel_t>(i, j);
            output.at<Pixel_t>(i, j) = cv::saturate_cast<unsigned char>((Pixel - blacklevel) * mul_fact);
        }
    }
}

int run_opencv_ref(cv::Mat& srcImageR, cv::Mat& dstRefImage, int16_t blacklevel, float mul_fact) {
    blacklevel_ref(srcImageR, dstRefImage, blacklevel, mul_fact);
    return 0;
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
            errorMessage << argv[0] << " <xclbin> <inputImage>  "
                                       "[width] [height] [iterations]";
            std::cerr << errorMessage.str();
            throw std::invalid_argument(errorMessage.str());
        }

        const char* xclBinName = argv[1];
        //////////////////////////////////////////
        // Read image from file and resize
        //////////////////////////////////////////
        cv::Mat srcImageR;
        srcImageR = cv::imread(argv[2], 0);

        int width = srcImageR.cols;
        if (argc >= 4) width = atoi(argv[3]);
        int height = srcImageR.rows;
        if (argc >= 5) height = atoi(argv[4]);

        if ((width != srcImageR.cols) || (height != srcImageR.rows))
            cv::resize(srcImageR, srcImageR, cv::Size(width, height));

        int iterations = 1;
        if (argc >= 6) iterations = atoi(argv[5]);

        std::cout << "Image size" << std::endl;
        std::cout << srcImageR.rows << std::endl;
        std::cout << srcImageR.cols << std::endl;
        std::cout << srcImageR.elemSize() << std::endl;
        std::cout << "Image size (end)" << std::endl;

        int op_width = srcImageR.cols;
        int op_height = srcImageR.rows;

        //////////////////////////////////////////
        // Run opencv reference test (filter2D design)
        //////////////////////////////////////////
        uint8_t black_level = 32;
        const int MaxLevel = 255; // 8b input value
        float MulValue1 = (float)((float)MaxLevel / (MaxLevel - black_level));
        uint16_t MulValue = 37470; // Q(1.15)

        cv::Mat dstRefImage(op_height, op_width, CV_8UC1);
        START_TIMER
        run_opencv_ref(srcImageR, dstRefImage, black_level, MulValue1);
        STOP_TIMER("OpenCV Ref");

        // Initializa device
        xF::deviceInit(xclBinName);

        // Allocate input buffer
        std::vector<uint8_t> srcData;
        srcData.assign(srcImageR.data, (srcImageR.data + srcImageR.total() * srcImageR.channels()));
        cv::Mat src(srcImageR.rows, srcImageR.cols, srcImageR.type(), (void*)srcData.data());

        // Allocate output buffer
        std::vector<uint8_t> dstData;
        dstData.assign(op_height * op_width, 0);
        cv::Mat dst(op_height, op_width, CV_8UC1, (void*)dstData.data());

        xF::xfcvDataMovers<xF::TILER, uint8_t, TILE_HEIGHT, TILE_WIDTH, VECTORIZATION_FACTOR, 1, 0, true> tiler(0, 0);
        xF::xfcvDataMovers<xF::STITCHER, uint8_t, TILE_HEIGHT, TILE_WIDTH, VECTORIZATION_FACTOR, 1, 0, true> stitcher;

#if !__X86_DEVICE__
        std::cout << "Graph init. This does nothing because CDO in boot PDI "
                     "already configures AIE.\n";
        auto gHndl = xrt::graph(xF::gpDhdl, xF::xclbin_uuid, "bl");
        std::cout << "XRT graph opened" << std::endl;
        gHndl.reset();
        std::cout << "Graph reset done" << std::endl;
        gHndl.update("bl.k1.in[1]", black_level);
        gHndl.update("bl.k1.in[2]", MulValue);
#endif

        START_TIMER
        tiler.compute_metadata(srcImageR.size(), cv::Size(op_width, op_height));
        STOP_TIMER("Meta data compute time")

        std::chrono::microseconds tt(0);
        for (int i = 0; i < iterations; i++) {
            //@{
            std::cout << "Iteration : " << (i + 1) << std::endl;
            std::cout << "Sending data: " << srcImageR.size() << "\n";
            std::cout << "Receiving data: " << dst.size() << "\n";
            START_TIMER
            auto tiles_sz = tiler.host2aie_nb(srcData.data(), srcImageR.size(), {"bl.in1"});

#if !__X86_DEVICE__
            std::cout << "Graph running for " << (tiles_sz[0] * tiles_sz[1]) << " iterations.\n";

            gHndl.run(tiles_sz[0] * tiles_sz[1]);
#endif
            stitcher.aie2host_nb(dstData.data(), dst.size(), tiles_sz, {"bl.out1"});

#if !__X86_DEVICE__
            gHndl.wait();
#endif
            stitcher.wait({"bl.out1"});

            STOP_TIMER("blacklevel function")

            std::cout << "Data transfer complete (Stitcher)\n";
            tt += tdiff;
            //@}

            // Analyze output {
            std::cout << "Analyzing diff\n";
            cv::Mat diff(op_height, op_width, srcImageR.type());
            cv::absdiff(dstRefImage, dst, diff);

            cv::imwrite("ref.png", dstRefImage);
            cv::imwrite("aie.png", dst);
            cv::imwrite("diff.png", diff);

            float err_per;
            analyzeDiff(diff, 1, err_per);
            if (err_per > 0.0f) {
                std::cerr << "Test failed" << std::endl;
                exit(-1);
            }
            //}
        }
#if !__X86_DEVICE__
        gHndl.end(0);
#endif
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
