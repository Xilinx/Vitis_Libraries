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
#include "xf_stitch_tb_config.h"

#include "common/xf_axi.hpp"
#include <iostream>
#include <fstream>
#include <strstream>
// Per-stream top-left on the stitched output canvas for xf::cv::stich_feed (see xf_stich.hpp).
// When mask non-zero regions start at (0,0), this matches the legacy TB hardcoded corners.
static void computeStitchMaskCornersFromMasks(const cv::Mat mask_img[4], cv::Point out_corners[4]) {
    int mask_corners_accel_temp[8];
    mask_corners_accel_temp[0] = mask_img[0].cols;
    mask_corners_accel_temp[1] = mask_img[0].rows;
    mask_corners_accel_temp[2] = mask_img[1].cols;
    mask_corners_accel_temp[3] = mask_img[1].rows;
    mask_corners_accel_temp[4] = mask_img[2].cols;
    mask_corners_accel_temp[5] = mask_img[2].rows;
    mask_corners_accel_temp[6] = mask_img[3].cols;
    mask_corners_accel_temp[7] = mask_img[3].rows;

    // Find the maximum cols and rows among the 4 masks
    int max_cols = mask_img[0].cols;
    int max_rows = mask_img[0].rows;
    for (int i = 1; i < 4; ++i) {
        if (mask_img[i].cols > max_cols) max_cols = mask_img[i].cols;
        if (mask_img[i].rows > max_rows) max_rows = mask_img[i].rows;
    }
    std::cout << "max_cols: " << max_cols << ", max_rows: " << max_rows << std::endl;
    out_corners[0].x = 0;
    out_corners[1].x = max_cols - mask_img[1].cols;
    out_corners[2].x = 0;
    out_corners[3].x = 0;

    out_corners[0].y = mask_img[0].rows;
    out_corners[1].y = mask_img[0].rows;
    out_corners[2].y = max_rows;
    out_corners[3].y = mask_img[0].rows;
}

