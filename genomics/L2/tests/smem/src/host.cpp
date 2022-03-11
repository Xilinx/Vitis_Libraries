/*
 * (c) Copyright 2022 Xilinx, Inc. All rights reserved.
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
#include "gensynhelper.hpp"

int main(int argc, char** argv) {
    std::srand(std::time(nullptr));
    if (argc != 3 && argc != 5) {
        printf("Usage: %s bitstream testcase_dir [number of tests] [batch_size] \n", argv[0]);
        printf("  Or:  %s bitstream testcase_dir \n", argv[0]);
        return -1;
    }
    int test_num = 1024;
    int batch_size = 256 * COMPUTE_UNIT;

    if (argc != 3) {
        test_num = atoi(argv[3]);
        batch_size = atoi(argv[4]);
    }
    randomTest(argv[1], argv[2], test_num, batch_size);
    return 0;
}
