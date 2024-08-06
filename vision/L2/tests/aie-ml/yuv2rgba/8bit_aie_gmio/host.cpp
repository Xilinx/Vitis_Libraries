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

void cvtColor_OCVImpl(cv::Mat srcImageY, cv::Mat srcImageUV, cv::Mat dstRefImage) {
    cv::cvtColorTwoPlane(srcImageY, srcImageUV, dstRefImage, cv::COLOR_YUV2RGBA_NV12);
}

/*
 ******************************************************************************
 * Top level executable
 ******************************************************************************
 */

int main(int argc, char** argv) {
    try {
        if (argc < 4) {
            std::stringstream errorMessage;
            errorMessage << argv[0] << " <xclbin> <inputImageY> <inputImageUV> [width] [height] [iterations]";
            std::cerr << errorMessage.str();
            throw std::invalid_argument(errorMessage.str());
        }

        const char* xclBinName = argv[1];
        //////////////////////////////////////////
        // Read image from file and resize
        //////////////////////////////////////////
        cv::Mat srcImageY = cv::imread(argv[2], 0);
        int width_Y = srcImageY.cols;
        if (argc >= 5) width_Y = atoi(argv[4]);
        int height_Y = srcImageY.rows;
        if (argc >= 6) height_Y = atoi(argv[5]);

        if ((width_Y != srcImageY.cols) || (height_Y != srcImageY.rows))
            cv::resize(srcImageY, srcImageY, cv::Size(width_Y, height_Y));

        cv::Mat srcImageUV = cv::imread(argv[3], -1);
        if ((width_Y / 2 != srcImageUV.cols) || (height_Y / 2 != srcImageUV.rows))
            cv::resize(srcImageUV, srcImageUV, cv::Size(width_Y / 2, height_Y / 2));

        int iterations = 1;
        if (argc >= 7) iterations = atoi(argv[6]);

        std::cout << "Image size of UV : " << srcImageUV.cols << "x" << srcImageUV.rows
                  << " (element size: " << srcImageUV.elemSize() << ",total: " << srcImageUV.total()
                  << ", size: " << srcImageUV.size() << ")" << std::endl;
        int width = srcImageUV.cols;
        int height = srcImageUV.rows;

        int op_width = srcImageY.cols;
        int op_height = srcImageY.rows;
        std::cout << "Image size of Y : " << srcImageY.cols << "x" << srcImageY.rows
                  << " (element size: " << srcImageY.elemSize() << ",total: " << srcImageY.total()
                  << ", size: " << srcImageY.size() << ")" << std::endl;

        //////////////////////////////////////////
        // Run opencv reference test (yuv2rgba design)
        //////////////////////////////////////////
        cv::Mat dstRefImage(height_Y, width_Y, CV_8UC4);
        cvtColor_OCVImpl(srcImageY, srcImageUV, dstRefImage);

        // Initializa device
        xF::deviceInit(xclBinName);

        // Load image

        std::cout << "Load image start" << std::endl;

        std::vector<uint8_t> srcDataY;
        srcDataY.assign(srcImageY.data, (srcImageY.data + srcImageY.total() * srcImageY.channels()));
        cv::Mat src1(srcImageY.rows, srcImageY.cols, CV_8UC1, (void*)srcDataY.data());

        std::vector<uint8_t> srcDataUV;
        srcDataUV.assign(srcImageUV.data, (srcImageUV.data + (srcImageUV.total() * 2)));
        cv::Mat srcUV(srcImageUV.rows, srcImageUV.cols, CV_8UC2, (void*)srcDataUV.data());

        std::vector<uint8_t> dstData;
        dstData.assign(srcImageY.rows * srcImageY.cols * 4, 0);
        cv::Mat dst(srcImageY.rows, srcImageY.cols, CV_8UC4, (void*)dstData.data());

        xF::xfcvDataMovers<xF::TILER, uint8_t, TILE_HEIGHT, TILE_WIDTH, VECTORIZATION_FACTOR, 1, 0, true> tiler1(0, 0);
        xF::xfcvDataMovers<xF::TILER, uint8_t, TILE_HEIGHT / 2, TILE_WIDTH / 2, VECTORIZATION_FACTOR, 1, 0, true>
            tiler2(0, 0, 2);
        xF::xfcvDataMovers<xF::STITCHER, uint8_t, TILE_HEIGHT, TILE_WIDTH, VECTORIZATION_FACTOR, 1, 0, true> stitcher(
            4);
        short int tile_width = TILE_WIDTH;
        short int tile_height = TILE_HEIGHT;
#if !__X86_DEVICE__
        std::cout << "Graph init. This does nothing because CDO in boot PDI already configures AIE.\n";
        auto gHndl = xrt::graph(xF::gpDhdl, xF::xclbin_uuid, "mygraph");
        std::cout << "XRT graph opened" << std::endl;
        gHndl.reset();
        std::cout << "Graph reset done" << std::endl;
        gHndl.update("mygraph.k1.in[2]", tile_width);
        gHndl.update("mygraph.k1.in[3]", tile_height);
#endif

        START_TIMER
        tiler1.compute_metadata(srcImageY.size());
        tiler2.compute_metadata(srcImageUV.size());
        STOP_TIMER("Meta data compute time")

        std::chrono::microseconds tt(0);
        for (int i = 0; i < iterations; i++) {
            //@{
            std::cout << "Iteration : " << (i + 1) << std::endl;
            START_TIMER
            auto tiles_sz = tiler1.host2aie_nb(srcDataY.data(), srcImageY.size(), {"mygraph.in1"});
            tiler2.host2aie_nb(srcDataUV.data(), srcImageUV.size(), {"mygraph.in2"});
#if !__X86_DEVICE__
            std::cout << "Graph run(" << (tiles_sz[0] * tiles_sz[1]) << ")\n";
            gHndl.run(tiles_sz[0] * tiles_sz[1]);
// gHndl.wait();
#endif
            stitcher.aie2host_nb(dstData.data(), dst.size(), tiles_sz, {"mygraph.out"});
            std::cout << "Graph run complete\n";
            tiler1.wait({"mygraph.in1"});
            tiler2.wait({"mygraph.in2"});
#if !__X86_DEVICE__
            gHndl.wait();
#endif
            stitcher.wait({"mygraph.out"});
            STOP_TIMER("Total time to process frame")
            std::cout << "Data transfer complete (Stitcher)\n";
            tt += tdiff;
        }

        // Analyze output {
        std::cout << "Analyzing diff\n";
        cv::Mat diff;
        cv::absdiff(dstRefImage, dst, diff);
        cv::imwrite("ref.png", dstRefImage);
        cv::imwrite("aie.png", dst);
        cv::imwrite("diff.png", diff);

        float err_per = 0.0f;
        analyzeDiff(diff, 2, err_per);
        if (err_per > 0.0f) {
            std::cerr << "Test failed" << std::endl;
            exit(-1);
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
