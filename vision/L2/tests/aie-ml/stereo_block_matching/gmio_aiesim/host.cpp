
/*
 * Copyright 1986-2022 Xilinx, Inc. All Rights Reserved.
 * Copyright 2023-2024 Advanced Micro Devices, Inc. All Rights Reserved.
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
#include "config.h"

#include "adf/adf_api/XRTConfig.h"
#include <adf.h>
#include <chrono>
#include <cmath>
#include <common/xf_aie_sw_utils.hpp>
#include <fstream>
#include <sstream>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <vector>
#include "xrt/xrt_kernel.h"
#include <common/xfcvDataMovers.h>
#include <xaiengine.h>
#include <xrt/experimental/xrt_kernel.h>

using namespace adf;

#include "graph.cpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgcodecs/imgcodecs.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/opencv.hpp"
#define DEBUG1

template <class T, int SP = 0>
void file_write_16bit(cv::Mat img, const char file_path[500]) {
    FILE* fp = fopen(file_path, "w");
    long int cnt = 0;
    float maxv = -999;
    float minv = 999;
    for (int i = 0; i < img.rows; i++) {
        for (int j = 0; j < img.cols; j++) {
            if (SP == 0) {
                T a = img.at<T>(i, j);
                fprintf(fp, "%d ", (T)a);
            }
            if ((j + 1) % img.cols == 0) {
                fprintf(fp, "\n");
            }
        }
    }
    fclose(fp);
}

template <class T, int SP = 0>
void file_write(cv::Mat img, const char file_path[500]) {
    FILE* fp = fopen(file_path, "w");
    long int cnt = 0;

    float maxv = -999;
    float minv = 999;
    int ctr = 0;
    for (int i = 0; i < img.rows; i++) {
        for (int j = 0; j < img.cols; j++) {
            // for (int k = 0; k < img.channels(); k++) {
            if (SP == 0) {
                T a = img.at<uchar>(i, j);
                fprintf(fp, "%d ", (int)a);
                ctr = ctr + 1;
            }
            if ((ctr) % img.cols == 0) {
                fprintf(fp, "\n");
            }
        }
    }
    fclose(fp);

    //    if(SP==1)
    //      printf("\n*** min : max    =     %f  :  %f ", minv, maxv);
}

static std::vector<char> load_xclbin(xrtDeviceHandle device, const std::string& fnm) {
    if (fnm.empty()) throw std::runtime_error("No XCLBIN specified");

    /* Load bit stream */
    std::ifstream stream(fnm);
    stream.seekg(0, stream.end);
    size_t size = stream.tellg();
    stream.seekg(0, stream.beg);

    std::vector<char> header(size);
    stream.read(header.data(), size);

    auto top = reinterpret_cast<const axlf*>(header.data());
    if (xrtDeviceLoadXclbin(device, top)) throw std::runtime_error("Bitstream download failed");

    return header;
}

/*
 ******************************************************************************
 * Top level executable
 ******************************************************************************
 */
#define _TEXTURE_THRESHOLD_ 20
#define _UNIQUENESS_RATIO_ 15
#define _PRE_FILTER_CAP_ 31
#define _MIN_DISP_ 0

int run_opencv_ref_sbm(cv::Mat& lImage, cv::Mat& rImage, cv::Mat& disp8) {
    cv::Mat disp;
    cv::Ptr<cv::StereoBM> stereobm = cv::StereoBM::create(NO_DISPARITIES, TILE_WINSIZE);
    stereobm->setPreFilterCap(_PRE_FILTER_CAP_);
    stereobm->setUniquenessRatio(_UNIQUENESS_RATIO_);
    stereobm->setTextureThreshold(_TEXTURE_THRESHOLD_);
    stereobm->compute(lImage, rImage, disp);
    disp.convertTo(disp8, CV_8U, (256.0 / NO_DISPARITIES) / (16.));
    return 0;
}

