/*
 * (c) Copyright 2019 Xilinx, Inc. All rights reserved.
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
 *
 */
#include "zlib.hpp"
#include <fstream>
#include <vector>
#include "cmdlineparser.h"

void xil_compress_top(std::string& compress_mod, std::string& single_bin, uint8_t deviceId, uint8_t max_cr) {
    // Xilinx ZLIB object
    xil_zlib xlz(single_bin, 1, max_cr, deviceId);

    std::cout << std::fixed << std::setprecision(2) << "KT(MBps)\t\t:";

    std::ifstream inFile(compress_mod.c_str(), std::ifstream::binary);
    if (!inFile) {
        std::cout << "Unable to open file";
        exit(1);
    }

    uint64_t input_size = get_file_size(inFile);

    const char* sizes[] = {"B", "kB", "MB", "GB", "TB"};
    double len = input_size;
    int order = 0;
    while (len >= 1000) {
        order++;
        len = len / 1000;
    }

    std::string lz_compress_in = compress_mod;
    std::string lz_compress_out = compress_mod;
    lz_compress_out = lz_compress_out + ".zlib";

    // Call ZLIB compression
    uint32_t enbytes = xlz.compress_file(lz_compress_in, lz_compress_out, input_size);

    if (enbytes > 0) {
        std::cout.precision(3);
        std::cout << std::fixed << std::setprecision(2) << std::endl
                  << "ZLIB_CR\t\t\t:" << (double)input_size / enbytes << std::endl
                  << std::fixed << std::setprecision(3) << "File Size(" << sizes[order] << ")\t\t:" << len << std::endl
                  << "File Name\t\t:" << lz_compress_in << std::endl;
        std::cout << "\n";
        std::cout << "Output Location: " << lz_compress_out.c_str() << std::endl;
    }
}

int main(int argc, char* argv[]) {
    sda::utils::CmdLineParser parser;
    parser.addSwitch("--xclbin", "-cx", "XCLBIN", "");
    parser.addSwitch("--compress", "-c", "Compress", "");
    parser.addSwitch("--file_list", "-l", "List of Input Files", "");
    parser.addSwitch("--device", "-dev", "FPGA Card # to be used", "");
    parser.addSwitch("--max_cr", "-mcr", "Maximum CR", "10");
    parser.parse(argc, argv);

    std::string compress_bin = parser.value("xclbin");
    std::string compress_mod = parser.value("compress");
    std::string filelist = parser.value("file_list");
    std::string dev_id_str = parser.value("device");
    std::string mcr = parser.value("max_cr");

    uint8_t max_cr_val = 0;
    if (!(mcr.empty())) {
        max_cr_val = atoi(mcr.c_str());
    } else {
        // Default block size
        max_cr_val = MAX_CR;
    }

    if (compress_bin.empty()) {
        std::string opt = "input XCLBIN file by using (-cx)";
        error_message(opt);
        exit(1);
    }

    if (compress_mod.empty()) {
        std::string opt = "input file to compress by using (-c)";
        error_message(opt);
        exit(1);
    }

    int deviceId = 0;
    if (!dev_id_str.empty()) { // check device Id to run on
        deviceId = atoi(dev_id_str.c_str());
    }

    if (!compress_mod.empty()) xil_compress_top(compress_mod, compress_bin, deviceId, max_cr_val);
}
