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
#include "xf_crop_pipeline_tb_config.h"
#include "xcl2.hpp"
#include "xf_opencl_wrap.hpp"

#define ERROR_THRESHOLD 2

#define Y2R_f 1.164
#define U2R_f 0
#define V2R_f 1.596
#define Y2G_f 1.164
#define U2G_f -0.391
#define V2G_f -0.813
#define Y2B_f 1.164
#define U2B_f 2.018
#define V2B_f 0
void yuv4442rgb_ref(cv::Mat& yuvImage, cv::Mat& rgb_out_ref) {
    // Step 3: Convert the image to YUV color space and pack into rgb format
    for (int y = 0; y < yuvImage.rows; y++) {
        for (int x = 0; x < yuvImage.cols; x += 1) {
            // R = 1.164*Y + 1.596*V = Y + 0.164*Y + V + 0.596*V
            // G = 1.164*Y - 0.813*V - 0.391*U = Y + 0.164*Y - 0.813*V - 0.391*U
            // B = 1.164*Y + 2.018*U = Y + 0.164 + 2*U + 0.018*U

            // Calculate R values
            cv::Vec3b yuvPixel1 = yuvImage.at<cv::Vec3b>(y, x);

            int8_t u_val = (int8_t)(yuvPixel1[1] - 128);
            int8_t v_val = (int8_t)(yuvPixel1[2] - 128);

            double R_double = ((1.164 * yuvPixel1[0]) + (1.596 * v_val));
            int R = std::floor(R_double);
            unsigned char r_value = cv::saturate_cast<unsigned char>(R);
            // Calculate G values
            // cv::Vec3b yuvPixel1 = yuvImage.at<cv::Vec3b>(y, x);
            double G_double = ((1.164 * yuvPixel1[0]) - (0.813 * v_val) - (0.391 * u_val));
            int G = std::floor(G_double);
            unsigned char g_value = cv::saturate_cast<unsigned char>(G);
            // Calculate B values
            // cv::Vec3b yuvPixel1 = yuvImage.at<cv::Vec3b>(y, x);
            double B_double = ((1.164 * yuvPixel1[0]) + (2.018 * u_val));
            int B = std::floor(B_double);
            unsigned char b_value = cv::saturate_cast<unsigned char>(B);

            // Pack R, G, B values
            // rgb_out_ref.at<unsigned int>(y, x) = (unsigned int)((r_value << 16) | (g_value<<8) | b_value);
            rgb_out_ref.at<cv::Vec3b>(y, x)[0] = r_value;
            rgb_out_ref.at<cv::Vec3b>(y, x)[1] = g_value;
            rgb_out_ref.at<cv::Vec3b>(y, x)[2] = b_value;

            /*if(y==0 && x==0)
            {
                 printf("r_value=%d\g_value=%d\nb_value=%d\n", r_value, g_value, b_value);
                 printf("yuvPixel1[0]=%d\yuvPixel1[1]=%d\yuvPixel1[2]=%d\n", yuvPixel1[0], yuvPixel1[1], yuvPixel1[2]);
            }*/
        }
    }
}
void extract_y_uv(cv::Mat& YUV420, cv::Mat& y_img, cv::Mat& uv_img, int height, int width) {
    y_img = YUV420(cv::Range(0, height), cv::Range(0, width));

    cv::Mat outImage_Ueven_openCV(height / 4, width / 2, CV_8UC1);
    cv::Mat outImage_Uodd_openCV(height / 4, width / 2, CV_8UC1);
    cv::Mat outImage_Veven_openCV(height / 4, width / 2, CV_8UC1);
    cv::Mat outImage_Vodd_openCV(height / 4, width / 2, CV_8UC1);

    outImage_Ueven_openCV = YUV420(cv::Range(height, height + height / 4), cv::Range(0, width / 2));
    outImage_Uodd_openCV = YUV420(cv::Range(height, height + height / 4), cv::Range(width / 2, width));
    outImage_Veven_openCV = YUV420(cv::Range(height + height / 4, height + height / 2), cv::Range(0, width / 2));
    outImage_Vodd_openCV = YUV420(cv::Range(height + height / 4, height + height / 2), cv::Range(width / 2, width));

    int ii = 0;
    for (int i = 0; i < height / 4; i++) {
        int jj = 0;
        for (int j = 0; j < (width / 2); j = j + 1, jj += 2) {
            uv_img.at<uint8_t>(ii, jj) = outImage_Ueven_openCV.at<uint8_t>(i, j);
            uv_img.at<uint8_t>(ii, jj + 1) = outImage_Veven_openCV.at<uint8_t>(i, j);
        }

        jj = 0;
        ii = ii + 1;
        for (int j = 0; j < (width / 2); j = j + 1, jj += 2) {
            uv_img.at<uint8_t>(ii, jj) = outImage_Uodd_openCV.at<uint8_t>(i, j);
            uv_img.at<uint8_t>(ii, jj + 1) = outImage_Vodd_openCV.at<uint8_t>(i, j);
        }
        ii = ii + 1;
    }
}
cv::Mat convert8UC2To16UC1(const cv::Mat& src) {
    // Validate input type
    if (src.type() != CV_8UC2) {
        throw std::invalid_argument("Input image must be of type CV_8UC2");
    }
    // Create an output image of type CV_16SC1
    cv::Mat dst(src.rows, src.cols, CV_16UC1);
    // Split the source image into two channels
    std::vector<cv::Mat> channels(2);
    cv::split(src, channels);
    // Combine the two channels into a single 16-bit signed channel
    // Assuming channels[0] is the high byte and channels[1] is the low byte
    for (int i = 0; i < src.rows; ++i) {
        for (int j = 0; j < src.cols; ++j) {
            uint8_t u_b = channels[0].at<uint8_t>(i, j);
            uint8_t v_b = channels[1].at<uint8_t>(i, j);

            // Combine high byte and low byte into a single signed 16-bit value
            uint16_t value = static_cast<uint16_t>((v_b << 8) | u_b);
            // Assign to the output ]matrix
            dst.at<uint16_t>(i, j) = value;
        }
    }
    return dst;
}
/*
 * NV122RGB
 */