int run_opencv_ref_sobel(cv::Mat& lImage, cv::Mat& rImage, cv::Mat& leftsobelClipx_ocv, cv::Mat& rightsobelClipx_ocv) {
    cv::Mat leftsobelx, rightsobelx;
    cv::Sobel(lImage, leftsobelx, CV_16S, 1, 0, 3);
    cv::Sobel(rImage, rightsobelx, CV_16S, 1, 0, 3);

    // CLIP

    int16_t cap = 31;
    for (int x = 0; x < leftsobelx.rows; x++) {
        for (int y = 0; y < leftsobelx.cols; y++) {
            int16_t pix = leftsobelx.at<int16_t>(x, y);
            uint8_t temp = (uint8_t)(pix < -cap ? 0 : pix > cap ? cap * 2 : pix + cap);
            leftsobelClipx_ocv.at<uint8_t>(x, y) = (uint8_t)temp;
            if (x == 0) leftsobelClipx_ocv.at<uint8_t>(x, y) = (uint8_t)31; // to match aie
            int16_t pix1 = rightsobelx.at<int16_t>(x, y);
            uint8_t temp1 = (uint8_t)(pix1 < -cap ? 0 : pix1 > cap ? cap * 2 : pix1 + cap);
            rightsobelClipx_ocv.at<uint8_t>(x, y) = (uint8_t)temp1;
            if (x == 0) rightsobelClipx_ocv.at<uint8_t>(x, y) = (uint8_t)31; // to match aie
        }
    }
    return 0;
}

int set_borders_to_const(cv::Mat& dstRefImageL_sobel, cv::Mat& dstImageL_sobel) {
    for (int x = 0; x < IMAGE_HEIGHT; x++) {
        dstRefImageL_sobel.at<uint8_t>(x, 0) = (uint8_t)31;
        dstRefImageL_sobel.at<uint8_t>(x, IMAGE_WIDTH - 1) = (uint8_t)31;
        dstImageL_sobel.at<uint8_t>(x, 0) = (uint8_t)31;
        dstImageL_sobel.at<uint8_t>(x, IMAGE_WIDTH - 1) = (uint8_t)31;
    }

    for (int y = 0; y < IMAGE_WIDTH; y++) {
        dstRefImageL_sobel.at<uint8_t>(0, y) = (uint8_t)31;
        dstRefImageL_sobel.at<uint8_t>(IMAGE_HEIGHT - 1, y) = (uint8_t)31;
        dstImageL_sobel.at<uint8_t>(0, y) = (uint8_t)31;
        dstImageL_sobel.at<uint8_t>(IMAGE_HEIGHT - 1, y) = (uint8_t)31;
    }
    return 0;
}

int select_output_ranges(cv::Mat& disp8, cv::Mat& aie_outImage8, cv::Mat& dstRefImage, cv::Mat& aie_outImage8_final) {
    cv::Rect myROIo(NO_DISPARITIES + (TILE_WINSIZE - 1) / 2 + (TILE_WINSIZE_SOBEL - 1) / 2,
                    (TILE_WINSIZE - 1) / 2 + (TILE_WINSIZE_SOBEL - 1) / 2,
                    IMAGE_WIDTH - NO_DISPARITIES - (TILE_WINSIZE - 1) - (TILE_WINSIZE_SOBEL - 1),
                    IMAGE_HEIGHT - (TILE_WINSIZE - 1) - (TILE_WINSIZE_SOBEL - 1));
    dstRefImage = disp8(myROIo);
    cv::Rect myROIo_aie(0, 1, dstRefImage.cols, dstRefImage.rows);
    aie_outImage8_final = aie_outImage8(myROIo_aie);

    return 0;
}

int remove_top_border(cv::Mat& dstRefImageL_sobel, cv::Mat& croppedImage) {
    int topBorderHeight = 1;
    cv::Rect roi(0, topBorderHeight, dstRefImageL_sobel.cols, dstRefImageL_sobel.rows - topBorderHeight);
    croppedImage = dstRefImageL_sobel(roi);
    return 0;
}

int remove_left_border(cv::Mat& dstRefImageL_sobel, cv::Mat& croppedImage) {
    int leftBorderWidth = 1;
    cv::Rect roi(leftBorderWidth, 0, dstRefImageL_sobel.cols - leftBorderWidth, dstRefImageL_sobel.rows);
    croppedImage = dstRefImageL_sobel(roi);

    return 0;
}

void removeFirstNColumns(cv::Mat& lImage, int NO_DISPARITIES) {
    cv::Mat newImage = lImage(cv::Rect(NO_DISPARITIES, 0, lImage.cols - NO_DISPARITIES, lImage.rows));
    lImage = newImage;
}

