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

#include "graph.cpp"
#include <opencv2/opencv.hpp>

/*
 ******************************************************************************
 * Top level executable
 ******************************************************************************
 */

int main(int argc, char** argv) {
    try {
        if (argc != 6) {
            std::stringstream errorMessage;
            errorMessage << argv[0] << " <xclbin> <inputImage1> <input_width> <input_height> <iterations> ";
            std::cerr << errorMessage.str();
            throw std::invalid_argument(errorMessage.str());
        }

        const char* xclBinName = argv[1];
        //////////////////////////////////////////
        // Read image from file and resize
        //////////////////////////////////////////
        cv::Mat srcImage;
        srcImage = cv::imread(argv[2], 1);

        cv::Mat srcImage1 = srcImage(cv::Range(0, srcImage.rows), cv::Range(0, srcImage.cols));
        cvtColor(srcImage, srcImage1, cv::COLOR_BGR2RGBA, 0);

        int width_Y = srcImage1.cols;
        if (argc >= 3) width_Y = atoi(argv[3]);
        int height_Y = srcImage1.rows;
        if (argc >= 4) height_Y = atoi(argv[4]);
        if ((width_Y != srcImage1.cols) || (height_Y != srcImage1.rows))
            cv::resize(srcImage1, srcImage1, cv::Size(width_Y, height_Y));

        int iterations = 1;
        if (argc >= 5) iterations = atoi(argv[5]);

        int op_width = srcImage1.cols;
        int op_height = srcImage1.rows;

        std::cout << "Image1 size" << std::endl;
        std::cout << srcImage1.rows << std::endl;
        std::cout << srcImage1.cols << std::endl;
        std::cout << srcImage1.elemSize() << std::endl;
        std::cout << "Image size (end)" << std::endl;

        //////////////////////////////////////////
        // Run opencv reference test (absdiff design)
        //////////////////////////////////////////

        // coeffs
        int16_t* coeffs = (int16_t*)malloc(16 * sizeof(int16_t) + 3 * 3 * sizeof(int16_t));
        uint16_t* coeffs_awb = (uint16_t*)(coeffs + 16);
        int16_t* coeffs_ccm = coeffs;

        // awb rtps
        int min[4], max[4];
        min[0] = 36;
        min[1] = 25;
        min[2] = 14;
        min[3] = 0;
        max[0] = 172;
        max[1] = 93;
        max[2] = 234;
        max[3] = 0;
        compute_awb_params(coeffs_awb, min, max);

        // c-ref

        // Initializa device
        xF::deviceInit(xclBinName);

        // Load image
        void* srcData1 = nullptr;
        xrtBufferHandle src_hndl1 = xrtBOAlloc(xF::gpDhdl, (srcImage1.total() * srcImage1.elemSize()), 0, 0);
        std::cout << "image xrtBufferHandle done.\n";

        srcData1 = xrtBOMap(src_hndl1);
        memcpy(srcData1, srcImage1.data, (srcImage1.total() * srcImage1.elemSize()));
        std::cout << "memcpy  done.\n";

        std::vector<uint8_t> srcData1_vec;
        srcData1_vec.assign(srcImage1.data, (srcImage1.data + srcImage1.total() * 4));

        // Allocate output buffer
        void* dstData = nullptr;
        xrtBufferHandle dst_hndl =
            xrtBOAlloc(xF::gpDhdl, (op_height * op_width * 4), 0, 0); // '2' for unsigned short type
        dstData = xrtBOMap(dst_hndl);
        cv::Mat dst(op_height, op_width, CV_8UC4, dstData);

        T* ref_out = (T*)malloc(srcImage1.total() * 4);
        awbnorm_colorcorrectionmatrix(srcData1_vec.data(), ref_out, coeffs_awb, coeffs_ccm, op_width, op_height);
        // Allocate output buffer
        cv::Mat dstRefImage(op_height, op_width, CV_8UC4, (void*)ref_out);

        xF::xfcvDataMovers<xF::TILER, uint8_t, TILE_HEIGHT, TILE_WIDTH, VECTORIZATION_FACTOR> tiler1(0, 0, false, 2);
        xF::xfcvDataMovers<xF::STITCHER, uint8_t, TILE_HEIGHT, TILE_WIDTH, VECTORIZATION_FACTOR> stitcher;

        std::cout << "Graph init. This does nothing because CDO in boot PDI already configures AIE.\n";

        ccm_graph.init();
        for (int j = 0; j < 16 + 9; j++) printf("host_coeff: %d \n", (int)coeffs[j]);
        ccm_graph.update(ccm_graph.coeff, coeffs, (16 + 9));
        std::chrono::microseconds tt(0);
        tiler1.compute_metadata(srcImage1.size());
        for (int itr = 0; itr < iterations; itr++) {
            //@{
            START_TIMER
            auto tiles_sz = tiler1.host2aie_nb(src_hndl1, srcImage1.size());
            stitcher.aie2host_nb(dst_hndl, dst.size(), tiles_sz);
            std::cout << "Graph Run(" << tiles_sz[0] * tiles_sz[1] << ")" << std::endl;
            ccm_graph.run(tiles_sz[0] * tiles_sz[1]);
            ccm_graph.wait();

            tiler1.wait();
            stitcher.wait();
            STOP_TIMER("Total time to process frame")
            std::cout << "Data transfer complete (Stitcher)\n";

            tt += tdiff;
        }

        ccm_graph.end();
        //@}
        // Analyze output {
        std::cout << "Analyzing diff";

        cv::Mat diff;
        cv::absdiff(dstRefImage, dst, diff);
        cv::imwrite("ref.png", dstRefImage);
        cv::imwrite("aie.png", dst);
        cv::imwrite("diff.png", diff);

        std::cout << "Average time to process frame : " << (((float)tt.count() * 0.001) / (float)iterations) << " ms"
                  << std::endl;
        std::cout << "Average frames per second : " << (((float)1000000 / (float)tt.count()) * (float)iterations)
                  << " fps" << std::endl;
        float err_per;
        analyzeDiff(diff, 1, err_per);
        if (err_per > 0.0f) {
            std::cerr << "Test failed" << std::endl;
            exit(-1);
        }
        //}
        std::cout << "Test passed" << std::endl;

        return 0;
    } catch (std::exception& e) {
        const char* errorMessage = e.what();
        std::cerr << "Exception caught: " << errorMessage << std::endl;
        exit(-1);
    }
}
