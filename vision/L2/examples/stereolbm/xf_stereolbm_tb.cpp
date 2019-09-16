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

using namespace std;

int main(int argc, char** argv) {
    cv::setUseOptimized(false);

    if (argc != 4) {
        std::cout << "Usage: " << argv[0] << " <XCLBIN File> <INPUT IMAGE PATH 1> <INPUT IMAGE PATH 2>" << std::endl;
        return EXIT_FAILURE;
    }

    cv::Mat left_img, right_img;

    // Reading in the images:
    left_img = cv::imread(argv[2], 0);
    right_img = cv::imread(argv[3], 0);

    if (left_img.data == NULL) {
        std::cout << "ERROR: Cannot open image " << argv[2] << std::endl;
        return EXIT_FAILURE;
    }

    if (right_img.data == NULL) {
        std::cout << "ERROR: Cannot open image " << argv[3] << std::endl;
        return EXIT_FAILURE;
    }

    cv::Mat disp, hls_disp;

    // OpenCV reference function:
    /*cv::StereoBM bm;
    bm.state->preFilterCap = 31;
    bm.state->preFilterType = CV_STEREO_BM_XSOBEL;
    bm.state->SADWindowSize = SAD_WINDOW_SIZE;
    bm.state->minDisparity = 0;
    bm.state->numberOfDisparities = NO_OF_DISPARITIES;
    bm.state->textureThreshold = 20;
    bm.state->uniquenessRatio = 15;
    bm(left_img, right_img, disp);*/

    cv::Ptr<cv::StereoBM> stereobm = cv::StereoBM::create(NO_OF_DISPARITIES, SAD_WINDOW_SIZE);
    stereobm->setPreFilterCap(31);
    stereobm->setUniquenessRatio(15);
    stereobm->setTextureThreshold(20);
    stereobm->compute(left_img, right_img, disp);

    hls_disp.create(disp.rows, disp.cols, disp.depth());

    cv::Mat disp8, hls_disp8;
    disp.convertTo(disp8, CV_8U, (256.0 / NO_OF_DISPARITIES) / (16.));
    cv::imwrite("ocv_output.png", disp8);

    hls_disp8.create(disp8.rows, disp8.cols, disp8.depth());

    // OpenCL section:
    std::vector<unsigned char> bm_state_params(4);
    bm_state_params[0] = 31;
    bm_state_params[1] = 15;
    bm_state_params[2] = 20;
    bm_state_params[3] = 0;

    size_t image_in_size_bytes = left_img.rows * left_img.cols * sizeof(unsigned char);
    size_t vec_in_size_bytes = bm_state_params.size() * sizeof(unsigned char);
    size_t image_out_size_bytes = hls_disp.rows * hls_disp.cols * sizeof(unsigned short int);

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
    OCL_CHECK(err, cl::Kernel kernel(program, "stereolbm_accel", &err));

    // Allocate the buffers:
    OCL_CHECK(err, cl::Buffer buffer_inImageL(context, CL_MEM_READ_ONLY, image_in_size_bytes, NULL, &err));
    OCL_CHECK(err, cl::Buffer buffer_inImageR(context, CL_MEM_READ_ONLY, image_in_size_bytes, NULL, &err));
    OCL_CHECK(err, cl::Buffer buffer_inVecBM(context, CL_MEM_READ_ONLY, vec_in_size_bytes, NULL, &err));
    OCL_CHECK(err, cl::Buffer buffer_outImage(context, CL_MEM_WRITE_ONLY, image_out_size_bytes, NULL, &err));

    // Set kernel arguments:
    OCL_CHECK(err, err = kernel.setArg(0, buffer_inImageL));
    OCL_CHECK(err, err = kernel.setArg(1, buffer_inImageR));
    OCL_CHECK(err, err = kernel.setArg(2, buffer_inVecBM));
    OCL_CHECK(err, err = kernel.setArg(3, buffer_outImage));

    // Initialize the buffers:
    cl::Event event;

    OCL_CHECK(err, queue.enqueueWriteBuffer(buffer_inImageL,     // buffer on the FPGA
                                            CL_TRUE,             // blocking call
                                            0,                   // buffer offset in bytes
                                            image_in_size_bytes, // Size in bytes
                                            left_img.data,       // Pointer to the data to copy
                                            nullptr, &event));

    OCL_CHECK(err, queue.enqueueWriteBuffer(buffer_inImageR,     // buffer on the FPGA
                                            CL_TRUE,             // blocking call
                                            0,                   // buffer offset in bytes
                                            image_in_size_bytes, // Size in bytes
                                            right_img.data,      // Pointer to the data to copy
                                            nullptr, &event));

    OCL_CHECK(err, queue.enqueueWriteBuffer(buffer_inVecBM,         // buffer on the FPGA
                                            CL_TRUE,                // blocking call
                                            0,                      // buffer offset in bytes
                                            vec_in_size_bytes,      // Size in bytes
                                            bm_state_params.data(), // Pointer to the data to copy
                                            nullptr, &event));

    // Execute the kernel:
    OCL_CHECK(err, err = queue.enqueueTask(kernel));

    // Copy Result from Device Global Memory to Host Local Memory
    queue.enqueueReadBuffer(buffer_outImage, // This buffers data will be read
                            CL_TRUE,         // blocking call
                            0,               // offset
                            image_out_size_bytes,
                            hls_disp.data, // Data will be stored here
                            nullptr, &event);

    // Clean up:
    queue.finish();

    // Convert 16U output to 8U output:
    hls_disp.convertTo(hls_disp8, CV_8U, (256.0 / NO_OF_DISPARITIES) / (16.));
    cv::imwrite("hls_out.jpg", hls_disp8);

    int cnt = 0, total = 0;

    // Changing the invalid value from negative to zero for validating the difference:
    for (int i = 0; i < disp.rows; i++) {
        for (int j = 0; j < disp.cols; j++) {
            if (disp.at<short>(i, j) < 0) {
                disp.at<short>(i, j) = 0;
            }
        }
    }

    // Error computation, removing off the border, different kind of border computations:
    for (int i = SAD_WINDOW_SIZE; i < hls_disp8.rows - SAD_WINDOW_SIZE; i++) {
        for (int j = SAD_WINDOW_SIZE; j < hls_disp8.cols - SAD_WINDOW_SIZE; j++) {
            total++;
            int diff = (disp.at<unsigned short>(i, j)) - (hls_disp.at<unsigned short>(i, j));
            if (diff < 0) diff = -diff;
            if (diff > 1) {
                cnt++;
            }
        }
    }

    float percentage = ((float)cnt / (float)total) * 100.0;
    std::cout << "INFO: Error Percentage = " << percentage << "%" << std::endl;

    if (percentage > 0.0f) {
        std::cout << "ERROR: Test Failed" << std::endl;
        return EXIT_FAILURE;
    } else {
        std::cout << "INFO: Test Pass" << std::endl;
    }

    return 0;
}
