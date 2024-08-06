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

/*
 ******************************************************************************
 * Top level executable
 ******************************************************************************
 */
void opencv_ref(cv::Mat src, cv::Mat dst) {
    cv::transpose(src, dst);
    return;
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
        cv::Mat srcImageR;
        srcImageR = cv::imread(argv[2], 0);

        int width = srcImageR.cols;
        if (argc >= 4) width = atoi(argv[3]);
        int height = srcImageR.rows;
        if (argc >= 5) height = atoi(argv[4]);

        int iterations = 1;
        if (argc >= 6) iterations = atoi(argv[5]);

        if ((width != srcImageR.cols) || (height != srcImageR.rows))
            cv::resize(srcImageR, srcImageR, cv::Size(width, height));

        std::cout << "Image size" << std::endl;
        std::cout << srcImageR.cols << std::endl;
        std::cout << srcImageR.rows << std::endl;
        std::cout << srcImageR.elemSize() << std::endl;
        std::cout << srcImageR.type() << std::endl;
        std::cout << "Image size (end)" << std::endl;
        int op_width = srcImageR.rows;
        int op_height = srcImageR.cols;

        //////////////////////////////////////////
        // Run opencv reference test (filter2D design)
        //////////////////////////////////////////
        cv::Mat dstRefImage(op_height, op_width, srcImageR.type());
        opencv_ref(srcImageR, dstRefImage);

        // Initializa device
        xF::deviceInit(xclBinName);

        // Load image
        std::vector<uint8_t> srcData;
        srcData.assign(srcImageR.data, (srcImageR.data + srcImageR.total()));
        cv::Mat src(srcImageR.rows, srcImageR.cols, CV_8UC1, (void*)srcData.data());

        std::vector<uint8_t> dstData;
        dstData.assign(op_height * op_width, 0);
        cv::Mat dst(op_height, op_width, CV_8UC1, (void*)dstData.data());

        xF::xfcvDataMovers<xF::TILER, uint8_t, TILE_HEIGHT, TILE_WIDTH, 16, 1, 0, true> tiler(0, 0);
        xF::xfcvDataMovers<xF::STITCHER, uint8_t, TILE_WIDTH, TILE_HEIGHT, 16, 1, 0, true> stitcher;

#if !__X86_DEVICE__
        std::cout << "Graph init. This does nothing because CDO in boot PDI "
                     "already configures AIE.\n";
        auto gHndl = xrt::graph(xF::gpDhdl, xF::xclbin_uuid, "tr");
        std::cout << "XRT graph opened" << std::endl;
        gHndl.reset();
        gHndl.update("tr.k.in[2]", op_width);
#endif

        START_TIMER
        tiler.compute_metadata(srcImageR.size());
        STOP_TIMER("Meta data compute time")

        std::chrono::microseconds tt(0);
        for (int i = 0; i < iterations; i++) {
            //@{
            std::cout << "Iteration : " << (i + 1) << std::endl;
            START_TIMER
            auto tiles_sz = tiler.host2aie_nb(srcData.data(), srcImageR.size(), {"tr.in1"});
#if !__X86_DEVICE__
            std::cout << "Graph running for " << (tiles_sz[0] * tiles_sz[1]) << " iterations.\n";
            gHndl.run(tiles_sz[0] * tiles_sz[1]);
#endif
            stitcher.aie2host_nb(dstData.data(), dst.size(), tiles_sz, {"tr.out1"});

            tiler.wait({"tr.in1"});
#if !__X86_DEVICE__
            gHndl.wait();
#endif
            stitcher.wait({"tr.out1"});
            STOP_TIMER("resize function")
            std::cout << "Data transfer complete (Stitcher)\n";
            tt += tdiff;
            //@}

            // Analyze output {
            std::cout << "Analyzing diff\n";
            cv::Mat diff(op_height, op_width, CV_8UC1);
            cv::absdiff(dstRefImage, dst, diff);
            cv::imwrite("ref.png", dstRefImage);
            cv::imwrite("aie.png", dst);
            cv::imwrite("diff.png", diff);

            float err_per;
            analyzeDiff(diff, 1, err_per);
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
