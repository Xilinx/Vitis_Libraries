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
//#include "xf_isp_types.h"
//#include "common/xf_axi.hpp"
#include <iostream>
#include <fstream>
#include <strstream>
#include <string>
#include <iomanip>
#include <opencv2/stitching/detail/blenders.hpp>
#include "xf_AVFIRS_4stream_tb_config.h"
#include "common/xf_axi.hpp"
#include <time.h>

#define READ_MAPS_FROM_FILE 1

using namespace std;

// Helper function to read binary file containing both mapx and mapy with size headers
// Binary format: [int32 mapx_rows][int32 mapx_cols][float mapx_data...][int32 mapy_rows][int32 mapy_cols][float
// mapy_data...]
bool readBinaryMapsFromFile(const std::string& filename, cv::Mat& map_x, cv::Mat& map_y, int& out_rows, int& out_cols) {
    std::ifstream ifs(filename, std::ios::binary);
    if (!ifs.is_open()) {
        std::cerr << "Error: Cannot open binary file: " << filename << std::endl;
        return false;
    }

    // Read mapx size header
    int32_t mapx_rows, mapx_cols;
    ifs.read(reinterpret_cast<char*>(&mapx_rows), sizeof(int32_t));
    ifs.read(reinterpret_cast<char*>(&mapx_cols), sizeof(int32_t));

    if (ifs.fail()) {
        std::cerr << "Error: Failed to read mapx size header from: " << filename << std::endl;
        return false;
    }

    std::cout << "  File: " << filename << std::endl;
    std::cout << "    mapx size: " << mapx_rows << " x " << mapx_cols << std::endl;

    // Create mapx matrix and read data
    map_x.create(mapx_rows, mapx_cols, CV_32FC1);
    size_t mapx_data_size = static_cast<size_t>(mapx_rows) * mapx_cols * sizeof(float);
    ifs.read(reinterpret_cast<char*>(map_x.data), mapx_data_size);

    if (ifs.fail()) {
        std::cerr << "Error: Failed to read mapx data from: " << filename << std::endl;
        return false;
    }

    // Read mapy size header
    int32_t mapy_rows, mapy_cols;
    ifs.read(reinterpret_cast<char*>(&mapy_rows), sizeof(int32_t));
    ifs.read(reinterpret_cast<char*>(&mapy_cols), sizeof(int32_t));

    if (ifs.fail()) {
        std::cerr << "Error: Failed to read mapy size header from: " << filename << std::endl;
        return false;
    }

    std::cout << "    mapy size: " << mapy_rows << " x " << mapy_cols << std::endl;

    // Create mapy matrix and read data
    map_y.create(mapy_rows, mapy_cols, CV_32FC1);
    size_t mapy_data_size = static_cast<size_t>(mapy_rows) * mapy_cols * sizeof(float);
    ifs.read(reinterpret_cast<char*>(map_y.data), mapy_data_size);

    if (ifs.fail()) {
        std::cerr << "Error: Failed to read mapy data from: " << filename << std::endl;
        return false;
    }

    ifs.close();

    // Verify mapx and mapy have same dimensions
    if (mapx_rows != mapy_rows || mapx_cols != mapy_cols) {
        std::cerr << "Warning: mapx and mapy size mismatch in " << filename << std::endl;
        std::cerr << "  mapx: " << mapx_rows << "x" << mapx_cols << ", mapy: " << mapy_rows << "x" << mapy_cols
                  << std::endl;
    }

    // Return the mapx size as the output size
    out_rows = mapx_rows;
    out_cols = mapx_cols;

    return true;
}

// Load dewarp maps from binary files (each file contains both mapx and mapy)
void initializeDewarpMaps(const std::string& dewarp_folder,
                          cv::Mat map_x[],
                          cv::Mat map_y[],
                          cv::Mat map_x_int[],
                          cv::Mat map_y_int[],
                          int dewarp_rows[],
                          int dewarp_cols[]) {
#if READ_MAPS_FROM_FILE
    std::cout << "Loading dewarp maps from: " << dewarp_folder << std::endl;

    for (int k = 0; k < 4; k++) {
        std::string map_file = dewarp_folder + "dewarp_LUT_" + std::to_string(k + 1) + ".bin";

        if (!readBinaryMapsFromFile(map_file, map_x[k], map_y[k], dewarp_rows[k], dewarp_cols[k])) {
            std::cerr << "Failed to load dewarp maps for camera " << (k + 1) << std::endl;
            exit(-1);
        }

        // Create integer maps
        map_x_int[k].create(dewarp_rows[k], dewarp_cols[k], CV_32SC1);
        map_y_int[k].create(dewarp_rows[k], dewarp_cols[k], CV_32SC1);

        for (int i = 0; i < dewarp_rows[k]; i++) {
            for (int j = 0; j < dewarp_cols[k]; j++) {
                map_x_int[k].at<int32_t>(i, j) = static_cast<int32_t>(map_x[k].at<float>(i, j) * 256);
                map_y_int[k].at<int32_t>(i, j) = static_cast<int32_t>(map_y[k].at<float>(i, j) * 256);
            }
        }
    }
#endif
}

