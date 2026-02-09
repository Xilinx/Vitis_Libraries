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

// imports
#define PROFILE

#include <adf/adf_api/XRTConfig.h>
#include <chrono>
#include <common/xf_aie_sw_utils.hpp>
#include <common/xfcvDataMovers.h>
#include <fstream>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <experimental/xrt_kernel.h>
#include <experimental/xrt_graph.h>
#include <xrt/experimental/xrt_aie.h>

#include "graph.cpp"
#include "config.h"
/*
 ******************************************************************************
 * Top level executable
 ******************************************************************************
 */

// reference fn
void pixelwise_select_ref(uint8_t* frame, uint8_t* mask, uint8_t* bg, uint8_t* out, int elements) {
    for (int i = 0; i < elements; i++) out[i] = mask[i] ? frame[i] : bg[i];
    return;
}

// main fn
int main(int argc, char** argv) {
    // read run parameters-height, width, iteration, image path
    if (argc < 3) {
        std::stringstream errorMessage;
        errorMessage << argv[0] << " <xclbin> <inputImage>  "
                                   "[width] [height] [iterations]";
        std::cerr << errorMessage.str();
        throw std::invalid_argument(errorMessage.str());
    }

    // Initializa device
    const char* xclBinName = argv[1];
    xF::deviceInit(xclBinName);

    // Read image
    cv::Mat srcImage;
    srcImage = cv::imread(argv[2], 0);

    int width = srcImage.cols;
    if (argc >= 4) width = atoi(argv[3]);

    int height = srcImage.rows;
    if (argc >= 5) height = atoi(argv[4]);

    int iterations = 1;
    if (argc >= 6) iterations = atoi(argv[5]);

    // Resize image if need be
    if ((width != srcImage.cols) || (height != srcImage.rows)) cv::resize(srcImage, srcImage, cv::Size(width, height));

    std::cout << "Image size" << std::endl;
    std::cout << srcImage.rows << std::endl;
    std::cout << srcImage.cols << std::endl;
    std::cout << srcImage.elemSize() << std::endl;
    std::cout << "Image size (end)" << std::endl;

    // allocate input buffer and transfer data

    // frame
    // void* frameData = nullptr;
    // xrt::bo frame_hndl = xrt::bo(xF::gpDhdl, (srcImage.total() * srcImage.elemSize()), 0, 0);
    // frameData = frame_hndl.map();
    // memcpy(frameData, srcImage.data, (srcImage.total() * srcImage.elemSize()));

    std::vector<uint8_t> frameData;
    frameData.assign(srcImage.data, (srcImage.data + srcImage.total() * srcImage.channels()));
    cv::Mat src1(srcImage.rows, srcImage.cols, CV_8UC1, (void*)frameData.data());

    // mask
    // void* maskData = nullptr;
    // xrt::bo mask_hndl = xrt::bo(xF::gpDhdl, (srcImage.total() * 1), 0, 0);
    // maskData = mask_hndl.map();

    std::vector<uint8_t> maskData;
    maskData.assign(srcImage.data, (srcImage.data + srcImage.total() * srcImage.channels()));
    cv::Mat src2(srcImage.rows, srcImage.cols, CV_8UC1, (void*)maskData.data());

    // uint8_t* temp = (uint8_t)maskData;
    // for (int i = 0; i < srcImage.total(); i++) temp[i] = rand() % 2;

    // background
    // void* bgData = nullptr;
    // xrt::bo bg_hndl = xrt::bo(xF::gpDhdl, (srcImage.total() * srcImage.elemSize()), 0, 0);
    // bgData = bg_hndl.map();

    std::vector<uint8_t> bgData;
    bgData.assign(srcImage.data, (srcImage.data + srcImage.total() * srcImage.channels()));
    cv::Mat src3(srcImage.rows, srcImage.cols, CV_8UC1, (void*)bgData.data());

    // uint8_t* temp_ = (uint8_t)bgData;
    // for (int i = 0; i < srcImage.total(); i++) temp_[i] = rand() % 256;

    // allocate output buffer and transfer data
    // void* outData = nullptr;
    // xrt::bo* out_hndl = new xrt::bo(xF::gpDhdl, (srcImage.total() * srcImage.elemSize() * 4), 0, 0);
    // outData = out_hndl->map();

    std::vector<uint8_t> outData;
    outData.assign(srcImage.rows * srcImage.cols * srcImage.channels(), 0);
    cv::Mat dst(srcImage.rows, srcImage.cols, CV_8UC1, (void*)outData.data());

    // declare tiler and sticher
    xF::xfcvDataMovers<xF::TILER, uint8_t, TILE_HEIGHT, TILE_WIDTH, 16, 1, 0, true> frame_tiler(0, 0);
    xF::xfcvDataMovers<xF::TILER, uint8_t, TILE_HEIGHT, TILE_WIDTH, 16, 1, 0, true> mask_tiler(0, 0);
    xF::xfcvDataMovers<xF::TILER, uint8_t, TILE_HEIGHT, TILE_WIDTH, 16, 1, 0, true> bg_tiler(0, 0);

    xF::xfcvDataMovers<xF::STITCHER, uint8_t, TILE_HEIGHT, TILE_WIDTH, 16, 1, 0, true> stitcher;

// tiler compute metadata
// ps.init();
#if !__X86_DEVICE__
    std::cout << "Graph init. This does nothing because CDO in boot PDI "
                 "already configures AIE.\n";
    auto gHndl = xrt::graph(xF::gpDhdl, xF::xclbin_uuid, "ps");
    std::cout << "XRT graph opened" << std::endl;
    gHndl.reset();
    std::cout << "Graph reset done" << std::endl;
#endif
    START_TIMER
    frame_tiler.compute_metadata(srcImage.size(), cv::Size(srcImage.cols, srcImage.rows));
    mask_tiler.compute_metadata(srcImage.size(), cv::Size(srcImage.cols, srcImage.rows));
    bg_tiler.compute_metadata(srcImage.size(), cv::Size(srcImage.cols, srcImage.rows));
    STOP_TIMER("Meta data compute time")

    // run graph for number of iterations
    std::chrono::microseconds tt(0);
    for (int i = 0; i < iterations; i++) {
        START_TIMER
        auto frame_tiles_sz = frame_tiler.host2aie_nb(frameData.data(), srcImage.size(), {"ps.in_f"});
        auto mask_tiles_sz = mask_tiler.host2aie_nb(maskData.data(), srcImage.size(), {"ps.in_m"});
        auto bg_tiles_sz = bg_tiler.host2aie_nb(bgData.data(), srcImage.size(), {"ps.in_b"});
#if !__X86_DEVICE__

        // for (int j = 0; j < frame_tiles_sz[0] * frame_tiles_sz[1]; j++) {
        // std::cout<<"Running graph for tile: " <<j;
        gHndl.run(frame_tiles_sz[0] * frame_tiles_sz[1]);
// gHndl.wait();
// std::cout<<"    [DONE:"<<j<<"]"<<std::endl;
// }
#endif
        stitcher.aie2host_nb(outData.data(), dst.size(), frame_tiles_sz, {"ps.out"});
        frame_tiler.wait({"ps.in_f"});
        mask_tiler.wait({"ps.in_m"});
        bg_tiler.wait({"ps.in_b"});
        std::cout << "Data transfer complete (Tiler)\n";
#if !__X86_DEVICE__
        gHndl.wait();
#endif
        stitcher.wait({"ps.out"});
        STOP_TIMER("pixelwise_select function")
        std::cout << "Data transfer complete (Stitcher)\n";
        tt += tdiff;
    }
#if !__X86_DEVICE__
    gHndl.end(0);
#endif
    std::cout << "Test passed" << std::endl;
    std::cout << "Average time to process frame : " << (((float)tt.count() * 0.001) / (float)iterations) << " ms"
              << std::endl;
    std::cout << "Average frames per second : " << (((float)1000000 / (float)tt.count()) * (float)iterations) << " fps"
              << std::endl;
    // save the output

    // run reference fn and compare output
    uint8_t* outref = (uint8_t*)malloc(srcImage.total());
    pixelwise_select_ref((uint8_t*)frameData.data(), (uint8_t*)maskData.data(), (uint8_t*)bgData.data(), outref,
                         srcImage.total());

    // Checking output
    int err = 0;
    uint8_t* od = (uint8_t*)outData.data();
    for (int i = 0; i < srcImage.total(); i++) {
        if (outref[i] != od[i]) {
            err++;
            std::cout << i << "\t:"
                      << "(" << unsigned(((uint8_t*)frameData.data())[i]) << ","
                      << unsigned(((uint8_t*)maskData.data())[i]) << "," << unsigned(((uint8_t*)bgData.data())[i])
                      << ")-->"
                      << "(" << unsigned(outref[i]) << "," << unsigned(od[i]) << ")" << std::endl;
        }
    }

    if (err) {
        std::cout << "Test failed" << std::endl;
        exit(-1);
    }

    // print result
    // TODO: print result
    std::cout << "Test passed" << std::endl;
}
