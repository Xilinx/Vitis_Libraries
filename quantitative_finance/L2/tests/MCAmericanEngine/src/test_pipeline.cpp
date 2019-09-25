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
                out((k + 1) * SZ - 1, k * SZ) = doubleToBits(d);
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
                    out((k + 1) * SZ - 1, k * SZ) = doubleToBits(d);
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
            double inD = bitsToDouble(in);
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
                double inD = bitsToDouble(in);
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
    if (!parser.getCmdOption("-xclbin", xclbin_path)) {
        std::cout << "ERROR:xclbin path is not set!\n";
        return 1;
    }
    // AXI depth
    int data_size = depthP;            //= depthP = 1024(calibrate samples)*10(steps)
                                       //*2(iter), width: 64*UN
    int matdata_size = depthM;         //=depthM = 9*10(steps)*2(iter), width: 64
    int coefdata_size = TIMESTEPS - 1; // 9;//=(steps-1), width: 4*64
    std::cout << "data_size is " << data_size << std::endl;

    // pipeline mode buffer a
    ap_uint<64 * UN_PATH>* output_price_a = aligned_alloc<ap_uint<64 * UN_PATH> >(data_size); // 64*UN
    ap_uint<64>* output_mat_a = aligned_alloc<ap_uint<64> >(matdata_size);
    TEST_DT* output_a = aligned_alloc<TEST_DT>(1);
    // pipeline mode buffer b
    ap_uint<64 * UN_PATH>* output_price_b = aligned_alloc<ap_uint<64 * UN_PATH> >(data_size); // 64*UN
    ap_uint<64>* output_mat_b = aligned_alloc<ap_uint<64> >(matdata_size);
    TEST_DT* output_b = aligned_alloc<TEST_DT>(1);

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
    cl::Kernel kernel_MCAE_k0_a(program, "MCAE_k0");
    cl::Kernel kernel_MCAE_k0_b(program, "MCAE_k0");
    std::cout << "kernel has been created" << std::endl;

    cl_mem_ext_ptr_t mext_o_a[3];
    mext_o_a[0].flags = XCL_MEM_DDR_BANK0;
    mext_o_a[0].obj = output_price_a;
    mext_o_a[0].param = 0;

    mext_o_a[1].flags = XCL_MEM_DDR_BANK0;
    mext_o_a[1].obj = output_mat_a;
    mext_o_a[1].param = 0;

    mext_o_a[2].flags = XCL_MEM_DDR_BANK0;
    mext_o_a[2].obj = output_a;
    mext_o_a[2].param = 0;

    cl_mem_ext_ptr_t mext_o_b[3];
    mext_o_b[0].flags = XCL_MEM_DDR_BANK0;
    mext_o_b[0].obj = output_price_b;
    mext_o_b[0].param = 0;

    mext_o_b[1].flags = XCL_MEM_DDR_BANK0;
    mext_o_b[1].obj = output_mat_b;
    mext_o_b[1].param = 0;

    mext_o_b[2].flags = XCL_MEM_DDR_BANK0;
    mext_o_b[2].obj = output_b;
    mext_o_b[2].param = 0;

    // create device buffer and map dev buf to host buf
    cl::Buffer output_price_buf_a, output_mat_buf_a, output_buf_a;
    output_price_buf_a = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                                    sizeof(ap_uint<64 * UN_PATH>) * data_size, &mext_o_a[0]);
    output_mat_buf_a = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                                  sizeof(ap_uint<64>) * matdata_size, &mext_o_a[1]);
    output_buf_a = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE, sizeof(TEST_DT),
                              &mext_o_a[2]);

    cl::Buffer output_price_buf_b, output_mat_buf_b, output_buf_b;
    output_price_buf_b = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                                    sizeof(ap_uint<64 * UN_PATH>) * data_size, &mext_o_b[0]);
    output_mat_buf_b = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                                  sizeof(ap_uint<64>) * matdata_size, &mext_o_b[1]);
    output_buf_b = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE, sizeof(TEST_DT),
                              &mext_o_b[2]);

    std::vector<cl::Memory> ob_out_a;
    ob_out_a.push_back(output_buf_a);

    std::vector<cl::Memory> ob_out_b;
    ob_out_b.push_back(output_buf_b);

    int loop_nm = 100;
    std::vector<std::vector<cl::Event> > evt0(loop_nm);
    std::vector<std::vector<cl::Event> > evt1(loop_nm);
    for (int i = 0; i < loop_nm; i++) {
        evt0[i].resize(1);
        evt1[i].resize(1);
    }
    // setup kernel args
    kernel_MCAE_k0_a.setArg(0, underlying);
    kernel_MCAE_k0_a.setArg(1, volatility);
    kernel_MCAE_k0_a.setArg(2, riskFreeRate);
    kernel_MCAE_k0_a.setArg(3, dividendYield);
    kernel_MCAE_k0_a.setArg(4, timeLength);
    kernel_MCAE_k0_a.setArg(5, strike);
    kernel_MCAE_k0_a.setArg(6, optionType);
    kernel_MCAE_k0_a.setArg(7, output_price_buf_a);
    kernel_MCAE_k0_a.setArg(8, output_mat_buf_a);
    kernel_MCAE_k0_a.setArg(9, output_buf_a);
    kernel_MCAE_k0_a.setArg(10, requiredTolerance);
    kernel_MCAE_k0_a.setArg(11, calibSamples);
    kernel_MCAE_k0_a.setArg(12, requiredSamples);
    kernel_MCAE_k0_a.setArg(13, timeSteps);

    kernel_MCAE_k0_b.setArg(0, underlying);
    kernel_MCAE_k0_b.setArg(1, volatility);
    kernel_MCAE_k0_b.setArg(2, riskFreeRate);
    kernel_MCAE_k0_b.setArg(3, dividendYield);
    kernel_MCAE_k0_b.setArg(4, timeLength);
    kernel_MCAE_k0_b.setArg(5, strike);
    kernel_MCAE_k0_b.setArg(6, optionType);
    kernel_MCAE_k0_b.setArg(7, output_price_buf_b);
    kernel_MCAE_k0_b.setArg(8, output_mat_buf_b);
    kernel_MCAE_k0_b.setArg(9, output_buf_b);
    kernel_MCAE_k0_b.setArg(10, requiredTolerance);
    kernel_MCAE_k0_b.setArg(11, calibSamples);
    kernel_MCAE_k0_b.setArg(12, requiredSamples);
    kernel_MCAE_k0_b.setArg(13, timeSteps);

    std::cout << "kernel start------" << std::endl;
    // launch kernel and calculate kernel execution time
    q.finish();
    gettimeofday(&start_time, 0);
    for (int i = 0; i < loop_nm; ++i) {
        int use_a = i & 1;
        if (use_a) {
            if (i < 2) {
                q.enqueueTask(kernel_MCAE_k0_a, nullptr, &evt0[i][0]);
            } else {
                q.enqueueTask(kernel_MCAE_k0_a, &evt1[i - 2], &evt0[i][0]);
            }
            q.enqueueMigrateMemObjects(ob_out_a, 1, &evt0[i], &evt1[i][0]);
        } else {
            if (i < 2) {
                q.enqueueTask(kernel_MCAE_k0_b, nullptr, &evt0[i][0]);
            } else {
                q.enqueueTask(kernel_MCAE_k0_b, &evt1[i - 2], &evt0[i][0]);
            }
            q.enqueueMigrateMemObjects(ob_out_b, 1, &evt0[i], &evt1[i][0]);
        }
    }
    q.finish();
    gettimeofday(&end_time, 0);
    std::cout << "kernel end------" << std::endl;
    std::cout << "Execution time " << tvdiff(&start_time, &end_time) << "us" << std::endl;
    TEST_DT out = output_a[0];
    std::cout << "output = " << out << std::endl;

    return 0;
}
