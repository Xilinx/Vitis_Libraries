/*
 * Copyright (C) 2023 Advanced Micro Devices, Inc.
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
#include <bitset>
#include <iostream>

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

void bayerizeImage(cv::Mat img, cv::Mat& cfa_output, unsigned short code) {
    for (int i = 0; i < img.rows; i++) {
        for (int j = 0; j < img.cols; j++) {
            cv::Vec3w in = img.at<cv::Vec3w>(i, j);

            if (code == 0) {            // BG
                if ((i & 1) == 0) {     // even row
                    if ((j & 1) == 0) { // even col
                        cfa_output.at<ushort>(i, j) = in[0];
                    } else { // odd col
                        cfa_output.at<ushort>(i, j) = in[1];
                    }
                } else {                // odd row
                    if ((j & 1) == 0) { // even col
                        cfa_output.at<ushort>(i, j) = in[1];
                    } else { // odd col
                        cfa_output.at<ushort>(i, j) = in[2];
                    }
                }
            }
            if (code == 1) {            // GB
                if ((i & 1) == 0) {     // even row
                    if ((j & 1) == 0) { // even col
                        cfa_output.at<ushort>(i, j) = in[1];
                    } else { // odd col
                        cfa_output.at<ushort>(i, j) = in[0];
                    }
                } else {                // odd row
                    if ((j & 1) == 0) { // even col
                        cfa_output.at<ushort>(i, j) = in[2];
                    } else { // odd col
                        cfa_output.at<ushort>(i, j) = in[1];
                    }
                }
            }
            if (code == 2) {            // GR
                if ((i & 1) == 0) {     // even row
                    if ((j & 1) == 0) { // even col
                        cfa_output.at<ushort>(i, j) = in[1];
                    } else { // odd col
                        cfa_output.at<ushort>(i, j) = in[2];
                    }
                } else {                // odd row
                    if ((j & 1) == 0) { // even col
                        cfa_output.at<ushort>(i, j) = in[0];
                    } else { // odd col
                        cfa_output.at<ushort>(i, j) = in[1];
                    }
                }
            }
            if (code == 3) {            // RG
                if ((i & 1) == 0) {     // even row
                    if ((j & 1) == 0) { // even col
                        cfa_output.at<ushort>(i, j) = in[2];
                    } else { // odd col
                        cfa_output.at<ushort>(i, j) = in[1];
                    }
                } else {                // odd row
                    if ((j & 1) == 0) { // even col
                        cfa_output.at<ushort>(i, j) = in[1];
                    } else { // odd col
                        cfa_output.at<ushort>(i, j) = in[0];
                    }
                }
            }
        }
    }
}

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

int main(int argc, char** argv) {
    if (argc != 3) {
        fprintf(stderr, "Invalid Number of Arguments!\nUsage:\n");
        fprintf(stderr, "<Executable Name> <input image path> <Input LUT file> \n");
        return -1;
    }

    cv::Mat in_img, out_img, out_img_ir, ocv_ref, in_gray, diff, cfa_bayer_output, gamma_img;

    int height, width;

    /*  reading in the color image  */

    in_img = cv::imread(argv[1], -1); // read image

    height = in_img.rows;
    width = in_img.cols;

    if (in_img.data == NULL) {
        fprintf(stderr, "Cannot open image at %s\n", argv[1]);
        return 0;
    }

    cv::imwrite("in_img.png", in_img);

    cfa_bayer_output.create(in_img.rows, in_img.cols, CV_16UC1);
    gamma_img.create(in_img.rows, in_img.cols, CV_16UC1);

    unsigned short bformat = XF_BAYER_PATTERN; // Bayer format BG-0; GB-1; GR-2; RG-3

    bayerizeImage(in_img, cfa_bayer_output, bformat);
    cv::imwrite("cfa_output.png", cfa_bayer_output);

    int params[3][4][3] = {
        {{8192, 4, 0}, {22528, 16, 6144}, {34816, 64, 18432}, {65536, 512, 32768}},
        {{8192, 4, 0}, {22528, 16, 6144}, {34816, 64, 18432}, {65536, 512, 32768}},
        {{8192, 4, 0}, {22528, 16, 6144}, {34816, 64, 18432}, {65536, 512, 32768}}}; // 16 bit to 24 bit

    ap_ufixed<48, 24> params_14bit[3][4][3] = {
        {{16384, 0.03125, 0}, {65536, 0.015625, 256}, {2097152, 0.0039, 1024}, {16777215, 0.000488, 8192}},
        {{16384, 0.03125, 0}, {65536, 0.015625, 256}, {2097152, 0.0039, 1024}, {16777215, 0.000488, 8192}},
        {{16384, 0.03125, 0},
         {65536, 0.015625, 256},
         {2097152, 0.0039, 1024},
         {16777215, 0.000488, 8192}}}; // 24 bit to 14 bit

    uint32_t params_degamma[3][DEGAMMA_KP][3] = {
        {{2048, 1136, 0},
         {4096, 4105, 57344},
         {6144, 7552, 189184},
         {8192, 10994, 404367},
         {10240, 14924, 900096},
         {12288, 18318, 1405312},
         {14336, 22153, 1801090},
         {16384, 27443, 2190040}},
        {{2048, 1136, 0},
         {4096, 4105, 57344},
         {6144, 7552, 189184},
         {8192, 10994, 404367},
         {10240, 14924, 900096},
         {12288, 18318, 1405312},
         {14336, 22153, 1801090},
         {16384, 27443, 2190040}},
        {{2048, 1136, 0},
         {4096, 4105, 57344},
         {6144, 7552, 189184},
         {8192, 10994, 404367},
         {10240, 14924, 900096},
         {12288, 18318, 1405312},
         {14336, 22153, 1801090},
         {16384, 27443, 2190040}}}; // 8 knee points {upper_bound, slope, intercept} 14bit

    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            int pxl_val1 = cfa_bayer_output.at<unsigned short>(i, j);
            float val = (float)pxl_val1 / 16383;
            float val1 = (float)std::pow(val, 0.4545);
            float out_val1 = val1 * 16383.0;

            gamma_img.at<unsigned short>(i, j) = (int)out_val1;
        }
    }
    imwrite("gamma.png", gamma_img);
    size_t image_out_size_bytes, image_in_size_bytes, image_out_ir_size_bytes;

    if (USE_CSC == 0) {
        out_img.create(in_img.rows, in_img.cols, CV_8UC3);
        image_out_size_bytes = in_img.rows * in_img.cols * 3 * sizeof(unsigned char);
    }
    if (USE_CSC == 1) {
        out_img.create(in_img.rows, in_img.cols, CV_16UC1);
        image_out_size_bytes = in_img.rows * in_img.cols * 1 * sizeof(unsigned short);
    }

    out_img_ir.create(in_img.rows, in_img.cols, CV_16UC1);
    size_t vec_in_size_bytes = 256 * 3 * sizeof(unsigned char);

    image_in_size_bytes = in_img.rows * in_img.cols * sizeof(unsigned short);
    image_out_ir_size_bytes = in_img.rows * in_img.cols * 1 * sizeof(unsigned short);

    int lut_dim = 33;
    int lut_size = lut_dim * lut_dim * lut_dim * 3;
    float* lut = (float*)malloc(sizeof(float) * lut_size);
    unsigned int* casted_lut = (unsigned int*)malloc(sizeof(unsigned int) * lut_size);

    unsigned short rgain = 256;
    unsigned short bgain = 256;
    unsigned short ggain = 200;

    unsigned short pawb = 128;
    unsigned short paec = 128;

    unsigned char gamma_lut[256 * 3];

    // gtm user parameters
    float c1 = 3.0;
    float c2 = 1.5;

    unsigned int gtm_config_1 = (unsigned int)(c1 * 256);
    unsigned int gtm_config_2 = (unsigned int)(c2 * 256);
    // 6 represents 0
    // 7 represents -1
    // All other numbers represent inverse of their value raised to 2 powers (ex: -5 represents -(1/32) )

    signed char R_IR_C1_wgts[25] = {-5, -5, 6, -5, -5, 6, 1, -4, -2, 6, 6,  -5, -4,
                                    -5, 6,  6, -2, -4, 1, 6, -5, -5, 6, -5, -5};
    signed char R_IR_C2_wgts[25] = {-5, -5, 6, -5, -5, 6,  -2, -4, 1,  6, 6,  -5, -4,
                                    -5, 6,  6, 1,  -4, -2, 6,  -5, -5, 6, -5, -5};
    signed char B_at_R_wgts[25] = {3, 6, -3, 6, 3, 6, 6, 6, 6, 6, 3, 6, -1, 6, 3, 6, 6, 6, 6, 6, 3, 6, -3, 6, 3};
    signed char IR_at_R_wgts[9] = {2, 6, 2, 6, -4, 6, 2, 6, 2};
    signed char IR_at_B_wgts[9] = {2, 6, 2, 6, -4, 6, 2, 6, 2};
    signed char sub_wgts[4] = {3, 1, 2, 5};

    int roi_tlx = 0;
    int roi_tly = 0;
    int roi_brx = 127;
    int roi_bry = 127;
    int stats_size_aec = STATS_SIZE_AEC;
    int stats_size_awb = STATS_SIZE_AWB;
    int N = 8;
    int M = 8;

    // N = 0 or M = 0 is not possible so assigning it to 1 if value is 0.
    if (N == 0) {
        N = 1;
    }
    if (M == 0) {
        M = 1;
    }

    // Create a memory to hold HLS implementation output:
    std::vector<uint32_t> hlsstats_aec(stats_size_aec * NUM_OUT_CH * MAX_ROWS * MAX_COLS);
    std::vector<uint32_t> hlsstats_awb(stats_size_awb * NUM_OUT_CH * MAX_ROWS * MAX_COLS);

    ap_uint<13> final_bins_awb[FINAL_BINS_NUM] = {63, 127, 191, 255};
    // This means the
    // merged_bin[0] = bin[  0 to  63]
    // merged_bin[1] = bin[ 64 to 127]
    // merged_bin[2] = bin[127 to 191]
    // merged_bin[3] = bin[192 to 255]

    ap_uint<13> final_bins_aec[FINAL_BINS_NUM] = {1023, 2047, 3069, 4095};
    // This means the
    // merged_bin[0] = bin[  0 to  1023]
    // merged_bin[1] = bin[ 64 to 2047]
    // merged_bin[2] = bin[127 to 3069]
    // merged_bin[3] = bin[192 to 4095]
    std::string fileText;

    std::ifstream infile(argv[2]);

    if (infile.fail()) {
        fprintf(stderr, "ERROR: Cannot open input lut file %s\n ", argv[3]);
        return EXIT_FAILURE;
    }

    int idx = 0;
    float tmp_lut_val_flt = 0.0f;

    while (getline(infile, fileText)) {
        std::string spstr;
        std::stringstream ss(fileText);
        while (ss >> spstr) {
            if (idx > lut_size) {
                fprintf(stderr, "ERROR: Lut file size content larger than specified lut dimension\n ");
                return -1;
            }
            tmp_lut_val_flt = stof(spstr);
            lut[idx] = tmp_lut_val_flt;
            casted_lut[idx++] = *((unsigned int*)(&tmp_lut_val_flt));
        }
    }

    infile.close();

    int blk_height = 0;
    int blk_width = 0;

    for (int i = 0; i < 32; i++) {
        if ((1 << i) > (height / 2)) break;
        blk_height = (1 << i);
    }

    for (int i = 0; i < 32; i++) {
        if ((1 << i) > (width / 2)) break;
        blk_width = (1 << i);
    }

