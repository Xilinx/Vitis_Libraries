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

#include <chrono>
#include <common/xf_aie_sw_utils.hpp>
#include <common/xfcvDataMovers.h>
#include <fstream>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <experimental/xrt_kernel.h>
#include <experimental/xrt_graph.h>

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
        void* srcData1 = nullptr;
        void* srcData2 = nullptr;
        std::cout << "Load image start" << std::endl;

        xrt::bo dbl_hndl = xrt::bo(
            xF::gpDhdl, (srcImageUV.total() * srcImageUV.elemSize() + srcImageY.total() * srcImageY.elemSize()), 0, 0);
        xrt::bo sub_bo_hndl_1 = xrt::bo(dbl_hndl, srcImageY.total() * srcImageY.elemSize(), 0);
        xrt::bo sub_bo_hndl_2 =
            xrt::bo(dbl_hndl, srcImageUV.total() * srcImageUV.elemSize(), srcImageY.total() * srcImageY.elemSize());
        std::cout << "Load image dbl_hndl" << std::endl;

        srcData1 = sub_bo_hndl_1.map();
        srcData2 = sub_bo_hndl_2.map();
        std::cout << "Load image dbl_hndl1" << std::endl;

        memcpy(srcData1, srcImageY.data, (srcImageY.total() * srcImageY.elemSize()));
        memcpy(srcData2, srcImageUV.data, (srcImageUV.total() * srcImageUV.elemSize()));
        std::cout << "Load image dbl_hndl2" << std::endl;

        // Allocate output buffer
        void* dstData = nullptr;
        xrt::bo* dst_hndl = new xrt::bo(xF::gpDhdl, (op_height * op_width * srcImageY.elemSize()) * 4, 0, 0);
        dstData = dst_hndl->map();
        cv::Mat dst(op_height, op_width, CV_8UC4, dstData);
        std::cout << "Load image dbl_hndl3" << std::endl;

        xF::xfcvDataMovers<xF::TILER, uint8_t, TILE_HEIGHT, TILE_WIDTH, VECTORIZATION_FACTOR> tiler1(0, 0);
        xF::xfcvDataMovers<xF::TILER, uint8_t, TILE_HEIGHT / 2, TILE_WIDTH / 2, VECTORIZATION_FACTOR> tiler2(0, 0);
        xF::xfcvDataMovers<xF::STITCHER, uint8_t, TILE_HEIGHT, TILE_WIDTH, VECTORIZATION_FACTOR> stitcher;
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
            auto tiles_sz = tiler1.host2aie_nb(&sub_bo_hndl_1, srcImageY.size());
            tiler2.host2aie_nb(&sub_bo_hndl_2, srcImageUV.size());
            stitcher.aie2host_nb(dst_hndl, dst.size(), tiles_sz);
            START_TIMER
#if !__X86_DEVICE__
            std::cout << "Graph running for " << (tiles_sz[0] * tiles_sz[1]) << " iterations.\n";
            for (int i = 0; i < tiles_sz[0] * tiles_sz[1]; i++) {
                std::cout << "Running iteration : " << i << std::endl;
                gHndl.run(1);
                gHndl.wait();
                std::cout << "Done iteration : " << i << std::endl;
            }
#endif
            std::cout << "Graph run complete\n";
            tiler1.wait();
            std::cout << "Tiler1 done\n";
            tiler2.wait();
            std::cout << "Data transfer complete (Tiler)\n";

            stitcher.wait();
            STOP_TIMER("Total time to process frame")
            std::cout << "Data transfer complete (Stitcher)\n";
            tt += tdiff;
            //@}
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
