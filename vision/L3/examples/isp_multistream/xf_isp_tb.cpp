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

struct ispparams_config {
    unsigned short rgain = 256;
    unsigned short bgain = 256;
    unsigned short ggain = 256;
    unsigned short pawb = 128;
    unsigned short bayer_p = 2; // bayer pattern GR
    unsigned short black_level = 32;
    unsigned short height = 168;
    unsigned short width = 256;
    unsigned short blk_height = 32;
    unsigned short blk_width = 32;
    unsigned short lut_dim = 33;
};

void bayerizeImage(cv::Mat img, cv::Mat& cfa_output, unsigned short code) {
    for (int i = 0; i < img.rows; i++) {
        for (int j = 0; j < img.cols; j++) {
            cv::Vec3w in = img.at<cv::Vec3w>(i, j);
            if (code == 0) {            // BG
                if ((i & 1) == 0) {     // even row
                    if ((j & 1) == 0) { // even col
                        cfa_output.at<PXL_TYPE>(i, j) = in[0];
                    } else { // odd col
                        cfa_output.at<PXL_TYPE>(i, j) = in[1];
                    }
                } else {                // odd row
                    if ((j & 1) == 0) { // even col
                        cfa_output.at<PXL_TYPE>(i, j) = in[1];
                    } else { // odd col
                        cfa_output.at<PXL_TYPE>(i, j) = in[2];
                    }
                }
            }
            if (code == 1) {            // GB
                if ((i & 1) == 0) {     // even row
                    if ((j & 1) == 0) { // even col
                        cfa_output.at<PXL_TYPE>(i, j) = in[1];
                    } else { // odd col
                        cfa_output.at<PXL_TYPE>(i, j) = in[0];
                    }
                } else {                // odd row
                    if ((j & 1) == 0) { // even col
                        cfa_output.at<PXL_TYPE>(i, j) = in[2];
                    } else { // odd col
                        cfa_output.at<PXL_TYPE>(i, j) = in[1];
                    }
                }
            }
            if (code == 2) {            // GR
                if ((i & 1) == 0) {     // even row
                    if ((j & 1) == 0) { // even col
                        cfa_output.at<PXL_TYPE>(i, j) = in[1];
                    } else { // odd col
                        cfa_output.at<PXL_TYPE>(i, j) = in[2];
                    }
                } else {                // odd row
                    if ((j & 1) == 0) { // even col
                        cfa_output.at<PXL_TYPE>(i, j) = in[0];
                    } else { // odd col
                        cfa_output.at<PXL_TYPE>(i, j) = in[1];
                    }
                }
            }
            if (code == 3) {            // RG
                if ((i & 1) == 0) {     // even row
                    if ((j & 1) == 0) { // even col
                        cfa_output.at<PXL_TYPE>(i, j) = in[2];
                    } else { // odd col
                        cfa_output.at<PXL_TYPE>(i, j) = in[1];
                    }
                } else {                // odd row
                    if ((j & 1) == 0) { // even col
                        cfa_output.at<PXL_TYPE>(i, j) = in[1];
                    } else { // odd col
                        cfa_output.at<PXL_TYPE>(i, j) = in[0];
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
                short wr_ocv[NO_EXPS][W_B_SIZE]) {
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

    double wr[NO_EXPS][W_B_SIZE];

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

void function_lut(std::string fileText,
                  std::ifstream& infile,
                  int idx,
                  float* lut,
                  unsigned int* casted_lut,
                  int lut_size,
                  float tmp_lut_val_flt) {
    while (getline(infile, fileText)) {
        std::string spstr;
        std::stringstream ss(fileText);
        while (ss >> spstr) {
            if (idx > lut_size) {
                fprintf(stderr, "ERROR: Lut file size content larger than specified lut dimension\n ");
                //                return -1;
            }
            tmp_lut_val_flt = stof(spstr);
            lut[idx] = tmp_lut_val_flt;
            casted_lut[idx++] = *((unsigned int*)(&tmp_lut_val_flt));
        }
    }
    infile.close();
}

void compute_pxl_16to12(int pxl_val, int& out_val, float params[3][4][3], int color_idx) {
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

void degma_image(cv::Mat img_16bit,
                 cv::Mat img_16_nonlnr,
                 int pxl_val1,
                 float val,
                 float val1,
                 float out_val1,
                 int height,
                 int width) {
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
#if T_8U
            pxl_val1 = img_16bit.at<unsigned char>(i, j);
            float val = (float)pxl_val1 / 255;
            float val1 = (float)std::pow(val, 0.4545);
            float out_val1 = val1 * 255.0;

            img_16_nonlnr.at<unsigned char>(i, j) = (int)out_val1;
#else
            pxl_val1 = img_16bit.at<unsigned short>(i, j);
            float val = (float)pxl_val1 / 65535;
            float val1 = (float)std::pow(val, 0.4545);
            float out_val1 = val1 * 65535.0;

            img_16_nonlnr.at<unsigned short>(i, j) = (int)out_val1;
#endif
        }
    }
}

void get_image(cv::Mat img_16bit,
               cv::Mat img_12bit,
               int height,
               int width,
               int pxl_val,
               int out_val,
               int color_idx,
               int row_idx,
               int col_idx,
               unsigned short bayer_p,
               float parms_16to12[3][4][3]) {
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            pxl_val = img_16bit.at<unsigned short>(i, j);
            row_idx = i;
            col_idx = j;

            if (bayer_p == XF_BAYER_GB) {
                col_idx += 1;
            }

            if (bayer_p == XF_BAYER_GR) {
                row_idx += 1;
            }

            if (bayer_p == 3) { // 0 to 3
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

            compute_pxl_16to12(pxl_val, out_val, parms_16to12, color_idx);
            img_12bit.at<unsigned short>(i, j) = out_val;
        }
    }
}

int main(int argc, char** argv) {
    cv::Mat in_img1, in_img2, in_img3, in_img4, in_img5, in_img6, in_img7, in_img8, interleaved_img1, interleaved_img2,
        interleaved_img3, interleaved_img4, diff, out_img1_12bit, out_img2_12bit, out_img3_12bit, out_img4_12bit;

    unsigned short in_width, in_height;
    int height, width;

    if (USE_HDR_FUSION) {
        if (argc != 10) {
            fprintf(stderr, "Invalid Number of Arguments!\nUsage:\n");
            fprintf(stderr, "<Executable Name> <input image path> <Input LUT file>\n");
            return -1;
        }

#if T_8U

        in_img1 = cv::imread(argv[1], 0);
        in_img2 = cv::imread(argv[2], 0);
        in_img3 = cv::imread(argv[3], 0);
        in_img4 = cv::imread(argv[4], 0);
        in_img5 = cv::imread(argv[5], 0);
        in_img6 = cv::imread(argv[6], 0);
        in_img7 = cv::imread(argv[7], 0);
        in_img8 = cv::imread(argv[8], 0);

#else
        in_img1 = cv::imread(argv[1], -1);
        in_img2 = cv::imread(argv[2], -1);
        in_img3 = cv::imread(argv[3], -1);
        in_img4 = cv::imread(argv[4], -1);
        in_img5 = cv::imread(argv[5], -1);
        in_img6 = cv::imread(argv[6], -1);
        in_img7 = cv::imread(argv[7], -1);
        in_img8 = cv::imread(argv[8], -1);

#endif
        if (in_img1.data == NULL) {
            fprintf(stderr, "Cannot open image 1 at %s\n", argv[1]);
            return 0;
        }
        if (in_img2.data == NULL) {
            fprintf(stderr, "Cannot open image 2 at %s\n", argv[2]);
            return 0;
        }

        if (in_img3.data == NULL) {
            fprintf(stderr, "Cannot open image 3 at %s\n", argv[3]);
            return 0;
        }
        if (in_img4.data == NULL) {
            fprintf(stderr, "Cannot open image 4 at %s\n", argv[4]);
            return 0;
        }
        if (in_img5.data == NULL) {
            fprintf(stderr, "Cannot open image 5 at %s\n", argv[5]);
            return 0;
        }
        if (in_img6.data == NULL) {
            fprintf(stderr, "Cannot open image 6 at %s\n", argv[6]);
            return 0;
        }

        if (in_img7.data == NULL) {
            fprintf(stderr, "Cannot open image 7 at %s\n", argv[7]);
            return 0;
        }
        if (in_img8.data == NULL) {
            fprintf(stderr, "Cannot open image 8 at %s\n", argv[8]);
            return 0;
        }
        height = in_img1.rows;
        width = in_img1.cols;
        imwrite("input1.png", in_img1);
        imwrite("input2.png", in_img2);
        imwrite("input3.png", in_img3);
        imwrite("input4.png", in_img4);
        imwrite("input5.png", in_img5);
        imwrite("input6.png", in_img6);
        imwrite("input7.png", in_img7);
        imwrite("input8.png", in_img8);

    } else {
        if (argc != 6) {
            fprintf(stderr, "Invalid Number of Arguments!\nUsage:\n");
            fprintf(stderr, "<Executable Name> <input image path> <Input LUT file>\n");
            return -1;
        }

#if T_8U

        in_img1 = cv::imread(argv[1], 0);
        in_img2 = cv::imread(argv[2], 0);
        in_img3 = cv::imread(argv[3], 0);
        in_img4 = cv::imread(argv[4], 0);

#else
        in_img1 = cv::imread(argv[1], -1);
        in_img2 = cv::imread(argv[2], -1);
        in_img3 = cv::imread(argv[3], -1);
        in_img4 = cv::imread(argv[4], -1);

#endif
        if (in_img1.data == NULL) {
            fprintf(stderr, "Cannot open image 1 at %s\n", argv[1]);
            return 0;
        }
        if (in_img2.data == NULL) {
            fprintf(stderr, "Cannot open image 2 at %s\n", argv[2]);
            return 0;
        }

        if (in_img3.data == NULL) {
            fprintf(stderr, "Cannot open image 3 at %s\n", argv[3]);
            return 0;
        }
        if (in_img4.data == NULL) {
            fprintf(stderr, "Cannot open image 4 at %s\n", argv[4]);
            return 0;
        }
        height = in_img1.rows;
        width = in_img1.cols;
        imwrite("input1.png", in_img1);
        imwrite("input2.png", in_img2);
        imwrite("input3.png", in_img3);
        imwrite("input4.png", in_img4);
    }
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
    signed int ccm_matrix_int[NUM_STREAMS][3][3];
    signed int offsetarray_int[NUM_STREAMS][3];

    for (int n = 0; n < NUM_STREAMS; n++) {
        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 3; j++) {
                ccm_matrix_int[n][i][j] = (signed int)(ccm_matrix[i][j] * 1048576);
            }
            offsetarray_int[n][i] = (signed int)(offsetarray[i] * 1048576);
        }
    }

    size_t ccm_matrix_int_size_bytes = NUM_STREAMS * 3 * 3 * sizeof(signed int);
    size_t offsetarray_int_size_bytes = NUM_STREAMS * 3 * sizeof(signed int);
    /////////////////////////////////////// CL ////////////////////////
    float alpha = 1.0f;
    float optical_black_value = 0.0f;
    float intersec = 0.25f;
    float rho = 512;
    float imax = (W_B_SIZE - 1);
    float t[NO_EXPS] = {1.0f, 0.25f}; //{1.0f,0.25f,0.0625f};

    short wr_ocv[NUM_STREAMS][NO_EXPS][W_B_SIZE];

    // wr_ocv_gen function call
    for (int i = 0; i < NUM_STREAMS; i++) {
        wr_ocv_gen(alpha, optical_black_value, intersec, rho, imax, t, wr_ocv[i]);
    }

    short wr_hls[NUM_STREAMS][NO_EXPS * XF_NPPC * W_B_SIZE];
    for (int n = 0; n < NUM_STREAMS; n++) {
        for (int k = 0; k < XF_NPPC; k++) {
            for (int i = 0; i < NO_EXPS; i++) {
                for (int j = 0; j < (W_B_SIZE); j++) {
                    wr_hls[n][(i + k * NO_EXPS) * W_B_SIZE + j] = wr_ocv[n][i][j];
                }
            }
        }
    }

    unsigned char gamma_lut[NUM_STREAMS][256 * 3];
    uint32_t hist0_awb[NUM_STREAMS][3][HIST_SIZE] = {0};
    uint32_t hist1_awb[NUM_STREAMS][3][HIST_SIZE] = {0};

    uint32_t hist0_aec[NUM_STREAMS][AEC_HIST_SIZE] = {0};
    uint32_t hist1_aec[NUM_STREAMS][AEC_HIST_SIZE] = {0};

    float gamma_val_r = 0.5f, gamma_val_g = 0.8f, gamma_val_b = 0.8f;

    for (int n_strm = 0; n_strm < NUM_STREAMS; n_strm++) {
        compute_gamma(gamma_val_r, gamma_val_g, gamma_val_b, gamma_lut[n_strm]);
    }

    float c1[NUM_STREAMS], c2[NUM_STREAMS];
    unsigned int c1_int[NUM_STREAMS], c2_int[NUM_STREAMS];

    for (int i = 0; i < NUM_STREAMS; i++) {
        c1[i] = 3.0;
        c2[i] = 1.5;
        c1_int[i] = c1[i] * 256;
        c2_int[i] = c2[i] * 256;
    }

    signed char R_IR_C1_wgts[NUM_STREAMS][25] = {
        {6, 6, 6, 6, 6, 6, 6, 6, 0, 6, 6, 6, 6, 6, 6, 6, 0, 6, 6, 6, 6, 6, 6, 6, 6},
        {6, 6, 6, 6, 6, 6, 6, 6, 0, 6, 6, 6, 6, 6, 6, 6, 0, 6, 6, 6, 6, 6, 6, 6, 6},
        {6, 6, 6, 6, 6, 6, 6, 6, 0, 6, 6, 6, 6, 6, 6, 6, 0, 6, 6, 6, 6, 6, 6, 6, 6},
        {6, 6, 6, 6, 6, 6, 6, 6, 0, 6, 6, 6, 6, 6, 6, 6, 0, 6, 6, 6, 6, 6, 6, 6, 6}};

    signed char R_IR_C2_wgts[NUM_STREAMS][25] = {
        {6, 6, 6, 6, 6, 6, 0, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 0, 6, 6, 6, 6, 6, 6},
        {6, 6, 6, 6, 6, 6, 0, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 0, 6, 6, 6, 6, 6, 6},
        {6, 6, 6, 6, 6, 6, 0, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 0, 6, 6, 6, 6, 6, 6},
        {6, 6, 6, 6, 6, 6, 0, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 0, 6, 6, 6, 6, 6, 6}};

    signed char B_at_R_wgts[NUM_STREAMS][25] = {
        {6, 6, 0, 6, 6, 6, 6, 6, 6, 6, 0, 6, 6, 6, 0, 6, 6, 6, 6, 6, 6, 6, 0, 6, 6},
        {6, 6, 0, 6, 6, 6, 6, 6, 6, 6, 0, 6, 6, 6, 0, 6, 6, 6, 6, 6, 6, 6, 0, 6, 6},
        {6, 6, 0, 6, 6, 6, 6, 6, 6, 6, 0, 6, 6, 6, 0, 6, 6, 6, 6, 6, 6, 6, 0, 6, 6},
        {6, 6, 0, 6, 6, 6, 6, 6, 6, 6, 0, 6, 6, 6, 0, 6, 6, 6, 6, 6, 6, 6, 0, 6, 6}};

    signed char IR_at_R_wgts[NUM_STREAMS][9] = {{2, 6, 2, 6, 6, 6, 2, 6, 2},
                                                {2, 6, 2, 6, 6, 6, 2, 6, 2},
                                                {2, 6, 2, 6, 6, 6, 2, 6, 2},
                                                {2, 6, 2, 6, 6, 6, 2, 6, 2}};

    signed char IR_at_B_wgts[NUM_STREAMS][9] = {{2, 6, 2, 6, 6, 6, 2, 6, 2},
                                                {2, 6, 2, 6, 6, 6, 2, 6, 2},
                                                {2, 6, 2, 6, 6, 6, 2, 6, 2},
                                                {2, 6, 2, 6, 6, 6, 2, 6, 2}};

    signed char sub_wgts[NUM_STREAMS][4] = {{3, 1, 2, 5}, {3, 1, 2, 5}, {3, 1, 2, 5}, {3, 1, 2, 5}};

    size_t filter1_in_size_bytes = NUM_STREAMS * 25 * sizeof(unsigned char);
    size_t filter2_in_size_bytes = NUM_STREAMS * 9 * sizeof(unsigned char);
    size_t sub_wgts_in_size_bytes = NUM_STREAMS * 4 * sizeof(unsigned char);

    struct ispparams_config params[NUM_STREAMS];
    unsigned short array_params[NUM_STREAMS][11];
    size_t lut_in_size_bytes = 0;

    for (int i = 0; i < NUM_STREAMS; i++) {
        array_params[i][0] = params[i].rgain;
        array_params[i][1] = params[i].bgain;
        array_params[i][2] = params[i].ggain;
        array_params[i][3] = params[i].pawb;
        array_params[i][4] = params[i].bayer_p;
        array_params[i][5] = params[i].black_level;
        array_params[i][6] = params[i].height;
        array_params[i][7] = params[i].width;
        array_params[i][8] = params[i].blk_height;
        array_params[i][9] = params[i].blk_width;
        array_params[i][10] = params[i].lut_dim;
    }
    int lut_size = params[0].lut_dim * params[0].lut_dim * params[0].lut_dim * 3;
    lut_in_size_bytes += lut_size * sizeof(float);

    float* lut1 = (float*)malloc(sizeof(float) * lut_size);
    unsigned int* casted_lut1 = (unsigned int*)malloc(sizeof(unsigned int) * lut_size);

    std::string fileText1;
    std::ifstream infile1(argv[5]);

    if (infile1.fail()) {
        fprintf(stderr, "ERROR: Cannot open input lut file 1 %s\n ", argv[5]);
        return EXIT_FAILURE;
    }

    int idx1 = 0;
    float tmp_lut_val_flt = 0.0f;
    function_lut(fileText1, infile1, idx1, lut1, casted_lut1, lut_size, tmp_lut_val_flt);

    //////////Decompanding///////////

    // This is sent to accel
    int dcp_params_12to16[NUM_STREAMS][3][4][3];

    int dc_params12to16[3][4][3] = {{{1024, 4, 0}, {1536, 8, 512}, {3072, 16, 1024}, {4096, 32, 2048}},
                                    {{1024, 4, 0}, {1536, 8, 512}, {3072, 16, 1024}, {4096, 32, 2048}},
                                    {{1024, 4, 0}, {1536, 8, 512}, {3072, 16, 1024}, {4096, 32, 2048}}};

    for (int n = 0; n < NUM_STREAMS; n++) {
        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 4; j++) {
                for (int k = 0; k < 3; k++) {
                    dcp_params_12to16[n][i][j][k] = dc_params12to16[i][j][k];
                }
            }
        }
    }

    float dcp_params_16to12[NUM_STREAMS][3][4][3];

    float dc_params_16to12[3][4][3] = {
        {{4096, 0.25, 0}, {8192, 0.125, 512}, {32768, 0.0625, 1024}, {65536, 0.03125, 2048}},
        {{4096, 0.25, 0}, {8192, 0.125, 512}, {32768, 0.0625, 1024}, {65536, 0.03125, 2048}},
        {{4096, 0.25, 0}, {8192, 0.125, 512}, {32768, 0.0625, 1024}, {65536, 0.03125, 2048}}};

    for (int n = 0; n < NUM_STREAMS; n++) {
        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 4; j++) {
                for (int k = 0; k < 3; k++) {
                    dcp_params_16to12[n][i][j][k] = dc_params_16to12[i][j][k];
                }
            }
        }
    }

