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
#include "lz4.hpp"
#include <fstream>
#include <vector>
#include "cmdlineparser.h"

using namespace xf::compression;

int validate(std::string& inFile_name, std::string& outFile_name) {
    std::string command = "cmp " + inFile_name + " " + outFile_name;
    int ret = system(command.c_str());
    return ret;
}

static uint64_t getFileSize(std::ifstream& file) {
    file.seekg(0, file.end);
    uint64_t file_size = file.tellg();
    file.seekg(0, file.beg);
    return file_size;
}

void xilCompressTop(std::string& compress_mod, uint32_t block_size, std::string& compress_bin) {
    // Xilinx LZ4 object
    xfLz4 xlz;

    // LZ4 Compression Binary Name
    std::string binaryFileName;
    binaryFileName = compress_bin;

    // Create xfLz4 object
    xlz.init(binaryFileName, 1, block_size);

    std::ifstream inFile(compress_mod.c_str(), std::ifstream::binary);
    if (!inFile) {
        std::cout << "Unable to open file";
        exit(1);
    }
    uint64_t input_size = getFileSize(inFile);
    inFile.close();

    double len = input_size;
    int order = 0;
    while (len >= 1000) {
        order++;
        len = len / 1000;
    }

    std::string lz_compress_in = compress_mod;
    std::string lz_compress_out = compress_mod;
    lz_compress_out = lz_compress_out + ".lz4";

    bool file_list_flag = false;
    uint64_t enbytes = xlz.compressFile(lz_compress_in, lz_compress_out, input_size, file_list_flag, 0);

#ifdef EVENT_PROFILE
    auto total_start = std::chrono::high_resolution_clock::now();
#endif
// Call LZ4 compression
#ifdef EVENT_PROFILE
    auto total_end = std::chrono::high_resolution_clock::now();
    auto total_time_ns = std::chrono::duration<double, std::nano>(total_end - total_start);
#endif

    const char* sizes[] = {"B", "kB", "MB", "GB", "TB"};
    std::cout.precision(3);
    std::cout << std::setprecision(2) << "LZ4_CR\t\t\t:" << (double)input_size / enbytes << std::endl
              << std::fixed << std::setprecision(3) << "File Size(" << sizes[order] << ")\t\t:" << len << std::endl
              << "File Name\t\t:" << lz_compress_in << std::endl;
    std::cout << "\n";
    std::cout << "Output Location: " << lz_compress_out.c_str() << std::endl;
    std::cout << "Compressed file size: " << enbytes << std::endl;

#ifdef EVENT_PROFILE
    std::cout << "Total Time (milli sec): " << total_time_ns.count() / 1000000 << std::endl;
#endif

    xlz.release();
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

void xilCompressDecompressList(std::string& file_list,
                               std::string& ext1,
                               std::string& ext2,
                               bool c_flow,
                               bool d_flow,
                               uint32_t block_size,
                               std::string& compress_bin,
                               std::string& decompress_bin) {
    // Compression
    // LZ4 Compression Binary Name
    std::string binaryFileName;
    binaryFileName = compress_bin;

    // Create xfLz4 object
    xfLz4 xlz;

    if (c_flow == 0) {
        std::cout << "\n";
        xlz.init(binaryFileName, 1, block_size);
    }
    std::cout << "\n";

    bool file_list_flag = true;

    std::cout << "--------------------------------------------------------------" << std::endl;
    if (c_flow == 0)
        std::cout << "                     Xilinx Compress                          " << std::endl;
    else
        std::cout << "                     Standard Compress                        " << std::endl;
    std::cout << "--------------------------------------------------------------" << std::endl;

    if (c_flow == 0) {
        std::cout << "\n";
        std::cout << "E2E(MBps)\tLZ4_CR\t\tFile Size(MB)\t\tFile Name" << std::endl;
        std::cout << "\n";
    } else {
        std::cout << "\n";
        std::cout << "File Size(MB)\t\tFile Name" << std::endl;
        std::cout << "\n";
    }

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

        uint64_t input_size = getFileSize(inFile);
        inFile.close();

        std::string lz_compress_in = line;
        std::string lz_compress_out = line;
        lz_compress_out = lz_compress_out + ext1;

        // Call LZ4 compression
        uint64_t enbytes = xlz.compressFile(lz_compress_in, lz_compress_out, input_size, file_list_flag, c_flow);
        if (c_flow == 0) {
            std::cout << "\t\t" << (double)input_size / enbytes << "\t\t" << std::fixed << std::setprecision(3)
                      << (double)input_size / 1000000 << "\t\t\t" << lz_compress_in << std::endl;
        } else {
            std::cout << std::fixed << std::setprecision(3);
            std::cout << (double)input_size / 1000000 << "\t\t\t" << lz_compress_in << std::endl;
        }
    }

    if (c_flow == 0) {
        xlz.release();
    }

    // De-Compression
    // LZ4 Decompression Binary Name
    std::string binaryFileName_decompress;
    binaryFileName_decompress = decompress_bin;

    // Xilinx LZ4 object
    if (d_flow == 0) {
        // Create xfLz4 object
        std::cout << "\n";
        xlz.init(binaryFileName_decompress, 0, block_size);
        if (c_flow == 1) {
            xlz.init(binaryFileName, 1, block_size);
        }
    }

    std::ifstream infilelist_dec(file_list.c_str());
    std::string line_dec;

    std::cout << "\n";
    std::cout << "--------------------------------------------------------------" << std::endl;
    if (d_flow == 0)
        std::cout << "                     Xilinx De-Compress                       " << std::endl;
    else
        std::cout << "                     Standard De-Compress                     " << std::endl;

    std::cout << "--------------------------------------------------------------" << std::endl;
    if (d_flow == 0) {
        std::cout << "\n";
        std::cout << "E2E(MBps)\tFile Size(MB)\t\tFile Name" << std::endl;
        std::cout << "\n";
    } else {
        std::cout << "\n";
        std::cout << "File Size(MB)\tFile Name" << std::endl;
        std::cout << "\n";
    }

    // Decompress list of files
    // This loop does LZ4 decompress on list
    // of files.
    while (std::getline(infilelist_dec, line_dec)) {
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

        // Call LZ4 decompression
        xlz.decompressFile(lz_decompress_in, lz_decompress_out, input_size, file_list_flag, d_flow);

        if (d_flow == 0) {
            std::cout << std::fixed << std::setprecision(3) << "\t\t" << (double)input_size / 1000000 << "\t\t"
                      << lz_decompress_in << std::endl;
        } else {
            std::cout << std::fixed << std::setprecision(3);
            std::cout << (double)input_size / 1000000 << "\t\t" << lz_decompress_in << std::endl;
        }
    } // While loop ends

    if (d_flow == 0) {
        xlz.release();
    }
}
void xilBatchVerify(
    std::string& file_list, int f, uint32_t block_size, std::string& compress_bin, std::string& decompress_bin) {
    if (f == 0) { // All flows are tested (Xilinx, Standard)

        // Xilinx LZ4 flow

        // Flow : Xilinx LZ4 Compress vs Xilinx LZ4 Decompress
        {
            // Xilinx LZ4 compression
            std::string ext1 = ".xe2xd.lz4";
            std::string ext2 = ".xe2xd.lz4";
            xilCompressDecompressList(file_list, ext1, ext2, 0, 0, block_size, compress_bin, decompress_bin);

            // Validate
            std::cout << "\n";
            std::cout << "----------------------------------------------------------------------------------------"
                      << std::endl;
            std::cout << "                       Validate: Xilinx LZ4 Compress vs Xilinx LZ4 Decompress           "
                      << std::endl;
            std::cout << "----------------------------------------------------------------------------------------"
                      << std::endl;
            std::string ext3 = ".xe2xd.lz4.orig";
            xilValidate(file_list, ext3);
        }

        // Standard LZ4 flow

        // Flow : Xilinx LZ4 Compress vs Standard LZ4 Decompress
        {
            // Xilinx LZ4 compression
            std::string ext2 = ".xe2sd.lz4";
            std::string ext1 = ".xe2sd.lz4";
            xilCompressDecompressList(file_list, ext1, ext2, 0, 1, block_size, compress_bin, decompress_bin);

            std::cout << "\n";
            std::cout << "---------------------------------------------------------------------------------------"
                      << std::endl;
            std::cout << "                       Validate: Xilinx LZ4 Compress vs Standard LZ4 Decompress        "
                      << std::endl;
            std::cout << "---------------------------------------------------------------------------------------"
                      << std::endl;
            std::string ext3 = ".xe2sd";
            xilValidate(file_list, ext3);

        } // End of Flow : Xilinx LZ4 Compress vs Standard LZ4 Decompress

        { // Start of Flow : Standard LZ4 Compress vs Xilinx LZ4 Decompress

            // Standard LZ4 compression
            std::string ext1 = ".se2xd";
            std::string ext2 = ".std.lz4";
            xilCompressDecompressList(file_list, ext1, ext2, 1, 0, block_size, compress_bin, decompress_bin);

            // Validate
            std::cout << "\n";
            std::cout << "----------------------------------------------------------------------------------------"
                      << std::endl;
            std::cout << "                       Validate: Standard Compress vs Xilinx LZ4 Decompress             "
                      << std::endl;
            std::cout << "----------------------------------------------------------------------------------------"
                      << std::endl;
            std::string ext = ".std.lz4.orig";
            xilValidate(file_list, ext);

        } // End of Flow : Standard LZ4 Compress vs Xilinx LZ4 Decompress

    }                  // Flow = 0 ends here
    else if (f == 1) { // Only Xilinx flows are tested

        // Flow : Xilinx LZ4 Compress vs Xilinx LZ4 Decompress
        {
            // Xilinx LZ4 compression
            std::string ext1 = ".xe2xd.lz4";
            std::string ext2 = ".xe2xd.lz4";
            xilCompressDecompressList(file_list, ext1, ext2, 0, 0, block_size, compress_bin, decompress_bin);

            // Validate
            std::cout << "\n";
            std::cout << "----------------------------------------------------------------------------------------"
                      << std::endl;
            std::cout << "                       Validate: Xilinx LZ4 Compress vs Xilinx LZ4 Decompress           "
                      << std::endl;
            std::cout << "----------------------------------------------------------------------------------------"
                      << std::endl;
            std::string ext3 = ".xe2xd.lz4.orig";
            xilValidate(file_list, ext3);
        }

    } // Flow = 1 ends here
    else if (f == 2) {
        // Flow : Xilinx LZ4 Compress vs Standard LZ4 Decompress
        {
            // Xilinx LZ4 compression
            std::string ext1 = ".xe2sd.lz4";
            std::string ext2 = ".xe2sd.lz4";
            xilCompressDecompressList(file_list, ext1, ext2, 0, 1, block_size, compress_bin, decompress_bin);

            // Validate
            std::cout << "\n";
            std::cout << "---------------------------------------------------------------------------------------"
                      << std::endl;
            std::cout << "                       Validate: Xilinx LZ4 Compress vs Standard LZ4 Decompress        "
                      << std::endl;
            std::cout << "---------------------------------------------------------------------------------------"
                      << std::endl;
            std::string ext3 = ".xe2sd";
            xilValidate(file_list, ext3);

        } // End of Flow : Xilinx LZ4 Compress vs Standard LZ4 Decompress

    } // Flow = 2 ends here
    else if (f == 3) {
        { // Start of Flow : Standard LZ4 Compress vs Xilinx LZ4 Decompress

            // Standard LZ4 compression
            std::string ext1 = ".se2xd";
            std::string ext2 = ".std.lz4";
            xilCompressDecompressList(file_list, ext1, ext2, 1, 0, block_size, compress_bin, decompress_bin);

            // Validate
            std::cout << "\n";
            std::cout << "----------------------------------------------------------------------------------------"
                      << std::endl;
            std::cout << "                       Validate: Standard Compress vs Xilinx LZ4 Decompress             "
                      << std::endl;
            std::cout << "----------------------------------------------------------------------------------------"
                      << std::endl;
            std::string ext = ".std.lz4.orig";
            xilValidate(file_list, ext);

        } // End of Flow : Standard LZ4 Compress vs Xilinx LZ4 Decompress
    }     // Flow = 3 ends here
    else {
        std::cout << "-x option is wrong" << f << std::endl;
        std::cout << "-x - 0 all features" << std::endl;
        std::cout << "-x - 1 Xilinx (C/D)" << std::endl;
        std::cout << "-x - 2 Xilinx Compress vs Standard Decompress" << std::endl;
        std::cout << "-x - 3 Standard Compress vs Xilinx Decompress" << std::endl;
    }
}

