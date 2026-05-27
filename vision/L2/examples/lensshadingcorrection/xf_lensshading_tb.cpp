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
#include "xcl2.hpp"
#include "xf_lensshading_tb_config.h"
#include <iostream>
#include <math.h>

using namespace std;

void bayerizeImage(cv::Mat in_img, cv::Mat& bayer_image, cv::Mat& cfa_output, int code) {
    for (int i = 0; i < in_img.rows; i++) {
        for (int j = 0; j < in_img.cols; j++) {
#if T_8U
            cv::Vec3b in = in_img.at<cv::Vec3b>(i, j);
            cv::Vec3b b;
#else
            cv::Vec3w in = in_img.at<cv::Vec3w>(i, j);
            cv::Vec3w b;
#endif
            b[0] = 0;
            b[1] = 0;
            b[2] = 0;

            if (code == 0) {            // BG
                if ((i & 1) == 0) {     // even row
                    if ((j & 1) == 0) { // even col
                        b[0] = in[0];
                        cfa_output.at<pxltype>(i, j) = in[0];
                    } else { // odd col
                        b[1] = in[1];
                        cfa_output.at<pxltype>(i, j) = in[1];
                    }
                } else {                // odd row
                    if ((j & 1) == 0) { // even col
                        b[1] = in[1];
                        cfa_output.at<pxltype>(i, j) = in[1];
                    } else { // odd col
                        b[2] = in[2];
                        cfa_output.at<pxltype>(i, j) = in[2];
                    }
                }
            }
            if (code == 1) {            // GB
                if ((i & 1) == 0) {     // even row
                    if ((j & 1) == 0) { // even col
                        b[1] = in[1];
                        cfa_output.at<pxltype>(i, j) = in[1];
                    } else { // odd col
                        b[0] = in[0];
                        cfa_output.at<pxltype>(i, j) = in[0];
                    }
                } else {                // odd row
                    if ((j & 1) == 0) { // even col
                        b[2] = in[2];
                        cfa_output.at<pxltype>(i, j) = in[2];
                    } else { // odd col
                        b[1] = in[1];
                        cfa_output.at<pxltype>(i, j) = in[1];
                    }
                }
            }
            if (code == 2) {            // GR
                if ((i & 1) == 0) {     // even row
                    if ((j & 1) == 0) { // even col
                        b[1] = in[1];
                        cfa_output.at<pxltype>(i, j) = in[1];
                    } else { // odd col
                        b[2] = in[2];
                        cfa_output.at<pxltype>(i, j) = in[2];
                    }
                } else {                // odd row
                    if ((j & 1) == 0) { // even col
                        b[0] = in[0];
                        cfa_output.at<pxltype>(i, j) = in[0];
                    } else { // odd col
                        b[1] = in[1];
                        cfa_output.at<pxltype>(i, j) = in[1];
                    }
                }
            }
            if (code == 3) {            // RG
                if ((i & 1) == 0) {     // even row
                    if ((j & 1) == 0) { // even col
                        b[2] = in[2];
                        cfa_output.at<pxltype>(i, j) = in[2];
                    } else { // odd col
                        b[1] = in[1];
                        cfa_output.at<pxltype>(i, j) = in[1];
                    }
                } else {                // odd row
                    if ((j & 1) == 0) { // even col
                        b[1] = in[1];
                        cfa_output.at<pxltype>(i, j) = in[1];
                    } else { // odd col
                        b[0] = in[0];
                        cfa_output.at<pxltype>(i, j) = in[0];
                    }
                }
            }
#if T_8U
            bayer_image.at<cv::Vec3b>(i, j) = b;
#else
            bayer_image.at<cv::Vec3w>(i, j) = b;
#endif
        }
    }
}

// OpenCV reference function:
void LSC_ref(cv::Mat& _src, cv::Mat& _dst) {
    int center_pixel_pos_x = (_src.cols / 2);
    int center_pixel_pos_y = (_src.rows / 2);
    float max_distance = std::sqrt((_src.rows - center_pixel_pos_y) * (_src.rows - center_pixel_pos_y) +
                                   (_src.cols - center_pixel_pos_x) * (_src.cols - center_pixel_pos_x));

    for (int i = 0; i < _src.rows; i++) {
        for (int j = 0; j < _src.cols; j++) {
            float distance = std::sqrt((center_pixel_pos_y - i) * (center_pixel_pos_y - i) +
                                       (center_pixel_pos_x - j) * (center_pixel_pos_x - j)) /
                             max_distance;

            float gain = (0.01759 * ((distance + 28.37) * (distance + 28.37))) - 13.36;
#if T_8U
            int value = (_src.at<unsigned char>(i, j) * gain);
            if (value > 255) {
                value = 255;
            }
            _dst.at<unsigned char>(i, j) = cv::saturate_cast<unsigned char>(value);
#endif
#if T_16U
            int value = (_src.at<unsigned short>(i, j) * gain);
            if (value > 65535) {
                value = 65535;
            }
            _dst.at<unsigned short>(i, j) = cv::saturate_cast<unsigned short>(value);

#endif
        }
    }
}

