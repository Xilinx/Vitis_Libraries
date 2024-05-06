/*
 * Copyright 2023 Xilinx, Inc.
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
#include "all_in_one_reference/xf_all_in_one_ref.hpp"
using namespace std;

void bayerizeImage(cv::Mat img, cv::Mat& cfa_output, int code) {
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

int g_value_com(unsigned short& value_in, float& alpha, float& ob) {
    float radiance_out = (value_in - ob) / alpha;

    return radiance_out;
}
double compute_datareliabilityweight(float& C, float& mu_h, float& mu_l, float& r) {
    double wr;

    if (r < mu_l)
        wr = exp(-C * (std::pow((r - mu_l), 2)));
    else if (r < mu_h)
        wr = 1;
    else
        wr = exp(-C * (std::pow((r - mu_h), 2)));

    return wr;
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
void wr_ocv_gen(float& alpha,
                float& optical_black_value,
                float& intersec,
                float& rho,
                float& imax,
                float* t,
                short wr_ocv[NO_EXPS][W_B_SIZE],
                double wr[NO_EXPS][W_B_SIZE]) {
    int m = NO_EXPS;

    float gamma_out[NO_EXPS] = {0.0, 0.0};
    for (int i = 0; i < m - 1; i++) {
        gamma_out[i] = (rho * (imax - optical_black_value) - optical_black_value * (imax - rho)) /
                       (t[i] * rho + t[i + 1] * (imax - rho));
    }

    float mu_h[NO_EXPS] = {0.0, 0.0};
    float mu_l[NO_EXPS] = {0.0, 0.0};

    for (int i = 0; i < m - 1; i++) {
        if (i == 0) {
            float value = (rho - optical_black_value) / alpha;
            mu_h[i] = value / t[0];
        } else {
            mu_h[i] = gamma_out[i] - (gamma_out[i - 1] - mu_h[i - 1]);
        }

        mu_l[i + 1] = 2 * gamma_out[i] - mu_h[i];
    }

    float value_max = (imax - optical_black_value) / alpha;
    mu_h[m - 1] = value_max / t[m - 1];

    float c_inters = -(log(intersec) / (std::pow((gamma_out[0] - mu_h[0]), 2)));

    // double wr[NO_EXPS][W_B_SIZE];

    FILE* fp = fopen("weights.txt", "w");
    for (int i = 0; i < NO_EXPS; i++) {
        for (int j = 0; j < (W_B_SIZE); j++) {
            float rv = (float)(j / t[i]);
            wr[i][j] = compute_datareliabilityweight(c_inters, mu_h[i], mu_l[i], rv);
            wr_ocv[i][j] = wr[i][j] * 16384;
            fprintf(fp, "%lf,", wr[i][j]);
        }
        fprintf(fp, "\n");
    }
    fclose(fp);
}

void compute_pxl(int pxl_val, int& out_val, float params[3][4][3], int color_idx) {
    if (pxl_val < params[color_idx][0][0]) {
        out_val = params[color_idx][0][1] * pxl_val + params[color_idx][0][2];
    } else if (pxl_val < params[color_idx][1][0]) {
        out_val = params[color_idx][1][1] * pxl_val + params[color_idx][1][2];
    } else if (pxl_val < params[color_idx][2][0]) {
        out_val = params[color_idx][2][1] * pxl_val + params[color_idx][2][2];
    } else {
        out_val = params[color_idx][3][1] * pxl_val + params[color_idx][3][2];
    }

    return;
}

int main(int argc, char** argv) {
    if (argc != 4) {
        fprintf(stderr, "Invalid Number of Arguments!\nUsage:\n");
        fprintf(stderr, "<Executable Name> <input image 1 (SEF) path> <input image 2 (LEF) path> <Input LUT file> \n");
        return -1;
    }

    cv::Mat in_img1, in_img2, in_img, interleaved_img, out_img, out_img_ir, ocv_ref, in_gray, diff, out_img_12bit;
    int height, width;

    if (USE_HDR_FUSION) {
#if T_8U
        in_img1 = cv::imread(argv[1], 0); // read image
        in_img2 = cv::imread(argv[2], 0); // read image
#else
        in_img1 = cv::imread(argv[1], -1); // read image
        in_img2 = cv::imread(argv[2], -1); // read image
#endif
        if (in_img1.data == NULL) {
            fprintf(stderr, "Cannot open image at %s\n", argv[1]);
            return 0;
        }
        if (in_img2.data == NULL) {
            fprintf(stderr, "Cannot open image at %s\n", argv[2]);
            return 0;
        }
        height = in_img1.rows;
        width = in_img1.cols;
    } else {
#if T_8U
        in_img = cv::imread(argv[2], 0); // read image
#else
        in_img = cv::imread(argv[2], -1); // read image
#endif
        if (in_img.data == NULL) {
            fprintf(stderr, "Cannot open image at %s\n", argv[2]);
            return 0;
        }
        height = in_img.rows;
        width = in_img.cols;
    }

    unsigned short in_width, in_height;

    unsigned short bformat = XF_BAYER_PATTERN; // Bayer format BG-0; GB-1; GR-2; RG-3
    cv::Mat cfa_bayer_output_org(in_img.rows, in_img.cols, CV_16UC1);
    cv::Mat gamma_img;
    gamma_img.create(in_img.rows, in_img.cols, CV_16UC1);
    if (USE_HDR_FUSION) {
        cv::imwrite("in_img1.png", in_img1);
        cv::imwrite("in_img2.png", in_img2);
        cv::Mat cfa_bayer_output_sef(in_img1.rows, in_img1.cols, CV_16UC1);
        cv::Mat cfa_bayer_output_lef(in_img2.rows, in_img2.cols, CV_16UC1);
        bayerizeImage(in_img1, cfa_bayer_output_sef, bformat);
        bayerizeImage(in_img2, cfa_bayer_output_lef, bformat);
        cv::imwrite("cfa_output_sef.png", cfa_bayer_output_sef);
        cv::imwrite("cfa_output_lef.png", cfa_bayer_output_lef);

        interleaved_img.create(cv::Size(cfa_bayer_output_sef.cols + NUM_H_BLANK, cfa_bayer_output_sef.rows * 2),
                               CV_IN_TYPE);

        int sc = 1;
        int cnt = 0, cnt1 = 0;
        for (int r = 0; r < height * 2; r++) {
            for (int c = 0; c < width + NUM_H_BLANK; c++) {
                if (r < NUM_V_BLANK_LINES) {
                    if (c >= NUM_H_BLANK)
                        interleaved_img.at<unsigned short>(r, c) =
                            cfa_bayer_output_lef.at<unsigned short>(r, c - NUM_H_BLANK);
                    else
                        interleaved_img.at<unsigned short>(r, c) = 0;
                }

                if (r >= NUM_V_BLANK_LINES && r <= ((2 * height) - NUM_V_BLANK_LINES)) {
                    if (r % 2 == 0) {
                        if (c >= NUM_H_BLANK)
                            interleaved_img.at<unsigned short>(r, c) =
                                cfa_bayer_output_sef.at<unsigned short>(cnt, c - NUM_H_BLANK);
                        else
                            interleaved_img.at<unsigned short>(r, c) = 0;
                    } else {
                        if (c >= NUM_H_BLANK)
                            interleaved_img.at<unsigned short>(r, c) =
                                cfa_bayer_output_lef.at<unsigned short>(cnt1 + NUM_V_BLANK_LINES - 1, c - NUM_H_BLANK);
                        else
                            interleaved_img.at<unsigned short>(r, c) = 0;
                    }
                }
                if (r >= ((2 * height) - NUM_V_BLANK_LINES)) {
                    if (c >= NUM_H_BLANK)
                        interleaved_img.at<unsigned short>(r, c) =
                            cfa_bayer_output_sef.at<unsigned short>(cnt, c - NUM_H_BLANK);
                    else
                        interleaved_img.at<unsigned short>(r, c) = 0;
                }
            }
            if (r % 2 == 0 && r >= NUM_V_BLANK_LINES) {
                cnt++;
            }
            if (r % 2 != 0 && r >= NUM_V_BLANK_LINES) {
                cnt1++;
            }
        }

        imwrite("interleaved_img.png", interleaved_img);

    } else {
        cv::imwrite("in_img.png", in_img);
        bayerizeImage(in_img, cfa_bayer_output_org, bformat);
        cv::imwrite("cfa_output_org.png", cfa_bayer_output_org);

        int out_val1;
        int pxl_val1;

        for (int i = 0; i < height; i++) {
            for (int j = 0; j < width; j++) {
#if T_8U
                pxl_val1 = cfa_bayer_output_org.at<CVTYPE>(i, j);
                float val = (float)pxl_val1 / 255;
                float val1 = (float)std::pow(val, 0.4545);
                float out_val1 = val1 * 255.0;

                gamma_img.at<CVTYPE>(i, j) = (int)out_val1;
#else
                pxl_val1 = cfa_bayer_output_org.at<CVTYPE>(i, j);
                float val = (float)pxl_val1 / 65535;
                float val1 = (float)std::pow(val, 0.4545);
                float out_val1 = val1 * 65535.0;

                gamma_img.at<CVTYPE>(i, j) = (int)out_val1;
#endif
            }
        }

        imwrite("gamma.png", gamma_img);

        // conversion from   16 bit to 12 bit
        float params_16_to_12[3][4][3] = {
            {{4096, 0.25, 0}, {8192, 0.125, 512}, {32768, 0.0625, 1024}, {65536, 0.03125, 2048}},
            {{4096, 0.25, 0}, {8192, 0.125, 512}, {32768, 0.0625, 1024}, {65536, 0.03125, 2048}},
            {{4096, 0.25, 0}, {8192, 0.125, 512}, {32768, 0.0625, 1024}, {65536, 0.03125, 2048}}};

        out_img_12bit.create(cfa_bayer_output_org.rows, cfa_bayer_output_org.cols, CV_16UC1);

        int pxl_val;
        int out_val;
        int color_idx, row_idx, col_idx;

        // Convertion of 16bit image to 12bit image
        for (int i = 0; i < height; i++) {
            for (int j = 0; j < width; j++) {
                pxl_val = gamma_img.at<CVTYPE>(i, j);
                row_idx = i;
                col_idx = j;

                if (bformat == XF_BAYER_GB) {
                    col_idx += 1;
                }

                if (bformat == XF_BAYER_GR) {
                    row_idx += 1;
                }

                if (bformat == XF_BAYER_RG) { // RGRG
                                              // GBGB
                    col_idx += 1;
                    row_idx += 1;
                }
                if ((row_idx & 1) == 0) {     // even row
                    if ((col_idx & 1) == 0) { // even col
                        color_idx = 0;        // R location
                    } else {                  // odd col
                        color_idx = 1;        // G location
                    }
                } else {                      // odd row
                    if ((col_idx & 1) == 0) { // even col
                        color_idx = 1;        // G location
                    } else {                  // odd col
                        color_idx = 2;        // B location
                    }
                }
                compute_pxl(pxl_val, out_val, params_16_to_12, color_idx);
                out_img_12bit.at<CVTYPE>(i, j) = out_val;
            }
        }

        imwrite("12_bit.png", out_img_12bit); // 12bit image
    }

    size_t image_out_size_bytes;
    size_t image_in_size_bytes;
    size_t image_out_size_bytes_decom;
    size_t image_out_size_bytes_deggama;
    size_t image_out_size_bytes_demo;

    if (USE_CSC == 0) {
        out_img.create(height, width, CV_8UC3);
        image_out_size_bytes = height * width * 3 * sizeof(unsigned char);
    }
    if (USE_CSC == 1) {
        out_img.create(height, width, CV_16UC1);
        image_out_size_bytes = height * width * 1 * sizeof(unsigned short);
    }

    out_img_ir.create(height, width, CV_16UC1);
    size_t vec_in_size_bytes = 256 * 3 * sizeof(unsigned char);
    size_t vec_weight_size_bytes = NO_EXPS * XF_NPPCX * W_B_SIZE * sizeof(short);

    if (USE_HDR_FUSION) {
        image_in_size_bytes = interleaved_img.rows * interleaved_img.cols * sizeof(CVTYPE);
    } else {
        image_in_size_bytes = height * width * sizeof(CVTYPE);
    }
    size_t image_out_ir_size_bytes = height * width * 1 * sizeof(unsigned short);

    float alpha = 1.0f;
    float optical_black_value = 0.0f;
    float intersec = 0.25f;
    float rho = 512;
    float imax = (W_B_SIZE - 1);
    float t[NO_EXPS] = {1.0f, 0.25f}; //{1.0f,0.25f,0.0625f};
    short wr_ocv[NO_EXPS][W_B_SIZE];
    double wr[NO_EXPS][W_B_SIZE];

    // wr_ocv_gen() function call
    wr_ocv_gen(alpha, optical_black_value, intersec, rho, imax, t, wr_ocv, wr);

    short wr_hls[NO_EXPS * XF_NPPCX * W_B_SIZE];

    for (int k = 0; k < XF_NPPCX; k++) {
        for (int i = 0; i < NO_EXPS; i++) {
            for (int j = 0; j < (W_B_SIZE); j++) {
                wr_hls[(i + k * NO_EXPS) * W_B_SIZE + j] = wr_ocv[i][j];
            }
        }
    }

    int lut_dim = 33;
    int lut_size = lut_dim * lut_dim * lut_dim * 3;
    float* lut = (float*)malloc(sizeof(float) * lut_size);
    unsigned int* casted_lut = (unsigned int*)malloc(sizeof(unsigned int) * lut_size);

    unsigned short rgain = 256;
    unsigned short bgain = 256;
    unsigned short ggain = 200;

    unsigned short pawb = 128;

    unsigned char gamma_lut[256 * 3];
    uint32_t hist0_awb[3][HIST_SIZE] = {0};
    uint32_t hist1_awb[3][HIST_SIZE] = {0};

    // gtm user parameters
    float c1 = 3.0;
    float c2 = 1.5;

    unsigned int gtm_config_1 = (unsigned int)(c1 * 256);
    unsigned int gtm_config_2 = (unsigned int)(c2 * 256);

    // 6 represents 0
    // 7 represents -1
    // All other numbers represent inverse of their value raised to 2 powers (ex: -5 represents -(1/32) )

    char R_IR_C1_wgts[25] = {6, 6, 6, 6, 6, 6, 6, 6, 0, 6, 6, 6, 6, 6, 6, 6, 0, 6, 6, 6, 6, 6, 6, 6, 6};

    char R_IR_C2_wgts[25] = {6, 6, 6, 6, 6, 6, 0, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 0, 6, 6, 6, 6, 6, 6};

    char B_at_R_wgts[25] = {6, 6, 0, 6, 6, 6, 6, 6, 6, 6, 0, 6, 6, 6, 0, 6, 6, 6, 6, 6, 6, 6, 0, 6, 6};

    char IR_at_R_wgts[9] = {2, 6, 2, 6, 6, 6, 2, 6, 2};

    char IR_at_B_wgts[9] = {2, 6, 2, 6, 6, 6, 2, 6, 2};

    char sub_wgts[4] = {3, 1, 2, 5};

    std::string fileText;

    std::ifstream infile(argv[3]);

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

    int params_decomand[3][4][3] = {{{1024, 4, 0}, {1536, 8, 512}, {3072, 16, 1024}, {4096, 32, 2048}},
                                    {{1024, 4, 0}, {1536, 8, 512}, {3072, 16, 1024}, {4096, 32, 2048}},
                                    {{1024, 4, 0}, {1536, 8, 512}, {3072, 16, 1024}, {4096, 32, 2048}}};

#if T_8U
    /*float params[3][N][3] = {{{64, 0.19, 0}, {128, 0.68, 31}, {192, 1.25, 104}, {256, 1.88, 225}},
		{{64, 0.19, 0}, {128, 0.68, 31}, {192, 1.25, 104}, {256, 1.88, 225}},
		{{64, 0.19, 0}, {128, 0.68, 31}, {192, 1.25, 104}, {256, 1.88, 225}}}; */ //

    ap_ufixed<32, 18> params[3][DEGAMMA_KP][3] = {{{32, 0.08, 0},
                                                   {64, 0.3, 7},
                                                   {96, 0.55, 23},
                                                   {128, 0.82, 49},
                                                   {160, 1.1, 84},
                                                   {192, 1.4, 132},
                                                   {224, 1.75, 200},
                                                   {256, 2, 256}},
                                                  {{32, 0.08, 0},
                                                   {64, 0.3, 7},
                                                   {96, 0.55, 23},
                                                   {128, 0.82, 49},
                                                   {160, 1.1, 84},
                                                   {192, 1.4, 132},
                                                   {224, 1.75, 200},
                                                   {256, 2, 256}},
                                                  {{32, 0.08, 0},
                                                   {64, 0.3, 7},
                                                   {96, 0.55, 23},
                                                   {128, 0.82, 49},
                                                   {160, 1.1, 84},
                                                   {192, 1.4, 132},
                                                   {224, 1.75, 200},
                                                   {256, 2, 256}}}; // 8 knee points {upper_bound, slope, intercept}