unsigned char saturate_cast(float Value) {
    unsigned char Value_uchar;
    if (Value > 255)
        Value_uchar = 255;
    else if (Value < 0)
        Value_uchar = 0;
    else
        Value_uchar = (unsigned char)floor(Value);
    return Value_uchar;
}
void NV122RGB(unsigned char* Y_ptr, unsigned char* UV_ptr, unsigned char* RGB_ptr, int Width, int Height, char CN) {
    int i, j, index, index_offset;
    int Y00, Y01, Y10, Y11, U, V;
    float V2Rtemp, U2Gtemp, V2Gtemp, U2Btemp;

    unsigned char *Yptr, *UVptr;
    int WidthStep = Width * CN;

    for (i = 0; i < (Height / 2); i++) {
        Yptr = &Y_ptr[2 * i * Width];
        UVptr = &UV_ptr[i * Width];

        index_offset = i * Width * CN * 2;
        for (j = 0; j < Width; j = j + 2) {
            index = index_offset + (j * CN);

            U = UVptr[j] - 128;
            V = UVptr[j + 1] - 128;

            Y00 = (Yptr[j] > 16) ? (Yptr[j] - 16) : 0;
            Y01 = (Yptr[j + 1] > 16) ? (Yptr[j + 1] - 16) : 0;
            Y10 = (Yptr[j + Width] > 16) ? (Yptr[j + Width] - 16) : 0;
            Y11 = (Yptr[j + Width + 1] > 16) ? (Yptr[j + Width + 1] - 16) : 0;

            V2Rtemp = V2R_f * V;
            U2Gtemp = U2G_f * U;
            V2Gtemp = V2G_f * V;
            U2Btemp = U2B_f * U;

            RGB_ptr[index] = saturate_cast(Y2R_f * Y00 + V2Rtemp + 0.5);
            RGB_ptr[index + 1] = saturate_cast(Y2G_f * Y00 + U2Gtemp + V2Gtemp + 0.5);
            RGB_ptr[index + 2] = saturate_cast(Y2B_f * Y00 + U2Btemp + 0.5);
            RGB_ptr[index + CN] = saturate_cast(Y2R_f * Y01 + V2Rtemp);
            RGB_ptr[index + CN + 1] = saturate_cast(Y2G_f * Y01 + U2Gtemp + V2Gtemp);
            RGB_ptr[index + CN + 2] = saturate_cast(Y2B_f * Y01 + U2Btemp);
            RGB_ptr[index + WidthStep] = saturate_cast(Y2R_f * Y10 + V2Rtemp);
            RGB_ptr[index + WidthStep + 1] = saturate_cast(Y2G_f * Y10 + U2Gtemp + V2Gtemp);
            RGB_ptr[index + WidthStep + 2] = saturate_cast(Y2B_f * Y10 + U2Btemp);
            RGB_ptr[index + WidthStep + CN] = saturate_cast(Y2R_f * Y11 + V2Rtemp);
            RGB_ptr[index + WidthStep + CN + 1] = saturate_cast(Y2G_f * Y11 + U2Gtemp + V2Gtemp);
            RGB_ptr[index + WidthStep + CN + 2] = saturate_cast(Y2B_f * Y11 + U2Btemp);
            if (CN == 4) {
                RGB_ptr[index + 3] = 255;
                RGB_ptr[index + CN + 3] = 255;
                RGB_ptr[index + WidthStep + 3] = 255;
                RGB_ptr[index + WidthStep + CN + 3] = 255;
            }
        }
    }
}
void cvtColor_RGB2YUY2(cv::Mat& src, cv::Mat& dst) {
    cv::Mat temp;
    cv::cvtColor(src, temp, 83);

    std::vector<uint8_t> v1;
    for (int i = 0; i < src.rows; i++) {
        for (int j = 0; j < src.cols; j++) {
            v1.push_back(temp.at<cv::Vec3b>(i, j)[0]);
            j % 2 ? v1.push_back(temp.at<cv::Vec3b>(i, j)[2]) : v1.push_back(temp.at<cv::Vec3b>(i, j)[1]);
        }
    }
    cv::Mat yuy2(src.rows, src.cols, CV_8UC2);
    memcpy(yuy2.data, v1.data(), src.cols * src.rows * 2);
    dst = yuy2;
}
void YUYV2RGB_ref(unsigned char* YUV_ptr, unsigned char* RGB_ptr, int Width, int Height, char CN) {
    int i, j, k, index, index_offset;
    int Y0, Y1, U, V;
    unsigned char* Yptr;
    float V2Rtemp, U2Gtemp, V2Gtemp, U2Btemp;
    printf("width=%d height=%d\n", Width, Height);
    for (i = 0; i < Height; i++) {
        Yptr = &YUV_ptr[i * Width * 2];
        index_offset = i * Width * CN;
        for (j = 0, k = 0; j < Width; j = j + 2, k = k + 4) {
            index = index_offset + (j * CN);

            Y0 = (Yptr[k] > 16) ? (Yptr[k] - 16) : 0; // Yptr[k] - 16;
            U = Yptr[k + 1] - 128;
            Y1 = (Yptr[k + 2] > 16) ? (Yptr[k + 2] - 16) : 0; // Yptr[k + 2] - 16;
            V = Yptr[k + 3] - 128;

            V2Rtemp = V2R_f * V;
            U2Gtemp = U2G_f * U;
            V2Gtemp = V2G_f * V;
            U2Btemp = U2B_f * U;

            RGB_ptr[index] = saturate_cast(Y2R_f * Y0 + V2Rtemp);
            RGB_ptr[index + 1] = saturate_cast(Y2G_f * Y0 + U2Gtemp + V2Gtemp);
            RGB_ptr[index + 2] = saturate_cast(Y2B_f * Y0 + U2Btemp);

            RGB_ptr[index + CN] = saturate_cast(Y2R_f * Y1 + V2Rtemp);
            RGB_ptr[index + CN + 1] = saturate_cast(Y2G_f * Y1 + U2Gtemp + V2Gtemp);
            RGB_ptr[index + CN + 2] = saturate_cast(Y2B_f * Y1 + U2Btemp);

            if (CN == 4) {
                RGB_ptr[index + 3] = 255;
                RGB_ptr[index + CN + 3] = 255;
            }
        }
    }
}

