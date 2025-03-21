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
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <xrt/experimental/xrt_kernel.h>
#include <xaiengine.h>

#include "opencv2/opencv.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgcodecs/imgcodecs.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include <opencv2/imgproc/types_c.h>

#include "config.h"

using namespace std;

void convert16UC1to8UC2(const cv::Mat& inputImage, cv::Mat& outputImage) {
    if (inputImage.type() != CV_16UC1) {
        throw std::invalid_argument("Input image must be of type CV_16UC1");
    }
    outputImage.create(inputImage.rows, inputImage.cols, CV_8UC2);
    for (int y = 0; y < inputImage.rows; ++y) {
        for (int x = 0; x < inputImage.cols; ++x) {
            uint16_t value = inputImage.at<uint16_t>(y, x);
            uint8_t lowByte = static_cast<uint8_t>(value & 0xFF);         // Lower 8 bits
            uint8_t highByte = static_cast<uint8_t>((value >> 8) & 0xFF); // Upper 8 bits
            outputImage.at<cv::Vec2b>(y, x) = cv::Vec2b(lowByte, highByte);
        }
    }
}
void run_ref(cv::Mat& y, cv::Mat& uv, cv::Mat& dst, cv::Mat& dst1, uint16_t y_height, uint16_t y_width) {
    // y
    cv::Size ynewSize(y_width / 5, y_height / 5);
    cv::resize(y, dst, ynewSize, 0, 0, cv::INTER_AREA);
    // uv
    cv::Mat uv_image_c2;
    convert16UC1to8UC2(uv, uv_image_c2);
    cv::Size uvnewSize(uv.cols / 5, uv.rows / 5);
    cv::resize(uv_image_c2, dst1, uvnewSize, 0, 0, cv::INTER_AREA);
}
/*
 ******************************************************************************
 * Top level executable
 ******************************************************************************
 */
