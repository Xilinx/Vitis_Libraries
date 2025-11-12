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
#include "xf_reprojectimageto3d_tb_config.h"
#include "xcl2.hpp"
#include <time.h>
#include <fstream>
using namespace std;

#define _TEXTURE_THRESHOLD_ 20
#define _UNIQUENESS_RATIO_ 15
#define _PRE_FILTER_CAP_ 31
#define _MIN_DISP_ 0

#define FLT_MAX1 3.402823466e+38F
#define FLT_EPSILON1 1.192092896e-07F
void cref_reprojectimage(cv::Mat& disp, cv::Mat& xyz, short int* q, int16_t min_disp, bool hand_mis_val = false) {
    short int res_16[4];
    long long int res_32[4];
    short int Q_val[4][4];
    for (int i = 0, p = 0; i < 16; i += 4, p++) {
        Q_val[p][0] = q[i];
        Q_val[p][1] = q[i + 1];
        Q_val[p][2] = q[i + 2];
        Q_val[p][3] = q[i + 3];
    }
    for (int i = 0; i < disp.rows; i++) {
        for (int j = 0; j < disp.cols; j++) {
            if (disp.at<short int>(i, j) == 0) {
                xyz.at<cv::Vec3s>(i, j)[0] = 0;
                xyz.at<cv::Vec3s>(i, j)[1] = 0;
                xyz.at<cv::Vec3s>(i, j)[2] = -1;
            } else {
                for (int k = 0; k < 4; k++) {
                    long long int temp = (Q_val[k][0] * j) + (Q_val[k][1] * i) +
                                         (Q_val[k][2] * disp.at<short int>(i, j)) + (Q_val[k][3] * 1);
                    res_32[k] = (temp);
                }
                xyz.at<cv::Vec3s>(i, j)[0] = (short int)std::round(res_32[0] / res_32[3]);
                xyz.at<cv::Vec3s>(i, j)[1] = (short int)std::round(res_32[1] / res_32[3]);
                xyz.at<cv::Vec3s>(i, j)[2] = (short int)std::round(res_32[2] / res_32[3]);
            }
            if (hand_mis_val) {
                if (std::abs(disp.at<short int>(i, j) - min_disp) <= 0) {
                    xyz.at<cv::Vec3s>(i, j)[2] = 10000;
                }
            }
        }
    }
}
int16_t float2fixed(const double x, const uint8_t fractional_bits) {
    return int16_t(round(x * (1 << fractional_bits)));
}

