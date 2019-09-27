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
#ifndef HLS_TEST
#include "xcl2.hpp"
#endif
#include <sys/time.h>
#include <cstring>
#include <fstream>
#include <iostream>
#include <vector>
#include "MCAE_kernel.hpp"
#include "ap_int.h"
#include "utils.hpp"
#ifndef HLS_TEST
#define TEST_k0
#define TEST_k1
#define TEST_k2
#endif

#define XCL_BANK(n) (((unsigned int)(n)) | XCL_MEM_TOPOLOGY)

#define XCL_BANK0 XCL_BANK(0)
#define XCL_BANK1 XCL_BANK(1)
#define XCL_BANK2 XCL_BANK(2)
#define XCL_BANK3 XCL_BANK(3)
#define XCL_BANK4 XCL_BANK(4)
#define XCL_BANK5 XCL_BANK(5)
#define XCL_BANK6 XCL_BANK(6)
#define XCL_BANK7 XCL_BANK(7)
#define XCL_BANK8 XCL_BANK(8)
#define XCL_BANK9 XCL_BANK(9)
#define XCL_BANK10 XCL_BANK(10)
#define XCL_BANK11 XCL_BANK(11)
#define XCL_BANK12 XCL_BANK(12)
#define XCL_BANK13 XCL_BANK(13)
#define XCL_BANK14 XCL_BANK(14)
#define XCL_BANK15 XCL_BANK(15)
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
    std::string xclbin_path;
#ifndef HLS_TEST
    if (!parser.getCmdOption("-xclbin", xclbin_path)) {
        std::cout << "ERROR:xclbin path is not set!\n";
        return 1;
    }
#endif
    std::string mode = "hw";
    if (std::getenv("XCL_EMULATION_MODE") != nullptr) {
        mode = std::getenv("XCL_EMULATION_MODE");
    }
    std::cout << "[INFO]Running in " << mode << " mode" << std::endl;
    // AXI depth
    int data_size = depthP;            // 20480;//= depthP = 1024(calibrate
                                       // samples)*10(steps) *2(iter), width: 64*UN
    int matdata_size = depthM;         ////180;//=depthM = 9*10(steps)*2(iter), width: 64
    int coefdata_size = TIMESTEPS - 1; // 9;//=(steps-1), width: 4*64
    std::cout << "data_size is " << data_size << std::endl;

    ap_uint<64 * UN_K1>* output_price = aligned_alloc<ap_uint<64 * UN_K1> >(data_size); // 64*UN
    ap_uint<64>* output_mat = aligned_alloc<ap_uint<64> >(matdata_size);
    ap_uint<64 * COEF>* coef = aligned_alloc<ap_uint<64 * COEF> >(coefdata_size);
    TEST_DT* output = aligned_alloc<TEST_DT>(1);
    TEST_DT* output2 = aligned_alloc<TEST_DT>(1);

    ap_uint<64 * UN_K1>* output_price_b = aligned_alloc<ap_uint<64 * UN_K1> >(data_size); // 64*UN
    ap_uint<64>* output_mat_b = aligned_alloc<ap_uint<64> >(matdata_size);
    ap_uint<64 * COEF>* coef_b = aligned_alloc<ap_uint<64 * COEF> >(coefdata_size);
    TEST_DT* output_b = aligned_alloc<TEST_DT>(1);
    TEST_DT* output2_b = aligned_alloc<TEST_DT>(1);

    // -------------setup params---------------

    int timeSteps = 100;
    TEST_DT underlying = 36;
    TEST_DT strike = 40.0;
    int optionType = 1;
    TEST_DT volatility = 0.20;
    TEST_DT riskFreeRate = 0.06;
    TEST_DT dividendYield = 0.0;
    TEST_DT timeLength = 1;
    TEST_DT requiredTolerance = 0.02;

    unsigned int requiredSamples = 24576 / KN2;
    int calibSamples = 4096;
    int maxsamples = 0;
    double golden_output = 3.978;
    std::string num_str;
    int loop_nm = 100;
    if (parser.getCmdOption("-cal", num_str)) {
        try {
            calibSamples = std::stoi(num_str);
        } catch (...) {
            calibSamples = 4096;
        }
    }
    if (parser.getCmdOption("-s", num_str)) {
        try {
            timeSteps = std::stoi(num_str);
        } catch (...) {
            timeSteps = 100;
        }
    }
    if (parser.getCmdOption("-p", num_str)) {
        try {
            requiredSamples = std::stoi(num_str);
        } catch (...) {
            requiredSamples = 24576 / KN2;
        }
    }

    if (mode.compare("hw_emu") == 0) {
        timeSteps = UN_K2_STEP;
        golden_output = 4.18;
        loop_nm = 1;
    } else if (mode.compare("sw_emu") == 0) {
        loop_nm = 1;
    }
    std::cout << "loop_nm: " << loop_nm << std::endl;
    std::cout << "paths: " << requiredSamples << std::endl;
