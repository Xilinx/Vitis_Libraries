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

// Reference functions:
void xmean(cv::Mat& img, float* result) {
    unsigned long Sum = 0;
    int i, j;

    /* Sum of All Pixels */
    for (i = 0; i < img.rows; ++i) {
        for (j = 0; j < img.cols; ++j) {
            Sum += img.at<uchar>(i, j); // imag.data[i]}
        }
    }
    result[0] = (float)Sum / (float)(img.rows * img.cols);
}

void variance(cv::Mat& Img, float* mean, double* var) {
    double sum = 0.0;
    for (int i = 0; i < Img.rows; i++) {
        for (int j = 0; j < Img.cols; j++) {
            double x = (double)mean[0] - ((double)Img.at<uint8_t>(i, j));
            sum = sum + pow(x, (double)2.0);
        }
    }

    var[0] = (sum / (double)(Img.rows * Img.cols));
}

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cout << "Usage: " << argv[0] << " <INPUT IMAGE PATH 1>" << std::endl;
        return EXIT_FAILURE;
    }

    cv::Mat in_img, in_gray;

    // Reading in the image:
    in_img = cv::imread(argv[1], 0);

    if (in_img.data == NULL) {
        std::cout << "ERROR: Cannot open image " << argv[1] << std::endl;
        return EXIT_FAILURE;
    }

    int channels = in_img.channels();
    int height = in_img.rows;
    int width = in_img.cols;

    // Allocate memory for the output values:
    std::vector<unsigned short> mean(channels);
    std::vector<unsigned short> stddev(channels);

    // OpenCL section:
    size_t image_in_size_bytes = height * width;
    size_t vec_out_size_bytes = channels * sizeof(unsigned short);

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
    std::string binaryFile = xcl::find_binary_file(device_name, "krnl_meanstddev");
    cl::Program::Binaries bins = xcl::import_binary_file(binaryFile);
    devices.resize(1);
    OCL_CHECK(err, cl::Program program(context, devices, bins, NULL, &err));

    // Create a kernel:
    OCL_CHECK(err, cl::Kernel kernel(program, "meanstddev_accel", &err));

    // Allocate the buffers:
    OCL_CHECK(err, cl::Buffer buffer_inImage(context, CL_MEM_READ_ONLY, image_in_size_bytes, NULL, &err));
    OCL_CHECK(err, cl::Buffer buffer_meanOut(context, CL_MEM_WRITE_ONLY, vec_out_size_bytes, NULL, &err));
    OCL_CHECK(err, cl::Buffer buffer_stddevOut(context, CL_MEM_WRITE_ONLY, vec_out_size_bytes, NULL, &err));

    // Set kernel arguments:
    OCL_CHECK(err, err = kernel.setArg(0, buffer_inImage));
    OCL_CHECK(err, err = kernel.setArg(1, buffer_meanOut));
    OCL_CHECK(err, err = kernel.setArg(2, buffer_stddevOut));
    OCL_CHECK(err, err = kernel.setArg(3, height));
    OCL_CHECK(err, err = kernel.setArg(4, width));

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
    queue.enqueueReadBuffer(buffer_meanOut, // This buffers data will be read
                            CL_TRUE,        // blocking call
                            0,              // offset
                            vec_out_size_bytes,
                            mean.data(), // Data will be stored here
                            nullptr, &event);

    queue.enqueueReadBuffer(buffer_stddevOut, // This buffers data will be read
                            CL_TRUE,          // blocking call
                            0,                // offset
                            vec_out_size_bytes,
                            stddev.data(), // Data will be stored here
                            nullptr, &event);

    // Clean up:
    queue.finish();

    // OpenCV reference:
    std::vector<float> mean_c(channels);
    std::vector<float> stddev_c(channels);
    std::vector<double> var_c(channels);

    float mean_hls[channels], stddev_hls[channels];
    float diff_mean[channels], diff_stddev[channels];

    // Two Pass Mean and Variance:
    xmean(in_img, mean_c.data());
    variance(in_img, mean_c.data(), var_c.data());

    // Results verification:
    int err_cnt = 0;

    std::cout << "INFO: Results obtained:" << std::endl;

    for (int c = 0; c < channels; c++) {
        stddev_c[c] = sqrt(var_c[c]);
        mean_hls[c] = (float)mean[c] / 256;
        stddev_hls[c] = (float)stddev[c] / 256;
        diff_mean[c] = mean_c[c] - mean_hls[c];
        diff_stddev[c] = stddev_c[c] - stddev_hls[c];

        std::cout << "\tRef.     Mean = " << mean_c[c] << "\tResult = " << mean_hls[c] << "\tDiff = " << diff_mean[c]
                  << std::endl;
        std::cout << "\tRef. Std.Dev. = " << stddev_c[c] << "\tResult = " << stddev_hls[c]
                  << "\tDiff = " << diff_stddev[c] << std::endl;

        if ((diff_mean[c] > 0.1f) || (diff_stddev[c] > 0.1f)) err_cnt++;
    }

    if (err_cnt > 0) {
        std::cout << "ERROR: Test Failed." << std::endl;
        return EXIT_FAILURE;
    }

    return 0;
}
