/*
 * Copyright 2022 Xilinx, Inc.
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

#include "xf_security/crc32c_sc.hpp"

#include <algorithm>
#include <iterator>

#include <sys/time.h>
#include <new>
#include <cstdlib>
#include <ap_int.h>
#include <iostream>

#include <vector>
#include <string>
#include <fstream>

class ArgParser {
   public:
    ArgParser(int& argc, const char** argv) {
        for (int i = 1; i < argc; ++i) mTokens.push_back(std::string(argv[i]));
    }
    bool getCmdOption(const std::string option, std::string& value) const {
        std::vector<std::string>::const_iterator itr;
        itr = std::find(this->mTokens.begin(), this->mTokens.end(), option);
        if (itr != this->mTokens.end() && ++itr != this->mTokens.end()) {
            value = *itr;
            return true;
        }
        return false;
    }

   private:
    std::vector<std::string> mTokens;
};

int main(int argc, char* argv[]) {
    ArgParser parser(argc, (const char**)argv);

    std::string testfile_path;
    if (!parser.getCmdOption("-testfile", testfile_path)) {
        std::cout << "ERROR: testfile path is not set!\n";
        return 1;
    }

    std::string cnt_str;
    if (!parser.getCmdOption("-n", cnt_str)) {
        std::cout << "ERROR: number of test is not set!\n";
        return 1;
    }
    uint32_t num = std::stoi(cnt_str); // total number of tasks

    // push back file path & size
    std::vector<std::string> in_file;
    std::vector<uint32_t> in_size;
    std::vector<uint32_t> out_result;
    for (int i = 0; i < num; i++) {
        std::string file_name = testfile_path + std::to_string(i);
        std::ifstream ifs(file_name, std::ios::binary);
        if (!ifs) {
            std::cout << "ERROR: open testfile failed.\n";
            return 1;
        }
        uint32_t size;
        ifs.seekg(0, std::ios::end);
        size = ifs.tellg();
        ifs.seekg(0, std::ios::beg);

        in_file.push_back(file_name);
        in_size.push_back(size);
    }

    // run CRC32C calculation
    int run_status = crc32c_run(in_file, in_size, out_result);

    // check results
    std::cout << "============================================================" << std::endl;
    uint32_t golden = 0x2589b2e0;
    uint32_t err = 0;
    for (int i = 0; i < num; i++) {
        if (out_result[i] != golden) {
            printf("out_result[%d] = %08x\n", i, out_result[i]);
            err++;
        }
    }
    if (err) {
        std::cout << "Test failed.\n";
    } else {
        std::cout << "Test passed.\n";
    }

    return err;
}
