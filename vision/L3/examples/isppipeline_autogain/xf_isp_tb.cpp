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
#include "xf_isp_tb_config.h"

#include "xcl2.hpp"

const float bt2020_bt709_arr[3][3] = {
    {1.6605, -0.5876, -0.0728}, {-0.1246, 1.1329, -0.0083}, {-0.0182, -0.1006, 1.1187}};

const float bt2020_bt709_off[3] = {0.0, 0.0, 0.0};

const float bt709_bt2020_arr[3][3] = {{0.627, 0.329, 0.0433}, {0.0691, 0.92, 0.0113}, {0.0164, 0.088, 0.896}};

const float bt709_bt2020_off[3] = {0.0, 0.0, 0.0};

const float rgb_yuv_601_arr[3][3] = {{0.257, 0.504, 0.098}, {-0.148, -0.291, 0.439}, {0.439, -0.368, -0.071}};

const float rgb_yuv_601_off[3] = {0.0625, 0.500, 0.500};

const float rgb_yuv_709_arr[3][3] = {{0.183, 0.614, 0.062}, {-0.101, -0.338, 0.439}, {0.439, -0.399, -0.040}};

const float rgb_yuv_709_off[3] = {0.0625, 0.500, 0.500};

const float rgb_yuv_2020_arr[3][3] = {
    {0.225613, 0.582282, 0.050928}, {-0.119918, -0.309494, 0.429412}, {0.429412, -0.394875, -0.034537}};

const float rgb_yuv_2020_off[3] = {0.062745, 0.500, 0.500};

const float yuv_rgb_601_arr[3][3] = {{1.164, 0.000, 1.596}, {1.164, -0.813, -0.391}, {1.164, 2.018, 0.000}};

const float yuv_rgb_601_off[3] = {-0.87075, 0.52925, -1.08175};

const float yuv_rgb_709_arr[3][3] = {{1.164, 0.000, 1.793}, {1.164, -0.213, -0.534}, {1.164, 2.115, 0.000}};

const float yuv_rgb_709_off[3] = {-0.96925, 0.30075, -1.13025};

const float yuv_rgb_2020_arr[3][3] = {
    {1.164384, 0.000000, 1.717000}, {1.164384, -0.191603, -0.665274}, {1.164384, 2.190671, 0.000000}};

const float yuv_rgb_2020_off[3] = {-0.931559, 0.355379, -1.168395};

const float full_to_16_235_arr[3][3] = {
    {0.856305, 0.000000, 0.000000}, {0.000000, 0.856305, 0.000000}, {0.000000, 0.000000, 0.856305}};

const float full_to_16_235_off[3] = {0.0625, 0.0625, 0.0625};

const float full_from_16_235_arr[3][3] = {
    {1.167808, 0.000000, 0.000000}, {0.000000, 1.167808, 0.000000}, {0.000000, 0.000000, 1.167808}};

const float full_from_16_235_off[3] = {-0.0729880, -0.0729880, -0.0729880};

