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
#include <sys/time.h>
#include <cstring>
#include <fstream>
#include <iostream>
#include <vector>
#include "MCAE_kernel.hpp"
#include "ap_int.h"
#include "utils.hpp"
#include "xcl2.hpp"

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

template <int UN>
void readAsset(int steps, ap_uint<UN * SZ>* inPrice, std::string& file_name) {
    std::ifstream infile(file_name);
    std::string line;
    for (int i = 0; i < steps; ++i) {
        ap_uint<UN* SZ> out = 0;
        for (int k = 0; k < UN; ++k) {
            if (std::getline(infile, line)) {
                double d = std::stod(line);
                out((k + 1) * SZ - 1, k * SZ) = xf::fintech::internal::doubleToBits(d);
            } else {
                std::cout << "Read asset number wrong\n";
            }
        }
        inPrice[i] = out;
    }
    infile.close();
}
template <int UN>
void readAsset(int steps, int paths, ap_uint<UN * SZ>* inPrice, std::string& file_name) {
    std::ifstream infile(file_name);
    std::string line;
    for (int i = 0; i < steps; ++i) {
        for (int l = 0; l < paths / UN; ++l) {
            ap_uint<UN* SZ> out = 0;
            for (int k = 0; k < UN; ++k) {
                if (std::getline(infile, line)) {
                    double d = std::stod(line);
                    out((k + 1) * SZ - 1, k * SZ) = xf::fintech::internal::doubleToBits(d);
                } else {
                    std::cout << "Read asset number wrong\n";
                }
            }
            inPrice[i * paths / UN + l] = out;
        }
    }
    infile.close();
}
template <int UN>
void writeAsset(int steps, ap_uint<UN * SZ>* outPrice, std::string& file_name) {
    std::ofstream outfile(file_name);
    for (int i = 0; i < steps; ++i) {
        ap_uint<UN* SZ> out = outPrice[i];
        for (int k = 0; k < UN; ++k) {
            uint64_t in = out((k + 1) * SZ - 1, k * SZ);
            double inD = xf::fintech::internal::bitsToDouble(in);
            outfile << std::setprecision(15) << inD << std::endl;
        }
    }
    outfile.close();
}
template <int UN>
void writeAsset(int steps, int paths, ap_uint<UN * SZ>* outPrice, std::string& file_name) {
    std::ofstream outfile(file_name);
    for (int i = 0; i < steps; ++i) {
        for (int l = 0; l < paths / UN; ++l) {
            ap_uint<UN* SZ> out = outPrice[i * paths / UN + l];
            for (int k = 0; k < UN; ++k) {
                uint64_t in = out((k + 1) * SZ - 1, k * SZ);
                double inD = xf::fintech::internal::bitsToDouble(in);
                outfile << std::setprecision(15) << inD << std::endl;
            }
        }
    }
    outfile.close();
}
int main(int argc, const char* argv[]) {
    std::cout << "\n----------------------MC(American) Engine-----------------\n";
    // cmd parser
    ArgParser parser(argc, argv);
    std::string mode;
    std::string xclbin_path;
    if (parser.getCmdOption("-mode", mode) && mode == "fpga") {
        // run_fpga = true;
        if (!parser.getCmdOption("-xclbin", xclbin_path)) {
            std::cout << "ERROR:xclbin path is not set!\n";
            return 1;
        }
    }
    // number of call for kernel
    int loop_nm = 1;
    // AXI depth
    int data_size = depthP;            //= depthP = 1024(calibrate samples)*10(steps)
                                       //*2(iter), width: 64*UN
    int matdata_size = depthM;         //=depthM = 9*10(steps)*2(iter), width: 64
    int coefdata_size = TIMESTEPS - 1; // 9;//=(steps-1), width: 4*64
    std::cout << "data_size is " << data_size << std::endl;

    ap_uint<64 * UN_PATH>* output_price = aligned_alloc<ap_uint<64 * UN_PATH> >(data_size); // 64*UN
    ap_uint<64>* output_mat = aligned_alloc<ap_uint<64> >(matdata_size);
    ap_uint<64 * COEF>* coef = aligned_alloc<ap_uint<64 * COEF> >(coefdata_size);
    TEST_DT* output = aligned_alloc<TEST_DT>(1);
    TEST_DT* output_1 = aligned_alloc<TEST_DT>(1);

    // -------------setup params---------------
    int timeSteps = 100;
    TEST_DT underlying = 36;
    TEST_DT strike = 40.0;
    int optionType = 1;
    TEST_DT volatility = 0.20;
    TEST_DT riskFreeRate = 0.06;
    TEST_DT dividendYield = 0.0;

    // do pre-process on CPU
    TEST_DT timeLength = 1;
    TEST_DT requiredTolerance = 0.02;
    unsigned int requiredSamples = 24576;
    int calibSamples = 4096;
    int maxsamples = 0;
    struct timeval start_time, end_time;
    std::string in_str;
    if (parser.getCmdOption("-cal", in_str)) {
        try {
            calibSamples = std::stoi(in_str);
        } catch (...) {
            calibSamples = 4096;
        }
    }
    if (parser.getCmdOption("-s", in_str)) {
        try {
            timeSteps = std::stoi(in_str);
        } catch (...) {
            timeSteps = 100;
        }
    }
    if (parser.getCmdOption("-p", in_str)) {
        try {
            requiredSamples = std::stoi(in_str);
        } catch (...) {
            requiredSamples = 24576;
        }
    }
    // platform related operations
    std::vector<cl::Device> devices = xcl::get_xil_devices();
    cl::Device device = devices[0];

    // Creating Context and Command Queue for selected Device
    cl::Context context(device);
    cl::CommandQueue q(context, device, CL_QUEUE_PROFILING_ENABLE | CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE);
    std::string devName = device.getInfo<CL_DEVICE_NAME>();
    printf("Found Device=%s\n", devName.c_str());

    // cl::Program::Binaries xclBins =
    // xcl::import_binary_file("../xclbin/MCAE_u250_hw.xclbin");
    cl::Program::Binaries xclBins = xcl::import_binary_file(xclbin_path);
    devices.resize(1);
    cl::Program program(context, devices, xclBins);
    cl::Kernel kernel_MCAE_k0(program, "MCAE_k0");
    std::cout << "kernel has been created" << std::endl;

    cl_mem_ext_ptr_t mext_o[3];
    mext_o[0].flags = XCL_MEM_DDR_BANK0;
    mext_o[0].obj = output_price;
    mext_o[0].param = 0;

    mext_o[1].flags = XCL_MEM_DDR_BANK0;
    mext_o[1].obj = output_mat;
    mext_o[1].param = 0;

    mext_o[2].flags = XCL_MEM_DDR_BANK0;
    mext_o[2].obj = output;
    mext_o[2].param = 0;

    // create device buffer and map dev buf to host buf
    cl::Buffer output_price_buf, output_mat_buf, coef_buf, output_buf; //, output_buf_1;
    output_price_buf = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                                  sizeof(ap_uint<64 * UN_PATH>) * data_size, &mext_o[0]);
    output_mat_buf = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                                sizeof(ap_uint<64>) * matdata_size, &mext_o[1]);
    output_buf = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE, sizeof(TEST_DT),
                            &mext_o[2]);

    std::vector<cl::Memory> ob_out;
    ob_out.push_back(output_buf);

    q.finish();
    gettimeofday(&start_time, 0);
    for (int i = 0; i < loop_nm; ++i) {
        kernel_MCAE_k0.setArg(0, underlying);
        kernel_MCAE_k0.setArg(1, volatility);
        kernel_MCAE_k0.setArg(2, riskFreeRate);
        kernel_MCAE_k0.setArg(3, dividendYield);
        kernel_MCAE_k0.setArg(4, timeLength);
        kernel_MCAE_k0.setArg(5, strike);
        kernel_MCAE_k0.setArg(6, optionType);
        kernel_MCAE_k0.setArg(7, output_price_buf);
        kernel_MCAE_k0.setArg(8, output_mat_buf);
        kernel_MCAE_k0.setArg(9, output_buf);
        kernel_MCAE_k0.setArg(10, requiredTolerance);
        kernel_MCAE_k0.setArg(11, calibSamples);
        kernel_MCAE_k0.setArg(12, requiredSamples);
        kernel_MCAE_k0.setArg(13, timeSteps);

        // launch kernel and calculate kernel execution time
        std::cout << "kernel start------" << std::endl;
        q.enqueueTask(kernel_MCAE_k0, nullptr, nullptr);
        q.finish();
    }
    std::cout << "kernel end------" << std::endl;
    q.enqueueMigrateMemObjects(ob_out, 1, nullptr, nullptr);
    q.finish();
    gettimeofday(&end_time, 0);
    std::cout << "Execution time " << tvdiff(&start_time, &end_time) << "us" << std::endl;

    TEST_DT out = output[0];
    std::cout << "output = " << out << std::endl;

    // verify the output price with golden
    // reference value:
    //  - for 112 (UN_PATH=2, UN_STEP=2, UN_PRICING=2), 3.97471
    // notice that when the employed seed changes, the result also varies.
    double golden_output = 3.97;
    double diff = out - golden_output;
    if (diff < 0.2 && diff > -0.2) {
        std::cout << "PASSED!!! the output is confidential!" << std::endl;
    } else {
        std::cout << "FAILURE!!! incorrect ouput value calculated!" << std::endl;
    }

    return 0;
}
