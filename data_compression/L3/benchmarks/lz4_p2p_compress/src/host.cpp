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
#include "lz4_p2p_comp.hpp"
#include <fstream>
#include <vector>
#include "cmdlineparser.h"

void compress_multiple_files(const std::vector<std::string>& inFileVec,
                             const std::vector<std::string>& outFileVec,
                             uint32_t block_size,
                             const std::string& compress_bin,
                             bool enable_p2p) {
    std::vector<char*> inVec;
    std::vector<int> fd_p2p_vec;
    std::vector<char*> outVec;
    std::vector<uint32_t> inSizeVec;

    uint64_t total_size = 0;
    uint64_t total_in_size = 0;

    printf("NumFiles:%d\n", inFileVec.size());
    for (uint32_t fid = 0; fid < inFileVec.size(); fid++) {
        uint64_t original_size = 0;
        std::string inFile_name = inFileVec[fid];
        std::ifstream inFile(inFile_name.c_str(), std::ifstream::binary);
        if (!inFile) {
            std::cout << "Unable to open file";
            exit(1);
        }
        uint32_t input_size = xflz4::get_file_size(inFile);

        std::string outFile_name = outFileVec[fid];
        uint32_t out_size;

        char* in = (char*)aligned_alloc(4096, input_size);
        inFile.read(in, input_size);
        inVec.push_back(in);
        inSizeVec.push_back(input_size);
    }
    xflz4 xlz(compress_bin, 0);
    xlz.m_block_size_in_kb = block_size;
    xlz.compress_in_line_multiple_files(inVec, outFileVec, inSizeVec, enable_p2p);
}

int validateFile(std::string& inFile_name, std::string& origFile_name) {
    std::string command = "cmp " + inFile_name + " " + origFile_name;
    int ret = system(command.c_str());
    return ret;
}

void xil_compress_file_list(std::string& file_list, uint32_t block_size, std::string& compress_bin, bool enable_p2p) {
    std::ifstream infilelist_comp(file_list.c_str());
    std::string line_comp;
    std::string ext1 = ".xe2sd";

    std::vector<std::string> inFileList;
    std::vector<std::string> outFileList;
    std::vector<std::string> origFileList;

    while (std::getline(infilelist_comp, line_comp)) {
        std::string orig_file = line_comp + ext1;
        std::string out_file = line_comp + ext1 + ".lz4";
        inFileList.push_back(line_comp);
        origFileList.push_back(orig_file);
        outFileList.push_back(out_file);
    }
    compress_multiple_files(inFileList, outFileList, block_size, compress_bin, enable_p2p);
    std::cout << std::endl;
}

int main(int argc, char* argv[]) {
    sda::utils::CmdLineParser parser;
    parser.addSwitch("--compress_xclbin", "-cx", "Compress XCLBIN", "compress");
    parser.addSwitch("--p2p_flow", "-p2p", "P2P Flow", "0");
    parser.addSwitch("--file_list", "-l", "List of Input Files", "");
    parser.addSwitch("--block_size", "-B", "Compress Block Size [0-64: 1-256: 2-1024: 3-4096]", "0");
    parser.addSwitch("--id", "-id", "Device ID", "0");
    parser.parse(argc, argv);

    std::string compress_bin = parser.value("compress_xclbin");
    std::string p2p = parser.value("p2p_flow");
    std::string filelist = parser.value("file_list");
    std::string block_size = parser.value("block_size");
    std::string device_ids = parser.value("id");
    uint8_t device_id = 0;

    if (!(device_ids.empty())) {
        device_id = atoi(device_ids.c_str());
    }
    uint32_t bSize = 0;

    bool enable_p2p = 0;
    if (!(p2p.empty())) enable_p2p = atoi(p2p.c_str());

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

    // "-l" List of Files
    if (!filelist.empty()) {
        std::cout << "\n" << std::endl;
        xil_compress_file_list(filelist, bSize, compress_bin, enable_p2p);
    }
}