#endif

#if T_16U

    ap_ufixed<32, 18> params[3][DEGAMMA_KP][3] = {
        {{8192, 0.082, 0},
         {16384, 0.296, 1749},
         {24576, 0.545, 5825},
         {32768, 0.816, 12476},
         {40960, 1.1, 21782},
         {49152, 1.4, 34162},
         {57344, 1.715, 49506},
         {65536, 2.0, 65536}},
        {{8192, 0.082, 0},
         {16384, 0.296, 1749},
         {24576, 0.545, 5825},
         {32768, 0.816, 12476},
         {40960, 1.1, 21782},
         {49152, 1.4, 34162},
         {57344, 1.715, 49506},
         {65536, 2.0, 65536}},
        {{8192, 0.082, 0},
         {16384, 0.296, 1749},
         {24576, 0.545, 5825},
         {32768, 0.816, 12476},
         {40960, 1.1, 21782},
         {49152, 1.4, 34162},
         {57344, 1.715, 49506},
         {65536, 2.0, 65536}}}; // 8 knee points {upper_bound, slope, intercept}
#endif
    // degamma config

    /* center colomn is in Q18_14 format */

    unsigned int params_degamma[3][DEGAMMA_KP][3];
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < DEGAMMA_KP; j++) {
            for (int k = 0; k < 3; k++) {
                if (k == 1) {
                    params_degamma[i][j][k] = (unsigned int)((params[i][j][k]) * (16384));
                } else {
                    params_degamma[i][j][k] = (unsigned int)((params[i][j][k]));
                }
            }
        }
    }

    ///////////////////////////all in one reference section//////////////
    cv::Mat final_ocv;

