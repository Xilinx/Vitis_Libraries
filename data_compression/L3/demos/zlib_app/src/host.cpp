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
#include "zlib_old.hpp"
#include <fstream>
#include <vector>
#include "cmdlineparser.h"

using namespace xf::compression;

void xil_validate(std::string& file_list, std::string& ext);

void xil_compress_decompress_list(std::string& file_list,
                                  std::string& ext1,
                                  std::string& ext2,
                                  int cu,
                                  std::string& single_bin,
                                  uint8_t max_cr,
                                  enum list_mode mode = COMP_DECOMP) {
    // Create xfZlib object
    xfZlib xlz(single_bin, max_cr, BOTH, 0, 0, DYNAMIC, XILINX_ZLIB);
    ERROR_STATUS(xlz.error_code());

    if (mode != ONLY_DECOMPRESS) {
        std::cout << "--------------------------------------------------------------" << std::endl;
        std::cout << "                     Xilinx Zlib Compress                          " << std::endl;
        std::cout << "--------------------------------------------------------------" << std::endl;

        std::cout << "\n";
        std::cout << "E2E(MBps)\tCR\t\tFile Size(MB)\t\tFile Name" << std::endl;
        std::cout << "\n";

        std::ifstream infilelist(file_list.c_str());
        std::string line;

        // Compress list of files
        // This loop does LZ4 compression on list
        // of files.
        while (std::getline(infilelist, line)) {
            std::ifstream inFile(line.c_str(), std::ifstream::binary);
            if (!inFile) {
                std::cout << "Unable to open file";
                exit(1);
            }

            uint64_t input_size = get_file_size(inFile);
            inFile.close();

            std::string compress_in = line;
            std::string compress_out = line;
            compress_out = compress_out + ext1;

            // Call Zlib compression
            uint64_t enbytes = xlz.compress_file(compress_in, compress_out, input_size);

            std::cout << "\t\t" << (double)input_size / enbytes << "\t\t" << std::fixed << std::setprecision(3)
                      << (double)input_size / 1000000 << "\t\t\t" << compress_in << std::endl;
        }
    }

    // Decompress
    if (mode != ONLY_COMPRESS) {
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
            xlz.decompress_file(decompress_in, decompress_out, input_size, cu);

            std::cout << std::fixed << std::setprecision(3) << "\t\t" << (double)input_size / 1000000 << "\t\t"
                      << decompress_in << std::endl;
        } // While loop ends
    }
}

void xil_batch_verify(
    std::string& file_list, int cu, enum list_mode mode, std::string& single_bin, uint8_t max_cr, uint8_t device_id) {
    std::string ext1;
    std::string ext2;

    // Xilinx ZLIB Compression
    ext1 = ".xe2xd.zlib";
    ext2 = ".xe2xd.zlib";

    xil_compress_decompress_list(file_list, ext1, ext2, cu, single_bin, max_cr, mode);

    // Validate
    std::cout << "\n";
    std::cout << "----------------------------------------------------------------------------------------"
              << std::endl;
    std::cout << "                       Validate: Xilinx Zlib Compress vs Xilinx Zlib Decompress           "
              << std::endl;
    std::cout << "----------------------------------------------------------------------------------------"
              << std::endl;
    std::string ext3 = ".xe2xd.zlib.orig";
    xil_validate(file_list, ext3);
}

void xil_decompress_top(
    std::string& decompress_mod, int cu, std::string& single_bin, uint8_t max_cr, uint8_t device_id) {
    // Xilinx ZLIB object
    xfZlib xlz(single_bin, max_cr, DECOMP_ONLY, device_id, 0, DYNAMIC, XILINX_ZLIB);
    ERROR_STATUS(xlz.error_code());

    std::cout << std::fixed << std::setprecision(2) << "E2E(MBps)\t\t:";

    std::ifstream inFile(decompress_mod.c_str(), std::ifstream::binary);
    if (!inFile) {
        std::cout << "Unable to open file";
        exit(1);
    }
    uint32_t input_size = get_file_size(inFile);

    std::string lz_decompress_in = decompress_mod;
    std::string lz_decompress_out = decompress_mod;
    lz_decompress_out = lz_decompress_out + ".raw";

    const char* sizes[] = {"B", "kB", "MB", "GB", "TB"};
    double len = input_size;
    int order = 0;
    while (len >= 1000) {
        order++;
        len = len / 1000;
    }

    // Call ZLIB compression
    // uint32_t enbytes =
    xlz.decompress_file(lz_decompress_in, lz_decompress_out, input_size, cu);
    std::cout << std::fixed << std::setprecision(3) << std::endl
              << "File Size(" << sizes[order] << ")\t\t:" << len << std::endl
              << "File Name\t\t:" << lz_decompress_in << std::endl;
}

