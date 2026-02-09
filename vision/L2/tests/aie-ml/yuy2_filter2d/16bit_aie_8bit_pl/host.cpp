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
#include <fstream>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
//#include <xaiengine.h>
//#include <xrt/experimental/xrt_kernel.h>
#include <experimental/xrt_kernel.h>
#include <experimental/xrt_graph.h>

//#include "graph.cpp"
#define int64 INT164
#define uint64 UINT164
#include <common/xf_aie_sw_utils.hpp>
#include <common/xfcvDataMovers.h>
#include "config.h"

void run_ref(uint8_t* srcImageR, uint8_t* dstRefImage, float coeff[9], int16_t height, int16_t width) {
    float window[9];

    width *= 2;

    for (int i = 0; i < height * width; i++) {
        int row = i / width;
        int col = i % width;

        if (col % 2) {
            dstRefImage[i] = srcImageR[i];
            continue;
        }

        int w = 0;
        for (int j = -1; j <= 1; j++) {
            for (int k = -2; k <= 2; k += 2) {
                int r = std::max(row + j, 0);
                int c = std::max(col + k, 0);
                r = std::min(r, height - 1);
                c = std::min(c, width - 2);
                window[w++] = srcImageR[r * width + c];
            }
        }

        float s = 0;
        for (int j = 0; j < 9; j++) s += window[j] * coeff[j];
        dstRefImage[i] = s;
    }
    return;
}

void cvtColor_RGB2YUY2(cv::Mat& src, cv::Mat& dst) {
    cv::Mat temp;
    cv::cvtColor(src, temp, 83);

    std::vector<uint8_t> v1;
    for (int i = 0; i < src.rows; i++) {
        for (int j = 0; j < src.cols; j++) {
            v1.push_back(temp.at<cv::Vec3b>(i, j)[0]);
            j % 2 ? v1.push_back(temp.at<cv::Vec3b>(i, j)[2]) : v1.push_back(temp.at<cv::Vec3b>(i, j)[1]);
        }
    }

    cv::Mat yuy2(src.rows, src.cols, CV_8UC2);
    memcpy(yuy2.data, v1.data(), src.cols * src.rows * 2);
    dst = yuy2;
}

/*
 ******************************************************************************
 * Top level executable
 ******************************************************************************
 */

void p(uint8_t* arr, int h, int w) {
    for (int i = 0; i < h; i++) {
        for (int j = 0; j < w; j++) std::cout << arr[i * w + j] << " ";
        std::cout << "\n";
    }
    std::cout << "\n";
    std::cout << "\n";
}

