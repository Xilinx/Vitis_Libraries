/*
 * Copyright 1986-2022 Xilinx, Inc. All Rights Reserved.
 * Copyright 2023-2025 Advanced Micro Devices, Inc. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
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
#include <experimental/xrt_kernel.h>
#include <experimental/xrt_graph.h>

#include "config.h"
#include "graph.cpp"

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

        cv::Mat srcImageRGB, srcImageBGR;
        srcImageBGR = cv::imread(argv[2], 1);
        cv::cvtColor(srcImageBGR, srcImageRGB, cv::COLOR_BGR2RGB);
        int iterations = 1;

        // Convert to 4K RGB
        int width_in = IMAGE_WIDTH_IN;
        int height_in = IMAGE_HEIGHT_IN;

        cv::resize(srcImageRGB, srcImageRGB, cv::Size(width_in, height_in));
        cv::imwrite("input_rgba_check.png", srcImageRGB);
        std::cout << "Image size of RGB : " << srcImageRGB.cols << " x " << srcImageRGB.rows << " x "
                  << srcImageRGB.channels() << std::endl;

        //////////////////////////////////////////
        // Run opencv reference test
        //////////////////////////////////////////

        // Convert the BGR image to YUV format (YUV444 in OpenCV uses I420 format, which is planar)
        cv::Mat image_yuv;
        cv::cvtColor(srcImageRGB, image_yuv, cv::COLOR_BGR2YUV);

        // Extract the Y, U, and V channels from the YUV image
        cv::Mat srcImageY;
        cv::extractChannel(image_yuv, srcImageY, 0); // Extract Y channel
        std::cout << "Image size of srcImageY : " << srcImageY.cols << " x " << srcImageY.rows << " x "
                  << srcImageY.channels() << " " << std::endl;

        // Calculate the std_dev of the srcImageY

        cv::Scalar mean1, std_dev;
        cv::meanStdDev(srcImageY, mean1, std_dev);
        std::cout << "std_dev of the srcImageY:" << std::endl;

        // calculate the sum of (x-mean)^2
        double sumSquaredDiffR = 0.0;
        for (int y = 0; y < srcImageY.rows; y++) {
            for (int x = 0; x < srcImageY.cols; x++) {
                uint8_t pixel = srcImageY.at<uint8_t>(y, x);
                sumSquaredDiffR += std::pow((double)pixel - mean1[0], 2);
            }
        }

        //////////////////////////////////////////
        // Run on AIE
        //////////////////////////////////////////

        // Initializa device
        xF::deviceInit(xclBinName);

        // Load image
        void* srcData = nullptr;
        std::cout << "src_hndl size" << (srcImageY.total() * srcImageY.elemSize()) << std::endl;
        xrt::bo src_hndl = xrt::bo(xF::gpDhdl, (srcImageY.total() * srcImageY.elemSize()), 0, 0);
        srcData = src_hndl.map();
        memcpy(srcData, srcImageY.data, (srcImageY.total() * srcImageY.elemSize()));

        // Allocate output buffer
        void* dstData;
        xrt::bo dst_hndl = xrt::bo(xF::gpDhdl, (IMAGE_HEIGHT_OUT * IMAGE_WIDTH_OUT * CHANNELS), 0, 0);
        dstData = dst_hndl.map();
        cv::Mat dst(IMAGE_HEIGHT_OUT, IMAGE_WIDTH_OUT, CV_8UC1, (void*)dstData);
        xF::xfcvDataMoverParams params(cv::Size(IMAGE_WIDTH_IN, height_in),
                                       cv::Size(IMAGE_WIDTH_OUT, IMAGE_HEIGHT_OUT));
        xF::xfcvDataMovers<xF::TILER, uint8_t, TILE_HEIGHT_IN, TILE_WIDTH_IN, 64, NO_COLS, 16, false> tiler(0, 0, false,
                                                                                                            4);
        xF::xfcvDataMovers<xF::STITCHER, uint8_t, TILE_HEIGHT_OUT, TILE_WIDTH_OUT, 64, NO_COLS, 16, false> stitcher(
            false);

        std::cout << "Graph init. This does nothing because CDO in boot PDI "
                     "already configures AIE.\n";
#if !__X86_DEVICE__
        std::vector<xrt::graph> gHndl;
        std::string graph_name_RTP[3];
        int reset = 1;
        float m_Y = (float)mean1[0];

        for (int k = 0; k < NO_COLS; k++) {
            std::string graph_name = "stddev[" + std::to_string(k) + "]";
            std::cout << graph_name << std::endl;
            gHndl.push_back(xrt::graph(xF::gpDhdl, xF::xclbin_uuid, graph_name));
            std::cout << "XRT graph opened" << std::endl;
            gHndl.back().reset();
            std::cout << "Graph reset done" << std::endl;
            for (int i = 0; i < NO_CORES_PER_COL; i++) {
                for (int j = 1; j < 3; j++) {
                    graph_name_RTP[j] = graph_name + ".k[" + std::to_string(i) + "].in[" + std::to_string(j) + "]";
                    std::cout << graph_name_RTP[j] << std::endl;
                }
                gHndl[k].update(graph_name_RTP[1], reset);
                gHndl[k].update(graph_name_RTP[2], m_Y);
            }
        }
#endif

        START_TIMER
        tiler.compute_metadata(srcImageY.size(), cv::Size(IMAGE_WIDTH_OUT, IMAGE_HEIGHT_OUT));
        STOP_TIMER("Meta data compute time = ")

        std::chrono::microseconds tt(0);
        for (int i = 0; i < iterations; i++) {
#if !__X86_DEVICE__
            START_TIMER
            std::cout << std::endl;
            std::cout << " ****   Iteration : **** " << (i + 1) << std::endl;

            reset = 1;
            for (int k = 0; k < NO_COLS; k++) {
                std::string graph_name = "stddev[" + std::to_string(k) + "]";
                for (int i = 0; i < NO_CORES_PER_COL; i++) {
                    for (int j = 1; j < 2; j++) {
                        graph_name_RTP[j] = graph_name + ".k[" + std::to_string(i) + "].in[" + std::to_string(j) + "]";
                    }
                    gHndl[k].update(graph_name_RTP[1], reset);
                }
            }
#endif
            //@{
            auto tiles_sz = tiler.host2aie_nb(&src_hndl, srcImageY.size(), params);
            stitcher.aie2host_nb(&dst_hndl, dst.size(), tiles_sz);

#if !__X86_DEVICE__

            for (int i = 0; i < NO_COLS; i++) {
                std::cout << "Graph run(" << 1 << ")\n";
                gHndl[i].run(1);
            }
            for (int i = 0; i < NO_COLS; i++) {
                gHndl[i].wait();
            }

            reset = 0;
            for (int k = 0; k < NO_COLS; k++) {
                std::string graph_name = "stddev[" + std::to_string(k) + "]";
                for (int i = 0; i < NO_CORES_PER_COL; i++) {
                    for (int j = 1; j < 2; j++) {
                        graph_name_RTP[j] = graph_name + ".k[" + std::to_string(i) + "].in[" + std::to_string(j) + "]";
                    }
                    gHndl[k].update(graph_name_RTP[1], reset);
                }
            }
            for (int i = 0; i < NO_COLS; i++) {
                std::cout << "Graph run(" << tiler.tilesPerCore(i) / NO_CORES_PER_COL - 1 << ")\n";
                gHndl[i].run(tiler.tilesPerCore(i) / NO_CORES_PER_COL - 1);
            }
            for (int i = 0; i < NO_COLS; i++) {
                gHndl[i].wait();
            }

#endif
            tiler.wait();
            stitcher.wait();
            STOP_TIMER("stddev function")
            std::cout << "Data transfer complete (Stitcher) \n";
            tt += tdiff;
            //@}
        }
#if !__X86_DEVICE__
        for (int i = 0; i < NO_COLS; i++) {
            gHndl[i].end(0);
        }
#endif
        //////////////////////////////////////////
        // Analyze Difference
        //////////////////////////////////////////
        std::cout << std::endl;
        std::cout << " *** OPENCV Sum of Squared Differences (Y): =" << sumSquaredDiffR << std::endl;
        std::cout << " *** OPENCV STD DEV (Y): " << std::sqrt(sumSquaredDiffR / (IMAGE_HEIGHT_IN * IMAGE_WIDTH_IN))
                  << std::endl;
        std::cout << std::endl;

        std::cout << std::endl;
        cv::Vec4f pixel = dst.at<cv::Vec4f>(IMAGE_HEIGHT_OUT - 1, 0);
        std::cout << " *** AIE Sum of Squared Differences (Y): = " << pixel[0] << std::endl;
        std::cout << " *** AIE STD DEV (Y) " << std::sqrt(pixel[0] / (IMAGE_HEIGHT_IN * IMAGE_WIDTH_IN)) << ", "
                  << std::endl;
        std::cout << std::endl;

        return 0;

    } catch (std::exception& e) {
        const char* errorMessage = e.what();
        std::cerr << "Exception caught: " << errorMessage << std::endl;
        exit(-1);
    }
}
