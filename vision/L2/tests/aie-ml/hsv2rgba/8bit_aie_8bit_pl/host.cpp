/*
 * Copyright 1986-2022 Xilinx, Inc. All Rights Reserved.
 * Copyright 2023-2025 Advanced Micro Devices, Inc. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
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

/*
 ******************************************************************************
 * Top level executable
 ******************************************************************************
 */

cv::Vec3b HSVtoRGB(const cv::Vec3b& hsv) {
    // std::cout << " r , g, b = " << (unsigned)rgb[0]  << " " << (unsigned)rgb[1] << " " << (unsigned)rgb[2]  << " " <<
    // std::endl;
    unsigned char h = hsv[0];
    unsigned char s = hsv[1];
    unsigned char v = hsv[2];
    
    unsigned char r, g, b;
    if (s == 0) {
        r = g = b = v;
        return cv::Vec3b(r, g, b);
    }

    // find the region
    unsigned char region = h/30; //divided the 180 into six parts
    unsigned char remainder = (h % 30) * 6; 

    unsigned char p = (v * (255 - s)) / 255;
    unsigned char q = (v * (255 - (s * remainder) / 255)) / 255;
    unsigned char t = (v * (255 - (s * (255 - remainder)) / 255)) / 255;

    switch (region) {
        case 0:  r = v; g = t; b = p; break;
        case 1:  r = q; g = v; b = p; break;
        case 2:  r = p; g = v; b = t; break;
        case 3:  r = p; g = q; b = v; break;
        case 4:  r = t; g = p; b = v; break;
        default: r = v; g = p; b = q; break;
    }

    return cv::Vec3b(b, g, r);
}

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
        cv::Mat srcImageHSV;
        srcImageHSV = cv::imread(argv[2], 1);

        int width = srcImageHSV.cols;
        if (argc >= 4) width = atoi(argv[3]);
        int height = srcImageHSV.rows;
        if (argc >= 5) height = atoi(argv[4]);

        if ((width != srcImageHSV.cols) || (height != srcImageHSV.rows))
            cv::resize(srcImageHSV, srcImageHSV, cv::Size(width, height));

        int iterations = 1;
        if (argc >= 6) iterations = atoi(argv[5]);

        // Convert to 4K RGB
        cv::imwrite("input_rgba_check.png", srcImageHSV);
        std::cout << "Image size of RGB : " << srcImageHSV.cols << " x " << srcImageHSV.rows << " x "
                  << srcImageHSV.channels() << std::endl;

        //////////////////////////////////////////
        // Run opencv reference
        //////////////////////////////////////////

        cv::Mat opencvRGB;
        cv::cvtColor(srcImageHSV, opencvRGB, cv::COLOR_HSV2BGR);
        // Convert OpenCV HSV to displayable format
        cv::Mat opencvRGBDisplay;
        opencvRGB.convertTo(opencvRGBDisplay, CV_8UC3);

        //////////////////////////////////////////
        // Run native reference
        //////////////////////////////////////////

        // Convert the image to HSV manually
        cv::Mat manualRGB(srcImageHSV.rows, srcImageHSV.cols, CV_8UC3);

        for (int i = 0; i < srcImageHSV.rows; ++i) {
            for (int j = 0; j < srcImageHSV.cols; ++j) {
                cv::Vec3b hsvPixel = srcImageHSV.at<cv::Vec3b>(i, j);
                manualRGB.at<cv::Vec3b>(i, j) = HSVtoRGB(hsvPixel);
            }
        }

        //////////////////////////////////////////
        // Run AIE
        //////////////////////////////////////////
        cv::Mat srcImage = srcImageHSV;
       
        // Initializa device
        xF::deviceInit(xclBinName);

        // Load image
        void* srcData = nullptr;
        std::cout << "src_hndl size" << (srcImage.total() * srcImage.elemSize()) << std::endl;
        xrt::bo src_hndl = xrt::bo(xF::gpDhdl, (srcImage.total() * srcImage.elemSize()), 0, 0);
        srcData = src_hndl.map();
        memcpy(srcData, srcImage.data, (srcImage.total() * srcImage.elemSize()));

        // Allocate output buffer
        void* dstData;
        xrt::bo dst_hndl =
            xrt::bo(xF::gpDhdl, (srcImage.cols * srcImage.rows * (CHANNELS - 1) * srcImage.elemSize()), 0, 0);
        dstData = dst_hndl.map();
        cv::Mat dst(srcImage.rows, srcImage.cols, CV_8UC3, (void*)dstData);
        xF::xfcvDataMoverParams params(cv::Size(srcImage.cols, srcImage.rows),
                                       cv::Size(srcImage.cols, srcImage.rows));
        xF::xfcvDataMovers<xF::TILER, uint8_t, TILE_HEIGHT, TILE_WIDTH, 64, NO_COLS, 16, false, false> tiler(0, 0, true,
                                                                                                            4);
        xF::xfcvDataMovers<xF::STITCHER, uint8_t, TILE_HEIGHT, TILE_WIDTH, 64, NO_COLS, 16, false, false> stitcher(
            true);

        std::cout << "Graph init. This does nothing because CDO in boot PDI "
                     "already configures AIE.\n";