#if USE_CSC
    final_ocv.create(height, width, CV_16UC1);
#else
    final_ocv.create(height, width, CV_8UC3);
#endif

#if DEBUG
    cv::Mat HDR_ref_out;
    HDR_ref_out.create(height, width, CV_IN_TYPE);

    cv::Mat rggb_out_ref;
    rggb_out_ref.create(height, width, CV_IN_TYPE);

    cv::Mat aec_ref_out;
    aec_ref_out.create(height, width, CV_IN_TYPE);

    cv::Mat blacklevel_out_ref;
    blacklevel_out_ref.create(height, width, CV_IN_TYPE);

    cv::Mat bpc_ref;
    bpc_ref.create(height, width, CV_IN_TYPE);

    cv::Mat degamma_out_ref;
    degamma_out_ref.create(height, width, CV_IN_TYPE);

    cv::Mat lsc_out_ref;
    lsc_out_ref.create(height, width, CV_IN_TYPE);

    cv::Mat demosaic_out_ref(height, width, CV_OUT_TYPE);

    cv::Mat awb_out_ref;
    awb_out_ref.create(height, width, CV_OUT_TYPE);

    cv::Mat gaincontrol_out_ref;
    gaincontrol_out_ref.create(height, width, CV_IN_TYPE);

    cv::Mat ccm_out_ref(height, width, CV_OUT_TYPE);

    cv::Mat tm_out_ref;
    tm_out_ref.create(height, width, CV_GTM_TYPE);

    cv::Mat gamma_out_ref;
    gamma_out_ref.create(height, width, CV_GTM_TYPE);

    cv::Mat lut3d_out_ref;
    lut3d_out_ref.create(height, width, CV_GTM_TYPE);

    cv::Mat yuyv_out_ref;

