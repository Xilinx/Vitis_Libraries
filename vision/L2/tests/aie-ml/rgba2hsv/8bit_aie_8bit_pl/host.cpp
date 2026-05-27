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

cv::Vec3f RGBtoHSV(const cv::Vec3b& rgb) {
    // std::cout << " r , g, b = " << (unsigned)rgb[0]  << " " << (unsigned)rgb[1] << " " << (unsigned)rgb[2]  << " " <<
    // std::endl;
    float r = rgb[0] / 255.0f; // OpenCV stores BGR, so access red as rgb[2]
    float g = rgb[1] / 255.0f;
    float b = rgb[2] / 255.0f;

    // std::cout << " r , g, b = " << r << " " << g << " " << b << " " << std::endl;

    float maxVal = std::max({r, g, b});
    float minVal = std::min({r, g, b});
    float delta = maxVal - minVal;
    // std::cout << " maxVal , minVal, delta = " << maxVal << " " << minVal << " " << delta << " " << std::endl;

    // Calculate Lightness
    float v = maxVal;
    // std::cout << " l  = " << l  << std::endl;

    // Calculate Saturation
    float s = 0.0f;
    if (delta != 0.0f) {
        s = delta / (maxVal);
    }
    // std::cout << " delta , denom, s = " << delta << " " << (1.0f - std::abs(2.0f * l - 1.0f)) << " " << s << " " <<
    // std::endl;

    // Calculate Hue
    float h = 0.0f;
    if (delta != 0.0f) {
        if (maxVal == r) {
            // std::cout << " (g - b) / delta" << (g - b) / delta << "  " << "std::fmod(((g - b) / delta), 6.0f) = " <<
            // std::fmod(((g - b) / delta), 6.0f) << std::endl;
            h = 60.0f * (g - b) / delta; // std::fmod(((g - b) / delta), 6.0f);
            // std::cout << " h1 = " << h << std::endl;
        } else if (maxVal == g) {
            h = 60.0f * (((b - r) / delta) + 2.0f);
            // std::cout << " h2 = " << h << std::endl;
        } else if (maxVal == b) {
            h = 60.0f * (((r - g) / delta) + 4.0f);
            // std::cout << " h3 = " << h << std::endl;
        }
    }

    if (h < 0.0f) {
        h += 180.0f;
        // std::cout << " h = " << h << std::endl;
    }
    return cv::Vec3f(h, s, v);
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
        cv::Mat srcImageRGB, srcImageBGR;
        srcImageBGR = cv::imread(argv[2], 1);

        int width = srcImageBGR.cols;
        if (argc >= 4) width = atoi(argv[3]);
        int height = srcImageBGR.rows;
        if (argc >= 5) height = atoi(argv[4]);

        if ((width != srcImageBGR.cols) || (height != srcImageBGR.rows))
            cv::resize(srcImageBGR, srcImageBGR, cv::Size(width, height));
        
        cv::cvtColor(srcImageBGR, srcImageRGB, cv::COLOR_BGR2RGB);
        cv::imwrite("src_BGR.png", srcImageBGR);
        cv::imwrite("src_RGB.png", srcImageRGB);

        int iterations = 1;
        if (argc >= 6) iterations = atoi(argv[5]);

        // Convert to 4K RGB
        int width_in = srcImageRGB.cols;
        int height_in = srcImageRGB.rows;
        //cv::resize(srcImageRGB, srcImageRGB, cv::Size(width_in, height_in));
        cv::imwrite("input_rgba_check.png", srcImageBGR);
        std::cout << "Image size of RGB : " << srcImageRGB.cols << " x " << srcImageRGB.rows << " x "
                  << srcImageRGB.channels() << std::endl;

        //////////////////////////////////////////
        // Run opencv reference
        //////////////////////////////////////////

        cv::Mat opencvHSV;
        cv::cvtColor(srcImageRGB, opencvHSV, cv::COLOR_RGB2HSV);
        // Convert OpenCV HLS to displayable format
        cv::Mat opencvHSVDisplay;
        opencvHSV.convertTo(opencvHSVDisplay, CV_8UC3);

        //////////////////////////////////////////
        // Run native reference
        //////////////////////////////////////////

        // Convert the image to HLS manually
        cv::Mat manualHSV(srcImageRGB.rows, srcImageRGB.cols, CV_32FC3);

        for (int i = 0; i < srcImageRGB.rows; ++i) {
            for (int j = 0; j < srcImageRGB.cols; ++j) {
                cv::Vec3b rgbPixel = srcImageRGB.at<cv::Vec3b>(i, j);
                manualHSV.at<cv::Vec3f>(i, j) = RGBtoHSV(rgbPixel);
            }
        }
        // Convert Manual HLS to displayable format
        cv::Mat manualHSVDisplay(srcImageRGB.rows, srcImageRGB.cols, CV_8UC3);
        for (int i = 0; i < manualHSV.rows; ++i) // manualHSV.rows
        {
            for (int j = 0; j < manualHSV.cols; ++j) // manualHSV.cols
            {
                cv::Vec3f hsvPixel = manualHSV.at<cv::Vec3f>(i, j);
                manualHSVDisplay.at<cv::Vec3b>(i, j) = cv::Vec3b(hsvPixel[0] / 2, hsvPixel[1] * 255, hsvPixel[2] * 255);
            }
        }

        //////////////////////////////////////////
        // Run AIE
        //////////////////////////////////////////

        cv::Mat srcImage = srcImageRGB;

       
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
        xF::xfcvDataMovers<xF::TILER, uint8_t, TILE_HEIGHT, TILE_WIDTH, 64, NO_COLS, 16, false> tiler(0, 0, true,
                                                                                                            4);
        xF::xfcvDataMovers<xF::STITCHER, uint8_t, TILE_HEIGHT, TILE_WIDTH, 64, NO_COLS, 16, false> stitcher(
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
            STOP_TIMER("rgb2hls function")

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
        cv::imwrite("aieHSV.png", dst);
        cv::imwrite("opencvHSV.png", opencvHSV);
        cv::imwrite("nativeHSV.png", manualHSVDisplay);

        //////////////////////////////////////////
        // Analyze output
        //////////////////////////////////////////

        cv::Mat diff;
        cv::absdiff(opencvHSVDisplay, dst, diff); // Absolute difference

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

        cv::absdiff(manualHSVDisplay, dst, diff); // Absolute difference

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