#if !__X86_DEVICE__
        std::vector<xrt::graph> gHndl;
        for (int k = 0; k < NO_COLS; k++) {
            std::string graph_name = "mygraph[" + std::to_string(k) + "]";
            std::cout << graph_name << std::endl;
            gHndl.push_back(xrt::graph(xF::gpDhdl, xF::xclbin_uuid, graph_name));
            std::cout << "XRT graph opened" << std::endl;
            gHndl.back().reset();
            std::cout << "Graph reset done" << std::endl;
            
        }
#endif

        START_TIMER
        tiler.compute_metadata(srcImage.size(), cv::Size(srcImage.cols, srcImage.rows));
        STOP_TIMER("Meta data compute time i ")

        std::chrono::microseconds tt(0);
        for (int i = 0; i < iterations; i++) {
            //@{
            std::cout << std::endl;
            std::cout << " ****  Iteration : **** " << (i + 1) << std::endl;
            START_TIMER
            auto tiles_sz = tiler.host2aie_nb(&src_hndl, srcImage.size(), params);
            stitcher.aie2host_nb(&dst_hndl, dst.size(), tiles_sz);

#if !__X86_DEVICE__

            for (int i = 0; i < NO_COLS; i++) {
                std::cout << "Graph run(" << tiler.tilesPerCore(i) / NO_CORES_PER_COL << ")\n";

                gHndl[i].run(tiler.tilesPerCore(i) / NO_CORES_PER_COL);
            }
            for (int i = 0; i < NO_COLS; i++) {
                gHndl[i].wait();
            }

#endif

            tiler.wait();
            stitcher.wait();
            STOP_TIMER("hsv2rgb function")

            std::cout << "Data transfer complete (Stitcher) \n";
            tt += tdiff;
            //@}
        }
#if !__X86_DEVICE__
        for (int i = 0; i < NO_COLS; i++) {
            gHndl[i].end(0);
        }
#endif
        std::cout << "Image size of dst : " << dst.cols << " x " << dst.rows << " x " << dst.channels() << " "
                  << std::endl;
        cv::cvtColor(dst, dst, cv::COLOR_RGB2BGR);
        cv::imwrite("aieRGB.png", dst);
        cv::imwrite("opencvRGB.png", opencvRGB);
        cv::imwrite("nativeRGB.png", manualRGB);

        //////////////////////////////////////////
        // Analyze output
        //////////////////////////////////////////

        cv::Mat diff;
        cv::absdiff(opencvRGBDisplay, dst, diff); // Absolute difference
        // Split into individual channels
        std::vector<cv::Mat> channels;
        cv::split(diff, channels);

        // Find the max pixel value in each channel
        double maxValR, maxValG, maxValB;
        cv::minMaxLoc(channels[0], nullptr, &maxValB); // Blue channel
        cv::minMaxLoc(channels[1], nullptr, &maxValG); // Green channel
        cv::minMaxLoc(channels[2], nullptr, &maxValR); // Red channel

        // Find the overall maximum
        double maxAbsoluteDifference = std::max({maxValB, maxValG, maxValR});

        cv::absdiff(manualRGB, dst, diff); // Absolute difference

        // Split into individual channels
        cv::split(diff, channels);

        // Find the max pixel value in each channel
        cv::minMaxLoc(channels[0], nullptr, &maxValB); // Blue channel
        cv::minMaxLoc(channels[1], nullptr, &maxValG); // Green channel
        cv::minMaxLoc(channels[2], nullptr, &maxValR); // Red channel

        // Find the overall maximum
        double maxAbsoluteDifference1 = std::max({maxValB, maxValG, maxValR});

        //////////////////////////////////////////
        // Print results
        //////////////////////////////////////////

        std::cout << std::endl;
        std::cout << " **************************** " << std::endl;
        std::cout << " ****    TEST RESULTS    **** " << std::endl;
        std::cout << " **************************** " << std::endl;

        std::cout << "Maximum absolute difference between opencv and aie images: " << maxAbsoluteDifference
                  << std::endl;
        std::cout << "Maximum absolute difference between native and aie images: " << maxAbsoluteDifference1
                  << std::endl;
        std::cout << "Average time to process frame : " << (((float)tt.count() * 0.001) / (float)iterations) << " ms"
                  << std::endl;
        std::cout << "Average frames per second : " << (((float)1000000 / (float)tt.count()) * (float)iterations)
                  << " fps" << std::endl;

        std::this_thread::sleep_for(std::chrono::seconds(2));
        std::cout << std::endl;

        return 0;

    } catch (std::exception& e) {
        const char* errorMessage = e.what();
        std::cerr << "Exception caught: " << errorMessage << std::endl;
        exit(-1);
    }
}