void compute_gamma(float r_g, float g_g, float b_g, uchar gamma_lut[256 * 3]) {
    float gamma_inv[256] = {
        0.000000, 0.003922, 0.007843, 0.011765, 0.015686, 0.019608, 0.023529, 0.027451, 0.031373, 0.035294, 0.039216,
        0.043137, 0.047059, 0.050980, 0.054902, 0.058824, 0.062745, 0.066667, 0.070588, 0.074510, 0.078431, 0.082353,
        0.086275, 0.090196, 0.094118, 0.098039, 0.101961, 0.105882, 0.109804, 0.113725, 0.117647, 0.121569, 0.125490,
        0.129412, 0.133333, 0.137255, 0.141176, 0.145098, 0.149020, 0.152941, 0.156863, 0.160784, 0.164706, 0.168627,
        0.172549, 0.176471, 0.180392, 0.184314, 0.188235, 0.192157, 0.196078, 0.200000, 0.203922, 0.207843, 0.211765,
        0.215686, 0.219608, 0.223529, 0.227451, 0.231373, 0.235294, 0.239216, 0.243137, 0.247059, 0.250980, 0.254902,
        0.258824, 0.262745, 0.266667, 0.270588, 0.274510, 0.278431, 0.282353, 0.286275, 0.290196, 0.294118, 0.298039,
        0.301961, 0.305882, 0.309804, 0.313725, 0.317647, 0.321569, 0.325490, 0.329412, 0.333333, 0.337255, 0.341176,
        0.345098, 0.349020, 0.352941, 0.356863, 0.360784, 0.364706, 0.368627, 0.372549, 0.376471, 0.380392, 0.384314,
        0.388235, 0.392157, 0.396078, 0.400000, 0.403922, 0.407843, 0.411765, 0.415686, 0.419608, 0.423529, 0.427451,
        0.431373, 0.435294, 0.439216, 0.443137, 0.447059, 0.450980, 0.454902, 0.458824, 0.462745, 0.466667, 0.470588,
        0.474510, 0.478431, 0.482353, 0.486275, 0.490196, 0.494118, 0.498039, 0.501961, 0.505882, 0.509804, 0.513725,
        0.517647, 0.521569, 0.525490, 0.529412, 0.533333, 0.537255, 0.541176, 0.545098, 0.549020, 0.552941, 0.556863,
        0.560784, 0.564706, 0.568627, 0.572549, 0.576471, 0.580392, 0.584314, 0.588235, 0.592157, 0.596078, 0.600000,
        0.603922, 0.607843, 0.611765, 0.615686, 0.619608, 0.623529, 0.627451, 0.631373, 0.635294, 0.639216, 0.643137,
        0.647059, 0.650980, 0.654902, 0.658824, 0.662745, 0.666667, 0.670588, 0.674510, 0.678431, 0.682353, 0.686275,
        0.690196, 0.694118, 0.698039, 0.701961, 0.705882, 0.709804, 0.713725, 0.717647, 0.721569, 0.725490, 0.729412,
        0.733333, 0.737255, 0.741176, 0.745098, 0.749020, 0.752941, 0.756863, 0.760784, 0.764706, 0.768627, 0.772549,
        0.776471, 0.780392, 0.784314, 0.788235, 0.792157, 0.796078, 0.800000, 0.803922, 0.807843, 0.811765, 0.815686,
        0.819608, 0.823529, 0.827451, 0.831373, 0.835294, 0.839216, 0.843137, 0.847059, 0.850980, 0.854902, 0.858824,
        0.862745, 0.866667, 0.870588, 0.874510, 0.878431, 0.882353, 0.886275, 0.890196, 0.894118, 0.898039, 0.901961,
        0.905882, 0.909804, 0.913725, 0.917647, 0.921569, 0.925490, 0.929412, 0.933333, 0.937255, 0.941176, 0.945098,
        0.949020, 0.952941, 0.956863, 0.960784, 0.964706, 0.968627, 0.972549, 0.976471, 0.980392, 0.984314, 0.988235,
        0.992157, 0.996078, 1.000000};

    unsigned char gam_r = 0, gam_g = 0, gam_b = 0;

    for (int i = 0; i < 256; ++i) {
        float r_inv = (float)1 / r_g;
        float g_inv = (float)1 / g_g;
        float b_inv = (float)1 / b_g;
        float powval_r = (float)std::pow(gamma_inv[i], r_inv);
        short tempgamma_r = (powval_r * 255.0);

        if (tempgamma_r > 255) {
            gam_r = 255;
        } else {
            gam_r = tempgamma_r;
        }

        float powval_g = (float)std::pow(gamma_inv[i], g_inv);
        short tempgamma_g = (powval_g * 255.0);

        if (tempgamma_g > 255) {
            gam_g = 255;
        } else {
            gam_g = tempgamma_g;
        }

        float powval_b = (float)std::pow(gamma_inv[i], b_inv);
        short tempgamma_b = (powval_b * 255.0);

        if (tempgamma_b > 255) {
            gam_b = 255;
        } else {
            gam_b = tempgamma_b;
        }
        gamma_lut[i] = gam_r;
        gamma_lut[i + 256] = gam_g;
        gamma_lut[i + 512] = gam_b;
    }
}