// Load dewarp maps from binary files (each file contains both mapx and mapy)
void initializestich_accelMaps(const std::string& stitch_accel_folder,
                               cv::Mat map_x[],
                               cv::Mat map_y[],
                               cv::Mat map_x_int[],
                               cv::Mat map_y_int[],
                               int stitch_rows[],
                               int stitch_cols[]) {
#if READ_MAPS_FROM_FILE
    std::cout << "Loading stitch_accel maps from: " << stitch_accel_folder << std::endl;

    for (int k = 0; k < 4; k++) {
        std::string map_file = stitch_accel_folder + "stitching_LUT_" + std::to_string(k + 1) + ".bin";

        if (!readBinaryMapsFromFile(map_file, map_x[k], map_y[k], stitch_rows[k], stitch_cols[k])) {
            std::cerr << "Failed to load stitch_accel maps for camera " << (k + 1) << std::endl;
            exit(-1);
        }

        // Create integer maps
        map_x_int[k].create(stitch_rows[k], stitch_cols[k], CV_32SC1);
        map_y_int[k].create(stitch_rows[k], stitch_cols[k], CV_32SC1);

        for (int i = 0; i < stitch_rows[k]; i++) {
            for (int j = 0; j < stitch_cols[k]; j++) {
                map_x_int[k].at<int32_t>(i, j) = static_cast<int32_t>(map_x[k].at<float>(i, j) * 256);
                map_y_int[k].at<int32_t>(i, j) = static_cast<int32_t>(map_y[k].at<float>(i, j) * 256);
            }
        }
    }
#endif
}

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
    cv::Mat final_output, ir_output;

    if (argc != 5) {
        fprintf(
            stderr,
            "Usage: <executable> <images_folder> <dewarp_lut_folder> <stitch_inputs_folder> <mask_inputs_folder>\n");
        fprintf(stderr, "  images_folder: folder containing frame_000000.png to frame_000003.png\n");
        fprintf(stderr, "  dewarp_lut_folder: folder containing dewarp_LUT_X.bin (X=1-4)\n");
        fprintf(stderr, "  stitch_inputs_folder: folder containing stitch_LUT_X.bin (X=1-4)\n");
        fprintf(stderr, "  mask_inputs_folder: folder containing mask_inputs/\n");
        return -1;
    }

    std::string images_folder = argv[1];
    // Ensure folder ends with /
    if (images_folder.back() != '/') images_folder += '/';
    std::cout << "Images folder: " << images_folder << std::endl;

#if READ_MAPS_FROM_FILE
    std::string dewarp_folder = argv[2];
    if (dewarp_folder.back() != '/') dewarp_folder += '/';
    std::cout << "Dewarp LUT folder: " << dewarp_folder << std::endl;

    std::string stitch_folder = argv[3];
    if (stitch_folder.back() != '/') stitch_folder += '/';
    std::cout << "Stitch inputs folder: " << stitch_folder << std::endl;

    std::string mask_folder = argv[4];
    if (mask_folder.back() != '/') mask_folder += '/';
    std::cout << "Mask inputs folder: " << mask_folder << std::endl;
#endif

    cv::Mat img[4], diff;
    // Load images from folder
    std::string image_names[4] = {images_folder + "frame_000000.png", images_folder + "frame_000001.png",
                                  images_folder + "frame_000002.png", images_folder + "frame_000003.png"};

    for (int i = 0; i < 4; i++) {
        img[i] = cv::imread(image_names[i], cv::IMREAD_COLOR);
        std::cout << "Loading image (COLOR): " << image_names[i] << std::endl;
    }

    for (int i = 0; i < 4; i++) {
        if (img[i].data == NULL) {
            fprintf(stderr, "Cannot open image at %s\n", image_names[i].c_str());
            return -1;
        }
    }

// Load mask images from mask_inputs folder and extract corners/sizes
#if READ_MAPS_FROM_FILE
    cv::Mat mask_img[4];
    cv::Point mask_corners[4]; // Top-left corner of valid region in each mask
    cv::Size mask_sizes[4];    // Size (width, height) of valid region in each mask
    cv::Rect mask_rois[4];     // Bounding rectangle of valid region

    std::cout << "\n===== Mask Analysis =====" << std::endl;
    for (int i = 0; i < 4; i++) {
        std::string mask_name = mask_folder + "mask" + std::to_string(i + 1) + "_000000.png";
        mask_img[i] = cv::imread(mask_name, cv::IMREAD_GRAYSCALE);
        std::cout << "Mask " << (i + 1) << ": " << mask_name << std::endl;

        if (mask_img[i].data == NULL) {
            std::cout << "  Status: NOT FOUND (using full image as mask)" << std::endl;
            // Create a full white mask if not found
            mask_img[i].create(img[i].rows, img[i].cols, CV_8UC1);
            mask_img[i].setTo(cv::Scalar(255));
            mask_corners[i] = cv::Point(0, 0);
            mask_sizes[i] = cv::Size(img[i].cols, img[i].rows);
            mask_rois[i] = cv::Rect(0, 0, img[i].cols, img[i].rows);
        } else {
            std::cout << "  Image size: " << mask_img[i].cols << " x " << mask_img[i].rows << std::endl;

            // Find bounding rectangle of non-zero (valid) pixels
            // Method 1: Using findNonZero and boundingRect
            std::vector<cv::Point> non_zero_points;
            cv::findNonZero(mask_img[i], non_zero_points);

            if (non_zero_points.empty()) {
                std::cout << "  WARNING: Mask is empty (all zeros)!" << std::endl;
                mask_corners[i] = cv::Point(0, 0);
                mask_sizes[i] = cv::Size(0, 0);
                mask_rois[i] = cv::Rect(0, 0, 0, 0);
            } else {
                mask_rois[i] = cv::boundingRect(non_zero_points);
                mask_corners[i] = mask_rois[i].tl(); // Top-left corner
                mask_sizes[i] = mask_rois[i].size(); // Width x Height
            }
        }

        std::cout << "  Size (w x h): " << mask_sizes[i].width << " x " << mask_sizes[i].height << std::endl;
        std::cout << "  ROI: [" << mask_rois[i].x << ", " << mask_rois[i].y << ", " << mask_rois[i].width << ", "
                  << mask_rois[i].height << "]" << std::endl;

        // Count non-zero pixels for coverage percentage
        int non_zero_count = cv::countNonZero(mask_img[i]);
        int total_pixels = mask_img[i].rows * mask_img[i].cols;
        float coverage = (float)non_zero_count / total_pixels * 100.0f;
        std::cout << "  Valid pixels: " << non_zero_count << " / " << total_pixels << " (" << std::fixed
                  << std::setprecision(1) << coverage << "%)" << std::endl;
    }
    std::cout << "========================\n" << std::endl;
