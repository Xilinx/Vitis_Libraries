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
        std::cout << srcImageR.rows << std::endl;
        std::cout << srcImageR.cols << std::endl;
        std::cout << srcImageR.elemSize() << std::endl;
        std::cout << srcImageR.type() << std::endl;
        std::cout << "Image size (end)" << std::endl;
        int op_width = IMAGE_WIDTH_OUT;
        int op_height = IMAGE_HEIGHT_OUT;

        uint32_t scale_x_fix = compute_scalefactor<16>(IMAGE_WIDTH_IN, IMAGE_WIDTH_OUT);
        uint32_t scale_y_fix = compute_scalefactor<16>(IMAGE_HEIGHT_IN, IMAGE_HEIGHT_OUT);
        //////////////////////////////////////////
        // Run opencv reference test (filter2D design)
        //////////////////////////////////////////
        cv::Mat srcImageRresize, srcImageRalpha;
        cv::resize(srcImageR, srcImageRresize, cv::Size(op_width, op_height));
        cv::Mat dstRefImage = srcImageRresize;

        // Initializa device
        xF::deviceInit(xclBinName);

        // Load image
        void* srcData = nullptr;
        xrt::bo src_hndl = xrt::bo(xF::gpDhdl, (srcImageR.total() * srcImageR.elemSize()), 0, 0);
        srcData = src_hndl.map();
        // xrtBufferHandle src_hndl = xrtBOAlloc(xF::gpDhdl, (srcImageR.total() * srcImageR.elemSize()), 0, 0);
        // srcData = xrtBOMap(src_hndl);
        memcpy(srcData, srcImageR.data, (srcImageR.total() * srcImageR.elemSize()));

        // Allocate output buffer
        void* dstData = nullptr;
        xrt::bo dst_hndl = xrt::bo(xF::gpDhdl, (op_height * op_width * srcImageR.elemSize()), 0, 0);
        dstData = dst_hndl.map();
        // xrtBufferHandle dst_hndl = xrtBOAlloc(xF::gpDhdl, (op_height * op_width * srcImageR.elemSize()), 0, 0);
        // dstData = xrtBOMap(dst_hndl);
        cv::Mat dst(op_height, op_width, dstRefImage.type(), dstData);

        xF::xfcvDataMoverParams params(srcImageR.size(), cv::Size(op_width, op_height));
        xF::xfcvDataMovers<xF::TILER, uint8_t, TILE_HEIGHT_IN, TILE_WIDTH_IN, 16> tiler(0, 0, false, 4);
        xF::xfcvDataMovers<xF::STITCHER, int8_t, TILE_HEIGHT_OUT, TILE_WIDTH_OUT, 16> stitcher(false);

        std::cout << "Graph init. This does nothing because CDO in boot PDI "
                     "already configures AIE.\n";

#if !__X86_DEVICE__
        auto gHndl = xrt::graph(xF::gpDhdl, xF::xclbin_uuid, "resize");
        std::cout << "XRT graph opened" << std::endl;
        gHndl.reset();
        std::cout << "Graph reset done" << std::endl;
        gHndl.update("resize.k.in[1]", scale_x_fix);
        gHndl.update("resize.k.in[2]", scale_y_fix);
#endif

        START_TIMER
        tiler.compute_metadata(srcImageR.size(), cv::Size(op_width, op_height));
        STOP_TIMER("Meta data compute time")

        std::chrono::microseconds tt(0);
        for (int i = 0; i < iterations; i++) {
            //@{
            std::cout << "Iteration : " << (i + 1) << std::endl;
            START_TIMER
            auto tiles_sz = tiler.host2aie_nb(&src_hndl, srcImageR.size(), params);
            stitcher.aie2host_nb(&dst_hndl, dst.size(), tiles_sz);

#if !__X86_DEVICE__
            std::cout << "Graph run(" << tiles_sz[0] * tiles_sz[1] << ")\n";
            gHndl.run(tiles_sz[0] * tiles_sz[1]);
            gHndl.wait();
#endif

            tiler.wait();
            stitcher.wait();

            STOP_TIMER("resize function")
            std::cout << "Data transfer complete (Stitcher)\n";
            tt += tdiff;
            //@}
        }
#if !__X86_DEVICE__
        gHndl.end(0);
#endif

        // Analyze output {
        std::cout << "Analyzing diff\n";
        cv::Mat diff(op_height, op_width, CV_8UC1);
        int ref = 0, aie = 0;

        for (int ii = 0; ii < dst.rows; ii++) {
            for (int jj = 0; jj < dst.cols; jj++) {
                for (int kk = 0; kk < dst.channels(); kk++) {
                    ref = dstRefImage.data[(ii * dstRefImage.cols * dstRefImage.channels()) +
                                           (jj * dstRefImage.channels()) + kk];
                    aie = dst.data[(ii * dst.cols * dst.channels()) + (jj * dst.channels()) + kk];
                    diff.at<uchar>(ii, jj) = abs(ref - aie);
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

        return 0;

    } catch (std::exception& e) {
        const char* errorMessage = e.what();
        std::cerr << "Exception caught: " << errorMessage << std::endl;
        exit(-1);
    }
}
