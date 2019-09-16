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
#include "xf_demosaicing_ref.hpp"
#include "xcl2.hpp"

void bayerizeImage(cv::Mat img, cv::Mat& bayer_image, cv::Mat& cfa_output, int code) {
    // FILE *fp = fopen("output.txt","w");
    for (int i = 0; i < img.rows; i++) {
        for (int j = 0; j < img.cols; j++) {
            cv::Vec3b in = img.at<cv::Vec3b>(i, j);
            cv::Vec3b b;
            b[0] = 0;
            b[1] = 0;
            b[2] = 0;

            if (code == 0) {            // BG
                if ((i & 1) == 0) {     // even row
                    if ((j & 1) == 0) { // even col
                        b[0] = in[0];
                        cfa_output.at<unsigned char>(i, j) = in[0];
                    } else { // odd col
                        b[1] = in[1];
                        cfa_output.at<unsigned char>(i, j) = in[1];
                    }
                } else {                // odd row
                    if ((j & 1) == 0) { // even col
                        b[1] = in[1];
                        cfa_output.at<unsigned char>(i, j) = in[1];
                    } else { // odd col
                        b[2] = in[2];
                        cfa_output.at<unsigned char>(i, j) = in[2];
                    }
                }
            }
            if (code == 1) {            // GB
                if ((i & 1) == 0) {     // even row
                    if ((j & 1) == 0) { // even col
                        b[1] = in[1];
                        cfa_output.at<unsigned char>(i, j) = in[1];
                    } else { // odd col
                        b[0] = in[0];
                        cfa_output.at<unsigned char>(i, j) = in[0];
                    }
                } else {                // odd row
                    if ((j & 1) == 0) { // even col
                        b[2] = in[2];
                        cfa_output.at<unsigned char>(i, j) = in[2];
                    } else { // odd col
                        b[1] = in[1];
                        cfa_output.at<unsigned char>(i, j) = in[1];
                    }
                }
            }
            if (code == 2) {            // GR
                if ((i & 1) == 0) {     // even row
                    if ((j & 1) == 0) { // even col
                        b[1] = in[1];
                        cfa_output.at<unsigned char>(i, j) = in[1];
                    } else { // odd col
                        b[2] = in[2];
                        cfa_output.at<unsigned char>(i, j) = in[2];
                    }
                } else {                // odd row
                    if ((j & 1) == 0) { // even col
                        b[0] = in[0];
                        cfa_output.at<unsigned char>(i, j) = in[0];
                    } else { // odd col
                        b[1] = in[1];
                        cfa_output.at<unsigned char>(i, j) = in[1];
                    }
                }
            }
            if (code == 3) {            // RG
                if ((i & 1) == 0) {     // even row
                    if ((j & 1) == 0) { // even col
                        b[2] = in[2];
                        cfa_output.at<unsigned char>(i, j) = in[2];
                    } else { // odd col
                        b[1] = in[1];
                        cfa_output.at<unsigned char>(i, j) = in[1];
                    }
                } else {                // odd row
                    if ((j & 1) == 0) { // even col
                        b[1] = in[1];
                        cfa_output.at<unsigned char>(i, j) = in[1];
                    } else { // odd col
                        b[0] = in[0];
                        cfa_output.at<unsigned char>(i, j) = in[0];
                    }
                }
            }
            bayer_image.at<cv::Vec3b>(i, j) = b;
        }
    }
}

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cout << "Usage: " << argv[0] << " <XCLBIN File> <INPUT IMAGE PATH 1>" << std::endl;
        return EXIT_FAILURE;
    }

    // Reading in input image:
    cv::Mat img = cv::imread(argv[2], 1);

    if (img.empty()) {
        std::cout << "ERROR: Cannot open image " << argv[2] << std::endl;
        return EXIT_FAILURE;
    }

    // Create the Bayer pattern CFA output
    cv::Mat cfa_bayer_output(img.rows, img.cols, CV_8UC1);             // simulate the Bayer pattern CFA output
    cv::Mat cfa_bayer_output_converted(img.rows, img.cols, CV_INTYPE); // simulate the Bayer pattern CFA output
    cv::Mat color_cfa_bayer_output(img.rows, img.cols, img.type());    // Bayer pattern CFA output in color
    int code = BAYER_PATTERN;                                          // Bayer format BG-0; GB-1; GR-2; RG-3

    bayerizeImage(img, color_cfa_bayer_output, cfa_bayer_output, code);
    cv::imwrite("bayer_image.png", color_cfa_bayer_output);
    cv::imwrite("cfa_output.png", cfa_bayer_output);

    // Convert cfa_bayer_output to chosen type:
    cfa_bayer_output.convertTo(cfa_bayer_output_converted, CV_INTYPE);

    // Demosaic the CFA output using reference code
    cv::Mat ref_output_image(img.rows, img.cols, CV_OUTTYPE);
    demosaicImage(cfa_bayer_output_converted, ref_output_image, code);
    cv::imwrite("reference_output_image.png", ref_output_image);

    // Allocate memory for kernel output:
    cv::Mat output_image_hls(img.rows, img.cols, CV_OUTTYPE);

