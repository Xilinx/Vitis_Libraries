/*
 * Copyright 2019 Xilinx, Inc.
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
 */
#include "lz4_p2p.hpp"
#include "lz4_p2p_dec.hpp"
#include <fstream>
#include <iostream>
#include <cassert>
#include "cmdlineparser.h"

int validate(std::string& inFile_name, std::string& outFile_name) {
    std::string command = "cmp " + inFile_name + " " + outFile_name;
    int ret = system(command.c_str());
    return ret;
}

void decompress_multiple_files(const std::vector<std::string>& inFileVec,
                               const std::vector<std::string>& outFileVec,
                               const std::string& decompress_bin) {
    std::vector<char*> outVec;
    std::vector<uint64_t> orgSizeVec;
    std::vector<uint64_t> inSizeVec;
    std::vector<int> fd_p2p_vec;
    std::vector<cl_event> userEventVec;
    uint64_t total_size = 0;
    uint64_t total_in_size = 0;

    for (uint32_t fid = 0; fid < inFileVec.size(); fid++) {
        uint64_t original_size = 0;
        std::string inFile_name = inFileVec[fid];
        std::ifstream inFile(inFile_name.c_str(), std::ifstream::binary);
        uint64_t input_size = xfLz4::get_file_size(inFile);
        inFile.close();

        int fd_p2p_c_in = open(inFile_name.c_str(), O_RDONLY | O_DIRECT);
        if (fd_p2p_c_in <= 0) {
            std::cout << "P2P: Unable to open input file, fd: " << fd_p2p_c_in << std::endl;
            exit(1);
        }
        std::vector<uint8_t, aligned_allocator<uint8_t> > in_4kbytes(4 * KB);
        read(fd_p2p_c_in, (char*)in_4kbytes.data(), 4 * KB);
        lseek(fd_p2p_c_in, 0, SEEK_SET);
        fd_p2p_vec.push_back(fd_p2p_c_in);
        std::memcpy(&original_size, &in_4kbytes[6], 4);
        total_size += original_size;
        total_in_size += input_size;
        orgSizeVec.push_back(original_size);
        char* out = (char*)aligned_alloc(4096, original_size);
        outVec.push_back(out);
        inSizeVec.push_back(input_size);
    }
    xfLz4 xlz(decompress_bin);
    xlz.decompress_in_line_multiple_files(inFileVec, fd_p2p_vec, outVec, orgSizeVec, inSizeVec);
    for (uint32_t fid = 0; fid < inFileVec.size(); fid++) {
        std::string outFile_name = outFileVec[fid];
        std::ofstream outFile(outFile_name.c_str(), std::ofstream::binary);
        outFile.write((char*)outVec[fid], orgSizeVec[fid]);
        close(fd_p2p_vec[fid]);
        outFile.close();
    }
}

void xil_decompress_file_list(std::string& file_list, std::string& decompress_bin) {
    std::ifstream infilelist_dec(file_list.c_str());
    std::string line_dec;
    std::string ext1 = ".lz4";
    std::vector<std::string> inFileList;
    std::vector<std::string> outFileList;
    std::vector<std::string> orgFileList;
    while (std::getline(infilelist_dec, line_dec)) {
        std::string in_file = line_dec + ext1;
        std::string out_file = line_dec + ext1 + ".org";
        inFileList.push_back(in_file);
        orgFileList.push_back(line_dec);
        outFileList.push_back(out_file);
    }
    decompress_multiple_files(inFileList, outFileList, decompress_bin);
    std::cout << std::endl;
    for (size_t i = 0; i < inFileList.size(); i++) {
        int ret = validate(orgFileList[i], outFileList[i]);
        if (ret) {
            std::cout << "FAILED: " << inFileList[i] << std::endl;
        } else {
            std::cout << "PASSED: " << inFileList[i] << std::endl;
        }
    }
}

int main(int argc, char* argv[]) {
    sda::utils::CmdLineParser parser;
    parser.addSwitch("--decompress_xclbin", "-dx", "Decompress XCLBIN", "decompress");
    parser.addSwitch("--decompress_mode", "-d", "Decompress Mode", "");
    parser.addSwitch("--single_xclbin", "-sx", "Single XCLBIN", "p2p_decompress");
    parser.addSwitch("--file_list", "-l", "List of Input Files", "");
    parser.parse(argc, argv);

    std::string decompress_xclbin = parser.value("decompress_xclbin");
    std::string decompress_mod = parser.value("decompress_mode");
    std::string single_bin = parser.value("single_xclbin");
    std::string filelist = parser.value("file_list");

    int fopt = 1;

    // "-l" List of Files
    if (!filelist.empty()) {
        xil_decompress_file_list(filelist, decompress_xclbin);
    }
}
