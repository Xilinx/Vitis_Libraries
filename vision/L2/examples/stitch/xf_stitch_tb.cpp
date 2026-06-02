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

#include "common/xf_headers.hpp"
#include "xcl2.hpp"
#include "xf_stitch_tb_config.h"

#include <opencv2/stitching/detail/blenders.hpp>
#include <vector>

// Per-stream top-left on the stitched output canvas (same as AVFIRS_4stream TB).
static void computeStitchMaskCornersFromMasks(const cv::Mat mask_img[4], cv::Point out_corners[4]) {
    int max_cols = mask_img[0].cols;
    int max_rows = mask_img[0].rows;
    for (int i = 1; i < 4; ++i) {
        if (mask_img[i].cols > max_cols) max_cols = mask_img[i].cols;
        if (mask_img[i].rows > max_rows) max_rows = mask_img[i].rows;
    }

    out_corners[0].x = 0;
    out_corners[1].x = max_cols - mask_img[1].cols;
    out_corners[2].x = 0;
    out_corners[3].x = 0;

    out_corners[0].y = mask_img[0].rows;
    out_corners[1].y = mask_img[0].rows;
    out_corners[2].y = max_rows;
    out_corners[3].y = mask_img[0].rows;
}

static const int STREAM_MAX_H[4] = {HEIGHT_1, HEIGHT_2, HEIGHT_3, HEIGHT_4};
static const int STREAM_MAX_W[4] = {WIDTH_1, WIDTH_2, WIDTH_3, WIDTH_4};

static bool check_stream_bounds(const cv::Mat& m, int stream_idx, const char* path) {
    if (m.rows > STREAM_MAX_H[stream_idx] || m.cols > STREAM_MAX_W[stream_idx]) {
        fprintf(stderr, "Image %s (%dx%d) exceeds stream %d max %dx%d\n", path, m.cols, m.rows, stream_idx + 1,
                STREAM_MAX_W[stream_idx], STREAM_MAX_H[stream_idx]);
        return false;
    }
    return true;
}