void bayerizeImage(cv::Mat in_img, cv::Mat& cfa_output, int code) {
    for (int i = 0; i < in_img.rows; i++) {
        for (int j = 0; j < in_img.cols; j++) {
#if T_8U
            cv::Vec3b in = in_img.at<cv::Vec3b>(i, j);
#define pxltype unsigned char
#else
            cv::Vec3w in = in_img.at<cv::Vec3w>(i, j);
#define pxltype unsigned short
#endif

            if (code == 0) {            // BG
                if ((i & 1) == 0) {     // even row
                    if ((j & 1) == 0) { // even col
                        cfa_output.at<pxltype>(i, j) = in[0];
                    } else { // odd col
                        cfa_output.at<pxltype>(i, j) = in[1];
                    }
                } else {                // odd row
                    if ((j & 1) == 0) { // even col
                        cfa_output.at<pxltype>(i, j) = in[1];
                    } else { // odd col
                        cfa_output.at<pxltype>(i, j) = in[2];
                    }
                }
            }
            if (code == 1) {            // GB
                if ((i & 1) == 0) {     // even row
                    if ((j & 1) == 0) { // even col
                        cfa_output.at<pxltype>(i, j) = in[1];
                    } else { // odd col
                        cfa_output.at<pxltype>(i, j) = in[0];
                    }
                } else {                // odd row
                    if ((j & 1) == 0) { // even col
                        cfa_output.at<pxltype>(i, j) = in[2];
                    } else { // odd col
                        cfa_output.at<pxltype>(i, j) = in[1];
                    }
                }
            }
            if (code == 2) {            // GR
                if ((i & 1) == 0) {     // even row
                    if ((j & 1) == 0) { // even col
                        cfa_output.at<pxltype>(i, j) = in[1];
                    } else { // odd col
                        cfa_output.at<pxltype>(i, j) = in[2];
                    }
                } else {                // odd row
                    if ((j & 1) == 0) { // even col
                        cfa_output.at<pxltype>(i, j) = in[0];
                    } else { // odd col
                        cfa_output.at<pxltype>(i, j) = in[1];
                    }
                }
            }
            if (code == 3) {            // RG
                if ((i & 1) == 0) {     // even row
                    if ((j & 1) == 0) { // even col
                        cfa_output.at<pxltype>(i, j) = in[2];
                    } else { // odd col
                        cfa_output.at<pxltype>(i, j) = in[1];
                    }
                } else {                // odd row
                    if ((j & 1) == 0) { // even col
                        cfa_output.at<pxltype>(i, j) = in[1];
                    } else { // odd col
                        cfa_output.at<pxltype>(i, j) = in[0];
                    }
                }
            }
        }
    }
}

