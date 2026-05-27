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
using namespace cv;

void findMaxDifference_hist_table(const Mat& img1, const Mat& img2) {
    if (img1.empty() || img2.empty()) {
        std::cout << "Error: One or both images are empty!" << std::endl;
        return;
    }

    if (img1.size() != img2.size() || img1.type() != img2.type()) {
        std::cout << "Error: Images must have the same size and type!" << std::endl;
        return;
    }

    // Compute error map
    cv::Mat error;
    cv::absdiff(img1, img2, error);

    // Count occurrences of each error value
    std::map<int, int> error_count;
    for (int i = 0; i < error.rows; i++) {
        for (int j = 0; j < error.cols; j++) {
            int value = error.at<uchar>(i, j);
            error_count[value]++;
        }
    }

    // Print table
    std::cout << "Error Value | Count\n";
    std::cout << "-----------|------\n";
    for (const auto& pair : error_count) {
        std::cout << pair.first << " | " << pair.second << "\n";
    }
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
        int iterations = 1;

        //////////////////////////////////////////
        // Read image from file and resize
        //////////////////////////////////////////
        cv::Mat srcImageBGR, hlsImage;
        srcImageBGR = cv::imread(argv[2], 1);
        if (srcImageBGR.empty()) {
            std::cout << "Error: Could not load image!\n";
            return -1;
        }
        cv::cvtColor(srcImageBGR, hlsImage, cv::COLOR_BGR2HLS);
        cv::imwrite("hlsImage_in.png", hlsImage);
        std::cout << "Image size of hlsImage : " << hlsImage.cols << " x " << hlsImage.rows << " x "
                  << hlsImage.channels() << " x " << hlsImage.elemSize() << std::endl;

        // Convert to 4K HLS
        int width_in = IMAGE_WIDTH_IN;
        int height_in = IMAGE_HEIGHT_IN;
        cv::resize(hlsImage, hlsImage, cv::Size(width_in, height_in));
        cv::imwrite("hlsImage_resize.png", hlsImage);
        std::cout << "Image size of RGB : " << hlsImage.cols << " x " << hlsImage.rows << " x " << hlsImage.channels()
                  << std::endl;

        //////////////////////////////////////////
        // Run opencv reference
        //////////////////////////////////////////
        cv::Mat bgrImage_opencv;
        cv::cvtColor(hlsImage, bgrImage_opencv, cv::COLOR_HLS2BGR);
        cv::imwrite("bgrImage_opencv.png", bgrImage_opencv);
        std::cout << "Image size of bgrImage_opencv : " << bgrImage_opencv.cols << " x " << bgrImage_opencv.rows
                  << " x " << bgrImage_opencv.channels() << " x " << bgrImage_opencv.elemSize() << std::endl;

        //////////////////////////////////////////
        // Run AIE
        //////////////////////////////////////////

        // Initializa device
        xF::deviceInit(xclBinName);

        // Load image
        void* srcData = nullptr;
        std::cout << "src_hndl size" << (hlsImage.total() * hlsImage.elemSize()) << std::endl;
        xrt::bo src_hndl = xrt::bo(xF::gpDhdl, (hlsImage.total() * hlsImage.elemSize()), 0, 0);
        srcData = src_hndl.map();
        memcpy(srcData, hlsImage.data, (hlsImage.total() * hlsImage.elemSize()));

        // Allocate output buffer
        void* dstData;
        xrt::bo dst_hndl =
            xrt::bo(xF::gpDhdl, (IMAGE_HEIGHT_OUT * IMAGE_WIDTH_OUT * (CHANNELS - 1) * hlsImage.elemSize()), 0, 0);
        dstData = dst_hndl.map();
        cv::Mat dst(IMAGE_HEIGHT_OUT, IMAGE_WIDTH_OUT, CV_8UC3, (void*)dstData);
        xF::xfcvDataMoverParams params(cv::Size(IMAGE_WIDTH_IN, IMAGE_HEIGHT_IN),
                                       cv::Size(IMAGE_WIDTH_OUT, IMAGE_HEIGHT_OUT));
        xF::xfcvDataMovers<xF::TILER, uint8_t, TILE_HEIGHT_IN, TILE_WIDTH_IN, 64, NO_COLS, 16, false> tiler(0, 0, true,
                                                                                                            4);
        xF::xfcvDataMovers<xF::STITCHER, uint8_t, TILE_HEIGHT_OUT, TILE_WIDTH_OUT, 64, NO_COLS, 16, false> stitcher(
            true);

        std::cout << "Graph init. This does nothing because CDO in boot PDI "
                     "already configures AIE.\n";

#if !__X86_DEVICE__
        std::vector<xrt::graph> gHndl;
        for (int k = 0; k < NO_COLS; k++) {
            std::string graph_name = "hls2rgb[" + std::to_string(k) + "]";
            std::cout << graph_name << std::endl;
            gHndl.push_back(xrt::graph(xF::gpDhdl, xF::xclbin_uuid, graph_name));
            std::cout << "XRT graph opened" << std::endl;
            gHndl.back().reset();
            std::cout << "Graph reset done" << std::endl;
        }
#endif

        START_TIMER
        tiler.compute_metadata(hlsImage.size(), cv::Size(IMAGE_WIDTH_OUT, IMAGE_HEIGHT_OUT));
        STOP_TIMER("Meta data compute time = ")

        std::chrono::microseconds tt(0);
        for (int i = 0; i < iterations; i++) {
            //@{
            std::cout << std::endl;
            std::cout << " ****  Iteration : **** " << (i + 1) << std::endl;
            START_TIMER
            auto tiles_sz = tiler.host2aie_nb(&src_hndl, hlsImage.size(), params);
            stitcher.aie2host_nb(&dst_hndl, dst.size(), tiles_sz);

#if !__X86_DEVICE__

            for (int i = 0; i < NO_COLS; i++) {
                std::cout << "Graph run(" << tiler.tilesPerCore(i) / NO_CORES_PER_COL << ")\n";

                gHndl[i].run(tiler.tilesPerCore(i) / NO_CORES_PER_COL);
            }
            for (int i = 0; i < NO_COLS; i++) {
                gHndl[i].wait();
            }

#endif

            tiler.wait();
            stitcher.wait();
            STOP_TIMER("hls2rgb function")

            std::cout << "Data transfer complete (Stitcher) \n";
            tt += tdiff;
            //@}
        }
#if !__X86_DEVICE__
        for (int i = 0; i < NO_COLS; i++) {
            gHndl[i].end(0);
        }
#endif
        std::cout << "Image size of dst : " << dst.cols << " x " << dst.rows << " x " << dst.channels() << " "
                  << std::endl;
        cv::imwrite("aieout.png", dst);
        cv::imwrite("opencvout.png", bgrImage_opencv);

        //////////////////////////////////////////
        // Analyze output
        //////////////////////////////////////////

        std::cout << std::endl;
        std::cout << " **************************** " << std::endl;
        std::cout << " ****    ERROR TABLE     **** " << std::endl;
        std::cout << " ****    OPENCV/AIE      **** " << std::endl;
        std::cout << " **************************** " << std::endl;

        findMaxDifference_hist_table(bgrImage_opencv, dst);

        std::cout << std::endl;
        std::cout << " **************************** " << std::endl;
        std::cout << " ****    PROFILE NOs.    **** " << std::endl;
        std::cout << " **************************** " << std::endl;

        std::cout << "Average time to process frame : " << (((float)tt.count() * 0.001) / (float)iterations) << " ms"
                  << std::endl;
        std::cout << "Average frames per second : " << (((float)1000000 / (float)tt.count()) * (float)iterations)
                  << " fps" << std::endl;

        std::this_thread::sleep_for(std::chrono::seconds(2));
        std::cout << std::endl;

        return 0;

    } catch (std::exception& e) {
        const char* errorMessage = e.what();
        std::cerr << "Exception caught: " << errorMessage << std::endl;
        exit(-1);
    }
}
