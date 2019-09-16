/***************************************************************************
Copyright (c) 2018, Xilinx, Inc.
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors
may be used to endorse or promote products derived from this software
without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

***************************************************************************/
#include "common/xf_headers.h"
#include "xcl2.hpp"

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cout << "Usage: " << argv[0] << " <XCLBIN File> <INPUT IMAGE PATH 1>" << std::endl;
        return EXIT_FAILURE;
    }

    cv::Mat in_gray, in_gray1, out_gray, diff;

    // Reading in the image:
    in_gray = cv::imread(argv[2], 0);

    if (in_gray.data == NULL) {
        std::cout << "ERROR: Cannot open image " << argv[2] << std::endl;
        return EXIT_FAILURE;
    }

    int channels = in_gray.channels();

    // OpenCV function
    std::vector<double> ocv_scl(channels);

    for (int i = 0; i < channels; ++i) ocv_scl[i] = cv::sum(in_gray)[i];

    // OpenCL section:
    std::vector<double> hls_sum(channels);

    size_t image_in_size_bytes = in_gray.rows * in_gray.cols * sizeof(unsigned char);
    size_t vec_out_size_bytes = (channels) * sizeof(double);

    cl_int err;
    std::cout << "INFO: Running OpenCL section." << std::endl;

    // Get the device:
    std::vector<cl::Device> devices = xcl::get_xil_devices();
    cl::Device device = devices[0];

    // Context, command queue and device name:
    OCL_CHECK(err, cl::Context context(device, NULL, NULL, NULL, &err));
    OCL_CHECK(err, cl::CommandQueue queue(context, device, CL_QUEUE_PROFILING_ENABLE, &err));
    OCL_CHECK(err, std::string device_name = device.getInfo<CL_DEVICE_NAME>(&err));

    std::cout << "INFO: Device found - " << device_name << std::endl;

    // Load binary:
    unsigned fileBufSize;
    std::string binaryFile = argv[1];
    char* fileBuf = xcl::read_binary_file(binaryFile, fileBufSize);
    cl::Program::Binaries bins{{fileBuf, fileBufSize}};
    devices.resize(1);
    OCL_CHECK(err, cl::Program program(context, devices, bins, NULL, &err));

    // Create a kernel:
    OCL_CHECK(err, cl::Kernel kernel(program, "sum_accel", &err));

    // Allocate the buffers:
    OCL_CHECK(err, cl::Buffer buffer_inImage(context, CL_MEM_READ_ONLY, image_in_size_bytes, NULL, &err));
    OCL_CHECK(err, cl::Buffer buffer_outVec(context, CL_MEM_WRITE_ONLY, vec_out_size_bytes, NULL, &err));

    // Set kernel arguments:
    OCL_CHECK(err, err = kernel.setArg(0, buffer_inImage));
    OCL_CHECK(err, err = kernel.setArg(1, buffer_outVec));

    // Initialize the buffers:
    cl::Event event;

    OCL_CHECK(err, queue.enqueueWriteBuffer(buffer_inImage,      // buffer on the FPGA
                                            CL_TRUE,             // blocking call
                                            0,                   // buffer offset in bytes
                                            image_in_size_bytes, // Size in bytes
                                            in_gray.data,        // Pointer to the data to copy
                                            nullptr, &event));

    // Execute the kernel:
    OCL_CHECK(err, err = queue.enqueueTask(kernel));

    // Copy Result from Device Global Memory to Host Local Memory
    queue.enqueueReadBuffer(buffer_outVec, // This buffers data will be read
                            CL_TRUE,       // blocking call
                            0,             // offset
                            vec_out_size_bytes,
                            hls_sum.data(), // Data will be stored here
                            nullptr, &event);

    // Clean up:
    queue.finish();

    // Results verification:
    int cnt = 0;

    for (int i = 0; i < in_gray.channels(); i++) {
        if (abs(double(ocv_scl[i]) - hls_sum[i]) > 0.01f) cnt++;
    }

    if (cnt > 0) {
        std::cout << "INFO: Error percentage = " << 100.0 * float(cnt) / float(channels) << "%. Test Failed."
                  << std::endl;
        return EXIT_FAILURE;
    } else
        std::cout << "Test Passed." << std::endl;

    return 0;
}
