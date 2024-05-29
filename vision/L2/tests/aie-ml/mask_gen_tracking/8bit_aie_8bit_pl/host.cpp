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
#include "maskgen_util.h"
#include "maskgen_ref.hpp"
#include "maskgen_track_ref.hpp"

#include "config.h"

/*
 ******************************************************************************
 * Top level executable
 ******************************************************************************
 */
void opencv_ref(cv::Mat srcPd,
                cv::Mat srcPs,
                cv::Mat dst,
                uint8_t fg_hold,
                uint8_t bg_hold,
                uint8_t depth_min,
                uint8_t depth_max,
                uint8_t fg_thresh_track_r,
                uint8_t bg_thresh_track_r,
                uint8_t pred_seg_thresh) {
    uint8_t* inputDataPredDepth = (uint8_t*)malloc(srcPd.rows * srcPd.cols * sizeof(char));
    uint8_t* inputDataPredSeg = (uint8_t*)malloc(srcPs.rows * srcPs.cols * sizeof(char));
    uint8_t* maskRef = (uint8_t*)malloc(srcPd.rows * srcPd.cols * sizeof(char));

    for (int i = 0; i < srcPd.rows; i++) {
        for (int j = 0; j < srcPd.cols; j++) {
            inputDataPredDepth[i * srcPd.cols + j] = (uint8_t)srcPd.data[(i * srcPd.cols) + (j)];
        }
    }

    for (int i = 0; i < srcPs.rows; i++) {
        for (int j = 0; j < srcPs.cols; j++) {
            inputDataPredSeg[i * srcPs.cols + j] = (uint8_t)srcPs.data[(i * srcPs.cols) + (j)];
        }
    }

    printf(" Before fg_thresh:%d\nbg_thresh:%d\n", fg_thresh_track_r, bg_thresh_track_r);

    uint32_t sum_r, non_zero_r;
    uint8_t depth_or_ref;

    maskgen_track_param_ref(inputDataPredDepth, inputDataPredSeg, (uint8)depth_min, (uint8)depth_max, (uint8)fg_hold,
                            (uint8)bg_hold, (uint8)pred_seg_thresh, (int)srcPd.rows, (int)srcPd.cols, depth_or_ref,
                            sum_r, non_zero_r, fg_thresh_track_r, bg_thresh_track_r);

    printf("REF : non zero:%d\n sum:%d\n dep_or:%d\n", non_zero_r, sum_r, depth_or_ref);
    printf("After fg_thresh:%d\nbg_thresh:%d\n", fg_thresh_track_r, bg_thresh_track_r);

    maskgen_ref(inputDataPredDepth, maskRef, (bool)MASKGEN_TRACKING, depth_min, depth_max, fg_hold, bg_hold,
                fg_thresh_track_r, bg_thresh_track_r, (int)srcPd.rows, (int)srcPd.cols);

    for (int i = 0; i < dst.rows; i++) {
        for (int j = 0; j < dst.cols; j++) {
            dst.data[i * srcPd.cols + j] = (uint8_t)maskRef[i * srcPd.cols + j];
        }
    }
    return;
}

