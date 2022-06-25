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
#include <ap_int.h>
#include <fstream>
#include <hls_math.h>
#include <iomanip>
#include <iostream>
#include "xcl2.hpp"
#include "utils.hpp"
#include "kernel_resize.hpp"
#include "xf_utils_sw/logger.hpp"

using namespace std;

int main(int argc, const char* argv[]) {
    int nerror = 0;

    xf::common::utils_sw::Logger logger(std::cout, std::cerr);
    cl_int fail;

    ArgParser parser(argc, argv);
    std::string xclbin_path;
    if (!parser.getCmdOption("-xclbin", xclbin_path)) {
        std::cout << "ERROR:xclbin path is not set!\n";
        return 1;
    }

    std::string infile;
    if (!parser.getCmdOption("-i", infile)) {
        std::cout << "ERROR: input file path is not set!\n";
        return 1;
    }

    std::string outfile(infile);
    std::size_t found = outfile.find_last_of(".");
    outfile.insert(found, "_resized");

    ap_uint<32> src_width, src_height;
    std::string input_width;
    if (!parser.getCmdOption("-srcw", input_width)) {
        std::cout << "INFO: image input width is not set!" << std::endl;
    } else {
        src_width = std::stoi(input_width);
    }

    std::string input_height;
    if (!parser.getCmdOption("-srch", input_height)) {
        std::cout << "INFO: image input height is not set!" << std::endl;
    } else {
        src_height = std::stoi(input_height);
    }

    ap_uint<32> dst_width, dst_height;
    std::string output_width;
    if (!parser.getCmdOption("-dstw", output_width)) {
        std::cout << "INFO: image output width is not set!" << std::endl;
    } else {
        dst_width = std::stoi(output_width);
    }

    std::string output_height;
    if (!parser.getCmdOption("-dsth", output_height)) {
        std::cout << "INFO: image output height is not set!" << std::endl;
    } else {
        dst_height = std::stoi(output_height);
    }

    if (src_width < dst_width || src_height < dst_height) {
        std::cout << "WARNING: The output size is invaild!\n";
        return 1;
    }

    ap_uint<WDATA> pixel_64;
    ap_uint<WBIT> srcPixel;
    ap_uint<WBIT> tmpDst;

    ap_uint<32>* configs = aligned_alloc<ap_uint<32> >(4 + 1);
    configs[0] = src_width;
    configs[1] = src_height;
    configs[2] = dst_width;
    configs[3] = dst_height;

    ap_uint<WDATA>* axi_src = aligned_alloc<ap_uint<WDATA> >(MAX_SRC);
    ap_uint<WDATA>* axi_dst = aligned_alloc<ap_uint<WDATA> >(MAX_DST);

    // read raw data
    FILE* fp;

    if ((fp = fopen(infile.c_str(), "rb")) == NULL) {
        cout << "Error reading file fail." << '\n' << "Please check the path: " << infile << endl;
        exit(1);
    } else {
        cout << endl << "Read image successfully." << endl;
    }

#if NPPC == 1
    for (int i = 0; i < src_width * src_height; i++) {
        fread(&srcPixel, 1, 1, fp);
        axi_src[i] = srcPixel;
    }
#else
    for (int i = 0; i < src_width * src_height; i++) {
        fread(&srcPixel, 1, 1, fp); // std::cout<< "pixel:" << srcPixel << std::endl;
        pixel_64.range((i % 8) * WBIT + WBIT - 1, (i % 8) * WBIT) = srcPixel.range(WBIT - 1, 0);
        if ((i + 1) % 8 == 0) axi_src[i / 8] = pixel_64;
    }
#endif
    if (fp != NULL) fclose(fp);

    // do pre-process on CPU
    struct timeval start_time, end_time;
    // platform related operations
    std::vector<cl::Device> devices = xcl::get_xil_devices();
    cl::Device device = devices[0];

    // Creating Context and Command Queue for selected Device
    cl::Context context(device, NULL, NULL, NULL, &fail);
    logger.logCreateContext(fail);
    cl::CommandQueue q(context, device, CL_QUEUE_PROFILING_ENABLE | CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE, &fail);
    logger.logCreateCommandQueue(fail);
    std::string devName = device.getInfo<CL_DEVICE_NAME>();
    printf("Found Device=%s\n", devName.c_str());

    cl::Program::Binaries xclBins = xcl::import_binary_file(xclbin_path);
    devices.resize(1);
    devices[0] = device;
    cl::Program program(context, devices, xclBins, NULL, &fail);
    logger.logCreateProgram(fail);
    cl::Kernel resize;
    resize = cl::Kernel(program, "kernel_resize", &fail);
    logger.logCreateKernel(fail);
    std::cout << "kernel has been created" << std::endl;

    std::vector<cl_mem_ext_ptr_t> mext_o(3);
    mext_o[0] = {(unsigned int)(0) | XCL_MEM_TOPOLOGY, configs, 0};
    mext_o[1] = {(unsigned int)(1) | XCL_MEM_TOPOLOGY, axi_src, 0};
    mext_o[2] = {(unsigned int)(2) | XCL_MEM_TOPOLOGY, axi_dst, 0};

    // create device buffer and map dev buf to host buf
    cl::Buffer configs_buf, axi_src_buf, axi_dst_buf;

    configs_buf = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                             sizeof(ap_uint<32>) * 5, &mext_o[0]);
    axi_src_buf = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY,
                             sizeof(ap_int<WDATA>) * MAX_SRC, &mext_o[1]);
    axi_dst_buf = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                             sizeof(ap_int<WDATA>) * MAX_DST, &mext_o[2]);

    std::vector<cl::Memory> init;
    init.push_back(configs_buf);
    init.push_back(axi_src_buf);
    init.push_back(axi_dst_buf);

    q.enqueueMigrateMemObjects(init, CL_MIGRATE_MEM_OBJECT_CONTENT_UNDEFINED, nullptr, nullptr);
    q.finish();

    std::vector<cl::Memory> ob_in;
    std::vector<cl::Memory> ob_out;
    std::vector<cl::Event> events_write(1);
    std::vector<cl::Event> events_kernel(1);
    std::vector<cl::Event> events_read(1);

    ob_in.push_back(configs_buf);
    ob_in.push_back(axi_src_buf);
    q.enqueueMigrateMemObjects(ob_in, 0, nullptr, &events_write[0]);

    ob_out.push_back(axi_dst_buf);
    // launch kernel and calculate kernel execution time
    std::cout << "INFO: kernel start------" << std::endl;
    gettimeofday(&start_time, 0);
    int j = 0;
    resize.setArg(j++, configs_buf);
    resize.setArg(j++, axi_src_buf);
    resize.setArg(j++, axi_dst_buf);

    q.enqueueTask(resize, &events_write, &events_kernel[0]);
    q.enqueueMigrateMemObjects(ob_out, 1, &events_kernel, &events_read[0]);
    q.finish();
    gettimeofday(&end_time, 0);

    std::cout << "INFO: kernel end------" << std::endl;
    std::cout << "INFO: Execution time " << tvdiff(&start_time, &end_time) / 1000.0 << "ms" << std::endl;

    cl_ulong ts, te;
    events_write[0].getProfilingInfo(CL_PROFILING_COMMAND_START, &ts);
    events_write[0].getProfilingInfo(CL_PROFILING_COMMAND_END, &te);
    float elapsed = ((float)te - (float)ts) / 1000000.0;
    logger.info(xf::common::utils_sw::Logger::Message::TIME_H2D_MS, elapsed);

    events_kernel[0].getProfilingInfo(CL_PROFILING_COMMAND_START, &ts);
    events_kernel[0].getProfilingInfo(CL_PROFILING_COMMAND_END, &te);
    elapsed = ((float)te - (float)ts) / 1000000.0;
    logger.info(xf::common::utils_sw::Logger::Message::TIME_KERNEL_MS, elapsed);

    events_read[0].getProfilingInfo(CL_PROFILING_COMMAND_START, &ts);
    events_read[0].getProfilingInfo(CL_PROFILING_COMMAND_END, &te);
    elapsed = ((float)te - (float)ts) / 1000000.0;
    logger.info(xf::common::utils_sw::Logger::Message::TIME_D2H_MS, elapsed);

    FILE* outFile;
    if ((outFile = fopen(outfile.c_str(), "wb")) == NULL) {
        cout << "Error writing file fail." << '\n' << "Please check the path: " << outfile << endl;
        exit(1);
    }
