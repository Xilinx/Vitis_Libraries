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
        cv::Mat srcImageRGBA, srcImageBGR;
        srcImageBGR = cv::imread(argv[2], 1);

        int width = srcImageBGR.cols;
        if (argc >= 4) width = atoi(argv[3]);
        int height = srcImageBGR.rows;
        if (argc >= 5) height = atoi(argv[4]);

        if ((width != srcImageBGR.cols) || (height != srcImageBGR.rows))
            cv::resize(srcImageBGR, srcImageBGR, cv::Size(width, height));

        cv::cvtColor(srcImageBGR, srcImageRGBA, cv::COLOR_BGR2RGBA);
        cv::imwrite("src_BGR.png", srcImageBGR);
        cv::imwrite("src_RGBA.png", srcImageRGBA);

        int iterations = 1;
        if (argc >= 6) iterations = atoi(argv[5]);

        // Convert to 4K RGB
        int width_in = srcImageRGBA.cols;
        int height_in = srcImageRGBA.rows;
        cv::imwrite("input_rgba_check.png", srcImageBGR);
        std::cout << "Image size of RGB : " << srcImageRGBA.cols << " x " << srcImageRGBA.rows << " x "
                  << srcImageRGBA.channels() << std::endl;

        cv::Mat opencvRGB;
        cv::cvtColor(srcImageRGBA, opencvRGB, cv::COLOR_RGBA2RGB);
        //////////////////////////////////////////
        // Run AIE
        //////////////////////////////////////////
        cv::Mat srcImage = srcImageRGBA;

        // Initializa device
        xF::deviceInit(xclBinName);

        // Load image
        void* srcData = nullptr;
        std::cout << "src_hndl size" << (srcImage.total() * srcImage.elemSize()) << std::endl;
        xrt::bo src_hndl = xrt::bo(xF::gpDhdl, (srcImage.total() * srcImage.elemSize()), 0, 0);
        srcData = src_hndl.map();
        memcpy(srcData, srcImage.data, (srcImage.total() * srcImage.elemSize()));

        // Allocate output buffer
        void* dstData;
        xrt::bo dst_hndl = xrt::bo(xF::gpDhdl, (srcImage.cols * srcImage.rows * srcImageBGR.channels()), 0, 0);
        dstData = dst_hndl.map();
        cv::Mat dst(srcImage.rows, srcImage.cols, CV_8UC3, (void*)dstData);
        xF::xfcvDataMoverParams params(cv::Size(srcImage.cols, srcImage.rows), cv::Size(srcImage.cols, srcImage.rows));
        xF::xfcvDataMovers<xF::TILER, uint8_t, TILE_HEIGHT, TILE_WIDTH, N, NO_COLS, 16, false> tiler(0, 0, false, 4);
        xF::xfcvDataMovers<xF::STITCHER, uint8_t, TILE_HEIGHT, TILE_WIDTH, N, NO_COLS, 16, false> stitcher(true);

        std::cout << "Graph init. This does nothing because CDO in boot PDI "
                     "already configures AIE.\n";

#if !__X86_DEVICE__
        std::vector<xrt::graph> gHndl;
        for (int k = 0; k < NO_COLS; k++) {
            std::string graph_name = "mygraph[" + std::to_string(k) + "]";
            std::cout << graph_name << std::endl;
            gHndl.push_back(xrt::graph(xF::gpDhdl, xF::xclbin_uuid, graph_name));
            std::cout << "XRT graph opened" << std::endl;
            gHndl.back().reset();
            std::cout << "Graph reset done" << std::endl;
        }
#endif

        START_TIMER
        tiler.compute_metadata(srcImage.size(), cv::Size(srcImage.cols, srcImage.rows));
        STOP_TIMER("Meta data compute time i ")

        std::chrono::microseconds tt(0);
        for (int i = 0; i < iterations; i++) {
            //@{
            std::cout << std::endl;
            std::cout << " ****  Iteration : **** " << (i + 1) << std::endl;
            START_TIMER
            auto tiles_sz = tiler.host2aie_nb(&src_hndl, srcImage.size(), params);
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
            STOP_TIMER("rgba2rgb function")

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

        //////////////////////////////////////////
        // Analyze output
        //////////////////////////////////////////

        cv::Mat diff;
        cv::absdiff(opencvRGB, dst, diff); // Absolute difference

        cv::imwrite("aie.png", dst);
        cv::imwrite("opencv.png", opencvRGB);
        cv::imwrite("diff.png", diff);

        float err_per;
        analyzeDiff(diff, 1, err_per);
        if (err_per > 0.0f) {
            std::cerr << "Test failed" << std::endl;
            exit(-1);
        }
        //////////////////////////////////////////
        // Print results
        //////////////////////////////////////////

        std::cout << std::endl;
        std::cout << " **************************** " << std::endl;
        std::cout << " ****    TEST RESULTS    **** " << std::endl;
        std::cout << " **************************** " << std::endl;

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