int main(int argc, char** argv) {
    try {
        if (argc < 3) {
            std::stringstream errorMessage;
            errorMessage << argv[0] << " <xclbin> <inputImage> [width] [height] [iterations]";
            std::cerr << errorMessage.str();
            throw std::invalid_argument(errorMessage.str());
        }
        std::ofstream err_data;
        const char* xclBinName = argv[1];
        //////////////////////////////////////////
        // Read image from file and resize
        //////////////////////////////////////////
        cv::Mat srcImageR, temp1, temp2;
        temp1 = cv::imread(argv[2], 1);
        if (temp1.data == NULL) {
            std::cout << "Image not read" << std::endl;
        }
        cvtColor_RGB2YUY2(temp1, temp2);
        temp2.convertTo(srcImageR, CV_8UC2);

        int width = srcImageR.cols;
        if (argc >= 4) width = atoi(argv[3]);
        int height = srcImageR.rows;
        if (argc >= 5) height = atoi(argv[4]);

        if ((width != srcImageR.cols) || (height != srcImageR.rows))
            cv::resize(srcImageR, srcImageR, cv::Size(width, height));

        int iterations = 1;
        if (argc >= 6) iterations = atoi(argv[5]);

        std::cout << "Image size" << std::endl;
        std::cout << "Rows : " << srcImageR.rows << std::endl;
        std::cout << "Cols : " << srcImageR.cols << std::endl;
        std::cout << "Channels : " << srcImageR.channels() << std::endl;
        std::cout << "Element size : " << srcImageR.elemSize() << std::endl;
        std::cout << "Total pixels : " << srcImageR.total() << std::endl;
        std::cout << "Type : " << srcImageR.type() << std::endl;
        std::cout << "Image size (end)" << std::endl;
        int op_width = srcImageR.cols;
        int op_height = srcImageR.rows;

        std::cout << "Load xclbin " << std::endl;
        // Initializa device
        xF::deviceInit(xclBinName);

        // Load image
        std::cout << "Loading image...\n";
        void* srcData = nullptr;
        // xrtBufferHandle src_hndl = xrtBOAlloc(xF::gpDhdl, (srcImageR.total() * srcImageR.elemSize()), 0, 0);
        // srcData = xrtBOMap(src_hndl);
        xrt::bo src_hndl = xrt::bo(xF::gpDhdl, (srcImageR.total() * srcImageR.elemSize()), 0, 0);
        // xrt::bo src_hndl = xrt::bo(xF::gpDhdl, 524288, 0, 0 );
        std::cout << "alloc bo \n";
        srcData = src_hndl.map();
        std::cout << "pointer map \n";
        memcpy(srcData, srcImageR.data, (srcImageR.total() * srcImageR.elemSize()));
        std::cout << "[DONE]\n";

        //////////////////////////////////////////
        // Run reference test (yuy2 filter2D design)
        //////////////////////////////////////////
        std::cout << "Starting reference fn...";
        uint8_t* dataRefOut = (uint8_t*)std::malloc(srcImageR.total() * srcImageR.elemSize());

        std::vector<uint8_t> srcData_vec;
        srcData_vec.assign(height * width * 2, 0);
        memcpy(srcData_vec.data(), srcImageR.data, srcImageR.total() * srcImageR.elemSize());

        run_ref((uint8_t*)srcData_vec.data(), dataRefOut, kData, srcImageR.rows, srcImageR.cols);
        std::cout << "[DONE]" << std::endl;

        // Allocate output buffer
        std::cout << "Creating  output buffer...";
        void* dstData = nullptr;
        xrt::bo dst_hndl = xrt::bo(xF::gpDhdl, (srcImageR.total() * srcImageR.elemSize()), 0, 0);
        dstData = dst_hndl.map();
        // xrtBufferHandle dst_hndl = xrtBOAlloc(xF::gpDhdl, (srcImageR.total() * srcImageR.elemSize()), 0, 0);
        // dstData = xrtBOMap(dst_hndl);
        cv::Mat dst(op_height, op_width, srcImageR.type(), dstData);
        std::cout << "[DONE]\n";

        std::cout << "Initialized Tiler & Stitcher.\n";
        xF::xfcvDataMovers<xF::TILER, int16_t, TILE_HEIGHT, TILE_WIDTH, VECTORIZATION_FACTOR> tiler(1, 1);
        xF::xfcvDataMovers<xF::STITCHER, int16_t, TILE_HEIGHT, TILE_WIDTH, VECTORIZATION_FACTOR> stitcher;

#if !__X86_DEVICE__
        auto gHndl = xrt::graph(xF::gpDhdl, xF::xclbin_uuid, "filter_graph");
        gHndl.reset();

        gHndl.update("filter_graph.k1.in[1]", float2fixed_coeff<10, 16>(kData));

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
            stitcher.aie2host_nb(&dst_hndl, dst.size(), tiles_sz);
#if !__X86_DEVICE__
            std::cout << "Graph run(" << (tiles_sz[0] * tiles_sz[1]) << ")\n";
            gHndl.run(tiles_sz[0] * tiles_sz[1]);
            gHndl.wait();
#endif

            tiler.wait();
            stitcher.wait();

            STOP_TIMER("YUY2 Filter2D function")
            std::cout << "Data transfer complete (Stitcher)\n";
            tt += tdiff;

            err_data.open("err.txt");
            // compare result
            {
                std::vector<uint8_t> dstData_vec;
                dstData_vec.assign(dst.data, (dst.data + dst.total()));
                int acceptableError = 1;
                int errCount = 0;
                uint8_t* dataOut = (uint8_t*)dstData_vec.data();
                for (int i = 0; i < srcImageR.cols * srcImageR.rows; i++) {
                    if (abs(dataRefOut[i] - dataOut[i]) > acceptableError) {
                        std::cout << "err at : i=" << i << " err=" << abs(dataRefOut[i] - dataOut[i]) << "="
                                  << unsigned(dataRefOut[i]) << "-" << unsigned(dataOut[i]) << std::endl;
                        err_data << "err at : i=" << i << " err=" << abs(dataRefOut[i] - dataOut[i]) << "="
                                 << unsigned(dataRefOut[i]) << "-" << unsigned(dataOut[i]) << std::endl;
                        errCount++;
                    }
                }
                if (errCount) {
                    std::cout << "Test failed!" << std::endl;
                    exit(-1);
                }
            }
        }
// filter_graph.end();
#if !__X86_DEVICE__
        gHndl.end();
#endif
        err_data.close();
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
