/*
 * Copyright 2019 Xilinx, Inc.
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
#include "xf_pp_pipeline_config.h"

#include <sys/time.h>

#include "xcl2.hpp"

int main(int argc, char* argv[]) {
    struct timeval start_pp_sw, end_pp_sw;
    double lat_pp_sw = 0.0f;
    cv::Mat img, result_hls, result_ocv, error;

    img = cv::imread(argv[1], 1);
    if (!img.data) {
        fprintf(stderr, "\n image not found");
        return -1;
    }
    int in_width, in_height;
    int out_width, out_height;

    in_width = img.cols;
    in_height = img.rows;

    out_height = 224;
    out_width = 224;

    result_hls.create(cv::Size(224, 224), CV_16SC3);

    // Mean values
    float params[9];
    params[3] = params[4] = params[5] = 0.0;
    params[0] = 104.007f;
    params[1] = 116.669f;
    params[2] = 122.679f;
    int th1 = 255, th2 = 255;
    int act_img_h, act_img_w;

    /////////////////////////////////////// CL ///////////////////////////////////////
    std::vector<cl::Device> devices = xcl::get_xil_devices();
    fprintf(stderr, "\nDevices fethced");
    cl::Device device = devices[0];
    fprintf(stderr, "\nCreating context");
    cl::Context context(device);
    fprintf(stderr, "\nConetxt created");

    std::string device_name = device.getInfo<CL_DEVICE_NAME>();
    std::string binaryFile = xcl::find_binary_file(device_name, "krnl_pp_pipeline_accel");
    cl::Program::Binaries bins = xcl::import_binary_file(binaryFile);

    devices.resize(1);

    cl_device_info info;
    cl_int errr;

    fprintf(stderr, "\nCreating program");
    cl::Program program(context, devices, bins);
    fprintf(stderr, "\nprogram created");
    cl::Kernel krnl(program, "pp_pipeline_accel");

    cl::CommandQueue q(context, device, CL_QUEUE_PROFILING_ENABLE);
    std::vector<cl::Memory> inBufVec, outBufVec, paramasbufvec;
    cl::Buffer imageToDevice(context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY, in_height * in_width * 3, img.data);
    cl::Buffer imageFromDevice(context, CL_MEM_USE_HOST_PTR | CL_MEM_WRITE_ONLY, out_height * out_width * 3 * 2,
                               result_hls.data);
    cl::Buffer paramsbuf(context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY, 9 * 4, params);

    inBufVec.push_back(imageToDevice);
    outBufVec.push_back(imageFromDevice);
    paramasbufvec.push_back(paramsbuf);

    krnl.setArg(0, imageToDevice);
    krnl.setArg(1, imageFromDevice);
    krnl.setArg(2, in_height);
    krnl.setArg(3, in_width);
    krnl.setArg(4, out_height);
    krnl.setArg(5, out_width);
    krnl.setArg(6, paramsbuf);
    krnl.setArg(7, th1);
    krnl.setArg(8, th2);

    fprintf(stderr, "\nArguments Set\n");

    q.enqueueMigrateMemObjects(inBufVec, 0 /* 0 means from host*/);
    q.enqueueMigrateMemObjects(paramasbufvec, 0 /* 0 means from host*/);

    // Profiling Objects
    cl_ulong start = 0;
    cl_ulong end = 0;
    double diff_prof = 0.0f;
    cl::Event event_sp;

    // Launch the kernel

    q.enqueueTask(krnl, NULL, &event_sp);
    clWaitForEvents(1, (const cl_event*)&event_sp);

    q.enqueueMigrateMemObjects(outBufVec, CL_MIGRATE_MEM_OBJECT_HOST);

    event_sp.getProfilingInfo(CL_PROFILING_COMMAND_START, &start);
    event_sp.getProfilingInfo(CL_PROFILING_COMMAND_END, &end);
    diff_prof = end - start;
    std::cout << "HW Kernel latency = " << (diff_prof / 1000000) << "ms" << std::endl;

    q.finish();

    /////////////////////////////////////// end of CL ///////////////////////////////////////
    /*Reference Implementation*/

    gettimeofday(&start_pp_sw, 0);
    cv::resize(img, result_ocv, cv::Size(224, 224));
    uchar* img_ocv_data = result_ocv.data;
    int frame_cntr1 = 0;
    float* data_ptr_cv = (float*)malloc(224 * 224 * 3 * sizeof(float));
    int idx = 0;
    float* dst1_cv = &data_ptr_cv[0];
    float* dst2_cv = &data_ptr_cv[224 * 224];
    float* dst3_cv = &data_ptr_cv[(3 - 1) * 224 * 224];
    for (int ll_rows = 0; ll_rows < 224; ll_rows++) {
        for (int ll_cols = 0; ll_cols < 224; ll_cols++) {
            dst1_cv[idx] = (float)img_ocv_data[frame_cntr1++] - params[0];
            dst2_cv[idx] = (float)img_ocv_data[frame_cntr1++] - params[1];
            dst3_cv[idx] = (float)img_ocv_data[frame_cntr1++] - params[2];
            idx++;
        }
    }
    gettimeofday(&end_pp_sw, 0);

    lat_pp_sw = (end_pp_sw.tv_sec * 1e6 + end_pp_sw.tv_usec) - (start_pp_sw.tv_sec * 1e6 + start_pp_sw.tv_usec);
    std::cout << "\n\n Software pre-processing latency " << lat_pp_sw / 1000 << "ms" << std::endl;

    // Error Checking
    int frame_cntr = 0;

    float* data_ptr = (float*)malloc(224 * 224 * 3 * sizeof(float));
    float* dst1 = &data_ptr[0];
    float* dst2 = &data_ptr[224 * 224];
    float* dst3 = &data_ptr[(3 - 1) * 224 * 224];

    short* img_data = (short*)result_hls.data;

    float max_error1 = 0.0, max_error2 = 0.0, max_error3 = 0.0;
    int idx1 = 0;
    for (int l_rows = 0; l_rows < 224; l_rows++) {
        for (int l_cols = 0; l_cols < 224; l_cols++) {
            dst1[idx1] = (float)img_data[frame_cntr++] / 128.0f;
            dst2[idx1] = (float)img_data[frame_cntr++] / 128.0f;
            dst3[idx1] = (float)img_data[frame_cntr++] / 128.0f;

            float err1 = fabs(dst1[idx1] - dst1_cv[idx1]);
            float err2 = fabs(dst2[idx1] - dst2_cv[idx1]);
            float err3 = fabs(dst3[idx1] - dst3_cv[idx1]);
            if (err1 > max_error1) max_error1 = err1;
            if (err2 > max_error2) max_error2 = err2;
            if (err3 > max_error3) max_error3 = err3;
            idx1++;

        } // l_cols
    }     // l_rows

    if (max_error1 > 2 || max_error2 > 2 || max_error3 > 2) {
        fprintf(stderr, "\n Test Failed\n");
        return -1;

    } else {
        fprintf(stderr, "\n Test Passed\n");
        return 0;
    }
}