#if NPPC == 1
    for (int j = 0; j < dst_width * dst_height; j++) {
        tmpDst = axi_dst[j];
        fwrite(&tmpDst, 1, 1, outFile);
    }
#else
    for (int i = 0; i < DivCeil(dst_width * dst_height, WBIT); i++) {
        pixel_64 = axi_dst[i];
        for (int j = 0; j < WBIT; j++) {
            tmpDst.range(WBIT - 1, 0) = pixel_64.range(j * WBIT + WBIT - 1, j * WBIT);
            if ((i * 8 + j) < (dst_width * dst_height)) {
                fwrite(&tmpDst, 1, 1, outFile);
                // std::cout << (int)tmpDst << std::endl;
            }
        }
    }
#endif
    if (outFile != NULL) fclose(outFile);

    if (nerror) {
        std::cout << "\nFAIL: nerror= " << nerror << " errors found.\n";
        logger.error(xf::common::utils_sw::Logger::Message::TEST_FAIL);
    } else {
        std::cout << "The src image size is " << src_width << "*" << src_height << ".\nThe dst image size is "
                  << dst_width << "*" << dst_height << ".\n"
                  << "Image resized successfully." << std::endl;
        std::cout << "PASS: no error found.\n";
        logger.info(xf::common::utils_sw::Logger::Message::TEST_PASS);
    }

    free(configs);
    free(axi_src);
    free(axi_dst);
    return nerror;
}
