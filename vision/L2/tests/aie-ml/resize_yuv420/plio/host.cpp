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

#include <chrono>
#include <common/xf_aie_sw_utils.hpp>
#include <common/xfcvDataMovers.h>
#include <fstream>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <experimental/xrt_kernel.h>
#include <experimental/xrt_graph.h>

#include "opencv2/opencv.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgcodecs/imgcodecs.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include <opencv2/imgproc/types_c.h>

#include "config.h"

void cvtColor_OCVImpl(cv::Mat srcImage, cv::Mat dstRefImage) {
    cvtColor(srcImage, dstRefImage, cv::COLOR_RGBA2YUV_I420, 0);
}
void separateUV(const cv::Mat& UVimage, cv::Mat& Uimage, cv::Mat& Vimage) {
    // Check if the input UV image has exactly two channels
    if (UVimage.channels() != 2) {
        std::cerr << "UVimage must have 2 channels!" << std::endl;
        return;
    }

    // Split the 2-channel UV image into separate U and V channels
    std::vector<cv::Mat> channels(2);
    cv::split(UVimage, channels); // Split into U and V

    // The first channel corresponds to U, and the second one to V
    Uimage = channels[0];
    Vimage = channels[1];
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

cv::Mat combineUV(const cv::Mat& U, const cv::Mat& V) {
    // Ensure the input U and V images have the same size
    if (U.size() != V.size()) {
        throw std::invalid_argument("U and V images must have the same size.");
    }

    // Create an empty CV_8UC2 image to store the interleaved UV data
    cv::Mat UV(U.rows, U.cols, CV_8UC2);

    // Loop through each pixel and interleave U and V into UV
    for (int i = 0; i < U.rows; i++) {
        for (int j = 0; j < U.cols; j++) {
            // Get U and V values at the current pixel
            uchar u_val = U.at<uchar>(i, j);
            uchar v_val = V.at<uchar>(i, j);

            // Set the UV pixel (U is the first channel, V is the second)
            UV.at<cv::Vec2b>(i, j) = cv::Vec2b(u_val, v_val);
        }
    }

    return UV; // Return the combined UV image
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
            errorMessage << argv[0] << " <xclbin> <inputImageRGB>  [width] [height] [iterations]";
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
        cv::imwrite("input_rgba_check.png", srcImageRGB);
        std::cout << "Image size of RGB : " << srcImageRGB.cols << " x " << srcImageRGB.rows << " x "
                  << srcImageRGB.channels() << std::endl;

        // RGB2RGBA
        cv::Mat srcImageRGBA = srcImageRGB(cv::Range(0, srcImageRGB.rows), cv::Range(0, srcImageRGB.cols));
        cvtColor(srcImageRGB, srcImageRGBA, cv::COLOR_RGB2RGBA, 0);
        std::cout << "Image size of RGBA : " << srcImageRGBA.cols << " x " << srcImageRGBA.rows << " x "
                  << srcImageRGBA.channels() << std::endl;

        //////////////////////////////////////////
        // Convert 4K RGBA to YUV
        //////////////////////////////////////////
        int width_RGBA = srcImageRGBA.cols;
        int height_RGBA = srcImageRGBA.rows;

        cv::Mat outImage_YUV_openCV(height_RGBA + height_RGBA / 2, width_RGBA, CV_8UC1);
        cv::Mat outImage_Y_openCV(height_RGBA, width_RGBA, CV_8UC1);
        cv::Mat outImage_Ueven_openCV(height_RGBA / 4, width_RGBA / 2, CV_8UC1);
        cv::Mat outImage_Uodd_openCV(height_RGBA / 4, width_RGBA / 2, CV_8UC1);
        cv::Mat outImage_Veven_openCV(height_RGBA / 4, width_RGBA / 2, CV_8UC1);
        cv::Mat outImage_Vodd_openCV(height_RGBA / 4, width_RGBA / 2, CV_8UC1);
        cv::Mat outImage_UV_openCV(height_RGBA / 2, width_RGBA / 2, CV_8UC2);

        cvtColor_OCVImpl(srcImageRGBA,
                         outImage_YUV_openCV); // CALL OPENCV        std::cout << "CV impl done" << std::endl;

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

        std::cout << "Image size of Y : " << outImage_Y_openCV.cols << " x " << outImage_Y_openCV.rows << " x "
                  << outImage_Y_openCV.channels() << std::endl;
        std::cout << "Image size of UV : " << outImage_UV_openCV.cols << " x " << outImage_UV_openCV.rows << " x "
                  << outImage_UV_openCV.channels() << std::endl;

        //////////////////////////////////////////
        // Run opencv reference test
        //////////////////////////////////////////
        cv::Mat srcImageY = outImage_Y_openCV;
        cv::Mat srcImageUV = outImage_UV_openCV;

        int width_out_y = IMAGE_WIDTH_OUT;
        int height_out_y = IMAGE_HEIGHT_OUT;
        int width_out_uv = width_out_y / 2;
        int height_out_uv = height_out_y / 2;

        cv::Mat srcImageYresize, srcImageUresize, srcImageVresize, srcImageUVresize, srcImageU, srcImageV;
        cv::resize(srcImageY, srcImageYresize, cv::Size(width_out_y, height_out_y));
        cv::imwrite("out_resize_y.png", srcImageYresize);
        std::cout << "Image size of resize Y (out_resize_y.png): " << srcImageYresize.cols << "x"
                  << srcImageYresize.rows << "x" << srcImageYresize.channels()
                  << " elem_size = " << srcImageYresize.elemSize() << std::endl;

        // Separate UV into U and V planes
        separateUV(srcImageUV, srcImageU, srcImageV);
        cv::imwrite("srcImageU.png", srcImageU);
        std::cout << "Image size of separate U : " << srcImageU.cols << "x" << srcImageU.rows << "x"
                  << srcImageU.channels() << " elem_size = " << srcImageU.elemSize() << std::endl;
        cv::imwrite("srcImageV.png", srcImageV);
        std::cout << "Image size of separate V : " << srcImageV.cols << "x" << srcImageV.rows << "x"
                  << srcImageV.channels() << " elem_size = " << srcImageV.elemSize() << std::endl;

        cv::resize(srcImageU, srcImageUresize, cv::Size(width_out_uv, height_out_uv));
        cv::resize(srcImageV, srcImageVresize, cv::Size(width_out_uv, height_out_uv));

        std::cout << "Image size of resize separate U : " << srcImageUresize.cols << "x" << srcImageUresize.rows << "x"
                  << srcImageUresize.channels() << " elem_size = " << srcImageUresize.elemSize() << std::endl;
        std::cout << "Image size of resize separate V : " << srcImageVresize.cols << "x" << srcImageVresize.rows << "x"
                  << srcImageVresize.channels() << " elem_size = " << srcImageVresize.elemSize() << std::endl;

        // combine U and V planes
        srcImageUVresize = combineUV(srcImageUresize, srcImageVresize);

        // cv::imwrite("out_resize_uv.png", srcImageUVresize);
        std::cout << "Image size of resize UV  (out_resize_uv.png) : " << srcImageUVresize.cols << "x"
                  << srcImageUVresize.rows << "x" << srcImageUVresize.channels()
                  << " elem_size = " << srcImageUVresize.elemSize() << std::endl;

        cv::Mat srcImageUVresize_disp = convert8UC2To16SC1(srcImageUVresize);
        std::cout << "Image size of UV after 16bit (out_resize_uv.png) : " << srcImageUVresize_disp.cols << " x "
                  << srcImageUVresize_disp.rows << " x " << srcImageUVresize_disp.channels() << " "
                  << " elem_size = " << srcImageUVresize_disp.elemSize() << std::endl;
        cv::imwrite("out_resize_uv.png", srcImageUVresize_disp);

        //////////////////////////////////////////
        // Run on AIE
        //////////////////////////////////////////

        // Initializa device
        xF::deviceInit(xclBinName);

        uint32_t scale_y_fix = compute_scalefactor<16>(IMAGE_HEIGHT_IN, IMAGE_HEIGHT_OUT);
        uint32_t scale_x_fix = compute_scalefactor<16>(IMAGE_WIDTH_IN, IMAGE_WIDTH_OUT);

        std::cout << "scale_y_fix : " << scale_y_fix << std::endl;
        std::cout << "scale_x_fix : " << scale_x_fix << std::endl;

        //////////////////////////////////////////
        // Run on AIE ---- Y RESIZE
        //////////////////////////////////////////
        // Load image
        void* srcData1 = nullptr;
        std::cout << "src_hndl1 size" << (srcImageY.total() * srcImageY.elemSize()) << std::endl;
        xrt::bo src_hndl1 = xrt::bo(xF::gpDhdl, (srcImageY.total() * srcImageY.elemSize()), 0, 0);
        srcData1 = src_hndl1.map();
        memcpy(srcData1, srcImageY.data, (srcImageY.total() * srcImageY.elemSize()));

        // Allocate output buffer
        void* dstData1 = nullptr;
        xrt::bo dst_hndl1 = xrt::bo(xF::gpDhdl, (IMAGE_HEIGHT_OUT * IMAGE_WIDTH_OUT * srcImageY.elemSize()), 0, 0);
        dstData1 = dst_hndl1.map();
        cv::Mat dst1(IMAGE_HEIGHT_OUT, IMAGE_WIDTH_OUT, CV_8UC1, dstData1);

        xF::xfcvDataMoverParams params1(cv::Size(IMAGE_WIDTH_IN, IMAGE_HEIGHT_IN),
                                        cv::Size(IMAGE_WIDTH_OUT, IMAGE_HEIGHT_OUT));
        xF::xfcvDataMovers<xF::TILER, uint8_t, TILE_HEIGHT_IN, TILE_WIDTH_IN, VECTORIZATION_FACTOR, NO_COLS, 16, false>
            tiler1(0, 0, false, 4);
        xF::xfcvDataMovers<xF::STITCHER, uint8_t, TILE_HEIGHT_OUT, TILE_WIDTH_OUT, VECTORIZATION_FACTOR, NO_COLS, 16,
                           false>
            stitcher1(false);

#if !__X86_DEVICE__
        std::vector<xrt::graph> gHndlY;
        std::string graph_name_RTPY[3];
        for (int k = 0; k < NO_COLS; k++) {
            std::string graph_name = "resize_y[" + std::to_string(k) + "]";
            std::cout << graph_name << std::endl;
            gHndlY.push_back(xrt::graph(xF::gpDhdl, xF::xclbin_uuid, graph_name));
            std::cout << "XRT graph opened" << std::endl;
            gHndlY.back().reset();
            std::cout << "Graph reset done" << std::endl;
            for (int i = 0; i < NO_CORES_PER_COL_Y; i++) {
                for (int j = 1; j < 3; j++) {
                    graph_name_RTPY[j] = graph_name + ".k[" + std::to_string(i) + "].in[" + std::to_string(j) + "]";
                    std::cout << graph_name_RTPY[j] << std::endl;
                }
                gHndlY[k].update(graph_name_RTPY[1], scale_x_fix);
                gHndlY[k].update(graph_name_RTPY[2], scale_y_fix);
            }
        }
#endif

        START_TIMER
        tiler1.compute_metadata(srcImageY.size(), cv::Size(IMAGE_WIDTH_OUT, IMAGE_HEIGHT_OUT));
        STOP_TIMER("Meta data compute time")

        std::chrono::microseconds tt1(0);
        for (int i = 0; i < iterations; i++) {
            //@{
            std::cout << std::endl;
            std::cout << " **** Y Iteration : **** " << (i + 1) << std::endl;
            START_TIMER
            auto tiles_sz1 = tiler1.host2aie_nb(&src_hndl1, srcImageY.size(), params1);
            stitcher1.aie2host_nb(&dst_hndl1, dst1.size(), tiles_sz1);

#if !__X86_DEVICE__

            for (int i = 0; i < NO_COLS; i++) {
                std::cout << "Graph run(" << tiler1.tilesPerCore(i) / NO_CORES_PER_COL_Y << ")\n";

                gHndlY[i].run(tiler1.tilesPerCore(i) / NO_CORES_PER_COL_Y);
            }
            for (int i = 0; i < NO_COLS; i++) {
                gHndlY[i].wait();
            }

#endif

            tiler1.wait();
            stitcher1.wait();
            STOP_TIMER("resizeY function")

            std::cout << "Data transfer complete (Stitcher) \n";
            tt1 += tdiff;
            //@}
        }
#if !__X86_DEVICE__
        for (int i = 0; i < NO_COLS; i++) {
            gHndlY[i].end(0);
        }
#endif

        // Write aie output to .png
        cv::imwrite("AIE_Y.png", dst1);

        //////////////////////////////////////////
        // Run on AIE ---- Y RESIZE
        //////////////////////////////////////////

        scale_y_fix = compute_scalefactor<16>(IMAGE_HEIGHT_IN_UV, IMAGE_HEIGHT_OUT_UV);
        scale_x_fix = compute_scalefactor<16>(IMAGE_WIDTH_IN_UV, IMAGE_WIDTH_OUT_UV);

        void* srcData = nullptr;
        std::cout << "src_hndl size" << (srcImageUV.total() * srcImageUV.elemSize()) << std::endl;
        xrt::bo src_hndl = xrt::bo(xF::gpDhdl, (srcImageUV.total() * srcImageUV.elemSize()), 0, 0);
        srcData = src_hndl.map();
        memcpy(srcData, srcImageUV.data, (srcImageUV.total() * srcImageUV.elemSize()));

        // Allocate output buffer
        void* dstData;
        xrt::bo dst_hndl =
            xrt::bo(xF::gpDhdl, (IMAGE_HEIGHT_OUT_UV * IMAGE_WIDTH_OUT_UV * 2 * srcImageUV.elemSize()), 0, 0);
        dstData = dst_hndl.map();
        cv::Mat dst(IMAGE_HEIGHT_OUT_UV, IMAGE_WIDTH_OUT_UV, CV_8UC2, (void*)dstData);
        xF::xfcvDataMoverParams params(cv::Size(IMAGE_WIDTH_IN_UV, IMAGE_HEIGHT_IN_UV),
                                       cv::Size(IMAGE_WIDTH_OUT_UV, IMAGE_HEIGHT_OUT_UV));
        xF::xfcvDataMovers<xF::TILER, uint8_t, TILE_HEIGHT_IN_UV, TILE_WIDTH_IN_UV, 64, NO_COLS, 16, false> tiler(
            0, 0, false, 4); // CHK channels
        xF::xfcvDataMovers<xF::STITCHER, uint8_t, TILE_HEIGHT_OUT_UV, TILE_WIDTH_OUT_UV, 64, NO_COLS, 16, false>
            stitcher(false); // CHK sbm flag

        std::cout << "Graph init. This does nothing because CDO in boot PDI "
                     "already configures AIE.\n";

#if !__X86_DEVICE__
        std::vector<xrt::graph> gHndlUV;
        std::string graph_name_RTP[3];
        for (int k = 0; k < NO_COLS; k++) {
            std::string graph_name = "resize[" + std::to_string(k) + "]";
            std::cout << graph_name << std::endl;
            gHndlUV.push_back(xrt::graph(xF::gpDhdl, xF::xclbin_uuid, graph_name));
            std::cout << "XRT graph opened" << std::endl;
            gHndlUV.back().reset();
            std::cout << "Graph reset done" << std::endl;
            for (int i = 0; i < NO_CORES_PER_COL; i++) {
                for (int j = 1; j < 3; j++) {
                    graph_name_RTP[j] = graph_name + ".k[" + std::to_string(i) + "].in[" + std::to_string(j) + "]";
                    std::cout << graph_name_RTP[j] << std::endl;
                }
                gHndlUV[k].update(graph_name_RTP[1], scale_x_fix);
                gHndlUV[k].update(graph_name_RTP[2], scale_y_fix);
            }
        }
#endif

        START_TIMER
        tiler.compute_metadata(srcImageUV.size(), cv::Size(IMAGE_WIDTH_OUT_UV, IMAGE_HEIGHT_OUT_UV));
        STOP_TIMER("Meta data compute time i ")

        std::chrono::microseconds tt(0);
        for (int i = 0; i < iterations; i++) {
            std::cout << std::endl;
            std::cout << " **** UV Iteration : **** " << (i + 1) << std::endl;

            //@{
            START_TIMER
            auto tiles_sz = tiler.host2aie_nb(&src_hndl, srcImageUV.size(), params);
            stitcher.aie2host_nb(&dst_hndl, dst.size(), tiles_sz);

#if !__X86_DEVICE__

            for (int i = 0; i < NO_COLS; i++) {
                std::cout << "Graph run(" << tiler.tilesPerCore(i) / NO_CORES_PER_COL << ")\n";

                gHndlUV[i].run(tiler.tilesPerCore(i) / NO_CORES_PER_COL);
            }
            for (int i = 0; i < NO_COLS; i++) {
                gHndlUV[i].wait();
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
            gHndlUV[i].end(0);
        }
#endif
        // Write aie output to .png
        cv::Mat DST_AIE = convert8UC2To16SC1(dst);
        // std::cout << "Image size of YUYV after 16bit : " << DST_AIE.cols << " x " << DST_AIE.rows << " x "
        //          << DST_AIE.channels() << " " << std::endl;
        cv::imwrite("AIE_UV.png", DST_AIE);

        //////////////////////////////////////////
        // Analyze Difference of UV
        //////////////////////////////////////////
        // Create an output image for the absolute difference
        cv::Mat absDiffImage;
        cv::absdiff(srcImageUVresize, dst, absDiffImage);

        // Open a text file to write the absolute difference
        std::ofstream outFile("abs_diff_2channel_data.txt");
        if (!outFile.is_open()) {
            std::cerr << "Error: Could not open text file for writing!" << std::endl;
            return -1;
        }
        // Write the image dimensions and type
        outFile << "Width: " << absDiffImage.cols << "\n";
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

        //}
        std::cout << std::endl;
        std::cout << " **************************** " << std::endl;
        std::cout << " ****   UV TEST RESULTS **** " << std::endl;
        std::cout << " **************************** " << std::endl;

        std::cout << "Test passed with max diff = " << maxDifference << std::endl;
        std::cout << "Average time to process frame : " << (((float)tt.count() * 0.001) / (float)iterations) << " ms"
                  << std::endl;
        std::cout << "Average frames per second : " << (((float)1000000 / (float)tt.count()) * (float)iterations)
                  << " fps" << std::endl;

        //////////////////////////////////////////
        // Analyze Difference of Y
        //////////////////////////////////////////
        cv::absdiff(srcImageYresize, dst1, absDiffImage);
        maxDifference = 0;
        // Open a text file to write the absolute difference
        std::ofstream outFile1("abs_diff_Y_channel_data.txt");
        if (!outFile1.is_open()) {
            std::cerr << "Error: Could not open text file for writing!" << std::endl;
            return -1;
        }

        // Write the image dimensions and type
        outFile1 << "Width: " << absDiffImage.cols << "\n";
        outFile1 << "Height: " << absDiffImage.rows << "\n";
        outFile1 << "Channels: " << absDiffImage.channels() << "\n";

        for (int row = 0; row < absDiffImage.rows; ++row) {
            for (int col = 0; col < absDiffImage.cols; ++col) {
                // Access the two-channel pixel value
                uint8_t pixel = absDiffImage.at<uchar>(row, col);
                outFile1 << "(" << static_cast<int>(pixel) << ") ";
                maxDifference = std::max(maxDifference, static_cast<int>(pixel));
            }
        }
        if (maxDifference > 4) {
            std::cerr << "Test failed" << std::endl;
            exit(-1);
        }

        //}
        std::cout << std::endl;
        std::cout << " **************************** " << std::endl;
        std::cout << " ****   Y TEST RESULTS **** " << std::endl;
        std::cout << " **************************** " << std::endl;

        std::cout << "Test passed with max diff = " << maxDifference << std::endl;
        std::cout << "Average time to process frame : " << (((float)tt1.count() * 0.001) / (float)iterations) << " ms"
                  << std::endl;
        std::cout << "Average frames per second : " << (((float)1000000 / (float)tt1.count()) * (float)iterations)
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