void xilDecompressTop(std::string& decompress_mod, uint32_t block_size, std::string& decompress_bin) {
    // Create xfLz4 object
    xfLz4 xlz;

    // LZ4 Decompression Binary Name
    std::string binaryFileName;
    binaryFileName = decompress_bin;
    xlz.init(binaryFileName, 0, block_size);

    std::ifstream inFile(decompress_mod.c_str(), std::ifstream::binary);
    if (!inFile) {
        std::cout << "Unable to open file";
        exit(1);
    }

    uint64_t input_size = getFileSize(inFile);
    inFile.close();

    double len = input_size;
    int order = 0;
    while (len >= 1000) {
        order++;
        len = len / 1000;
    }

    string lz_decompress_in = decompress_mod;
    string lz_decompress_out = decompress_mod;
    lz_decompress_out = lz_decompress_out + ".orig";

    bool file_list_flag = false;

    // Call LZ4 decompression
    xlz.decompressFile(lz_decompress_in, lz_decompress_out, input_size, file_list_flag, 0);

#ifdef VERBOSE
    const char* sizes[] = {"B", "kB", "MB", "GB", "TB"};
    std::cout << std::fixed << std::setprecision(3) << "File Size(" << sizes[order] << ")\t\t:" << len << std::endl
              << "File Name\t\t:" << lz_decompress_in << std::endl;
    std::cout << "\n";
    std::cout << "Output Location: " << lz_decompress_out.c_str() << std::endl;
#endif

    xlz.release();
}

