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
/* #include <chrono>
#include <common/xf_aie_sw_utils.hpp>
#include <common/xfcvDataMovers.h>
#include <fstream>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <experimental/xrt_kernel.h>
#include <experimental/xrt_graph.h>
#include "xrt/xrt_kernel.h"
#include "xrt/xrt_graph.h"
#include "xrt/xrt_aie.h"
#include "config.h"
 */
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

//#include "graph.cpp"

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
void convert16UC1to8UC2(const cv::Mat& inputImage, cv::Mat& outputImage) {
    // Ensure the input image is of type CV_16UC1
    if (inputImage.type() != CV_16UC1) {
        throw std::invalid_argument("Input image must be of type CV_16UC1");
    }

    // Create an output image with type CV_8UC2 and the same dimensions as the input
    outputImage.create(inputImage.rows, inputImage.cols, CV_8UC2);

    // Iterate over the input image and scale down to 8-bit
    for (int y = 0; y < inputImage.rows; ++y) {
        for (int x = 0; x < inputImage.cols; ++x) {
            // Read the 16-bit value from the input image
            uint16_t value = inputImage.at<uint16_t>(y, x);

            // Scale the 16-bit value to an 8-bit value (e.g., by dividing by 256)
            uint8_t lowByte = static_cast<uint8_t>(value & 0xFF);         // Lower 8 bits
            uint8_t highByte = static_cast<uint8_t>((value >> 8) & 0xFF); // Upper 8 bits

            // Pack the two 8-bit values into the two channels of the output image
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
        std::vector<uint8_t> srcData;
        std::vector<uint8_t> srcData1;
        srcData.assign(srcImageY.data, (srcImageY.data + srcImageY.total() * srcImageY.channels()));
        srcData1.assign(srcImageUV.data, (srcImageUV.data + (srcImageUV.total() * 2)));

        std::cout << "srcData.size() = " << srcData.size() << std::endl;
        std::cout << "srcData1.size() = " << srcData1.size() << std::endl;

        // Allocate output buffer
        std::vector<uint8_t> dstData;
        std::vector<uint8_t> dstData1;
        dstData.assign(op_heighty * op_widthy, 0);
        dstData1.assign(op_heightuv * op_widthuv * 2, 0);
        std::cout << "dstData.size() = " << dstData.size() << std::endl;
        std::cout << "dstData1.size() = " << dstData1.size() << std::endl;

        cv::Mat dst(op_heighty, op_widthy, CV_8UC1, (void*)dstData.data());
        cv::Mat dst1(op_heightuv, op_widthuv, CV_8UC2, (void*)dstData1.data());

        // run C- reference
        cv::Mat dataRefOut(op_heighty, op_widthy, CV_8UC1);
        cv::Mat dataRefOut1(op_heightuv, op_widthuv, CV_8UC2);
        run_ref(srcImageY, srcImageUV, dataRefOut, dataRefOut1, srcImageY.rows, srcImageY.cols);

        xF::xfcvDataMovers<xF::TILER, uint8_t, Y_IN_TILE_HEIGHT, Y_IN_TILE_WIDTH, VECTORIZATION_FACTOR, 1, 0, true>
            tiler(0, 0);
        xF::xfcvDataMovers<xF::TILER, uint8_t, UV_IN_TILE_HEIGHT, (UV_IN_TILE_WIDTH / 2), VECTORIZATION_FACTOR, 1, 0,
                           true>
            tiler1(0, 0, 2);
        xF::xfcvDataMovers<xF::STITCHER, uint8_t, Y_OUT_TILE_HEIGHT, Y_OUT_TILE_WIDTH, VECTORIZATION_FACTOR, 1, 0, true>
            stitcher;
        xF::xfcvDataMovers<xF::STITCHER, uint8_t, UV_OUT_TILE_HEIGHT, (UV_OUT_TILE_WIDTH / 2), VECTORIZATION_FACTOR, 1,
                           0, true>
            stitcher1(2);

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
            auto tiles_sz = tiler.host2aie_nb(srcData.data(), srcImageY.size(), {"nv12resize_graph.yinptr"});
            stitcher.aie2host_nb(dstData.data(), dst.size(), tiles_sz, {"nv12resize_graph.youtptr"});

#if !__X86_DEVICE__
            START_TIMER
            gHndl.run(tiles_sz[0] * tiles_sz[1]);
#endif
#if !__X86_DEVICE__
            gHndl.wait();
#endif
            STOP_TIMER("NV12RESIZE_Y function")

            tiler.wait({"nv12resize_graph.yinptr"});

            stitcher.wait({"nv12resize_graph.youtptr"});

            tt += tdiff;
            //@}
            cv::imwrite("resized_y.png", dataRefOut);

            // compare the y results
            {
                int acceptableError = 1;
                int errCount = 0;
                uint8_t* dataOut = dstData.data();

                for (int i = 0; i < dataRefOut.cols * dataRefOut.rows; i++) {
                    if (abs(dataRefOut.data[i] - dataOut[i]) > acceptableError) {
                        std::cout << "err at : i=" << i << " err=" << abs(dataRefOut.data[i] - dataOut[i]) << "="
                                  << dataRefOut.data[i] << "-" << dataOut[i] << std::endl;
                        errCount++;
                    }
                }

                if (errCount) {
                    std::cerr << "Test failed!" << std::endl;
                    exit(-1);
                } else {
                    std::cout << "Test Passes\n" << std::endl;
                }
            }

            // TODO: Save the output image(dstOutImage) if need be
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
                auto tiles_sz1 = tiler1.host2aie_nb(srcData1.data(), srcImageUV.size(), {"nv12resize_uvgraph.uvinptr"});
                stitcher1.aie2host_nb(dstData1.data(), dst1.size(), tiles_sz1, {"nv12resize_uvgraph.uvoutptr"});
                START_TIMER
#if !__X86_DEVICE__
                gHndl.run(tiles_sz1[0] * tiles_sz1[1]);
#endif
#if !__X86_DEVICE__
                gHndl.wait();
#endif
                STOP_TIMER("NV12RESIZE function")

                tiler1.wait({"nv12resize_uvgraph.uvinptr"});
                stitcher1.wait({"nv12resize_uvgraph.uvoutptr"});
                tt += tdiff;
                //@}
                // compare the UV results
                {
                    int acceptableError = 1;
                    int errCount = 0;
                    uint8_t* dataOut = dstData1.data();
                    // FILE* fp=fopen("ref.txt","w");
                    // FILE* fp1=fopen("aie.txt","w");

                    for (int i = 0; i < dataRefOut1.cols * dataRefOut1.rows * 2; i++) {
                        if (abs(dataRefOut1.data[i] - dataOut[i]) > acceptableError) {
                            std::cout << "err at : i=" << i << " err=" << abs(dataRefOut1.data[i] - dataOut[i]) << "="
                                      << dataRefOut1.data[i] << "-" << dataOut[i] << std::endl;
                            errCount++;
                        }
                    }
                    if (errCount) {
                        std::cerr << "Test failed!" << std::endl;
                        exit(-1);
                    }
                }
                // TODO: Save the output image(dstOutImage) if need be
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
