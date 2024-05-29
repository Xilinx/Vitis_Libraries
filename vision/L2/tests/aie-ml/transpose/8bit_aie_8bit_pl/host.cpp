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
#include <xrt/experimental/xrt_kernel.h>
#include <xrt/experimental/xrt_graph.h>
#include <xrt/experimental/xrt_aie.h>
#include <cmath>
#include <string.h>
#include <vector>

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
        std::cout << "dstRef Image size" << std::endl;
        std::cout << dstRefImage.cols << std::endl;
        std::cout << dstRefImage.rows << std::endl;
        std::cout << "dstRef Image size (end)" << std::endl;
        opencv_ref(srcImageR, dstRefImage);

        // Initializa device
        xF::deviceInit(xclBinName);

        // Load image
        void* srcData = nullptr;
        xrt::bo src_hndl = xrt::bo(xF::gpDhdl, (srcImageR.total() * srcImageR.elemSize()), 0, 0);
        srcData = src_hndl.map();
        memcpy(srcData, srcImageR.data, (srcImageR.total() * srcImageR.elemSize()));

        // Allocate output buffer
        void* dstData = nullptr;
        xrt::bo* ptr_dstHndl = new xrt::bo(xF::gpDhdl, (op_height * op_width * srcImageR.elemSize()), 0, 0);
        dstData = ptr_dstHndl->map();
        cv::Mat dst(op_height, op_width, CV_8UC1, dstData);

        xF::xfcvDataMovers<xF::TILER, uint8_t, TILE_HEIGHT, TILE_WIDTH, 16> tiler(0, 0);
        xF::xfcvDataMovers<xF::STITCHER, uint8_t, TILE_WIDTH, TILE_HEIGHT, 16> stitcher;

#if !__X86_DEVICE__
        std::cout << "Graph init. This does nothing because CDO in boot PDI "
                     "already configures AIE.\n";
        auto gHndl = xrt::graph(xF::gpDhdl, xF::xclbin_uuid, "tr");
        std::cout << "XRT graph opened" << std::endl;
        gHndl.reset();
#endif

        START_TIMER
        tiler.compute_metadata(srcImageR.size());
        STOP_TIMER("Meta data compute time")

        gHndl.update("tr.k.in[2]", op_width);

        std::chrono::microseconds tt(0);
        for (int i = 0; i < iterations; i++) {
            //@{
            std::cout << "Iteration : " << (i + 1) << std::endl;
            START_TIMER
            auto tiles_sz = tiler.host2aie_nb(&src_hndl, srcImageR.size());
            stitcher.aie2host_nb(ptr_dstHndl, dst.size(), tiles_sz);
#if !__X86_DEVICE__
            std::cout << "Graph running for " << (tiles_sz[0] * tiles_sz[1]) << " iterations.\n";
            gHndl.run(tiles_sz[0] * tiles_sz[1]);
            gHndl.wait();
#endif
            tiler.wait();
            stitcher.wait();
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
