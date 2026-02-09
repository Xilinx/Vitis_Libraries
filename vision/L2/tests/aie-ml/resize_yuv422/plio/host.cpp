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

void separateYUYV(const cv::Mat& src, cv::Mat& Y, cv::Mat& U, cv::Mat& V) {
    // Validate input type
    if (src.type() != CV_8UC2) {
        throw std::invalid_argument("Input image must be of type CV_8UC2 (YUYV format)");
    }

    // Create output matrices for Y, U, and V planes
    int rows = src.rows;
    int cols = src.cols * 2; // Each YUYV pixel has 2 values but represents 2 pixels
    // std::cout << " COLS = " << src.cols << std::endl;
    Y = cv::Mat(rows, src.cols, CV_8UC1);     // Luminance plane
    U = cv::Mat(rows, src.cols / 2, CV_8UC1); // Chrominance (U) plane
    V = cv::Mat(rows, src.cols / 2, CV_8UC1); // Chrominance (V) plane

    // Process the YUYV data
    for (int i = 0; i < rows; ++i) {
        // const cv::Vec2b *src_row = src.ptr<cv::Vec2b>(i); // Read the YUYV row
        const uint8_t* src_row = src.ptr<uint8_t>(i);
        uint8_t* Y_row = Y.ptr<uint8_t>(i); // Write to the Y plane
        uint8_t* U_row = U.ptr<uint8_t>(i); // Write to the U plane
        uint8_t* V_row = V.ptr<uint8_t>(i); // Write to the V plane

        for (int j = 0; j < src.cols / 2; ++j) {
            // Extract Y, U, and V values from the YUYV format
            Y_row[j * 2] = src_row[4 * j];         // Y0
            Y_row[j * 2 + 1] = src_row[4 * j + 2]; // Y1
            U_row[j] = src_row[4 * j + 1];         // U (shared between two pixels)
            V_row[j] = src_row[4 * j + 3];         // V (shared between two pixels)
        }
    }
}

cv::Mat combineYUVToYUYV(const cv::Mat& Y, const cv::Mat& U, const cv::Mat& V) {
    // Validate input dimensions
    if (Y.type() != CV_8UC1 || U.type() != CV_8UC1 || V.type() != CV_8UC1) {
        throw std::invalid_argument("Y, U, and V planes must be of type CV_8UC1");
    }
    if (Y.cols != U.cols * 2 || Y.cols != V.cols * 2 || Y.rows != U.rows || Y.rows != V.rows) {
        throw std::invalid_argument("Y, U, and V planes dimensions do not match YUV 4:2:2 format");
    }

    // Create output YUYV image (CV_8UC2)
    cv::Mat YUYV(Y.rows, Y.cols, CV_8UC2);

    // Process the planes and interleave into YUYV format
    for (int i = 0; i < Y.rows; ++i) {
        const uint8_t* Y_row = Y.ptr<uint8_t>(i);
        const uint8_t* U_row = U.ptr<uint8_t>(i);
        const uint8_t* V_row = V.ptr<uint8_t>(i);
        // cv::Vec2b* YUYV_row = YUYV.ptr<cv::Vec2b>(i);
        uint8_t* YUYV_row = YUYV.ptr<uint8_t>(i);

        for (int j = 0; j < Y.cols / 2; ++j) {
            YUYV_row[4 * j] = Y_row[j * 2];         // Y0
            YUYV_row[4 * j + 1] = U_row[j];         // U (shared)
            YUYV_row[4 * j + 2] = Y_row[j * 2 + 1]; // Y1
            YUYV_row[4 * j + 3] = V_row[j];         // V (shared)
        }
    }

    return YUYV;
}