int main(int argc, char** argv) {
    if (argc != 6) {
        fprintf(stderr,
                "Usage: %s <stitch_remap0> <stitch_remap1> <stitch_remap2> <stitch_remap3> "
                "<mask0> <mask1> <mask2> <mask3>\n",
                argv[0]);
        return EXIT_FAILURE;
    }

    cv::Mat in_img1, in_img2, in_img3, in_img4;
    cv::Mat mask_img1, mask_img2, mask_img3, mask_img4;

    // Stitch remap inputs (same as L1/examples/stitch TB)
    in_img1 = cv::imread(argv[1], cv::IMREAD_COLOR);
    in_img2 = cv::imread(argv[2], cv::IMREAD_COLOR);
    in_img3 = cv::imread(argv[3], cv::IMREAD_COLOR);
    in_img4 = cv::imread(argv[4], cv::IMREAD_COLOR);

    std::string mask_folder = argv[5];
    if (mask_folder.back() != '/') mask_folder += '/';
    std::cout << "Mask inputs folder: " << mask_folder << std::endl;

    cv::Mat mask[4];
    for (int i = 0; i < 4; i++) {
        std::string mask_name = mask_folder + "mask" + std::to_string(i + 1) + "_000000.png";
        mask[i] = cv::imread(mask_name, cv::IMREAD_GRAYSCALE);
        std::cout << "Mask " << (i + 1) << ": " << mask_name << std::endl;
        std::cout << "  Image size: " << mask[i].cols << " x " << mask[i].rows << std::endl;
    }

    cv::Mat in[4] = {in_img1, in_img2, in_img3, in_img4};
    const char* in_paths[4] = {argv[1], argv[2], argv[3], argv[4]};
    const char* mask_paths[4] = {argv[5], argv[6], argv[7], argv[8]};

    for (int i = 0; i < 4; i++) {
        if (in[i].data == NULL) {
            fprintf(stderr, "Cannot open stitch remap image %s\n", in_paths[i]);
            return EXIT_FAILURE;
        }
        if (mask[i].data == NULL) {
            fprintf(stderr, "Cannot open mask image %s\n", mask_paths[i]);
            return EXIT_FAILURE;
        }
        if (!check_stream_bounds(in[i], i, in_paths[i]) || !check_stream_bounds(mask[i], i, mask_paths[i])) {
            return EXIT_FAILURE;
        }
        std::cout << "Stream " << i << " remap: " << in[i].cols << "x" << in[i].rows << " mask: " << mask[i].cols << "x"
                  << mask[i].rows << std::endl;
    }

    int img_sizes[8] = {in_img1.rows, in_img1.cols, in_img2.rows, in_img2.cols,
                        in_img3.rows, in_img3.cols, in_img4.rows, in_img4.cols};

    cv::Point mask_corners_pt[4];
    computeStitchMaskCornersFromMasks(mask, mask_corners_pt);

    int mask_corners[8];
    for (int i = 0; i < 4; i++) {
        mask_corners[i * 2] = mask_corners_pt[i].x;
        mask_corners[i * 2 + 1] = mask_corners_pt[i].y;
    }

    std::vector<cv::Point> blend_corners;
    std::vector<cv::Size> blend_sizes;
    for (int i = 0; i < 4; i++) {
        blend_corners.push_back(mask_corners_pt[i]);
        blend_sizes.push_back(cv::Size(in[i].cols, in[i].rows));
    }

    cv::Rect dst_roi = cv::detail::resultRoi(blend_corners, blend_sizes);
    int dst_w = dst_roi.width;
    int dst_h = dst_roi.height;

    std::cout << "Destination ROI: [" << dst_roi.x << ", " << dst_roi.y << ", " << dst_w << ", " << dst_h << "]"
              << std::endl;

    if (dst_h > HEIGHT_DST || dst_w > WIDTH_DST) {
        fprintf(stderr, "Stitched ROI %dx%d exceeds max canvas %dx%d\n", dst_h, dst_w, HEIGHT_DST, WIDTH_DST);
        return EXIT_FAILURE;
    }

    cv::Mat out_hls(dst_h, dst_w, CV_8UC3);

    size_t img_in_size_bytes[4];
    size_t mask_in_size_bytes[4];
    for (int i = 0; i < 4; i++) {
        img_in_size_bytes[i] = (size_t)in[i].rows * in[i].cols * in[i].elemSize();
        mask_in_size_bytes[i] = (size_t)mask[i].rows * mask[i].cols * mask[i].elemSize();
    }
    size_t image_out_size_bytes = (size_t)out_hls.rows * out_hls.cols * out_hls.elemSize();

    cl_int err;
    std::vector<cl::Device> devices = xcl::get_xil_devices();
    cl::Device device = devices[0];
    OCL_CHECK(err, cl::Context context(device, NULL, NULL, NULL, &err));
    OCL_CHECK(err, cl::CommandQueue queue(context, device, CL_QUEUE_PROFILING_ENABLE, &err));
    OCL_CHECK(err, std::string device_name = device.getInfo<CL_DEVICE_NAME>(&err));

    std::cout << "INFO: Device found - " << device_name << std::endl;
    std::cout << "Stitch canvas: " << dst_w << "x" << dst_h << std::endl;

    std::string binaryFile = xcl::find_binary_file(device_name, "krnl_stitch");
    cl::Program::Binaries bins = xcl::import_binary_file(binaryFile);
    devices.resize(1);
    OCL_CHECK(err, cl::Program program(context, devices, bins, NULL, &err));
    OCL_CHECK(err, cl::Kernel kernel(program, "stitch_accel", &err));

    std::vector<cl::Buffer> buf_img(4), buf_mask(4);
    for (int i = 0; i < 4; i++) {
        OCL_CHECK(err, buf_img[i] = cl::Buffer(context, CL_MEM_READ_ONLY, img_in_size_bytes[i], NULL, &err));
        OCL_CHECK(err, buf_mask[i] = cl::Buffer(context, CL_MEM_READ_ONLY, mask_in_size_bytes[i], NULL, &err));
    }
    OCL_CHECK(err, cl::Buffer buf_out(context, CL_MEM_WRITE_ONLY, image_out_size_bytes, NULL, &err));
    OCL_CHECK(err, cl::Buffer buf_sizes(context, CL_MEM_READ_ONLY, sizeof(img_sizes), NULL, &err));
    OCL_CHECK(err, cl::Buffer buf_corners(context, CL_MEM_READ_ONLY, sizeof(mask_corners), NULL, &err));

    int arg = 0;
    for (int i = 0; i < 4; i++) OCL_CHECK(err, err = kernel.setArg(arg++, buf_img[i]));
    for (int i = 0; i < 4; i++) OCL_CHECK(err, err = kernel.setArg(arg++, buf_mask[i]));
    OCL_CHECK(err, err = kernel.setArg(arg++, buf_out));
    OCL_CHECK(err, err = kernel.setArg(arg++, buf_sizes));
    OCL_CHECK(err, err = kernel.setArg(arg++, buf_corners));
    OCL_CHECK(err, err = kernel.setArg(arg++, dst_h));
    OCL_CHECK(err, err = kernel.setArg(arg++, dst_w));

    cl::Event event;
    for (int i = 0; i < 4; i++) {
        OCL_CHECK(err,
                  queue.enqueueWriteBuffer(buf_img[i], CL_TRUE, 0, img_in_size_bytes[i], in[i].data, nullptr, &event));
        OCL_CHECK(err, queue.enqueueWriteBuffer(buf_mask[i], CL_TRUE, 0, mask_in_size_bytes[i], mask[i].data, nullptr,
                                                &event));
    }
    OCL_CHECK(err, queue.enqueueWriteBuffer(buf_sizes, CL_TRUE, 0, sizeof(img_sizes), img_sizes, nullptr, &event));
    OCL_CHECK(err,
              queue.enqueueWriteBuffer(buf_corners, CL_TRUE, 0, sizeof(mask_corners), mask_corners, nullptr, &event));
    // Profiling Objects
    cl_ulong start = 0;
    cl_ulong end = 0;
    double diff_prof = 0.0f;
    cl::Event event_sp;

    // Execute the kernel:
    OCL_CHECK(err, err = queue.enqueueTask(kernel, NULL, &event_sp));
    clWaitForEvents(1, (const cl_event*)&event_sp);

    event_sp.getProfilingInfo(CL_PROFILING_COMMAND_START, &start);
    event_sp.getProfilingInfo(CL_PROFILING_COMMAND_END, &end);
    diff_prof = end - start;
    std::cout << "INFO: Latency for hardware function is " << (diff_prof / 1000000) << "ms" << std::endl;

    OCL_CHECK(err, queue.enqueueReadBuffer(buf_out, CL_TRUE, 0, image_out_size_bytes, out_hls.data, nullptr, &event));
    queue.finish();

    cv::imwrite("in_img1.jpg", in_img1);
    cv::imwrite("in_img2.jpg", in_img2);
    cv::imwrite("in_img3.jpg", in_img3);
    cv::imwrite("in_img4.jpg", in_img4);
    cv::imwrite("mask_img1.jpg", mask[0]);
    cv::imwrite("mask_img2.jpg", mask[1]);
    cv::imwrite("mask_img3.jpg", mask[2]);
    cv::imwrite("mask_img4.jpg", mask[3]);
    cv::imwrite("out_hls.jpg", out_hls);

    std::cout << "Testcase passed" << std::endl;
    return 0;
}