int main(int argc, char** argv) {
    try {
        std::cout << "enterd main\n" << std::endl;
        if (argc < 3) {
            std::stringstream errorMessage;
            errorMessage << argv[0] << " <xclbin> <Y inputImage> <UV inputImage> [iterations]";
            std::cerr << errorMessage.str();
            throw std::invalid_argument(errorMessage.str());
        }
        std::cout << "enterd xclbin\n" << std::endl;
        const char* xclBinName = argv[1];
        //////////////////////////////////////////
        // Read image from file and resize
        //////////////////////////////////////////
        cv::Mat srcImageY, srcImageUV;
        srcImageY = cv::imread(argv[2], 0);
        srcImageUV = cv::imread(argv[3], -1);
        std::cout << "read images\n" << std::endl;
        // TODO: remove this resize
        int ywidth = srcImageY.cols;
        int uvwidth = srcImageUV.cols;

        int iterations = 1;
        if (argc >= 5) iterations = atoi(argv[4]);

        std::cout << "Image size" << std::endl;
        std::cout << "srcImageY.rows= " << srcImageY.rows << std::endl;
        std::cout << "srcImageY.cols=" << srcImageY.cols << std::endl;
        std::cout << "srcImageY.channels()" << srcImageY.channels() << std::endl;
        std::cout << "srcImageY.elemSize()" << srcImageY.elemSize() << std::endl;
        std::cout << "srcImageY.total()" << srcImageY.total() << std::endl;
        std::cout << "srcImageY.type()" << srcImageY.type() << std::endl;

        int op_widthy = (srcImageY.cols / 5);
        int op_heighty = (srcImageY.rows / 5);
        int op_widthuv = (srcImageUV.cols / 5);
        int op_heightuv = (srcImageUV.rows / 5);

        // Initializa device
        xF::deviceInit(xclBinName);

        // Load image
        void* srcData = nullptr;
        xrt::bo src_hndl = xrt::bo(xF::gpDhdl, (srcImageY.total() * srcImageY.elemSize()), 0, 0);
        srcData = src_hndl.map();
        memcpy(srcData, srcImageY.data, (srcImageY.total() * srcImageY.elemSize()));

        void* srcData1 = nullptr;
        xrt::bo src_hndl1 = xrt::bo(xF::gpDhdl, (srcImageUV.total() * 2), 0, 0);
        srcData1 = src_hndl1.map();
        memcpy(srcData1, srcImageUV.data, (srcImageUV.total() * 2));

        // Allocate output buffer
        void* dstData = nullptr;
        xrt::bo* ptr_dstHndl = new xrt::bo(xF::gpDhdl, (op_heighty * op_widthy * srcImageY.elemSize()), 0, 0);
        dstData = ptr_dstHndl->map();
        cv::Mat dst(op_heighty, op_widthy, srcImageY.type(), dstData);

        void* dstData1 = nullptr;
        xrt::bo* ptr_dstHndl1 = new xrt::bo(xF::gpDhdl, (op_heightuv * op_widthuv * 2), 0, 0);
        dstData1 = ptr_dstHndl1->map();
        cv::Mat dst1(op_heightuv, op_widthuv, CV_8UC2, dstData1);

        // run C- reference
        cv::Mat dataRefOut(op_heighty, op_widthy, CV_8UC1);
        cv::Mat dataRefOut1(op_heightuv, op_widthuv, CV_8UC2);
        run_ref(srcImageY, srcImageUV, dataRefOut, dataRefOut1, srcImageY.rows, srcImageY.cols);

        xF::xfcvDataMovers<xF::TILER, uint8_t, Y_IN_TILE_HEIGHT, Y_IN_TILE_WIDTH, VECTORIZATION_FACTOR> tiler(0, 0);
        xF::xfcvDataMovers<xF::TILER, uint8_t, UV_IN_TILE_HEIGHT, (UV_IN_TILE_WIDTH / 2), VECTORIZATION_FACTOR> tiler1(
            0, 0);
        xF::xfcvDataMovers<xF::STITCHER, uint8_t, Y_OUT_TILE_HEIGHT, Y_OUT_TILE_WIDTH, VECTORIZATION_FACTOR> stitcher;
        xF::xfcvDataMovers<xF::STITCHER, uint8_t, UV_OUT_TILE_HEIGHT, (UV_OUT_TILE_WIDTH / 2), VECTORIZATION_FACTOR>
            stitcher1;

#if !__X86_DEVICE__
        std::cout << "Graph init. This does nothing because CDO in boot PDI "
                     "already configures AIE.\n";
        auto gHndl = xrt::graph(xF::gpDhdl, xF::xclbin_uuid, "nv12resize_graph");
        std::cout << "XRT graph opened" << std::endl;
        gHndl.reset();
        std::cout << "Graph reset done" << std::endl;
#endif
        START_TIMER
        tiler.compute_metadata(srcImageY.size());
        STOP_TIMER("Meta data compute time")
        std::chrono::microseconds tt(0);
        for (int i = 0; i < iterations; i++) {
            //@{
            std::cout << "Iteration : " << (i + 1) << std::endl;

            auto tiles_sz = tiler.host2aie_nb(&src_hndl, srcImageY.size());
            stitcher.aie2host_nb(ptr_dstHndl, dst.size(), tiles_sz);

#if !__X86_DEVICE__
            START_TIMER
            gHndl.run(tiles_sz[0] * tiles_sz[1]);
            gHndl.wait();
#endif

            tiler.wait();
            stitcher.wait();
            STOP_TIMER("NV12RESIZE_Y function")

            tt += tdiff;
            //@}
            cv::imwrite("resized_y.png", dataRefOut);

            // compare the y results
            {
                cv::Mat diff(op_heighty, op_widthy, CV_8UC1);
                cv::absdiff(dataRefOut, dst, diff);
                cv::imwrite("ref_y.png", dataRefOut);
                cv::imwrite("aie_y.png", dst);
                cv::imwrite("diff_y.png", diff);

                float err_per;
                analyzeDiff(diff, 1, err_per);
                if (err_per > 0.0f) {
                    std::cerr << "Test failed" << std::endl;
                    exit(-1);
                }
            }
        }
#if !__X86_DEVICE__
        gHndl.end(0);
#endif
        std::cout << "Test passed" << std::endl;
        std::cout << "Average time to process Y frame : " << (((float)tt.count() * 0.001) / (float)iterations) << " ms"
                  << std::endl;
        std::cout << "Average frames per second : " << (((float)1000000 / (float)tt.count()) * (float)iterations)
                  << " fps" << std::endl;

        {
#if !__X86_DEVICE__
            std::cout << "Graph init. This does nothing because CDO in boot PDI "
                         "already configures AIE.\n";
            auto gHndl = xrt::graph(xF::gpDhdl, xF::xclbin_uuid, "nv12resize_uvgraph");
            std::cout << "XRT graph opened" << std::endl;
            gHndl.reset();
            std::cout << "Graph reset done" << std::endl;
#endif
            START_TIMER
            tiler1.compute_metadata(srcImageUV.size());
            STOP_TIMER("Meta data compute time")

            std::chrono::microseconds tt(0);
            for (int i = 0; i < iterations; i++) {
                //@{
                std::cout << "Iteration : " << (i + 1) << std::endl;
                auto tiles_sz1 = tiler1.host2aie_nb(&src_hndl1, srcImageUV.size());
                stitcher1.aie2host_nb(ptr_dstHndl1, dst1.size(), tiles_sz1);
                START_TIMER
#if !__X86_DEVICE__
                gHndl.run(tiles_sz1[0] * tiles_sz1[1]);
                gHndl.wait();
#endif

                tiler1.wait();
                stitcher1.wait();
                STOP_TIMER("NV12RESIZE function")

                tt += tdiff;
                //@}
                // compare the UV results
                {
                    int acceptableError = 1;
                    int errCount = 0;
                    for (int i = 0; i < dataRefOut1.cols * dataRefOut1.rows * 2; i++) {
                        if (abs(dataRefOut1.data[i] - dst1.data[i]) > acceptableError) {
                            std::cout << "err at : i=" << i << " err=" << abs(dataRefOut1.data[i] - dst1.data[i]) << "="
                                      << dataRefOut1.data[i] << "-" << dst1.data[i] << std::endl;
                            errCount++;
                        }
                    }
                    if (errCount) {
                        std::cerr << "Test failed!" << std::endl;
                        exit(-1);
                    }
                }
            }
#if !__X86_DEVICE__
            gHndl.end(0);
#endif
            std::cout << "Test passed" << std::endl;
            std::cout << "Average time to process UV frame : " << (((float)tt.count() * 0.001) / (float)iterations)
                      << " ms" << std::endl;
            std::cout << "Average frames per second : " << (((float)1000000 / (float)tt.count()) * (float)iterations)
                      << " fps" << std::endl;
        }

        return 0;
    } catch (std::exception& e) {
        const char* errorMessage = e.what();
        std::cerr << "Exception caught: " << errorMessage << std::endl;
        exit(-1);
    }
}