#ifdef HLS_TEST
    MCAE_k0(underlying, volatility, riskFreeRate, dividendYield, timeLength, strike, optionType, output_price_b,
            output_mat_b, calibSamples, timeSteps);
    MCAE_k1(timeLength, riskFreeRate, strike, optionType, output_price_b, output_mat_b, coef_b, calibSamples,
            timeSteps);
    MCAE_k2(underlying, volatility, dividendYield, riskFreeRate, timeLength, strike, optionType, coef_b, output_b,
            requiredTolerance, requiredSamples, timeSteps);
#else
    struct timeval start_time, end_time;
    // platform related operations
    std::vector<cl::Device> devices = xcl::get_xil_devices();
    cl::Device device = devices[0];

    // Creating Context and Command Queue for selected Device
    cl::Context context(device);
#ifdef OUT_OF_ORDER
    cl::CommandQueue q(context, device, CL_QUEUE_PROFILING_ENABLE | CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE);
#else
    cl::CommandQueue q(context, device, CL_QUEUE_PROFILING_ENABLE);
#endif

    std::string devName = device.getInfo<CL_DEVICE_NAME>();
    printf("Found Device=%s\n", devName.c_str());

    // cl::Program::Binaries xclBins =
    // xcl::import_binary_file("../xclbin/MCAE_u250_hw.xclbin");
    cl::Program::Binaries xclBins = xcl::import_binary_file(xclbin_path);
    devices.resize(1);
    cl::Program program(context, devices, xclBins);
    cl::Kernel kernel_MCAE_k0[2];
    kernel_MCAE_k0[0] = cl::Kernel(program, "MCAE_k0");
    kernel_MCAE_k0[1] = cl::Kernel(program, "MCAE_k0");

    cl::Kernel kernel_MCAE_k1[2];
    kernel_MCAE_k1[0] = cl::Kernel(program, "MCAE_k1");
    kernel_MCAE_k1[1] = cl::Kernel(program, "MCAE_k1");

    cl::Kernel kernel_MCAE_k2[2];
    kernel_MCAE_k2[0] = cl::Kernel(program, "MCAE_k2");
    kernel_MCAE_k2[1] = cl::Kernel(program, "MCAE_k2");

#if KN2 == 2
    cl::Kernel kernel_MCAE_k3[2];
    kernel_MCAE_k3[0] = cl::Kernel(program, "MCAE_k3");
    kernel_MCAE_k3[1] = cl::Kernel(program, "MCAE_k3");
#endif

    std::cout << "kernel has been created" << std::endl;

    cl_mem_ext_ptr_t mext_o[5];
#ifndef USE_HBM
    mext_o[0].flags = XCL_MEM_DDR_BANK0;
#else
    mext_o[0].flags = XCL_BANK0;
#endif
    mext_o[0].obj = output_price;
    mext_o[0].param = 0;

#ifndef USE_HBM
    mext_o[1].flags = XCL_MEM_DDR_BANK1;
#else
    mext_o[1].flags = XCL_BANK1;
#endif
    mext_o[1].obj = output_mat;
    mext_o[1].param = 0;

#ifndef USE_HBM
    mext_o[2].flags = XCL_MEM_DDR_BANK2;
#else
    mext_o[2].flags = XCL_BANK2;
#endif
    mext_o[2].obj = coef;
    mext_o[2].param = 0;

#ifndef USE_HBM
    mext_o[3].flags = XCL_MEM_DDR_BANK3;
#else
    mext_o[3].flags = XCL_BANK3;
#endif
    mext_o[3].obj = output;
    mext_o[3].param = 0;

#ifndef USE_HBM
    mext_o[4].flags = XCL_MEM_DDR_BANK3;
#else
    mext_o[4].flags = XCL_BANK3;
