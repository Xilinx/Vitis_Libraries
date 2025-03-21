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

#include <adf/adf_api/XRTConfig.h>
#include <chrono>
#include <common/xf_aie_sw_utils.hpp>
#include <common/xfcvDataMovers.h>
#include <fstream>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <xaiengine.h>
#include <xrt/experimental/xrt_kernel.h>

#include "graph.cpp"
using namespace std;

void cvtColor_RGB2YUY2(cv::Mat& src, cv::Mat& dst) {
    cv::Mat temp;
    cv::cvtColor(src, temp, 83);

    std::vector<uint8_t> v1;
    for (int i = 0; i < src.rows; i++) {
        for (int j = 0; j < src.cols; j++) {
            v1.push_back(temp.at<cv::Vec3b>(i, j)[0]);
            j % 2 ? v1.push_back(temp.at<cv::Vec3b>(i, j)[2]) : v1.push_back(temp.at<cv::Vec3b>(i, j)[1]);
        }
    }

    cv::Mat yuy2(src.rows, src.cols, CV_8UC2);
    memcpy(yuy2.data, v1.data(), src.cols * src.rows * 2);
    dst = yuy2;
}

/*
 ******************************************************************************
 * Top level executable
 ******************************************************************************
 */

void p(int16_t* arr, int h, int w) {
    for (int i = 0; i < h; i++) {
        for (int j = 0; j < w; j++) std::cout << arr[i * w + j] << " ";
        std::cout << "\n";
    }
    std::cout << "\n";
    std::cout << "\n";
}

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
        cv::Mat srcImageR, temp1, temp2;
        temp1 = cv::imread(argv[2], 1);
        cvtColor_RGB2YUY2(temp1, temp2);
        temp2.convertTo(srcImageR, CV_16SC2);

        // TODO: remove this resize
        cv::resize(srcImageR, srcImageR, cv::Size(128, 128));

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
        std::cout << srcImageR.channels() << std::endl;
        std::cout << srcImageR.elemSize() << std::endl;
        std::cout << srcImageR.total() << std::endl;
        std::cout << srcImageR.type() << std::endl;

        int op_width = srcImageR.cols;
        int op_height = srcImageR.rows;

        // Initializa device
        xF::deviceInit(xclBinName);

        // Load image
        std::vector<int16_t> srcData;
        srcData.assign(height * width * 2, 0);
        memcpy(srcData.data(), srcImageR.data, height * width * 2 * 2);
        std::cout << "srcData.size() = " << srcData.size() << std::endl;

        // Allocate output buffer
        std::vector<int16_t> dstData;
        dstData.assign(op_height * op_width * 2, 0);
        std::cout << "dstData.size() = " << dstData.size() << std::endl;

        cv::Mat dst(op_height, op_width, CV_16SC2, (void*)dstData.data());
        cv::Mat dstOutImage(op_height, op_width, srcImageR.type());

        // run reference
        int16_t* dataRefOut = (int16_t*)std::malloc(srcImageR.cols * srcImageR.rows * 2 * sizeof(int16_t));
        run_ref(srcData.data(), dataRefOut, kData, srcImageR.rows, srcImageR.cols);

        xF::xfcvDataMovers<xF::TILER, int16_t, TILE_HEIGHT, TILE_WIDTH, VECTORIZATION_FACTOR, 1, 0, true> tiler(1, 1,
                                                                                                                2);
        xF::xfcvDataMovers<xF::STITCHER, int16_t, TILE_HEIGHT, TILE_WIDTH, VECTORIZATION_FACTOR, 1, 0, true> stitcher(
            2);

#if !__X86_DEVICE__
        std::cout << "Graph init. This does nothing because CDO in boot PDI "
                     "already configures AIE.\n";
        auto gHndl_filter2d = xrt::graph(xF::gpDhdl, xF::xclbin_uuid, "filter_graph");
        std::cout << "XRT graph opened" << std::endl;
        gHndl_filter2d.reset();
        std::array<int16_t, 16> ret = float2fixed_coeff<10, 16>(kData);
        gHndl_filter2d.update("filter_graph.KC", ret);
        std::cout << "Graph reset done" << std::endl;
#endif

        START_TIMER
        tiler.compute_metadata(srcImageR.size());
        STOP_TIMER("Meta data compute time")

        std::chrono::microseconds tt(0);
        for (int i = 0; i < iterations; i++) {
            //@{
            std::cout << "Iteration : " << (i + 1) << std::endl;
            START_TIMER
            // p(srcData.data(), op_height, op_width*2);
            auto tiles_sz = tiler.host2aie_nb(srcData.data(), srcImageR.size(), {"filter_graph.inptr"});
            stitcher.aie2host_nb(dstData.data(), dst.size(), tiles_sz, {"filter_graph.outptr"});
#if !__X86_DEVICE__
            std::cout << "Graph run(" << (tiles_sz[0] * tiles_sz[1]) << ")\n";
            gHndl_filter2d.run(tiles_sz[0] * tiles_sz[1]);
            gHndl_filter2d.wait();
#endif

            stitcher.wait({"filter_graph.outptr"});
            std::cout << "Graph run complete!\n";
            STOP_TIMER("filter2D function!!!")
            tt += tdiff;
            //@}

            // Saturate the output values to [0,255]
            dst = cv::max(dst, 0);
            dst = cv::min(dst, 255);

            // Convert 16-bit output to 8-bit
            dst.convertTo(dstOutImage, srcImageR.type());

            // compare the results
            {
                int acceptableError = 1;
                int errCount = 0;
                int16_t* dataOut = dstData.data();
                for (int i = 0; i < srcImageR.cols * srcImageR.rows * 2; i++) {
                    if (abs(dataRefOut[i] - dataOut[i]) > acceptableError) {
                        std::cout << "err at : i=" << i << " err=" << abs(dataRefOut[i] - dataOut[i]) << "="
                                  << dataRefOut[i] << "-" << dataOut[i] << std::endl;
                        errCount++;
                    }
                }
                if (errCount) {
                    std::cerr << "Test failed!" << std::endl;
                    exit(-1);
                }
            }
        }

        gHndl_filter2d.end();
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
