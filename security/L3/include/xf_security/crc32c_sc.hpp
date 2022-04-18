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

#include "xf_security/crc32c_krl.hpp"

#include <algorithm>
#include <iterator>

#include <sys/time.h>
#include <new>
#include <cstdlib>
#include <ap_int.h>
#include <iostream>

#include <vector>
#include <string>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

/**
 * @brief calculates the CRC32C of the data provided in input file vector, the results are emitted to the output result
 * vector.
 *
 * @param in_file input file vector.
 * @param in_size size of each input file in bytes.
 * @param out_result output result vector.
 *
 * @return 0 if run succeed, otherwise returns non-zero values.
 */
int crc32c_run(std::vector<std::string> in_file, std::vector<uint32_t> in_size, std::vector<uint32_t>& out_result);

/**
 * @brief aligned memory allocator.
 *
 * @tparam T data type of the element.
 *
 * @param num number of the elements in the buffer.
 *
 * @return the pointer to the allocated buffer.
 */
template <typename T>
T* aligned_alloc(std::size_t num) {
    void* ptr = nullptr;
    if (posix_memalign(&ptr, 4096, num * sizeof(T))) throw std::bad_alloc();
    return reinterpret_cast<T*>(ptr);
}

/**
 * @brief time counter
 *
 * @param tv0 start time.
 * @prram tv1 end time.
 *
 * @return number of us of the duration.
 */
inline int tvdiff(struct timeval* tv0, struct timeval* tv1) {
    return (tv1->tv_sec - tv0->tv_sec) * 1000000 + (tv1->tv_usec - tv0->tv_usec);
}
