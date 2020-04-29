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
#include "snappy.hpp"
#include <fstream>
#include <vector>
#include "cmdlineparser.h"

static uint64_t getFileSize(std::ifstream& file) {
    file.seekg(0, file.end);
    uint64_t file_size = file.tellg();
    file.seekg(0, file.beg);
    return file_size;
}

void xilCompressTop(std::string& compress_mod, uint32_t block_size, std::string& single_bin) {
    // Xilinx SNAPPY object
    xilSnappy xlz(single_bin, 1, block_size);

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

    std::string lz_compress_in = compress_mod;
    std::string lz_compress_out = compress_mod;
    lz_compress_out = lz_compress_out + ".snappy";

#ifdef EVENT_PROFILE
    auto total_start = std::chrono::high_resolution_clock::now();
#endif
    // Call SNAPPY compression
    uint64_t enbytes = xlz.compressFile(lz_compress_in, lz_compress_out, input_size, 0);
#ifdef EVENT_PROFILE
    auto total_end = std::chrono::high_resolution_clock::now();
    auto total_time_ns = std::chrono::duration<double, std::nano>(total_end - total_start);
#endif

#ifdef VERBOSE
    std::cout.precision(3);
    std::cout << std::fixed << std::setprecision(2) << std::endl
              << "SNAPPY_CR\t\t:" << (double)input_size / enbytes << std::endl
              << std::fixed << std::setprecision(3) << "File Size(" << sizes[order] << ")\t\t:" << len << std::endl
              << "File Name\t\t:" << lz_compress_in << std::endl;
    std::cout << "\n";
    std::cout << "Output Location: " << lz_compress_out.c_str() << std::endl;
    std::cout << "Compressed file size: " << enbytes << std::endl;
#endif

#ifdef EVENT_PROFILE
    std::cout << "Total Time (milli sec): " << total_time_ns.count() / 1000000 << std::endl;
#endif
}

void xilValidate(std::string& file_list, std::string& ext) {
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
            //        exit(1);
        }
    }
}