#endif
    mext_o[4].obj = output2;
    mext_o[4].param = 0;

    cl_mem_ext_ptr_t mext_o_b[5];
#ifndef USE_HBM
    mext_o_b[0].flags = XCL_MEM_DDR_BANK0;
#else
    mext_o_b[0].flags = XCL_BANK0;
#endif
    mext_o_b[0].obj = output_price_b;
    mext_o_b[0].param = 0;

#ifndef USE_HBM
    mext_o_b[1].flags = XCL_MEM_DDR_BANK1;
#else
    mext_o_b[1].flags = XCL_BANK1;
#endif
    mext_o_b[1].obj = output_mat_b;
    mext_o_b[1].param = 0;

#ifndef USE_HBM
    mext_o_b[2].flags = XCL_MEM_DDR_BANK2;
#else
    mext_o_b[2].flags = XCL_BANK2;
#endif
    mext_o_b[2].obj = coef_b;
    mext_o_b[2].param = 0;

#ifndef USE_HBM
    mext_o_b[3].flags = XCL_MEM_DDR_BANK3;
#else
    mext_o_b[3].flags = XCL_BANK3;
#endif
    mext_o_b[3].obj = output_b;
    mext_o_b[3].param = 0;

#ifndef USE_HBM
    mext_o_b[4].flags = XCL_MEM_DDR_BANK3;
#else
    mext_o_b[4].flags = XCL_BANK3;
#endif
    mext_o_b[4].obj = output2_b;
    mext_o_b[4].param = 0;
    // create device buffer and map dev buf to host buf
    cl::Buffer output_price_buf, output_mat_buf, coef_buf, output_buf, output_buf2;
    output_price_buf = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                                  sizeof(ap_uint<64 * UN_K1>) * data_size, &mext_o[0]);
    output_mat_buf = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                                sizeof(ap_uint<64>) * matdata_size, &mext_o[1]);
    coef_buf = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                          sizeof(ap_uint<64 * COEF>) * coefdata_size, &mext_o[2]);
    output_buf = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE, sizeof(TEST_DT),
                            &mext_o[3]);
    output_buf2 = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE, sizeof(TEST_DT),
                             &mext_o[4]);

    cl::Buffer output_price_buf_b, output_mat_buf_b, coef_buf_b, output_buf_b, output_buf2_b;
    output_price_buf_b = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                                    sizeof(ap_uint<64 * UN_K1>) * data_size, &mext_o_b[0]);
    output_mat_buf_b = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                                  sizeof(ap_uint<64>) * matdata_size, &mext_o_b[1]);
    coef_buf_b = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                            sizeof(ap_uint<64 * COEF>) * coefdata_size, &mext_o_b[2]);
    output_buf_b = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE, sizeof(TEST_DT),
                              &mext_o_b[3]);
    output_buf2_b = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                               sizeof(TEST_DT), &mext_o_b[4]);

    std::vector<cl::Memory> ob_out;
    ob_out.push_back(output_buf);
#if KN2 == 2
    ob_out.push_back(output_buf2);
#endif

    std::vector<cl::Memory> ob_out_b;
    ob_out_b.push_back(output_buf_b);
#if KN2 == 2
    ob_out_b.push_back(output_buf2_b);
