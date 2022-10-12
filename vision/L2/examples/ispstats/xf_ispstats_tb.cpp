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
#include "xf_ispstats_config.h"
#include "xcl2.hpp"

int main(int argc, char** argv) {
    if (argc != 3) { // BGR and BAYER can be run without changing Makefile
        fprintf(stderr, "Usage: %s <INPUT IMAGE PATH 1> <INPUT IMAGE PATH 2> \n", argv[0]);
        return EXIT_FAILURE;
    }
    cv::Mat in_img;
    unsigned int final_bins[FINAL_BINS_NUM] = {63, 127, 191, 255};
    // This means the
    // merged_bin[0] = bin[  0 to  63]
    // merged_bin[1] = bin[ 64 to 127]
    // merged_bin[2] = bin[127 to 191]
    // merged_bin[3] = bin[192 to 255]

    int roi_tlx = 0;
    int roi_tly = 0;
    int roi_brx = 127;
    int roi_bry = 127;
    int stats_size = 256;
    int N = 2;
    int M = 2;

    // N = 0 or M = 0 is not possible so assigning it to 1 if value is 0.
    if (N == 0) {
        N = 1;
    }
    if (M == 0) {
        M = 1;
    }

    uint32_t ind_ref_stats[N * M][3][256] = {0};
    uint32_t mrg_ref_stats[N * M][3][FINAL_BINS_NUM] = {0};

    int zone_width = int((roi_brx - roi_tlx + 1) / N);  // roi_width / N
    int zone_height = int((roi_bry - roi_tly + 1) / M); // roi_height / M

#if BGR
    // Read image
    in_img = cv::imread(argv[1], 1);
    if (in_img.data == NULL) {
        fprintf(stderr, "Cannot open image %s\n", argv[1]);
        return 0;
    }

    // Get histogram per ROI zones
    for (int row = 0; row < in_img.rows; row++) {
        for (int col = 0; col < in_img.cols; col++) {
            // Get data from in_img
            cv::Vec3b bgr = in_img.at<cv::Vec3b>(row, col);

            // Count if inside ROI
            if ((row >= roi_tlx) && (row <= (roi_brx)) && (col >= roi_tly) && (col <= (roi_bry))) {
                int zone_col = int((col - roi_tlx) / zone_width);
                int zone_row = int((row - roi_tly) / zone_height);
                int zone_idx = (zone_row * N) + zone_col;

                // Extract per channel
                for (int ch = 0; ch < 3; ch++) {
                    uint8_t val = 0;
                    val = bgr[ch];

                    uint32_t tmpval = ind_ref_stats[zone_idx][ch][val];
                    ind_ref_stats[zone_idx][ch][val] = tmpval + 1;
                }
            }
        }
    }
#endif

#if BAYER
    in_img = cv::imread(argv[2], 0); // bayer_pattern, 0: Gray

    if (in_img.data == NULL) {
        fprintf(stderr, "Cannot open image %s\n", argv[2]);
        return 0;
    }

    uint8_t kr = 0, kb = 0, kg = 0;

    for (int row = 0; row < in_img.rows; row++) {
        for (int col = 0; col < in_img.cols; col = col + 2) {
            if (row % 2 == 0) // EVEN ROW
            {
                if ((row >= roi_tlx) && (row <= roi_brx) && (col >= roi_tly) && (col <= roi_bry)) {
                    int zone_col = int((row - roi_tlx) / zone_width);
                    int zone_row = int((col - roi_tly) / zone_height);
                    int zone_idx = (zone_row * N) + zone_col;

                    kr = in_img.at<uchar>(row, col);
                    ind_ref_stats[zone_idx][0][kr] = ind_ref_stats[zone_idx][0][kr] + 1;

                    kg = in_img.at<uchar>(row, col + 1);
                    ind_ref_stats[zone_idx][1][kg] = ind_ref_stats[zone_idx][1][kg] + 1;
                }
            } else // ODD ROW (i%2!=0)
            {
                if ((row >= roi_tlx) && (row <= roi_brx) && (col >= roi_tly) && (col <= roi_bry)) {
                    int zone_col = int((row - roi_tlx) / zone_width);
                    int zone_row = int((col - roi_tly) / zone_height);
                    int zone_idx = (zone_row * N) + zone_col;

                    kg = in_img.at<uchar>(row, col);
                    ind_ref_stats[zone_idx][1][kg] = ind_ref_stats[zone_idx][1][kg] + 1;

                    kb = in_img.at<uchar>(row, col + 1);
                    ind_ref_stats[zone_idx][2][kb] = ind_ref_stats[zone_idx][2][kb] + 1;
                }
            }
        }
    }
#endif

    int num_ch = 3;
    int num_zones = N * M;
#if MERGE_BINS
    for (int zone_idx = 0; zone_idx < num_zones; zone_idx++) {
        for (int ch = 0; ch < num_ch; ch++) {
            uint32_t hi_bin = final_bins[0];
            uint32_t bin_group = 0;

            for (int i = 0; i < stats_size; i++) {
                uint32_t tmp_val = ind_ref_stats[zone_idx][ch][i];

                mrg_ref_stats[zone_idx][ch][bin_group] += tmp_val;

                if ((uint32_t)i == hi_bin) {
                    bin_group = bin_group + 1;
                    hi_bin = *(final_bins + bin_group);
                }
            }
        }
    }
#endif

    // Create a memory to hold HLS implementation output:
    std::vector<uint32_t> hlsstats(stats_size * 3 * MAX_ROWS * MAX_COLS);
#if MERGE_BINS
    size_t image_out_size_bytes = N * M * 3 * FINAL_BINS_NUM * sizeof(uint32_t);
#else
    size_t image_out_size_bytes = N * M * 3 * stats_size * sizeof(uint32_t);
#endif
    size_t image_in_size_bytes = in_img.rows * in_img.cols * 3 * sizeof(unsigned char);
    size_t bins_size_bytes = FINAL_BINS_NUM * 3 * sizeof(unsigned int);

    int rows = in_img.rows;
    int cols = in_img.cols;
    std::cout << "Input image height : " << rows << std::endl;
    std::cout << "Input image width  : " << cols << std::endl;

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
    std::cout << "Input Image Bit Depth:" << XF_DTPIXELDEPTH(TYPE, NPC1) << std::endl;
    std::cout << "Input Image Channels:" << XF_CHANNELS(TYPE, NPC1) << std::endl;
    std::cout << "NPPC:" << NPC1 << std::endl;

    // Load binary:
    std::string binaryFile = xcl::find_binary_file(device_name, "krnl_ispstats");
    cl::Program::Binaries bins = xcl::import_binary_file(binaryFile);
    devices.resize(1);
    OCL_CHECK(err, cl::Program program(context, devices, bins, NULL, &err));

    // Create a kernel:
    OCL_CHECK(err, cl::Kernel kernel(program, "ispstats_accel", &err));

    // Allocate the buffers:
    OCL_CHECK(err, cl::Buffer buffer_inImage(context, CL_MEM_READ_ONLY, image_in_size_bytes, NULL, &err));
    OCL_CHECK(err, cl::Buffer buffer_outImage(context, CL_MEM_WRITE_ONLY, image_out_size_bytes, NULL, &err));
    OCL_CHECK(err, cl::Buffer buffer_inBins(context, CL_MEM_READ_ONLY, bins_size_bytes, NULL, &err));

    // Set kernel arguments:
    OCL_CHECK(err, err = kernel.setArg(0, buffer_inImage));
    OCL_CHECK(err, err = kernel.setArg(1, buffer_outImage));
    OCL_CHECK(err, err = kernel.setArg(2, buffer_inBins));
    OCL_CHECK(err, err = kernel.setArg(3, rows));
    OCL_CHECK(err, err = kernel.setArg(4, cols));
    OCL_CHECK(err, err = kernel.setArg(5, roi_tlx));
    OCL_CHECK(err, err = kernel.setArg(6, roi_tly));
    OCL_CHECK(err, err = kernel.setArg(7, roi_brx));
    OCL_CHECK(err, err = kernel.setArg(8, roi_bry));
    OCL_CHECK(err, err = kernel.setArg(9, N));
    OCL_CHECK(err, err = kernel.setArg(10, M));

    // Initialize the buffers:
    cl::Event event;

    OCL_CHECK(err, queue.enqueueWriteBuffer(buffer_inImage,      // buffer on the FPGA
                                            CL_TRUE,             // blocking call
                                            0,                   // buffer offset in bytes
                                            image_in_size_bytes, // Size in bytes
                                            in_img.data,         // Pointer to the data to copy
                                            nullptr, &event));

    OCL_CHECK(err, queue.enqueueWriteBuffer(buffer_inBins,   // buffer on the FPGA
                                            CL_TRUE,         // blocking call
                                            0,               // buffer offset in bytes
                                            bins_size_bytes, // Size in bytes
                                            final_bins,      // Pointer to the data to copy
                                            nullptr, &event));

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
    std::cout << (diff_prof / 1000000) << "ms" << std::endl;

    // Copy Result from Device Global Memory to Host Local Memory
    queue.enqueueReadBuffer(buffer_outImage, // This buffers data will be read
                            CL_TRUE,         // blocking call
                            0,               // offset
                            image_out_size_bytes,
                            hlsstats.data(), // Data will be stored here
                            nullptr, &event);

    // Clean up:
    queue.finish();

    FILE *out_hls, *out_ref;
    out_hls = fopen("out_hls.txt", "w");
    out_ref = fopen("out_ref.txt", "w");

#if MERGE_BINS
    stats_size = FINAL_BINS_NUM;
#else
    stats_size = 256;
#endif
    // Total number of bins for all channels
    int total_bins = stats_size * 3;

    // Checking of output vs reference
    for (int zone = 0; zone < N * M; zone++) {
        fprintf(out_ref, "zone %d\n", zone);
        fprintf(out_hls, "zone %d\n", zone);
        for (int cnt = 0; cnt < stats_size; cnt++) {
            uint32_t bstat_hls = hlsstats[(zone * total_bins) + cnt];
            uint32_t gstat_hls = hlsstats[(zone * total_bins) + cnt + stats_size];
            uint32_t rstat_hls = hlsstats[(zone * total_bins) + cnt + stats_size * 2];

#if MERGE_BINS
            uint32_t bstat_ref = mrg_ref_stats[zone][0][cnt];
            uint32_t gstat_ref = mrg_ref_stats[zone][1][cnt];
            uint32_t rstat_ref = mrg_ref_stats[zone][2][cnt];
#else
            uint32_t bstat_ref = ind_ref_stats[zone][0][cnt];
            uint32_t gstat_ref = ind_ref_stats[zone][1][cnt];
            uint32_t rstat_ref = ind_ref_stats[zone][2][cnt];
#endif

            // Reference
            fprintf(out_ref, "bin%03d %u %u %u\n", cnt, bstat_ref, gstat_ref, rstat_ref);
            // HLS Output
            fprintf(out_hls, "bin%03d %u %u %u\n", cnt, bstat_hls, gstat_hls, rstat_hls);

            // Data checker
            if ((bstat_ref != bstat_hls) || (gstat_ref != gstat_hls) || (gstat_ref != gstat_hls)) {
                fprintf(stderr, "\nTest Failed\n");
                return 1;
            }
        }
    }

    fclose(out_hls);
    fclose(out_ref);

    std::cout << "Test Passed " << std::endl;

    return 0;
}
