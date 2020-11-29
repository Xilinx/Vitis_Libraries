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
#include "zlib_compress.hpp"
#include <fstream>
#include <vector>
#include "cmdlineparser.h"

static uint64_t getFileSize(std::ifstream& file) {
    file.seekg(0, file.end);
    uint64_t file_size = file.tellg();
    file.seekg(0, file.beg);
    return file_size;
}

void xilCompressDecompressList(
    std::string& file_list, std::string& ext, bool flow, uint32_t block_size, std::string& compress_bin) {
    // Compression
    xfZlib xzl(compress_bin, block_size);

    std::ifstream infilelist(file_list.c_str());
    std::string line;

    if (!(infilelist.good())) {
        std::cout << "Unable to open the list of files" << std::endl;
        exit(1);
    }

    std::getline(infilelist, line);
    if (!line.length()) {
        std::cout << "Input list of file is empty" << std::endl;
        exit(1);
    }

    std::cout << "\n";
    std::cout << "--------------------------------------------------------------" << std::endl;
    std::cout << "                     Xilinx GZIP Compress                       " << std::endl;
    std::cout << "--------------------------------------------------------------" << std::endl;
    if (flow == 0) {
        std::cout << "\n";
        std::cout << "KT(MBps)\tCR\t\tFile Size(MB)\t\tFile Name" << std::endl;
        std::cout << "\n";
    }

    // Compress list of files
    do {
        std::string file_line = line;
        std::ifstream inFile(file_line.c_str(), std::ifstream::binary);
        if (!inFile) {
            std::cout << "Unable to open file";
            exit(1);
        }

        uint32_t input_size = getFileSize(inFile);
        inFile.close();

        std::string lz_in = file_line;
        std::string lz_out = file_line;
        lz_out = lz_out + ext;

        // Call Zlib compression
        uint64_t enbytes = xzl.compressFile(lz_in, lz_out, input_size);

        if (flow == 0) {
            std::cout << "\t\t" << (double)input_size / enbytes << "\t\t" << std::fixed << std::setprecision(3)
                      << (double)input_size / 1000000 << "\t\t\t" << lz_in << std::endl;
        }
    } while (std::getline(infilelist, line)); // While loop ends
}

void xilBatchVerify(std::string& file_list, uint32_t block_size, std::string& compress_bin) {
    // Start of Flow : Xilinx GZIP Compress with Standard GZIP Decompress

    std::string ext = ".gz";
    xilCompressDecompressList(file_list, ext, 0, block_size, compress_bin);
}

void xilCompressTop(std::string& compress_mod, uint32_t block_size, std::string& compress_bin) {
    // Xilinx Zlib object
    xfZlib xzl(compress_bin, block_size);

#ifdef VERBOSE
    std::cout << std::fixed << std::setprecision(2) << "KT(MBps)\t\t:";
#endif

    std::ifstream inFile(compress_mod.c_str(), std::ifstream::binary);
    if (!inFile) {
        std::cout << "Unable to open file";
        exit(1);
    }
    uint64_t input_size = getFileSize(inFile);
    inFile.close();

    const char* sizes[] = {"B", "kB", "MB", "GB", "TB"};
    double len = input_size;
    int order = 0;
    while (len >= 1000) {
        order++;
        len = len / 1000;
    }

    std::string zlib_compress_in = compress_mod;
    std::string zlib_compress_out = compress_mod;
#ifdef GZIP_MODE
    zlib_compress_out = zlib_compress_out + ".gz";
#else
    zlib_compress_out = zlib_compress_out + ".zlib";
#endif

#ifdef EVENT_PROFILE
    auto total_start = std::chrono::high_resolution_clock::now();
#endif
    // Call Zlib compression
    uint64_t enbytes = xzl.compressFile(zlib_compress_in, zlib_compress_out, input_size);
#ifdef EVENT_PROFILE
    auto total_end = std::chrono::high_resolution_clock::now();
    auto total_time_ns = std::chrono::duration<double, std::nano>(total_end - total_start);
#endif

#ifdef VERBOSE
    std::cout.precision(3);
    std::cout << std::fixed << std::setprecision(2) << std::endl
              << "CR\t\t\t:" << (double)input_size / enbytes << std::endl
              << std::fixed << std::setprecision(3) << "File Size(" << sizes[order] << ")\t\t:" << len << std::endl
              << "File Name\t\t:" << zlib_compress_in << std::endl;
    std::cout << "\n";
    std::cout << "Output Location: " << zlib_compress_out.c_str() << std::endl;
    len = enbytes;
    order = 0;
    while (len >= 1000) {
        order++;
        len = len / 1000;
    }
    std::cout << "Compressed file size(" << sizes[order] << ")\t\t:" << enbytes << std::endl;
#endif

#ifdef EVENT_PROFILE
    std::cout << "Total Time (milli sec): " << total_time_ns.count() / 1000000 << std::endl;
#endif
}

int main(int argc, char* argv[]) {
    sda::utils::CmdLineParser parser;
    parser.addSwitch("--xclbin", "-cx", "XCLBIN", "compress");
    parser.addSwitch("--compress", "-c", "Compress", "");
    parser.addSwitch("--file_list", "-l", "List of Input Files", "");
    parser.addSwitch("--block_size", "-B", "Compress Block Size [0-32: 1-64: 2-1024: 3-4096]", "0");
    parser.parse(argc, argv);

    std::string compress_bin = parser.value("xclbin");
    std::string compress_mod = parser.value("compress");
    std::string filelist = parser.value("file_list");
    std::string block_size = parser.value("block_size");

    uint32_t bSize = 0;
    // Block Size
    if (!(block_size.empty())) {
        bSize = atoi(block_size.c_str());

        switch (bSize) {
            case 0:
                bSize = 32;
                break;
            case 1:
                bSize = 64;
                break;
            case 2:
                bSize = 1024;
                break;
            case 3:
                bSize = 4096;
                break;
            default:
                std::cout << "Invalid Block Size provided" << std::endl;
                parser.printHelp();
                exit(1);
        }
    } else {
        // Default Block Size - 32KB
        bSize = BLOCK_SIZE_IN_KB;
    }

    // "-c" - Compress Mode
    if (!compress_mod.empty()) xilCompressTop(compress_mod, bSize, compress_bin);

    // "-l" List of Files
    if (!filelist.empty()) {
        xilBatchVerify(filelist, bSize, compress_bin);
    }
}
