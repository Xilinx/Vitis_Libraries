/*
 * Copyright 2020 Xilinx, Inc.
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
#include "xf_blobfromimage_tb_config.h"
//#include <sys/time.h>

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

int main(int argc, char* argv[]) {
    struct timeval start_pp_sw, end_pp_sw;
    double lat_pp_sw = 0.0f;
    cv::Mat imgInput0, imgInput1, refOutput0, result_hls, result_ocv, error;
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
    ap_uint<16 * NPC1>* _imgInput0 = (ap_uint<16 * NPC1>*)yuyv_img.data;
    cv::imwrite("yuyv_img.png", yuyv_img);
    printf("YUV422.rows=%d YUV422.cols=%d", YUV422.rows, YUV422.cols);
    YUYV2RGB_ref(yuyv_img.data, RGB_Img.data, (width), height, 3);
    cv::imwrite("RGB_Img.png", RGB_Img);
    refOutput0 = RGB_Img;
#endif
#if YUV_400
    imgInput0 = cv::imread(argv[1], 0);
    if (!imgInput0.data) {
        fprintf(stderr, "Can't open image %s !!.\n ", argv[1]);
        return -1;
    }

#endif
#if (YUV_444 || RGB)
    imgInput0 = cv::imread(argv[1], 1);
    if (!imgInput0.data) {
        fprintf(stderr, "Can't open image %s !!.\n ", argv[1]);
        return -1;
    }
    ap_uint<32 * NPC1>* _imgInput0 = (ap_uint<32 * NPC1>*)imgInput0.data;

#endif

    int in_width, in_height;
    int out_width, out_height;

    in_width = imgInput0.cols;
    in_height = imgInput0.rows;
    std::cout << "Input image height : " << in_height << std::endl;
    std::cout << "Input image width  : " << in_width << std::endl;

    // Add padding in the input image
    // int in_pad = in_width;
    int in_stride = 0;

    out_height = 600; // atoi(argv[2]);
    out_width = 600;  // 224;//atoi(argv[3]);

    int resize_height = 600;
    int resize_width = 600;

    int roi_posx = 10;
    int roi_posy = 10;
#if YUV_400
    result_hls.create(cv::Size(out_width, out_height), CV_8UC1);
    // Mean and Scale values
    float params[2];
    params[1] = 0.25; // scale values
    // Mean values
    params[0] = 128.0f;

#else
    result_hls.create(cv::Size(out_width, out_height), CV_8UC3);
    // Mean and Scale values
    float params[6];
    params[3] = params[4] = params[5] = 0.25; // scale values
    // Mean values
    params[0] = 128.0f;
    params[1] = 128.0f;
    params[2] = 128.0f;

#endif

#if YUV_420
    (void)cl_kernel_mgr::registerKernel(
        "blobfromimage_yuv420", "krnl_blobfromimage_yuv420", XCLIN(_imgInput0, (in_width * in_height)),
        XCLIN(_imgInput1, (uv_Img_16.rows * uv_Img_16.cols * 2)), XCLOUT(result_hls), XCLIN(params, 6 * sizeof(float)),
        XCLIN(in_width), XCLIN(in_height), XCLIN(in_stride), XCLIN(resize_width), XCLIN(resize_height),
        XCLIN(out_width), XCLIN(out_height), XCLIN(out_width), XCLIN(roi_posx), XCLIN(roi_posy));
    cl_kernel_mgr::exec_all();

#endif
#if YUV_422
    (void)cl_kernel_mgr::registerKernel("blobfromimage_yuv422", "krnl_blobfromimage_yuv422",
                                        XCLIN(_imgInput0, (in_width * in_height * 2)), XCLOUT(result_hls),
                                        XCLIN(params, 6 * sizeof(float)), XCLIN(in_width), XCLIN(in_height),
                                        XCLIN(in_stride), XCLIN(resize_width), XCLIN(resize_height), XCLIN(out_width),
                                        XCLIN(out_height), XCLIN(out_width), XCLIN(roi_posx), XCLIN(roi_posy));
    cl_kernel_mgr::exec_all();

#endif
#if YUV_400
    (void)cl_kernel_mgr::registerKernel("blobfromimage_yuv400", "krnl_blobfromimage_accel",
                                        XCLIN(_imgInput0, (in_width * in_height)), XCLOUT(result_hls),
                                        XCLIN(params, 6 * sizeof(float)), XCLIN(in_width), XCLIN(in_height),
                                        XCLIN(in_stride), XCLIN(resize_width), XCLIN(resize_height), XCLIN(out_width),
                                        XCLIN(out_height), XCLIN(out_width), XCLIN(roi_posx), XCLIN(roi_posy));
    cl_kernel_mgr::exec_all();

#endif
#if (YUV_444)
    (void)cl_kernel_mgr::registerKernel("blobfromimage_yuv444", "krnl_blobfromimage_accel",
                                        XCLIN(_imgInput0, (in_width * in_height * 3)), XCLOUT(result_hls),
                                        XCLIN(params, 6 * sizeof(float)), XCLIN(in_width), XCLIN(in_height),
                                        XCLIN(in_stride), XCLIN(resize_width), XCLIN(resize_height), XCLIN(out_width),
                                        XCLIN(out_height), XCLIN(out_width), XCLIN(roi_posx), XCLIN(roi_posy));
    cl_kernel_mgr::exec_all();

#endif
#if (RGB)
    (void)cl_kernel_mgr::registerKernel("blobfromimage_rgb", "krnl_blobfromimage_rgb",
                                        XCLIN(_imgInput0, (in_width * in_height * 3)), XCLOUT(result_hls),
                                        XCLIN(params, 6 * sizeof(float)), XCLIN(in_width), XCLIN(in_height),
                                        XCLIN(in_stride), XCLIN(resize_width), XCLIN(resize_height), XCLIN(out_width),
                                        XCLIN(out_height), XCLIN(out_width), XCLIN(roi_posx), XCLIN(roi_posy));
    cl_kernel_mgr::exec_all();

#endif

/////////////////////////////////////// end of CL
//////////////////////////////////////////

/*Reference Implementation*/

