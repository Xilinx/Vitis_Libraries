/***************************************************************************
 Copyright (c) 2020, Xilinx, Inc.
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

#include "common/xf_headers.hpp"
#include "xf_isp_types.h"

#include "xcl2.hpp"

int main(int argc, char** argv) {
    if (argc != 2) {
        fprintf(stderr, "Invalid Number of Arguments!\nUsage:\n");
        fprintf(stderr, "<Executable Name> <input image path> \n");
        return -1;
    }

    cv::Mat in_img, out_img, ocv_ref, in_gray, diff;

    unsigned short in_width, in_height;

/*  reading in the color image  */
#if T_8U
    in_img = cv::imread(argv[1], 0); // read image
#else
    in_img = cv::imread(argv[1], -1); // read image
#endif

    if (in_img.data == NULL) {
        fprintf(stderr, "Cannot open image at %s\n", argv[1]);
        return 0;
    }

#if T_8U
    out_img.create(in_img.rows, in_img.cols, CV_8UC3);
    size_t image_in_size_bytes = in_img.rows * in_img.cols * sizeof(unsigned char);
    size_t image_out_size_bytes = in_img.rows * in_img.cols * 3 * sizeof(unsigned char);
#else
    out_img.create(in_img.rows, in_img.cols, CV_8UC3);
    size_t image_in_size_bytes = in_img.rows * in_img.cols * sizeof(unsigned short);
    size_t image_out_size_bytes = in_img.rows * in_img.cols * 3 * sizeof(unsigned char);
#endif

    // Write input image
    imwrite("input.png", in_img);

    /////////////////////////////////////// CL ////////////////////////

    int height = in_img.rows;
    int width = in_img.cols;
    // int channels=out_img.channels();

    cl_int err;
    std::cout << "INFO: Running OpenCL section." << std::endl;

    std::vector<cl::Device> devices = xcl::get_xil_devices();
    cl::Device device = devices[0];
    OCL_CHECK(err, cl::Context context(device, NULL, NULL, NULL, &err));
    OCL_CHECK(err, cl::CommandQueue q(context, device, CL_QUEUE_PROFILING_ENABLE, &err));

    std::string device_name = device.getInfo<CL_DEVICE_NAME>();
    std::string binaryFile = xcl::find_binary_file(device_name, "krnl_ISPPipeline");
    cl::Program::Binaries bins = xcl::import_binary_file(binaryFile);
    devices.resize(1);

    OCL_CHECK(err, cl::Program program(context, devices, bins, NULL, &err));

    // Create a kernel:
    OCL_CHECK(err, cl::Kernel kernel(program, "ISPPipeline_accel", &err));

    std::vector<cl::Memory> inBufVec, outBufVec;
    OCL_CHECK(err, cl::Buffer imageToDevice(context, CL_MEM_READ_ONLY, image_in_size_bytes, NULL, &err));
    OCL_CHECK(err, cl::Buffer imageFromDevice(context, CL_MEM_WRITE_ONLY, image_out_size_bytes, NULL, &err));

    // Set the kernel arguments
    OCL_CHECK(err, err = kernel.setArg(0, imageToDevice));
    OCL_CHECK(err, err = kernel.setArg(1, imageFromDevice));
    OCL_CHECK(err, err = kernel.setArg(2, height));
    OCL_CHECK(err, err = kernel.setArg(3, width));

    for (int i = 0; i < 2; i++) {
        OCL_CHECK(err, q.enqueueWriteBuffer(imageToDevice, CL_TRUE, 0, image_in_size_bytes, in_img.data));

        // Profiling Objects
        cl_ulong start = 0;
        cl_ulong end = 0;
        double diff_prof = 0.0f;
        cl::Event event_sp;

        // Launch the kernel
        OCL_CHECK(err, err = q.enqueueTask(kernel, NULL, &event_sp));
        clWaitForEvents(1, (const cl_event*)&event_sp);

        event_sp.getProfilingInfo(CL_PROFILING_COMMAND_START, &start);
        event_sp.getProfilingInfo(CL_PROFILING_COMMAND_END, &end);
        diff_prof = end - start;
        std::cout << (diff_prof / 1000000) << "ms" << std::endl;

        // Copying Device result data to Host memory
        q.enqueueReadBuffer(imageFromDevice, CL_TRUE, 0, image_out_size_bytes, out_img.data);
    }

    q.finish();

    /////////////////////////////////////// end of CL ////////////////////////

    // Write output image
    imwrite("hls_out.png", out_img);

    return 0;
}