int main(int argc, char** argv) {
    cv::Mat imgInput0, imgInput1, imgInput2;
    cv::Mat refOutput0, refOutput1, refOutput2;
    cv::Mat errImg0, errImg1, errImg2;

#if YUV_420
    imgInput0 = cv::imread(argv[1], 1);
    if (!imgInput0.data) {
        fprintf(stderr, "Can't open image %s !!.\n ", argv[1]);
        return -1;
    }
    //	rgb2YUV420
    int width = imgInput0.cols;
    int height = imgInput0.rows;
    cv::Mat YUV420(height + height / 2, width, CV_8UC1);
    cvtColor(imgInput0, YUV420, cv::COLOR_RGBA2YUV_I420, 0);
    cv::Mat Y_Img(height, width, CV_8UC1);
    cv::Mat uv_Img(height / 2, width / 2, CV_8UC2);
    cv::Mat uv_Img_16(height / 2, width / 2, CV_16UC1);
    cv::Mat RGB_Img(height, width, CV_8UC3);
    extract_y_uv(YUV420, Y_Img, uv_Img, height, width);
    uv_Img_16 = convert8UC2To16UC1(uv_Img);
    ap_uint<8 * NPC1>* _imgInput0 = (ap_uint<8 * NPC1>*)Y_Img.data;
    ap_uint<16 * NPC2>* _imgInput1 = (ap_uint<16 * NPC2>*)uv_Img_16.data;
    NV122RGB(Y_Img.data, uv_Img.data, RGB_Img.data, width, height, 3);
    refOutput0 = RGB_Img;

    // crop reference
    unsigned int x_loc = 0;
    unsigned int y_loc = 0;
    unsigned int ROI_height = 480;
    unsigned int ROI_width = 320;
    cv::Mat ocv_ref, out_img, diff;
    ocv_ref.create(ROI_height, ROI_width, refOutput0.type());
    out_img.create(ROI_height, ROI_width, refOutput0.type());
    diff.create(ROI_height, ROI_width, refOutput0.type());
    cv::Rect ROI(x_loc, y_loc, ROI_width, ROI_height);
    ocv_ref = refOutput0(ROI);
    int* roi = (int*)malloc(4 * sizeof(int));
    for (int i = 0, j = 0; i < 4; j++, i += 4) {
        roi[i] = x_loc;
        roi[i + 1] = y_loc;
        roi[i + 2] = ROI_height;
        roi[i + 3] = ROI_width;
    }

    cl_int err;
    std::cout << "INFO: Running OpenCL section." << std::endl;
    // Get the device:
    std::vector<cl::Device> devices = xcl::get_xil_devices();
    cl::Device device = devices[0];

    // Context, command queue and device name:
    OCL_CHECK(err, cl::Context context(device, NULL, NULL, NULL, &err));
    OCL_CHECK(err, cl::CommandQueue q(context, device, CL_QUEUE_PROFILING_ENABLE, &err));
    OCL_CHECK(err, std::string device_name = device.getInfo<CL_DEVICE_NAME>(&err));

    std::cout << "INFO: Device found - " << device_name << std::endl;
    std::cout << "Input Image Bit Depth:" << XF_DTPIXELDEPTH(IN_TYPE, NPPCX) << std::endl;
    std::cout << "Input Image Channels:" << XF_CHANNELS(IN_TYPE, NPPCX) << std::endl;
    std::cout << "NPPC:" << NPPCX << std::endl;

    // Load binary:
    std::string binaryFile = xcl::find_binary_file(device_name, "krnl_crop_nv122rgb");
    cl::Program::Binaries bins = xcl::import_binary_file(binaryFile);
    devices.resize(1);
    OCL_CHECK(err, cl::Program program(context, devices, bins, NULL, &err));

    // Create a kernel:
    OCL_CHECK(err, cl::Kernel krnl(program, "crop_nv122rgb", &err));

    // Allocate the buffers:
    std::vector<cl::Memory> inBufVec, outBufVec;
    OCL_CHECK(err, cl::Buffer imageToDevice(context, CL_MEM_READ_ONLY, (Y_Img.rows * Y_Img.cols), NULL, &err));
    OCL_CHECK(err,
              cl::Buffer imageToDevice1(context, CL_MEM_READ_ONLY, (uv_Img_16.rows * uv_Img_16.cols * 2), NULL, &err));
    OCL_CHECK(err, cl::Buffer structToDeviceroi(context, CL_MEM_READ_ONLY, (4 * sizeof(int)), NULL, &err));

    OCL_CHECK(err, cl::Buffer imageFromDevice(context, CL_MEM_WRITE_ONLY, (ROI_height * ROI_width * 3), NULL, &err));

    // Set the kernel arguments
    OCL_CHECK(err, err = krnl.setArg(0, imageToDevice));
    OCL_CHECK(err, err = krnl.setArg(1, imageToDevice1));
    OCL_CHECK(err, err = krnl.setArg(2, imageFromDevice));
    OCL_CHECK(err, err = krnl.setArg(3, structToDeviceroi));
    OCL_CHECK(err, err = krnl.setArg(4, height));
    OCL_CHECK(err, err = krnl.setArg(5, width));

    /* Copy input vectors to memory */
    OCL_CHECK(err, q.enqueueWriteBuffer(imageToDevice,                         // buffer on the FPGA
                                        CL_TRUE,                               // blocking call
                                        0,                                     // buffer offset in bytes
                                        (Y_Img.rows * Y_Img.cols),             // Size in bytes
                                        Y_Img.data));                          // Pointer to the data to copy
    OCL_CHECK(err, q.enqueueWriteBuffer(imageToDevice1,                        // buffer on the FPGA
                                        CL_TRUE,                               // b/locking call
                                        0,                                     // buffer offset in bytes
                                        (uv_Img_16.rows * uv_Img_16.cols * 2), // Size in bytes
                                        uv_Img_16.data));                      // Pointer to the data to copy
    OCL_CHECK(err, q.enqueueWriteBuffer(structToDeviceroi,                     // buffer on the FPGA
                                        CL_TRUE,                               // blocking call
                                        0,                                     // buffer offset in bytes
                                        (4 * sizeof(int)),                     // Size in bytes
                                        roi));

    // Profiling Objects
    cl_ulong start = 0;
    cl_ulong end = 0;
    double diff_prof = 0.0f;
    cl::Event event_sp;

    // Execute the kernel:
    OCL_CHECK(err, err = q.enqueueTask(krnl, NULL, &event_sp));

    clWaitForEvents(1, (const cl_event*)&event_sp);

    event_sp.getProfilingInfo(CL_PROFILING_COMMAND_START, &start);
    event_sp.getProfilingInfo(CL_PROFILING_COMMAND_END, &end);
    diff_prof = end - start;
    std::cout << (diff_prof / 1000000) << "ms" << std::endl;

    // Copying Device result data to Host memory
    OCL_CHECK(err, q.enqueueReadBuffer(imageFromDevice, // This buffers data will be read
                                       CL_TRUE,         // blocking call
                                       0,               // offset
                                       (ROI_height * ROI_width * 3),
                                       out_img.data)); // Data will be stored here

    q.finish();
    /////////////////////////////////////// end of CL
    //////////////////////////////////////////

    cv::imwrite("hls_crop.jpg", out_img); // hls image
    cv::imwrite("ocv_crop.jpg", ocv_ref); // reference image
    cv::absdiff(ocv_ref, out_img, diff);
    cv::imwrite("diff_crop.jpg", diff); // Save the difference image for debugging purpose

#endif
#if YUV_422
    imgInput0 = cv::imread(argv[1], 1);
    if (!imgInput0.data) {
        fprintf(stderr, "Can't open image %s !!.\n ", argv[1]);
        return -1;
    }

    //	rgb2YUV422
    int width = imgInput0.cols;
    int height = imgInput0.rows;
    cv::Mat YUV422(height, width, CV_8UC2);
    cv::Mat yuyv_img(height, width, CV_16UC1);
    cv::Mat RGB_Img(height, width, CV_8UC3);
    cvtColor_RGB2YUY2(imgInput0, YUV422);
    yuyv_img = convert8UC2To16UC1(YUV422);
    cv::imwrite("yuyv_img.png", yuyv_img);
    printf("YUV422.rows=%d YUV422.cols=%d", YUV422.rows, YUV422.cols);
    YUYV2RGB_ref(yuyv_img.data, RGB_Img.data, (width), height, 3);
    cv::imwrite("RGB_Img.png", RGB_Img);
    refOutput0 = RGB_Img;

    // crop reference
    unsigned int x_loc = 0;
    unsigned int y_loc = 0;
    unsigned int ROI_height = 480;
    unsigned int ROI_width = 320;

    cv::Mat ocv_ref, out_img, diff;
    ocv_ref.create(ROI_height, ROI_width, refOutput0.type());
    out_img.create(ROI_height, ROI_width, refOutput0.type());
    diff.create(ROI_height, ROI_width, refOutput0.type());
    cv::Rect ROI(x_loc, y_loc, ROI_width, ROI_height);
    ocv_ref = refOutput0(ROI);
    int* roi = (int*)malloc(4 * sizeof(int));
    for (int i = 0, j = 0; i < 4; j++, i += 4) {
        roi[i] = x_loc;
        roi[i + 1] = y_loc;
        roi[i + 2] = ROI_height;
        roi[i + 3] = ROI_width;
    }

    cl_int err;
    std::cout << "INFO: Running OpenCL section." << std::endl;
    // Get the device:
    std::vector<cl::Device> devices = xcl::get_xil_devices();
    cl::Device device = devices[0];

    // Context, command queue and device name:
    OCL_CHECK(err, cl::Context context(device, NULL, NULL, NULL, &err));
    OCL_CHECK(err, cl::CommandQueue q(context, device, CL_QUEUE_PROFILING_ENABLE, &err));
    OCL_CHECK(err, std::string device_name = device.getInfo<CL_DEVICE_NAME>(&err));

    std::cout << "INFO: Device found - " << device_name << std::endl;
    std::cout << "Input Image Bit Depth:" << XF_DTPIXELDEPTH(IN_TYPE, NPPCX) << std::endl;
    std::cout << "Input Image Channels:" << XF_CHANNELS(IN_TYPE, NPPCX) << std::endl;
    std::cout << "NPPC:" << NPPCX << std::endl;

    // Load binary:
    std::string binaryFile = xcl::find_binary_file(device_name, "krnl_crop_yuyv2rgb");
    cl::Program::Binaries bins = xcl::import_binary_file(binaryFile);
    devices.resize(1);
    OCL_CHECK(err, cl::Program program(context, devices, bins, NULL, &err));

    // Create a kernel:
    OCL_CHECK(err, cl::Kernel krnl(program, "crop_yuyv2rgb", &err));

    // Allocate the buffers:
    std::vector<cl::Memory> inBufVec, outBufVec;
    OCL_CHECK(err,
              cl::Buffer imageToDevice(context, CL_MEM_READ_ONLY, (yuyv_img.rows * yuyv_img.cols * 2), NULL, &err));
    OCL_CHECK(err, cl::Buffer structToDeviceroi(context, CL_MEM_READ_ONLY, (4 * sizeof(int)), NULL, &err));

    OCL_CHECK(err, cl::Buffer imageFromDevice(context, CL_MEM_WRITE_ONLY, (ROI_height * ROI_width * 3), NULL, &err));

    // Set the kernel arguments
    OCL_CHECK(err, err = krnl.setArg(0, imageToDevice));
    OCL_CHECK(err, err = krnl.setArg(1, imageFromDevice));
    OCL_CHECK(err, err = krnl.setArg(2, structToDeviceroi));
    OCL_CHECK(err, err = krnl.setArg(3, height));
    OCL_CHECK(err, err = krnl.setArg(4, width));

    /* Copy input vectors to memory */
    OCL_CHECK(err, q.enqueueWriteBuffer(imageToDevice,                       // buffer on the FPGA
                                        CL_TRUE,                             // blocking call
                                        0,                                   // buffer offset in bytes
                                        (yuyv_img.rows * yuyv_img.cols * 2), // Size in bytes
                                        yuyv_img.data));                     // Pointer to the data to copy
    OCL_CHECK(err, q.enqueueWriteBuffer(structToDeviceroi,                   // buffer on the FPGA
                                        CL_TRUE,                             // blocking call
                                        0,                                   // buffer offset in bytes
                                        (4 * sizeof(int)),                   // Size in bytes
                                        roi));

    // Profiling Objects
    cl_ulong start = 0;
    cl_ulong end = 0;
    double diff_prof = 0.0f;
    cl::Event event_sp;

    // Execute the kernel:
    OCL_CHECK(err, err = q.enqueueTask(krnl, NULL, &event_sp));

    clWaitForEvents(1, (const cl_event*)&event_sp);

    event_sp.getProfilingInfo(CL_PROFILING_COMMAND_START, &start);
    event_sp.getProfilingInfo(CL_PROFILING_COMMAND_END, &end);
    diff_prof = end - start;
    std::cout << (diff_prof / 1000000) << "ms" << std::endl;

    // Copying Device result data to Host memory
    OCL_CHECK(err, q.enqueueReadBuffer(imageFromDevice, // This buffers data will be read
                                       CL_TRUE,         // blocking call
                                       0,               // offset
                                       (ROI_height * ROI_width * 3),
                                       out_img.data)); // Data will be stored here

    q.finish();

    cv::imwrite("hls_crop.jpg", out_img); // hls image
    cv::imwrite("ocv_crop.jpg", ocv_ref); // reference image
    cv::absdiff(ocv_ref, out_img, diff);
    cv::imwrite("diff_crop.jpg", diff); // Save the difference image for debugging purpose

#endif
#if YUV_400
    cv::Mat in_img = cv::imread(argv[1], 0);
    if (in_img.data == NULL) {
        fprintf(stderr, "Cannot open image\n");
        return 0;
    }
    // cvtColor(in_img, in_img, cv::COLOR_BGRA2BGR);
    int height = in_img.rows;
    int width = in_img.cols;

    //////////////// 	Opencv  Reference  ////////////////////////
    // crop reference
    unsigned int x_loc = 0;
    unsigned int y_loc = 0;
    unsigned int ROI_height = 480;
    unsigned int ROI_width = 320;

    cv::Mat ocv_ref, out_img, diff;
    ocv_ref.create(ROI_height, ROI_width, in_img.type());
    out_img.create(ROI_height, ROI_width, in_img.type());
    diff.create(ROI_height, ROI_width, in_img.type());
    cv::Rect ROI(x_loc, y_loc, ROI_width, ROI_height);
    ocv_ref = in_img(ROI);
    int* roi = (int*)malloc(4 * sizeof(int));
    for (int i = 0, j = 0; i < 4; j++, i += 4) {
        roi[i] = x_loc;
        roi[i + 1] = y_loc;
        roi[i + 2] = ROI_height;
        roi[i + 3] = ROI_width;
    }
    cl_int err;
    std::cout << "INFO: Running OpenCL section." << std::endl;
    // Get the device:
    std::vector<cl::Device> devices = xcl::get_xil_devices();
    cl::Device device = devices[0];

    // Context, command queue and device name:
    OCL_CHECK(err, cl::Context context(device, NULL, NULL, NULL, &err));
    OCL_CHECK(err, cl::CommandQueue q(context, device, CL_QUEUE_PROFILING_ENABLE, &err));
    OCL_CHECK(err, std::string device_name = device.getInfo<CL_DEVICE_NAME>(&err));

    std::cout << "INFO: Device found - " << device_name << std::endl;
    std::cout << "Input Image Bit Depth:" << XF_DTPIXELDEPTH(IN_TYPE, NPPCX) << std::endl;
    std::cout << "Input Image Channels:" << XF_CHANNELS(IN_TYPE, NPPCX) << std::endl;
    std::cout << "NPPC:" << NPPCX << std::endl;

    // Load binary:
    std::string binaryFile = xcl::find_binary_file(device_name, "krnl_croppipeline");
    cl::Program::Binaries bins = xcl::import_binary_file(binaryFile);
    devices.resize(1);
    OCL_CHECK(err, cl::Program program(context, devices, bins, NULL, &err));

    // Create a kernel:
    OCL_CHECK(err, cl::Kernel krnl(program, "crop_yuv400", &err));

    // Allocate the buffers:
    std::vector<cl::Memory> inBufVec, outBufVec;
    OCL_CHECK(err, cl::Buffer imageToDevice(context, CL_MEM_READ_ONLY, (in_img.rows * in_img.cols), NULL, &err));
    OCL_CHECK(err, cl::Buffer structToDeviceroi(context, CL_MEM_READ_ONLY, (4 * sizeof(int)), NULL, &err));

    OCL_CHECK(err, cl::Buffer imageFromDevice(context, CL_MEM_WRITE_ONLY, (ROI_height * ROI_width), NULL, &err));

    // Set the kernel arguments
    OCL_CHECK(err, err = krnl.setArg(0, imageToDevice));
    OCL_CHECK(err, err = krnl.setArg(1, imageFromDevice));
    OCL_CHECK(err, err = krnl.setArg(2, structToDeviceroi));
    OCL_CHECK(err, err = krnl.setArg(3, height));
    OCL_CHECK(err, err = krnl.setArg(4, width));

    /* Copy input vectors to memory */
    OCL_CHECK(err, q.enqueueWriteBuffer(imageToDevice,               // buffer on the FPGA
                                        CL_TRUE,                     // blocking call
                                        0,                           // buffer offset in bytes
                                        (in_img.rows * in_img.cols), // Size in bytes
                                        in_img.data));               // Pointer to the data to copy.
    OCL_CHECK(err, q.enqueueWriteBuffer(structToDeviceroi,           // buffer on the FPGA
                                        CL_TRUE,                     // blocking call
                                        0,                           // buffer offset in bytes
                                        (4 * sizeof(int)),           // Size in bytes
                                        roi));

    // Profiling Objects
    cl_ulong start = 0;
    cl_ulong end = 0;
    double diff_prof = 0.0f;
    cl::Event event_sp;

    // Execute the kernel:
    OCL_CHECK(err, err = q.enqueueTask(krnl, NULL, &event_sp));

    clWaitForEvents(1, (const cl_event*)&event_sp);

    event_sp.getProfilingInfo(CL_PROFILING_COMMAND_START, &start);
    event_sp.getProfilingInfo(CL_PROFILING_COMMAND_END, &end);
    diff_prof = end - start;
    std::cout << (diff_prof / 1000000) << "ms" << std::endl;

    // Copying Device result data to Host memory
    OCL_CHECK(err, q.enqueueReadBuffer(imageFromDevice, // This buffers data will be read
                                       CL_TRUE,         // blocking call
                                       0,               // offset
                                       (ROI_height * ROI_width),
                                       out_img.data)); // Data will be stored here

    q.finish();

    cv::imwrite("hls_crop.jpg", out_img); // hls image
    cv::imwrite("ocv_crop.jpg", ocv_ref); // reference image
    cv::absdiff(ocv_ref, out_img, diff);
    cv::imwrite("diff_crop.jpg", diff); // Save the difference image for debugging purpose

#endif
#if YUV_444
    cv::Mat in_img = cv::imread(argv[1], 1);
    if (!in_img.data) {
        fprintf(stderr, "Can't open image %s !!.\n ", argv[1]);
        return -1;
    }

    int height = in_img.rows;
    int width = in_img.cols;

    // reference code
    cv::Mat refmat(HEIGHT, WIDTH, CV_8UC3);
    // cvtColor(imgInput0, ref_mat, cv::COLOR_RGB2YCrCb);
    yuv4442rgb_ref(in_img, refmat);
    // crop reference
    unsigned int x_loc = 0;
    unsigned int y_loc = 0;
    unsigned int ROI_height = 480;
    unsigned int ROI_width = 320;

    cv::Mat ocv_ref, out_img, diff;
    ocv_ref.create(ROI_height, ROI_width, refmat.type());
    out_img.create(ROI_height, ROI_width, refmat.type());
    diff.create(ROI_height, ROI_width, refmat.type());
    cv::Rect ROI(x_loc, y_loc, ROI_width, ROI_height);
    ocv_ref = refmat(ROI);
    int* roi = (int*)malloc(4 * sizeof(int));
    for (int i = 0, j = 0; i < 4; j++, i += 4) {
        roi[i] = x_loc;
        roi[i + 1] = y_loc;
        roi[i + 2] = ROI_height;
        roi[i + 3] = ROI_width;
    }

    cl_int err;
    std::cout << "INFO: Running OpenCL section." << std::endl;
    // Get the device:
    std::vector<cl::Device> devices = xcl::get_xil_devices();
    cl::Device device = devices[0];

    // Context, command queue and device name:
    OCL_CHECK(err, cl::Context context(device, NULL, NULL, NULL, &err));
    OCL_CHECK(err, cl::CommandQueue q(context, device, CL_QUEUE_PROFILING_ENABLE, &err));
    OCL_CHECK(err, std::string device_name = device.getInfo<CL_DEVICE_NAME>(&err));

    std::cout << "INFO: Device found - " << device_name << std::endl;
    std::cout << "Input Image Bit Depth:" << XF_DTPIXELDEPTH(IN_TYPE, NPPCX) << std::endl;
    std::cout << "Input Image Channels:" << XF_CHANNELS(IN_TYPE, NPPCX) << std::endl;
    std::cout << "NPPC:" << NPPCX << std::endl;

    // Load binary:
    std::string binaryFile = xcl::find_binary_file(device_name, "krnl_crop");
    cl::Program::Binaries bins = xcl::import_binary_file(binaryFile);
    devices.resize(1);
    OCL_CHECK(err, cl::Program program(context, devices, bins, NULL, &err));

    // Create a kernel:
    OCL_CHECK(err, cl::Kernel krnl(program, "crop_yuv444", &err));

    // Allocate the buffers:
    std::vector<cl::Memory> inBufVec, outBufVec;
    OCL_CHECK(err, cl::Buffer imageToDevice(context, CL_MEM_READ_ONLY, (in_img.rows * in_img.cols * 3), NULL, &err));
    OCL_CHECK(err, cl::Buffer structToDeviceroi(context, CL_MEM_READ_ONLY, (4 * sizeof(int)), NULL, &err));

    OCL_CHECK(err, cl::Buffer imageFromDevice(context, CL_MEM_WRITE_ONLY, (ROI_height * ROI_width * 3), NULL, &err));

    // Set the kernel arguments
    OCL_CHECK(err, err = krnl.setArg(0, imageToDevice));
    OCL_CHECK(err, err = krnl.setArg(1, imageFromDevice));
    OCL_CHECK(err, err = krnl.setArg(2, structToDeviceroi));
    OCL_CHECK(err, err = krnl.setArg(3, height));
    OCL_CHECK(err, err = krnl.setArg(4, width));

    /* Copy input vectors to memory */
    OCL_CHECK(err, q.enqueueWriteBuffer(imageToDevice,                   // buffer on the FPGA
                                        CL_TRUE,                         // blocking call
                                        0,                               // buffer offset in bytes
                                        (in_img.rows * in_img.cols * 3), // Size in bytes
                                        in_img.data));                   // Pointer to the data to copy.
    OCL_CHECK(err, q.enqueueWriteBuffer(structToDeviceroi,               // buffer on the FPGA
                                        CL_TRUE,                         // blocking call
                                        0,                               // buffer offset in bytes
                                        (4 * sizeof(int)),               // Size in bytes
                                        roi));

    // Profiling Objects
    cl_ulong start = 0;
    cl_ulong end = 0;
    double diff_prof = 0.0f;
    cl::Event event_sp;

    // Execute the kernel:
    OCL_CHECK(err, err = q.enqueueTask(krnl, NULL, &event_sp));

    clWaitForEvents(1, (const cl_event*)&event_sp);

    event_sp.getProfilingInfo(CL_PROFILING_COMMAND_START, &start);
    event_sp.getProfilingInfo(CL_PROFILING_COMMAND_END, &end);
    diff_prof = end - start;
    std::cout << (diff_prof / 1000000) << "ms" << std::endl;

    // Copying Device result data to Host memory
    OCL_CHECK(err, q.enqueueReadBuffer(imageFromDevice, // This buffers data will be read
                                       CL_TRUE,         // blocking call
                                       0,               // offset
                                       (ROI_height * ROI_width * 3),
                                       out_img.data)); // Data will be stored here

    q.finish();

    cv::imwrite("hls_crop.jpg", out_img); // hls image
    cv::imwrite("ocv_crop.jpg", ocv_ref); // reference image
    cv::absdiff(ocv_ref, out_img, diff);
    cv::imwrite("diff_crop.jpg", diff); // Save the difference image for debugging purpose

#endif
    float err_per;
    xf::cv::analyzeDiff(diff, ERROR_THRESHOLD, err_per);

    if (err_per > 3.0f) {
        fprintf(stderr, "\n1st Image Test Failed\n");
        return 1;
    } else
        std::cout << "Test Passed " << std::endl;
    /*#if (IYUV2NV12 || RGBA2NV12 || RGBA2NV21 || UYVY2NV12 || YUYV2NV12 || NV122IYUV || NV212IYUV || IYUV2YUV4 || \
         NV122YUV4 || NV212YUV4 || RGBA2IYUV || RGBA2YUV4 || UYVY2IYUV || YUYV2IYUV || NV122NV21 || NV212NV12)
        xf::cv::analyzeDiff(errImg1, ERROR_THRESHOLD, err_per);
        if (err_per > 3.0f) {
            fprintf(stderr, "\n2nd Image Test Failed\n");
            return 1;
        } else
            std::cout << "Test Passed " << std::endl;

    #endif
    #if (IYUV2YUV4 || NV122IYUV || NV122YUV4 || NV212IYUV || NV212YUV4 || RGBA2IYUV || RGBA2YUV4 || UYVY2IYUV ||
    YUYV2IYUV)
        xf::cv::analyzeDiff(errImg2, ERROR_THRESHOLD, err_per);
        if (err_per > 3.0f) {
            fprintf(stderr, "\n3rd Image Test Failed\n");
            return 1;
        } else
            std::cout << "Test Passed " << std::endl;
    #endif

        /* ## *************************************************************** ##*/
    return 0;
}
