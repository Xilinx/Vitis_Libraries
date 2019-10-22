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

void xil_decompress_top(std::string& decompress_mod, int cu, std::string& single_bin) {
    // Xilinx ZLIB object
    xil_zlib* xlz;
    xlz = new xil_zlib(single_bin, 1);
    // For Decompression m_bin_flow = 0
    xlz->m_bin_flow = 1;
    std::cout << std::fixed << std::setprecision(2) << "E2E\t\t\t:";

    std::ifstream inFile(decompress_mod.c_str(), std::ifstream::binary);
    if (!inFile) {
        std::cout << "Unable to open file";
        exit(1);
    }
    uint32_t input_size = get_file_size(inFile);

    std::string lz_decompress_in = decompress_mod;
    std::string lz_decompress_out = decompress_mod;
    lz_decompress_out = lz_decompress_out + ".raw";

    // uint32_t enbytes =
    xlz->decompress_file(lz_decompress_in, lz_decompress_out, input_size, cu);
    std::cout << std::fixed << std::setprecision(3) << std::endl
              << "File Size(MB)\t\t:" << (double)input_size / 1000000 << std::endl
              << "File Name\t\t:" << lz_decompress_in << std::endl;
}

int main(int argc, char* argv[]) {
    int cu_run;
    sda::utils::CmdLineParser parser;
    parser.addSwitch("--xclbin", "-dx", "XCLBIN", "compress");
    parser.addSwitch("--decompress", "-d", "DeCompress", "");

    parser.addSwitch("--file_list", "-l", "List of Input Files", "");
    parser.addSwitch("--cu", "-k", "CU", "0");
    parser.parse(argc, argv);

    std::string decompress_bin = parser.value("xclbin");
    std::string decompress_mod = parser.value("decompress");
    std::string cu = parser.value("cu");

    if (cu.empty()) {
        printf("please give -k option for cu\n");
        exit(0);
    } else {
        cu_run = atoi(cu.c_str());
    }

    if (!decompress_mod.empty()) {
        // "-d" - DeCompress Mode
        xil_decompress_top(decompress_mod, cu_run, decompress_bin);
    }
}
