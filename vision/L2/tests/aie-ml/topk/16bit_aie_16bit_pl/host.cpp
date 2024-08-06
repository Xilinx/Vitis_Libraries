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

typedef union value_convert {
    std::uint32_t u;
    float f;
} value_convert_t;

std::uint32_t f_to_u(float data) {
    value_convert_t vc{};
    vc.f = data;
    return vc.u;
}

float u_to_f(std::uint32_t data) {
    value_convert_t vc{};
    vc.u = data;
    return vc.f;
}

float f_to_bf(float data) {
    std::uint32_t u = f_to_u(data);
    u = (u + 0x7fff) & 0xFFFF0000;
    return u_to_f(u);
}

void opencv_ref(cv::Mat src, cv::Mat dst) {
    cv::transpose(src, dst);
    return;
}

void topKIndices(const float* arr, int* indices, int size, int k) {
    if (k <= 0 || k > size) {
        // Handle invalid values of k
        std::cerr << "Invalid value of k\n";
        return;
    }

    // Initialize indices array with values 0 to size-1
    for (int i = 0; i < size; ++i) {
        indices[i] = i;
    }

    // Sort the indices based on the values they point to
    std::sort(indices, indices + size, [&arr](int a, int b) { return arr[a] > arr[b]; });
}

int main(int argc, char** argv) {
    try {
        if (argc < 3) {
            std::stringstream errorMessage;
            errorMessage << argv[0] << " <xclbin> [width] [height] [iterations]";
            std::cerr << errorMessage.str();
            throw std::invalid_argument(errorMessage.str());
        }
        const char* xclBinName = argv[1];
        //////////////////////////////////////////
        // Read image from file and resize
        //////////////////////////////////////////
        // std::ifstream inputFile("input_score_host.txt");

        int width = 1024;
        if (argc >= 3) width = atoi(argv[2]);
        int height = 16;
        if (argc >= 4) height = atoi(argv[3]);

        int iterations = 1;
        if (argc >= 5) iterations = atoi(argv[4]);

        std::cout << "Image size" << std::endl;
        std::cout << width << std::endl;
        std::cout << height << std::endl;
        std::cout << "Image size (end)" << std::endl;

        int op_width = 8;
        int op_height = height;

        int BLOCK_SIZE_fl_in_Bytes = (width * height * sizeof(float));
        int BLOCK_SIZE_uint16_in_Bytes = (width * height * sizeof(uint16_t));

        float* inputData_fl = (float*)malloc(BLOCK_SIZE_fl_in_Bytes);
        float* inputData = (float*)malloc(BLOCK_SIZE_fl_in_Bytes / height);
        uint16_t* inputData_uint16 = (uint16_t*)malloc(BLOCK_SIZE_uint16_in_Bytes);

        for (int i = 0; i < height; i++) {
            for (int j = 0; j < width; j++) {
                float value;
                value = rand();

                inputData_fl[(i * width) + j] = value;

                float temp_f = f_to_bf(value);
                int* temp_int = reinterpret_cast<int*>(&temp_f);
                inputData_uint16[(i * width) + j] = (uint16_t)((*temp_int) >> 16);
            }
        }

        cv::Mat srcImageR(height, width, CV_16UC1);
        for (int i = 0; i < height; i++) {
            for (int j = 0; j < width; j++) {
                srcImageR.data[(i * width * srcImageR.elemSize()) + (j * srcImageR.elemSize()) + 0] =
                    (inputData_uint16[(i * width) + j] & 0x00ff);
                srcImageR.data[(i * width * srcImageR.elemSize()) + (j * srcImageR.elemSize()) + 1] =
                    (inputData_uint16[(i * width) + j] >> 8);
            }
        }

        //////////////////////////////////////////
        // Run opencv reference test (filter2D design)
        //////////////////////////////////////////
        int indices[width * height];
        for (int i = 0; i < height; i++) {
            for (int j = 0; j < width; j++) {
                inputData[j] = inputData_fl[(i * width) + j];
            }
            topKIndices(inputData, indices + (i * width), (width), (op_width));
        }

        // Initializa device
        xF::deviceInit(xclBinName);

        // Load image
        void* srcData = nullptr;
        xrt::bo src_hndl = xrt::bo(xF::gpDhdl, (srcImageR.total() * srcImageR.elemSize()), 0, 0);
        srcData = src_hndl.map();
        memcpy(srcData, srcImageR.data, (srcImageR.total() * srcImageR.elemSize()));

        void* dstData1 = nullptr;
        xrt::bo* ptr_dstHndl1 = new xrt::bo(xF::gpDhdl, (op_height * op_width * srcImageR.elemSize()), 0, 0);
        dstData1 = ptr_dstHndl1->map();
        cv::Mat dst1(op_height, op_width, CV_16UC1, dstData1);

        void* dstData2 = nullptr;
        xrt::bo* ptr_dstHndl2 = new xrt::bo(xF::gpDhdl, (op_height * op_width * srcImageR.elemSize()), 0, 0);
        dstData2 = ptr_dstHndl2->map();
        cv::Mat dst2(op_height, op_width, CV_16UC1, dstData2);

        xF::xfcvDataMovers<xF::TILER, uint16_t, TILE_HEIGHT, TILE_WIDTH_IN, 16> tiler(0, 0);
        xF::xfcvDataMovers<xF::STITCHER, uint16_t, TILE_HEIGHT, TILE_WIDTH_OUT, 16> stitcher1;
        xF::xfcvDataMovers<xF::STITCHER, uint16_t, TILE_HEIGHT, TILE_WIDTH_OUT, 16> stitcher2;

#if !__X86__
        std::cout << "Graph init. This does nothing because CDO in boot PDI "
                     "already configures AIE.\n";
        auto gHndl = xrt::graph(xF::gpDhdl, xF::xclbin_uuid, "topk");
        std::cout << "XRT graph opened" << std::endl;
        gHndl.reset();
#endif

        START_TIMER
        tiler.compute_metadata(srcImageR.size());
        STOP_TIMER("Meta data compute time")

        gHndl.update("topk.k.in[1]", TILE_ELEMENTS_IN);
        gHndl.update("topk.k.in[2]", TILE_ELEMENTS_OUT);
        gHndl.update("topk.k.in[3]", 0);

        std::chrono::microseconds tt(0);
        for (int i = 0; i < iterations; i++) {
            //@{
            std::cout << "Iteration : " << (i + 1) << std::endl;
            START_TIMER
            auto tiles_sz = tiler.host2aie_nb(&src_hndl, srcImageR.size());
            stitcher1.aie2host_nb(ptr_dstHndl1, dst1.size(), tiles_sz);
            stitcher2.aie2host_nb(ptr_dstHndl2, dst2.size(), tiles_sz);

#if !__X86__
            std::cout << "Graph running for " << (tiles_sz[0] * tiles_sz[1]) << " iterations.\n";
            gHndl.run(tiles_sz[0] * tiles_sz[1]);
            gHndl.wait();
#endif

            tiler.wait();
            stitcher1.wait();
            stitcher2.wait();
            STOP_TIMER("TopK function")
            std::cout << "Data transfer complete (Stitcher)\n";
            tt += tdiff;
            //@}
        }
        //}

        // Analyze output
        std::cout << "Analyzing diff\n";
        int temp;
        for (int i = 0; i < dst2.rows; i++) {
            std::cout << " First TopK-" << dst2.cols << " elements in " << srcImageR.cols << std::endl;
            for (int j = 0; j < dst2.cols; j++) {
                temp = ((dst2.data[(i * dst2.cols * dst2.elemSize()) + (j * dst2.elemSize()) + 1]) << 8) |
                       (dst2.data[(i * dst2.cols * dst2.elemSize()) + (j * dst2.elemSize()) + 0]);
                std::cout << "ref = " << indices[(i * srcImageR.cols) + j] << " aie = " << temp << std::endl;
            }
        }

        std::cout << "Test Completed" << std::endl;
        std::cout << "Average time to process frame : " << (((float)tt.count() * 0.001) / (float)iterations) << " ms"
                  << std::endl;
        std::cout << "Average frames per second : " << (((float)1000000 / (float)tt.count()) * (float)iterations)
                  << " fps" << std::endl;

        gHndl.end();
        return 0;

    } catch (std::exception& e) {
        const char* errorMessage = e.what();
        std::cerr << "Exception caught: " << errorMessage << std::endl;
        exit(-1);
    }
}