#endif

    computeStitchMaskCornersFromMasks(mask_img, mask_corners);

    std::cout << "Stitch canvas mask_corners (for accel / blender): ";
    for (int mi = 0; mi < 4; ++mi) {
        std::cout << "(" << mask_corners[mi].x << "," << mask_corners[mi].y << ") ";
    }
    std::cout << std::endl;

    /* Filled after stitch map init: per-tile {rows,cols} for kernel xf::Mat (not full mask canvas). */
    int img_sizes[8];

    int in_rows = img[0].rows;
    int in_cols = img[0].cols;

    // Dewarp map arrays - will be sized from binary files
    cv::Mat map_x[4], map_y[4], map_x_int[4], map_y_int[4];

    int dewarp_rows[4], dewarp_cols[4];

    // Stitch map arrays - will be sized from binary files
    cv::Mat stitch_map_x[4], stitch_map_y[4];
    int stitch_rows[4], stitch_cols[4];
    cv::Mat map_x_int_stitch[4], map_y_int_stitch[4];
    // Load dewarp maps from binary files (sizes extracted from file headers)
    initializeDewarpMaps(dewarp_folder, map_x, map_y, map_x_int, map_y_int, dewarp_rows, dewarp_cols);

    initializestich_accelMaps(stitch_folder, stitch_map_x, stitch_map_y, map_x_int_stitch, map_y_int_stitch,
                              stitch_rows, stitch_cols);

    for (int i = 0; i < 4; i++) {
        img_sizes[2 * i] = stitch_rows[i];
        img_sizes[2 * i + 1] = stitch_cols[i];
    }
    std::cout << "INFO: img_sizes from stitch tiles (h0,w0,...): ";
    for (int i = 0; i < 8; i++) std::cout << img_sizes[i] << (i < 7 ? "," : "");
    std::cout << std::endl;

    // Create output arrays with sizes from map files
    cv::Mat ocv_remapped[4];    // First remap output (dewarp size)
    cv::Mat stitch_remapped[4]; // Second remap output (stitch size)

    for (int i = 0; i < 4; i++) {
        // First remap output uses dewarp map size
        ocv_remapped[i].create(dewarp_rows[i], dewarp_cols[i], CV_OUT_TYPE);
        std::cout << "Dewarp output[" << i << "]: " << dewarp_rows[i] << " x " << dewarp_cols[i] << std::endl;

        // Second remap output uses stitch map size
        stitch_remapped[i].create(stitch_rows[i], stitch_cols[i], CV_OUT_TYPE);
        std::cout << "Stitch output[" << i << "]: " << stitch_rows[i] << " x " << stitch_cols[i] << std::endl;
    }

    // Combined matrix - uses stitch output size (all 4 stitch outputs stacked vertically)
    // Assuming all stitch outputs have same size, use stitch_rows[0] and stitch_cols[0]
    cv::Mat comb_matrix(stitch_rows[0] * 4, stitch_cols[0], CV_OUT_TYPE);
    std::cout << "Combined matrix size: " << stitch_rows[0] * 4 << " x " << stitch_cols[0] << std::endl;

    // Resize matrix
    cv::Mat resized_panorama;
    // CvtColor Output
    cv::Mat ocv_rgb2gray;

    // Gaussian Output and parameters
    cv::Mat ocv_gaussian;
    ocv_gaussian.create(in_rows, in_cols, CV_IN_TYPE); // create memory for OCV-ref image
    // Sigma value for Gaussian blur (float). Typical range: 0.5 to 2.0
    float sigma = 0.8333f;
    // Integer version of sigma (not usually used in OpenCV GaussianBlur; range: 0 - 2)
    int sigma_int = 0.8333f;

    // Sobel (OpenCV reference): cv::Sobel scale/delta — xf::cv::Sobel has no such args (see docs api-reference.rst).
    // OpenCV: dst = scale * derivative + delta (optional doubles, no xf-documented bounds). Use 1, 0 to match xf
    // kernels.
    int scale = 1;
    int delta = 0;
    cv::Mat ocv_sobel_x_1, ocv_sobel_y_1;
    int ddepth = CV_32F;

    // Magnitude Output and parameters
    cv::Mat ocv_magnitude, ocv_magnitude_res_16S;

    // Bit depth conversion parameter for magnitude image
    cv::Mat ocv_magnitude_res_8U;
    // shift value for bit depth conversion in convertTo
    int shift = 0;

    // Threshold output and parameters
    // [Range: threshold parameters for magnitude to edge conversion]
    unsigned char maxval = 255; // Maximum value for cv::threshold output
    unsigned char thresh = 50;  // Threshold value to binarize magnitude image
    // [End Range]
    cv::Mat ocv_thresh;

    // FindContours Output and parameters
    std::vector<std::vector<cv::Point> > ocv_contours;

    // Reference pipeline
    struct timespec begin_hw, end_hw;
    clock_gettime(CLOCK_REALTIME, &begin_hw);

    // remap merging
    cv::Mat map_x_merged[4], map_y_merged[4];
    cv::Mat map_x_merged_int[4], map_y_merged_int[4];
    for (int i = 0; i < 4; i++) {
        map_x_merged[i].create(stitch_rows[i], stitch_cols[i], CV_32FC1);
        map_y_merged[i].create(stitch_rows[i], stitch_cols[i], CV_32FC1);
        map_x_merged_int[i].create(stitch_rows[i], stitch_cols[i], CV_32SC1);
        map_y_merged_int[i].create(stitch_rows[i], stitch_cols[i], CV_32SC1);
        for (int y = 0; y < stitch_rows[i]; y++) {
            for (int x = 0; x < stitch_cols[i]; x++) {
                // 1. Read second-stage remap (into mid image)
                float bx = stitch_map_x[i].at<float>(y, x);
                float by = stitch_map_y[i].at<float>(y, x);

                // 2. Reject invalid coordinates
                if (bx < 0.0f || by < 0.0f || bx >= map_x[i].cols - 1 || by >= map_y[i].rows - 1) {
                    map_x_merged[i].at<float>(y, x) = -1.0f;
                    map_y_merged[i].at<float>(y, x) = -1.0f;
                    map_x_merged_int[i].at<int32_t>(y, x) = static_cast<int32_t>(-1 * 256);
                    map_y_merged_int[i].at<int32_t>(y, x) = static_cast<int32_t>(-1 * 256);
                    continue;
                }

                // 3. Integer base index
                int ix = (int)bx;
                int iy = (int)by;

                // 4. Fractional parts
                float fx = bx - ix;
                float fy = by - iy;

                // 5. Fetch 4 neighboring samples from mapA
                float ax00 = map_x[i].at<float>(iy, ix);
                float ay00 = map_y[i].at<float>(iy, ix);

                float ax10 = map_x[i].at<float>(iy, ix + 1);
                float ay10 = map_y[i].at<float>(iy, ix + 1);

                float ax01 = map_x[i].at<float>(iy + 1, ix);
                float ay01 = map_y[i].at<float>(iy + 1, ix);

                float ax11 = map_x[i].at<float>(iy + 1, ix + 1);
                float ay11 = map_y[i].at<float>(iy + 1, ix + 1);

                // 6. Bilinear interpolation
                float ax0 = ax00 + fx * (ax10 - ax00);
                float ay0 = ay00 + fx * (ay10 - ay00);

                float ax1 = ax01 + fx * (ax11 - ax01);
                float ay1 = ay01 + fx * (ay11 - ay01);

                float ax = ax0 + fy * (ax1 - ax0);
                float ay = ay0 + fy * (ay1 - ay0);

                // 7. Store final composed map
                map_x_merged[i].at<float>(y, x) = ax;
                map_y_merged[i].at<float>(y, x) = ay;
                map_x_merged_int[i].at<int32_t>(y, x) = static_cast<int32_t>(ax * 256);
                map_y_merged_int[i].at<int32_t>(y, x) = static_cast<int32_t>(ay * 256);
            }
        }
    }

    // XF_WIN_ROWS should be updated with the vakue of num_of_lines by user
    int num_of_lines;

    std::cout << "num_of_lines for map_y_merged: " << std::endl;
    for (int i = 0; i < 4; i++) {
        xf::cv::remapPreproc(map_y_merged[i], num_of_lines);
    }

    // Second remap (stitch) - apply stitch maps to dewarped images
    for (int i = 0; i < 4; i++)
        cv::remap(img[i], stitch_remapped[i], map_x_merged[i], map_y_merged[i], cv::INTER_LINEAR, cv::BORDER_CONSTANT,
                  cv::Scalar(0, 0, 0));

    // Output stitch remapped images
    for (int i = 0; i < 4; ++i) {
        std::ostringstream name;
        name << "stitch_remap_" << i << ".png";
        // cv::imwrite(name.str(), stitch_remapped[i]);
        std::cout << "Saved: " << name.str() << std::endl;
    }