#if USE_CSC
    yuyv_out_ref.create(height, width, CV_16UC1);
#else
    yuyv_out_ref.create(height, width, CV_8UC3);
#endif

    all_in_one_ref(out_img_12bit, wr, params_decomand, height, width, pawb, rgain, bgain, ggain, blk_height, blk_width,
                   lut_dim, lut, final_ocv, HDR_ref_out, rggb_out_ref, aec_ref_out, blacklevel_out_ref, bpc_ref,
                   degamma_out_ref, lsc_out_ref, gaincontrol_out_ref, demosaic_out_ref, awb_out_ref, ccm_out_ref,
                   tm_out_ref, gamma_out_ref, lut3d_out_ref, yuyv_out_ref);

#else
    all_in_one_ref(out_img_12bit, wr, params_decomand, height, width, pawb, rgain, bgain, ggain, blk_height, blk_width,
                   lut_dim, lut, final_ocv);
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

    ///////////////////////////////////// CL ////////////////////////
    size_t filter1_in_size_bytes = 25 * sizeof(unsigned char);
    size_t filter2_in_size_bytes = 9 * sizeof(unsigned char);
    size_t sub_wgts_in_size_bytes = 4 * sizeof(unsigned char);
    size_t decompand_params_in_size_bytes = 36 * sizeof(int);
    size_t degamma_params_in_size_bytes = 3 * DEGAMMA_KP * 3 * sizeof(unsigned int);

    size_t ir_image_out_size_bytes = height * width * 1 * sizeof(CVTYPE);
    size_t lut_in_size_bytes = lut_dim * lut_dim * lut_dim * sizeof(float) * 3;

    size_t ccm_matrix_int_size_bytes = 3 * 3 * sizeof(signed int);
    size_t offsetarray_int_size_bytes = 3 * sizeof(signed int);

    float gamma_val_r = 0.5f, gamma_val_g = 0.8f, gamma_val_b = 0.8f;

    std::cout << "INPUT_PTR_WIDTH :" << INPUT_PTR_WIDTH << std::endl;
    std::cout << "OUTPUT_PTR_WIDTH :" << OUTPUT_PTR_WIDTH << std::endl;
    std::cout << "LUT_PTR_WIDTH :" << LUT_PTR_WIDTH << std::endl;
    std::cout << "XF_NPPCX :" << XF_NPPCX << std::endl;

    compute_gamma(gamma_val_r, gamma_val_g, gamma_val_b, gamma_lut);

    cl_int err;
    std::cout << "INFO: Running OpenCL section." << std::endl;

    std::vector<cl::Device> devices = xcl::get_xil_devices();
    cl::Device device = devices[0];
    OCL_CHECK(err, cl::Context context(device, NULL, NULL, NULL, &err));
    OCL_CHECK(err, cl::CommandQueue q(context, device, CL_QUEUE_PROFILING_ENABLE, &err));

    std::string device_name = device.getInfo<CL_DEVICE_NAME>();
    std::string binaryFile = xcl::find_binary_file(device_name, "krnl_ISPPipeline");
    cl::Program::Binaries bins = xcl::import_binary_file(binaryFile);
    devices.resize(1);

    OCL_CHECK(err, cl::Program program(context, devices, bins, NULL, &err));

    // Create a kernel:
    OCL_CHECK(err, cl::Kernel kernel(program, "ISPPipeline_accel", &err));

    std::vector<cl::Memory> inBufVec, outBufVec;
    cout << "size " << image_in_size_bytes << endl;
    cout << "height " << height << " " << width << endl;
    OCL_CHECK(err, cl::Buffer imageToDevice(context, CL_MEM_READ_ONLY, image_in_size_bytes, NULL, &err));
    OCL_CHECK(err, cl::Buffer imageFromDevice(context, CL_MEM_WRITE_ONLY, image_out_size_bytes, NULL, &err));
    OCL_CHECK(err, cl::Buffer imageFromDevice_ir(context, CL_MEM_WRITE_ONLY, image_out_ir_size_bytes, NULL, &err));
    OCL_CHECK(err, cl::Buffer buffer_inLut(context, CL_MEM_READ_ONLY, lut_in_size_bytes, NULL, &err));
    OCL_CHECK(err, cl::Buffer buffer_inVec(context, CL_MEM_READ_ONLY, vec_in_size_bytes, NULL, &err));
    OCL_CHECK(err, cl::Buffer buffer_inVec_Weights(context, CL_MEM_READ_ONLY, vec_weight_size_bytes, NULL, &err));
    OCL_CHECK(err, cl::Buffer buffer_R_IR_C1(context, CL_MEM_READ_ONLY, filter1_in_size_bytes, NULL, &err));
    OCL_CHECK(err, cl::Buffer buffer_R_IR_C2(context, CL_MEM_READ_ONLY, filter1_in_size_bytes, NULL, &err));
    OCL_CHECK(err, cl::Buffer buffer_B_at_R(context, CL_MEM_READ_ONLY, filter1_in_size_bytes, NULL, &err));
    OCL_CHECK(err, cl::Buffer buffer_IR_at_R(context, CL_MEM_READ_ONLY, filter2_in_size_bytes, NULL, &err));
    OCL_CHECK(err, cl::Buffer buffer_IR_at_B(context, CL_MEM_READ_ONLY, filter2_in_size_bytes, NULL, &err));
    OCL_CHECK(err, cl::Buffer buffer_sub_wgts(context, CL_MEM_READ_ONLY, sub_wgts_in_size_bytes, NULL, &err));
    OCL_CHECK(
        err, cl::Buffer buffer_decompand_params(context, CL_MEM_READ_ONLY, decompand_params_in_size_bytes, NULL, &err));
    OCL_CHECK(err,
              cl::Buffer buffer_degamma_params(context, CL_MEM_READ_ONLY, degamma_params_in_size_bytes, NULL, &err));
    OCL_CHECK(err, cl::Buffer buffer_ccm_matrix_int(context, CL_MEM_READ_ONLY, ccm_matrix_int_size_bytes, NULL, &err));
    OCL_CHECK(err,
              cl::Buffer buffer_offsetarray_int(context, CL_MEM_READ_ONLY, offsetarray_int_size_bytes, NULL, &err));

    // Set the kernel arguments

    OCL_CHECK(err, err = kernel.setArg(0, imageToDevice));
    OCL_CHECK(err, err = kernel.setArg(1, imageFromDevice));
    OCL_CHECK(err, err = kernel.setArg(2, imageFromDevice_ir));
    OCL_CHECK(err, err = kernel.setArg(3, height));
    OCL_CHECK(err, err = kernel.setArg(4, width));
    OCL_CHECK(err, err = kernel.setArg(5, buffer_inVec_Weights));
    OCL_CHECK(err, err = kernel.setArg(6, buffer_decompand_params));
    OCL_CHECK(err, err = kernel.setArg(7, buffer_R_IR_C1));
    OCL_CHECK(err, err = kernel.setArg(8, buffer_R_IR_C2));
    OCL_CHECK(err, err = kernel.setArg(9, buffer_B_at_R));
    OCL_CHECK(err, err = kernel.setArg(10, buffer_IR_at_R));
    OCL_CHECK(err, err = kernel.setArg(11, buffer_IR_at_B));
    OCL_CHECK(err, err = kernel.setArg(12, buffer_sub_wgts));
    OCL_CHECK(err, err = kernel.setArg(13, pawb));
    OCL_CHECK(err, err = kernel.setArg(14, bformat));
    OCL_CHECK(err, err = kernel.setArg(15, buffer_degamma_params));
    OCL_CHECK(err, err = kernel.setArg(16, rgain));
    OCL_CHECK(err, err = kernel.setArg(17, bgain));
    OCL_CHECK(err, err = kernel.setArg(18, blk_height));
    OCL_CHECK(err, err = kernel.setArg(19, blk_width));
    OCL_CHECK(err, err = kernel.setArg(20, gtm_config_1));
    OCL_CHECK(err, err = kernel.setArg(21, gtm_config_2));
    OCL_CHECK(err, err = kernel.setArg(22, buffer_inVec));
    OCL_CHECK(err, err = kernel.setArg(23, buffer_inLut));
    OCL_CHECK(err, err = kernel.setArg(24, lut_dim));
    OCL_CHECK(err, err = kernel.setArg(25, buffer_ccm_matrix_int));
    OCL_CHECK(err, err = kernel.setArg(26, buffer_offsetarray_int));
    OCL_CHECK(err, err = kernel.setArg(27, ggain));

    double exec_sum = 0.0f;
    double avg_runtime = 0.0f;
    int loop_count = 4;
    for (int i = 0; i < loop_count; i++) {
        OCL_CHECK(err, q.enqueueWriteBuffer(buffer_inVec,      // buffer on the FPGA
                                            CL_TRUE,           // blocking call
                                            0,                 // buffer offset in bytes
                                            vec_in_size_bytes, // Size in bytes
                                            gamma_lut));

        OCL_CHECK(err, q.enqueueWriteBuffer(buffer_R_IR_C1,        // buffer on the FPGA
                                            CL_TRUE,               // blocking call
                                            0,                     // buffer offset in bytes
                                            filter1_in_size_bytes, // Size in bytes
                                            R_IR_C1_wgts));

        OCL_CHECK(err, q.enqueueWriteBuffer(buffer_R_IR_C2,        // buffer on the FPGA
                                            CL_TRUE,               // blocking call
                                            0,                     // buffer offset in bytes
                                            filter1_in_size_bytes, // Size in bytes
                                            R_IR_C2_wgts));

        OCL_CHECK(err, q.enqueueWriteBuffer(buffer_B_at_R,         // buffer on the FPGA
                                            CL_TRUE,               // blocking call
                                            0,                     // buffer offset in bytes
                                            filter1_in_size_bytes, // Size in bytes
                                            B_at_R_wgts));

        OCL_CHECK(err, q.enqueueWriteBuffer(buffer_IR_at_R,        // buffer on the FPGA
                                            CL_TRUE,               // blocking call
                                            0,                     // buffer offset in bytes
                                            filter2_in_size_bytes, // Size in bytes
                                            IR_at_R_wgts));

        OCL_CHECK(err, q.enqueueWriteBuffer(buffer_IR_at_B,        // buffer on the FPGA
                                            CL_TRUE,               // blocking call
                                            0,                     // buffer offset in bytes
                                            filter2_in_size_bytes, // Size in bytes
                                            IR_at_B_wgts));

        OCL_CHECK(err, q.enqueueWriteBuffer(buffer_sub_wgts,        // buffer on the FPGA
                                            CL_TRUE,                // blocking call
                                            0,                      // buffer offset in bytes
                                            sub_wgts_in_size_bytes, // Size in bytes
                                            sub_wgts));

        OCL_CHECK(err, q.enqueueWriteBuffer(buffer_decompand_params,        // buffer on the FPGA
                                            CL_TRUE,                        // blocking call
                                            0,                              // buffer offset in bytes
                                            decompand_params_in_size_bytes, // Size in bytes
                                            params_decomand));

        OCL_CHECK(err, q.enqueueWriteBuffer(buffer_degamma_params,        // buffer on the FPGA
                                            CL_TRUE,                      // blocking call
                                            0,                            // buffer offset in bytes
                                            degamma_params_in_size_bytes, // Size in bytes
                                            params_degamma));

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

        if (USE_HDR_FUSION) {
            OCL_CHECK(err, q.enqueueWriteBuffer(buffer_inVec_Weights,  // buffer on the FPGA
                                                CL_TRUE,               // blocking call
                                                0,                     // buffer offset in bytes
                                                vec_weight_size_bytes, // Size in bytes
                                                wr_hls));

            OCL_CHECK(err, q.enqueueWriteBuffer(imageToDevice, CL_TRUE, 0, image_in_size_bytes, interleaved_img.data));

        }

        else {
            OCL_CHECK(err, q.enqueueWriteBuffer(imageToDevice, CL_TRUE, 0, image_in_size_bytes, out_img_12bit.data));
        }

        OCL_CHECK(err, q.enqueueWriteBuffer(buffer_inLut,      // buffer on the FPGA
                                            CL_TRUE,           // blocking call
                                            0,                 // buffer offset in bytes
                                            lut_in_size_bytes, // Size in bytes
                                            casted_lut,        // Pointer to the data to copy
                                            nullptr));
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
        std::cout << (diff_prof / 1000000) << std::endl;
        exec_sum = exec_sum + diff_prof;

        // Copying Device result data to Host memory
        q.enqueueReadBuffer(imageFromDevice, CL_TRUE, 0, image_out_size_bytes, out_img.data);
        if (USE_RGBIR) {
            q.enqueueReadBuffer(imageFromDevice_ir, CL_TRUE, 0, image_out_ir_size_bytes, out_img_ir.data);
        }
    }

    q.finish();
    std::cout << "exec_sum =" << exec_sum << std::endl;
    avg_runtime = exec_sum / loop_count;
    std::cout << "avg_runtime =" << (avg_runtime / 1000000) << "ms" << std::endl;
    /////////////////////////////////////// end of CL ////////////////////////

    imwrite("hls_out.png", out_img);

    if (USE_RGBIR) {
        imwrite("hls_out_ir.png", out_img_ir);
    }
    cv::Mat diff_testing;
    diff_testing.create(height, width, out_img.type());

    std::cout << "testing kernel error" << std::endl;
    cv::absdiff(out_img, final_ocv, diff_testing);

    float err_per_testing;
    xf::cv::analyzeDiff(diff_testing, ERROR_THRESHOLD, err_per_testing);

    std::cout << "Test Finished" << std::endl;
    return 0;
}
