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
#include <experimental/xrt_kernel.h>
#include <experimental/xrt_graph.h>

#include "config.h"
#include "graph.cpp"

int run_opencv_ref(cv::Mat& srcImage1, cv::Mat& srcImage2, cv::Mat& dstRefImage, float alpha) {
    dstRefImage.create(srcImage1.rows, srcImage1.cols, CV_16UC1);
    cv::Mat ocv_ref_in1(srcImage1.rows, srcImage1.cols, CV_32FC1, 1);
    cv::Mat ocv_ref_in2(srcImage1.rows, srcImage1.cols, CV_32FC1, 1);

    srcImage1.convertTo(ocv_ref_in1, CV_32FC1);
    srcImage2.convertTo(ocv_ref_in2, CV_32FC1);

    // OpenCV function
    cv::accumulateWeighted(ocv_ref_in1, ocv_ref_in2, alpha, cv::noArray());
    ocv_ref_in2.convertTo(dstRefImage, CV_16UC1);

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
            errorMessage << argv[0] << " <xclbin> <inputImage1> <inputImage2> <alpha> ";
            std::cerr << errorMessage.str();
            throw std::invalid_argument(errorMessage.str());
        }

        const char* xclBinName = argv[1];
        //////////////////////////////////////////
        // Read image from file and resize
        //////////////////////////////////////////
        cv::Mat srcImage1, srcImage2;
        srcImage1 = cv::imread(argv[2], 0);
        srcImage2 = cv::imread(argv[3], 0);

        int width = srcImage1.cols;
        if (argc >= 6) width = atoi(argv[5]);

        int height = srcImage1.rows;
        if (argc >= 7) height = atoi(argv[6]);

        int iterations = 1;
        if (argc >= 8) iterations = atoi(argv[7]);

        // Resize image if need be
        if ((width != srcImage1.cols) || (height != srcImage1.rows))
            cv::resize(srcImage1, srcImage1, cv::Size(width, height));

        if ((width != srcImage2.cols) || (height != srcImage2.rows))
            cv::resize(srcImage2, srcImage2, cv::Size(width, height));

        std::cout << "Image1 size" << std::endl;
        std::cout << srcImage1.rows << std::endl;
        std::cout << srcImage1.cols << std::endl;
        std::cout << srcImage1.elemSize() << std::endl;
        std::cout << "Image2 size" << std::endl;
        std::cout << srcImage2.rows << std::endl;
        std::cout << srcImage2.cols << std::endl;
        std::cout << srcImage2.elemSize() << std::endl;
        std::cout << "Image size (end)" << std::endl;
        int op_width = srcImage1.cols;
        int op_height = srcImage1.rows;

        ////////////////////////////////////////////////////////////
        // Run opencv reference test (accumulate weighted design)
        ////////////////////////////////////////////////////////////
        cv::Mat dstRefImage;
        float alpha = atof(argv[4]);
        run_opencv_ref(srcImage1, srcImage2, dstRefImage, alpha);
        std::cout << "reference done.\n";
        // Initializa device
        xF::deviceInit(xclBinName);

        std::vector<uint8_t> srcData1;
        srcData1.assign(srcImage1.data, (srcImage1.data + srcImage1.total()));
        cv::Mat src1(srcImage1.rows, srcImage1.cols, CV_8UC1, (void*)srcData1.data());

        std::vector<uint8_t> srcData2;
        srcData2.assign(srcImage2.data, (srcImage2.data + srcImage2.total()));
        cv::Mat src2(srcImage2.rows, srcImage2.cols, CV_8UC1, (void*)srcData2.data());

        std::vector<uint16_t> dstData;
        dstData.assign(op_height * op_width * 2, 0);
        cv::Mat dst(op_height, op_width, CV_16UC1, (void*)dstData.data());

        xF::xfcvDataMovers<xF::TILER, uint8_t, TILE_HEIGHT, TILE_WIDTH, VECTORIZATION_FACTOR, 1, 0, true> tiler1(0, 0);
        xF::xfcvDataMovers<xF::TILER, uint8_t, TILE_HEIGHT, TILE_WIDTH, VECTORIZATION_FACTOR, 1, 0, true> tiler2(0, 0);
        xF::xfcvDataMovers<xF::STITCHER, uint16_t, TILE_HEIGHT, TILE_WIDTH, VECTORIZATION_FACTOR, 1, 0, true> stitcher;

#if !__X86_DEVICE__
        std::cout << "Graph init. This does nothing because CDO in boot PDI "
                     "already configures AIE.\n";
        auto gHndl = xrt::graph(xF::gpDhdl, xF::xclbin_uuid, "accumw_graph");
        std::cout << "XRT graph opened" << std::endl;
        gHndl.reset();
        std::cout << "Graph reset done" << std::endl;
        gHndl.update("accumw_graph.k1.in[2]", alpha);

#endif
        std::chrono::microseconds tt(0);
        for (int i = 0; i < iterations; i++) {
            //@{
            std::cout << "Iteration : " << (i + 1) << std::endl;
            START_TIMER
            tiler1.compute_metadata(srcImage1.size());
            tiler2.compute_metadata(srcImage1.size());
            STOP_TIMER("Meta data compute time")

            //@{
            START_TIMER
            auto tiles_sz = tiler1.host2aie_nb(srcData1.data(), srcImage1.size(), {"accumw_graph.in1"});
            tiler2.host2aie_nb(srcData2.data(), srcImage2.size(), {"accumw_graph.in2"});
// START_TIMER
#if !__X86_DEVICE__
            std::cout << "Graph running for " << (tiles_sz[0] * tiles_sz[1]) << " iterations.\n";
            gHndl.run(tiles_sz[0] * tiles_sz[1]);
#endif
            stitcher.aie2host_nb(dstData.data(), dst.size(), tiles_sz, {"accumw_graph.out1"});

            tiler1.wait({"accumw_graph.in1"});
            tiler2.wait({"accumw_graph.in2"});
#if !__X86_DEVICE__
            gHndl.wait();
#endif
            std::cout << "Data transfer complete (Tiler)\n";
            // STOP_TIMER("Total time to process frame")

            stitcher.wait({"accumw_graph.out1"});
            STOP_TIMER("Total time to process frame")
            std::cout << "Data transfer complete (Stitcher)\n";
            tt += tdiff;
        }
        //@}

        // Analyze output {
        std::cout << "Analyzing diff\n";
        cv::Mat diff;
        cv::absdiff(dstRefImage, dst, diff);
        cv::imwrite("ref.jpg", dstRefImage);
        cv::imwrite("aie.jpg", dst);
        // cv::imwrite("aie_png.png", dst);

        cv::imwrite("diff.jpg", diff);

        float err_per;
        analyzeDiff(diff, 1, err_per);
        if (err_per > 0.0f) {
            std::cerr << "Test failed" << std::endl;
            exit(-1);
        }
//}
#if !__X86_DEVICE__
        gHndl.end(0);
#endif
        std::cout << "Average time to process frame : " << (((float)tt.count() * 0.001) / (float)iterations) << " ms"
                  << std::endl;
        std::cout << "Average frames per second : " << (((float)1000000 / (float)tt.count()) * (float)iterations)
                  << " fps" << std::endl;

        std::cout << "Test passed" << std::endl;
        return 0;
    } catch (std::exception& e) {
        const char* errorMessage = e.what();
        std::cerr << "Exception caught: " << errorMessage << std::endl;
        exit(-1);
    }
}