int main(int argc, char** argv) {
    try {
        if (argc < 3) {
            std::stringstream errorMessage;
            errorMessage << argv[0]
                         << " <xclbin> <input Pred Depth Image> <input Pred Seg Image> [width] [height] [iterations]";
            std::cerr << errorMessage.str();
            throw std::invalid_argument(errorMessage.str());
        }
        const char* xclBinName = argv[1];
        //////////////////////////////////////////
        // Read image from file and resize
        //////////////////////////////////////////
        cv::Mat srcImagePd, srcImagePs;
        srcImagePd = cv::imread(argv[2], 0);
        srcImagePs = cv::imread(argv[3], 0);

        int width = srcImagePd.cols;
        if (argc >= 5) width = atoi(argv[4]);
        int height = srcImagePd.rows;
        if (argc >= 6) height = atoi(argv[5]);

        int iterations = 1;
        if (argc >= 7) iterations = atoi(argv[6]);

        if ((width != srcImagePd.cols) || (height != srcImagePd.rows))
            cv::resize(srcImagePd, srcImagePd, cv::Size(width, height));

        if ((width != srcImagePs.cols) || (height != srcImagePs.rows))
            cv::resize(srcImagePs, srcImagePs, cv::Size(width, height));

        std::cout << "Image size Pd" << std::endl;
        std::cout << srcImagePd.rows << std::endl;
        std::cout << srcImagePd.cols << std::endl;
        std::cout << srcImagePd.elemSize() << std::endl;
        std::cout << srcImagePd.type() << std::endl;
        std::cout << "Image size Pd(end)" << std::endl;

        std::cout << "Image size Ps" << std::endl;
        std::cout << srcImagePs.rows << std::endl;
        std::cout << srcImagePs.cols << std::endl;
        std::cout << srcImagePs.elemSize() << std::endl;
        std::cout << srcImagePs.type() << std::endl;
        std::cout << "Image size Ps(end)" << std::endl;

        int op_width_track = 16;
        int op_height_track = 1;

        int op_width = srcImagePd.cols;
        int op_height = srcImagePd.rows;

        // Variable instantiation
        uint16_t fg_thresh, bg_thresh;
        uint8_t fg_thresh_track_r, bg_thresh_track_r;
        uint32_t non_zero_count, sum;

        scalar_comp_utility((uint8_t)_FGTH, (uint8_t)_BGTH, (uint8_t)_MIN, (uint8_t)_MAX, fg_thresh, bg_thresh);
        printf("fg_thresh:%d\nbg_thresh:%d\n", fg_thresh, bg_thresh);

        //////////////////////////////////////////
        // Run opencv reference test (filter2D design)
        //////////////////////////////////////////
        cv::Mat dstRefImage(op_height, op_width, CV_8UC1);
        opencv_ref(srcImagePd, srcImagePs, dstRefImage, (uint8_t)_FGTH, (uint8_t)_BGTH, (uint8_t)_MIN, (uint8_t)_MAX,
                   fg_thresh_track_r, bg_thresh_track_r, (uint8_t)PRED_SEG_THRESH);

        // Initializa device
        xF::deviceInit(xclBinName);

        // Load image
        void* srcDataPd = nullptr;
        xrt::bo src_hndl_pd = xrt::bo(xF::gpDhdl, (srcImagePd.total() * srcImagePd.elemSize()), 0, 0);
        srcDataPd = src_hndl_pd.map();
        memcpy(srcDataPd, srcImagePd.data, (srcImagePd.total() * srcImagePd.elemSize()));

        void* srcDataPs = nullptr;
        xrt::bo src_hndl_ps = xrt::bo(xF::gpDhdl, (srcImagePs.total() * srcImagePs.elemSize()), 0, 0);
        srcDataPs = src_hndl_ps.map();
        memcpy(srcDataPs, srcImagePs.data, (srcImagePs.total() * srcImagePs.elemSize()));

        // Allocate output buffer
        void* dstDataTrack = nullptr;
        xrt::bo* ptr_dstHndl_track =
            new xrt::bo(xF::gpDhdl, (op_height_track * op_width_track * srcImagePd.elemSize()), 0, 0);
        dstDataTrack = ptr_dstHndl_track->map();
        cv::Mat dst_track(op_height_track, op_width_track, CV_8UC1, dstDataTrack);

        void* dstData = nullptr;
        xrt::bo* ptr_dstHndl = new xrt::bo(xF::gpDhdl, (op_height * op_width * srcImagePd.elemSize()), 0, 0);
        dstData = ptr_dstHndl->map();
        cv::Mat dst(op_height, op_width, CV_8UC1, dstData);

        xF::xfcvDataMovers<xF::TILER, uint8_t, TILE_HEIGHT_IN, TILE_WIDTH_IN, 16> tiler1(0, 0);
        xF::xfcvDataMovers<xF::TILER, uint8_t, TILE_HEIGHT_IN, TILE_WIDTH_IN, 16> tiler2(0, 0);
        xF::xfcvDataMovers<xF::STITCHER, uint8_t, TILE_HEIGHT_OUT, TILE_WIDTH_OUT, 16> stitcher1;

        xF::xfcvDataMovers<xF::TILER, uint8_t, TILE_HEIGHT_IN, TILE_WIDTH_IN, 16> tiler3(0, 0);
        xF::xfcvDataMovers<xF::STITCHER, uint8_t, TILE_HEIGHT_IN, TILE_WIDTH_IN, 16> stitcher2;

#if !__X86_DEVICE__
        std::cout << "Graph init. This does nothing because CDO in boot PDI "
                     "already configures AIE.\n";
        auto gHndl1 = xrt::graph(xF::gpDhdl, xF::xclbin_uuid, "maskGenTrack");
        auto gHndl2 = xrt::graph(xF::gpDhdl, xF::xclbin_uuid, "maskGen");
        std::cout << "XRT graph opened" << std::endl;
        gHndl1.reset();
        gHndl2.reset();
/*std::cout << "Graph reset done" << std::endl;*/
#endif

        std::chrono::microseconds tt(0);
        for (int i = 0; i < iterations; i++) {
            std::cout << "Iteration : " << (i + 1) << std::endl;
            gHndl1.update("maskGenTrack.k.in[2]", (uint8_t)_MIN);
            gHndl1.update("maskGenTrack.k.in[3]", (uint8_t)_MAX);
            gHndl1.update("maskGenTrack.k.in[4]", fg_thresh);
            gHndl1.update("maskGenTrack.k.in[5]", bg_thresh);
            gHndl1.update("maskGenTrack.k.in[6]", (uint8_t)PRED_SEG_THRESH);

            START_TIMER
            tiler1.compute_metadata(srcImagePd.size());
            tiler2.compute_metadata(srcImagePs.size());
            STOP_TIMER("Meta data compute time for tracking")

            //@{
            START_TIMER
            auto tiles_sz1 = tiler1.host2aie_nb(&src_hndl_pd, srcImagePd.size());
            tiler2.host2aie_nb(&src_hndl_ps, srcImagePs.size());
            stitcher1.aie2host_nb(ptr_dstHndl_track, dst_track.size(), tiles_sz1);

#if !__X86_DEVICE__
            std::cout << "Graph running for " << (tiles_sz1[0] * tiles_sz1[1]) << " iterations.\n";
            gHndl1.run(tiles_sz1[0] * tiles_sz1[1]);
            gHndl1.wait();
#endif
            tiler1.wait();
            tiler2.wait();
            stitcher1.wait();

            STOP_TIMER("MaskGen track function")
            std::cout << "Data transfer complete (Stitcher)\n";
            tt += tdiff;

            non_zero_count =
                dst_track.data[0] | (dst_track.data[1] << 8) | (dst_track.data[2] << 16) | (dst_track.data[3] << 24);
            sum = dst_track.data[4] | (dst_track.data[5] << 8) | (dst_track.data[6] << 16) | (dst_track.data[7] << 24);

            uint8_t dep_or = scalar_tracking_comp_utility((uint8_t)_FGTH, (uint8_t)_BGTH, (uint8_t)_MIN, (uint8_t)_MAX,
                                                          non_zero_count, sum, fg_thresh, bg_thresh);

            printf("AIE : non zero:%d\n sum:%d\n dep_or:%d\n", non_zero_count, sum, dep_or);

            gHndl2.update("maskGen.k.in[1]", (uint8_t)_MIN);
            gHndl2.update("maskGen.k.in[2]", (uint8_t)_MAX);
            gHndl2.update("maskGen.k.in[3]", fg_thresh);
            gHndl2.update("maskGen.k.in[4]", bg_thresh);

            START_TIMER
            tiler3.compute_metadata(srcImagePd.size());
            STOP_TIMER("Meta data compute time for tracking")

            START_TIMER
            auto tiles_sz2 = tiler3.host2aie_nb(&src_hndl_pd, srcImagePd.size());
            stitcher2.aie2host_nb(ptr_dstHndl, dst.size(), tiles_sz2);

#if !__X86_DEVICE__
            std::cout << "Graph running for " << (tiles_sz2[0] * tiles_sz2[1]) << " iterations.\n";
            gHndl2.run(tiles_sz2[0] * tiles_sz2[1]);
            gHndl2.wait();
#endif
            tiler3.wait();
            stitcher2.wait();

            STOP_TIMER("MaskGen function")
            std::cout << "Data transfer complete (Stitcher)\n";
            tt += tdiff;

            //@}
        }
        //}
        // Analyze output {
        std::cout << "Analyzing diff\n";
        cv::Mat diff(op_height, op_width, CV_8UC1);
        cv::absdiff(dstRefImage, dst, diff);
        cv::imwrite("ref.png", dstRefImage);
        cv::imwrite("aie.png", dst);
        cv::imwrite("diff.png", diff);

        float err_per;
        analyzeDiff(diff, 4, err_per);
        if (err_per > 0) {
            std::cerr << "Test failed" << std::endl;
            exit(-1);
        }

        std::cout << "Test passed" << std::endl;
        std::cout << "Average time to process frame : " << (((float)tt.count() * 0.001) / (float)iterations) << " ms"
                  << std::endl;
        std::cout << "Average frames per second : " << (((float)1000000 / (float)tt.count()) * (float)iterations)
                  << " fps" << std::endl;

#if !__X86_DEVICE__
        gHndl1.end(0);
        gHndl2.end(0);
#endif

        return 0;

    } catch (std::exception& e) {
        const char* errorMessage = e.what();
        std::cerr << "Exception caught: " << errorMessage << std::endl;
        exit(-1);
    }
}