int main(int argc, char** argv) {
    cv::setUseOptimized(false);

    printf("entered into main\n");
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <INPUT IMAGE PATH 1> <INPUT IMAGE PATH 2>\n", argv[0]);
        return EXIT_FAILURE;
    }

    cv::Mat left_img, right_img;

    // Reading in the images: Only Grayscale image
    left_img = cv::imread(argv[1], 0);
    right_img = cv::imread(argv[2], 0);

    if (left_img.data == NULL) {
        fprintf(stderr, "ERROR: Cannot open image %s\n ", argv[1]);
        return EXIT_FAILURE;
    }

    if (right_img.data == NULL) {
        fprintf(stderr, "ERROR: Cannot open image %s\n ", argv[2]);
        return EXIT_FAILURE;
    }

    uint16_t rows = left_img.rows;
    uint16_t cols = left_img.cols;

    cv::Mat disp, hls_disp;
    // reference code to generate disparity image
    cv::Ptr<cv::StereoBM> stereobm = cv::StereoBM::create(NO_OF_DISPARITIES, SAD_WINDOW_SIZE);
    stereobm->setPreFilterCap(_PRE_FILTER_CAP_);
    stereobm->setUniquenessRatio(_UNIQUENESS_RATIO_);
    stereobm->setTextureThreshold(_TEXTURE_THRESHOLD_);
    stereobm->compute(left_img, right_img, disp); // 16bit disp vals
    cv::Mat disp_s(rows, cols, CV_16SC1);
    for (int i = 0; i < disp.rows; i++) {
        for (int j = 0; j < disp.cols; j++) {
            if (disp.at<short>(i, j) < 0) {
                disp_s.at<unsigned short>(i, j) = 0;
            } else
                disp_s.at<unsigned short>(i, j) = (unsigned short)disp.at<short>(i, j);
        }
    }

    cv::Mat disp8;
    disp.convertTo(disp8, CV_8U, (256.0 / NO_OF_DISPARITIES) / (16.));
    cv::imwrite("disp.png", disp);
    cv::imwrite("disparity8bit.png", disp8);
    // end of disparity image creation

    // cv reference to 3d point creation
    cv::Mat XYZ(disp_s.size(), CV_16SC3);
    // finding Q matrix using rectify
    // double q[16]={1, 0, 0, -0.676, 0, 1, 0, -0.375, 0, 0, 0, 0.1345, 0, 0, 0, -1.675};
    double q[16] = {1.0, 0.0, 0.0, -609.5593338, 0.0, 1.0, 0.0,        -172.85400391,
                    0.0, 0.0, 0.0, 721.53771973, 0.0, 0.0, -1.8771874, 0.0};
    cv::Mat Q(4, 4, CV_64F, q);
    cv::reprojectImageTo3D(disp, XYZ, Q, true, CV_16SC3);

    short int* q_fixed = (short int*)malloc(16 * sizeof(short int));
    for (int i = 0; i < 16; i++) {
        q_fixed[i] = float2fixed(q[i], 5); // q(10.6)
    }
    double minDisparity = FLT_MAX1;
    cv::minMaxIdx(disp, &minDisparity, 0, 0, 0);
    short int min_disp = (short int)minDisparity;
    printf("min_disp=%d\n", min_disp);

    // c-reference
    cv::Mat XYZ_c(disp_s.size(), CV_16SC3);
    cref_reprojectimage(disp, XYZ_c, q_fixed, min_disp, true);

    FILE* fp1 = fopen("XYZ.txt", "w");
    FILE* fp2 = fopen("XYZ_c.txt", "w");

    FILE* fpx = fopen("absdiff.txt", "w");

    for (int i = 0; i < disp.rows; i++) {
        for (int j = 0; j < disp.cols; j++) {
            fprintf(fp1, "%d ", XYZ.at<cv::Vec3s>(i, j)[0]);
            fprintf(fp1, "%d ", XYZ.at<cv::Vec3s>(i, j)[1]);
            fprintf(fp1, "%d\n", XYZ.at<cv::Vec3s>(i, j)[2]);

            fprintf(fp2, "%d ", XYZ_c.at<cv::Vec3s>(i, j)[0]);
            fprintf(fp2, "%d ", XYZ_c.at<cv::Vec3s>(i, j)[1]);
            fprintf(fp2, "%d\n", XYZ_c.at<cv::Vec3s>(i, j)[2]);
        }
    }
    fclose(fp1);
    fclose(fp2);
    fclose(fpx);
    cv::Mat diff_cv(XYZ_c.rows, XYZ_c.cols, CV_16SC3);
    cv::absdiff(XYZ_c, XYZ, diff_cv);

    float err_per;
    xf::cv::analyzeDiff(diff_cv, 1, err_per);

    if (err_per > 0.0f) {
        fprintf(stderr, "ERROR: Test Failed.\n ");
    }

    // Creating host memory for the hw acceleration
    cv::Mat hls_xyz(rows, cols, CV_16SC3);
    /* std::vector<signed short> q_f(16);
     for(int i=0;i<16;i++)
         {
                 q_f[i]=q_fixed[i];
         }*/
    // OpenCL section:

    //    reprojectimageto3d_accel((ap_uint<INPUT_PTR_WIDTH>*)disp.data, (ap_uint<OUTPUT_PTR_WIDTH>*)hls_xyz.data,
    //    q_fixed, min_disp,  rows, cols, true);

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
    std::string binaryFile = xcl::find_binary_file(device_name, "krnl_reprojectimageto3d");
    cl::Program::Binaries bins = xcl::import_binary_file(binaryFile);
    devices.resize(1);
    OCL_CHECK(err, cl::Program program(context, devices, bins, NULL, &err));

    // Create a kernel:
    OCL_CHECK(err, cl::Kernel kernel(program, "reprojectimageto3d_accel", &err));

    // Allocate the buffers:
    OCL_CHECK(err, cl::Buffer buffer_inImage(context, CL_MEM_READ_ONLY, (disp.rows * disp.cols * 2), NULL, &err));
    OCL_CHECK(err, cl::Buffer buffer_inVecQ(context, CL_MEM_READ_ONLY, (16 * 2), NULL, &err));
    OCL_CHECK(err, cl::Buffer buffer_outImage(context, CL_MEM_WRITE_ONLY, (disp.rows * disp.cols * 2 * 3), NULL, &err));

    // Set kernel arguments:
    OCL_CHECK(err, err = kernel.setArg(0, buffer_inImage));
    OCL_CHECK(err, err = kernel.setArg(1, buffer_outImage));
    OCL_CHECK(err, err = kernel.setArg(2, buffer_inVecQ));
    OCL_CHECK(err, err = kernel.setArg(3, min_disp));
    OCL_CHECK(err, err = kernel.setArg(4, rows));
    OCL_CHECK(err, err = kernel.setArg(5, cols));
    OCL_CHECK(err, err = kernel.setArg(6, true));

    // Initialize the buffers:
    cl::Event event;

    OCL_CHECK(err, queue.enqueueWriteBuffer(buffer_inImage,              // buffer on the FPGA
                                            CL_TRUE,                     // blocking call
                                            0,                           // buffer offset in bytes
                                            (disp.rows * disp.cols * 2), // Size in bytes
                                            disp.data                    // Pointer to the data to copy
                                            ));

    OCL_CHECK(err, queue.enqueueWriteBuffer(buffer_inVecQ, // buffer on the FPGA
                                            CL_TRUE,       // blocking call
                                            0,             // buffer offset in bytes
                                            (16 * 2),      // Size in bytes
                                            q_fixed        // Pointer to the data to copy
                                            ));

    // Profiling Objects
    cl_ulong start = 0;
    cl_ulong end = 0;
    double diff_prof = 0.0f;

    // Execute the kernel:
    OCL_CHECK(err, err = queue.enqueueTask(kernel, NULL, &event));
    clWaitForEvents(1, (const cl_event*)&event);

    event.getProfilingInfo(CL_PROFILING_COMMAND_START, &start);
    event.getProfilingInfo(CL_PROFILING_COMMAND_END, &end);
    diff_prof = end - start;
    std::cout << (diff_prof / 1000000) << "ms" << std::endl;

    // Copy Result from Device Global Memory to Host Local Memory
    OCL_CHECK(err, queue.enqueueReadBuffer(buffer_outImage, // This buffers data will be read
                                           CL_TRUE,         // blocking call
                                           0,               // offset
                                           (disp.rows * disp.cols * 2 * 3),
                                           hls_xyz.data // Data will be stored here
                                           ));

    // Clean up:
    queue.finish();

    ////////  FUNCTIONAL VALIDATION  ////////
    // changing the invalid value from negative to zero for validating the
    // difference

    FILE* fp3 = fopen("XYZ_hls.txt", "w");

    for (int i = 0; i < disp_s.rows; i++) {
        for (int j = 0; j < disp_s.cols; j++) {
            fprintf(fp3, "%d ", hls_xyz.at<cv::Vec3s>(i, j)[0]);
            fprintf(fp3, "%d ", hls_xyz.at<cv::Vec3s>(i, j)[1]);
            fprintf(fp3, "%d\n", hls_xyz.at<cv::Vec3s>(i, j)[2]);
        }
    }
    fclose(fp3);

    cv::Mat diff;
    diff.create(left_img.rows, left_img.cols, CV_16SC3);
    cv::absdiff(XYZ, hls_xyz, diff);

    cv::imwrite("hls_disp.jpg", hls_xyz);
    cv::imwrite("diff_img.jpg", diff);

    // removing border before diff analysis
    cv::Mat diff_c;
    diff_c.create((diff.rows - SAD_WINDOW_SIZE << 1), diff.cols - (SAD_WINDOW_SIZE << 1), CV_16SC3);
    cv::Rect roi;
    roi.x = SAD_WINDOW_SIZE;
    roi.y = SAD_WINDOW_SIZE;
    roi.width = diff.cols - (SAD_WINDOW_SIZE << 1);
    roi.height = diff.rows - (SAD_WINDOW_SIZE << 1);
    diff_c = diff(roi);

    float err_per1;
    xf::cv::analyzeDiff(diff_c, 1, err_per1);

    if (err_per1 > 0.0f) {
        fprintf(stderr, "ERROR: Test Failed.\n ");
        return 1;
    } else
        std::cout << "Test Passed " << std::endl;
    return 0;
}
