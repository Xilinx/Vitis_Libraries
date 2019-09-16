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

    cv::Mat in_img, dst_hls, ocv_ref, in_gray, diff, in_mask;

    // Reading in the image:
    in_img = cv::imread(argv[2], 0);

    if (in_img.data == NULL) {
        std::cout << "ERROR: Cannot open image " << argv[2] << std::endl;
        return EXIT_FAILURE;
    }

#if DIM
    dst_hls.create(in_img.rows, 1, CV_8UC1);
    ocv_ref.create(in_img.rows, 1, CV_8UC1);
#else
    dst_hls.create(1, in_img.cols, CV_8UC1);
    ocv_ref.create(1, in_img.cols, CV_8UC1);
#endif

    unsigned char dimension = DIM;

    // OpenCL section:
    size_t image_in_size_bytes = in_img.rows * in_img.cols * sizeof(unsigned char);
    size_t image_out_size_bytes = dst_hls.rows * dst_hls.cols * sizeof(unsigned char);

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
    OCL_CHECK(err, cl::Kernel kernel(program, "reduce_accel", &err));

    // Allocate the buffers:
    OCL_CHECK(err, cl::Buffer buffer_inImage(context, CL_MEM_READ_ONLY, image_in_size_bytes, NULL, &err));
    OCL_CHECK(err, cl::Buffer buffer_outImage(context, CL_MEM_WRITE_ONLY, image_out_size_bytes, NULL, &err));

    // Set kernel arguments:
    OCL_CHECK(err, err = kernel.setArg(0, buffer_inImage));
    OCL_CHECK(err, err = kernel.setArg(1, dimension));
    OCL_CHECK(err, err = kernel.setArg(2, buffer_outImage));

    // Initialize the buffers:
    cl::Event event;

    OCL_CHECK(err, queue.enqueueWriteBuffer(buffer_inImage,      // buffer on the FPGA
                                            CL_TRUE,             // blocking call
                                            0,                   // buffer offset in bytes
                                            image_in_size_bytes, // Size in bytes
                                            in_img.data,         // Pointer to the data to copy
                                            nullptr, &event));

    // Execute the kernel:
    OCL_CHECK(err, err = queue.enqueueTask(kernel));

    // Copy Result from Device Global Memory to Host Local Memory
    queue.enqueueReadBuffer(buffer_outImage, // This buffers data will be read
                            CL_TRUE,         // blocking call
                            0,               // offset
                            image_out_size_bytes,
                            dst_hls.data, // Data will be stored here
                            nullptr, &event);

    // Clean up:
    queue.finish();

    // Reference function
    if ((CV_REDUCE == CV_REDUCE_AVG) || (CV_REDUCE == CV_REDUCE_SUM))
        cv::reduce(in_img, ocv_ref, DIM, CV_REDUCE, CV_32SC1); // avg, sum
    else
        cv::reduce(in_img, ocv_ref, DIM, CV_REDUCE, CV_8UC1);

    // Results verification:
    FILE* fp = fopen("hls", "w");
    FILE* fp1 = fopen("cv", "w");
    int err_cnt = 0;

#if DIM == 1
    for (unsigned int i = 0; i < dst_hls.rows; i++) {
        fprintf(fp, "%d\n", (unsigned char)dst_hls.data[i]);
        fprintf(fp1, "%d\n", ocv_ref.data[i]);
        unsigned int diff = ocv_ref.data[i] - (unsigned char)dst_hls.data[i];
        if (diff > 1) err_cnt++;
    }

    std::cout << "INFO: Percentage of pixels with an error = " << (float)err_cnt * 100 / (float)dst_hls.rows << "%"
              << std::endl;

#endif
#if DIM == 0
    for (int i = 0; i < dst_hls.cols; i++) {
        fprintf(fp, "%d\n", (unsigned char)dst_hls.data[i]);
        fprintf(fp1, "%d\n", ocv_ref.data[i]);
        unsigned int diff = ocv_ref.data[i] - (unsigned char)dst_hls.data[i];
        if (diff > 1) err_cnt++;
    }

    std::cout << "INFO: Percentage of pixels with an error = " << (float)err_cnt * 100 / (float)dst_hls.cols << "%"
              << std::endl;

#endif
    fclose(fp);
    fclose(fp1);

    if (err_cnt > 0) {
        std::cout << "ERROR: Test Failed." << std::endl;
        return EXIT_FAILURE;
    }

    return 0;
}