#endif

    kernel_MCAE_k0[0].setArg(0, underlying);
    kernel_MCAE_k0[0].setArg(1, volatility);
    kernel_MCAE_k0[0].setArg(2, riskFreeRate);
    kernel_MCAE_k0[0].setArg(3, dividendYield);
    kernel_MCAE_k0[0].setArg(4, timeLength);
    kernel_MCAE_k0[0].setArg(5, strike);
    kernel_MCAE_k0[0].setArg(6, optionType);
    kernel_MCAE_k0[0].setArg(7, output_price_buf);
    kernel_MCAE_k0[0].setArg(8, output_mat_buf);
    kernel_MCAE_k0[0].setArg(9, calibSamples);
    kernel_MCAE_k0[0].setArg(10, timeSteps);

    kernel_MCAE_k0[1].setArg(0, underlying);
    kernel_MCAE_k0[1].setArg(1, volatility);
    kernel_MCAE_k0[1].setArg(2, riskFreeRate);
    kernel_MCAE_k0[1].setArg(3, dividendYield);
    kernel_MCAE_k0[1].setArg(4, timeLength);
    kernel_MCAE_k0[1].setArg(5, strike);
    kernel_MCAE_k0[1].setArg(6, optionType);
    kernel_MCAE_k0[1].setArg(7, output_price_buf_b);
    kernel_MCAE_k0[1].setArg(8, output_mat_buf_b);
    kernel_MCAE_k0[1].setArg(9, calibSamples);
    kernel_MCAE_k0[1].setArg(10, timeSteps);

    kernel_MCAE_k1[0].setArg(0, timeLength);
    kernel_MCAE_k1[0].setArg(1, riskFreeRate);
    kernel_MCAE_k1[0].setArg(2, strike);
    kernel_MCAE_k1[0].setArg(3, optionType);
    kernel_MCAE_k1[0].setArg(4, output_price_buf);
    kernel_MCAE_k1[0].setArg(5, output_mat_buf);
    kernel_MCAE_k1[0].setArg(6, coef_buf);
    kernel_MCAE_k1[0].setArg(7, calibSamples);
    kernel_MCAE_k1[0].setArg(8, timeSteps);

    kernel_MCAE_k1[1].setArg(0, timeLength);
    kernel_MCAE_k1[1].setArg(1, riskFreeRate);
    kernel_MCAE_k1[1].setArg(2, strike);
    kernel_MCAE_k1[1].setArg(3, optionType);
    kernel_MCAE_k1[1].setArg(4, output_price_buf_b);
    kernel_MCAE_k1[1].setArg(5, output_mat_buf_b);
    kernel_MCAE_k1[1].setArg(6, coef_buf_b);
    kernel_MCAE_k1[1].setArg(7, calibSamples);
    kernel_MCAE_k1[1].setArg(8, timeSteps);

    kernel_MCAE_k2[0].setArg(0, underlying);
    kernel_MCAE_k2[0].setArg(1, volatility);
    kernel_MCAE_k2[0].setArg(2, dividendYield);
    kernel_MCAE_k2[0].setArg(3, riskFreeRate);
    kernel_MCAE_k2[0].setArg(4, timeLength);
    kernel_MCAE_k2[0].setArg(5, strike);
    kernel_MCAE_k2[0].setArg(6, optionType);
    kernel_MCAE_k2[0].setArg(7, coef_buf);
    kernel_MCAE_k2[0].setArg(8, output_buf);
    kernel_MCAE_k2[0].setArg(9, requiredTolerance);
    kernel_MCAE_k2[0].setArg(10, requiredSamples);
    kernel_MCAE_k2[0].setArg(11, timeSteps);

    kernel_MCAE_k2[1].setArg(0, underlying);
    kernel_MCAE_k2[1].setArg(1, volatility);
    kernel_MCAE_k2[1].setArg(2, dividendYield);
    kernel_MCAE_k2[1].setArg(3, riskFreeRate);
    kernel_MCAE_k2[1].setArg(4, timeLength);
    kernel_MCAE_k2[1].setArg(5, strike);
    kernel_MCAE_k2[1].setArg(6, optionType);
    kernel_MCAE_k2[1].setArg(7, coef_buf_b);
    kernel_MCAE_k2[1].setArg(8, output_buf_b);
    kernel_MCAE_k2[1].setArg(9, requiredTolerance);
    kernel_MCAE_k2[1].setArg(10, requiredSamples);
    kernel_MCAE_k2[1].setArg(11, timeSteps);

#if KN2 == 2
    kernel_MCAE_k3[0].setArg(0, underlying);
    kernel_MCAE_k3[0].setArg(1, volatility);
    kernel_MCAE_k3[0].setArg(2, dividendYield);
    kernel_MCAE_k3[0].setArg(3, riskFreeRate);
    kernel_MCAE_k3[0].setArg(4, timeLength);
    kernel_MCAE_k3[0].setArg(5, strike);
    kernel_MCAE_k3[0].setArg(6, optionType);
    kernel_MCAE_k3[0].setArg(7, coef_buf);
    kernel_MCAE_k3[0].setArg(8, output_buf2);
    kernel_MCAE_k3[0].setArg(9, requiredTolerance);
    kernel_MCAE_k3[0].setArg(10, requiredSamples);
    kernel_MCAE_k3[0].setArg(11, timeSteps);

    kernel_MCAE_k3[1].setArg(0, underlying);
    kernel_MCAE_k3[1].setArg(1, volatility);
    kernel_MCAE_k3[1].setArg(2, dividendYield);
    kernel_MCAE_k3[1].setArg(3, riskFreeRate);
    kernel_MCAE_k3[1].setArg(4, timeLength);
    kernel_MCAE_k3[1].setArg(5, strike);
    kernel_MCAE_k3[1].setArg(6, optionType);
    kernel_MCAE_k3[1].setArg(7, coef_buf_b);
    kernel_MCAE_k3[1].setArg(8, output_buf2_b);
    kernel_MCAE_k3[1].setArg(9, requiredTolerance);
    kernel_MCAE_k3[1].setArg(10, requiredSamples);
    kernel_MCAE_k3[1].setArg(11, timeSteps);
