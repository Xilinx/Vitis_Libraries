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

#include <dirent.h>

#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "common/xf_headers.hpp"
#include "xf_threshold_config.h"

#include "xcl2.hpp"
#include <unistd.h>
#include <time.h>
#include <vector>
#include <iostream>
#include "xf_opencl_wrap.hpp"
#include "opencv2/features2d.hpp"

#define ENABLE_INERMEDIATE_STORE 1
#define ENABLE_DEBUG_LOG 1
#define CONSTANT_OTSU_GRAY 13
#define READ_y8 0
#define READ_COLOR_IMG 1
#define CONSTANT_GREEN_PLANE 10
#define IN_IMG_DEPTH 3

using namespace cv;
using namespace std;

std::vector<std::string> loadFolderContents(const std::string& folderName, bool doSort);

int main(int argc, char** argv) {
    if (argc > 3) {
        fprintf(stderr, "Invalid Number of Arguments!\nUsage:\n");
        fprintf(stderr, "<Executable Name> <input image path> \n");
        return -1;
    }

    cv::Mat in_img, out_img, ocv_ref, in_gray, diff;
    cv::Mat preout_img;
    cv::Mat gray_image;
    cv::Mat input_rgb_image;

    clock_t tstart = 0, tend = 0, threshold_time = 0, gaus_time = 0, contour_time = 0;
    clock_t temp = 0, temp2 = 0, findcountour = 0, drawcontour = 0, median2 = 0;
    clock_t countnzero = 0;

    unsigned short in_width, in_height, in_depth;
    size_t data_out_size_bytes = sizeof(unsigned char);
    uint8_t Otsuval;

#if READ_y8
    FILE* fp_y8;

    fp_y8 = fopen(argv[1], "r");
    char data_y8[1024000];
    if (fp_y8) {
        fread(data_y8, 1, 1024000, fp_y8);
        fclose(fp_y8);
    }
    in_img.create(800, 1280, CV_8UC1);
    in_img.data = (unsigned char*)data_y8;

    out_img.create(in_img.rows, in_img.cols, CV_8UC1);
    preout_img.create(in_img.rows, in_img.cols, CV_8UC1);

#endif

#if READ_COLOR_IMG
    /*  reading in the color image  */
    // in_img = cv::imread(argv[1], cv::IMREAD_GRAYSCALE);
    in_img = cv::imread(argv[1], cv::IMREAD_ANYCOLOR | cv::IMREAD_ANYDEPTH);
    if (in_img.data == NULL) {
        fprintf(stderr, "Cannot open image at %s\n", argv[1]);
        return 0;
    }

    in_width = in_img.cols;
    in_height = in_img.rows;
    in_depth = in_img.depth();

    fprintf(stderr, "row = %d col = %d depth = %d\n", in_height, in_width, in_depth);

    ocv_ref.create(in_img.rows, in_img.cols, in_img.depth());
    out_img.create(in_img.rows, in_img.cols, CV_8UC1);
    preout_img.create(in_img.rows, in_img.cols, CV_8UC1);
    diff.create(in_img.rows, in_img.cols, in_img.depth());

////////////////  reference code  ////////////////
#endif

    unsigned char thresh;
    unsigned char maxval = 255;

    /* OpenCV threshold for comparision */
    cv::threshold(in_img, ocv_ref, thresh, maxval, cv::THRESH_BINARY);
    if (ocv_ref.data == NULL) {
        fprintf(stderr, "threshold ocv_ref is NULL\n");
        return 0;
    }
    //////////////////  end opencv reference code//////////

    /////////////////////////////////////// CL ////////////////////////

    int height = in_img.rows;
    int width = in_img.cols;
    float sigma = 0.0;

    cl_int err;
    std::cout << "\nINFO: Running OpenCL section.\n" << std::endl;

    std::vector<cl::Device> devices = xcl::get_xil_devices();
    cl::Device device = devices[0];
    OCL_CHECK(err, cl::Context context(device, NULL, NULL, NULL, &err));
    OCL_CHECK(err, cl::CommandQueue q(context, device, CL_QUEUE_PROFILING_ENABLE, &err));

    std::string device_name = device.getInfo<CL_DEVICE_NAME>();
    std::string binaryFile = xcl::find_binary_file(device_name, "krnl_threshold_xo");
    cl::Program::Binaries bins = xcl::import_binary_file(binaryFile);
    devices.resize(1);

    std::cout << "\nkernel loaded\n" << std::endl;

    OCL_CHECK(err, cl::Program program(context, devices, bins, NULL, &err));

    /* ************************* Create a Gaussian_OTSU Accel ******************************** */
    tstart = clock();

    OCL_CHECK(err, cl::Kernel kernel(program, "gaussian_otsu_accel", &err));

    std::cout << "\ngaussian_otsu_accel created\n" << std::endl;

    std::vector<cl::Memory> inBufVec1, outBufVec1;
    /* Input image is RGB */
    OCL_CHECK(err, cl::Buffer imageToGaus(context, CL_MEM_READ_ONLY, (height * width * IN_IMG_DEPTH), NULL, &err));
    OCL_CHECK(err, cl::Buffer imageFromGaus(context, CL_MEM_READ_WRITE, (height * width), NULL, &err));

    // Out buffer for Otsu value
    OCL_CHECK(err, cl::Buffer buffer_outData(context, CL_MEM_WRITE_ONLY, data_out_size_bytes, NULL, &err));

    // Set the kernel arguments
    OCL_CHECK(err, err = kernel.setArg(0, imageToGaus));
    OCL_CHECK(err, err = kernel.setArg(1, imageFromGaus));
    OCL_CHECK(err, err = kernel.setArg(2, height));
    OCL_CHECK(err, err = kernel.setArg(3, width));
    OCL_CHECK(err, err = kernel.setArg(4, sigma));
    OCL_CHECK(err, err = kernel.setArg(5, buffer_outData));

    // Initialize the buffers:
    cl::Event event;
    // Profiling Objects
    cl_ulong start = 0;
    cl_ulong end = 0;
    double diff_prof = 0.0f;

    OCL_CHECK(err, cl::Kernel krnl(program, "preprocess_accel", &err));
    OCL_CHECK(err, cl::Buffer imageToDevice(context, CL_MEM_READ_ONLY, (height * width), NULL, &err));
    OCL_CHECK(err, cl::Buffer imageFromDevice(context, CL_MEM_READ_WRITE, (height * width), NULL, &err));

    // Profiling Objects
    cl_ulong th_start = 0;
    cl_ulong th_end = 0;
    double th_diff_prof = 0.0f;
    cl::Event event_sp, cca_event_sp;
    unsigned char* tmp_out_data1 = (unsigned char*)malloc(height * width);
    unsigned char* tmp_out_data2 = (unsigned char*)malloc(height * width);
    cv::Mat cca_outimg;
    cca_outimg.create(in_img.rows, in_img.cols, in_img.type());
    int def_pix, obj_pix;

    double time_taken;

    OCL_CHECK(err, cl::Kernel cca_krnl(program, "cca_custom_accel", &err));
    OCL_CHECK(err, cl::Buffer cca_imageToDevice(context, CL_MEM_READ_ONLY, (height * width), NULL, &err));
    OCL_CHECK(err, cl::Buffer cca_tempbuffer(context, CL_MEM_READ_ONLY, (height * width), NULL, &err));
    OCL_CHECK(err, cl::Buffer cca_tempbuffer_2(context, CL_MEM_READ_ONLY, (height * width), NULL, &err));
    OCL_CHECK(err, cl::Buffer cca_imageFromDevice(context, CL_MEM_READ_WRITE, (height * width), NULL, &err));
    OCL_CHECK(err, cl::Buffer obj_pix_buffer(context, CL_MEM_READ_WRITE, 4, NULL, &err));
    OCL_CHECK(err, cl::Buffer def_pix_buffer(context, CL_MEM_READ_WRITE, (height * width), NULL, &err));

    // Set the kernel arguments
    OCL_CHECK(err, err = cca_krnl.setArg(0, cca_imageToDevice));
    OCL_CHECK(err, err = cca_krnl.setArg(1, cca_imageToDevice));
    OCL_CHECK(err, err = cca_krnl.setArg(2, cca_tempbuffer));
    OCL_CHECK(err, err = cca_krnl.setArg(3, cca_tempbuffer_2));
    OCL_CHECK(err, err = cca_krnl.setArg(4, cca_imageFromDevice));
    OCL_CHECK(err, err = cca_krnl.setArg(5, obj_pix_buffer));
    OCL_CHECK(err, err = cca_krnl.setArg(6, def_pix_buffer));
    OCL_CHECK(err, err = cca_krnl.setArg(7, height));
    OCL_CHECK(err, err = cca_krnl.setArg(8, width));

    OCL_CHECK(err, q.enqueueWriteBuffer(imageToGaus,                     // buffer on the FPGA
                                        CL_TRUE,                         // blocking call
                                        0,                               // buffer offset in bytes
                                        (height * width * IN_IMG_DEPTH), // Size in bytes
                                        in_img.data,                     // Pointer to the data to copy
                                        nullptr, &event));

#if ENABLE_DEBUG_LOG
    std::cout << "\nGaussian going to enqueue the task " << std::endl;
#endif

    // Execute the kernel:
    OCL_CHECK(err, err = q.enqueueTask(kernel));

    clWaitForEvents(1, (const cl_event*)&event);

    temp = clock();
    gaus_time = temp - tstart;

#if ENABLE_DEBUG_LOG
    std::cout << "\nGaussian got event " << std::endl;
#endif

    event.getProfilingInfo(CL_PROFILING_COMMAND_START, &start);
    event.getProfilingInfo(CL_PROFILING_COMMAND_END, &end);
    diff_prof = end - start;
    std::cout << (diff_prof / 1000000) << "Gaussian OTSU took ms" << std::endl;

    // Copy Result from Device Global Memory to Host Local Memory
    /*		q.enqueueReadBuffer(imageFromGaus, // This buffers data will be read
                                                            CL_TRUE,         // blocking call
                                                            0,               // offset
                                                            (height * width),
                                                            out_img.data, // Data will be stored here
                                                            nullptr, &event);
    */
    std::cout << "\n read gaussian output img\n" << std::endl;

    // Copy Result from Device Global Memory to Host Local Memory
    q.enqueueReadBuffer(buffer_outData, // This buffers data will be read
                        CL_TRUE,        // blocking call
                        0,              // offset
                        data_out_size_bytes,
                        &Otsuval, // Data will be stored here
                        nullptr, &event);

    std::cout << "INFO: Gaussian OTSU done." << std::endl;
    //	imwrite("gaussian.jpg", out_img);

    //	q.finish();
    /* ****************************** End of Gaussian_otsu_accel kernel ******************************** */

    /* ************************* Create a threshold kernel ******************************** */
    temp2 = clock();
    printf("OTSU Val = %d\n", Otsuval);
    thresh = Otsuval - CONSTANT_OTSU_GRAY;

#if ENABLE_DEBUG_LOG
    std::cout << "\npre-process accel arg loaded\n" << std::endl;
#endif

    // Set the pre-process kernel arguments
    OCL_CHECK(err, err = krnl.setArg(0, imageToDevice));
    OCL_CHECK(err, err = krnl.setArg(1, imageFromDevice));
    OCL_CHECK(err, err = krnl.setArg(2, thresh));
    OCL_CHECK(err, err = krnl.setArg(3, maxval));
    OCL_CHECK(err, err = krnl.setArg(4, height));
    OCL_CHECK(err, err = krnl.setArg(5, width));

    OCL_CHECK(err, q.enqueueWriteBuffer(imageToDevice, CL_TRUE, 0, (height * width), out_img.data));

#if ENABLE_DEBUG_LOG
    fprintf(stderr, "pre-process job submitted\n");
#endif

    // Launch the kernel
    OCL_CHECK(err, err = q.enqueueTask(krnl, NULL, &event_sp));
    clWaitForEvents(1, (const cl_event*)&event_sp);

    temp = clock();
    threshold_time = temp - temp2;

    event_sp.getProfilingInfo(CL_PROFILING_COMMAND_START, &th_start);
    event_sp.getProfilingInfo(CL_PROFILING_COMMAND_END, &th_end);
    th_diff_prof = th_end - th_start;
    std::cout << (th_diff_prof / 1000000) << "pre-process took ms" << std::endl;

    // Copying Device result data to Host memory
    q.enqueueReadBuffer(imageFromDevice, CL_TRUE, 0, (height * width), preout_img.data, nullptr, &event_sp);

#if ENABLE_DEBUG_LOG
    std::cout << "INFO: pre-process done." << std::endl;
#endif

#if ENABLE_INERMEDIATE_STORE
    imwrite("preprocess.jpg", preout_img);
#endif

    q.finish();

    /*
    (void)cl_kernel_mgr::registerKernel("cca_custom_accel", "defect-detect", XCLIN(out_img),
                                                                            XCLIN(out_img), XCLIN(tmp_out_data1, height
    * width),
                                                                            XCLIN(tmp_out_data2, height * width),
    XCLOUT(cca_outimg), XCLOUT(&obj_pix, 4),
                                                                            XCLOUT(&def_pix, 4), XCLIN(height),
    XCLIN(width));

    cl_kernel_mgr::exec_all(); */

    OCL_CHECK(err, q.enqueueWriteBuffer(cca_imageToDevice, CL_TRUE, 0, (height * width), preout_img.data));

    OCL_CHECK(err, err = q.enqueueTask(cca_krnl, NULL, &cca_event_sp));
    clWaitForEvents(1, (const cl_event*)&cca_event_sp);

    q.enqueueReadBuffer(cca_imageFromDevice, CL_TRUE, 0, (height * width), cca_outimg.data);
    q.enqueueReadBuffer(obj_pix_buffer, CL_TRUE, 0, 4, &obj_pix);
    q.enqueueReadBuffer(def_pix_buffer, CL_TRUE, 0, 4, &def_pix);

    printf("Mango Pixel = %d Defect Pixel = %d\n", obj_pix, def_pix);

    cv::imwrite("Defect_image.png", cca_outimg);

    tend = clock() - tstart;

    time_taken = ((double)tend) / CLOCKS_PER_SEC; // in seconds

    //    printf("defect detection xfopenCV took %lf seconds to execute \n", time_taken);

    time_taken = ((double)gaus_time) / CLOCKS_PER_SEC; // in seconds
    printf(" Gaussian OTSU took = %lf \n ", time_taken);

    time_taken = ((double)threshold_time) / CLOCKS_PER_SEC; // in seconds
    printf("Time Taken by threshold = %lf \n ", time_taken);

    printf("\n====================== End of defect detection ==============================\n\n");

    q.finish();
    /////////////////////////////////////// end of CL ////////////////////////

    // Compute absolute difference image
    absdiff(ocv_ref, out_img, diff);

    return 0;
}

std::vector<std::string> loadFolderContents(const std::string& folderName, bool doSort = true) {
    std::vector<std::string> fileNames;

    DIR* directoryHandle;

    if ((directoryHandle = opendir(folderName.c_str())) != nullptr) {
        struct dirent* directoryEntry;
        while ((directoryEntry = readdir(directoryHandle)) != nullptr) {
            std::string fileName = directoryEntry->d_name;
            if ((fileName == ".") || (fileName == "..") || (fileName.size() == 0) || (fileName[0] == '.')) {
                continue;
            }
            fileNames.push_back(folderName + "/" + fileName);
        }
        closedir(directoryHandle);
    }

    if (doSort) {
        std::sort(fileNames.begin(), fileNames.end());
    }

    return fileNames;
}
