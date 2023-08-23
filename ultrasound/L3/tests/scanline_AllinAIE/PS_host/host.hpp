/*
 * Copyright (C) 2019-2022, Xilinx, Inc.
 * Copyright (C) 2022-2023, Advanced Micro Devices, Inc.
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

#pragma once

#define CL_HPP_CL_1_2_DEFAULT_BUILD
#define CL_HPP_TARGET_OPENCL_VERSION 120
#define CL_HPP_MINIMUM_OPENCL_VERSION 120
#define CL_HPP_ENABLE_PROGRAM_CONSTRUCTION_FROM_ARRAY_COMPATIBILITY 1

#include <CL/cl2.hpp>
#include <stdlib.h>
// This is used for the PL Kernels
#include "xrt.h"
#include "experimental/xrt_kernel.h"

// Using the ADF API that call XRT API
#include "adf/adf_api/XRTConfig.h"

// Customized buffer allocation for 4K boundary alignment
template <typename T>
struct aligned_allocator {
    using value_type = T;
    T* allocate(std::size_t num) {
        void* ptr = nullptr;
        if (posix_memalign(&ptr, 4096, num * sizeof(T))) throw std::bad_alloc();
        return reinterpret_cast<T*>(ptr);
    }
    void deallocate(T* p, std::size_t num) { free(p); }
};

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

static std::vector<char> load_xclbin(xrtDeviceHandle device, const std::string& fnm) {
    if (fnm.empty()) throw std::runtime_error("No xclbin specified");

    // load bit stream
    std::ifstream stream(fnm);
    stream.seekg(0, stream.end);
    size_t size = stream.tellg();
    stream.seekg(0, stream.beg);

    std::vector<char> header(size);
    stream.read(header.data(), size);

    auto top = reinterpret_cast<const axlf*>(header.data());
    if (xrtDeviceLoadXclbin(device, top)) throw std::runtime_error("Xclbin loading failed");

    return header;
}

template <typename T>
T* data_loading(std::string filename, int& size) {
    T* buffer;

    std::ifstream infile(filename, std::ios::in);

    if (infile.is_open()) {
        std::string line;
        for (size = 0; std::getline(infile, line); ++size)
            ;

        // back to the beginning of file
        infile.clear();
        infile.seekg(0, infile.beg);

        // data loading
        buffer = new T[size];

        for (int i = 0; i < size; i++) {
            infile >> buffer[i];
        }
    } else {
        std::cout << "direction input is empty!" << std::endl;
    }

    infile.close();

    std::cout << "file:" << filename << " size:" << size << std::endl;
    return buffer;
}
/*
template <typename T>
void Json_out(std::string filename, int& size, T* p_output_data){
    std::fstream outJson;
    nlohmann::json json_out;
    std::string res;

    //std::string output_json_path = "xf_output_res_Line.json";
    outJson.open(filename, std::ios::out);
    if (!outJson.is_open()) {
        std::cout << "Can't open file" << std::endl;
    }

    float rf_data_[size] = {0};
    json_out["rf_data"] = p_output_data;

    res = json_out.dump();

    outJson << res;
    outJson.close();
}
*/
template <typename T, typename T_cast>
void writeFile(T* input, int N, std::string filename) {
    FILE* file = std::fopen(filename.c_str(), "w");
    if (file) {
        for (int i = 0; i < N; i += 1) {
            std::fprintf(file, "%.9f\n", input[i]);
        }
        std::fclose(file);
    }
}

template <typename T>
void save_les_bin(T* rf_data_les, const char* nm, int num_line, int num_sample, int num_upSample) {
    char fname[128];
    sprintf(fname, "data_%s_line%d_element%d_sample.bin", nm, num_line, num_sample, num_upSample);
    FILE* fp = fopen(nm, "w");
    assert(fp);
    assert(rf_data_les);
    long size = num_line * num_sample * num_upSample;
    fwrite((void*)rf_data_les, sizeof(float), size, fp);
    fclose(fp);
    // printf("%s: RF data saved in file %s in bin format\n",m_name_full, fname);
}

template <typename T>
void system_md5(std::string fn) {
    char cmd[256];
    if (strlen(fn.c_str()) > 256 - strlen("md5sum ")) return;
    sprintf(cmd, "md5sum %s", fn.c_str());
    system(cmd);
}