#if YUV_444
    cv::Mat refmat(HEIGHT, WIDTH, CV_8UC3);
    refOutput0.create(cv::Size(HEIGHT, WIDTH), CV_8UC3);
    yuv4442rgb_ref(imgInput0, refmat);
    refOutput0 = refmat;
#endif
#if RGB
    cv::Mat refmat(HEIGHT, WIDTH, CV_8UC3);
    refOutput0.create(cv::Size(HEIGHT, WIDTH), CV_8UC3);
    refOutput0 = imgInput0;
#endif

    printf("***************hls run completed******************\n");
#if (!YUV_400)
    cv::imwrite("hw_op.png", result_hls);                                      // Write output image from Kernel
    cv::resize(refOutput0, result_ocv, cv::Size(resize_width, resize_height)); // First Resize the image
    cv::imwrite("cv_resized.png", result_ocv);
    uchar* img_ocv_data = result_ocv.data; // result_ocv_crop.data;
    int frame_cntr1 = 0;
    float* data_ptr_cv = (float*)malloc(out_width * out_height * 3 * sizeof(float));
    int idx = 0;
    float* dst1_cv = &data_ptr_cv[0];
    float* dst2_cv = &data_ptr_cv[out_height * out_width];
    float* dst3_cv = &data_ptr_cv[(3 - 1) * out_width * out_height];
    for (int ll_rows = 0; ll_rows < out_height; ll_rows++) {
        for (int ll_cols = 0; ll_cols < out_width; ll_cols++) {
            dst1_cv[idx] = (float)(img_ocv_data[frame_cntr1++] - params[0]) * params[3];
            dst2_cv[idx] = (float)(img_ocv_data[frame_cntr1++] - params[1]) * params[4];
            dst3_cv[idx] = (float)(img_ocv_data[frame_cntr1++] - params[2]) * params[5];
            idx++;
        }
    }
#else
    cv::imwrite("hw_op.png", result_hls);                                     // Write output image from Kernel
    cv::resize(imgInput0, result_ocv, cv::Size(resize_width, resize_height)); // First Resize the image
    cv::imwrite("cv_resized.png", result_ocv);
    uchar* img_ocv_data = result_ocv.data; // result_ocv_crop.data;
    int frame_cntr1 = 0;
    float* data_ptr_cv = (float*)malloc(out_width * out_height * sizeof(float));
    int idx = 0;
    float* dst1_cv = &data_ptr_cv[0];
    // float* dst2_cv = &data_ptr_cv[out_height * out_width];
    // float* dst3_cv = &data_ptr_cv[(3 - 1) * out_width * out_height];
    for (int ll_rows = 0; ll_rows < out_height; ll_rows++) {
        for (int ll_cols = 0; ll_cols < out_width; ll_cols++) {
            dst1_cv[idx] = (float)(img_ocv_data[frame_cntr1++] - params[0]) * params[1];
            // dst2_cv[idx] = (float)(img_ocv_data[frame_cntr1++] - params[1]) * params[4];
            // dst3_cv[idx] = (float)(img_ocv_data[frame_cntr1++] - params[2]) * params[5];
            idx++;
        }
    }