void xil_compress_top(std::string& compress_mod, std::string& single_bin, uint8_t max_cr, uint8_t device_id) {
    // Xilinx ZLIB object
    xfZlib xlz(single_bin, max_cr, COMP_ONLY, device_id, 0, DYNAMIC, XILINX_ZLIB);
    ERROR_STATUS(xlz.error_code());

    std::cout << std::fixed << std::setprecision(2) << "E2E(MBps)\t\t:";

    std::ifstream inFile(compress_mod.c_str(), std::ifstream::binary);
    if (!inFile) {
        std::cout << "Unable to open file";
        exit(1);
    }
    uint32_t input_size = get_file_size(inFile);

    std::string lz_compress_in = compress_mod;
    std::string lz_compress_out = compress_mod;
    lz_compress_out = lz_compress_out + ".zlib";

    const char* sizes[] = {"B", "kB", "MB", "GB", "TB"};
    double len = input_size;
    int order = 0;
    while (len >= 1000) {
        order++;
        len = len / 1000;
    }

    // Call ZLIB compression
    uint32_t enbytes = xlz.compress_file(lz_compress_in, lz_compress_out, input_size);

    std::cout.precision(3);
    std::cout << std::fixed << std::setprecision(2) << std::endl
              << "ZLIB_CR\t\t\t:" << (double)input_size / enbytes << std::endl
              << std::fixed << std::setprecision(3) << "File Size(" << sizes[order] << ")\t\t:" << len << std::endl
              << "File Name\t\t:" << lz_compress_in << std::endl;
    std::cout << "\n";
    std::cout << "Output Location: " << lz_compress_out.c_str() << std::endl;
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

void xilCompressDecompressTop(std::string& compress_decompress_mod,
                              std::string& single_bin,
                              uint8_t device_id,
                              uint8_t max_cr_val) {
    // Create xfZlib object
    xfZlib xlz(single_bin, max_cr_val, BOTH, device_id, 0, DYNAMIC, XILINX_ZLIB);
    ERROR_STATUS(xlz.error_code());

    std::cout << "--------------------------------------------------------------" << std::endl;
    std::cout << "                     Xilinx Zlib Compress                          " << std::endl;
    std::cout << "--------------------------------------------------------------" << std::endl;

    std::cout << "\n";

    std::cout << std::fixed << std::setprecision(2) << "E2E(MBps)\t\t:";

    std::ifstream inFile(compress_decompress_mod.c_str(), std::ifstream::binary);
    if (!inFile) {
        std::cout << "Unable to open file";
        exit(1);
    }

    uint64_t input_size = get_file_size(inFile);
    inFile.close();

    std::string compress_in = compress_decompress_mod;
    std::string compress_out = compress_decompress_mod;
    compress_out = compress_out + ".zlib";

    const char* sizes[] = {"B", "kB", "MB", "GB", "TB"};
    double len = input_size;
    int order = 0;
    while (len >= 1000) {
        order++;
        len = len / 1000;
    }

    // Call Zlib compression
    uint64_t enbytes = xlz.compress_file(compress_in, compress_out, input_size);
    std::cout << std::fixed << std::setprecision(2) << std::endl
              << "CR\t\t\t:" << (double)input_size / enbytes << std::endl
              << std::fixed << std::setprecision(3) << "File Size(" << sizes[order] << ")\t\t:" << len << std::endl
              << "File Name\t\t:" << compress_in << std::endl;

    // Decompress

    std::cout << "\n";
    std::cout << "--------------------------------------------------------------" << std::endl;
    std::cout << "                     Xilinx Zlib DeCompress                       " << std::endl;
    std::cout << "--------------------------------------------------------------" << std::endl;
    std::cout << "\n";

    std::cout << std::fixed << std::setprecision(2) << "E2E(MBps)\t\t:";

    // Decompress list of files

    std::string lz_decompress_in = compress_decompress_mod + ".zlib";
    std::string lz_decompress_out = compress_decompress_mod;
    lz_decompress_out = lz_decompress_in + ".orig";

    std::ifstream inFile_dec(lz_decompress_in.c_str(), std::ifstream::binary);
    if (!inFile_dec) {
        std::cout << "Unable to open file";
        exit(1);
    }

    input_size = get_file_size(inFile_dec);
    inFile_dec.close();

    // Call Zlib decompression
    xlz.decompress_file(lz_decompress_in, lz_decompress_out, input_size, 0);

    len = input_size;
    order = 0;
    while (len >= 1000) {
        order++;
        len = len / 1000;
    }

    std::cout << std::fixed << std::setprecision(2) << std::endl
              << std::fixed << std::setprecision(3) << "File Size(" << sizes[order] << ")\t\t:" << len << std::endl
              << "File Name\t\t:" << lz_decompress_in << std::endl;

    // Validate
    std::cout << "\n";

    std::string inputFile = compress_decompress_mod;
    std::string outputFile = compress_decompress_mod + ".zlib" + ".orig";
    int ret = validate(inputFile, outputFile);
    if (ret == 0) {
        std::cout << (ret ? "FAILED\t" : "PASSED\t") << "\t" << inputFile << std::endl;
    } else {
        std::cout << "Validation Failed" << outputFile.c_str() << std::endl;
    }
}

int main(int argc, char* argv[]) {
    int cu_run;
    sda::utils::CmdLineParser parser;
    parser.addSwitch("--compress", "-c", "Compress", "");
    parser.addSwitch("--decompress", "-d", "DeCompress", "");
    parser.addSwitch("--single_xclbin", "-sx", "Single XCLBIN", "single");
    parser.addSwitch("--compress_decompress", "-v", "Compress Decompress", "");

    parser.addSwitch("--file_list", "-l", "List of Input Files", "");
    parser.addSwitch("--cu", "-k", "CU", "0");
    parser.addSwitch("--id", "-id", "Device ID", "0");
    parser.addSwitch("--max_cr", "-mcr", "Maximum CR", "20");
    parser.parse(argc, argv);

    std::string compress_mod = parser.value("compress");
    std::string filelist = parser.value("file_list");
    std::string decompress_mod = parser.value("decompress");
    std::string single_bin = parser.value("single_xclbin");
    std::string compress_decompress_mod = parser.value("compress_decompress");
    std::string cu = parser.value("cu");
    std::string device_ids = parser.value("id");
    std::string mcr = parser.value("max_cr");

    uint8_t max_cr_val = 0;
    if (!(mcr.empty())) {
        max_cr_val = atoi(mcr.c_str());
    } else {
        // Default block size
        max_cr_val = MAX_CR;
    }

    uint8_t device_id = 0;
    if (!(device_ids.empty())) device_id = std::stoi(device_ids);

    if (cu.empty()) {
        printf("please give -k option for cu\n");
        exit(0);
    } else {
        cu_run = atoi(cu.c_str());
    }

    if (!compress_decompress_mod.empty())
        xilCompressDecompressTop(compress_decompress_mod, single_bin, device_id, max_cr_val);

    if (!filelist.empty()) {
        list_mode lMode;
        // "-l" - List of files
        if (!compress_mod.empty()) {
            lMode = ONLY_COMPRESS;
        } else if (!decompress_mod.empty()) {
            lMode = ONLY_DECOMPRESS;
        } else {
            lMode = COMP_DECOMP;
        }
        xil_batch_verify(filelist, cu_run, lMode, single_bin, max_cr_val, device_id);
    } else if (!compress_mod.empty()) {
        // "-c" - Compress Mode
        xil_compress_top(compress_mod, single_bin, max_cr_val, device_id);
    } else if (!decompress_mod.empty())
        // "-d" - DeCompress Mode
        xil_decompress_top(decompress_mod, cu_run, single_bin, max_cr_val, device_id);
}