#if MERGE_BINS
    size_t stats_out_size_bytes_aec = N * M * NUM_OUT_CH * FINAL_BINS_NUM * sizeof(uint32_t);
    size_t stats_out_size_bytes_awb = stats_out_size_bytes_aec;
#else
    size_t stats_out_size_bytes_aec = N * M * NUM_OUT_CH * STATS_SIZE_AEC * sizeof(uint32_t);
    size_t stats_out_size_bytes_awb = N * M * NUM_OUT_CH * STATS_SIZE_AWB * sizeof(uint32_t);
#endif

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
    size_t filter1_size_bytes = 25 * sizeof(unsigned char);
    size_t filter2_size_bytes = 9 * sizeof(unsigned char);
    size_t sub_wgts_size_bytes = 4 * sizeof(unsigned char);
    size_t params_bytes = 36 * sizeof(int);
    size_t degamma_params_bytes = 3 * DEGAMMA_KP * 3 * sizeof(uint32_t);
    size_t params_14bit_bytes = 36 * sizeof(long);

    size_t lut_in_size_bytes = lut_dim * lut_dim * lut_dim * sizeof(float) * 3;
    size_t bins_size_bytes = FINAL_BINS_NUM * 3 * sizeof(unsigned int);

    float gamma_val_r = 0.5f, gamma_val_g = 0.8f, gamma_val_b = 0.8f;
    size_t ccm_matrix_int_size_bytes = 3 * 3 * sizeof(signed int);
    size_t offsetarray_int_size_bytes = 3 * sizeof(signed int);
    compute_gamma(gamma_val_r, gamma_val_g, gamma_val_b, gamma_lut);

    cl_int err;
    std::cout << "INFO: Running OpenCL section." << std::endl;

    std::vector<cl::Device> devices = xcl::get_xil_devices();
    cl::Device device = devices[0];
    OCL_CHECK(err, cl::Context context(device, NULL, NULL, NULL, &err));
    OCL_CHECK(err, cl::CommandQueue q(context, device, CL_QUEUE_PROFILING_ENABLE, &err));

    std::string device_name = device.getInfo<CL_DEVICE_NAME>();
    std::string binaryFile = xcl::find_binary_file(device_name, "krnl_ISPPipeline24bit");
    cl::Program::Binaries bins = xcl::import_binary_file(binaryFile);
    devices.resize(1);

    std::cout << "INFO: Device found - " << device_name << std::endl;
    std::cout << "Input Image Bit Depth:" << XF_DTPIXELDEPTH(XF_INP_T, XF_NPPCX) << std::endl;
    std::cout << "Input Image Channels:" << XF_CHANNELS(XF_INP_T, XF_NPPCX) << std::endl;
    std::cout << "NPPC:" << XF_NPPCX << std::endl;

    OCL_CHECK(err, cl::Program program(context, devices, bins, NULL, &err));

    // Create a kernel:
    OCL_CHECK(err, cl::Kernel kernel(program, "ISPPipeline24bit_accel", &err));

    std::vector<cl::Memory> inBufVec, outBufVec;
    OCL_CHECK(err, cl::Buffer imageToDevice(context, CL_MEM_READ_ONLY, image_in_size_bytes, NULL, &err));
    OCL_CHECK(err, cl::Buffer imageFromDevice(context, CL_MEM_WRITE_ONLY, image_out_size_bytes, NULL, &err));
    OCL_CHECK(err, cl::Buffer imageFromDevice_ir(context, CL_MEM_WRITE_ONLY, image_out_ir_size_bytes, NULL, &err));
    OCL_CHECK(err, cl::Buffer buffer_R_IR_C1(context, CL_MEM_READ_ONLY, filter1_size_bytes, NULL, &err));
    OCL_CHECK(err, cl::Buffer buffer_R_IR_C2(context, CL_MEM_READ_ONLY, filter1_size_bytes, NULL, &err));
    OCL_CHECK(err, cl::Buffer buffer_B_at_R(context, CL_MEM_READ_ONLY, filter1_size_bytes, NULL, &err));
    OCL_CHECK(err, cl::Buffer buffer_IR_at_R(context, CL_MEM_READ_ONLY, filter2_size_bytes, NULL, &err));
    OCL_CHECK(err, cl::Buffer buffer_IR_at_B(context, CL_MEM_READ_ONLY, filter2_size_bytes, NULL, &err));
    OCL_CHECK(err, cl::Buffer buffer_sub_wgts(context, CL_MEM_READ_ONLY, sub_wgts_size_bytes, NULL, &err));
    OCL_CHECK(err, cl::Buffer buffer_stats_aec(context, CL_MEM_WRITE_ONLY, stats_out_size_bytes_aec, NULL, &err));
    OCL_CHECK(err, cl::Buffer buffer_stats_awb(context, CL_MEM_WRITE_ONLY, stats_out_size_bytes_awb, NULL, &err));
    OCL_CHECK(err, cl::Buffer buffer_inBins_aec(context, CL_MEM_READ_ONLY, bins_size_bytes, NULL, &err));
    OCL_CHECK(err, cl::Buffer buffer_inBins_awb(context, CL_MEM_READ_ONLY, bins_size_bytes, NULL, &err));
    OCL_CHECK(err, cl::Buffer buffer_inLut(context, CL_MEM_READ_ONLY, lut_in_size_bytes, NULL, &err));
    OCL_CHECK(err, cl::Buffer buffer_inVec(context, CL_MEM_READ_ONLY, vec_in_size_bytes, NULL, &err));
    OCL_CHECK(err, cl::Buffer buffer_params(context, CL_MEM_READ_ONLY, params_bytes, NULL, &err));
    OCL_CHECK(err, cl::Buffer buffer_degamma_params(context, CL_MEM_READ_ONLY, degamma_params_bytes, NULL, &err));
    OCL_CHECK(err, cl::Buffer buffer_params_14bit(context, CL_MEM_READ_ONLY, params_14bit_bytes, NULL, &err));
    OCL_CHECK(err, cl::Buffer buffer_ccm_matrix_int(context, CL_MEM_READ_ONLY, ccm_matrix_int_size_bytes, NULL, &err));
    OCL_CHECK(err,
              cl::Buffer buffer_offsetarray_int(context, CL_MEM_READ_ONLY, offsetarray_int_size_bytes, NULL, &err));
    // Set the kernel arguments

    OCL_CHECK(err, err = kernel.setArg(0, imageToDevice));
    OCL_CHECK(err, err = kernel.setArg(1, imageFromDevice));
    OCL_CHECK(err, err = kernel.setArg(2, imageFromDevice_ir));
    OCL_CHECK(err, err = kernel.setArg(3, height));
    OCL_CHECK(err, err = kernel.setArg(4, width));
    OCL_CHECK(err, err = kernel.setArg(5, rgain));
    OCL_CHECK(err, err = kernel.setArg(6, bgain));
    OCL_CHECK(err, err = kernel.setArg(7, buffer_R_IR_C1));
    OCL_CHECK(err, err = kernel.setArg(8, buffer_R_IR_C2));
    OCL_CHECK(err, err = kernel.setArg(9, buffer_B_at_R));
    OCL_CHECK(err, err = kernel.setArg(10, buffer_IR_at_R));
    OCL_CHECK(err, err = kernel.setArg(11, buffer_IR_at_B));
    OCL_CHECK(err, err = kernel.setArg(12, buffer_sub_wgts));
    OCL_CHECK(err, err = kernel.setArg(13, buffer_params));
    OCL_CHECK(err, err = kernel.setArg(14, buffer_params_14bit));
    OCL_CHECK(err, err = kernel.setArg(15, buffer_degamma_params));
    OCL_CHECK(err, err = kernel.setArg(16, bformat));
    OCL_CHECK(err, err = kernel.setArg(17, buffer_stats_aec));
    OCL_CHECK(err, err = kernel.setArg(18, buffer_stats_awb));
    OCL_CHECK(err, err = kernel.setArg(19, buffer_inBins_aec));
    OCL_CHECK(err, err = kernel.setArg(20, buffer_inBins_awb));
    OCL_CHECK(err, err = kernel.setArg(21, roi_tlx));
    OCL_CHECK(err, err = kernel.setArg(22, roi_tly));
    OCL_CHECK(err, err = kernel.setArg(23, roi_brx));
    OCL_CHECK(err, err = kernel.setArg(24, roi_bry));
    OCL_CHECK(err, err = kernel.setArg(25, N));
    OCL_CHECK(err, err = kernel.setArg(26, M));
    OCL_CHECK(err, err = kernel.setArg(27, blk_height));
    OCL_CHECK(err, err = kernel.setArg(28, blk_width));
    OCL_CHECK(err, err = kernel.setArg(29, gtm_config_1));
    OCL_CHECK(err, err = kernel.setArg(30, gtm_config_2));
    OCL_CHECK(err, err = kernel.setArg(31, buffer_inVec));
    OCL_CHECK(err, err = kernel.setArg(32, buffer_inLut));
    OCL_CHECK(err, err = kernel.setArg(33, lut_dim));
    OCL_CHECK(err, err = kernel.setArg(34, pawb));
    OCL_CHECK(err, err = kernel.setArg(35, paec));
    OCL_CHECK(err, err = kernel.setArg(36, buffer_ccm_matrix_int));
    OCL_CHECK(err, err = kernel.setArg(37, buffer_offsetarray_int));
    OCL_CHECK(err, err = kernel.setArg(38, ggain));

    for (int i = 0; i < 4; i++) {
        OCL_CHECK(err, q.enqueueWriteBuffer(buffer_inVec,      // buffer on the FPGA
                                            CL_TRUE,           // blocking call
                                            0,                 // buffer offset in bytes
                                            vec_in_size_bytes, // Size in bytes
                                            gamma_lut));

        OCL_CHECK(err, q.enqueueWriteBuffer(buffer_R_IR_C1,     // buffer on the FPGA
                                            CL_TRUE,            // blocking call
                                            0,                  // buffer offset in bytes
                                            filter1_size_bytes, // Size in bytes
                                            R_IR_C1_wgts));

        OCL_CHECK(err, q.enqueueWriteBuffer(buffer_R_IR_C2,     // buffer on the FPGA
                                            CL_TRUE,            // blocking call
                                            0,                  // buffer offset in bytes
                                            filter1_size_bytes, // Size in bytes
                                            R_IR_C2_wgts));

        OCL_CHECK(err, q.enqueueWriteBuffer(buffer_B_at_R,      // buffer on the FPGA
                                            CL_TRUE,            // blocking call
                                            0,                  // buffer offset in bytes
                                            filter1_size_bytes, // Size in bytes
                                            B_at_R_wgts));

        OCL_CHECK(err, q.enqueueWriteBuffer(buffer_IR_at_R,     // buffer on the FPGA
                                            CL_TRUE,            // blocking call
                                            0,                  // buffer offset in bytes
                                            filter2_size_bytes, // Size in bytes
                                            IR_at_R_wgts));

        OCL_CHECK(err, q.enqueueWriteBuffer(buffer_IR_at_B,     // buffer on the FPGA
                                            CL_TRUE,            // blocking call
                                            0,                  // buffer offset in bytes
                                            filter2_size_bytes, // Size in bytes
                                            IR_at_B_wgts));

        OCL_CHECK(err, q.enqueueWriteBuffer(buffer_sub_wgts,     // buffer on the FPGA
                                            CL_TRUE,             // blocking call
                                            0,                   // buffer offset in bytes
                                            sub_wgts_size_bytes, // Size in bytes
                                            sub_wgts));

        OCL_CHECK(err, q.enqueueWriteBuffer(buffer_params, // buffer on the FPGA
                                            CL_TRUE,       // blocking call
                                            0,             // buffer offset in bytes
                                            params_bytes,  // Size in bytes
                                            params));

        OCL_CHECK(err, q.enqueueWriteBuffer(buffer_degamma_params, // buffer on the FPGA
                                            CL_TRUE,               // blocking call
                                            0,                     // buffer offset in bytes
                                            degamma_params_bytes,  // Size in bytes
                                            params_degamma));

        OCL_CHECK(err, q.enqueueWriteBuffer(buffer_params_14bit, // buffer on the FPGA
                                            CL_TRUE,             // blocking call
                                            0,                   // buffer offset in bytes
                                            params_14bit_bytes,  // Size in bytes
                                            params_14bit));

        OCL_CHECK(err, q.enqueueWriteBuffer(imageToDevice, CL_TRUE, 0, image_in_size_bytes, gamma_img.data));

        OCL_CHECK(err, q.enqueueWriteBuffer(buffer_inLut,      // buffer on the FPGA
                                            CL_TRUE,           // blocking call
                                            0,                 // buffer offset in bytes
                                            lut_in_size_bytes, // Size in bytes
                                            casted_lut,        // Pointer to the data to copy
                                            nullptr));

        OCL_CHECK(err, q.enqueueWriteBuffer(buffer_inBins_aec, // buffer on the FPGA
                                            CL_TRUE,           // blocking call
                                            0,                 // buffer offset in bytes
                                            bins_size_bytes,   // Size in bytes
                                            final_bins_aec,    // Pointer to the data to copy
                                            nullptr));

        OCL_CHECK(err, q.enqueueWriteBuffer(buffer_inBins_awb, // buffer on the FPGA
                                            CL_TRUE,           // blocking call
                                            0,                 // buffer offset in bytes
                                            bins_size_bytes,   // Size in bytes
                                            final_bins_awb,    // Pointer to the data to copy
                                            nullptr));
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

        // Copying Device result data to Host memory
        q.enqueueReadBuffer(imageFromDevice, CL_TRUE, 0, image_out_size_bytes, out_img.data);

        q.enqueueReadBuffer(buffer_stats_aec, // This buffers data will be read
                            CL_TRUE,          // blocking call
                            0,                // offset
                            stats_out_size_bytes_aec,
                            hlsstats_aec.data(), // Data will be stored here
                            nullptr);

        q.enqueueReadBuffer(buffer_stats_awb, // This buffers data will be read
                            CL_TRUE,          // blocking call
                            0,                // offset
                            stats_out_size_bytes_awb,
                            hlsstats_awb.data(), // Data will be stored here
                            nullptr);

        if (USE_RGBIR) {
            q.enqueueReadBuffer(imageFromDevice_ir, CL_TRUE, 0, image_out_ir_size_bytes, out_img_ir.data);
        }
    }

    q.finish();

    /////////////////////////////////////// end of CL ////////////////////////

    FILE *out_awb, *out_aec;
    out_awb = fopen("out_awb.txt", "w");
    out_aec = fopen("out_aec.txt", "w");

    int stats_size;