cv::Mat convert8UC2To16SC1(const cv::Mat& src) {
    // Validate input type
    if (src.type() != CV_8UC2) {
        throw std::invalid_argument("Input image must be of type CV_8UC2");
    }

    // Create an output image of type CV_16SC1
    cv::Mat dst(src.rows, src.cols, CV_16SC1);

    // Split the source image into two channels
    std::vector<cv::Mat> channels(2);
    cv::split(src, channels);

    // Combine the two channels into a single 16-bit signed channel
    // Assuming channels[0] is the high byte and channels[1] is the low byte
    for (int i = 0; i < src.rows; ++i) {
        for (int j = 0; j < src.cols; ++j) {
            uint8_t high_byte = channels[0].at<uint8_t>(i, j);
            uint8_t low_byte = channels[1].at<uint8_t>(i, j);

            // Combine high byte and low byte into a single signed 16-bit value
            int16_t value = static_cast<int16_t>((high_byte << 8) | low_byte);

            // Assign to the output matrix
            dst.at<int16_t>(i, j) = value;
        }
    }
    return dst;
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
        cv::Mat srcImageRGB;
        srcImageRGB = cv::imread(argv[2], 1);
        int iterations = 10;

        // Convert to 4K RGB
        int width_in = IMAGE_WIDTH_IN;
        int height_in = IMAGE_HEIGHT_IN;
        cv::resize(srcImageRGB, srcImageRGB, cv::Size(width_in, height_in));
        std::cout << "Image size of srcImageRGB : " << srcImageRGB.cols << " x " << srcImageRGB.rows << " x "
                  << srcImageRGB.channels() << " " << std::endl;
        cv::imwrite("srcImageRGB.png", srcImageRGB);

        //////////////////////////////////////////
        // Convert 4K RGBA to YUV
        //////////////////////////////////////////
        cv::Mat srcImage;
        cvtColor_RGB2YUY2(srcImageRGB, srcImage);
        std::cout << "Image size of YUYV : " << srcImage.cols << " x " << srcImage.rows << " x " << srcImage.channels()
                  << " " << std::endl;

        // For image print only of YUYV
        cv::Mat YUYV_REF_IN = convert8UC2To16SC1(srcImage);
        std::cout << "Image size of YUYV after 16bit : " << YUYV_REF_IN.cols << " x " << YUYV_REF_IN.rows << " x "
                  << YUYV_REF_IN.channels() << " " << std::endl;
        cv::imwrite("srcImage.png", YUYV_REF_IN);

        //////////////////////////////////////////
        // Run opencv reference test (RESIZE YUYV)
        //////////////////////////////////////////
        cv::Mat Y, U, V;

        for (int i = 0; i < srcImage.rows; ++i) { // Print first 10 rows only
            for (int j = 0; j < srcImage.cols; ++j) {
                for (int k = 0; k < srcImage.channels(); k++) {
                    uchar pixelValue = srcImage.at<cv::Vec2b>(i, j)[k];
                }
            }
        }
        separateYUYV(srcImage, Y, U, V);

        /*
        std::cout << "Image size of Y : " << Y.cols << " x " << Y.rows << " x " << Y.channels() << " " << std::endl;
        std::cout << "Image size of U : " << U.cols << " x " << U.rows << " x " << U.channels() << " " << std::endl;
        std::cout << "Image size of V : " << V.cols << " x " << V.rows << " x " << V.channels() << " " << std::endl;
        cv::imwrite("out/Y.png", Y);
        cv::imwrite("out/U.png", U);
        cv::imwrite("out/V.png", V);
        */

        // resize each plane
        cv::resize(Y, Y, cv::Size(IMAGE_WIDTH_OUT, IMAGE_HEIGHT_OUT));
        cv::resize(U, U, cv::Size(IMAGE_WIDTH_OUT / 2, IMAGE_HEIGHT_OUT));
        cv::resize(V, V, cv::Size(IMAGE_WIDTH_OUT / 2, IMAGE_HEIGHT_OUT));
        /*
        std::cout << "Image size of Y : " << Y.cols << " x " << Y.rows << " x " << Y.channels() << " " << std::endl;
        std::cout << "Image size of U : " << U.cols << " x " << U.rows << " x " << U.channels() << " " << std::endl;
        std::cout << "Image size of V : " << V.cols << " x " << V.rows << " x " << V.channels() << " " << std::endl;
        cv::imwrite("out/Y_after_resize.png", Y);
        cv::imwrite("out/U_after_resize.png", U);
        cv::imwrite("out/V_after_resize.png", V);
        */

        // combine the planes
        cv::Mat YUYV_resize_out = combineYUVToYUYV(Y, U, V);
        // std::cout << "Image size of YUYV_resize_out : " << YUYV_resize_out.cols << " x " << YUYV_resize_out.rows
        //           << " x " << YUYV_resize_out.channels() << " " << YUYV_resize_out.type() << std::endl;

        cv::Mat YUYV_REF = convert8UC2To16SC1(YUYV_resize_out);
        // std::cout << "Image size of YUYV after 16bit : " << YUYV_REF.cols << " x " << YUYV_REF.rows << " x "
        //           << YUYV_REF.channels() << " " << std::endl;
        cv::imwrite("YUYV_resize_out.png", YUYV_REF);

        /*
        cv::Mat YUYV_resize_out_16bit = convert8UC2To16SC1(YUYV_resize_out);
        std::cout << "Image size of YUYV resize after 16bit : " << YUYV_resize_out_16bit.cols << " x "
                  << YUYV_resize_out_16bit.rows << " x " << YUYV_resize_out_16bit.channels() << " " << std::endl;
        cv::imwrite("out/YUYV_resize_out.png", YUYV_resize_out_16bit);
        */

        //////////////////////////////////////////
        // Run on AIE
        //////////////////////////////////////////

        uint32_t scale_y_fix = compute_scalefactor<16>(IMAGE_HEIGHT_IN, IMAGE_HEIGHT_OUT);
        uint32_t scale_x_fix = compute_scalefactor<16>(IMAGE_WIDTH_IN, IMAGE_WIDTH_OUT);

        std::cout << "scale_y_fix : " << scale_y_fix << std::endl;
        std::cout << "scale_x_fix : " << scale_x_fix << std::endl;

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
        xrt::bo dst_hndl = xrt::bo(xF::gpDhdl, (IMAGE_HEIGHT_OUT * IMAGE_WIDTH_OUT * 2 * srcImage.elemSize()), 0, 0);
        dstData = dst_hndl.map();
        cv::Mat dst(IMAGE_HEIGHT_OUT, IMAGE_WIDTH_OUT, CV_8UC2, (void*)dstData);
        xF::xfcvDataMoverParams params(cv::Size(IMAGE_WIDTH_IN, IMAGE_HEIGHT_IN),
                                       cv::Size(IMAGE_WIDTH_OUT, IMAGE_HEIGHT_OUT));
        xF::xfcvDataMovers<xF::TILER, uint8_t, TILE_HEIGHT_IN, TILE_WIDTH_IN, 64, NO_COLS, 16, false> tiler(
            0, 0, false, 4); // CHK channels
        xF::xfcvDataMovers<xF::STITCHER, uint8_t, TILE_HEIGHT_OUT, TILE_WIDTH_OUT, 64, NO_COLS, 16, false> stitcher(
            false); // CHK sbm flag

        std::cout << "Graph init. This does nothing because CDO in boot PDI "
                     "already configures AIE.\n";

#if !__X86_DEVICE__
        std::vector<xrt::graph> gHndl;
        std::string graph_name_RTP[3];
        for (int k = 0; k < NO_COLS; k++) {
            std::string graph_name = "resize[" + std::to_string(k) + "]";
            std::cout << graph_name << std::endl;
            gHndl.push_back(xrt::graph(xF::gpDhdl, xF::xclbin_uuid, graph_name));
            std::cout << "XRT graph opened" << std::endl;
            gHndl.back().reset();
            std::cout << "Graph reset done" << std::endl;
            for (int i = 0; i < NO_CORES_PER_COL; i++) {
                for (int j = 1; j < 3; j++) {
                    graph_name_RTP[j] = graph_name + ".k[" + std::to_string(i) + "].in[" + std::to_string(j) + "]";
                    std::cout << graph_name_RTP[j] << std::endl;
                }
                gHndl[k].update(graph_name_RTP[1], scale_x_fix);
                gHndl[k].update(graph_name_RTP[2], scale_y_fix);
            }
        }
#endif

        START_TIMER
        tiler.compute_metadata(srcImage.size(), cv::Size(IMAGE_WIDTH_OUT, IMAGE_HEIGHT_OUT));
        STOP_TIMER("Meta data compute time i ")

        std::chrono::microseconds tt(0);
        for (int i = 0; i < iterations; i++) {
            //@{
            std::cout << std::endl;
            std::cout << " **** Iteration : **** " << (i + 1) << std::endl;
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
            STOP_TIMER("resize function")

            std::cout << "Data transfer complete (Stitcher) \n";
            tt += tdiff;
            //@}
        }
#if !__X86_DEVICE__
        for (int i = 0; i < NO_COLS; i++) {
            gHndl[i].end(0);
        }
#endif
        //////////////////////////////////////////
        // Write AIE output to .png
        //////////////////////////////////////////

        cv::Mat DST_AIE = convert8UC2To16SC1(dst);
        // std::cout << "Image size of YUYV after 16bit : " << DST_AIE.cols << " x " << DST_AIE.rows << " x "
        //          << DST_AIE.channels() << " " << std::endl;
        cv::imwrite("AIE.png", DST_AIE);

        //////////////////////////////////////////
        // Analyze Difference of UV
        //////////////////////////////////////////

        cv::Mat absDiffImage;
        cv::absdiff(YUYV_resize_out, dst, absDiffImage);

        // Open a text file to write the absolute difference
        std::ofstream outFile("abs_diff_2channel_data.txt");
        if (!outFile.is_open()) {
            std::cerr << "Error: Could not open text file for writing!" << std::endl;
            return -1;
        }
        // Write the image dimensions and type
        outFile << "Width = : " << absDiffImage.cols << "\n";
        outFile << "Height: " << absDiffImage.rows << "\n";
        outFile << "Channels: " << absDiffImage.channels() << "\n";
        // Compute the maximum difference
        int maxDifference = 0;
        // Iterate through the pixels
        for (int row = 0; row < absDiffImage.rows; ++row) {
            for (int col = 0; col < absDiffImage.cols; ++col) {
                // Access the two-channel pixel value
                cv::Vec2b pixel = absDiffImage.at<cv::Vec2b>(row, col);
                outFile << "(" << static_cast<int>(pixel[0]) << ", " // Channel 1
                        << static_cast<int>(pixel[1]) << ") ";       // Channel 2

                // Update the maximum difference
                maxDifference = std::max(maxDifference, static_cast<int>(pixel[0]));
                maxDifference = std::max(maxDifference, static_cast<int>(pixel[1]));
            }
            outFile << "\n";
        }

        outFile.close();

        if (maxDifference > 4) {
            std::cerr << "Test failed" << std::endl;
            exit(-1);
        }

        std::cout << std::endl;
        std::cout << " **************************** " << std::endl;
        std::cout << " ******   TEST RESULTS ****** " << std::endl;
        std::cout << " **************************** " << std::endl;

        std::cout << "Test passed with max diff = " << maxDifference << std::endl;
        std::cout << "Average time to process frame : " << (((float)tt.count() * 0.001) / (float)iterations) << " ms"
                  << std::endl;
        std::cout << "Average frames per second : " << (((float)1000000 / (float)tt.count()) * (float)iterations)
                  << " fps" << std::endl;
        std::cout << " **************************** " << std::endl;

        std::this_thread::sleep_for(std::chrono::seconds(2));
        std::cout << std::endl;
        return 0;

    } catch (std::exception& e) {
        const char* errorMessage = e.what();
        std::cerr << "Exception caught: " << errorMessage << std::endl;
        exit(-1);
    }
}
