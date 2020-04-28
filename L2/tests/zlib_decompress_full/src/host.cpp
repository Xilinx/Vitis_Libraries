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

void xil_decompress_top(
    std::string& decompress_mod, int cu, std::string& single_bin, uint8_t deviceId, uint8_t max_cr) {
    // Xilinx ZLIB object
    xil_zlib xlz(single_bin, 0, max_cr, deviceId, FULL);

    std::cout << std::fixed << std::setprecision(2) << "E2E\t\t\t:";

    std::ifstream inFile(decompress_mod.c_str(), std::ifstream::binary);
    if (!inFile) {
        std::cout << "Unable to open file";
        exit(1);
    }
    uint32_t input_size = get_file_size(inFile);

    const char* sizes[] = {"B", "kB", "MB", "GB", "TB"};
    double len = input_size;
    int order = 0;
    while (len >= 1000) {
        order++;
        len = len / 1000;
    }

    std::string lz_decompress_in = decompress_mod;
    std::string lz_decompress_out = decompress_mod;
    lz_decompress_out = lz_decompress_out + ".raw";

    xlz.decompress_file(lz_decompress_in, lz_decompress_out, input_size, cu);
    std::cout << std::fixed << std::setprecision(3) << std::endl
              << "File Size(" << sizes[order] << ")\t\t:" << len << std::endl
              << "File Name\t\t:" << lz_decompress_in << std::endl;
}

int main(int argc, char* argv[]) {
    int cu_run;
    sda::utils::CmdLineParser parser;
    parser.addSwitch("--xclbin", "-dx", "XCLBIN", "compress");
    parser.addSwitch("--decompress", "-d", "DeCompress", "");
    parser.addSwitch("--device", "-dev", "FPGA Card # to be used", "");

    parser.addSwitch("--file_list", "-l", "List of Input Files", "");
    parser.addSwitch("--cu", "-k", "CU", "0");
    parser.addSwitch("--max_cr", "-mcr", "Maximum CR", "10");
    parser.parse(argc, argv);

    std::string decompress_bin = parser.value("xclbin");
    std::string decompress_mod = parser.value("decompress");
    std::string cu = parser.value("cu");
    std::string dev_id_str = parser.value("device");
    std::string mcr = parser.value("max_cr");

    uint8_t max_cr_val = 0;
    if (!(mcr.empty())) {
        max_cr_val = atoi(mcr.c_str());
    } else {
        // Default block size
        max_cr_val = MAX_CR;
    }

    int deviceId = 0;
    if (!dev_id_str.empty()) { // check device Id to run on
        deviceId = atoi(dev_id_str.c_str());
    }

    if (cu.empty()) {
        printf("please give -k option for cu\n");
        exit(0);
    } else {
        cu_run = atoi(cu.c_str());
    }

    if (!decompress_mod.empty()) {
        // "-d" - DeCompress Mode
        xil_decompress_top(decompress_mod, cu_run, decompress_bin, deviceId, max_cr_val);
    }
}