void xilCompressDecompressList(xilSnappy& xlz,
                               std::string& file_list,
                               std::string& ext1,
                               std::string& ext2,
                               bool c_flow,
                               bool d_flow,
                               uint32_t block_size) {
    // Compression

    std::cout << "\n";
    cout << "--------------------------------------------------------------" << std::endl;
    if (c_flow == 0)
        std::cout << "                     Xilinx Compress                          " << std::endl;
    else
        std::cout << "                     Standard Compress                        " << std::endl;
    std::cout << "--------------------------------------------------------------" << std::endl;

    if (c_flow == 0) {
        std::cout << "\n";
        std::cout << "KT(MBps)\tSNAPPY_CR\t\tFile Size(MB)\t\tFile Name" << std::endl;
        std::cout << "\n";
    } else {
        std::cout << "\n";
        std::cout << "File Size(MB)\t\tFile Name" << std::endl;
        std::cout << "\n";
    }

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

    // Compress list of files
    // This loop does SNAPPY compression on list
    // of files.
    do {
        std::ifstream inFile(line.c_str(), std::ifstream::binary);
        if (!inFile) {
            std::cout << "Unable to open file" << std::endl;
            exit(1);
        }

        uint64_t input_size = getFileSize(inFile);
        inFile.close();

        std::string lz_compress_in = line;
        std::string lz_compress_out = line;
        lz_compress_out = lz_compress_out + ext1;

        // Call SNAPPY compression
        uint64_t enbytes = xlz.compressFile(lz_compress_in, lz_compress_out, input_size, c_flow);
        if (c_flow == 0) {
            std::cout << "\t\t" << (double)input_size / enbytes << "\t\t\t" << std::fixed << std::setprecision(3)
                      << (double)input_size / 1000000 << "\t\t\t" << lz_compress_in << std::endl;
        } else {
            std::cout << std::fixed << std::setprecision(3);
            std::cout << (double)input_size / 1000000 << "\t\t\t" << lz_compress_in << std::endl;
        }
    } while (std::getline(infilelist, line));

    // De-Compression

    std::ifstream infilelist_dec(file_list.c_str());
    std::string line_dec;

    if (!(infilelist_dec.good())) {
        std::cout << "Unable to open the list of files" << std::endl;
        exit(1);
    }

    std::getline(infilelist_dec, line_dec);
    if (!line_dec.length()) {
        std::cout << "Input list of file is empty" << std::endl;
        exit(1);
    }

    std::cout << "\n";
    std::cout << "--------------------------------------------------------------" << std::endl;
    if (d_flow == 0)
        std::cout << "                     Xilinx De-Compress                       " << std::endl;
    else
        std::cout << "                     Standard De-Compress                     " << std::endl;

    std::cout << "--------------------------------------------------------------" << std::endl;
    if (d_flow == 0) {
        std::cout << "\n";
        std::cout << "KT(MBps)\tFile Size(MB)\t\tFile Name" << std::endl;
        std::cout << "\n";
    } else {
        std::cout << "\n";
        std::cout << "File Size(MB)\tFile Name" << std::endl;
        std::cout << "\n";
    }

    // Decompress list of files
    // This loop does SNAPPY decompress on list
    // of files.
    do {
        std::string file_line = line_dec;
        file_line = file_line + ext2;

        std::ifstream inFile_dec(file_line.c_str(), std::ifstream::binary);
        if (!inFile_dec) {
            std::cout << "Unable to open file";
            exit(1);
        }

        uint64_t input_size = getFileSize(inFile_dec);
        inFile_dec.close();

        std::string lz_decompress_in = file_line;
        std::string lz_decompress_out = file_line;
        lz_decompress_out = lz_decompress_out + ".orig";

        xlz.decompressFile(lz_decompress_in, lz_decompress_out, input_size, d_flow);

        if (d_flow == 0) {
            std::cout << std::fixed << std::setprecision(3) << "\t\t" << (double)input_size / 1000000 << "\t\t\t"
                      << lz_decompress_in << std::endl;
        } else {
            std::cout << std::fixed << std::setprecision(3);
            std::cout << (double)input_size / 1000000 << "\t\t" << lz_decompress_in << std::endl;
        }
    } while (std::getline(infilelist_dec, line_dec)); // While loop ends
}
void xilBatchVerify(std::string& file_list, int f, uint32_t block_size, std::string& single_bin, uint8_t max_cr) {
    if (f == 0) { // All flows are tested (Xilinx, Standard)

        xilSnappy xlz(single_bin, 2, block_size, max_cr);
        // Xilinx SNAPPY flow

        // Flow : Xilinx SNAPPY Compress vs Xilinx SNAPPY Decompress
        {
            // Xilinx SNAPPY compression
            std::string ext1 = ".xe2xd.snappy";
            std::string ext2 = ".xe2xd.snappy";
            xilCompressDecompressList(xlz, file_list, ext1, ext2, 0, 0, block_size);

            // Validate
            std::cout << "\n";
            std::cout << "----------------------------------------------------------------------------------------"
                      << std::endl;
            std::cout
                << "                       Validate: Xilinx SNAPPY Compress vs Xilinx SNAPPY Decompress           "
                << std::endl;
            std::cout << "----------------------------------------------------------------------------------------"
                      << std::endl;
            std::string ext3 = ".xe2xd.snappy.orig";
            xilValidate(file_list, ext3);
        }

        // Standard SNAPPY flow

        // Flow : Xilinx SNAPPY Compress vs Standard SNAPPY Decompress
        {
            // Xilinx SNAPPY compression
            std::string ext2 = ".xe2sd.snappy";
            std::string ext1 = ".xe2sd.snappy";
            xilCompressDecompressList(xlz, file_list, ext1, ext2, 0, 1, block_size);

            std::cout << "\n";
            std::cout << "---------------------------------------------------------------------------------------"
                      << std::endl;
            std::cout << "                       Validate: Xilinx SNAPPY Compress vs Standard SNAPPY Decompress        "
                      << std::endl;
            std::cout << "---------------------------------------------------------------------------------------"
                      << std::endl;
            std::string ext3 = ".xe2sd";
            xilValidate(file_list, ext3);

        } // End of Flow : Xilinx SNAPPY Compress vs Standard SNAPPY Decompress

        { // Start of Flow : Standard SNAPPY Compress vs Xilinx SNAPPY Decompress

            // Standard SNAPPY compression
            std::string ext1 = ".se2xd";
            std::string ext2 = ".std.snappy";
            xilCompressDecompressList(xlz, file_list, ext1, ext2, 1, 0, block_size);

            // Validate
            std::cout << "\n";
            std::cout << "----------------------------------------------------------------------------------------"
                      << std::endl;
            std::cout << "                       Validate: Standard Compress vs Xilinx SNAPPY Decompress             "
                      << std::endl;
            std::cout << "----------------------------------------------------------------------------------------"
                      << std::endl;
            std::string ext = ".std.snappy.orig";
            xilValidate(file_list, ext);

        } // End of Flow : Standard SNAPPY Compress vs Xilinx SNAPPY Decompress

    }                  // Flow = 0 ends here
    else if (f == 1) { // Only Xilinx flows are tested
        xilSnappy xlz(single_bin, 2, block_size, max_cr);

        // Flow : Xilinx SNAPPY Compress vs Xilinx SNAPPY Decompress
        {
            // Xilinx SNAPPY compression
            std::string ext1 = ".xe2xd.snappy";
            std::string ext2 = ".xe2xd.snappy";
            xilCompressDecompressList(xlz, file_list, ext1, ext2, 0, 0, block_size);

            // Validate
            std::cout << "\n";
            std::cout << "----------------------------------------------------------------------------------------"
                      << std::endl;
            std::cout
                << "                       Validate: Xilinx SNAPPY Compress vs Xilinx SNAPPY Decompress           "
                << std::endl;
            std::cout << "----------------------------------------------------------------------------------------"
                      << std::endl;
            std::string ext3 = ".xe2xd.snappy.orig";
            xilValidate(file_list, ext3);
        }

    } // Flow = 1 ends here
    else if (f == 2) {
        xilSnappy xlz(single_bin, 1, block_size, max_cr);
        // Flow : Xilinx SNAPPY Compress vs Standard SNAPPY Decompress
        {
            // Xilinx SNAPPY compression
            std::string ext1 = ".xe2sd.snappy";
            std::string ext2 = ".xe2sd.snappy";
            xilCompressDecompressList(xlz, file_list, ext1, ext2, 0, 1, block_size);

            // Validate
            std::cout << "\n";
            std::cout << "---------------------------------------------------------------------------------------"
                      << std::endl;
            std::cout << "                       Validate: Xilinx SNAPPY Compress vs Standard SNAPPY Decompress        "
                      << std::endl;
            std::cout << "---------------------------------------------------------------------------------------"
                      << std::endl;
            std::string ext3 = ".xe2sd";
            xilValidate(file_list, ext3);

        } // End of Flow : Xilinx SNAPPY Compress vs Standard SNAPPY Decompress

    } // Flow = 2 ends here
    else if (f == 3) {
        xilSnappy xlz(single_bin, 0, block_size, max_cr);
        { // Start of Flow : Standard SNAPPY Compress vs Xilinx SNAPPY Decompress

            // Standard SNAPPY compression
            std::string ext1 = ".se2xd";
            std::string ext2 = ".std.snappy";
            xilCompressDecompressList(xlz, file_list, ext1, ext2, 1, 0, block_size);

            // Validate
            std::cout << "\n";
            std::cout << "----------------------------------------------------------------------------------------"
                      << std::endl;
            std::cout << "                       Validate: Standard Compress vs Xilinx SNAPPY Decompress             "
                      << std::endl;
            std::cout << "----------------------------------------------------------------------------------------"
                      << std::endl;
            std::string ext = ".std.snappy.orig";
            xilValidate(file_list, ext);

        } // End of Flow : Standard SNAPPY Compress vs Xilinx SNAPPY Decompress
    }     // Flow = 3 ends here
    else {
        std::cout << "-x option is wrong" << f << std::endl;
        std::cout << "-x - 0 all features" << std::endl;
        std::cout << "-x - 1 Xilinx (C/D)" << std::endl;
        std::cout << "-x - 2 Xilinx Compress vs Standard Decompress" << std::endl;
        std::cout << "-x - 3 Standard Compress vs Xilinx Decompress" << std::endl;
    }
}