int main(int argc, char** argv) {
    if (argc != 2) {
        fprintf(stderr, "Invalid Number of Arguments!\nUsage:\n");
        fprintf(stderr, "<Executable Name> <input image path> \n");
        return -1;
    }

    cv::Mat in_img, out_img, ocv_ref, in_gray, diff, bayer_img;

    unsigned short in_width, in_height;

/*  reading in the color image  */
#if T_8U
    in_img = cv::imread(argv[1], 1); // read image
#else
    in_img = cv::imread(argv[1], -1); // read image
#endif

    if (in_img.data == NULL) {
        fprintf(stderr, "Cannot open image at %s\n", argv[1]);
        return 0;
    }

#if T_8U
    out_img.create(in_img.rows, in_img.cols, CV_16UC1);
    size_t vec_in_size_bytes = 256 * 3 * sizeof(unsigned char);
    size_t image_in_size_bytes = in_img.rows * in_img.cols * sizeof(unsigned char);
    size_t image_out_size_bytes = in_img.rows * in_img.cols * 1 * sizeof(unsigned short);
#else
    out_img.create(in_img.rows, in_img.cols, CV_16UC1);
    size_t vec_in_size_bytes = 256 * 3 * sizeof(unsigned char);
    size_t image_in_size_bytes = in_img.rows * in_img.cols * sizeof(unsigned short);
    size_t image_out_size_bytes = in_img.rows * in_img.cols * 1 * sizeof(unsigned short);
#endif
    unsigned short bformat = XF_BAYER_PATTERN; // Bayer format BG-0; GB-1; GR-2; RG-3
#if T_8U
    bayer_img.create(in_img.rows, in_img.cols, CV_8UC1);
#elif T_16U
    bayer_img.create(in_img.rows, in_img.cols, CV_16UC1);
#endif
    bayerizeImage(in_img, bayer_img, bformat);

    // Write input image
    imwrite("input.png", in_img);
    imwrite("bayer_img.png", bayer_img);

    float ccm_matrix[3][3];
    float offsetarray[3];

    switch (XF_CCM_TYPE) {
        case 0:
            for (int i = 0; i < 3; i++) {
                for (int j = 0; j < 3; j++) {
                    ccm_matrix[i][j] = bt2020_bt709_arr[i][j];
                }
                offsetarray[i] = bt2020_bt709_off[i];
            }

            break;
        case 1:
            for (int i = 0; i < 3; i++) {
                for (int j = 0; j < 3; j++) {
                    ccm_matrix[i][j] = bt709_bt2020_arr[i][j];
                }
                offsetarray[i] = bt709_bt2020_off[i];
            }

            break;
        case 2:
            for (int i = 0; i < 3; i++) {
                for (int j = 0; j < 3; j++) {
                    ccm_matrix[i][j] = rgb_yuv_601_arr[i][j];
                }
                offsetarray[i] = rgb_yuv_601_off[i];
            }

            break;
        case 3:
            for (int i = 0; i < 3; i++) {
                for (int j = 0; j < 3; j++) {
                    ccm_matrix[i][j] = rgb_yuv_709_arr[i][j];
                }
                offsetarray[i] = rgb_yuv_709_off[i];
            }

            break;
        case 4:
            for (int i = 0; i < 3; i++) {
                for (int j = 0; j < 3; j++) {
                    ccm_matrix[i][j] = rgb_yuv_2020_arr[i][j];
                }
                offsetarray[i] = rgb_yuv_2020_off[i];
            }

            break;
        case 5:
            for (int i = 0; i < 3; i++) {
                for (int j = 0; j < 3; j++) {
                    ccm_matrix[i][j] = yuv_rgb_601_arr[i][j];
                }
                offsetarray[i] = yuv_rgb_601_off[i];
            }

            break;
        case 6:
            for (int i = 0; i < 3; i++) {
                for (int j = 0; j < 3; j++) {
                    ccm_matrix[i][j] = yuv_rgb_709_arr[i][j];
                }
                offsetarray[i] = yuv_rgb_709_off[i];
            }

            break;
        case 7:
            for (int i = 0; i < 3; i++) {
                for (int j = 0; j < 3; j++) {
                    ccm_matrix[i][j] = yuv_rgb_2020_arr[i][j];
                }
                offsetarray[i] = yuv_rgb_2020_off[i];
            }

            break;
        case 8:
            for (int i = 0; i < 3; i++) {
                for (int j = 0; j < 3; j++) {
                    ccm_matrix[i][j] = full_to_16_235_arr[i][j];
                }
                offsetarray[i] = full_to_16_235_off[i];
            }

            break;
        case 9:
            for (int i = 0; i < 3; i++) {
                for (int j = 0; j < 3; j++) {
                    ccm_matrix[i][j] = full_from_16_235_arr[i][j];
                }
                offsetarray[i] = full_from_16_235_off[i];
            }

            break;
        default:
            break;
    }
    // cmm matrix shifted 20 bits to the left
    signed int ccm_matrix_int[3][3];
    signed int offsetarray_int[3];

    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            ccm_matrix_int[i][j] = (signed int)(ccm_matrix[i][j] * 1048576);
        }
        offsetarray_int[i] = (signed int)(offsetarray[i] * 1048576);
    }

    /////////////////////////////////////// CL ////////////////////////

    int height = in_img.rows;
    int width = in_img.cols;
    std::cout << "Input image height : " << height << std::endl;
    std::cout << "Input image width  : " << width << std::endl;
    size_t ccm_matrix_int_size_bytes = 3 * 3 * sizeof(signed int);
    size_t offsetarray_int_size_bytes = 3 * sizeof(signed int);

    unsigned short rgain = 256;
    unsigned short bgain = 256;
    unsigned short ggain = 200;

    unsigned char mode_reg = 1;

    unsigned short pawb = 128;

    unsigned char gamma_lut[256 * 3];
    uint32_t hist0_awb[3][HIST_SIZE] = {0};
    uint32_t hist1_awb[3][HIST_SIZE] = {0};

    float gamma_val_r = 0.5f, gamma_val_g = 0.8f, gamma_val_b = 0.8f;

    compute_gamma(gamma_val_r, gamma_val_g, gamma_val_b, gamma_lut);

    // int channels=out_img.channels();

    cl_int err;
    std::cout << "INFO: Running OpenCL section." << std::endl;

    std::vector<cl::Device> devices = xcl::get_xil_devices();
    cl::Device device = devices[0];
    OCL_CHECK(err, cl::Context context(device, NULL, NULL, NULL, &err));
    OCL_CHECK(err, cl::CommandQueue q(context, device, CL_QUEUE_PROFILING_ENABLE, &err));
    std::cout << "Input Image Bit Depth:" << XF_DTPIXELDEPTH(IN_TYPE, XF_NPPCX) << std::endl;
    std::cout << "Input Image Channels:" << XF_CHANNELS(IN_TYPE, XF_NPPCX) << std::endl;
    std::cout << "NPPC:" << XF_NPPCX << std::endl;

    std::string device_name = device.getInfo<CL_DEVICE_NAME>();
    std::string binaryFile = xcl::find_binary_file(device_name, "krnl_ISPPipeline");
    cl::Program::Binaries bins = xcl::import_binary_file(binaryFile);
    devices.resize(1);

    OCL_CHECK(err, cl::Program program(context, devices, bins, NULL, &err));

    // Create a kernel:
    OCL_CHECK(err, cl::Kernel kernel(program, "ISPPipeline_accel", &err));

    std::vector<cl::Memory> inBufVec, outBufVec;
    OCL_CHECK(err, cl::Buffer imageToDevice(context, CL_MEM_READ_ONLY, image_in_size_bytes, NULL, &err));
    OCL_CHECK(err, cl::Buffer imageFromDevice(context, CL_MEM_WRITE_ONLY, image_out_size_bytes, NULL, &err));
    OCL_CHECK(err, cl::Buffer buffer_inVec(context, CL_MEM_READ_ONLY, vec_in_size_bytes, NULL, &err));
    OCL_CHECK(err, cl::Buffer buffer_ccm_matrix_int(context, CL_MEM_READ_ONLY, ccm_matrix_int_size_bytes, NULL, &err));
    OCL_CHECK(err,
              cl::Buffer buffer_offsetarray_int(context, CL_MEM_READ_ONLY, offsetarray_int_size_bytes, NULL, &err));
    // Set the kernel arguments
    OCL_CHECK(err, err = kernel.setArg(0, imageToDevice));
    OCL_CHECK(err, err = kernel.setArg(1, imageFromDevice));
    OCL_CHECK(err, err = kernel.setArg(2, height));
    OCL_CHECK(err, err = kernel.setArg(3, width));
    OCL_CHECK(err, err = kernel.setArg(4, buffer_inVec));
    OCL_CHECK(err, err = kernel.setArg(5, mode_reg));
    OCL_CHECK(err, err = kernel.setArg(6, pawb));
    OCL_CHECK(err, err = kernel.setArg(7, buffer_ccm_matrix_int));
    OCL_CHECK(err, err = kernel.setArg(8, buffer_offsetarray_int));
    OCL_CHECK(err, err = kernel.setArg(9, bformat));

    for (int i = 0; i < 3; i++) {
        OCL_CHECK(err, q.enqueueWriteBuffer(buffer_inVec,      // buffer on the FPGA
                                            CL_TRUE,           // blocking call
                                            0,                 // buffer offset in bytes
                                            vec_in_size_bytes, // Size in bytes
                                            gamma_lut));
        OCL_CHECK(err, q.enqueueWriteBuffer(imageToDevice, CL_TRUE, 0, image_in_size_bytes, bayer_img.data));

        // Profiling Objects
        cl_ulong start = 0;
        cl_ulong end = 0;
        double diff_prof = 0.0f;
        cl::Event event_sp;

        // Launch the kernel
        OCL_CHECK(err, err = q.enqueueTask(kernel, NULL, &event_sp));
        clWaitForEvents(1, (const cl_event*)&event_sp);

        event_sp.getProfilingInfo(CL_PROFILING_COMMAND_START, &start);
        event_sp.getProfilingInfo(CL_PROFILING_COMMAND_END, &end);
        diff_prof = end - start;
        std::cout << (diff_prof / 1000000) << "ms" << std::endl;
        OCL_CHECK(err, q.enqueueWriteBuffer(buffer_ccm_matrix_int,     // buffer on the FPGA
                                            CL_TRUE,                   // blocking call
                                            0,                         // buffer offset in bytes
                                            ccm_matrix_int_size_bytes, // Size in bytes
                                            ccm_matrix_int));

        OCL_CHECK(err, q.enqueueWriteBuffer(buffer_offsetarray_int,     // buffer on the FPGA
                                            CL_TRUE,                    // blocking call
                                            0,                          // buffer offset in bytes
                                            offsetarray_int_size_bytes, // Size in bytes
                                            offsetarray_int));

        // Copying Device result data to Host memory
        q.enqueueReadBuffer(imageFromDevice, CL_TRUE, 0, image_out_size_bytes, out_img.data);
    }
    q.finish();

    /////////////////////////////////////// end of CL ////////////////////////

    // Write output image
    imwrite("hls_out.png", out_img);
    std::cout << "Test Finished" << std::endl;

    return 0;
}
