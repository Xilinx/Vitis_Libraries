/*
 * Copyright (C) 2019-2022, Xilinx, Inc.
 * Copyright (C) 2022-2023, Advanced Micro Devices, Inc.
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
#include "xf_stereo_pipeline_ref.h"
#include "xf_stereo_pipeline_tb_config.h"
#include "cameraParameters.h"

#include "xcl2.hpp"

#define _PROFILE_ 0

// One disparity level (x16) tolerance for fixed-point subpixel differences per pixel.
#define DISP_DIFF_THRESH 16
// Maximum allowed percentage of ROI pixels above DISP_DIFF_THRESH.
#define DISP_ERR_PER_THRESH 2.0f

using namespace std;

static void clamp_invalid_disparity(cv::Mat& disp) {
    for (int i = 0; i < disp.rows; i++) {
        for (int j = 0; j < disp.cols; j++) {
            short v = (short)disp.at<unsigned short>(i, j);
            if (v < 0) {
                disp.at<unsigned short>(i, j) = 0;
            }
        }
    }
}

static float analyze_diff(const cv::Mat& diff, int thresh) {
    int cnt = 0;
    int valid_pixels = diff.rows * diff.cols;
    double minval = 1e9;
    double maxval = 0.0;

    for (int r = 0; r < diff.rows; r++) {
        for (int c = 0; c < diff.cols; c++) {
            unsigned short v = diff.at<unsigned short>(r, c);
            if ((int)v > thresh) {
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
    std::cout << "\tMinimum error in disparity = " << minval << std::endl;
    std::cout << "\tMaximum error in disparity = " << maxval << std::endl;
    std::cout << "\tPercentage of pixels above error threshold = " << err_per << std::endl;
    return err_per;
}

static void setup_bm_state_arr(int* bm_state_arr) {
    bm_state_arr[0] = XF_STEREO_PREFILTER_SOBEL_TYPE;
    bm_state_arr[1] = SAD_WINDOW_SIZE;
    bm_state_arr[2] = 31;
    bm_state_arr[3] = SAD_WINDOW_SIZE;
    bm_state_arr[4] = 0;
    bm_state_arr[5] = NO_OF_DISPARITIES;
    bm_state_arr[6] = 20;
    bm_state_arr[7] = 15;
    bm_state_arr[8] = PARALLEL_UNITS;
    bm_state_arr[9] = (NO_OF_DISPARITIES / PARALLEL_UNITS) + ((NO_OF_DISPARITIES % PARALLEL_UNITS) != 0);
    bm_state_arr[10] = PARALLEL_UNITS * bm_state_arr[9] - NO_OF_DISPARITIES;
}

int main(int argc, char** argv) {
    cv::setUseOptimized(false);

    if (argc != 3) {
        fprintf(stderr, "Invalid Number of Arguments!\nUsage: <executable> <left image> <right image>\n");
        return -1;
    }

    cv::Mat left_img, right_img;
    left_img = cv::imread(argv[1], 0);
    if (left_img.data == NULL) {
        fprintf(stderr, "Cannot open left image at %s\n", argv[1]);
        return 0;
    }
    right_img = cv::imread(argv[2], 0);
    if (right_img.data == NULL) {
        fprintf(stderr, "Cannot open right image at %s\n", argv[2]);
        return 0;
    }

    //////////////////	HLS TOP Function Call  ////////////////////////
    int rows = left_img.rows;
    int cols = left_img.cols;
    std::cout << "Input image height : " << rows << std::endl;
    std::cout << "Input image width  : " << cols << std::endl;

    cv::Mat disp_img(rows, cols, CV_16UC1);
    cv::Mat ref_disp_img(rows, cols, CV_16UC1);

    // allocate mem for camera parameters for rectification and bm_state class
    float* cameraMA_l_fl = (float*)malloc(XF_CAMERA_MATRIX_SIZE * sizeof(float));
    float* cameraMA_r_fl = (float*)malloc(XF_CAMERA_MATRIX_SIZE * sizeof(float));
    float* irA_l_fl = (float*)malloc(XF_CAMERA_MATRIX_SIZE * sizeof(float));
    float* irA_r_fl = (float*)malloc(XF_CAMERA_MATRIX_SIZE * sizeof(float));
    float* distC_l_fl = (float*)malloc(XF_DIST_COEFF_SIZE * sizeof(float));
    float* distC_r_fl = (float*)malloc(XF_DIST_COEFF_SIZE * sizeof(float));
    int* bm_state_arr = (int*)malloc(11 * sizeof(int));

    setup_bm_state_arr(bm_state_arr);

    // copy camera params
    for (int i = 0; i < XF_CAMERA_MATRIX_SIZE; i++) {
        cameraMA_l_fl[i] = (float)cameraMA_l[i];
        cameraMA_r_fl[i] = (float)cameraMA_r[i];
        irA_l_fl[i] = (float)irA_l[i];
        irA_r_fl[i] = (float)irA_r[i];
    }

    // copy distortion coefficients
    for (int i = 0; i < XF_DIST_COEFF_SIZE; i++) {
        distC_l_fl[i] = (float)distC_l[i];
        distC_r_fl[i] = (float)distC_r[i];
    }

    std::cout << "INFO: Running software reference." << std::endl;
    stereopipeline_ref(left_img, right_img, ref_disp_img, cameraMA_l_fl, cameraMA_r_fl, distC_l_fl, distC_r_fl,
                       irA_l_fl, irA_r_fl, XF_DIST_COEFF_SIZE, bm_state_arr);

    /////////////////////////////////////// CL ////////////////////////
    cl_int err;
    std::cout << "INFO: Running OpenCL section." << std::endl;

    // Context, command queue and device name:
    std::vector<cl::Device> devices = xcl::get_xil_devices();
    cl::Device device = devices[0];
    OCL_CHECK(err, cl::Context context(device, NULL, NULL, NULL, &err));
    OCL_CHECK(err, cl::CommandQueue q(context, device, CL_QUEUE_PROFILING_ENABLE, &err));
    OCL_CHECK(err, std::string device_name = device.getInfo<CL_DEVICE_NAME>(&err));
    std::cout << "INFO: Device found - " << device_name << std::endl;
    std::cout << "Input Image Bit Depth:" << XF_DTPIXELDEPTH(XF_8UC1, XF_NPPCX) << std::endl;
    std::cout << "Input Image Channels:" << XF_CHANNELS(XF_8UC1, XF_NPPCX) << std::endl;
    std::cout << "NPPC:" << XF_NPPCX << std::endl;

    // Load binary:
    std::string binaryFile = xcl::find_binary_file(device_name, "krnl_stereopipeline");
    cl::Program::Binaries bins = xcl::import_binary_file(binaryFile);
    devices.resize(1);
    OCL_CHECK(err, cl::Program program(context, devices, bins, NULL, &err));

    // Create a kernel:
    OCL_CHECK(err, cl::Kernel krnl(program, "stereopipeline_accel", &err));

    // Allocate the buffers:
    OCL_CHECK(err, cl::Buffer imageToDeviceL(context, CL_MEM_READ_ONLY, rows * cols, NULL, &err));
    OCL_CHECK(err, cl::Buffer imageToDeviceR(context, CL_MEM_READ_ONLY, rows * cols, NULL, &err));
    OCL_CHECK(err, cl::Buffer imageFromDevice(context, CL_MEM_WRITE_ONLY, rows * cols * 2, NULL, &err));
    OCL_CHECK(err,
              cl::Buffer arrToDeviceCML(context, CL_MEM_READ_ONLY, sizeof(float) * XF_CAMERA_MATRIX_SIZE, NULL, &err));
    OCL_CHECK(err,
              cl::Buffer arrToDeviceCMR(context, CL_MEM_READ_ONLY, sizeof(float) * XF_CAMERA_MATRIX_SIZE, NULL, &err));
    OCL_CHECK(err,
              cl::Buffer arrToDeviceDCL(context, CL_MEM_READ_ONLY, sizeof(float) * XF_DIST_COEFF_SIZE, NULL, &err));
    OCL_CHECK(err,
              cl::Buffer arrToDeviceDCR(context, CL_MEM_READ_ONLY, sizeof(float) * XF_DIST_COEFF_SIZE, NULL, &err));
    OCL_CHECK(err,
              cl::Buffer arrToDeviceRAL(context, CL_MEM_READ_ONLY, sizeof(float) * XF_CAMERA_MATRIX_SIZE, NULL, &err));
    OCL_CHECK(err,
              cl::Buffer arrToDeviceRAR(context, CL_MEM_READ_ONLY, sizeof(float) * XF_CAMERA_MATRIX_SIZE, NULL, &err));
    OCL_CHECK(err, cl::Buffer structToDevicesbmstate(context, CL_MEM_READ_ONLY, sizeof(int) * 11, NULL, &err));

    // Set the kernel arguments
    OCL_CHECK(err, err = krnl.setArg(0, imageToDeviceL));
    OCL_CHECK(err, err = krnl.setArg(1, imageToDeviceR));
    OCL_CHECK(err, err = krnl.setArg(2, imageFromDevice));
    OCL_CHECK(err, err = krnl.setArg(3, arrToDeviceCML));
    OCL_CHECK(err, err = krnl.setArg(4, arrToDeviceCMR));
    OCL_CHECK(err, err = krnl.setArg(5, arrToDeviceDCL));
    OCL_CHECK(err, err = krnl.setArg(6, arrToDeviceDCR));
    OCL_CHECK(err, err = krnl.setArg(7, arrToDeviceRAL));
    OCL_CHECK(err, err = krnl.setArg(8, arrToDeviceRAR));
    OCL_CHECK(err, err = krnl.setArg(9, structToDevicesbmstate));
    OCL_CHECK(err, err = krnl.setArg(10, rows));
    OCL_CHECK(err, err = krnl.setArg(11, cols));

    // Copying input data to Device buffer from host memory
    OCL_CHECK(err, q.enqueueWriteBuffer(imageToDeviceL, CL_TRUE, 0, rows * cols, left_img.data));
    OCL_CHECK(err, q.enqueueWriteBuffer(imageToDeviceR, CL_TRUE, 0, rows * cols, right_img.data));
    OCL_CHECK(err,
              q.enqueueWriteBuffer(arrToDeviceCML, CL_TRUE, 0, sizeof(float) * XF_CAMERA_MATRIX_SIZE, cameraMA_l_fl));
    OCL_CHECK(err,
              q.enqueueWriteBuffer(arrToDeviceCMR, CL_TRUE, 0, sizeof(float) * XF_CAMERA_MATRIX_SIZE, cameraMA_r_fl));
    OCL_CHECK(err, q.enqueueWriteBuffer(arrToDeviceDCL, CL_TRUE, 0, sizeof(float) * XF_DIST_COEFF_SIZE, distC_l_fl));
    OCL_CHECK(err, q.enqueueWriteBuffer(arrToDeviceDCR, CL_TRUE, 0, sizeof(float) * XF_DIST_COEFF_SIZE, distC_r_fl));
    OCL_CHECK(err, q.enqueueWriteBuffer(arrToDeviceRAL, CL_TRUE, 0, sizeof(float) * XF_CAMERA_MATRIX_SIZE, irA_l_fl));
    OCL_CHECK(err, q.enqueueWriteBuffer(arrToDeviceRAR, CL_TRUE, 0, sizeof(float) * XF_CAMERA_MATRIX_SIZE, irA_r_fl));
    OCL_CHECK(err, q.enqueueWriteBuffer(structToDevicesbmstate, CL_TRUE, 0, sizeof(int) * 11, bm_state_arr));

    // Profiling Objects
    cl_ulong start = 0;
    cl_ulong end = 0;
    double diff_prof = 0.0f;
    cl::Event event_sp;

    // Launch the kernel
    OCL_CHECK(err, err = q.enqueueTask(krnl, NULL, &event_sp));

    // profiling
    clWaitForEvents(1, (const cl_event*)&event_sp);
#if _PROFILE_
    event_sp.getProfilingInfo(CL_PROFILING_COMMAND_START, &start);
    event_sp.getProfilingInfo(CL_PROFILING_COMMAND_END, &end);
    diff_prof = end - start;
    std::cout << (diff_prof / 1000000) << "ms" << std::endl;
#endif

    // Copying Device result data to Host memory
    OCL_CHECK(err, q.enqueueReadBuffer(imageFromDevice, CL_TRUE, 0, rows * cols * 2, disp_img.data));

    q.finish();
    /////////////////////////////////////// end of CL ////////////////////////

    clamp_invalid_disparity(ref_disp_img);
    clamp_invalid_disparity(disp_img);

    cv::Mat diff;
    cv::absdiff(ref_disp_img, disp_img, diff);

    cv::Mat diff_c((diff.rows - (SAD_WINDOW_SIZE << 1)), (diff.cols - (SAD_WINDOW_SIZE << 1)), CV_16UC1);
    cv::Rect roi(SAD_WINDOW_SIZE, SAD_WINDOW_SIZE, diff.cols - (SAD_WINDOW_SIZE << 1),
                 diff.rows - (SAD_WINDOW_SIZE << 1));
    diff(roi).copyTo(diff_c);

    // Write output images
    cv::Mat ref_out_disp(rows, cols, CV_8UC1);
    cv::Mat hls_out_disp(rows, cols, CV_8UC1);
    ref_disp_img.convertTo(ref_out_disp, CV_8U, (256.0 / NO_OF_DISPARITIES) / (16.));
    disp_img.convertTo(hls_out_disp, CV_8U, (256.0 / NO_OF_DISPARITIES) / (16.));
    cv::imwrite("ref_output.png", ref_out_disp);
    cv::imwrite("hls_output.png", hls_out_disp);
    cv::Mat diff8;
    diff.convertTo(diff8, CV_8U, (256.0 / NO_OF_DISPARITIES) / (16.));
    cv::imwrite("diff_img.png", diff8);

    ////////  FUNCTIONAL VALIDATION  ////////
    float err_per = analyze_diff(diff_c, DISP_DIFF_THRESH);
    int ret = 0;
    if (err_per > DISP_ERR_PER_THRESH) {
        fprintf(stderr, "ERROR: Test Failed.\n");
        ret = 1;
    } else {
        std::cout << "Test Passed" << std::endl;
    }

    free(cameraMA_l_fl);
    free(cameraMA_r_fl);
    free(irA_l_fl);
    free(irA_r_fl);
    free(distC_l_fl);
    free(distC_r_fl);
    free(bm_state_arr);

    printf("run complete !\n");
    return ret;
}