// ========== OpenCV FeatherBlender ==========
#if READ_MAPS_FROM_FILE
    std::cout << "\n===== OpenCV FeatherBlender =====" << std::endl;

    // Use corners from mask images, but sizes from stitch_remapped images
    std::vector<cv::Point> blend_corners;
    std::vector<cv::Size> blend_sizes;

    std::cout << "Using corners from mask images with stitch_remapped sizes:" << std::endl;
    for (int i = 0; i < 4; i++) {
        blend_corners.push_back(mask_corners[i]);
        blend_sizes.push_back(cv::Size(stitch_remapped[i].cols, stitch_remapped[i].rows));
        std::cout << "  Image " << i << ": corner=(" << mask_corners[i].x << "," << mask_corners[i].y
                  << "), stitch_size=" << stitch_remapped[i].cols << "x" << stitch_remapped[i].rows
                  << ", mask_size=" << mask_img[i].cols << "x" << mask_img[i].rows << std::endl;
    }

    // Calculate the destination ROI using mask corners + stitch_remapped sizes
    cv::Rect dst_roi = cv::detail::resultRoi(blend_corners, blend_sizes);
    std::cout << "\nDestination ROI: [" << dst_roi.x << ", " << dst_roi.y << ", " << dst_roi.width << ", "
              << dst_roi.height << "]" << std::endl;

    // Create FeatherBlender
    // Range for feather_sharpness is typically [0.0, 1.0], where lower values produce wider blending
    float feather_sharpness = 0.02f;
    cv::detail::FeatherBlender blender;
    blender.setSharpness(feather_sharpness);
    std::cout << "FeatherBlender sharpness: " << feather_sharpness << std::endl;

    // Prepare the blender with destination ROI
    blender.prepare(dst_roi);
    std::cout << "Blender prepared with ROI: " << dst_roi.width << "x" << dst_roi.height << std::endl;

    // Feed each image with its mask and corner from mask analysis
    for (int i = 0; i < 4; i++) {
        cv::Mat img_s;
        stitch_remapped[i].convertTo(img_s, CV_16SC3);

        cv::Mat feed_mask;
        if (mask_img[i].type() != CV_8UC1) {
            cv::cvtColor(mask_img[i], feed_mask, cv::COLOR_BGR2GRAY);
        } else {
            feed_mask = mask_img[i].clone();
        }

        // Resize mask to match stitch_remapped size
        if (feed_mask.size() != stitch_remapped[i].size()) {
            cv::Mat resized_mask;
            cv::resize(feed_mask, resized_mask, stitch_remapped[i].size(), 0, 0, cv::INTER_NEAREST);
            std::cout << "Feeding image " << i << ":" << std::endl;
            std::cout << "  stitch_remapped: " << img_s.cols << "x" << img_s.rows << std::endl;
            std::cout << "  mask resized from " << feed_mask.cols << "x" << feed_mask.rows << " to "
                      << resized_mask.cols << "x" << resized_mask.rows << std::endl;
            std::cout << "  corner: (" << mask_corners[i].x << "," << mask_corners[i].y << ")" << std::endl;
            feed_mask = resized_mask;
        } else {
            std::cout << "Feeding image " << i << ":" << std::endl;
            std::cout << "  stitch_remapped: " << img_s.cols << "x" << img_s.rows << std::endl;
            std::cout << "  mask: " << feed_mask.cols << "x" << feed_mask.rows << std::endl;
            std::cout << "  corner: (" << mask_corners[i].x << "," << mask_corners[i].y << ")" << std::endl;
        }

        blender.feed(img_s, feed_mask, mask_corners[i]);
        std::cout << "  Fed successfully" << std::endl;
    }

    // Blend and get result
    std::cout << "\nBlending..." << std::endl;
    cv::Mat blend_result, blend_result_mask;
    blender.blend(blend_result, blend_result_mask);

    cv::Mat final_blend;
    blend_result.convertTo(final_blend, CV_8UC3);

    std::cout << "Blend result size: " << final_blend.cols << "x" << final_blend.rows << std::endl;

    std::cout << "Saved: blended_panorama.png" << std::endl;
    std::cout << "Saved: blend_mask.png" << std::endl;
    std::cout << "=========================\n" << std::endl;