// OpenCL section:
#if T_16U
    size_t image_in_size_bytes = img.rows * img.cols * img.channels() * sizeof(short);
    size_t image_out_size_bytes =
        ref_output_image.rows * ref_output_image.cols * ref_output_image.channels() * sizeof(short);
#else
    size_t image_in_size_bytes = img.rows * img.cols * img.channels() * sizeof(unsigned char);
    size_t image_out_size_bytes =
        ref_output_image.rows * ref_output_image.cols * ref_output_image.channels() * sizeof(unsigned char);
#endif

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
    OCL_CHECK(err, cl::Kernel kernel(program, "demosaicing", &err));

    // Allocate the buffers:
    OCL_CHECK(err, cl::Buffer buffer_inImage(context, CL_MEM_READ_ONLY, image_in_size_bytes, NULL, &err));
    OCL_CHECK(err, cl::Buffer buffer_outImage(context, CL_MEM_WRITE_ONLY, image_out_size_bytes, NULL, &err));

    // Set kernel arguments:
    OCL_CHECK(err, err = kernel.setArg(0, buffer_inImage));
    OCL_CHECK(err, err = kernel.setArg(1, buffer_outImage));

    // Initialize the buffers:
    cl::Event event;

    OCL_CHECK(err, queue.enqueueWriteBuffer(buffer_inImage,                  // buffer on the FPGA
                                            CL_TRUE,                         // blocking call
                                            0,                               // buffer offset in bytes
                                            image_in_size_bytes,             // Size in bytes
                                            cfa_bayer_output_converted.data, // Pointer to the data to copy
                                            nullptr, &event));

    // Execute the kernel:
    OCL_CHECK(err, err = queue.enqueueTask(kernel));

    // Copy Result from Device Global Memory to Host Local Memory
    queue.enqueueReadBuffer(buffer_outImage, // This buffers data will be read
                            CL_TRUE,         // blocking call
                            0,               // offset
                            image_out_size_bytes,
                            output_image_hls.data, // Data will be stored here
                            nullptr, &event);

    // Clean up:
    queue.finish();

    // Results verification:
    for (int i = 0; i < img.rows; i++) {
        for (int j = 0; j < (img.cols); j++) {
#if T_16U
            cv::Vec3w out = output_image_hls.at<cv::Vec3w>(i, j);
            cv::Vec3w ref_out = ref_output_image.at<cv::Vec3w>(i, j);
#else
            cv::Vec3b out = output_image_hls.at<cv::Vec3b>(i, j);
            cv::Vec3b ref_out = ref_output_image.at<cv::Vec3b>(i, j);
#endif

            int err_b = ((int)out[0] - (int)ref_out[0]);
            int err_g = ((int)out[1] - (int)ref_out[1]);
            int err_r = ((int)out[2] - (int)ref_out[2]);
            err_r = abs(err_r);
            err_g = abs(err_g);
            err_b = abs(err_b);

            if ((err_b > ERROR_THRESHOLD) || (err_g > ERROR_THRESHOLD) || (err_r > ERROR_THRESHOLD)) {
                std::cout << "ERROR: Results verification failed:" << std::endl;
                std::cout << "\tRef: " << (int)ref_out[0] << "\t" << (int)ref_out[1] << "\t" << (int)ref_out[2]
                          << std::endl;
                std::cout << "\tHLS: " << (int)out[0] << "\t" << (int)out[1] << "\t" << (int)out[2] << std::endl;
                std::cout << "\tError location: row = " << i << "\tcol = " << j << std::endl;
                return EXIT_FAILURE;
            }
        }
    }

    cv::imwrite("output_image_hls.jpg", output_image_hls);

    return 0;
}