void xilCompressDecompressTop(std::string& compress_decompress_mod,
                              uint32_t block_size,
                              std::string& compress_bin,
                              std::string& decompress_bin) {
    // Compression
    // LZ4 Compression Binary Name
    std::string binaryFileName = compress_bin;

    // Create xfLz4 object
    xfLz4 xlz;

    xlz.init(binaryFileName, 1, block_size);

    std::cout << "\n";

    bool file_list_flag = false;

    std::cout << "--------------------------------------------------------------" << std::endl;
    std::cout << "                     Xilinx Compress                          " << std::endl;
    std::cout << "--------------------------------------------------------------" << std::endl;

    std::cout << "\n";

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
    lz_compress_out = lz_compress_out + ".lz4";

    // Call LZ4 compression
    uint64_t enbytes = xlz.compressFile(lz_compress_in, lz_compress_out, input_size, file_list_flag, 0);
    std::cout << std::setprecision(2) << "LZ4_CR\t\t\t:" << (double)input_size / enbytes << std::endl
              << std::fixed << std::setprecision(3) << "File Size(" << sizes[order] << ")\t\t:" << len << std::endl
              << "File Name\t\t:" << lz_compress_in << std::endl;

    xlz.release();

    // De-Compression
    binaryFileName = decompress_bin;

    // Create xfLz4 object
    xfLz4 d_xlz;

    d_xlz.init(binaryFileName, 0, block_size);

    std::cout << "\n";
    std::cout << "--------------------------------------------------------------" << std::endl;
    std::cout << "                     Xilinx De-Compress                       " << std::endl;
    std::cout << "--------------------------------------------------------------" << std::endl;
    std::cout << "\n";

    // Decompress list of files
    // This loop does LZ4 decompress on list
    // of files.

    std::string lz_decompress_in = compress_decompress_mod + ".lz4";
    std::string lz_decompress_out = compress_decompress_mod;
    lz_decompress_out = lz_decompress_in + ".orig";

    std::ifstream inFile_dec(lz_decompress_in.c_str(), std::ifstream::binary);
    if (!inFile_dec) {
        std::cout << "Unable to open file";
        exit(1);
    }

    uint64_t input_size1 = getFileSize(inFile_dec);
    inFile_dec.close();

    // Call LZ4 decompression
    d_xlz.decompressFile(lz_decompress_in, lz_decompress_out, input_size1, file_list_flag, 0);

    std::cout << std::fixed << std::setprecision(3) << "File Size(" << sizes[order] << ")\t\t:" << len << std::endl
              << "File Name\t\t:" << lz_compress_in << std::endl;

    d_xlz.release();

    // Validate
    std::cout << "\n";
    std::string inputFile = compress_decompress_mod;
    std::string outputFile = compress_decompress_mod + ".lz4" + ".orig";
    int ret = validate(inputFile, outputFile);
    if (ret == 0) {
        std::cout << (ret ? "FAILED\t" : "PASSED\t") << "\t" << inputFile << std::endl;
    } else {
        std::cout << "Validation Failed" << outputFile.c_str() << std::endl;
    }
}