#endif
    // ========== End OpenCV FeatherBlender ==========

    // Match kernel xf::cv::resize(imgOutput, resizedOutput): stitch panorama -> (in_rows, in_cols).
    cv::resize(final_blend, resized_panorama, cv::Size(in_cols, in_rows), 0, 0, cv::INTER_LINEAR);
    cv::cvtColor(resized_panorama, ocv_rgb2gray, cv::COLOR_RGB2GRAY);
    cv::GaussianBlur(ocv_rgb2gray, ocv_gaussian, cv::Size(GAUSSIAN_FILTER_WIDTH, GAUSSIAN_FILTER_WIDTH), sigma, sigma,
                     cv::BORDER_CONSTANT);
    cv::Sobel(ocv_gaussian, ocv_sobel_x_1, ddepth, 1, 0, SOBEL_FILTER_WIDTH, scale, delta, cv::BORDER_CONSTANT);
    cv::Sobel(ocv_gaussian, ocv_sobel_y_1, ddepth, 0, 1, SOBEL_FILTER_WIDTH, scale, delta, cv::BORDER_CONSTANT);
    cv::magnitude(ocv_sobel_x_1, ocv_sobel_y_1, ocv_magnitude);
    ocv_magnitude.convertTo(ocv_magnitude_res_16S, CV_16S);
    ocv_magnitude_res_16S.convertTo(ocv_magnitude_res_8U, CV_8U);
    cv::threshold(ocv_magnitude_res_8U, ocv_thresh, thresh, maxval, THRESH_TYPE);
    cv::findContours(ocv_thresh, ocv_contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
    clock_gettime(CLOCK_REALTIME, &end_hw);
    cv::Mat refContourOutput(in_rows, in_cols, CV_8UC1);
    refContourOutput.setTo(cv::Scalar(255));
    drawContours(refContourOutput, ocv_contours, -1, cv::Scalar(0, 255, 0), 0); // Green contours

    cv::imwrite("contours.png", refContourOutput);

    long seconds, nanoseconds;
    double hw_time;
    seconds = end_hw.tv_sec - begin_hw.tv_sec;
    nanoseconds = end_hw.tv_nsec - begin_hw.tv_nsec;
    hw_time = seconds + nanoseconds * 1e-9;
    hw_time = hw_time * 1e3;

    std::cout << "Latency for CPU function is " << hw_time << "ms" << std::endl;

    for (size_t i = 0; i < ocv_contours.size(); ++i) {
        std::cout << "External Contour " << i << ": ";
        for (auto& p : ocv_contours[i]) {
            std::cout << "(" << p.x << "," << p.y << ") ";
        }
        std::cout << "\n";
    }

    // Output parameters
    ap_uint<32>* points = (ap_uint<32>*)malloc(MAX_TOTAL_POINTS * sizeof(int));
    ap_uint<32>* offsets = (ap_uint<32>*)malloc((MAX_CONTOURS + 1) * sizeof(int));
    ap_uint<32>* numc = (ap_uint<32>*)malloc(sizeof(int));
    // Dimensions match kernel stitch-tile img_sizes[8] and dst ROI (not in_rows/in_cols).
    cv::Mat remapOutput1_stitch_duplicate_2(img_sizes[0], img_sizes[1], CV_8UC1);
    cv::Mat remapOutput2_stitch_duplicate_2(img_sizes[2], img_sizes[3], CV_8UC1);
    cv::Mat remapOutput3_stitch_duplicate_2(img_sizes[4], img_sizes[5], CV_8UC1);
    cv::Mat remapOutput4_stitch_duplicate_2(img_sizes[6], img_sizes[7], CV_8UC1);
    cv::Mat imgOutput_duplicate_2(dst_roi.height, dst_roi.width, CV_8UC1);
    cv::Mat resizedOutput_duplicate_2(in_rows, in_cols, CV_8UC1);
    cv::Mat RGB2GRAYOutput_1_duplicate_2(in_rows, in_cols, CV_8UC1);
    cv::Mat RGB2GRAYOutput_2_duplicate_2(in_rows, in_cols, CV_8UC1);
    cv::Mat RGB2GRAYOutput_3_duplicate_2(in_rows, in_cols, CV_8UC1);
    cv::Mat RGB2GRAYOutput_4_duplicate_2(in_rows, in_cols, CV_8UC1);
    cv::Mat gaussianOutput_duplicate_2(in_rows, in_cols, CV_8UC1);
    cv::Mat dstgx_duplicate_2(in_rows, in_cols, CV_16SC1);
    cv::Mat dstgy_duplicate_2(in_rows, in_cols, CV_16SC1);
    cv::Mat magOutput_duplicate_2(in_rows, in_cols, CV_16SC1);
    cv::Mat convertOutput_duplicate_2(in_rows, in_cols, CV_8UC1);
    cv::Mat thresOutput_duplicate_2(in_rows, in_cols, CV_8UC1);

    // OpenCL section: buffer bytes must match device transfers and, where used, the cv::Mat host views.
    // in_rows / in_cols come from img[0]; kernel setArg(24,25) uses a single (rows,cols) for all four Array2xfMat
    // paths.
    for (int i = 0; i < 4; i++) {
        if (img[i].rows != in_rows || img[i].cols != in_cols || img[i].channels() != img[0].channels()) {
            std::cerr << "FATAL: all four inputs must be " << in_rows << "x" << in_cols << "x" << img[0].channels()
                      << " ch; image " << i << " is " << img[i].rows << "x" << img[i].cols << "x" << img[i].channels()
                      << "\n";
            return -1;
        }
    }
    const size_t image_in_size_bytes =
        (size_t)in_rows * (size_t)in_cols * (size_t)img[0].channels() * sizeof(unsigned char);
    const size_t image_size_bytes = (size_t)in_rows * (size_t)in_cols * sizeof(unsigned char);
    const size_t image_16S_size_bytes = (size_t)in_rows * (size_t)in_cols * sizeof(short);
    // Stitch merge maps: device transfer size = underlying OpenCV mat byte count (CV_32SC1, stitch size).
    size_t mapx_bytes[4], mapy_bytes[4];
    for (int i = 0; i < 4; i++) {
        mapx_bytes[i] = (size_t)map_x_merged_int[i].total() * map_x_merged_int[i].elemSize();
        mapy_bytes[i] = (size_t)map_y_merged_int[i].total() * map_y_merged_int[i].elemSize();
    }
    size_t image_out_points_size_bytes = (MAX_TOTAL_POINTS * sizeof(int));
    size_t image_out_offsets_size_bytes = ((MAX_CONTOURS + 1) * sizeof(int));
    size_t image_num_contours_size_bytes = (sizeof(int));
    // Per-tile masks for device: crop top-left if canvas ≥ tile; else INTER_NEAREST resize to tile.

    size_t mask_in_size_bytes[4];
    for (int i = 0; i < 4; i++) {
        mask_in_size_bytes[i] = (size_t)mask_img[i].rows * (size_t)mask_img[i].cols * sizeof(unsigned char);
    }

    int mask_corners_host[8];
    for (int mi = 0; mi < 4; mi++) {
        mask_corners_host[2 * mi] = mask_corners[mi].x;
        mask_corners_host[2 * mi + 1] = mask_corners[mi].y;
    }
    const int dst_height_host = dst_roi.height;
    const int dst_width_host = dst_roi.width;
    const size_t stitch0_bytes =
        (size_t)remapOutput1_stitch_duplicate_2.total() * remapOutput1_stitch_duplicate_2.elemSize();
    const size_t stitch1_bytes =
        (size_t)remapOutput2_stitch_duplicate_2.total() * remapOutput2_stitch_duplicate_2.elemSize();
    const size_t stitch2_bytes =
        (size_t)remapOutput3_stitch_duplicate_2.total() * remapOutput3_stitch_duplicate_2.elemSize();
    const size_t stitch3_bytes =
        (size_t)remapOutput4_stitch_duplicate_2.total() * remapOutput4_stitch_duplicate_2.elemSize();
    const size_t dst_pano_bytes = (size_t)imgOutput_duplicate_2.total() * imgOutput_duplicate_2.elemSize();

    static const struct {
        int rh, rw;
    } kTileMax[4] = {{HEIGHT_1, WIDTH_1}, {HEIGHT_2, WIDTH_2}, {HEIGHT_3, WIDTH_3}, {HEIGHT_4, WIDTH_4}};
    for (int ti = 0; ti < 4; ti++) {
        int h = img_sizes[2 * ti], w = img_sizes[2 * ti + 1];
        if (h <= 0 || w <= 0 || h > kTileMax[ti].rh || w > kTileMax[ti].rw) {
            std::cerr << "FATAL: tile " << ti << " img_sizes " << h << "x" << w << " out of range (1.."
                      << kTileMax[ti].rh << " x 1.." << kTileMax[ti].rw << ")\n";
            return -1;
        }
    }
    if (dst_height_host <= 0 || dst_width_host <= 0 || dst_height_host > HEIGHT_DST || dst_width_host > WIDTH_DST) {
        std::cerr << "FATAL: dst_roi " << dst_height_host << "x" << dst_width_host << " out of range (1.." << HEIGHT_DST
                  << " x 1.." << WIDTH_DST << ")\n";
        return -1;
    }
    if (in_rows <= 0 || in_cols <= 0 || in_rows > HEIGHT || in_cols > WIDTH) {
        std::cerr << "FATAL: in_rows/in_cols " << in_rows << "x" << in_cols << " out of range (1.." << HEIGHT
                  << " x 1.." << WIDTH << ")\n";
        return -1;
    }

    cl_int err;
    std::cout << "INFO: Running OpenCL section." << std::endl;

    std::vector<cl::Device> devices = xcl::get_xil_devices();
    cl::Device device = devices[0];
    OCL_CHECK(err, cl::Context context(device, NULL, NULL, NULL, &err));
    OCL_CHECK(err, cl::CommandQueue ocl_cmdq(context, device, CL_QUEUE_PROFILING_ENABLE, &err));
    std::string device_name = device.getInfo<CL_DEVICE_NAME>();

    std::cout << "INFO: Device found - " << device_name << std::endl;
    std::cout << "Input Image Bit Depth:" << XF_DTPIXELDEPTH(IN_TYPE, XF_NPPCX) << std::endl;
    std::cout << "Input Image Channels:" << XF_CHANNELS(IN_TYPE, XF_NPPCX) << std::endl;
    std::cout << "NPPC:" << XF_NPPCX << std::endl;

    std::string binaryFile = xcl::find_binary_file(device_name, "krnl_AVFIRS_4stream");
    cl::Program::Binaries bins = xcl::import_binary_file(binaryFile);
    devices.resize(1);
    std::cout << "INFO: Programming FPGA with \"" << binaryFile << "\" (embedded may take minutes)..." << std::endl;
    std::cout.flush();
    OCL_CHECK(err, cl::Program program(context, devices, bins, NULL, &err));
    std::cout << "INFO: xclbin programmed successfully." << std::endl;
    std::cout.flush();

    OCL_CHECK(err, cl::Kernel kernel(program, "AVFIRS_4stream_accel", &err));
    std::cout << "INFO: Kernel object created." << std::endl;
    std::cout.flush();

    OCL_CHECK(err, cl::Buffer buffer_inImage1(context, CL_MEM_READ_ONLY, image_in_size_bytes, NULL, &err));
    OCL_CHECK(err, cl::Buffer buffer_inImage2(context, CL_MEM_READ_ONLY, image_in_size_bytes, NULL, &err));
    OCL_CHECK(err, cl::Buffer buffer_inImage3(context, CL_MEM_READ_ONLY, image_in_size_bytes, NULL, &err));
    OCL_CHECK(err, cl::Buffer buffer_inImage4(context, CL_MEM_READ_ONLY, image_in_size_bytes, NULL, &err));
    OCL_CHECK(err, cl::Buffer buffer_mapx1(context, CL_MEM_READ_ONLY, mapx_bytes[0], NULL, &err));
    OCL_CHECK(err, cl::Buffer buffer_mapx2(context, CL_MEM_READ_ONLY, mapx_bytes[1], NULL, &err));
    OCL_CHECK(err, cl::Buffer buffer_mapx3(context, CL_MEM_READ_ONLY, mapx_bytes[2], NULL, &err));
    OCL_CHECK(err, cl::Buffer buffer_mapx4(context, CL_MEM_READ_ONLY, mapx_bytes[3], NULL, &err));
    OCL_CHECK(err, cl::Buffer buffer_mapy1(context, CL_MEM_READ_ONLY, mapy_bytes[0], NULL, &err));
    OCL_CHECK(err, cl::Buffer buffer_mapy2(context, CL_MEM_READ_ONLY, mapy_bytes[1], NULL, &err));
    OCL_CHECK(err, cl::Buffer buffer_mapy3(context, CL_MEM_READ_ONLY, mapy_bytes[2], NULL, &err));
    OCL_CHECK(err, cl::Buffer buffer_mapy4(context, CL_MEM_READ_ONLY, mapy_bytes[3], NULL, &err));
    OCL_CHECK(err, cl::Buffer buffer_img_sizes(context, CL_MEM_READ_ONLY, 8 * sizeof(int), NULL, &err));
    OCL_CHECK(err, cl::Buffer buffer_mask_corners(context, CL_MEM_READ_ONLY, 8 * sizeof(int), NULL, &err));
    OCL_CHECK(err, cl::Buffer buffer_outPoints(context, CL_MEM_WRITE_ONLY, image_out_points_size_bytes, NULL, &err));
    OCL_CHECK(err, cl::Buffer buffer_outOffsets(context, CL_MEM_WRITE_ONLY, image_out_offsets_size_bytes, NULL, &err));
    OCL_CHECK(err,
              cl::Buffer buffer_numContours(context, CL_MEM_WRITE_ONLY, image_num_contours_size_bytes, NULL, &err));
    OCL_CHECK(err, cl::Buffer buffer_mask1(context, CL_MEM_READ_ONLY, mask_in_size_bytes[0], NULL, &err));
    OCL_CHECK(err, cl::Buffer buffer_mask2(context, CL_MEM_READ_ONLY, mask_in_size_bytes[1], NULL, &err));
    OCL_CHECK(err, cl::Buffer buffer_mask3(context, CL_MEM_READ_ONLY, mask_in_size_bytes[2], NULL, &err));
    OCL_CHECK(err, cl::Buffer buffer_mask4(context, CL_MEM_READ_ONLY, mask_in_size_bytes[3], NULL, &err));
    OCL_CHECK(err,
              cl::Buffer buffer_resizedOutput_duplicate_2(context, CL_MEM_WRITE_ONLY, image_size_bytes, NULL, &err));

    OCL_CHECK(err, err = kernel.setArg(0, buffer_inImage1));
    OCL_CHECK(err, err = kernel.setArg(1, buffer_inImage2));
    OCL_CHECK(err, err = kernel.setArg(2, buffer_inImage3));
    OCL_CHECK(err, err = kernel.setArg(3, buffer_inImage4));
    OCL_CHECK(err, err = kernel.setArg(4, buffer_mapx1));
    OCL_CHECK(err, err = kernel.setArg(5, buffer_mapy1));
    OCL_CHECK(err, err = kernel.setArg(6, buffer_mapx2));
    OCL_CHECK(err, err = kernel.setArg(7, buffer_mapy2));
    OCL_CHECK(err, err = kernel.setArg(8, buffer_mapx3));
    OCL_CHECK(err, err = kernel.setArg(9, buffer_mapy3));
    OCL_CHECK(err, err = kernel.setArg(10, buffer_mapx4));
    OCL_CHECK(err, err = kernel.setArg(11, buffer_mapy4));
    OCL_CHECK(err, err = kernel.setArg(12, buffer_mask1));
    OCL_CHECK(err, err = kernel.setArg(13, buffer_mask2));
    OCL_CHECK(err, err = kernel.setArg(14, buffer_mask3));
    OCL_CHECK(err, err = kernel.setArg(15, buffer_mask4));
    OCL_CHECK(err, err = kernel.setArg(16, buffer_outPoints));
    OCL_CHECK(err, err = kernel.setArg(17, buffer_outOffsets));
    OCL_CHECK(err, err = kernel.setArg(18, buffer_numContours));
    OCL_CHECK(err, err = kernel.setArg(19, sigma));
    OCL_CHECK(err, err = kernel.setArg(20, shift));
    OCL_CHECK(err, err = kernel.setArg(21, thresh));
    OCL_CHECK(err, err = kernel.setArg(22, maxval));
    OCL_CHECK(err, err = kernel.setArg(23, in_rows));
    OCL_CHECK(err, err = kernel.setArg(24, in_cols));
    OCL_CHECK(err, err = kernel.setArg(25, buffer_img_sizes));
    OCL_CHECK(err, err = kernel.setArg(26, buffer_mask_corners));
    OCL_CHECK(err, err = kernel.setArg(27, dst_height_host));
    OCL_CHECK(err, err = kernel.setArg(28, dst_width_host));
    OCL_CHECK(err, err = kernel.setArg(29, buffer_resizedOutput_duplicate_2));

    OCL_CHECK(err, err = ocl_cmdq.enqueueWriteBuffer(buffer_inImage1, CL_TRUE, 0, image_in_size_bytes, img[0].data));
    OCL_CHECK(err, err = ocl_cmdq.enqueueWriteBuffer(buffer_inImage2, CL_TRUE, 0, image_in_size_bytes, img[1].data));
    OCL_CHECK(err, err = ocl_cmdq.enqueueWriteBuffer(buffer_inImage3, CL_TRUE, 0, image_in_size_bytes, img[2].data));
    OCL_CHECK(err, err = ocl_cmdq.enqueueWriteBuffer(buffer_inImage4, CL_TRUE, 0, image_in_size_bytes, img[3].data));
    OCL_CHECK(err,
              err = ocl_cmdq.enqueueWriteBuffer(buffer_mapx1, CL_TRUE, 0, mapx_bytes[0], map_x_merged_int[0].data));
    OCL_CHECK(err,
              err = ocl_cmdq.enqueueWriteBuffer(buffer_mapy1, CL_TRUE, 0, mapy_bytes[0], map_y_merged_int[0].data));
    OCL_CHECK(err,
              err = ocl_cmdq.enqueueWriteBuffer(buffer_mapx2, CL_TRUE, 0, mapx_bytes[1], map_x_merged_int[1].data));
    OCL_CHECK(err,
              err = ocl_cmdq.enqueueWriteBuffer(buffer_mapy2, CL_TRUE, 0, mapy_bytes[1], map_y_merged_int[1].data));
    OCL_CHECK(err,
              err = ocl_cmdq.enqueueWriteBuffer(buffer_mapx3, CL_TRUE, 0, mapx_bytes[2], map_x_merged_int[2].data));
    OCL_CHECK(err,
              err = ocl_cmdq.enqueueWriteBuffer(buffer_mapy3, CL_TRUE, 0, mapy_bytes[2], map_y_merged_int[2].data));
    OCL_CHECK(err,
              err = ocl_cmdq.enqueueWriteBuffer(buffer_mapx4, CL_TRUE, 0, mapx_bytes[3], map_x_merged_int[3].data));
    OCL_CHECK(err,
              err = ocl_cmdq.enqueueWriteBuffer(buffer_mapy4, CL_TRUE, 0, mapy_bytes[3], map_y_merged_int[3].data));
    OCL_CHECK(err, err = ocl_cmdq.enqueueWriteBuffer(buffer_img_sizes, CL_TRUE, 0, 8 * sizeof(int), img_sizes));
    OCL_CHECK(err,
              err = ocl_cmdq.enqueueWriteBuffer(buffer_mask_corners, CL_TRUE, 0, 8 * sizeof(int), mask_corners_host));
    OCL_CHECK(err,
              err = ocl_cmdq.enqueueWriteBuffer(buffer_mask1, CL_TRUE, 0, mask_in_size_bytes[0], mask_img[0].data));
    OCL_CHECK(err,
              err = ocl_cmdq.enqueueWriteBuffer(buffer_mask2, CL_TRUE, 0, mask_in_size_bytes[1], mask_img[1].data));
    OCL_CHECK(err,
              err = ocl_cmdq.enqueueWriteBuffer(buffer_mask3, CL_TRUE, 0, mask_in_size_bytes[2], mask_img[2].data));
    OCL_CHECK(err,
              err = ocl_cmdq.enqueueWriteBuffer(buffer_mask4, CL_TRUE, 0, mask_in_size_bytes[3], mask_img[3].data));

    cl_ulong start = 0;
    cl_ulong end = 0;
    double diff_prof = 0.0f;
    cl::Event event;
    std::cout << "INFO: Launching AVFIRS_4stream_accel..." << std::endl;
    OCL_CHECK(err, err = ocl_cmdq.enqueueTask(kernel, NULL, &event));
    clWaitForEvents(1, (const cl_event*)&event);
    event.getProfilingInfo(CL_PROFILING_COMMAND_START, &start);
    event.getProfilingInfo(CL_PROFILING_COMMAND_END, &end);
    diff_prof = (end - start) / 1000000;
    std::cout << "INFO: Kernel wall (profiling) ~ " << diff_prof << " ms" << std::endl;

    OCL_CHECK(err, err = ocl_cmdq.enqueueReadBuffer(buffer_outPoints, CL_TRUE, 0, image_out_points_size_bytes, points));
    OCL_CHECK(err,
              err = ocl_cmdq.enqueueReadBuffer(buffer_outOffsets, CL_TRUE, 0, image_out_offsets_size_bytes, offsets));
    OCL_CHECK(err,
              err = ocl_cmdq.enqueueReadBuffer(buffer_numContours, CL_TRUE, 0, image_num_contours_size_bytes, numc));
    OCL_CHECK(err, err = ocl_cmdq.enqueueReadBuffer(buffer_resizedOutput_duplicate_2, CL_TRUE, 0, image_size_bytes,
                                                    resizedOutput_duplicate_2.data));
    OCL_CHECK(err, err = ocl_cmdq.finish());

    // Compare OpenCV (ocv) and OpenCL (ocl) resized panorama output
    cv::imwrite("ocv_resized_panorama.jpg", resized_panorama);                   // OCV result
    cv::imwrite("ocl_resizedOutput_duplicate_2.jpg", resizedOutput_duplicate_2); // OCL result

    // Optionally compute and save difference image for visual inspection
    // cv::Mat ocv_vs_ocl_diff;
    // cv::absdiff(resized_panorama, resizedOutput_duplicate_2, ocv_vs_ocl_diff);
    // cv::imwrite("diff_ocv_vs_ocl.jpg", ocv_vs_ocl_diff);
    // imwrite("diff.png", diff);

    std::vector<std::vector<cv::Point> > hlscontours;
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
        hlscontours.push_back(std::move(contour_pts));
        std::cout << "\n";
    }

    cv::Mat hlsContourOutput(in_rows, in_cols, CV_8UC1);
    hlsContourOutput.setTo(cv::Scalar(255));
    drawContours(hlsContourOutput, hlscontours, -1, cv::Scalar(0, 255, 0), 0); // Green contours
    imwrite("hls_contours.png", hlsContourOutput);

    cv::absdiff(hlsContourOutput, refContourOutput, diff);
    imwrite("diff.png", diff);
    return 0;
}
