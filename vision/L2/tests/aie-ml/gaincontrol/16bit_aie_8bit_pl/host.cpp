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

//#include <adf/adf_api/XRTConfig.h>
#include <chrono>
#include <common/xf_aie_sw_utils.hpp>
#include <common/xfcvDataMovers.h>
#include <fstream>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <experimental/xrt_kernel.h>
#include <experimental/xrt_graph.h>

#include "config.h"
template <int code>
void gainControlOCV(cv::Mat& input, cv::Mat& output, unsigned char rgain, unsigned char bgain) {
    int height = input.size().height;
    int width = input.size().width;
    typedef uint8_t realSize;
    typedef uint16_t maxSize;
    maxSize pixel;

    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            pixel = (maxSize)input.at<realSize>(i, j); // extracting each pixel
            bool cond1, cond2;
            cond1 = (j % 2 == 0);
            cond2 = (j % 2 != 0);
            if (code == XF_BAYER_RG) {
                if (i % 2 == 0 && cond1)
                    pixel = (maxSize)((pixel * rgain) >> 6);
                else if (i % 2 != 0 && cond2)
                    pixel = (maxSize)((pixel * bgain) >> 6);
            } else if (code == XF_BAYER_GR) {
                if (i % 2 == 0 && cond2)
                    pixel = (maxSize)((pixel * rgain) >> 6);
                else if (i % 2 != 0 && cond1)
                    pixel = (maxSize)((pixel * bgain) >> 6);
            } else if (code == XF_BAYER_BG) {
                if (i % 2 == 0 && cond1)
                    pixel = (maxSize)((pixel * bgain) >> 6);
                else if (i % 2 == 0 && cond2)
                    pixel = (maxSize)((pixel * rgain) >> 6);
            } else if (code == XF_BAYER_GB) {
                if (i % 2 == 0 && cond2)
                    pixel = (maxSize)((pixel * bgain) >> 6);
                else if (i % 2 != 0 && cond1)
                    pixel = (maxSize)((pixel * rgain) >> 6);
            }
            output.at<realSize>(i, j) = cv::saturate_cast<realSize>(pixel); // writing each pixel
        }
    }
}

template <int code>
int run_opencv_ref(cv::Mat& srcImageR, cv::Mat& dstRefImage, uint8_t rgain, uint8_t bgain) {
    gainControlOCV<code>(srcImageR, dstRefImage, rgain, bgain);
    return 0;
}

/*
 ******************************************************************************
 * Top level executable
 ******************************************************************************
 */

int main(int argc, char** argv) {
    try {
        if (argc < 5) {
            std::stringstream errorMessage;
            errorMessage << argv[0] << " <xclbin> <inputImage> <rgain> <bgain> "
                                       "[width] [height] [iterations]";
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
        if (argc >= 6) width = atoi(argv[5]);
        int height = srcImageR.rows;
        if (argc >= 7) height = atoi(argv[6]);

        if ((width != srcImageR.cols) || (height != srcImageR.rows))
            cv::resize(srcImageR, srcImageR, cv::Size(width, height));

        int iterations = 1;
        if (argc >= 8) iterations = atoi(argv[7]);

        std::cout << "Image size" << std::endl;
        std::cout << srcImageR.rows << std::endl;
        std::cout << srcImageR.cols << std::endl;
        std::cout << srcImageR.elemSize() << std::endl;
        std::cout << "Image size (end)" << std::endl;
        int op_width = srcImageR.cols;
        int op_height = srcImageR.rows;

        //////////////////////////////////////////
        // Run opencv reference test (filter2D design)
        //////////////////////////////////////////
        uint8_t rgain = atoi(argv[3]);
        uint8_t bgain = atoi(argv[4]);
        cv::Mat dstRefImage(op_height, op_width, CV_8UC1);
        START_TIMER
        run_opencv_ref<XF_BAYER_RG>(srcImageR, dstRefImage, rgain, bgain);
        STOP_TIMER("OpenCV Ref");

        // Initializa device
        xF::deviceInit(xclBinName);

        // Allocate input buffer
        void* srcData = nullptr;
        xrt::bo src_hndl = xrt::bo(xF::gpDhdl, (srcImageR.total() * srcImageR.elemSize()), 0, 0);
        srcData = src_hndl.map();
        memcpy(srcData, srcImageR.data, (srcImageR.total() * srcImageR.elemSize()));

        // Allocate output buffer
        void* dstData = nullptr;
        xrt::bo* ptr_dstHndl = new xrt::bo(xF::gpDhdl, (op_height * op_width * srcImageR.elemSize()), 0, 0);
        dstData = ptr_dstHndl->map();
        cv::Mat dst(op_height, op_width, srcImageR.type(), dstData);

        xF::xfcvDataMovers<xF::TILER, uint8_t, TILE_HEIGHT, TILE_WIDTH, VECTORIZATION_FACTOR> tiler(0, 0);
        xF::xfcvDataMovers<xF::STITCHER, uint8_t, TILE_HEIGHT, TILE_WIDTH, VECTORIZATION_FACTOR> stitcher;

#if !__X86_DEVICE__
        std::cout << "Graph init. This does nothing because CDO in boot PDI "
                     "already configures AIE.\n";
        auto gHndl = xrt::graph(xF::gpDhdl, xF::xclbin_uuid, "gc");
        std::cout << "XRT graph opened" << std::endl;
        gHndl.reset();
        std::cout << "Graph reset done" << std::endl;
        gHndl.update("gc.k1.in[1]", rgain);
        gHndl.update("gc.k1.in[2]", bgain);
#endif
        START_TIMER
        tiler.compute_metadata(srcImageR.size());
        STOP_TIMER("Meta data compute time")

        std::chrono::microseconds tt(0);
        for (int i = 0; i < iterations; i++) {
            //@{
            std::cout << "Iteration : " << (i + 1) << std::endl;
            START_TIMER
            auto tiles_sz = tiler.host2aie_nb(&src_hndl, srcImageR.size());
            stitcher.aie2host_nb(ptr_dstHndl, dst.size(), tiles_sz);

            std::cout << "Graph run(" << (tiles_sz[0] * tiles_sz[1]) << ")\n";
            gHndl.run(tiles_sz[0] * tiles_sz[1]);

            gHndl.wait();
            tiler.wait();
            stitcher.wait();

            STOP_TIMER("gaincontrol function")
            std::cout << "Data transfer complete (Stitcher)\n";
            tt += tdiff;
            //@}

            // Analyze output {
            std::cout << "Analyzing diff\n";
            cv::Mat diff(op_height, op_width, srcImageR.type());
            cv::absdiff(dstRefImage, dst, diff);
            cv::imwrite("ref.png", dstRefImage);
            cv::imwrite("aie.png", dst);
            cv::imwrite("diff.png", diff);

            float err_per;
            analyzeDiff(diff, 1, err_per);
            if (err_per > 0.0f) {
                std::cerr << "Test failed" << std::endl;
                exit(-1);
            }
            //}
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