void appendColumns(cv::Mat& lImage, int NO_DISPARITIES, const cv::Scalar& fillColor = cv::Scalar(0, 0, 0)) {
    int newWidth = lImage.cols + NO_DISPARITIES;
    cv::Mat newImage(lImage.rows, newWidth, lImage.type(), fillColor);
    lImage.copyTo(newImage(cv::Rect(0, 0, lImage.cols, lImage.rows)));
    lImage = newImage;
}

int main(int argc, char** argv) {
    try {
        if (argc < 3) {
            std::stringstream errorMessage;
            errorMessage << argv[0] << " <xclbin> <inputImage1> <inputImage2> [iterations]";
            std::cerr << errorMessage.str();
            throw std::invalid_argument(errorMessage.str());
        }

        // Initializa device
        const char* xclBinName(argv[1]);
        xF::deviceInit(xclBinName);

        //////////////////////////////////////////////
        // Read image from file and resize to full-HD
        //////////////////////////////////////////////

        cv::Mat lImageHD, rImageHD;
        lImageHD = cv::imread(argv[2], 0);
        rImageHD = cv::imread(argv[3], 0);

        std::cout << "lImageHD size (RxC) is " << lImageHD.rows << "x" << lImageHD.cols << std::endl;
        std::cout << "rImageHD size (RxC) is " << rImageHD.rows << "x" << rImageHD.cols << std::endl;

        // Resize to FHD
        cv::Mat lImage, rImage;
        int up_width = IMAGE_WIDTH;
        int up_height = IMAGE_HEIGHT;
        cv::resize(lImageHD, lImage, cv::Size(up_width, up_height), cv::INTER_LINEAR);
        cv::resize(rImageHD, rImage, cv::Size(up_width, up_height), cv::INTER_LINEAR);

        std::cout << "lImage size (RxC) is " << lImage.rows << "x" << lImage.cols << std::endl;
        std::cout << "rImage size (RxC) is " << rImage.rows << "x" << rImage.cols << std::endl;

        int iterations = 1;
        if (argc >= 5) iterations = atoi(argv[4]);

        //////////////////////////////////////////
        // Run opencv reference test (SBM)
        //////////////////////////////////////////
        std::cout << "[INFO] Running OpenCV SBM ... " << std::endl;

        START_TIMER
        cv::Mat disp8;
        run_opencv_ref_sbm(lImage, rImage, disp8);
        STOP_TIMER("OpenCV Ref of SBM");

        cv::imwrite("disp8_ocv_output.png", disp8);
        std::cout << "OpenCV Disparity Matrix size (RxC) is " << disp8.rows << "x" << disp8.cols << std::endl;

        // Initializa device
        xF::deviceInit(xclBinName);

        ///////////////////////////////////
        // RUN  SOBEL LEFT IMAGE on AIE
        ///////////////////////////////////
        std::cout << "[INFO] Running SOBEL LEFT on AIE ... " << std::endl;

        //// PRE-PROCESS LIMAGE ////
        // remove first no-disparities columns from lImage
        removeFirstNColumns(lImage, NO_DISPARITIES);

        appendColumns(lImage, NO_DISPARITIES);

        // Allocate input buffer
        std::vector<uint8_t> lData_sobel;
        lData_sobel.assign(lImage.data, (lImage.data + lImage.total()));

        // Allocate output buffer
        std::vector<uint8_t> dstDataL_sobel;
        dstDataL_sobel.assign((IMAGE_HEIGHT + 2 * (TILE_WINSIZE)) * (IMAGE_WIDTH + 32 + 240), 0);
        cv::Mat dstImageL_sobel((IMAGE_HEIGHT + 2 * (TILE_WINSIZE)), (IMAGE_WIDTH + 32 + 240), CV_8UC1,
                                (void*)dstDataL_sobel.data());

        xF::xfcvDataMovers<xF::TILER, uint8_t, TILE_IN_HEIGHT_SOBEL, TILE_IN_WIDTH_SOBEL, TILE_IN_WIDTH_SOBEL,
                           NO_CORES_SOBEL, 0, true>
            tilerL_sobel(2, 0);
        xF::xfcvDataMovers<xF::STITCHER, uint8_t, TILE_OUT_HEIGHT_SOBEL, TILE_OUT_WIDTH_SOBEL, TILE_OUT_WIDTH_SOBEL,
                           NO_CORES_SOBEL, 0, true>
            stitcherL_sobel;

#if !__X86_DEVICE__
        std::cout << "Graph init. This does nothing because CDO in boot PDI "
                     "already configures AIE.\n";
        auto gHndl_sobel = xrt::graph(xF::gpDhdl, xF::xclbin_uuid, "demo_sobel");
        std::cout << "XRT graph opened" << std::endl;
        gHndl_sobel.reset();
        std::cout << "Graph reset done" << std::endl;
#endif

        START_TIMER
        tilerL_sobel.compute_metadata(lImage.size(), cv::Size(0, 0), false, TILE_WIDTH, TILE_HEIGHT, false, true);
        STOP_TIMER("Meta data compute time")

        int iterationsL_sobel = 1;

        std::chrono::microseconds tt1(0);
        for (int i = 0; i < iterationsL_sobel; i++) {
            //@{
            std::cout << "Iteration : " << (i + 1) << std::endl;
            std::cout << "Sending data: " << lImage.size() << "\n";
            std::cout << "Receiving data: " << dstImageL_sobel.size() << "\n";

            auto tiles_sz =
                tilerL_sobel.host2aie_nb(lData_sobel.data(), lImage.size(), {"demo_sobel.in[0]", "demo_sobel.in[1]"});
            std::cout << "Graph running for " << (tiles_sz[0] * tiles_sz[1] / NO_CORES_SOBEL)
                      << " iterationsL_sobel.\n";
            stitcherL_sobel.aie2host_nb(dstDataL_sobel.data(), dstImageL_sobel.size(), tiles_sz,
                                        {"demo_sobel.out[0]", "demo_sobel.out[1]"});

#if !__X86_DEVICE__
            START_TIMER
            gHndl_sobel.run(tiles_sz[0] * tiles_sz[1] / (NO_CORES_SOBEL));
#endif

#if !__X86_DEVICE__
            gHndl_sobel.wait();
            STOP_TIMER("Total time to process frame")

#endif

            stitcherL_sobel.wait({"demo_sobel.out[0]", "demo_sobel.out[1]"});
            std::cout << "Data transfer complete (Stitcher - SOBEL LEFT IMAGE)\n";
            tt1 += tdiff;
        }

        ///////////////////////////////////
        // RUN  SOBEL RIGHT IMAGE on AIE
        ///////////////////////////////////
        std::cout << "[INFO] Running SOBEL RIGHT on AIE ... " << std::endl;

        // Allocate input buffer
        std::vector<uint8_t> RData_sobel;
        RData_sobel.assign(rImage.data, (rImage.data + rImage.total()));

        // Allocate output buffer
        std::vector<uint8_t> dstDataR_sobel;
        dstDataR_sobel.assign((IMAGE_HEIGHT + 2 * (TILE_WINSIZE)) * (IMAGE_WIDTH + 32 + 240), 0);
        cv::Mat dstImageR_sobel((IMAGE_HEIGHT + 2 * (TILE_WINSIZE)), (IMAGE_WIDTH + 32 + 240), CV_8UC1,
                                (void*)dstDataR_sobel.data());

        xF::xfcvDataMovers<xF::TILER, uint8_t, TILE_IN_HEIGHT_SOBEL, TILE_IN_WIDTH_SOBEL, TILE_IN_WIDTH_SOBEL,
                           NO_CORES_SOBEL, 0, true>
            tilerR_sobel(2, 0);
        xF::xfcvDataMovers<xF::STITCHER, uint8_t, TILE_OUT_HEIGHT_SOBEL, TILE_OUT_WIDTH_SOBEL, TILE_OUT_WIDTH_SOBEL,
                           NO_CORES_SOBEL, 0, true>
            stitcherR_sobel;

#if !__X86_DEVICE__
        std::cout << "Graph init. This does nothing because CDO in boot PDI "
                     "already configures AIE.\n";
        gHndl_sobel.reset();
        std::cout << "Graph reset done" << std::endl;
#endif

        START_TIMER
        tilerR_sobel.compute_metadata(rImage.size(), cv::Size(0, 0), false, TILE_WIDTH, TILE_HEIGHT, false, true);
        STOP_TIMER("Meta data compute time")

        int iterationsR_sobel = 1;
        std::chrono::microseconds tt2(0);
        for (int i = 0; i < iterationsR_sobel; i++) {
            //@{
            std::cout << "Iteration : " << (i + 1) << std::endl;
            std::cout << "Sending data: " << rImage.size() << "\n";
            std::cout << "Receiving data: " << dstImageR_sobel.size() << "\n";

            //@{
            auto tiles_sz =
                tilerR_sobel.host2aie_nb(RData_sobel.data(), rImage.size(), {"demo_sobel.in[0]", "demo_sobel.in[1]"});
            std::cout << "Graph running for " << (tiles_sz[0] * tiles_sz[1] / NO_CORES_SOBEL)
                      << " iterationsR_sobel.\n";
            stitcherR_sobel.aie2host_nb(dstDataR_sobel.data(), dstImageR_sobel.size(), tiles_sz,
                                        {"demo_sobel.out[0]", "demo_sobel.out[1]"});

#if !__X86_DEVICE__
            START_TIMER
            gHndl_sobel.run(tiles_sz[0] * tiles_sz[1] / (NO_CORES_SOBEL));
#endif

#if !__X86_DEVICE__
            gHndl_sobel.wait();
#endif

            STOP_TIMER("Total time to process frame")
            std::cout << "Data transfer complete (Stitcher - SOBEL RIGHT IMAGE)\n";
            tt2 += tdiff;
            stitcherR_sobel.wait({"demo_sobel.out[0]", "demo_sobel.out[1]"});
        }

#if !__X86_DEVICE__
        gHndl_sobel.end(0);
#endif

        ///////////////////////////////////
        // RUN  SBM on AIE
        ///////////////////////////////////
        std::cout << "[INFO] Running SBM on AIE ... " << std::endl;
        START_TIMER
        cv::Mat srcImageR =
            dstImageR_sobel(cv::Rect(0, 0, dstImageR_sobel.cols, dstImageR_sobel.rows - TILE_WINSIZE)).clone();
        cv::Mat srcImageL =
            dstImageL_sobel(cv::Rect(0, 0, dstImageL_sobel.cols, dstImageL_sobel.rows - TILE_WINSIZE)).clone();

        // To have sufficient tiles for 11 cores
        int append_cols = 200;
        /*appendColumns(srcImageR, append_cols);
        appendColumns(srcImageL, append_cols);*/

        STOP_TIMER("DATA TRANFER compute time")

        START_TIMER

        // Convert the cv::Mat to a std::vector
        std::vector<uint8_t> srcDataL;
        // If the matrix is not continuous, we need to manually copy each row
        for (int i = 0; i < srcImageL.rows; ++i) {
            srcDataL.insert(srcDataL.end(), srcImageL.ptr<uchar>(i),
                            srcImageL.ptr<uchar>(i) + srcImageL.cols * srcImageL.elemSize());
        }
        std::vector<uint8_t> srcDataR;
        for (int i = 0; i < srcImageR.rows; ++i) {
            srcDataR.insert(srcDataR.end(), srcImageR.ptr<uchar>(i),
                            srcImageR.ptr<uchar>(i) + srcImageR.cols * srcImageL.elemSize());
        }
        STOP_TIMER("Matrix to Array compute time")

        // Allocate output buffer
        std::vector<int16_t> dstData;
        dstData.assign(TILE_OUT_HEIGHT * (IMAGE_WIDTH + NO_DISPARITIES + 32 + append_cols) * sizeof(int16_t), 0);
        cv::Mat dstImage(TILE_OUT_HEIGHT, (IMAGE_WIDTH + NO_DISPARITIES + 32 + append_cols), CV_16SC1,
                         (void*)dstData.data());

        xF::xfcvDataMovers<xF::TILER, uint8_t, TILE_IN_HEIGHT_WITH_WINDOW, LEFT_TILE_IN_WIDTH, 64, NO_CORES, 0, true>
            tiler1(LOVERLAP, 0);
        xF::xfcvDataMovers<xF::TILER, uint8_t, TILE_IN_HEIGHT_WITH_WINDOW, RIGHT_TILE_IN_WIDTH, RIGHT_TILE_IN_WIDTH,
                           NO_CORES, 0, true>
            tiler2(ROVERLAP, 0);
        xF::xfcvDataMovers<xF::STITCHER, int16_t, TILE_OUT_HEIGHT, COLS_COMBINE, COLS_COMBINE, NO_CORES, 0, true>
            stitcher(1);

#if !__X86_DEVICE__
        std::cout << "Graph init. This does nothing because CDO in boot PDI "
                     "already configures AIE.\n";
        auto gHndl = xrt::graph(xF::gpDhdl, xF::xclbin_uuid, "sbm_graph");
        std::cout << "XRT graph opened" << std::endl;
        gHndl.reset();
        std::cout << "Graph reset done" << std::endl;
#endif
        START_TIMER
        tiler1.compute_metadata(srcImageL.size(), cv::Size(0, 0), false, TILE_WIDTH, TILE_HEIGHT, false, true);
        tiler2.compute_metadata(srcImageR.size(), cv::Size(0, 0), false, TILE_WIDTH, TILE_HEIGHT, false, true);
        STOP_TIMER("Meta data compute time")

        std::chrono::microseconds tt(0);
        for (int i = 0; i < iterations; i++) {
            //@{
            std::cout << "Iteration : " << (i + 1) << std::endl;
            std::cout << "Sending data: " << srcImageR.size() << "\n";
            std::cout << "Receiving data: " << dstImage.size() << "\n";

            //@{
            tiler1.host2aie_nb(srcDataL.data(), srcImageL.size(),
                               {"sbm_graph.in_sobel_left_tile[0]"});
            auto tiles_sz = tiler2.host2aie_nb(srcDataR.data(), srcImageR.size(),
                                               {"sbm_graph.in_sobel_right_tile[0]"});
            stitcher.aie2host_nb(dstData.data(), dstImage.size(), tiles_sz,
                                 {"sbm_graph.out_interp[0]"});
            std::cout << "Graph running for " << (tiles_sz[0] * tiles_sz[1]) / NO_CORES << " iterations.\n";

#if !__X86_DEVICE__
            START_TIMER
            gHndl.run((tiles_sz[0] * tiles_sz[1]) / NO_CORES);
#endif

#if !__X86_DEVICE__
            gHndl.wait();
#endif
            STOP_TIMER("Total time to process frame")
            tt += tdiff;
            std::cout << "Data transfer complete (Stitcher)\n";

            stitcher.wait({"sbm_graph.out_interp[0]"});

            //}
        }

        // Write aie_OutImage.png
        cv::Mat aie_outImage8;
        dstImage.convertTo(aie_outImage8, CV_8U, (256.0 / NO_DISPARITIES) / (16.));
        cv::imwrite("aie_outImage8.png", aie_outImage8);

        // Analyze output {
        std::cout << "Analyzing diff6\n";

        // select output ignoring disparities and borders
        cv::Mat diff, dstRefImage, aie_outImage8_final;
        select_output_ranges(disp8, aie_outImage8, dstRefImage, aie_outImage8_final);
        cv::absdiff(dstRefImage, aie_outImage8_final, diff);

        std::cout << " NO_CORES = " << NO_CORES << " TILE_WINSIZE is == " << TILE_WINSIZE
                  << " NO_DISPARITIES = " << NO_DISPARITIES << std::endl;
        cv::imwrite("ref.jpg", dstRefImage);
        cv::imwrite("aie_final.jpg", aie_outImage8_final);
        cv::imwrite("diff.jpg", diff);

        float err_per;
        analyzeDiff(diff, 1, err_per);
        if (err_per > 0.0f) {
            std::cerr << "Test failed..." << std::endl;
            exit(-1);
        }

//}
#if !__X86_DEVICE__
        gHndl.end(0);
#endif
        std::cout << "Average time to process frame : " << (((float)tt.count() * 0.001) / (float)iterations) << " ms"
                  << std::endl;
        std::cout << "Average frames per second : " << (((float)1000000 / (float)tt.count()) * (float)iterations)
                  << " fps" << std::endl;

        std::cout << " ***** Test passed ***** " << std::endl;

        return 0;
    } catch (std::exception& e) {
        const char* errorMessage = e.what();
        std::cerr << "Exception caught: " << errorMessage << std::endl;
        exit(-1);
    }
}