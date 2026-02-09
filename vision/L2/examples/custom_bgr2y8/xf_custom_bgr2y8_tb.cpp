/*
 * Copyright 2022 Xilinx, Inc.
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
#include <stdlib.h>
#include <ap_int.h>
#include <iostream>
#include <math.h>

#include "xf_custom_bgr2y8_tb_config.h"
#include "xcl2.hpp"
#include "xf_opencl_wrap.hpp"

int main(int argc, char** argv) {
    if (argc != 2) {
        printf("Usage: %s <INPUT IMAGE PATH > \n", argv[0]);
        return EXIT_FAILURE;
    }

    cv::Mat input_rgb_image, gray_image, gray_image_hls, diff;

    // Reading in the images:
    input_rgb_image = cv::imread(argv[1], 1);

    if (input_rgb_image.data == NULL) {
        printf("ERROR: Cannot open image %s\n ", argv[1]);
        return EXIT_FAILURE;
    }

    gray_image.create(input_rgb_image.rows, input_rgb_image.cols, CV_OUT_TYPE);
    diff.create(input_rgb_image.rows, input_rgb_image.cols, CV_OUT_TYPE);
    gray_image_hls.create(input_rgb_image.rows, input_rgb_image.cols, CV_OUT_TYPE);

    for (int i = 0; i < input_rgb_image.rows; i++) {
        for (int j = 0; j < input_rgb_image.cols; j++) {
            cv::Vec3b a = input_rgb_image.at<cv::Vec3b>(i, j);

            unsigned char b = a.val[0];
            unsigned char g = a.val[1];
            unsigned char r = a.val[2];

            unsigned char min, max;
            max = (((r > b) ? r : b) > g ? ((r > b) ? r : b) : g);
            min = (((r < b) ? r : b) < g ? ((r < b) ? r : b) : g);

            // Solution 4: RGB to HSV

            float norm_b = (float)b / 255;
            float norm_g = (float)g / 255;
            float norm_r = (float)r / 255;
            float norm_min, norm_max;
            norm_max =
                (((norm_r > norm_b) ? norm_r : norm_b) > norm_g ? ((norm_r > norm_b) ? norm_r : norm_b) : norm_g);
            norm_min =
                (((norm_r < norm_b) ? norm_r : norm_b) < norm_g ? ((norm_r < norm_b) ? norm_r : norm_b) : norm_g);

            int V_per = norm_max * 100;

            float sat, Hue;
            int sat_per;
            int H_degree;
            if (norm_max > 0)
                sat = (norm_max - norm_min) / norm_max;
            else
                sat = 0;

            sat_per = sat * 100;

            if (norm_max == norm_r)
                Hue = (norm_g - norm_b) / (norm_max - norm_min);
            else if (norm_max == norm_g)
                Hue = 2.0 + (norm_b - norm_r) / (norm_max - norm_min);
            else
                Hue = 4.0 + (norm_r - norm_g) / (norm_max - norm_min);

            H_degree = Hue * 60;

            if ((V_per < 20) && (sat_per < 70)) { // obvious defect and background
                gray_image.at<unsigned char>(i, j) = 0;
            } else {
                if ((H_degree < 15) && (V_per < 40) && (sat_per < 90) && (sat_per > 60)) // Brown color range
                    gray_image.at<unsigned char>(i, j) = 0;
                else if ((((H_degree > 30) && (H_degree < 45))) && (V_per < 35) && (sat_per < 90) &&
                         (sat_per > 60)) // Brown color range
                    gray_image.at<unsigned char>(i, j) = 0;
                else if ((H_degree > 50) && (H_degree < 90) && (V_per < 45)) // dimmer region enhancement
                    gray_image.at<unsigned char>(i, j) = (max + 30) < 256 ? (max + 30) : 255;
                else // other cases
                    gray_image.at<unsigned char>(i, j) = max;
            }
        }
    }

    struct bgr2y8_params params;

    ap_uint<8> array_params[12];
    array_params[0] = params.black_Vmax;
    array_params[1] = params.black_Smax;
    array_params[2] = params.brown_Hmax;
    array_params[3] = params.brown_Vmax;
    array_params[4] = params.Smin;
    array_params[5] = params.Smax;
    array_params[6] = params.darkgreen_Vmax;
    array_params[7] = params.darkgreen_Hmin;
    array_params[8] = params.darkgreen_Hmax;
    array_params[9] = params.green_Hmax;
    array_params[10] = params.green_Hmin;
    array_params[11] = params.green_Vmax;

    // CL section
    size_t image_in_size_bytes = input_rgb_image.rows * input_rgb_image.cols * 3 * sizeof(unsigned char);
    size_t image_out_size_bytes = input_rgb_image.rows * input_rgb_image.cols * 1 * sizeof(unsigned char);
    size_t array_size_bytes = 12 * sizeof(unsigned char);

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
    std::string binaryFile = xcl::find_binary_file(device_name, "krnl_custom_bgr2y8");
    cl::Program::Binaries bins = xcl::import_binary_file(binaryFile);

    devices.resize(1);
    OCL_CHECK(err, cl::Program program(context, devices, bins, NULL, &err));

    // Create a kernel:
    OCL_CHECK(err, cl::Kernel kernel(program, "custom_bgr2y8_accel", &err));

    // Allocate the buffers:
    OCL_CHECK(err, cl::Buffer buffer_inImage(context, CL_MEM_READ_ONLY, image_in_size_bytes, NULL, &err));
    OCL_CHECK(err, cl::Buffer buffer_outImage(context, CL_MEM_WRITE_ONLY, image_out_size_bytes, NULL, &err));
    OCL_CHECK(err, cl::Buffer buffer_array(context, CL_MEM_READ_ONLY, array_size_bytes, NULL, &err));

    std::cout << "kernel args" << std::endl;
    // Set kernel arguments:
    OCL_CHECK(err, err = kernel.setArg(0, buffer_inImage));
    OCL_CHECK(err, err = kernel.setArg(1, buffer_outImage));
    OCL_CHECK(err, err = kernel.setArg(2, buffer_array));
    OCL_CHECK(err, err = kernel.setArg(3, input_rgb_image.rows));
    OCL_CHECK(err, err = kernel.setArg(4, input_rgb_image.cols));

    // Initialize the buffers:
    cl_ulong start = 0;
    cl_ulong end = 0;
    double diff_prof = 0.0f;
    cl::Event event;

    OCL_CHECK(err, queue.enqueueWriteBuffer(buffer_inImage,       // buffer on the FPGA
                                            CL_TRUE,              // blocking call
                                            0,                    // buffer offset in bytes
                                            image_in_size_bytes,  // Size in bytes
                                            input_rgb_image.data, // Pointer to the data to copy
                                            nullptr, &event));

    OCL_CHECK(err, queue.enqueueWriteBuffer(buffer_array,     // buffer on the FPGA
                                            CL_TRUE,          // blocking call
                                            0,                // buffer offset in bytes
                                            array_size_bytes, // Size in bytes
                                            array_params,     // Pointer to the data to copy
                                            nullptr, &event));

    std::cout << "before enqueue task" << std::endl;

    // Execute the kernel:
    OCL_CHECK(err, err = queue.enqueueTask(kernel));

    std::cout << "after enqueue task" << std::endl;

    event.getProfilingInfo(CL_PROFILING_COMMAND_START, &start);
    event.getProfilingInfo(CL_PROFILING_COMMAND_END, &end);
    diff_prof = end - start;
    std::cout << "Kernel execution time: " << (diff_prof / 1000000) << "ms" << std::endl;

    // Copy Result from Device Global Memory to Host Local Memory
    queue.enqueueReadBuffer(buffer_outImage, // This buffers data will be read
                            CL_TRUE,         // blocking call
                            0,               // offset
                            image_out_size_bytes,
                            gray_image_hls.data, // Data will be stored here
                            nullptr, &event);

    // Clean up:
    queue.finish();

    std::cout << "after finish" << std::endl;

    cv::imwrite("gray_img_hls.png", gray_image_hls);
    cv::imwrite("gray_img.png", gray_image);

    // Compute absolute difference image
    cv::absdiff(gray_image, gray_image_hls, diff);

    // Save the difference image for debugging purpose:
    cv::imwrite("error.png", diff);
    float err_per;
    xf::cv::analyzeDiff(diff, 1, err_per);
    if (err_per > 1) {
        std::cerr << "ERROR: Test Failed." << std::endl;
        return 1;
    } else
        std::cout << "Test Passed " << std::endl;

    return 0;
}