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

void xil_compress_top(std::string& compress_mod, std::string& single_bin) {
    // Xilinx ZLIB object
    xil_zlib* xlz;
    xlz = new xil_zlib(single_bin, 0);

    // For compression m_bin_flow = 0
    xlz->m_bin_flow = 0;

    std::cout << std::fixed << std::setprecision(2) << "E2E\t\t\t:";

    std::ifstream inFile(compress_mod.c_str(), std::ifstream::binary);
    if (!inFile) {
        std::cout << "Unable to open file";
        exit(1);
    }
    uint32_t input_size = get_file_size(inFile);

    std::string lz_compress_in = compress_mod;
    std::string lz_compress_out = compress_mod;
    lz_compress_out = lz_compress_out + ".zlib";

    // Call ZLIB compression
    uint32_t enbytes = xlz->compress_file(lz_compress_in, lz_compress_out, input_size);

    std::cout.precision(3);
    std::cout << std::fixed << std::setprecision(2) << std::endl
              << "ZLIB_CR\t\t\t:" << (double)input_size / enbytes << std::endl
              << std::fixed << std::setprecision(3) << "File Size(MB)\t\t:" << (double)input_size / 1000000 << std::endl
              << "File Name\t\t:" << lz_compress_in << std::endl;
    std::cout << "\n";
    std::cout << "Output Location: " << lz_compress_out.c_str() << std::endl;
}

int main(int argc, char* argv[]) {
    int cu_run;
    sda::utils::CmdLineParser parser;
    parser.addSwitch("--xclbin", "-cx", "XCLBIN", "compress");
    parser.addSwitch("--compress", "-c", "Compress", "");

    parser.addSwitch("--file_list", "-l", "List of Input Files", "");
    parser.addSwitch("--cu", "-k", "CU", "0");
    parser.parse(argc, argv);

    std::string compress_bin = parser.value("xclbin");
    std::string compress_mod = parser.value("compress");
    std::string filelist = parser.value("file_list");
    std::string cu = parser.value("cu");

    if (cu.empty()) {
        printf("please give -k option for cu\n");
        exit(0);
    } else {
        cu_run = atoi(cu.c_str());
    }

    if (!compress_mod.empty()) xil_compress_top(compress_mod, compress_bin);
}
