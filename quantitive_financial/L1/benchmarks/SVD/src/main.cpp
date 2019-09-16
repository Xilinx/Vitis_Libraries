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
#include <iostream>
#include "svd.hpp"
#include "util.hpp"

int main(int argc, char** argv) {
    std::string xclbinName = read_verify_env_string("XCLBINNAME");
    std::string input_path = read_verify_env_string("INPATH");
    std::string output_path = read_verify_env_string("OUTPATH");

    std::cout << "Environment info XCLBINNAME: " << xclbinName << std::endl;
    std::cout << "Environment info OUTPATH: " << input_path << std::endl;
    std::cout << "Environment info OUTPATH: " << output_path << std::endl;

    std::cout << "xclbin is " << xclbinName << std::endl;

    double errA;
    benchmark_svd_functions(input_path, xclbinName, output_path, errA);

    if (errA > 0.0001) {
        std::cout << "result false" << std::endl;
        return -1;
    } else {
        std::cout << "result correct" << std::endl;
        return 0;
    }
}
