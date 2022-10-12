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
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <xaiengine.h>
#include <xrt/experimental/xrt_kernel.h>

#include "graph.cpp"

enum XF_demosaicing { XF_BAYER_BG, XF_BAYER_GB, XF_BAYER_GR, XF_BAYER_RG };

template <bool border_reflect>
void demosaicImage(cv::Mat cfa_output, cv::Mat& output_image, int code) {
    int block[5][5];
    for (int i = 0; i < output_image.rows; i++) {
        for (int j = 0; j < output_image.cols; j++) {
            for (int k = -2, ki = 0; k < 3; k++, ki++) {
                for (int l = -2, li = 0; l < 3; l++, li++) {
                    if (border_reflect) {
                        int x = std::min(std::max((i + k), 0), (output_image.rows - 1));
                        int y = std::min(std::max((j + l), 0), (output_image.cols - 1));
                        if (cfa_output.type() == CV_8UC1)
                            block[ki][li] = (int)cfa_output.at<unsigned char>(x, y);
                        else
                            block[ki][li] = (int)cfa_output.at<unsigned short>(x, y);
                    } else {
                        if (i + k >= 0 && i + k < output_image.rows && j + l >= 0 && j + l < output_image.cols) {
                            if (cfa_output.type() == CV_8UC1)
                                block[ki][li] = (int)cfa_output.at<unsigned char>(i + k, j + l);
                            else
                                block[ki][li] = (int)cfa_output.at<unsigned short>(i + k, j + l);
                        } else {
                            block[ki][li] = 0;
                        }
                    }
                }
            }
            cv::Vec3f out_pix;

            if (code == 0) {                     // BG
                if ((i & 0x00000001) == 0) {     // B row
                    if ((j & 0x00000001) == 0) { // B location
                        out_pix[0] = 8 * (float)block[2][2];
                        out_pix[1] = -1.0 * (float)(block[0][2] + block[2][0] + block[2][4] + block[4][2]) +
                                     2.0 * (float)(block[1][2] + block[2][1] + block[2][3] + block[3][2]) +
                                     4.0 * (float)block[2][2];
                        out_pix[2] = -1.5 * (float)(block[0][2] + block[2][0] + block[2][4] + block[4][2]) +
                                     2.0 * (float)(block[1][1] + block[1][3] + block[3][1] + block[3][3]) +
                                     6.0 * (float)block[2][2];
                    } else { // G location
                        out_pix[0] = 0.5 * (float)(block[0][2] + block[4][2]) +
                                     (-1.0) * (float)(block[1][1] + block[1][3] + block[2][0] + block[2][4] +
                                                      block[3][1] + block[3][3]) +
                                     4.0 * (float)(block[2][1] + block[2][3]) + 5.0 * (float)(block[2][2]);
                        out_pix[1] = 8.0 * (float)block[2][2];
                        out_pix[2] = -1.0 * (float)(block[0][2] + block[1][1] + block[1][3] + block[3][1] +
                                                    block[3][3] + block[4][2]) +
                                     4.0 * (float)(block[1][2] + block[3][2]) +
                                     0.5 * (float)(block[2][0] + block[2][4]) + 5.0 * (float)block[2][2];
                    }
                } else {                         // R row
                    if ((j & 0x00000001) == 0) { // G location
                        out_pix[0] = 0.5 * (float)(block[2][0] + block[2][4]) +
                                     (-1.0) * (float)(block[1][1] + block[1][3] + block[0][2] + block[4][2] +
                                                      block[3][1] + block[3][3]) +
                                     4.0 * (float)(block[1][2] + block[3][2]) + 5.0 * (float)(block[2][2]);
                        out_pix[1] = 8.0 * (float)block[2][2];
                        out_pix[2] = -1.0 * (float)(block[2][0] + block[1][1] + block[1][3] + block[3][1] +
                                                    block[3][3] + block[2][4]) +
                                     4.0 * (float)(block[2][1] + block[2][3]) +
                                     0.5 * (float)(block[0][2] + block[4][2]) + 5.0 * (float)block[2][2];
                    } else { // R location
                        out_pix[0] = -1.5 * (float)(block[0][2] + block[2][0] + block[2][4] + block[4][2]) +
                                     2.0 * (float)(block[1][1] + block[1][3] + block[3][1] + block[3][3]) +
                                     6.0 * (float)block[2][2];
                        out_pix[1] = -1.0 * (float)(block[0][2] + block[2][0] + block[2][4] + block[4][2]) +
                                     2.0 * (float)(block[1][2] + block[2][1] + block[2][3] + block[3][2]) +
                                     4.0 * (float)block[2][2];
                        out_pix[2] = 8.0 * (float)block[2][2];
                    }
                }
            } else if (code == 1) {              // GB
                if ((i & 0x00000001) == 0) {     // B row
                    if ((j & 0x00000001) == 0) { // G location
                        out_pix[0] = 0.5 * (float)(block[0][2] + block[4][2]) +
                                     (-1.0) * (float)(block[1][1] + block[1][3] + block[2][0] + block[2][4] +
                                                      block[3][1] + block[3][3]) +
                                     4.0 * (float)(block[2][1] + block[2][3]) + 5.0 * (float)(block[2][2]);
                        out_pix[1] = 8.0 * (float)block[2][2];
                        out_pix[2] = -1.0 * (float)(block[0][2] + block[1][1] + block[1][3] + block[3][1] +
                                                    block[3][3] + block[4][2]) +
                                     4.0 * (float)(block[1][2] + block[3][2]) +
                                     0.5 * (float)(block[2][0] + block[2][4]) + 5.0 * (float)block[2][2];
                    } else { // B location
                        out_pix[0] = 8 * (float)block[2][2];
                        out_pix[1] = -1.0 * (float)(block[0][2] + block[2][0] + block[2][4] + block[4][2]) +
                                     2.0 * (float)(block[1][2] + block[2][1] + block[2][3] + block[3][2]) +
                                     4.0 * (float)block[2][2];
                        out_pix[2] = -1.5 * (float)(block[0][2] + block[2][0] + block[2][4] + block[4][2]) +
                                     2.0 * (float)(block[1][1] + block[1][3] + block[3][1] + block[3][3]) +
                                     6.0 * (float)block[2][2];
                    }
                } else {                         // R row
                    if ((j & 0x00000001) == 0) { // R location
                        out_pix[0] = -1.5 * (float)(block[0][2] + block[2][0] + block[2][4] + block[4][2]) +
                                     2.0 * (float)(block[1][1] + block[1][3] + block[3][1] + block[3][3]) +
                                     6.0 * (float)block[2][2];
                        out_pix[1] = -1.0 * (float)(block[0][2] + block[2][0] + block[2][4] + block[4][2]) +
                                     2.0 * (float)(block[1][2] + block[2][1] + block[2][3] + block[3][2]) +
                                     4.0 * (float)block[2][2];
                        out_pix[2] = 8.0 * (float)block[2][2];
                    } else { // G location
                        out_pix[0] = 0.5 * (float)(block[2][0] + block[2][4]) +
                                     (-1.0) * (float)(block[1][1] + block[1][3] + block[0][2] + block[4][2] +
                                                      block[3][1] + block[3][3]) +
                                     4.0 * (float)(block[1][2] + block[3][2]) + 5.0 * (float)(block[2][2]);
                        out_pix[1] = 8.0 * (float)block[2][2];
                        out_pix[2] = -1.0 * (float)(block[2][0] + block[1][1] + block[1][3] + block[3][1] +
                                                    block[3][3] + block[2][4]) +
                                     4.0 * (float)(block[2][1] + block[2][3]) +
                                     0.5 * (float)(block[0][2] + block[4][2]) + 5.0 * (float)block[2][2];
                    }
                }
            } else if (code == 2) {              // GR
                if ((i & 0x00000001) == 0) {     // R row
                    if ((j & 0x00000001) == 0) { // G location
                        out_pix[0] = 0.5 * (float)(block[2][0] + block[2][4]) +
                                     (-1.0) * (float)(block[1][1] + block[1][3] + block[0][2] + block[4][2] +
                                                      block[3][1] + block[3][3]) +
                                     4.0 * (float)(block[1][2] + block[3][2]) + 5.0 * (float)(block[2][2]);
                        out_pix[1] = 8.0 * (float)block[2][2];
                        out_pix[2] = -1.0 * (float)(block[2][0] + block[1][1] + block[1][3] + block[3][1] +
                                                    block[3][3] + block[2][4]) +
                                     4.0 * (float)(block[2][1] + block[2][3]) +
                                     0.5 * (float)(block[0][2] + block[4][2]) + 5.0 * (float)block[2][2];
                    } else { // R location
                        out_pix[0] = -1.5 * (float)(block[0][2] + block[2][0] + block[2][4] + block[4][2]) +
                                     2.0 * (float)(block[1][1] + block[1][3] + block[3][1] + block[3][3]) +
                                     6.0 * (float)block[2][2];
                        out_pix[1] = -1.0 * (float)(block[0][2] + block[2][0] + block[2][4] + block[4][2]) +
                                     2.0 * (float)(block[1][2] + block[2][1] + block[2][3] + block[3][2]) +
                                     4.0 * (float)block[2][2];
                        out_pix[2] = 8.0 * (float)block[2][2];
                    }
                } else {                         // B row
                    if ((j & 0x00000001) == 0) { // B location
                        out_pix[0] = 8 * (float)block[2][2];
                        out_pix[1] = -1.0 * (float)(block[0][2] + block[2][0] + block[2][4] + block[4][2]) +
                                     2.0 * (float)(block[1][2] + block[2][1] + block[2][3] + block[3][2]) +
                                     4.0 * (float)block[2][2];
                        out_pix[2] = -1.5 * (float)(block[0][2] + block[2][0] + block[2][4] + block[4][2]) +
                                     2.0 * (float)(block[1][1] + block[1][3] + block[3][1] + block[3][3]) +
                                     6.0 * (float)block[2][2];
                    } else { // G location
                        out_pix[0] = 0.5 * (float)(block[0][2] + block[4][2]) +
                                     (-1.0) * (float)(block[1][1] + block[1][3] + block[2][0] + block[2][4] +
                                                      block[3][1] + block[3][3]) +
                                     4.0 * (float)(block[2][1] + block[2][3]) + 5.0 * (float)(block[2][2]);
                        out_pix[1] = 8.0 * (float)block[2][2];
                        out_pix[2] = -1.0 * (float)(block[0][2] + block[1][1] + block[1][3] + block[3][1] +
                                                    block[3][3] + block[4][2]) +
                                     4.0 * (float)(block[1][2] + block[3][2]) +
                                     0.5 * (float)(block[2][0] + block[2][4]) + 5.0 * (float)block[2][2];
                    }
                }
            } else if (code == 3) {              // RG
                if ((i & 0x00000001) == 0) {     // R row
                    if ((j & 0x00000001) == 0) { // R location
                        out_pix[0] = -1.5 * (float)(block[0][2] + block[2][0] + block[2][4] + block[4][2]) +
                                     2.0 * (float)(block[1][1] + block[1][3] + block[3][1] + block[3][3]) +
                                     6.0 * (float)block[2][2];
                        out_pix[1] = -1.0 * (float)(block[0][2] + block[2][0] + block[2][4] + block[4][2]) +
                                     2.0 * (float)(block[1][2] + block[2][1] + block[2][3] + block[3][2]) +
                                     4.0 * (float)block[2][2];
                        out_pix[2] = 8.0 * (float)block[2][2];
                    } else { // G location
                        out_pix[0] = -1.0 * (float)(block[0][2] + block[1][1] + block[1][3] + block[3][1] +
                                                    block[3][3] + block[4][2]) +
                                     4.0 * (float)(block[1][2] + block[3][2]) +
                                     0.5 * (float)(block[2][0] + block[2][4]) + 5.0 * (float)block[2][2];
                        out_pix[1] = 8.0 * (float)block[2][2];
                        out_pix[2] = 0.5 * (float)(block[0][2] + block[4][2]) +
                                     (-1.0) * (float)(block[1][1] + block[1][3] + block[2][0] + block[2][4] +
                                                      block[3][1] + block[3][3]) +
                                     4.0 * (float)(block[2][1] + block[2][3]) + 5.0 * (float)(block[2][2]);
                    }
                } else {                         // B row
                    if ((j & 0x00000001) == 0) { // G location
                        out_pix[0] = 0.5 * (float)(block[0][2] + block[4][2]) +
                                     (-1.0) * (float)(block[1][1] + block[1][3] + block[2][0] + block[2][4] +
                                                      block[3][1] + block[3][3]) +
                                     4.0 * (float)(block[2][1] + block[2][3]) + 5.0 * (float)(block[2][2]);
                        out_pix[1] = 8.0 * (float)block[2][2];
                        out_pix[2] = -1.0 * (float)(block[0][2] + block[1][1] + block[1][3] + block[3][1] +
                                                    block[3][3] + block[4][2]) +
                                     4.0 * (float)(block[1][2] + block[3][2]) +
                                     0.5 * (float)(block[2][0] + block[2][4]) + 5.0 * (float)block[2][2];
                    } else { // B location
                        out_pix[0] = 8.0 * (float)block[2][2];
                        out_pix[1] = -1.0 * (float)(block[0][2] + block[2][0] + block[2][4] + block[4][2]) +
                                     2.0 * (float)(block[1][2] + block[2][1] + block[2][3] + block[3][2]) +
                                     4.0 * (float)block[2][2];
                        out_pix[2] = -1.5 * (float)(block[0][2] + block[2][0] + block[2][4] + block[4][2]) +
                                     2.0 * (float)(block[1][1] + block[1][3] + block[3][1] + block[3][3]) +
                                     6.0 * (float)block[2][2];
                    }
                }
            }
            out_pix /= 8.0;
            if (output_image.type() == CV_8UC3) {
                output_image.at<cv::Vec3b>(i, j) = (cv::Vec3b)(out_pix);
            } else
                output_image.at<cv::Vec3w>(i, j) = (cv::Vec3w)(out_pix);
        }
    }
}

