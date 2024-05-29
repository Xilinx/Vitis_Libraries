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
        // OPENCV
        cv::Mat outImage_YUV_openCV(height_RGBA + height_RGBA / 2, width_RGBA, CV_8UC1);
        cv::Mat outImage_Y_openCV(height_RGBA, width_RGBA, CV_8UC1);
        cv::Mat outImage_Ueven_openCV(height_RGBA / 4, width_RGBA / 2, CV_8UC1);
        cv::Mat outImage_Uodd_openCV(height_RGBA / 4, width_RGBA / 2, CV_8UC1);
        cv::Mat outImage_Veven_openCV(height_RGBA / 4, width_RGBA / 2, CV_8UC1);
        cv::Mat outImage_Vodd_openCV(height_RGBA / 4, width_RGBA / 2, CV_8UC1);
        cv::Mat outImage_UV_openCV(height_RGBA / 2, width_RGBA / 2, CV_16UC1);

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

        // Initializa device
        xF::deviceInit(xclBinName);

        // Load image
        void* srcData1 = nullptr;
        xrt::bo src_hndl1 = xrt::bo(xF::gpDhdl, (srcImageRGBA.total() * srcImageRGBA.elemSize()), 0, 0);
        srcData1 = src_hndl1.map();
        memcpy(srcData1, srcImageRGBA.data, (srcImageRGBA.total() * srcImageRGBA.elemSize()));

        // Allocate output buffer
        void* dstData1 = nullptr;
        void* dstData2 = nullptr;
        xrt::bo* dst_hndl1 = new xrt::bo(xF::gpDhdl, (op_height * op_width), 0, 0);     // '2' for unsigned short type
        xrt::bo* dst_hndl2 = new xrt::bo(xF::gpDhdl, (op_height * op_width) / 2, 0, 0); // '2' for unsigned short type

        dstData1 = dst_hndl1->map();
        dstData2 = dst_hndl2->map();
        cv::Mat dst1(op_height, op_width, CV_8UC1, dstData1);
        cv::Mat dst2(op_height / 2, op_width / 2, CV_16UC1, dstData2);

        xF::xfcvDataMovers<xF::TILER, uint8_t, TILE_HEIGHT, TILE_WIDTH, VECTORIZATION_FACTOR> tiler1(0, 0);
        xF::xfcvDataMovers<xF::STITCHER, uint8_t, TILE_HEIGHT, TILE_WIDTH, VECTORIZATION_FACTOR> stitcher1;
        xF::xfcvDataMovers<xF::STITCHER, uint8_t, TILE_HEIGHT / 2, TILE_WIDTH / 2, VECTORIZATION_FACTOR> stitcher2;

        std::cout << "Graph init. This does nothing because CDO in boot PDI already configures AIE.\n";
        uint16_t tile_width = TILE_WIDTH;
        uint16_t tile_height = TILE_HEIGHT;
#if !__X86_DEVICE__
        auto gHndl = xrt::graph(xF::gpDhdl, xF::xclbin_uuid, "mygraph");
        std::cout << "XRT graph opened" << std::endl;
        gHndl.reset();
        std::cout << "Graph reset done" << std::endl;
        gHndl.update("mygraph.k1.in[1]", tile_width);
        gHndl.update("mygraph.k1.in[2]", tile_height);
#endif

        // mygraph.update(mygraph.tile_width, tile_width);
        // mygraph.update(mygraph.tile_height, tile_height);

        START_TIMER
        tiler1.compute_metadata(srcImageRGBA.size());
        STOP_TIMER("Meta data compute time")

        std::chrono::microseconds tt(0);
        for (int i = 0; i < iterations; i++) {
            //@{
            START_TIMER
            std::cout << "Iteration : " << (i + 1) << std::endl;
            auto tiles_sz = tiler1.host2aie_nb(&src_hndl1, srcImageRGBA.size());
            stitcher1.aie2host_nb(dst_hndl1, dst1.size(), tiles_sz);
            stitcher2.aie2host_nb(dst_hndl2, dst2.size(), tiles_sz);

            std::cout << "Graph run(" << (tiles_sz[0] * tiles_sz[1]) << ")\n";

            START_TIMER
#if !__X86_DEVICE__
            for (int i = 0; i < tiles_sz[0] * tiles_sz[1]; i++) {
                std::cout << "Running iteration : " << i << std::endl;
                gHndl.run(1);
                gHndl.wait();
                std::cout << "Done iteration : " << i << std::endl;
            }
#endif
            STOP_TIMER("Total time to process frame")

            std::cout << "Graph run complete\n";

            tiler1.wait();
            std::cout << "Data transfer complete (Tiler)\n";

            stitcher1.wait();
            std::cout << "Stitcher 1 done\n";
            stitcher2.wait();
            STOP_TIMER("Total time to process frame")
            std::cout << "Data transfer complete (Stitcher)\n";
            tt += tdiff;
            //@}
        }

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
        cv::imwrite("ref2.png", outImage_UV_openCV);
        cv::imwrite("aie2.png", dst2);
        cv::imwrite("diff2.png", diff2);

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
