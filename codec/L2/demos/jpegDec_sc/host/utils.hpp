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

#ifndef UTILS_H
#define UTILS_H
#include <sys/time.h>
#include <new>
#include <cstdlib>
#include <algorithm>
#include <string>
#include <vector>
// ------------------------------------------------------------

#if __linux
template <typename T>
T* aligned_alloc(std::size_t num) {
    void* ptr = nullptr;
    if (posix_memalign(&ptr, 4096, num * sizeof(T))) {
        throw std::bad_alloc();
    }
    return reinterpret_cast<T*>(ptr);
}
#endif

// ------------------------------------------------------------
// Compute time difference
unsigned long diff(const struct timeval* newTime, const struct timeval* oldTime) {
    return (newTime->tv_sec - oldTime->tv_sec) * 1000000 + (newTime->tv_usec - oldTime->tv_usec);
}

// ------------------------------------------------------------
// load the data file (.txt, .bin, .jpg ...)to ptr
template <typename T>
int load_dat(T*& data, const std::string& name, int& size) {
    uint64_t n;
    std::string fn = name;
    FILE* f = fopen(fn.c_str(), "rb");
    std::cout << "WARNING: " << fn << " will be opened for binary read." << std::endl;
    if (!f) {
        std::cerr << "ERROR: " << fn << " cannot be opened for binary read." << std::endl;
        return -1;
    }

    fseek(f, 0, SEEK_END);
    n = (uint64_t)ftell(f);
    if (n > MAX_DEC_PIX) {
        std::cout << " read n bytes > MAX_DEC_PIX, please set a larger MAX_DEC_PIX " << std::endl;
        return 1;
    }
#if __linux
    data = aligned_alloc<T>(n);
#else
    data = (T*)malloc(MAX_DEC_PIX);
#endif
    fseek(f, 0, SEEK_SET);
    size = fread(data, sizeof(char), n, f);
    fclose(f);
    std::cout << n << " entries read from " << fn << std::endl;

    return 0;
}

// ------------------------------------------------------------
// get the arg
class ArgParser {
   public:
    ArgParser(int& argc, const char* argv[]) {
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
    bool getCmdOption(const std::string option) const {
        std::vector<std::string>::const_iterator itr;
        itr = std::find(this->mTokens.begin(), this->mTokens.end(), option);
        if (itr != this->mTokens.end())
            return true;
        else
            return false;
    }

   private:
    std::vector<std::string> mTokens;
};

#endif