/*
 * Copyright (C) 2019-2022, Xilinx, Inc.
 * Copyright (C) 2022-2026, Advanced Micro Devices, Inc.
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

#include "ap_int.h"
#include "hls_stream.h"
#include "xf_dense_npyr_optical_flow_tb_config.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "xcl2.hpp"

#include <opencv2/core/core.hpp>
#include <opencv2/opencv.hpp>
//#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui/highgui.hpp>
//#include <iostream>
#include <string>
#include <cmath>

// Per-component flow difference threshold (pixels). Tight tolerance for LK golden vs accelerator.
#define FLOW_DIFF_THRESH 0.5f

static void getPseudoColorInt(pix_t pix, float fx, float fy, rgba_t& rgba) {
    // normalization factor is key for good visualization. Make this auto-ranging
    // or controllable from the host TODO
    // const int normFac = 127/2;
    const int normFac = 10;

    int y = 127 + (int)(fy * normFac);
    int x = 127 + (int)(fx * normFac);
    if (y > 255) y = 255;
    if (y < 0) y = 0;
    if (x > 255) x = 255;
    if (x < 0) x = 0;

    rgb_t rgb;
    if (x > 127) {
        if (y < 128) {
            // 1 quad
            rgb.r = x - 127 + (127 - y) / 2;
            rgb.g = (127 - y) / 2;
            rgb.b = 0;
        } else {
            // 4 quad
            rgb.r = x - 127;
            rgb.g = 0;
            rgb.b = y - 127;
        }
    } else {
        if (y < 128) {
            // 2 quad
            rgb.r = (127 - y) / 2;
            rgb.g = 127 - x + (127 - y) / 2;
            rgb.b = 0;
        } else {
            // 3 quad
            rgb.r = 0;
            rgb.g = 128 - x;
            rgb.b = y - 127;
        }
    }

    rgba.r = pix / 4 + 3 * rgb.r / 4;
    rgba.g = pix / 4 + 3 * rgb.g / 4;
    rgba.b = pix / 4 + 3 * rgb.b / 4;
    rgba.a = 255;
    // rgba.r = rgb.r;
    // rgba.g = rgb.g;
    // rgba.b = rgb.b ;
}

static void getOutPix(float* fx, float* fy, pix_t* p, hls::stream<rgba_t>& out_pix, int rows, int cols, int size) {
    for (int r = 0; r < rows; r++) {
// clang-format off
        #pragma HLS LOOP_TRIPCOUNT min=1 max=ROWS
        // clang-format on
        for (int c = 0; c < cols; c++) {
// clang-format off
            #pragma HLS LOOP_TRIPCOUNT min=1 max=COLS
            #pragma HLS PIPELINE
            // clang-format on
            float fx_ = *(fx + r * cols + c);
            float fy_ = *(fy + r * cols + c);

            pix_t p_ = *(p + r * cols + c);
            rgba_t out_pix_;
            getPseudoColorInt(p_, fx_, fy_, out_pix_);

            out_pix.write(out_pix_);
        }
    }
}

static void writeMatRowsRGBA(hls::stream<rgba_t>& pixStream, unsigned int* dst, int rows, int cols, int size) {
    for (int i = 0; i < size; i++) {
// clang-format off
        #pragma HLS LOOP_TRIPCOUNT min=1 max=ROWS*COLS/NPPCX
        #pragma HLS PIPELINE
        // clang-format on
        rgba_t tmpData = pixStream.read();
        *(dst + i) = (unsigned int)tmpData.a << 24 | (unsigned int)tmpData.b << 16 | (unsigned int)tmpData.g << 8 |
                     (unsigned int)tmpData.r;
    }
}

// Pixel accessor with zero border (matches uninitialized line-buffer edges in HLS).
static inline int ref_pix(const cv::Mat& img, int r, int c) {
    if (r < 0 || r >= img.rows || c < 0 || c >= img.cols) {
        return 0;
    }
    return (int)img.at<uchar>(r, c);
}

// Software reference: dense non-pyramid Lucas-Kanade, matching xf::cv::DenseNonPyrLKOpticalFlow.
// frame_curr / frame_prev align with dense_non_pyr_of_accel img_curr / img_prev ports (im0 / im1).
static void dense_npyr_of_ref(const cv::Mat& frame_curr,
                              const cv::Mat& frame_prev,
                              cv::Mat& flowx_ref,
                              cv::Mat& flowy_ref) {
    const int rows = frame_curr.rows;
    const int cols = frame_curr.cols;
    const int W = KMED;

    flowx_ref.create(rows, cols, CV_32FC1);
    flowy_ref.create(rows, cols, CV_32FC1);
    flowx_ref.setTo(0);
    flowy_ref.setTo(0);

    for (int r = 0; r < rows; r++) {
        for (int c = 0; c < cols; c++) {
            float fx = 0.0f;
            float fy = 0.0f;

            if (r >= W && c >= (W + 1)) {
                long long ixix = 0;
                long long ixiy = 0;
                long long iyiy = 0;
                long long dix = 0;
                long long diy = 0;

                // Window spans [r-W, r] x [c-W-1, c], consistent with HLS border checks.
                for (int y = r - W; y <= r; y++) {
                    for (int x = c - W - 1; x <= c; x++) {
                        const int ix = (ref_pix(frame_curr, y, x + 1) - ref_pix(frame_curr, y, x - 1)) / 2;
                        const int iy = (ref_pix(frame_curr, y + 1, x) - ref_pix(frame_curr, y - 1, x)) / 2;
                        const int it = ref_pix(frame_curr, y, x) - ref_pix(frame_prev, y, x);
                        ixix += (long long)ix * ix;
                        ixiy += (long long)ix * iy;
                        iyiy += (long long)iy * iy;
                        dix += (long long)ix * it;
                        diy += (long long)iy * it;
                    }
                }

                const float det = (float)ixix * (float)iyiy - (float)ixiy * (float)ixiy;
                if (det > 1.0f) {
                    const float i00 = (float)iyiy / det;
                    const float i01 = (float)(-ixiy) / det;
                    const float i10 = (float)(-ixiy) / det;
                    const float i11 = (float)ixix / det;
                    fx = i00 * (float)dix + i01 * (float)diy;
                    fy = i10 * (float)dix + i11 * (float)diy;
                }
            }

            flowx_ref.at<float>(r, c) = fx;
            flowy_ref.at<float>(r, c) = fy;
        }
    }
}

static float analyze_flow_diff(const cv::Mat& flowx_hls,
                               const cv::Mat& flowy_hls,
                               const cv::Mat& flowx_ref,
                               const cv::Mat& flowy_ref,
                               int margin,
                               float err_thresh) {
    int rows = flowx_hls.rows;
    int cols = flowx_hls.cols;
    int cnt = 0;
    int valid_pixels = 0;
    double minval = 1e9;
    double maxval = 0.0;

    for (int r = margin; r < rows - margin; r++) {
        for (int c = margin; c < cols - margin; c++) {
            float dx = std::abs(flowx_hls.at<float>(r, c) - flowx_ref.at<float>(r, c));
            float dy = std::abs(flowy_hls.at<float>(r, c) - flowy_ref.at<float>(r, c));
            float v = std::max(dx, dy);
            valid_pixels++;
            if (v > err_thresh) {
                cnt++;
            }
            if (minval > v) {
                minval = v;
            }
            if (maxval < v) {
                maxval = v;
            }
        }
    }

    float err_per = (valid_pixels > 0) ? (100.0f * (float)cnt / (float)valid_pixels) : 0.0f;
    std::cout << "\tMinimum error in flow = " << minval << std::endl;
    std::cout << "\tMaximum error in flow = " << maxval << std::endl;
    std::cout << "\tPercentage of pixels above error threshold = " << err_per << std::endl;
    return err_per;
}

static void write_flow_output_image(const cv::Mat& flowx_in,
                                    const cv::Mat& flowy_in,
                                    const cv::Mat& frame1,
                                    cv::Mat& frame_out,
                                    const char* out_filename) {
    int height = flowx_in.rows;
    int width = flowx_in.cols;

    frame_out.create(height, width, CV_8UC4);

    float* flowx_copy = (float*)malloc(MAX_HEIGHT * MAX_WIDTH * sizeof(float));
    float* flowy_copy = (float*)malloc(MAX_HEIGHT * MAX_WIDTH * sizeof(float));
    unsigned int* outputBuffer = (unsigned int*)malloc(MAX_HEIGHT * MAX_WIDTH * sizeof(unsigned int));
    if (flowx_copy == NULL || flowy_copy == NULL || outputBuffer == NULL) {
        fprintf(stderr, "Failed to allocate memory for flow visualization output\n");
        free(flowx_copy);
        free(flowy_copy);
        free(outputBuffer);
        return;
    }

    for (int f = 0; f < height; f++) {
        for (int i = 0; i < width; i++) {
            flowx_copy[f * width + i] = flowx_in.at<float>(f, i);
            flowy_copy[f * width + i] = flowy_in.at<float>(f, i);
        }
    }

    hls::stream<rgba_t> out_pix("Color pixel");
    getOutPix(flowx_copy, flowy_copy, frame1.data, out_pix, height, width, width * height);
    writeMatRowsRGBA(out_pix, outputBuffer, height, width, width * height);

    unsigned char p1, p2, p3, p4;
    unsigned int pix = 0;
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            rgba_t* outbuf_copy = (rgba_t*)(outputBuffer + i * width + j);
            p1 = outbuf_copy->r;
            p2 = outbuf_copy->g;
            p3 = outbuf_copy->b;
            p4 = outbuf_copy->a;
            pix = ((unsigned int)p4 << 24) | ((unsigned int)p3 << 16) | ((unsigned int)p2 << 8) | (unsigned int)p1;
            frame_out.at<unsigned int>(i, j) = pix;
        }
    }

    cv::imwrite(out_filename, frame_out);

    free(flowx_copy);
    free(flowy_copy);
    free(outputBuffer);
}

int main(int argc, char** argv) {
    cv::Mat frame0, frame1;
    cv::Mat flowx, flowy;
    cv::Mat flowx_ref, flowy_ref;
    cv::Mat frame_out;
    cv::Mat frame_out_ref;

    if (argc != 3) {
        fprintf(stderr, "Usage incorrect. Correct usage: ./exe <frame0/im0> <frame1/im1>\n");
        return -1;
    }

    frame0 = cv::imread(argv[1], 0);
    frame1 = cv::imread(argv[2], 0);

    if (frame0.empty() || frame1.empty()) {
        fprintf(stderr, "input files not found!\n");
        return -1;
    }

    flowx.create(frame0.rows, frame0.cols, CV_32FC1);
    flowy.create(frame0.rows, frame0.cols, CV_32FC1);
    flowx_ref.create(frame0.rows, frame0.cols, CV_32FC1);
    flowy_ref.create(frame0.rows, frame0.cols, CV_32FC1);

    int cnt = 0;
    char out_string[200];

    /////////////////////////////////////// CL ////////////////////////

    int height = frame0.rows;
    int width = frame0.cols;
    std::cout << "Input image height : " << height << std::endl;
    std::cout << "Input image width  : " << width << std::endl;

    // im0 -> img_curr, im1 -> img_prev (same as dense_non_pyr_of_accel port order).
    std::cout << "INFO: Running software reference (dense non-pyramid LK)." << std::endl;
    dense_npyr_of_ref(frame0, frame1, flowx_ref, flowy_ref);

    std::cout << "INFO: Running OpenCL accelerator." << std::endl;
    std::cout << "Starting xrt programmingms" << std::endl;

    std::vector<cl::Device> devices = xcl::get_xil_devices();
    cl::Device device = devices[0];
    cl::Context context(device);

    std::cout << "device context created" << std::endl;
    std::cout << "Input Image Bit Depth:" << XF_DTPIXELDEPTH(IN_TYPE, NPPCX) << std::endl;
    std::cout << "Input Image Channels:" << XF_CHANNELS(IN_TYPE, NPPCX) << std::endl;
    std::cout << "NPPCX:" << NPPCX << std::endl;

    cl::CommandQueue q(context, device, CL_QUEUE_PROFILING_ENABLE);

    std::cout << "command queue created" << std::endl;

    // Create Program and Kernel
    std::string device_name = device.getInfo<CL_DEVICE_NAME>();
    std::string binaryFile = xcl::find_binary_file(device_name, "krnl_lknpyrof");
    cl::Program::Binaries bins = xcl::import_binary_file(binaryFile);
    devices.resize(1);
    cl::Program program(context, devices, bins);
    cl::Kernel krnl(program, "dense_non_pyr_of_accel");

    std::cout << "kernel loaded" << std::endl;

    // Allocate Buffer in Global Memory
    cl::Buffer currImageToDevice(context, CL_MEM_READ_ONLY, (height * width));
    cl::Buffer prevImageToDevice(context, CL_MEM_READ_ONLY, (height * width));

    std::cout << "input buffer created" << std::endl;

    cl::Buffer outxImageFromDevice(context, CL_MEM_USE_HOST_PTR | CL_MEM_WRITE_ONLY,
                                   (height * width * OUT_BYTES_PER_CHANNEL), flowx.data);
    cl::Buffer outyImageFromDevice(context, CL_MEM_USE_HOST_PTR | CL_MEM_WRITE_ONLY,
                                   (height * width * OUT_BYTES_PER_CHANNEL), flowy.data);

    std::vector<cl::Memory> outBufVec0, outBufVec1;
    outBufVec0.push_back(outxImageFromDevice);
    outBufVec1.push_back(outyImageFromDevice);

    std::cout << "output buffer created" << std::endl;

    krnl.setArg(0, currImageToDevice);
    krnl.setArg(1, prevImageToDevice);
    krnl.setArg(2, outxImageFromDevice);
    krnl.setArg(3, outyImageFromDevice);
    krnl.setArg(4, height);
    krnl.setArg(5, width);

    std::cout << "arguments copied" << std::endl;

    // Copying input data to Device buffer from host memory
    q.enqueueWriteBuffer(currImageToDevice, CL_TRUE, 0, (height * width), frame0.data);
    q.enqueueWriteBuffer(prevImageToDevice, CL_TRUE, 0, (height * width), frame1.data);

    std::cout << "input buffer copied" << std::endl;

    // Profiling Objects
    cl_ulong start = 0;
    cl_ulong end = 0;
    double diff_prof = 0.0f;
    cl::Event event_sp;

    // Launch the kernel
    q.enqueueTask(krnl, NULL, &event_sp);
    clWaitForEvents(1, (const cl_event*)&event_sp);

    // Profiling
    event_sp.getProfilingInfo(CL_PROFILING_COMMAND_START, &start);
    event_sp.getProfilingInfo(CL_PROFILING_COMMAND_END, &end);
    diff_prof = end - start;
    std::cout << (diff_prof / 1000000) << "ms" << std::endl;

    std::cout << "ip returned" << std::endl;
    // Copying Device result data to Host memory
    q.enqueueMigrateMemObjects(outBufVec0, CL_MIGRATE_MEM_OBJECT_HOST);
    std::cout << "output x  buffer read" << std::endl;
    q.enqueueMigrateMemObjects(outBufVec1, CL_MIGRATE_MEM_OBJECT_HOST);
    std::cout << "output y  buffer read" << std::endl;

    std::cout << "done" << std::endl;
    q.finish();

    /////////////////////////////////////// end of CL ////////////////////////

    ////////  FUNCTIONAL VALIDATION  ////////
    float err_per = analyze_flow_diff(flowx, flowy, flowx_ref, flowy_ref, KMED, FLOW_DIFF_THRESH);
    if (err_per > 5.0f) {
        fprintf(stderr, "ERROR: Test Failed.\n");
        return 1;
    }
    std::cout << "Test Passed" << std::endl;

    sprintf(out_string, "out_ref_%d.png", cnt);
    write_flow_output_image(flowx_ref, flowy_ref, frame1, frame_out_ref, out_string);

    sprintf(out_string, "out_%d.png", cnt);
    write_flow_output_image(flowx, flowy, frame1, frame_out, out_string);

    return 0;
}
