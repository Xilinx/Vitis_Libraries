/*
 * (c) Copyright 2020 Xilinx, Inc. All rights reserved.
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
#include "zstd.hpp"
#include <fstream>
#include <vector>
#include "cmdlineparser.h"

void xil_validate(std::string& file_list, std::string& ext) {
    std::cout << "\n";
    std::cout << "Status\t\tFile Name" << std::endl;
    std::cout << "\n";

    std::ifstream infilelist_val(file_list.c_str());
    std::string line_val;

    while (std::getline(infilelist_val, line_val)) {
        std::string line_in = line_val;
        std::string line_out = line_in + ext;

        int ret = 0;
        // Validate input and output files
        ret = validate(line_in, line_out);
        if (ret == 0) {
            std::cout << (ret ? "FAILED\t" : "PASSED\t") << "\t" << line_in << std::endl;
        } else {
            std::cout << "Validation Failed" << line_out.c_str() << std::endl;
            exit(1);
        }
    }
    infilelist_val.close();
}

void xil_decompress_top(std::string& decompress_mod, std::string& decompress_bin, uint8_t deviceId) {
    // Xilinx ZSTD object
    xil_zstd xlz(decompress_bin, MAX_CR, deviceId);

    std::cout << std::fixed << std::setprecision(2) << "KT(MBps)\t\t:";

    std::ifstream inFile(decompress_mod.c_str(), std::ifstream::binary);
    if (!inFile) {
        std::cout << "Unable to open file";
        exit(1);
    }
    uint32_t input_size = get_file_size(inFile);

    std::string lz_decompress_in = decompress_mod;
    std::string lz_decompress_out = decompress_mod;
    lz_decompress_out = lz_decompress_out + ".orig";

    // Call ZSTD decompression
    xlz.decompress_file(lz_decompress_in, lz_decompress_out, input_size);
    std::cout << std::fixed << std::setprecision(3) << std::endl
              << "File Size(MB)\t\t:" << (double)input_size / 1000000 << std::endl
              << "File Name\t\t:" << lz_decompress_in << std::endl;
}

void xil_decompress_list(std::string& file_list, std::string& decompress_bin, uint8_t deviceId) {
    std::string ext2;

    // Xilinx ZLIB Decompression
    ext2 = ".zst";

    std::ifstream infilelist_dec(file_list.c_str());
    std::string line_dec;

    std::cout << "\n";
    std::cout << "--------------------------------------------------------------" << std::endl;
    std::cout << "                     Xilinx Zstd DeCompress                   " << std::endl;
    std::cout << "--------------------------------------------------------------" << std::endl;
    std::cout << "\n";
    std::cout << "KT(MBps)\tFile Size(MB)\t\tFile Name" << std::endl;
    std::cout << "\n";

    xil_zstd xlz(decompress_bin, MAX_CR, deviceId);
    // Decompress list of files
    while (std::getline(infilelist_dec, line_dec)) {
        std::string file_line = line_dec;
        file_line = file_line + ext2;

        std::ifstream inFile_dec(file_line.c_str(), std::ifstream::binary);
        if (!inFile_dec) {
            std::cout << "Unable to open file";
            exit(1);
        }

        uint64_t input_size = get_file_size(inFile_dec);
        inFile_dec.close();

        std::string decompress_in = file_line;
        std::string decompress_out = file_line;
        decompress_out = decompress_out + ".orig";

        // Call Zlib decompression
        xlz.decompress_file(decompress_in, decompress_out, input_size);

        std::cout << std::fixed << std::setprecision(3) << "\t\t" << (double)input_size / 1000000 << "\t\t"
                  << decompress_in << std::endl;
    }
    infilelist_dec.close();
    // Validate
    std::cout << "\n";
    std::cout << "----------------------------------------------------------------------------------------"
              << std::endl;
    std::cout << "                       Validate: Original vs Xilinx Zlib Decompress           " << std::endl;
    std::cout << "----------------------------------------------------------------------------------------"
              << std::endl;
    std::string ext3 = ext2 + ".orig";
    xil_validate(file_list, ext3);
}

int main(int argc, char* argv[]) {
    sda::utils::CmdLineParser parser;
    parser.addSwitch("--decompress_xclbin", "-dx", "Decompress XCLBIN", "decompress");
    parser.addSwitch("--decompress", "-d", "decompress", "");
    parser.addSwitch("--device", "-dev", "FPGA Card # to be used", "");
    parser.addSwitch("--file_list", "-l", "List of Input Files", "");
    parser.parse(argc, argv);

    std::string decompress_mod = parser.value("decompress");
    std::string decompress_bin = parser.value("decompress_xclbin");
    std::string dev_id_str = parser.value("device");
    std::string filelist = parser.value("file_list");

    int deviceId = 0;
    if (!dev_id_str.empty()) { // check device Id to run on
        deviceId = atoi(dev_id_str.c_str());
    }
    if (!filelist.empty()) {
        xil_decompress_list(filelist, decompress_bin, deviceId);
    } else if (!decompress_mod.empty()) {
        // "-d" - DeCompress Mode
        xil_decompress_top(decompress_mod, decompress_bin, deviceId);
    }
}
