/**********
© Copyright (C) 2019-2022, Xilinx, Inc.
© Copyright (C) 2022-2023, Advanced Micro Devices, Inc.
Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at
    http://www.apache.org/licenses/LICENSE-2.0
Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
**********/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <fstream>
#include <string>

// This is used for the PL Kernels
#include "xrt.h"
#include "experimental/xrt_kernel.h"

// Using the ADF API that call XRT API
#include "adf/adf_api/XRTConfig.h"

#include "scanline.hpp"

#define INPUT_RANGE 28
#define OUTPUT_RANGE 6
#define ITER 1

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

void init_output_buffer(float* res_out[], float* res_out_size) {
    /* for this test, assume 1 graph run in aie processing */

    float golden_sizeOut[OUTPUT_RANGE];
    golden_sizeOut[0] = 128; // image-points output
    golden_sizeOut[1] = 32;  // delay output
    golden_sizeOut[2] = 32;  // focusing output
    golden_sizeOut[3] = 32;  // samples
    golden_sizeOut[4] = 32;  // apodization
    golden_sizeOut[5] = 128; // apodization

    // output buffer init
    for (int i = 0; i < OUTPUT_RANGE; i++) {
        int size = golden_sizeOut[i];
        res_out[i] = new float[size];
    }

    // size init
    for (int i = 0; i < OUTPUT_RANGE; i++) {
        res_out_size[i] = golden_sizeOut[i];
    }
}

int main(int argc, const char** argv) {
    //////////////////////////////////////////
    // input cmd parser
    //////////////////////////////////////////
    ArgParser parser(argc, argv);

    std::string xclbin_path;
    if (!parser.getCmdOption("-xclbin", xclbin_path)) {
        std::cout << "ERROR:[-xclbin] xclbin path is not set!\n";
        return 1;
    }

    std::string data_path;
    if (!parser.getCmdOption("-data", data_path)) {
        std::cout << "ERROR:[-data] data path is not set!\n";
        return 1;
    }

    // output result buffer
    float* res_out[OUTPUT_RANGE];
    float res_out_size[OUTPUT_RANGE];

    // init output buffer
    init_output_buffer(res_out, res_out_size);

    // run aie application
    us::L3::scanline<>(xclbin_path, data_path, res_out, res_out_size, ITER);

    //////////////////////////////////////////
    // Comparing the execution data to the golden data
    //////////////////////////////////////////

    int err_cnt = 0;
    std::string golden_file_set[OUTPUT_RANGE];
    golden_file_set[0] = data_path + "golden/image_points.txt";
    golden_file_set[1] = data_path + "golden/delay_to_PL.txt";
    golden_file_set[2] = data_path + "golden/focusing_output.txt";
    golden_file_set[3] = data_path + "golden/samples_to_PL.txt";
    golden_file_set[4] = data_path + "golden/apodization.txt";
    golden_file_set[5] = data_path + "golden/C.txt";

    // loading golden data
    float* golden_buf_mem[OUTPUT_RANGE];
    int golden_sizeOut[OUTPUT_RANGE];

    // mem data output length define
    golden_sizeOut[0] = 128; // image-points output
    golden_sizeOut[1] = 32;  // delay output
    golden_sizeOut[2] = 32;  // focusing output
    golden_sizeOut[3] = 32;  // samples
    golden_sizeOut[4] = 32;  // apodization
    golden_sizeOut[5] = 128; // interpolation

    for (int i = 0; i < OUTPUT_RANGE; i++) {
        golden_buf_mem[i] = data_loading<float>(golden_file_set[i], golden_sizeOut[i]);
    }

    for (int k = 0; k < OUTPUT_RANGE; k++) {
        std::cout << golden_file_set[k] << std::endl;
        for (int i = 0; i < golden_sizeOut[k]; i++) {
            float result_tmp = (float)res_out[k][i];
            float result_golden = (float)golden_buf_mem[k][i];
            if (result_tmp != result_golden) {
                printf("i:%d, golden:%f result:%f\n", i, result_golden, result_tmp);
                err_cnt++;
            }
        }
    }

    for (int i = 0; i < OUTPUT_RANGE; i++) {
        delete[] res_out[i];
    }

    // End
    printf("Test Done, err_cnt:%d\n", err_cnt);

    return err_cnt;
}