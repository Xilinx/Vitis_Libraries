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

void xil_decompress_list(
    std::string& file_list, std::string& ext, std::string& decompress_bin, uint8_t deviceId, uint8_t max_cr_val) {
    // Xilinx ZLIB object
    xil_zlib xlz(decompress_bin, 0, max_cr_val, deviceId);

    // Decompress
    std::ifstream infilelist_dec(file_list.c_str());
    std::string line_dec;

    std::cout << "\n";
    std::cout << "--------------------------------------------------------------" << std::endl;
    std::cout << "                     Xilinx Zlib DeCompress                       " << std::endl;
    std::cout << "--------------------------------------------------------------" << std::endl;
    std::cout << "\n";
    std::cout << "E2E(MBps)\tFile Size(MB)\t\tFile Name" << std::endl;
    std::cout << "\n";

    // Decompress list of files
    while (std::getline(infilelist_dec, line_dec)) {
        std::string file_line = line_dec;
        file_line = file_line + ext;

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
    } // While loop ends
}

void xil_decompress_top(std::string& decompress_mod, std::string& single_bin, uint8_t deviceId, uint8_t max_cr) {
    // Xilinx ZLIB object
    xil_zlib xlz(single_bin, 0, max_cr, deviceId);

    std::cout << std::fixed << std::setprecision(2) << "KT(MBps)\t\t:";

    std::ifstream inFile(decompress_mod.c_str(), std::ifstream::binary);
    if (!inFile) {
        std::cout << "Unable to open file";
        exit(1);
    }
    uint32_t input_size = get_file_size(inFile);
    if (input_size == 0) {
        std::cout << "\n";
        std::cout << "Invalid File Size: " << input_size << " Bytes" << std::endl;
        return;
    }
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

    xlz.decompress_file(lz_decompress_in, lz_decompress_out, input_size);
    std::cout << std::fixed << std::setprecision(3) << std::endl
              << "File Size(" << sizes[order] << ")\t\t:" << len << std::endl
              << "File Name\t\t:" << lz_decompress_in << std::endl;
}

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
}

void xil_batch_verify(std::string& file_list, std::string& decompress_bin, uint8_t deviceId, uint8_t max_cr_val) {
    std::string ext;

    // Xilinx ZLIB De-compression
    ext = ".xe2xd.zlib";

    xil_decompress_list(file_list, ext, decompress_bin, deviceId, max_cr_val);

    // Validate
    std::cout << "\n";
    std::cout << "----------------------------------------------------------------------------------------"
              << std::endl;
    std::cout << "                       Validate: Xilinx Zlib Compress vs Xilinx Zlib Decompress           "
              << std::endl;
    std::cout << "----------------------------------------------------------------------------------------"
              << std::endl;
    std::string origExt = ".xe2xd.zlib.orig";
    xil_validate(file_list, origExt);
}

int main(int argc, char* argv[]) {
    int cu_run;
    sda::utils::CmdLineParser parser;
    parser.addSwitch("--xclbin", "-dx", "XCLBIN", "");
    parser.addSwitch("--decompress", "-d", "DeCompress", "");
    parser.addSwitch("--device", "-dev", "FPGA Card # to be used", "");
    parser.addSwitch("--file_list", "-l", "List of Input Files", "");
    parser.addSwitch("--max_cr", "-mcr", "Maximum CR", "10");
    parser.parse(argc, argv);

    std::string decompress_bin = parser.value("xclbin");
    std::string decompress_mod = parser.value("decompress");
    std::string dev_id_str = parser.value("device");
    std::string mcr = parser.value("max_cr");
    std::string filelist = parser.value("file_list");

    uint8_t max_cr_val = 0;
    if (!(mcr.empty())) {
        max_cr_val = atoi(mcr.c_str());
    } else {
        // Default block size
        max_cr_val = MAX_CR;
    }

    if (decompress_bin.empty()) {
        std::string opt = "input XCLBIN file by using (-dx)";
        error_message(opt);
        exit(1);
    }

    if (decompress_mod.empty() && filelist.empty()) {
        std::string opt = "input file to decompress by using (-d) or filelist (-l)";
        error_message(opt);
        exit(1);
    }

    int deviceId = 0;
    if (!dev_id_str.empty()) { // check device Id to run on
        deviceId = atoi(dev_id_str.c_str());
    }

    if (!filelist.empty()) {
        // "-l" - List of files
        xil_batch_verify(filelist, decompress_bin, deviceId, max_cr_val);
    } else if (!decompress_mod.empty()) {
        // "-d" - DeCompress Mode
        xil_decompress_top(decompress_mod, decompress_bin, deviceId, max_cr_val);
    }
}