void xilDecompressTop(std::string& decompress_mod, uint32_t block_size, std::string& single_bin, uint8_t max_cr_val) {
    // Create xilSnappy object
    xilSnappy xlz(single_bin, 0, block_size, max_cr_val);

#ifdef VERBOSE
    std::cout << std::fixed << std::setprecision(2) << "KT(MBps)\t\t:";
#endif

    std::ifstream inFile(decompress_mod.c_str(), std::ifstream::binary);
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

    string lz_decompress_in = decompress_mod;
    string lz_decompress_out = decompress_mod;
    lz_decompress_out = lz_decompress_out + ".orig";

    // Call SNAPPY decompression
    xlz.decompressFile(lz_decompress_in, lz_decompress_out, input_size, 0);
#ifdef VERBOSE
    std::cout << std::fixed << std::setprecision(3) << std::endl
              << "File Size(" << sizes[order] << ")\t\t:" << len << std::endl
              << "File Name\t\t:" << lz_decompress_in << std::endl;
    std::cout << "\n";
    std::cout << "Output Location: " << lz_decompress_out.c_str() << std::endl;
#endif
}

void xilCompressDecompressTop(std::string& compress_decompress_mod,
                              uint32_t block_size,
                              std::string& single_bin,
                              uint8_t max_cr_val) {
    // Compression
    // Create xilSnappy object
    xilSnappy xlz(single_bin, 2, block_size, max_cr_val);

    std::cout << "\n";

    std::cout << "--------------------------------------------------------------" << std::endl;
    std::cout << "                     Xilinx Compress                          " << std::endl;
    std::cout << "--------------------------------------------------------------" << std::endl;

    std::cout << "\n";
    std::cout << std::fixed << std::setprecision(2) << "KT(MBps)\t\t:";

    std::ifstream inFile(compress_decompress_mod.c_str(), std::ifstream::binary);
    if (!inFile) {
        std::cout << "Unable to open file" << std::endl;
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
    std::string lz_compress_in = compress_decompress_mod;
    std::string lz_compress_out = compress_decompress_mod;
    lz_compress_out = lz_compress_out + ".snappy";

    // Call snappy compression
    uint64_t enbytes = xlz.compressFile(lz_compress_in, lz_compress_out, input_size, 0);
    std::cout << std::fixed << std::setprecision(2) << std::endl
              << "SNAPPY_CR\t\t:" << (double)input_size / enbytes << std::endl
              << std::fixed << std::setprecision(3) << "File Size(" << sizes[order] << ")\t\t:" << len << std::endl
              << "File Name\t\t:" << lz_compress_in << std::endl;

    // De-Compression

    std::cout << "\n";
    std::cout << "--------------------------------------------------------------" << std::endl;
    std::cout << "                     Xilinx De-Compress                       " << std::endl;
    std::cout << "--------------------------------------------------------------" << std::endl;
    std::cout << "\n";

    std::cout << std::fixed << std::setprecision(2) << "KT(MBps)\t\t:";

    // Decompress list of files
    // This loop does snappy decompress on list
    // of files.

    std::string lz_decompress_in = compress_decompress_mod + ".snappy";
    std::string lz_decompress_out = compress_decompress_mod;
    lz_decompress_out = lz_decompress_in + ".orig";

    std::ifstream inFile_dec(lz_decompress_in.c_str(), std::ifstream::binary);
    if (!inFile_dec) {
        std::cout << "Unable to open file";
        exit(1);
    }

    uint64_t input_size1 = getFileSize(inFile_dec);
    inFile_dec.close();

    // Call snappy decompression
    xlz.decompressFile(lz_decompress_in, lz_decompress_out, input_size1, 0);

    std::cout << std::fixed << std::setprecision(3) << std::endl
              << "File Size(" << sizes[order] << ")\t\t:" << len << std::endl
              << "File Name\t\t:" << lz_decompress_in << std::endl;

    // Validate
    std::cout << "\n";
    std::string inputFile = compress_decompress_mod;
    std::string outputFile = compress_decompress_mod + ".snappy" + ".orig";
    int ret = validate(inputFile, outputFile);
    if (ret == 0) {
        std::cout << (ret ? "FAILED\t" : "PASSED\t") << "\t" << inputFile << std::endl;
    } else {
        std::cout << "Validation Failed" << outputFile.c_str() << std::endl;
    }
}

int main(int argc, char* argv[]) {
    sda::utils::CmdLineParser parser;
    parser.addSwitch("--single_xclbin", "-sx", "Single XCLBIN", "compress_decompress");
    parser.addSwitch("--compress_decompress", "-v", "Compress Decompress", "");
    parser.addSwitch("--compress", "-c", "Compress", "");
    parser.addSwitch("--file_list", "-l", "List of Input Files", "");
    parser.addSwitch("--decompress", "-d", "Decompress", "");
    parser.addSwitch("--block_size", "-B", "Compress Block Size [0-64: 1-256: 2-1024: 3-4096]", "0");
    parser.addSwitch("--flow", "-x", "Validation [0-All: 1-XcXd: 2-XcSd: 3-ScXd]", "1");
    parser.addSwitch("--max_cr", "-mcr", "Maximum CR", "10");
    parser.parse(argc, argv);

    std::string single_bin = parser.value("single_xclbin");
    std::string compress_decompress_mod = parser.value("compress_decompress");
    std::string compress_mod = parser.value("compress");
    std::string filelist = parser.value("file_list");
    std::string decompress_mod = parser.value("decompress");
    std::string flow = parser.value("flow");
    std::string block_size = parser.value("block_size");
    std::string mcr = parser.value("max_cr");

    uint8_t max_cr_val = 0;
    if (!(mcr.empty())) {
        max_cr_val = atoi(mcr.c_str());
    } else {
        // Default block size
        max_cr_val = MAX_CR;
    }

    uint32_t bSize = 0;
    // Block Size
    if (!(block_size.empty())) {
        bSize = atoi(block_size.c_str());

        switch (bSize) {
            case 0:
                bSize = 64;
                break;
            case 1:
                bSize = 256;
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
        // Default Block Size - 64KB
        bSize = BLOCK_SIZE_IN_KB;
    }

    int fopt = 0;
    if (!(flow.empty()))
        fopt = atoi(flow.c_str());
    else
        fopt = 1;

    // "-c" - Compress Mode
    if (!compress_mod.empty()) xilCompressTop(compress_mod, bSize, single_bin);

    // "-d" Decompress Mode
    if (!decompress_mod.empty()) xilDecompressTop(decompress_mod, bSize, single_bin, max_cr_val);

    if (!compress_decompress_mod.empty())
        xilCompressDecompressTop(compress_decompress_mod, bSize, single_bin, max_cr_val);

    // "-l" List of Files
    if (!filelist.empty()) {
        if (fopt == 0 || fopt == 2 || fopt == 3) {
            std::cout << "\n" << std::endl;
            std::cout << "Validation flows with Standard SNAPPY ";
            std::cout << "requires executable" << std::endl;
            std::cout << "Please build SNAPPY executable ";
            std::cout << "from following source ";
            std::cout << "https://github.com/snappy/snappy.git" << std::endl;
        }
        xilBatchVerify(filelist, fopt, bSize, single_bin, max_cr_val);
    }
}