/////////////DEGAMMA///////////////////////
// degamma config

/* center colomn is in Q18_14 format */
#if T_8U
    /*float params[3][N][3] = {{{64, 0.19, 0}, {128, 0.68, 31}, {192, 1.25, 104}, {256, 1.88, 225}},
		{{64, 0.19, 0}, {128, 0.68, 31}, {192, 1.25, 104}, {256, 1.88, 225}},
		{{64, 0.19, 0}, {128, 0.68, 31}, {192, 1.25, 104}, {256, 1.88, 225}}}; */ //

    ap_ufixed<32, 18> params_fixed[3][DGAMMA_KP][3] = {
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

    ap_ufixed<32, 18> params_fixed[3][DGAMMA_KP][3] = {
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

    unsigned int params_degamma[3][DGAMMA_KP][3];
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < DGAMMA_KP; j++) {
            for (int k = 0; k < 3; k++) {
                if (k == 1) {
                    params_degamma[i][j][k] = (unsigned int)((params_fixed[i][j][k]) * (16384));
                } else {
                    params_degamma[i][j][k] = (unsigned int)((params_fixed[i][j][k]));
                }
            }
        }
    }

    unsigned int dgam_params[NUM_STREAMS][3][DGAMMA_KP][3];

    for (int n = 0; n < NUM_STREAMS; n++) {
        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < DGAMMA_KP; j++) {
                for (int k = 0; k < 3; k++) {
                    dgam_params[n][i][j][k] = params_degamma[i][j][k];
                }
            }
        }
    }

    /////////////////////////////////////
    int dg_pxl_val1;
    float dg_val;
    float dg_val1;
    float dg_out_val1;
    int pxl_val;
    int out_val;
    int color_idx, row_idx, col_idx;

    if (USE_HDR_FUSION) {
        cv::Mat cfa_bayer_output_org1(in_img1.rows, in_img1.cols, CV_16UC1);
        cv::Mat cfa_bayer_output_org2(in_img2.rows, in_img2.cols, CV_16UC1);
        cv::Mat cfa_bayer_output_org3(in_img3.rows, in_img3.cols, CV_16UC1);
        cv::Mat cfa_bayer_output_org4(in_img4.rows, in_img4.cols, CV_16UC1);
        cv::Mat cfa_bayer_output_org5(in_img5.rows, in_img5.cols, CV_16UC1);
        cv::Mat cfa_bayer_output_org6(in_img6.rows, in_img6.cols, CV_16UC1);
        cv::Mat cfa_bayer_output_org7(in_img7.rows, in_img7.cols, CV_16UC1);
        cv::Mat cfa_bayer_output_org8(in_img8.rows, in_img8.cols, CV_16UC1);

        bayerizeImage(in_img1, cfa_bayer_output_org1, params[0].bayer_p);
        bayerizeImage(in_img2, cfa_bayer_output_org2, params[1].bayer_p);
        bayerizeImage(in_img3, cfa_bayer_output_org3, params[2].bayer_p);
        bayerizeImage(in_img4, cfa_bayer_output_org4, params[3].bayer_p);
        bayerizeImage(in_img5, cfa_bayer_output_org5, params[0].bayer_p);
        bayerizeImage(in_img6, cfa_bayer_output_org6, params[1].bayer_p);
        bayerizeImage(in_img7, cfa_bayer_output_org7, params[2].bayer_p);
        bayerizeImage(in_img8, cfa_bayer_output_org8, params[3].bayer_p);

        cv::imwrite("bayer_16bit_1.png", cfa_bayer_output_org1);
        cv::imwrite("bayer_16bit_2.png", cfa_bayer_output_org2);
        cv::imwrite("bayer_16bit_3.png", cfa_bayer_output_org3);
        cv::imwrite("bayer_16bit_4.png", cfa_bayer_output_org4);
        cv::imwrite("bayer_16bit_5.png", cfa_bayer_output_org5);
        cv::imwrite("bayer_16bit_6.png", cfa_bayer_output_org6);
        cv::imwrite("bayer_16bit_7.png", cfa_bayer_output_org7);
        cv::imwrite("bayer_16bit_8.png", cfa_bayer_output_org8);

        interleaved_img1.create(cv::Size(cfa_bayer_output_org1.cols + NUM_H_BLANK, cfa_bayer_output_org1.rows * 2),
                                CV_16UC1);
        interleaved_img2.create(cv::Size(cfa_bayer_output_org1.cols + NUM_H_BLANK, cfa_bayer_output_org1.rows * 2),
                                CV_16UC1);
        interleaved_img3.create(cv::Size(cfa_bayer_output_org1.cols + NUM_H_BLANK, cfa_bayer_output_org1.rows * 2),
                                CV_16UC1);
        interleaved_img4.create(cv::Size(cfa_bayer_output_org1.cols + NUM_H_BLANK, cfa_bayer_output_org1.rows * 2),
                                CV_16UC1);

        int sc = 1;
        int cnt = 0, cnt1 = 0;
        for (int r = 0; r < height * 2; r++) {
            for (int c = 0; c < width + NUM_H_BLANK; c++) {
                if (r < NUM_V_BLANK_LINES) {
                    if (c >= NUM_H_BLANK) {
                        interleaved_img1.at<CVTYPE>(r, c) = cfa_bayer_output_org1.at<CVTYPE>(r, c - NUM_H_BLANK);
                        interleaved_img2.at<CVTYPE>(r, c) = cfa_bayer_output_org3.at<CVTYPE>(r, c - NUM_H_BLANK);
                        interleaved_img3.at<CVTYPE>(r, c) = cfa_bayer_output_org5.at<CVTYPE>(r, c - NUM_H_BLANK);
                        interleaved_img4.at<CVTYPE>(r, c) = cfa_bayer_output_org7.at<CVTYPE>(r, c - NUM_H_BLANK);
                    } else {
                        interleaved_img1.at<CVTYPE>(r, c) = 0;
                        interleaved_img2.at<CVTYPE>(r, c) = 0;
                        interleaved_img3.at<CVTYPE>(r, c) = 0;
                        interleaved_img4.at<CVTYPE>(r, c) = 0;
                    }
                }

                if (r >= NUM_V_BLANK_LINES && r <= ((2 * height) - NUM_V_BLANK_LINES)) {
                    if (r % 2 == 0) {
                        if (c >= NUM_H_BLANK) {
                            interleaved_img1.at<CVTYPE>(r, c) = cfa_bayer_output_org2.at<CVTYPE>(cnt, c - NUM_H_BLANK);
                            interleaved_img2.at<CVTYPE>(r, c) = cfa_bayer_output_org4.at<CVTYPE>(cnt, c - NUM_H_BLANK);
                            interleaved_img3.at<CVTYPE>(r, c) = cfa_bayer_output_org6.at<CVTYPE>(cnt, c - NUM_H_BLANK);
                            interleaved_img4.at<CVTYPE>(r, c) = cfa_bayer_output_org8.at<CVTYPE>(cnt, c - NUM_H_BLANK);
                        } else {
                            interleaved_img1.at<CVTYPE>(r, c) = 0;
                            interleaved_img2.at<CVTYPE>(r, c) = 0;
                            interleaved_img3.at<CVTYPE>(r, c) = 0;
                            interleaved_img4.at<CVTYPE>(r, c) = 0;
                        }
                    } else {
                        if (c >= NUM_H_BLANK) {
                            interleaved_img1.at<CVTYPE>(r, c) =
                                cfa_bayer_output_org1.at<CVTYPE>(cnt1 + NUM_V_BLANK_LINES - 1, c - NUM_H_BLANK);
                            interleaved_img2.at<CVTYPE>(r, c) =
                                cfa_bayer_output_org3.at<CVTYPE>(cnt1 + NUM_V_BLANK_LINES - 1, c - NUM_H_BLANK);
                            interleaved_img3.at<CVTYPE>(r, c) =
                                cfa_bayer_output_org5.at<CVTYPE>(cnt1 + NUM_V_BLANK_LINES - 1, c - NUM_H_BLANK);
                            interleaved_img4.at<CVTYPE>(r, c) =
                                cfa_bayer_output_org7.at<CVTYPE>(cnt1 + NUM_V_BLANK_LINES - 1, c - NUM_H_BLANK);
                        }

                        else {
                            interleaved_img1.at<CVTYPE>(r, c) = 0;
                            interleaved_img2.at<CVTYPE>(r, c) = 0;
                            interleaved_img3.at<CVTYPE>(r, c) = 0;
                            interleaved_img4.at<CVTYPE>(r, c) = 0;
                        }
                    }
                }
                if (r >= ((2 * height) - NUM_V_BLANK_LINES)) {
                    if (c >= NUM_H_BLANK) {
                        interleaved_img1.at<CVTYPE>(r, c) = cfa_bayer_output_org2.at<CVTYPE>(cnt, c - NUM_H_BLANK);
                        interleaved_img2.at<CVTYPE>(r, c) = cfa_bayer_output_org4.at<CVTYPE>(cnt, c - NUM_H_BLANK);
                        interleaved_img3.at<CVTYPE>(r, c) = cfa_bayer_output_org6.at<CVTYPE>(cnt, c - NUM_H_BLANK);
                        interleaved_img4.at<CVTYPE>(r, c) = cfa_bayer_output_org8.at<CVTYPE>(cnt, c - NUM_H_BLANK);
                    } else {
                        interleaved_img1.at<CVTYPE>(r, c) = 0;
                        interleaved_img2.at<CVTYPE>(r, c) = 0;
                        interleaved_img3.at<CVTYPE>(r, c) = 0;
                        interleaved_img4.at<CVTYPE>(r, c) = 0;
                    }
                }
            }
            if (r % 2 == 0 && r >= NUM_V_BLANK_LINES) {
                cnt++;
            }
            if (r % 2 != 0 && r >= NUM_V_BLANK_LINES) {
                cnt1++;
            }
        }
        imwrite("interleaved_img1.png", interleaved_img1);
        imwrite("interleaved_img2.png", interleaved_img2);
        imwrite("interleaved_img3.png", interleaved_img3);
        imwrite("interleaved_img4.png", interleaved_img4);
    } else {
        cv::Mat cfa_bayer_output_org1(in_img1.rows, in_img1.cols, CV_16UC1);
        cv::Mat cfa_bayer_output_org2(in_img2.rows, in_img2.cols, CV_16UC1);
        cv::Mat cfa_bayer_output_org3(in_img3.rows, in_img3.cols, CV_16UC1);
        cv::Mat cfa_bayer_output_org4(in_img4.rows, in_img4.cols, CV_16UC1);

        bayerizeImage(in_img1, cfa_bayer_output_org1, params[0].bayer_p);
        bayerizeImage(in_img2, cfa_bayer_output_org2, params[1].bayer_p);
        bayerizeImage(in_img3, cfa_bayer_output_org3, params[2].bayer_p);
        bayerizeImage(in_img4, cfa_bayer_output_org4, params[3].bayer_p);

        cv::imwrite("bayer_16bit_1.png", cfa_bayer_output_org1);
        cv::imwrite("bayer_16bit_1.png", cfa_bayer_output_org2);
        cv::imwrite("bayer_16bit_1.png", cfa_bayer_output_org3);
        cv::imwrite("bayer_16bit_1.png", cfa_bayer_output_org4);

        cv::Mat gamma_out1, gamma_out2, gamma_out3, gamma_out4;

        gamma_out1.create(in_img1.rows, in_img1.cols, CV_16UC1);
        gamma_out2.create(in_img2.rows, in_img2.cols, CV_16UC1);
        gamma_out3.create(in_img3.rows, in_img3.cols, CV_16UC1);
        gamma_out4.create(in_img4.rows, in_img4.cols, CV_16UC1);

        out_img1_12bit.create(gamma_out1.rows, gamma_out1.cols, CV_16UC1);
        out_img2_12bit.create(gamma_out2.rows, gamma_out2.cols, CV_16UC1);
        out_img3_12bit.create(gamma_out3.rows, gamma_out3.cols, CV_16UC1);
        out_img4_12bit.create(gamma_out4.rows, gamma_out4.cols, CV_16UC1);

        degma_image(cfa_bayer_output_org1, gamma_out1, dg_pxl_val1, dg_val, dg_val1, dg_out_val1, height, width);
        degma_image(cfa_bayer_output_org2, gamma_out2, dg_pxl_val1, dg_val, dg_val1, dg_out_val1, height, width);
        degma_image(cfa_bayer_output_org3, gamma_out3, dg_pxl_val1, dg_val, dg_val1, dg_out_val1, height, width);
        degma_image(cfa_bayer_output_org4, gamma_out4, dg_pxl_val1, dg_val, dg_val1, dg_out_val1, height, width);

        imwrite("gamma1.png", gamma_out1);
        imwrite("gamma2.png", gamma_out2);
        imwrite("gamma3.png", gamma_out3);
        imwrite("gamma4.png", gamma_out4);

        get_image(gamma_out1, out_img1_12bit, height, width, pxl_val, out_val, color_idx, row_idx, col_idx,
                  params[0].bayer_p, dcp_params_16to12[0]);

        get_image(gamma_out2, out_img2_12bit, height, width, pxl_val, out_val, color_idx, row_idx, col_idx,
                  params[1].bayer_p, dcp_params_16to12[1]);

        get_image(gamma_out3, out_img3_12bit, height, width, pxl_val, out_val, color_idx, row_idx, col_idx,
                  params[2].bayer_p, dcp_params_16to12[2]);

        get_image(gamma_out4, out_img4_12bit, height, width, pxl_val, out_val, color_idx, row_idx, col_idx,
                  params[3].bayer_p, dcp_params_16to12[3]);

        imwrite("12_bit1.png", out_img1_12bit);
        imwrite("12_bit2.png", out_img2_12bit);
        imwrite("12_bit3.png", out_img3_12bit);
        imwrite("12_bit4.png", out_img4_12bit);
    }
    cv::Mat out_img1(in_img1.rows, in_img1.cols, CV_OUT_TYPE);
    cv::Mat out_img2(in_img2.rows, in_img2.cols, CV_OUT_TYPE);
    cv::Mat out_img3(in_img3.rows, in_img3.cols, CV_OUT_TYPE);
    cv::Mat out_img4(in_img4.rows, in_img4.cols, CV_OUT_TYPE);

    cv::Mat out_img_ir1(in_img1.rows, in_img1.cols, CV_16UC1);
    cv::Mat out_img_ir2(in_img2.rows, in_img2.cols, CV_16UC1);
    cv::Mat out_img_ir3(in_img3.rows, in_img3.cols, CV_16UC1);
    cv::Mat out_img_ir4(in_img4.rows, in_img4.cols, CV_16UC1);

    size_t image_in_size_bytes;
    size_t image_out_size_bytes;
    size_t vec_in_size_bytes = NUM_STREAMS * 256 * 3 * sizeof(unsigned char);
    size_t vec_weight_size_bytes = NUM_STREAMS * NO_EXPS * XF_NPPC * W_B_SIZE * sizeof(short);

    if (USE_HDR_FUSION) {
        image_in_size_bytes = interleaved_img1.rows * interleaved_img1.cols * sizeof(CVTYPE);
    } else {
        image_in_size_bytes = in_img1.rows * in_img2.cols * sizeof(CVTYPE);
    }

    if (USE_CSC == 0) {
        image_out_size_bytes = height * width * 3 * sizeof(unsigned char);
    }
    if (USE_CSC == 1) {
        image_out_size_bytes = height * width * 1 * sizeof(unsigned short);
    }

    size_t image_out_ir_size_bytes = in_img1.rows * in_img1.cols * 1 * sizeof(unsigned short);
    size_t dcp_params_in_size_bytes = NUM_STREAMS * 36 * sizeof(int);
    size_t dgam_params_in_size_bytes = NUM_STREAMS * 3 * DGAMMA_KP * 3 * sizeof(int);
    size_t array_size_bytes = NUM_STREAMS * 11 * sizeof(unsigned short);
    size_t c1_size_bytes = NUM_STREAMS * sizeof(float);
    size_t c2_size_bytes = NUM_STREAMS * sizeof(float);
    size_t c1_int_size_bytes = NUM_STREAMS * sizeof(unsigned int);
    size_t c2_int_size_bytes = NUM_STREAMS * sizeof(unsigned int);

    cl_int err;
    std::cout << "INFO: Running OpenCL section." << std::endl;

    std::vector<cl::Device> devices = xcl::get_xil_devices();
    cl::Device device = devices[0];
    OCL_CHECK(err, cl::Context context(device, NULL, NULL, NULL, &err));
    OCL_CHECK(err, cl::CommandQueue q(context, device, CL_QUEUE_PROFILING_ENABLE, &err));

    std::string device_name = device.getInfo<CL_DEVICE_NAME>();
    std::string binaryFile = xcl::find_binary_file(device_name, "krnl_ISPPipeline");
    cl::Program::Binaries bins = xcl::import_binary_file(binaryFile);
    std::cout << "INFO: import_binary_file done." << std::endl;
    devices.resize(1);

    OCL_CHECK(err, cl::Program program(context, devices, bins, NULL, &err));
    // Create a kernel:
    OCL_CHECK(err, cl::Kernel kernel(program, "ISPPipeline_accel", &err));
    std::vector<cl::Memory> inBufVec, outBufVec;

    OCL_CHECK(err, cl::Buffer buffer_inImage1(context, CL_MEM_READ_ONLY, image_in_size_bytes, NULL, &err));
    OCL_CHECK(err, cl::Buffer buffer_inImage2(context, CL_MEM_READ_ONLY, image_in_size_bytes, NULL, &err));
    OCL_CHECK(err, cl::Buffer buffer_inImage3(context, CL_MEM_READ_ONLY, image_in_size_bytes, NULL, &err));
    OCL_CHECK(err, cl::Buffer buffer_inImage4(context, CL_MEM_READ_ONLY, image_in_size_bytes, NULL, &err));
    OCL_CHECK(err, cl::Buffer buffer_outImage1(context, CL_MEM_WRITE_ONLY, image_out_size_bytes, NULL, &err));
    OCL_CHECK(err, cl::Buffer buffer_outImage2(context, CL_MEM_WRITE_ONLY, image_out_size_bytes, NULL, &err));
    OCL_CHECK(err, cl::Buffer buffer_outImage3(context, CL_MEM_WRITE_ONLY, image_out_size_bytes, NULL, &err));
    OCL_CHECK(err, cl::Buffer buffer_outImage4(context, CL_MEM_WRITE_ONLY, image_out_size_bytes, NULL, &err));
    OCL_CHECK(err, cl::Buffer buffer_inVec(context, CL_MEM_READ_ONLY, vec_in_size_bytes, NULL, &err));
    OCL_CHECK(err, cl::Buffer buffer_inVec_Weights(context, CL_MEM_READ_ONLY, vec_weight_size_bytes, NULL, &err));
    OCL_CHECK(err, cl::Buffer buffer_array(context, CL_MEM_READ_ONLY, array_size_bytes, NULL, &err));
    OCL_CHECK(err, cl::Buffer buffer_c1(context, CL_MEM_READ_ONLY, c1_int_size_bytes, NULL, &err));
    OCL_CHECK(err, cl::Buffer buffer_c2(context, CL_MEM_READ_ONLY, c2_int_size_bytes, NULL, &err));

    OCL_CHECK(err, cl::Buffer buffer_IRoutImage1(context, CL_MEM_WRITE_ONLY, image_out_ir_size_bytes, NULL, &err));
    OCL_CHECK(err, cl::Buffer buffer_IRoutImage2(context, CL_MEM_WRITE_ONLY, image_out_ir_size_bytes, NULL, &err));
    OCL_CHECK(err, cl::Buffer buffer_IRoutImage3(context, CL_MEM_WRITE_ONLY, image_out_ir_size_bytes, NULL, &err));
    OCL_CHECK(err, cl::Buffer buffer_IRoutImage4(context, CL_MEM_WRITE_ONLY, image_out_ir_size_bytes, NULL, &err));

    OCL_CHECK(err, cl::Buffer buffer_R_IR_C1(context, CL_MEM_READ_ONLY, filter1_in_size_bytes, NULL, &err));
    OCL_CHECK(err, cl::Buffer buffer_R_IR_C2(context, CL_MEM_READ_ONLY, filter1_in_size_bytes, NULL, &err));
    OCL_CHECK(err, cl::Buffer buffer_B_at_R(context, CL_MEM_READ_ONLY, filter1_in_size_bytes, NULL, &err));
    OCL_CHECK(err, cl::Buffer buffer_IR_at_R(context, CL_MEM_READ_ONLY, filter2_in_size_bytes, NULL, &err));
    OCL_CHECK(err, cl::Buffer buffer_IR_at_B(context, CL_MEM_READ_ONLY, filter2_in_size_bytes, NULL, &err));
    OCL_CHECK(err, cl::Buffer buffer_sub_wgts(context, CL_MEM_READ_ONLY, sub_wgts_in_size_bytes, NULL, &err));
    OCL_CHECK(err, cl::Buffer buffer_inLut1(context, CL_MEM_READ_ONLY, lut_in_size_bytes, NULL, &err));
    OCL_CHECK(err, cl::Buffer buffer_inLut2(context, CL_MEM_READ_ONLY, lut_in_size_bytes, NULL, &err));
    OCL_CHECK(err, cl::Buffer buffer_inLut3(context, CL_MEM_READ_ONLY, lut_in_size_bytes, NULL, &err));
    OCL_CHECK(err, cl::Buffer buffer_inLut4(context, CL_MEM_READ_ONLY, lut_in_size_bytes, NULL, &err));
    OCL_CHECK(err, cl::Buffer buffer_dgam_params(context, CL_MEM_READ_ONLY, dgam_params_in_size_bytes, NULL, &err));
    OCL_CHECK(err, cl::Buffer buffer_decompand_params(context, CL_MEM_READ_ONLY, dcp_params_in_size_bytes, NULL, &err));

    OCL_CHECK(err, cl::Buffer buffer_ccm_matrix_int(context, CL_MEM_READ_ONLY, ccm_matrix_int_size_bytes, NULL, &err));
    OCL_CHECK(err,
              cl::Buffer buffer_offsetarray_int(context, CL_MEM_READ_ONLY, offsetarray_int_size_bytes, NULL, &err));

    // Set the kernel arguments
    OCL_CHECK(err, err = kernel.setArg(0, buffer_inImage1));
    OCL_CHECK(err, err = kernel.setArg(1, buffer_inImage2));
    OCL_CHECK(err, err = kernel.setArg(2, buffer_inImage3));
    OCL_CHECK(err, err = kernel.setArg(3, buffer_inImage4));
    OCL_CHECK(err, err = kernel.setArg(4, buffer_outImage1));
    OCL_CHECK(err, err = kernel.setArg(5, buffer_outImage2));
    OCL_CHECK(err, err = kernel.setArg(6, buffer_outImage3));
    OCL_CHECK(err, err = kernel.setArg(7, buffer_outImage4));
    OCL_CHECK(err, err = kernel.setArg(8, buffer_IRoutImage1));
    OCL_CHECK(err, err = kernel.setArg(9, buffer_IRoutImage2));
    OCL_CHECK(err, err = kernel.setArg(10, buffer_IRoutImage3));
    OCL_CHECK(err, err = kernel.setArg(11, buffer_IRoutImage4));
    OCL_CHECK(err, err = kernel.setArg(12, buffer_inVec_Weights));
    OCL_CHECK(err, err = kernel.setArg(13, buffer_decompand_params));
    OCL_CHECK(err, err = kernel.setArg(14, buffer_R_IR_C1));
    OCL_CHECK(err, err = kernel.setArg(15, buffer_R_IR_C2));
    OCL_CHECK(err, err = kernel.setArg(16, buffer_B_at_R));
    OCL_CHECK(err, err = kernel.setArg(17, buffer_IR_at_R));
    OCL_CHECK(err, err = kernel.setArg(18, buffer_IR_at_B));
    OCL_CHECK(err, err = kernel.setArg(19, buffer_sub_wgts));
    OCL_CHECK(err, err = kernel.setArg(20, buffer_dgam_params));
    OCL_CHECK(err, err = kernel.setArg(21, buffer_c1));
    OCL_CHECK(err, err = kernel.setArg(22, buffer_c2));
    OCL_CHECK(err, err = kernel.setArg(23, buffer_array));
    OCL_CHECK(err, err = kernel.setArg(24, buffer_inVec));
    OCL_CHECK(err, err = kernel.setArg(25, buffer_inLut1));
    OCL_CHECK(err, err = kernel.setArg(26, buffer_inLut2));
    OCL_CHECK(err, err = kernel.setArg(27, buffer_inLut3));
    OCL_CHECK(err, err = kernel.setArg(28, buffer_inLut4));
    OCL_CHECK(err, err = kernel.setArg(29, buffer_ccm_matrix_int));
    OCL_CHECK(err, err = kernel.setArg(30, buffer_offsetarray_int));

    for (int i = 0; i < 4; i++) {
        OCL_CHECK(err, q.enqueueWriteBuffer(buffer_inVec_Weights,  // buffer on the FPGA
                                            CL_TRUE,               // blocking call
                                            0,                     // buffer offset in bytes
                                            vec_weight_size_bytes, // Size in bytes
                                            wr_hls));

        OCL_CHECK(err, q.enqueueWriteBuffer(buffer_decompand_params,  // buffer on the FPGA
                                            CL_TRUE,                  // blocking call
                                            0,                        // buffer offset in bytes
                                            dcp_params_in_size_bytes, // Size in bytes
                                            dcp_params_12to16));

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
        OCL_CHECK(err, q.enqueueWriteBuffer(buffer_dgam_params,        // buffer on the FPGA
                                            CL_TRUE,                   // blocking call
                                            0,                         // buffer offset in bytes
                                            dgam_params_in_size_bytes, // Size in bytes
                                            dgam_params));

        OCL_CHECK(err, q.enqueueWriteBuffer(buffer_c1,         // buffer on the FPGA
                                            CL_TRUE,           // blocking call
                                            0,                 // buffer offset in bytes
                                            c1_int_size_bytes, // Size in bytes
                                            c1));
        OCL_CHECK(err, q.enqueueWriteBuffer(buffer_c2,         // buffer on the FPGA
                                            CL_TRUE,           // blocking call
                                            0,                 // buffer offset in bytes
                                            c2_int_size_bytes, // Size in bytes
                                            c2));

        OCL_CHECK(err, q.enqueueWriteBuffer(buffer_array,     // buffer on the FPGA
                                            CL_TRUE,          // blocking call
                                            0,                // buffer offset in bytes
                                            array_size_bytes, // Size in bytes
                                            array_params));

        OCL_CHECK(err, q.enqueueWriteBuffer(buffer_inVec,      // buffer on the FPGA
                                            CL_TRUE,           // blocking call
                                            0,                 // buffer offset in bytes
                                            vec_in_size_bytes, // Size in bytes
                                            gamma_lut));
        OCL_CHECK(err, q.enqueueWriteBuffer(buffer_inLut1,     // buffer on the FPGA
                                            CL_TRUE,           // blocking call
                                            0,                 // buffer offset in bytes
                                            lut_in_size_bytes, // Size in bytes
                                            casted_lut1,       // Pointer to the data to copy
                                            nullptr));

        OCL_CHECK(err, q.enqueueWriteBuffer(buffer_inLut2,     // buffer on the FPGA
                                            CL_TRUE,           // blocking call
                                            0,                 // buffer offset in bytes
                                            lut_in_size_bytes, // Size in bytes
                                            casted_lut1,       // Pointer to the data to copy
                                            nullptr));

        OCL_CHECK(err, q.enqueueWriteBuffer(buffer_inLut3,     // buffer on the FPGA
                                            CL_TRUE,           // blocking call
                                            0,                 // buffer offset in bytes
                                            lut_in_size_bytes, // Size in bytes
                                            casted_lut1,       // Pointer to the data to copy
                                            nullptr));
        OCL_CHECK(err, q.enqueueWriteBuffer(buffer_inLut4,     // buffer on the FPGA
                                            CL_TRUE,           // blocking call
                                            0,                 // buffer offset in bytes
                                            lut_in_size_bytes, // Size in bytes
                                            casted_lut1,       // Pointer to the data to copy
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

        if (USE_HDR_FUSION) {
            OCL_CHECK(err,
                      q.enqueueWriteBuffer(buffer_inImage1, CL_TRUE, 0, image_in_size_bytes, interleaved_img1.data));
            OCL_CHECK(err,
                      q.enqueueWriteBuffer(buffer_inImage2, CL_TRUE, 0, image_in_size_bytes, interleaved_img2.data));
            OCL_CHECK(err,
                      q.enqueueWriteBuffer(buffer_inImage3, CL_TRUE, 0, image_in_size_bytes, interleaved_img3.data));
            OCL_CHECK(err,
                      q.enqueueWriteBuffer(buffer_inImage4, CL_TRUE, 0, image_in_size_bytes, interleaved_img4.data));
        } else {
            OCL_CHECK(err, q.enqueueWriteBuffer(buffer_inImage1, CL_TRUE, 0, image_in_size_bytes, out_img1_12bit.data));
            OCL_CHECK(err, q.enqueueWriteBuffer(buffer_inImage2, CL_TRUE, 0, image_in_size_bytes, out_img2_12bit.data));
            OCL_CHECK(err, q.enqueueWriteBuffer(buffer_inImage3, CL_TRUE, 0, image_in_size_bytes, out_img3_12bit.data));
            OCL_CHECK(err, q.enqueueWriteBuffer(buffer_inImage4, CL_TRUE, 0, image_in_size_bytes, out_img4_12bit.data));
        }

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
        q.enqueueReadBuffer(buffer_outImage1, CL_TRUE, 0, image_out_size_bytes, out_img1.data);
        q.enqueueReadBuffer(buffer_outImage2, CL_TRUE, 0, image_out_size_bytes, out_img2.data);
        q.enqueueReadBuffer(buffer_outImage3, CL_TRUE, 0, image_out_size_bytes, out_img3.data);
        q.enqueueReadBuffer(buffer_outImage4, CL_TRUE, 0, image_out_size_bytes, out_img4.data);

        if (USE_RGBIR) {
            q.enqueueReadBuffer(buffer_IRoutImage1, CL_TRUE, 0, image_out_ir_size_bytes, out_img_ir1.data);
            q.enqueueReadBuffer(buffer_IRoutImage2, CL_TRUE, 0, image_out_ir_size_bytes, out_img_ir2.data);
            q.enqueueReadBuffer(buffer_IRoutImage3, CL_TRUE, 0, image_out_ir_size_bytes, out_img_ir3.data);
            q.enqueueReadBuffer(buffer_IRoutImage4, CL_TRUE, 0, image_out_ir_size_bytes, out_img_ir4.data);
        }
    }
    q.finish();
    /////////////////////////////////////// end of CL ////////////////////////

    // Write output image
    imwrite("hls_out1.png", out_img1);
    imwrite("hls_out2.png", out_img2);
    imwrite("hls_out3.png", out_img3);
    imwrite("hls_out4.png", out_img4);

    if (USE_RGBIR) {
        imwrite("hls_out_ir0.png", out_img_ir1);
        imwrite("hls_out_ir1.png", out_img_ir2);
        imwrite("hls_out_ir2.png", out_img_ir3);
        imwrite("hls_out_ir3.png", out_img_ir4);
    }
    std::cout << "Test Finished" << std::endl;

    return 0;
}