int main(int argc, char** argv) {
    cv::Mat in_img, out_img, out_img_hls, diff, cfa_bayer_output;
#if T_8U
    in_img = cv::imread(argv[1], 1);
#else
    in_img = cv::imread(argv[1], -1);
#endif
    if (!in_img.data) {
        return -1;
    }

    imwrite("in_img.png", in_img);
    std::cout << "Input image height : " << in_img.rows << std::endl;
    std::cout << "Input image width  : " << in_img.cols << std::endl;

#if T_8U
    out_img.create(in_img.rows, in_img.cols, CV_8UC1);
    out_img_hls.create(in_img.rows, in_img.cols, CV_8UC1);
    diff.create(in_img.rows, in_img.cols, CV_8UC1);
    cfa_bayer_output.create(in_img.rows, in_img.cols, CV_8UC1);
    size_t image_in_size_bytes = in_img.rows * in_img.cols * sizeof(unsigned char);
    size_t image_out_size_bytes = image_in_size_bytes;
#endif
#if T_16U
    out_img.create(in_img.rows, in_img.cols, CV_16UC1);
    out_img_hls.create(in_img.rows, in_img.cols, CV_16UC1);
    diff.create(in_img.rows, in_img.cols, CV_16UC1);
    cfa_bayer_output.create(in_img.rows, in_img.cols, CV_16UC1);
    size_t image_in_size_bytes = in_img.rows * in_img.cols * sizeof(unsigned short);
    size_t image_out_size_bytes = image_in_size_bytes;
#endif

    imwrite("out_img1.png", out_img);
    imwrite("out_img_hls1.png", out_img_hls);

    cv::Mat color_cfa_bayer_output(in_img.rows, in_img.cols, in_img.type()); // Bayer pattern CFA output in color
    unsigned short bformat = BPATTERN;                                       // Bayer format BG-0; GB-1; GR-2; RG-3

    bayerizeImage(in_img, color_cfa_bayer_output, cfa_bayer_output, bformat);
    cv::imwrite("bayer_image.png", color_cfa_bayer_output);
    cv::imwrite("cfa_output.png", cfa_bayer_output);

    LSC_ref(cfa_bayer_output, out_img);

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
    std::cout << "Input Image Bit Depth:" << XF_DTPIXELDEPTH(IN_TYPE, NPPCX) << std::endl;
    std::cout << "Input Image Channels:" << XF_CHANNELS(IN_TYPE, NPPCX) << std::endl;
    std::cout << "NPPC:" << NPPCX << std::endl;

    // Load binary:
    std::string binaryFile = xcl::find_binary_file(device_name, "krnl_lensshading");
    cl::Program::Binaries bins = xcl::import_binary_file(binaryFile);

    devices.resize(1);
    OCL_CHECK(err, cl::Program program(context, devices, bins, NULL, &err));

    // Create a kernel:
    OCL_CHECK(err, cl::Kernel kernel(program, "lensshading_accel", &err));

    // Allocate the buffers:
    OCL_CHECK(err, cl::Buffer buffer_inImage(context, CL_MEM_READ_ONLY, image_in_size_bytes, NULL, &err));
    OCL_CHECK(err, cl::Buffer buffer_outImage(context, CL_MEM_WRITE_ONLY, image_out_size_bytes, NULL, &err));

    // Set kernel arguments:
    OCL_CHECK(err, err = kernel.setArg(0, buffer_inImage));
    OCL_CHECK(err, err = kernel.setArg(1, buffer_outImage));
    OCL_CHECK(err, err = kernel.setArg(2, in_img.rows));
    OCL_CHECK(err, err = kernel.setArg(3, in_img.cols));

    // Initialize the buffers:
    cl::Event event;

    OCL_CHECK(err,
              queue.enqueueWriteBuffer(buffer_inImage,        // buffer on the FPGA
                                       CL_TRUE,               // blocking call
                                       0,                     // buffer offset in bytes
                                       image_in_size_bytes,   // Size in bytes
                                       cfa_bayer_output.data, // Pointer to the data to copy
                                       nullptr, &event));

    // Execute the kernel:
    OCL_CHECK(err, err = queue.enqueueTask(kernel));

    // Copy Result from Device Global Memory to Host Local Memory
    queue.enqueueReadBuffer(buffer_outImage, // This buffers data will be read
                            CL_TRUE,         // blocking call
                            0,               // offset
                            image_out_size_bytes,
                            out_img_hls.data, // Data will be stored here
                            nullptr, &event);

    // Clean up:
    queue.finish();

    // Write output image
    cv::imwrite("hls_out.png", out_img_hls);
    cv::imwrite("ocv_out.png", out_img);

    // Compute absolute difference image
    cv::absdiff(out_img_hls, out_img, diff);
    // Save the difference image for debugging purpose:
    cv::imwrite("error.png", diff);
    float err_per;
    xf::cv::analyzeDiff(diff, 1, err_per);

    if (err_per > 0.0f) {
        fprintf(stderr, " ERROR: Test Failed.\n ");
        return 1;
    }
    std::cout << " Test Passed " << std::endl;

    return 0;
}