#endif

    // number of call for kernel
    std::vector<std::vector<cl::Event> > evt0(loop_nm);
    std::vector<std::vector<cl::Event> > evt1(loop_nm);
    std::vector<std::vector<cl::Event> > evt2(loop_nm);
    std::vector<std::vector<cl::Event> > evt3(loop_nm);
    for (int i = 0; i < loop_nm; i++) {
        evt0[i].resize(1);
        evt1[i].resize(1);
        evt2[i].resize(KN2);
        evt3[i].resize(1);
    }

    std::cout << "kernel start------" << std::endl;

    q.finish();
    gettimeofday(&start_time, 0);
    for (int i = 0; i < loop_nm; ++i) {
        // launch kernel and calculate kernel execution time
        int use_a = i & 1;
        if (use_a) {
            if (i < 2) {
                q.enqueueTask(kernel_MCAE_k0[0], nullptr, &evt0[i][0]);
            } else {
                q.enqueueTask(kernel_MCAE_k0[0], &evt3[i - 2], &evt0[i][0]);
            }
            q.enqueueTask(kernel_MCAE_k1[0], &evt0[i], &evt1[i][0]);
            q.enqueueTask(kernel_MCAE_k2[0], &evt1[i], &evt2[i][0]);
#if KN2 == 2
            q.enqueueTask(kernel_MCAE_k3[0], &evt1[i], &evt2[i][1]);
#endif
            q.enqueueMigrateMemObjects(ob_out, 1, &evt2[i], &evt3[i][0]);
        } else {
            if (i < 2) {
                q.enqueueTask(kernel_MCAE_k0[1], nullptr, &evt0[i][0]);
            } else {
                q.enqueueTask(kernel_MCAE_k0[1], &evt3[i - 2], &evt0[i][0]);
            }
            q.enqueueTask(kernel_MCAE_k1[1], &evt0[i], &evt1[i][0]);
            q.enqueueTask(kernel_MCAE_k2[1], &evt1[i], &evt2[i][0]);
#if KN2 == 2
            q.enqueueTask(kernel_MCAE_k3[1], &evt1[i], &evt2[i][1]);
#endif
            q.enqueueMigrateMemObjects(ob_out_b, 1, &evt2[i], &evt3[i][0]);
        }
    }
    q.flush();
    q.finish();
    gettimeofday(&end_time, 0);
    std::cout << "kernel end------" << std::endl;

    TEST_DT out = output[0];
    std::cout << "output0 = " << out << std::endl;
    TEST_DT out2 = (KN2 == 2) ? output2[0] : 0;
    std::cout << "output1 = " << out2 << std::endl;

    TEST_DT out_b = output_b[0];
    std::cout << "output0_b = " << out_b << std::endl;
    TEST_DT out2_b = (KN2 == 2) ? output2_b[0] : 0;
    std::cout << "output1_b = " << out2_b << std::endl;

    TEST_DT out_price;
    if (loop_nm == 1) {
        out_price = (out_b + out2_b) / KN2;
        std::cout << "out_price = " << out_price << std::endl;
    }
    if (loop_nm > 1) {
        out_price = (out + out2 + out_b + out2_b) / 2 / KN2;
        std::cout << "out_price = " << out_price << std::endl;
    }

    int exec_time = tvdiff(&start_time, &end_time);
    double time_elapsed = double(exec_time) / 1000 / 1000;
    std::cout << "FPGA execution time: " << time_elapsed << " s\n"
              << "options number: " << loop_nm << " \n"
              << "opt/sec: " << double(loop_nm) / time_elapsed << std::endl;

    double diff = std::fabs(out_price - golden_output);
    if (diff > requiredTolerance) {
        std::cout << "Output is wrong!" << std::endl;
        return -1;
    }
#endif

    return 0;
}
