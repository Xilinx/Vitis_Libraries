/*
 * Copyright (C) 2019-2022, Xilinx, Inc.
 * Copyright (C) 2022-2025, Advanced Micro Devices, Inc.
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

#include "common/xf_headers.hpp"
#include "xcl2.hpp"
#include "xf_findcontours_tb_config.h"
#include <time.h>

int main(int argc, char** argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <INPUT IMAGE PATH 1> <THRESHOLD>\n", argv[0]);
        return EXIT_FAILURE;
    }

    cv::Mat in_gray, out_img, out_img_ocv, out_hls;

    // Reading in the image:
    in_gray = cv::imread(argv[1], 0);

    if (!in_gray.data) {
        fprintf(stderr, "ERROR: Cannot open image %s\n ", argv[1]);
        return EXIT_FAILURE;
    }

    std::vector<cv::KeyPoint> keypoints;

    // Opencv reference function
    // Threshold for fast:
    unsigned char threshold = 100;
    if (argc >= 3) threshold = atoi(argv[2]);
    cv::Mat refThreshold255, refThreshold1;
    cv::threshold(in_gray, refThreshold255, threshold, 255, cv::THRESH_BINARY);
    // cv::threshold(refThreshold255, refThreshold1, 0, 1, cv::THRESH_BINARY);

    // Output allocation from HLS implementation:
    ap_uint<32>* points = (ap_uint<32>*)malloc(MAX_TOTAL_POINTS * sizeof(int));
    ap_uint<32>* offsets = (ap_uint<32>*)malloc((MAX_CONTOURS + 1) * sizeof(int));
    ap_uint<32>* numc = (ap_uint<32>*)malloc(sizeof(int));

    // OpenCL section:
    size_t image_in_size_bytes = in_gray.rows * in_gray.cols * sizeof(unsigned char);
    size_t image_out_points_size_bytes = (MAX_TOTAL_POINTS * sizeof(int));
    size_t image_out_offsets_size_bytes = ((MAX_CONTOURS + 1) * sizeof(int));
    size_t image_num_contours_size_bytes = (sizeof(int));

    int rows = in_gray.rows;
    int cols = in_gray.cols;
    std::cout << "Input image height : " << rows << std::endl;
    std::cout << "Input image width  : " << cols << std::endl;
    cl_int err;
    std::cout << "INFO: Running OpenCL section." << std::endl;

    // Get the device:
    std::vector<cl::Device> devices = xcl::get_xil_devices();
    cl::Device device = devices[0];

    // Context, command queue and device name:
    cl::Context context(device);
    cl::CommandQueue queue(context, device, CL_QUEUE_PROFILING_ENABLE);
    std::string device_name = device.getInfo<CL_DEVICE_NAME>();

    std::cout << "INFO: Device found - " << device_name << std::endl;
    std::cout << "Input Image Bit Depth:" << XF_DTPIXELDEPTH(IN_TYPE, NPPCX) << std::endl;
    std::cout << "Input Image Channels:" << XF_CHANNELS(IN_TYPE, NPPCX) << std::endl;
    std::cout << "NPPC:" << NPPCX << std::endl;

    // Load binary:
    unsigned fileBufSize;
    std::string binaryFile = xcl::find_binary_file(device_name, "krnl_findcontours");
    cl::Program::Binaries bins = xcl::import_binary_file(binaryFile);
    devices.resize(1);
    cl::Program program(context, devices, bins);

    // Create a kernel:
    cl::Kernel kernel(program, "findcontours_accel");

    // Allocate the buffers:
    cl::Buffer buffer_inImage(context, CL_MEM_READ_ONLY, image_in_size_bytes);
    cl::Buffer buffer_outPoints(context, CL_MEM_WRITE_ONLY, image_out_points_size_bytes);
    cl::Buffer buffer_outOffsets(context, CL_MEM_WRITE_ONLY, image_out_offsets_size_bytes);
    cl::Buffer buffer_numContours(context, CL_MEM_WRITE_ONLY, image_num_contours_size_bytes);

    queue.enqueueWriteBuffer(buffer_inImage,      // buffer on the FPGA
                             CL_TRUE,             // blocking call
                             0,                   // buffer offset in bytes
                             image_in_size_bytes, // Size in bytes
                             refThreshold255.data // Pointer to the data to copy
                             );

    // Set kernel arguments:
    OCL_CHECK(err, err = kernel.setArg(0, buffer_inImage));
    OCL_CHECK(err, err = kernel.setArg(1, rows));
    OCL_CHECK(err, err = kernel.setArg(2, cols));
    OCL_CHECK(err, err = kernel.setArg(3, buffer_outPoints));
    OCL_CHECK(err, err = kernel.setArg(4, buffer_outOffsets));
    OCL_CHECK(err, err = kernel.setArg(5, buffer_numContours));

    // Initialize the buffers:

    cl_ulong start = 0;
    cl_ulong end = 0;
    double diff_prof = 0.0f;
    cl::Event event;

    // Execute the kernel:
    // OCL_CHECK(err, err = queue.enqueueTask(kernel, NULL, &event));
    queue.enqueueTask(kernel, NULL, &event);
    clWaitForEvents(1, (const cl_event*)&event);

    event.getProfilingInfo(CL_PROFILING_COMMAND_START, &start);
    event.getProfilingInfo(CL_PROFILING_COMMAND_END, &end);
    diff_prof = end - start;
    std::cout << (diff_prof / 1000000) << "ms" << std::endl;

    // Copy Result from Device Global Memory to Host Local Memory
    queue.enqueueReadBuffer(buffer_outPoints, // This buffers data will be read
                            CL_TRUE,          // blocking call
                            0,                // offset
                            image_out_points_size_bytes,
                            points // Data will be stored here
                            );

    queue.enqueueReadBuffer(buffer_outOffsets, // This buffers data will be read
                            CL_TRUE,           // blocking call
                            0,                 // offset
                            image_out_offsets_size_bytes,
                            offsets // Data will be stored here
                            );

    queue.enqueueReadBuffer(buffer_numContours, // This buffers data will be read
                            CL_TRUE,            // blocking call
                            0,                  // offset
                            image_num_contours_size_bytes,
                            numc // Data will be stored here
                            );

    // Clean up:
    queue.finish();

    std::vector<std::vector<cv::Point> > hls_contours;
    std::cout << "Contours: " << (unsigned)numc[0] << "\n";
    for (unsigned c = 0; c < (unsigned)numc[0]; ++c) {
        unsigned s = offsets[c];
        unsigned e = offsets[c + 1];
        std::cout << "Contour " << c << " size=" << (e - s) << "\n  ";
        std::vector<cv::Point> contour_pts;
        contour_pts.reserve(e - s);
        for (unsigned i = s; i < e; i++) {
            ap_uint<32> p = points[i];
            unsigned x = p & 0xFFFF;
            unsigned y = (p >> 16) & 0xFFFF;
            std::cout << "(" << x << "," << y << ") ";
            contour_pts.emplace_back((int)x, (int)y);
        }
        hls_contours.push_back(std::move(contour_pts));
        std::cout << "\n";
    }
    cv::Mat hlsContourOutput(in_gray.rows, in_gray.cols, CV_8UC1);
    hlsContourOutput.setTo(cv::Scalar(255));
    drawContours(hlsContourOutput, hls_contours, -1, cv::Scalar(0, 255, 0), 0); // Green contours
    imwrite("hls_contours.png", hlsContourOutput);

    // Write the kernel output:
    // cv::imwrite("hls_out.jpg", out_hls);
    std::vector<std::vector<cv::Point> > refcontours;

    // TIMER START CODE
    struct timespec begin_hw, end_hw;
    clock_gettime(CLOCK_REALTIME, &begin_hw);

    // OpenCV reference function
    // cv::FAST(in_gray, keypoints, threshold, NMS);
    cv::findContours(refThreshold255, refcontours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    // TIMER END CODE
    clock_gettime(CLOCK_REALTIME, &end_hw);
    long seconds, nanoseconds;
    double hw_time;
    seconds = end_hw.tv_sec - begin_hw.tv_sec;
    nanoseconds = end_hw.tv_nsec - begin_hw.tv_nsec;
    hw_time = seconds + nanoseconds * 1e-9;
    hw_time = hw_time * 1e3;

    std::cout << "Latency for CPU function is " << hw_time << "ms" << std::endl;

    cv::Mat refContourOutput(in_gray.rows, in_gray.cols, CV_8UC1);
    refContourOutput.setTo(cv::Scalar(255));
    imwrite("before_contour.png", refContourOutput);
    drawContours(refContourOutput, refcontours, -1, cv::Scalar(0, 255, 0), 0); // Green contours
    imwrite("Contours.png", refContourOutput);
    cv::Mat diff;
    cv::absdiff(hlsContourOutput, refContourOutput, diff);
    imwrite("diff.png", diff);

    return 0;
}