int main(int argc, char** argv) {
    if (argc != 6) {
        fprintf(stderr, "Usage: %s <INPUT IMAGE PATH 1> <INPUT IMAGE PATH 2>\n", argv[0]);
        return EXIT_FAILURE;
    }

    cv::Mat in_img1, in_img2, in_img3, in_img4, diff;
    cv::Mat mask_img[4];

// Reading in the images:
#if RGB == 1
    in_img1 = cv::imread(argv[1], cv::IMREAD_COLOR);
    in_img2 = cv::imread(argv[2], cv::IMREAD_COLOR);
    in_img3 = cv::imread(argv[3], cv::IMREAD_COLOR);
    in_img4 = cv::imread(argv[4], cv::IMREAD_COLOR);
#elif GRAY == 1
    in_img1 = cv::imread(argv[1], cv::IMREAD_GRAYSCALE);
    in_img2 = cv::imread(argv[2], cv::IMREAD_GRAYSCALE);
    in_img3 = cv::imread(argv[3], cv::IMREAD_GRAYSCALE);
    in_img4 = cv::imread(argv[4], cv::IMREAD_GRAYSCALE);
#endif
    std::string mask_folder = argv[5];
    if (mask_folder.back() != '/') mask_folder += '/';
    std::cout << "Mask inputs folder: " << mask_folder << std::endl;

    for (int i = 0; i < 4; i++) {
        std::string mask_name = mask_folder + "mask" + std::to_string(i + 1) + "_000000.png";
        mask_img[i] = cv::imread(mask_name, cv::IMREAD_GRAYSCALE);
        std::cout << "Mask " << (i + 1) << ": " << mask_name << std::endl;
        std::cout << "  Image size: " << mask_img[i].cols << " x " << mask_img[i].rows << std::endl;
    }

    // int mask_corners[8] = {0,427,910,426,0,1076,0,416};
    int mask_corners_accel[8] = {0};
    cv::Point mask_corners[4];
    computeStitchMaskCornersFromMasks(mask_img, mask_corners);

    // Fill accelerator and image size arrays
    for (int i = 0; i < 4; i++) {
        mask_corners_accel[i * 2] = mask_corners[i].x;
        mask_corners_accel[i * 2 + 1] = mask_corners[i].y;
    }
    int img_sizes[8] = {in_img1.rows, in_img1.cols, in_img2.rows, in_img2.cols,
                        in_img3.rows, in_img3.cols, in_img4.rows, in_img4.cols};

    // ---------- OpenCV FeatherBlender reference stitching for validation ----------

    std::vector<cv::Mat> stitch_images = {in_img1, in_img2, in_img3, in_img4};

    // Use computed corners and image sizes
    std::vector<cv::Point> blend_corners;
    std::vector<cv::Size> blend_sizes;
    for (int i = 0; i < 4; ++i) {
        blend_corners.push_back(mask_corners[i]);
        blend_sizes.push_back(stitch_images[i].size());
    }

    // Array of masks loaded above (ensure it's thresholded if FeatherBlender expects binary)
    cv::Mat mask_img_arr[4] = {mask_img[0], mask_img[1], mask_img[2], mask_img[3]};

    // Calculate overall panorama region of interest
    cv::Rect dst_roi = cv::detail::resultRoi(blend_corners, blend_sizes);

    std::cout << "\nDestination ROI: [" << dst_roi.x << ", " << dst_roi.y << ", " << dst_roi.width << ", "
              << dst_roi.height << "]" << std::endl;

    int dst_height = dst_roi.height;
    int dst_width = dst_roi.width;
#if RGB == 1
    cv::Mat out_img(dst_height, dst_width, CV_8UC3);
#elif GRAY == 1
    cv::Mat out_img(dst_height, dst_width, CV_8UC1);
#endif

    // Set up FeatherBlender
    const float feather_sharpness = 0.02f;
    cv::detail::FeatherBlender blender;
    blender.setSharpness(feather_sharpness);
    blender.prepare(dst_roi);

    // Feed images and corresponding masks
    for (int i = 0; i < 4; ++i) {
        cv::Mat img_s;
        stitch_images[i].convertTo(img_s, CV_16SC3);

        cv::Mat feed_mask;
        if (mask_img_arr[i].type() != CV_8UC1) {
            cv::cvtColor(mask_img_arr[i], feed_mask, cv::COLOR_BGR2GRAY);
        } else {
            feed_mask = mask_img_arr[i].clone();
        }

        // Resize mask to match input image if needed
        if (feed_mask.size() != stitch_images[i].size()) {
            cv::Mat resized_mask;
            cv::resize(feed_mask, resized_mask, stitch_images[i].size(), 0, 0, cv::INTER_NEAREST);
            feed_mask = resized_mask;
        }
        blender.feed(img_s, feed_mask, blend_corners[i]);
    }

    // Blend images and write outputs
    cv::Mat ocv_blend_result, ocv_blend_result_mask;
    blender.blend(ocv_blend_result, ocv_blend_result_mask);

    cv::Mat ocv_final_blend;
    ocv_blend_result.convertTo(ocv_final_blend, stitch_images[0].type()); // preserve type

    cv::imwrite("ocv_blended_panorama.png", ocv_final_blend);
    cv::imwrite("ocv_blend_mask.png", ocv_blend_result_mask);

    // ---------- End OpenCV FeatherBlender reference stitching ----------

    // Call the top fucntion
    stitch_accel((ap_uint<INPUT_PTR_WIDTH>*)in_img1.data, (ap_uint<INPUT_PTR_WIDTH>*)in_img2.data,
                 (ap_uint<INPUT_PTR_WIDTH>*)in_img3.data, (ap_uint<INPUT_PTR_WIDTH>*)in_img4.data,
                 (ap_uint<MASK_INPUT_PTR_WIDTH>*)mask_img[0].data, (ap_uint<MASK_INPUT_PTR_WIDTH>*)mask_img[1].data,
                 (ap_uint<MASK_INPUT_PTR_WIDTH>*)mask_img[2].data, (ap_uint<MASK_INPUT_PTR_WIDTH>*)mask_img[3].data,
                 (ap_uint<INPUT_PTR_WIDTH>*)out_img.data, img_sizes, mask_corners_accel, dst_height, dst_width);

    cv::imwrite("in_img1.jpg", in_img1);
    cv::imwrite("in_img2.jpg", in_img2);
    cv::imwrite("in_img3.jpg", in_img3);
    cv::imwrite("in_img4.jpg", in_img4);
    cv::imwrite("mask_img[0].jpg", mask_img[0]);
    cv::imwrite("mask_img[1].jpg", mask_img[1]);
    cv::imwrite("mask_img[2].jpg", mask_img[2]);
    cv::imwrite("mask_img[3].jpg", mask_img[3]);
    cv::imwrite("out_hls.jpg", out_img);

    //     cv::absdiff(out_img, ocv_final_blend, diff);
    //     cv::imwrite("diff.png", diff);
    //   float err_per;
    //   xf::cv::analyzeDiff(diff, 5, err_per);

    //   if (err_per > 1) {
    //       fprintf(stderr, "ERROR: Test Failed.\n ");
    //       return -1;
    //   } else
    //       std::cout << "Test Passed " << std::endl;
    return 0;
}