#endif

    // Error Checking
    int frame_cntr = 0;
#if (!YUV_400)
    float* data_ptr = (float*)malloc(out_width * out_height * 3 * sizeof(float));
    float* dst1 = &data_ptr[0];
    float* dst2 = &data_ptr[out_width * out_height];
    float* dst3 = &data_ptr[(3 - 1) * out_width * out_height];

    signed char* img_data = (signed char*)result_hls.data;
    float err_th = 1.0;
    float max_error1 = 0.0, max_error2 = 0.0, max_error3 = 0.0;
    int err_cnt1 = 0, err_cnt2 = 0, err_cnt3 = 0;
    int idx1 = 0;
    for (int l_rows = 0; l_rows < out_height; l_rows++) {
        for (int l_cols = 0; l_cols < out_width; l_cols++) {
            dst1[idx1] = (float)img_data[frame_cntr++];
            dst2[idx1] = (float)img_data[frame_cntr++];
            dst3[idx1] = (float)img_data[frame_cntr++];

            float err1 = fabs(dst1[idx1] - dst1_cv[idx1]);
            float err2 = fabs(dst2[idx1] - dst2_cv[idx1]);
            float err3 = fabs(dst3[idx1] - dst3_cv[idx1]);
            if (err1 > err_th && err_cnt1 < 20) {
                std::cout << "Ref = " << dst1_cv[idx1] << "HW = " << dst1[idx1] << std::endl;
                err_cnt1++;
            }
            if (err1 > max_error1) {
                max_error1 = err1;
            }
            if (err2 > max_error2) {
                max_error2 = err2;
            }
            if (err3 > max_error3) {
                max_error3 = err3;
            }
            if (err2 > err_th) {
                err_cnt2++;
            }
            if (err3 > err_th) {
                err_cnt3++;
            }
            idx1++;

        } // l_cols
    }     // l_rows

    std::cout << "\nMax Errors: channel 1 = " << max_error1;
    std::cout << "\tchannel 2 = " << max_error2;
    std::cout << "\tchannel 3 = " << max_error3;
    std::cout << "\nError Counts: Ch1 = " << err_cnt1;
    std::cout << "\tCh2 = " << err_cnt2;
    std::cout << "\tCh3 = " << err_cnt3 << std::endl;

    if (max_error1 > err_th || max_error2 > err_th || max_error3 > err_th) {
        fprintf(stderr, "\n Test Failed\n");
        return -1;

    } else {
        std::cout << "Test Passed " << std::endl;
        return 0;
    }
#else
    float* data_ptr = (float*)malloc(out_width * out_height * sizeof(float));
    float* dst1 = &data_ptr[0];
    signed char* img_data = (signed char*)result_hls.data;
    float err_th = 1.0;
    float max_error1 = 0.0, max_error2 = 0.0, max_error3 = 0.0;
    int err_cnt1 = 0, err_cnt2 = 0, err_cnt3 = 0;
    int idx1 = 0;
    for (int l_rows = 0; l_rows < out_height; l_rows++) {
        for (int l_cols = 0; l_cols < out_width; l_cols++) {
            dst1[idx1] = (float)img_data[frame_cntr++];

            float err1 = fabs(dst1[idx1] - dst1_cv[idx1]);

            if (err1 > err_th && err_cnt1 < 20) {
                std::cout << "Ref = " << dst1_cv[idx1] << "HW = " << dst1[idx1] << std::endl;
                err_cnt1++;
            }
            if (err1 > max_error1) {
                max_error1 = err1;
            }
            idx1++;

        } // l_cols
    }     // l_rows

    std::cout << "\nMax Errors: channel 1 = " << max_error1;
    std::cout << "\nError Counts: Ch1 = " << err_cnt1 << std::endl;
    ;

    if (max_error1 > err_th) {
        fprintf(stderr, "\n Test Failed\n");
        return -1;

    } else {
        std::cout << "Test Passed " << std::endl;
        return 0;
    }
#endif
}