int run_opencv_ref(cv::Mat& srcImageR, cv::Mat& dstRefImage) {
    demosaicImage<true>(srcImageR, dstRefImage, XF_BAYER_RG);
    return 0;
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
            errorMessage << argv[0] << " <xclbin> <inputImage> [width] [height] [iterations]";
            std::cerr << errorMessage.str();
            throw std::invalid_argument(errorMessage.str());
        }

        const char* xclBinName = argv[1];
        //////////////////////////////////////////
        // Read image from file and resize
        //////////////////////////////////////////
        cv::Mat srcImageR;
        srcImageR = cv::imread(argv[2], 0);

        int width = srcImageR.cols;
        if (argc >= 4) width = atoi(argv[3]);
        int height = srcImageR.rows;
        if (argc >= 5) height = atoi(argv[4]);

        if ((width != srcImageR.cols) || (height != srcImageR.rows))
            cv::resize(srcImageR, srcImageR, cv::Size(width, height));

        int iterations = 1;
        if (argc >= 6) iterations = atoi(argv[5]);

        std::cout << "Image size" << std::endl;
        std::cout << srcImageR.rows << std::endl;
        std::cout << srcImageR.cols << std::endl;
        std::cout << srcImageR.elemSize() << std::endl;
        std::cout << "Image size (end)" << std::endl;
        int op_width = srcImageR.cols;
        int op_height = srcImageR.rows;

        //////////////////////////////////////////
        // Run opencv reference test (filter2D design)
        //////////////////////////////////////////
        cv::Mat dstRefImage(srcImageR.rows, srcImageR.cols, CV_8UC3);
        run_opencv_ref(srcImageR, dstRefImage);

        cv::Mat dstRefBGR[3];
        cv::split(dstRefImage, dstRefBGR);

        // Initialize device
        xF::deviceInit(xclBinName);

        // Load image
        void* srcData = nullptr;
        xrtBufferHandle src_hndl = xrtBOAlloc(xF::gpDhdl, (srcImageR.total() * srcImageR.elemSize()), 0, 0);
        srcData = xrtBOMap(src_hndl);
        memcpy(srcData, srcImageR.data, (srcImageR.total() * srcImageR.elemSize()));

        // Allocate output buffer
        void* dstData = nullptr;
        xrtBufferHandle dst_hndl = xrtBOAlloc(xF::gpDhdl, (op_height * op_width * srcImageR.elemSize() * 4), 0, 0);
        dstData = xrtBOMap(dst_hndl);
        cv::Mat dst(op_height, op_width, CV_8UC4, dstData);

        xF::xfcvDataMovers<xF::TILER, int16_t, TILE_HEIGHT, TILE_WIDTH, VECTORIZATION_FACTOR, CORES> tiler(2, 2);
        xF::xfcvDataMovers<xF::STITCHER, int16_t, TILE_HEIGHT, (TILE_WIDTH << 2), VECTORIZATION_FACTOR, CORES> stitcher;

        std::cout << "Graph init. This does nothing because CDO in boot PDI "
                     "already configures AIE.\n";
        for (int i = 0; i < CORES; i++) demo[i].init();

        START_TIMER
        tiler.compute_metadata(srcImageR.size());
        STOP_TIMER("Meta data compute time")

        std::chrono::microseconds tt(0);
        for (int i = 0; i < iterations; i++) {
            //@{
            std::cout << "Iteration : " << (i + 1) << std::endl;
            START_TIMER
            auto tiles_sz = tiler.host2aie_nb(src_hndl, srcImageR.size());
            stitcher.aie2host_nb(dst_hndl, cv::Size((srcImageR.cols << 2), srcImageR.rows), tiles_sz);

            for (int i = 0; i < CORES; i++) {
                std::cout << "Graph run(" << (tiler.tilesPerCore(i)) << ")\n";
                demo[i].run(tiler.tilesPerCore(i));
            }

            for (int i = 0; i < CORES; i++) {
                demo[i].wait();
            }

            tiler.wait();
            stitcher.wait();

            STOP_TIMER("Demosaic function")
            tt += tdiff;
            //@}

            std::vector<cv::Mat> dstRGBA(4);
            cv::split(dst, dstRGBA);

            cv::Mat aieImage;
            std::vector<cv::Mat> dstBGR = {dstRGBA[2], dstRGBA[1], dstRGBA[0]};
            cv::merge(dstBGR, aieImage);

            // Analyze output {
            std::cout << "Analyzing Blue channel diff\n";
            cv::Mat diff_b;
            cv::absdiff(dstRefBGR[0], dstBGR[0], diff_b);

            std::cout << "Analyzing Green channel diff\n";
            cv::Mat diff_g;
            cv::absdiff(dstRefBGR[1], dstBGR[1], diff_g);

            std::cout << "Analyzing Red channel diff\n";
            cv::Mat diff_r;
            cv::absdiff(dstRefBGR[2], dstBGR[2], diff_r);

            for (int i = 0; i < dstRefBGR[2].rows; i++) {
                for (int j = 0; j < dstRefBGR[2].cols; j++) {
                    int ref = (int)dstRefBGR[2].at<unsigned char>(i, j);
                    int out = (int)dstBGR[2].at<unsigned char>(i, j);
                    int diff = std::abs(ref - out);
                    if (diff > 1) {
                        std::cout << "Diff : " << diff << " Ref : " << ref << " Out : " << out << " Loc : [" << i
                                  << ", " << j << "]" << std::endl;
                    }
                }
            }

            // Reference output
            cv::imwrite("ref.png", dstRefImage);
            cv::imwrite("ref_b.png", dstRefBGR[0]);
            cv::imwrite("ref_g.png", dstRefBGR[1]);
            cv::imwrite("ref_r.png", dstRefBGR[2]);

            // AIE output
            cv::imwrite("aie_b.png", dstBGR[0]);
            cv::imwrite("aie_g.png", dstBGR[1]);
            cv::imwrite("aie_r.png", dstBGR[2]);
            cv::imwrite("aie.png", aieImage);

            // Diff per channel
            cv::imwrite("diff_b.png", diff_b);
            cv::imwrite("diff_g.png", diff_g);
            cv::imwrite("diff_r.png", diff_r);

            // Analyze diff per channel
            float err_per_b, err_per_g, err_per_r;
            bool err = false;
            analyzeDiff(diff_b, 2, err_per_b);
            if (err_per_b > 0.0f) {
                std::cerr << "Test failed (Blue channel)" << std::endl;
                err = true;
            }

            analyzeDiff(diff_g, 2, err_per_g);
            if (err_per_g > 0.0f) {
                std::cerr << "Test failed (Green channel)" << std::endl;
                err = true;
            }

            analyzeDiff(diff_r, 2, err_per_r);
            if (err_per_r > 0.0f) {
                std::cerr << "Test failed (Red channel)" << std::endl;
                err = true;
            }

            if (err) {
                exit(-1);
            }
            //}
        }
        for (int i = 0; i < CORES; i++) {
            demo[i].end();
        }
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