#if MERGE_BINS
    stats_size_aec = FINAL_BINS_NUM;
    stats_size_awb = FINAL_BINS_NUM;
#endif

    int out_ch = NUM_OUT_CH;
    int num_zones = N * M;
    // Total number of bins for all channels
    int total_bins_aec = stats_size_aec * out_ch;
    int total_bins_awb = stats_size_awb * out_ch;

    std::cout << "out_ch = " << out_ch << std::endl;
    std::cout << "stats_size_aec = " << stats_size_aec << std::endl;
    std::cout << "stats_size_awb = " << stats_size_awb << std::endl;
    std::cout << "total_bins_aec = " << total_bins_aec << std::endl;
    std::cout << "total_bins_awb = " << total_bins_awb << std::endl;

    for (int zone = 0; zone < num_zones; zone++) {
        fprintf(out_awb, "zone %d\n", zone);
        for (int cnt = 0; cnt < stats_size_awb; cnt++) {
            uint32_t bstat_hls = hlsstats_awb[(zone * total_bins_awb) + cnt];
            uint32_t gstat_hls = hlsstats_awb[(zone * total_bins_awb) + cnt + stats_size];
            uint32_t rstat_hls = hlsstats_awb[(zone * total_bins_awb) + cnt + stats_size * 2];

            // HLS Output
            fprintf(out_awb, "bin%03d %u %u %u\n", cnt, bstat_hls, gstat_hls, rstat_hls);
        }
    }
    for (int zone = 0; zone < num_zones; zone++) {
        fprintf(out_aec, "zone %d\n", zone);
        for (int cnt = 0; cnt < stats_size_aec; cnt++) {
            uint32_t stat_hls = hlsstats_aec[(zone * total_bins_aec) + cnt];

            // HLS Output
            fprintf(out_aec, "bin%03d %u\n", cnt, stat_hls);
        }
    }

    fclose(out_awb);
    fclose(out_aec);

    // Write output image
    imwrite("hls_out.png", out_img);

    std::cout << "Test Finished" << std::endl;
    return 0;
}
