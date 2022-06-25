/*
 * Copyright 2019 Xilinx, Inc.
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
#include <ap_int.h>
#include <fstream>
#include <hls_math.h>
#include <iomanip>
#include <iostream>
//#include "xcl2.hpp"
#include "utils.hpp"
#include "kernel_resize.hpp"
#include "xf_utils_sw/logger.hpp"

using namespace std;

int main(int argc, char* argv[]) {
    int nerror = 0;

    xf::common::utils_sw::Logger logger(std::cout, std::cerr);
    cl_int fail;

    ArgParser parser(argc, (const char**)argv);

    std::string infile;
    if (!parser.getCmdOption("-i", infile)) {
        std::cout << "ERROR: input file path is not set!\n";
        return 1;
    }

    std::string outfile(infile);
    std::size_t found = outfile.find_last_of(".");
    outfile.insert(found, "_resized");

    ap_uint<32> src_width, src_height;
    std::string input_width;
    if (!parser.getCmdOption("-srcw", input_width)) {
        std::cout << "INFO: image input width is not set!" << std::endl;
    } else {
        src_width = std::stoi(input_width);
    }

    std::string input_height;
    if (!parser.getCmdOption("-srch", input_height)) {
        std::cout << "INFO: image input height is not set!" << std::endl;
    } else {
        src_height = std::stoi(input_height);
    }

    ap_uint<32> dst_width, dst_height;
    std::string output_width;
    if (!parser.getCmdOption("-dstw", output_width)) {
        std::cout << "INFO: image output width is not set!" << std::endl;
    } else {
        dst_width = std::stoi(output_width);
    }

    std::string output_height;
    if (!parser.getCmdOption("-dsth", output_height)) {
        std::cout << "INFO: image output height is not set!" << std::endl;
    } else {
        dst_height = std::stoi(output_height);
    }

    if (src_width < dst_width || src_height < dst_height) {
        std::cout << "WARNING: The output size is invaild!\n";
        return 1;
    }

    ap_uint<WDATA> pixel_64;
    ap_uint<72> pixel_72;
    ap_uint<WBIT> srcPixel;
    ap_uint<WBIT> tmpDst;

    ap_uint<32>* configs = aligned_alloc<ap_uint<32> >(4 + 1);
    configs[0] = src_width;
    configs[1] = src_height;
    configs[2] = dst_width;
    configs[3] = dst_height;

    ap_uint<WDATA>* axi_src = aligned_alloc<ap_uint<WDATA> >(MAX_SRC);
    ap_uint<WDATA>* axi_dst = aligned_alloc<ap_uint<WDATA> >(MAX_DST);

    // read raw data
    FILE* fp;

    if ((fp = fopen(infile.c_str(), "rb")) == NULL) {
        cout << "Error reading file fail." << '\n' << "Please check the path: " << infile << endl;
        exit(1);
    } else {
        cout << endl << "Read image successfully." << endl;
    }

#if NPPC == 1
    for (int i = 0; i < src_width * src_height; i++) {
        fread(&srcPixel, 1, 1, fp);
        axi_src[i] = srcPixel;
    }
#else
    for (int i = 0; i < src_width * src_height; i++) {
        fread(&srcPixel, 1, 1, fp); // std::cout<< "pixel:" << srcPixel << std::endl;
        pixel_64.range((i % 8) * WBIT + WBIT - 1, (i % 8) * WBIT) = srcPixel.range(WBIT - 1, 0);
        if ((i + 1) % 8 == 0) axi_src[i / 8] = pixel_64;
    }
#endif
    if (fp != NULL) fclose(fp);

    // send task requests
    auto configs_pool = resize_acc::create_bufpool(vpp::input);
    auto axi_src_pool = resize_acc::create_bufpool(vpp::input);
    auto axi_dst_pool = resize_acc::create_bufpool(vpp::output);
    resize_acc::send_while([&]() -> bool {
        ap_uint<32>* acc_configs = (ap_uint<32>*)resize_acc::alloc_buf(configs_pool, sizeof(ap_uint<32>) * 5);
        ap_uint<WDATA>* acc_axi_src =
            (ap_uint<WDATA>*)resize_acc::alloc_buf(axi_src_pool, sizeof(ap_int<WDATA>) * MAX_SRC);
        ap_uint<WDATA>* acc_axi_dst =
            (ap_uint<WDATA>*)resize_acc::alloc_buf(axi_dst_pool, sizeof(ap_int<WDATA>) * MAX_DST);

        memcpy(acc_configs, configs, sizeof(ap_uint<32>) * 5);
        memcpy(acc_axi_src, axi_src, sizeof(ap_int<WDATA>) * MAX_SRC);

        resize_acc::compute(acc_configs, acc_axi_src, acc_axi_dst);

        return 0;
    });

    // send result receiving requests
    resize_acc::receive_all_in_order([&]() {
        ap_uint<WDATA>* acc_axi_dst = (ap_uint<WDATA>*)resize_acc::get_buf(axi_dst_pool);
        memcpy(axi_dst, acc_axi_dst, sizeof(ap_int<WDATA>) * MAX_DST);
    });

    struct timeval start_time, end_time;
    gettimeofday(&start_time, 0);
    resize_acc::join();
    gettimeofday(&end_time, 0);

    std::cout << "INFO: kernel end------" << std::endl;
    std::cout << "INFO: Execution time " << tvdiff(&start_time, &end_time) / 1000.0 << "ms" << std::endl;

    FILE* outFile;
    if ((outFile = fopen(outfile.c_str(), "wb")) == NULL) {
        cout << "Error writing file fail." << '\n' << "Please check the path: " << outfile << endl;
        exit(1);
    }
#if NPPC == 1
    for (int j = 0; j < dst_width * dst_height; j++) {
        tmpDst = axi_dst[j];
        fwrite(&tmpDst, 1, 1, outFile);
    }
#else
    for (int i = 0; i < DivCeil(dst_width * dst_height, WBIT); i++) {
        pixel_64 = axi_dst[i];
        for (int j = 0; j < WBIT; j++) {
            tmpDst.range(WBIT - 1, 0) = pixel_64.range(j * WBIT + WBIT - 1, j * WBIT);
            if ((i * 8 + j) < (dst_width * dst_height)) {
                fwrite(&tmpDst, 1, 1, outFile);
                // std::cout << (int)tmpDst << std::endl;
            }
        }
    }
#endif
    if (outFile != NULL) fclose(outFile);

    if (nerror) {
        std::cout << "\nFAIL: nerror= " << nerror << " errors found.\n";
        logger.error(xf::common::utils_sw::Logger::Message::TEST_FAIL);
    } else {
        std::cout << "The src image size is " << src_width << "*" << src_height << ".\nThe dst image size is "
                  << dst_width << "*" << dst_height << ".\n"
                  << "Image resized successfully." << std::endl;
        std::cout << "PASS: no error found.\n";
        logger.info(xf::common::utils_sw::Logger::Message::TEST_PASS);
    }

    free(configs);
    free(axi_src);
    free(axi_dst);
    return nerror;
}
