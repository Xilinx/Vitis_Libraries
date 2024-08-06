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

void cvtColor_OCVImpl(cv::Mat srcImage, cv::Mat dstRefImage) {
    cvtColor(srcImage, dstRefImage, cv::COLOR_RGBA2YUV_I420, 0);
}

/*
 ******************************************************************************
 * Top level executable
 ******************************************************************************
 */

int main(int argc, char** argv) {
    try {
        if (argc < 3) {
            std::stringstream errorMessage;
            errorMessage << argv[0] << " <xclbin> <inputImageRGBA> [width] [height] [iterations]";
            std::cerr << errorMessage.str();
            throw std::invalid_argument(errorMessage.str());
        }

        const char* xclBinName = argv[1];
        //////////////////////////////////////////
        // Read image from file and resize
        //////////////////////////////////////////
        cv::Mat srcImageRGB = cv::imread(argv[2], 1);
        cv::Mat srcImageRGBA = srcImageRGB(cv::Range(0, srcImageRGB.rows), cv::Range(0, srcImageRGB.cols));
        cvtColor(srcImageRGB, srcImageRGBA, cv::COLOR_RGB2RGBA, 0);
        int width_RGBA = srcImageRGBA.cols;
        if (argc >= 4) width_RGBA = atoi(argv[3]);
        int height_RGBA = srcImageRGBA.rows;
        if (argc >= 5) height_RGBA = atoi(argv[4]);
        std::cout << width_RGBA << " x " << height_RGBA << std::endl;
        if ((width_RGBA != srcImageRGBA.cols) || (height_RGBA != srcImageRGBA.rows))
            cv::resize(srcImageRGBA, srcImageRGBA, cv::Size(width_RGBA, height_RGBA));

        int iterations = 1;
        if (argc >= 6) iterations = atoi(argv[5]);

        int op_width = srcImageRGBA.cols;
        int op_height = srcImageRGBA.rows;
        std::cout << "Image size of RGBA : " << srcImageRGBA.cols << "x" << srcImageRGBA.rows
                  << " (element size: " << srcImageRGBA.elemSize() << ",total: " << srcImageRGBA.total()
                  << ", size: " << srcImageRGBA.size() << ")" << std::endl;

        //////////////////////////////////////////
        // Run opencv reference test (yuv2rgba design)
        //////////////////////////////////////////
        cv::Mat outImage_YUV_openCV(height_RGBA + height_RGBA / 2, width_RGBA, CV_8UC1);
        cv::Mat outImage_Y_openCV(height_RGBA, width_RGBA, CV_8UC1);
        cv::Mat outImage_Ueven_openCV(height_RGBA / 4, width_RGBA / 2, CV_8UC1);
        cv::Mat outImage_Uodd_openCV(height_RGBA / 4, width_RGBA / 2, CV_8UC1);
        cv::Mat outImage_Veven_openCV(height_RGBA / 4, width_RGBA / 2, CV_8UC1);
        cv::Mat outImage_Vodd_openCV(height_RGBA / 4, width_RGBA / 2, CV_8UC1);
        cv::Mat outImage_UV_openCV(height_RGBA / 2, width_RGBA / 2, CV_8UC2);

        cvtColor_OCVImpl(srcImageRGBA, outImage_YUV_openCV); // CALL OPENCV
        std::cout << "CV impl done" << std::endl;

        // EXTRACT FROM OPENCV outImage_YUV_openCV
        outImage_Y_openCV = outImage_YUV_openCV(cv::Range(0, height_RGBA), cv::Range(0, width_RGBA));
        outImage_Ueven_openCV =
            outImage_YUV_openCV(cv::Range(height_RGBA, height_RGBA + height_RGBA / 4), cv::Range(0, width_RGBA / 2));
        outImage_Uodd_openCV = outImage_YUV_openCV(cv::Range(height_RGBA, height_RGBA + height_RGBA / 4),
                                                   cv::Range(width_RGBA / 2, width_RGBA));
        outImage_Veven_openCV = outImage_YUV_openCV(
            cv::Range(height_RGBA + height_RGBA / 4, height_RGBA + height_RGBA / 2), cv::Range(0, width_RGBA / 2));
        outImage_Vodd_openCV =
            outImage_YUV_openCV(cv::Range(height_RGBA + height_RGBA / 4, height_RGBA + height_RGBA / 2),
                                cv::Range(width_RGBA / 2, width_RGBA));

        // cv::merge((outImage_Ueven_openCV,outImage_Veven_openCV),outImage_UV_openCV);
        int ii = 0;
        for (int i = 0; i < height_RGBA / 4; i++) {
            int jj = 0;
            for (int j = 0; j < (width_RGBA / 2); j = j + 1, jj += 2) {
                outImage_UV_openCV.at<uint8_t>(ii, jj) = outImage_Ueven_openCV.at<uint8_t>(i, j);
                outImage_UV_openCV.at<uint8_t>(ii, jj + 1) = outImage_Veven_openCV.at<uint8_t>(i, j);
            }

            jj = 0;
            ii = ii + 1;
            for (int j = 0; j < (width_RGBA / 2); j = j + 1, jj += 2) {
                outImage_UV_openCV.at<uint8_t>(ii, jj) = outImage_Uodd_openCV.at<uint8_t>(i, j);
                outImage_UV_openCV.at<uint8_t>(ii, jj + 1) = outImage_Vodd_openCV.at<uint8_t>(i, j);
            }
            ii = ii + 1;
        }

        uint16_t tile_width = TILE_WIDTH;
        uint16_t tile_height = TILE_HEIGHT;
        // Initializa device
        xF::deviceInit(xclBinName);

        // Load image
        std::vector<uint8_t> srcData1;
        srcData1.assign(srcImageRGBA.data, (srcImageRGBA.data + srcImageRGBA.total() * 4));
        cv::Mat src1(srcImageRGBA.rows, srcImageRGBA.cols, CV_8UC4, (void*)srcData1.data());

        // Allocate output buffer
        std::vector<uint8_t> dstData1;
        std::vector<uint8_t> dstData2;
        dstData1.assign(srcImageRGBA.rows * srcImageRGBA.cols, 0);
        dstData2.assign((srcImageRGBA.rows * srcImageRGBA.cols) / 2, 0);
        cv::Mat dst1(op_height, op_width, CV_8UC1, (void*)dstData1.data());
        cv::Mat dst2(op_height / 2, op_width / 2, CV_8UC2, (void*)dstData2.data());
        cv::Mat dstOutImage1(op_height, op_width, CV_8UC1);
        cv::Mat dstOutImage2(op_height / 2, op_width / 2, CV_8UC2);

        xF::xfcvDataMovers<xF::TILER, uint8_t, TILE_HEIGHT, TILE_WIDTH, VECTORIZATION_FACTOR, 1, 0, true> tiler1(0, 0,
                                                                                                                 4);
        xF::xfcvDataMovers<xF::STITCHER, uint8_t, TILE_HEIGHT, TILE_WIDTH, VECTORIZATION_FACTOR, 1, 0, true> stitcher1(
            1);
        xF::xfcvDataMovers<xF::STITCHER, uint8_t, TILE_HEIGHT / 2, TILE_WIDTH / 2, VECTORIZATION_FACTOR, 1, 0, true>
            stitcher2(2);

#if !__X86_DEVICE__
        std::cout << "Graph init. This does nothing because CDO in boot PDI already configures AIE.\n";
        auto gHndl = xrt::graph(xF::gpDhdl, xF::xclbin_uuid, "mygraph");
        std::cout << "XRT graph opened" << std::endl;
        gHndl.reset();
        std::cout << "Graph reset done" << std::endl;

        gHndl.update("mygraph.k1.in[1]", tile_width);
        gHndl.update("mygraph.k1.in[2]", tile_height);
#endif

        START_TIMER
        tiler1.compute_metadata(srcImageRGBA.size(), cv::Size(width_RGBA, height_RGBA));
        STOP_TIMER("Meta data compute time")

        std::chrono::microseconds tt(0);
        for (int i = 0; i < iterations; i++) {
            std::cout << "Iteration : " << (i + 1) << std::endl;
            //@{
            START_TIMER
            auto tiles_sz = tiler1.host2aie_nb(srcData1.data(), srcImageRGBA.size(), {"mygraph.in"});

#if !__X86_DEVICE__
            std::cout << "Graph run(" << (tiles_sz[0] * tiles_sz[1]) << ")\n";
            gHndl.run(tiles_sz[0] * tiles_sz[1]);
            std::cout << "Graph run complete\n";
#endif
            stitcher1.aie2host_nb(dstData1.data(), dst1.size(), tiles_sz, {"mygraph.out1"});
            stitcher2.aie2host_nb(dstData2.data(), dst2.size(), tiles_sz, {"mygraph.out2"});
#if !__X86_DEVICE__
            gHndl.wait();
#endif
            stitcher1.wait({"mygraph.out1"});
            stitcher2.wait({"mygraph.out2"});

            STOP_TIMER("Total time to process frame")
            tt += tdiff;

            //@}

            // Analyze output {
            std::cout << "Analyzing diff\n";
            cv::Mat diff1, diff2;
            cv::absdiff(outImage_Y_openCV, dst1, diff1);
            cv::imwrite("ref1.png", outImage_Y_openCV);
            cv::imwrite("aie1.png", dst1);
            cv::imwrite("diff1.png", diff1);

            float err_per1 = 0.0f, err_per2 = 0.0f;
            analyzeDiff(diff1, 2, err_per1);
            if (err_per1 > 0.0f) {
                std::cerr << "Test 1 failed" << std::endl;
                exit(-1);
            }

            cv::absdiff(outImage_UV_openCV, dst2, diff2);
            // cv::imwrite("ref2.png", outImage_UV_openCV);
            // cv::imwrite("aie2.png", dst2);
            // cv::imwrite("diff2.png", diff2);

            // analyzeDiff(diff2, 2, err_per2);
            // if (err_per2 > 0.0f) {
            // std::cerr << "Test 2 failed" << std::endl;
            // exit(-1);
            // }
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