int main(int argc, char* argv[]) {
    sda::utils::CmdLineParser parser;
    parser.addSwitch("--compress_xclbin", "-cx", "Compress XCLBIN", "compress");
    parser.addSwitch("--decompress_xclbin", "-dx", "DeCompress XCLBIN", "decompress");
    parser.addSwitch("--compress", "-c", "Compress", "");
    parser.addSwitch("--file_list", "-l", "List of Input Files", "");
    parser.addSwitch("--decompress", "-d", "Decompress", "");
    parser.addSwitch("--compress_decompress", "-v", "Compress Decompress", "");
    parser.addSwitch("--block_size", "-B", "Compress Block Size [0-64: 1-256: 2-1024: 3-4096]", "0");
    parser.addSwitch("--flow", "-x", "Validation [0-All: 1-XcXd: 2-XcSd: 3-ScXd]", "1");
    parser.parse(argc, argv);

    std::string compress_bin = parser.value("compress_xclbin");
    std::string decompress_bin = parser.value("decompress_xclbin");
    std::string compress_mod = parser.value("compress");
    std::string filelist = parser.value("file_list");
    std::string decompress_mod = parser.value("decompress");
    std::string compress_decompress_mod = parser.value("compress_decompress");
    std::string flow = parser.value("flow");
    std::string block_size = parser.value("block_size");

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
    if (!compress_mod.empty()) xilCompressTop(compress_mod, bSize, compress_bin);

    // "-d" Decompress Mode
    if (!decompress_mod.empty()) xilDecompressTop(decompress_mod, bSize, decompress_bin);

    // "-v" Compress Decompress Mode
    if (!compress_decompress_mod.empty())
        xilCompressDecompressTop(compress_decompress_mod, bSize, compress_bin, decompress_bin);

    // "-l" List of Files
    if (!filelist.empty()) {
        if (fopt == 0 || fopt == 2 || fopt == 3) {
            std::cout << "\n" << std::endl;
            std::cout << "Validation flows with Standard LZ4 ";
            std::cout << "requires executable" << std::endl;
            std::cout << "Please build LZ4 executable ";
            std::cout << "from following source ";
            std::cout << "https://github.com/lz4/lz4.git" << std::endl;
        }
        xilBatchVerify(filelist, fopt, bSize, compress_bin, decompress_bin);
    }
}
