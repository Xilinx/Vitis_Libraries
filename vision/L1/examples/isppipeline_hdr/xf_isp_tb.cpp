/*
 * Copyright (C) 2022 - 2023, Advanced Micro Devices, Inc.
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
#include "xf_isp_types.h"

using namespace std;

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

/*********************************************************************************
 * Function:    Mat2MultiBayerAXIvideo
 * Parameters:
 * Return:
 * Description:  Currently fixed for Dual Pixel
 **********************************************************************************/
static void Mat2MultiBayerAXIvideo(cv::Mat& img, InVideoStrm_t& AXI_video_strm, unsigned char InColorFormat) {
    int i, j, k, l;

#if T_8U || T_10U || T_12U
    unsigned char cv_pix;
#else
    unsigned short cv_pix;
#endif
    ap_axiu<AXI_WIDTH_IN, 1, 1, 1> axi;
    int depth = XF_DTPIXELDEPTH(XF_SRC_T, XF_NPPC);

    for (i = 0; i < img.rows; i++) {
        for (j = 0; j < img.cols; j += XF_NPPC) {
            if ((i == 0) && (j == 0)) {
                axi.user = 1;
            } else {
                axi.user = 0;
            }
            if (j == (img.cols - XF_NPPC)) {
                axi.last = 1;
            } else {
                axi.last = 0;
            }
            axi.data = -1;
            for (l = 0; l < XF_NPPC; l++) {
                if (img.depth() == CV_16U)
                    cv_pix = img.at<unsigned short>(i, j + l);
                else
                    cv_pix = img.at<unsigned char>(i, j + l);
                switch (depth) {
                    case 10:
                        xf::cv::AXISetBitFields(axi, (l)*depth, depth, (unsigned char)cv_pix);
                        break;
                    case 12:
                        xf::cv::AXISetBitFields(axi, (l)*depth, depth, (unsigned char)cv_pix);
                        break;
                    case 16:
                        xf::cv::AXISetBitFields(axi, (l)*depth, depth, (unsigned short)cv_pix);
                        break;
                    case CV_8U:
                        xf::cv::AXISetBitFields(axi, (l)*depth, depth, (unsigned char)cv_pix);
                        break;
                    default:
                        xf::cv::AXISetBitFields(axi, (l)*depth, depth, (unsigned char)cv_pix);
                        break;
                }
            }
            axi.keep = -1;
            AXI_video_strm << axi;
        }
    }
}

/*********************************************************************************
 * Function:    MultiPixelAXIvideo2Mat
 * Parameters:  96bit stream with 4 pixels packed
 * Return:      None
 * Description: extract pixels from stream and write to open CV Image
 **********************************************************************************/
static void MultiPixelAXIvideo2Mat(OutVideoStrm_t& AXI_video_strm, cv::Mat& img, unsigned char ColorFormat) {
    int i, j, k, l;
    ap_axiu<AXI_WIDTH_OUT, 1, 1, 1> axi;

#if 1
    cv::Vec3b cv_pix;
#else
    cv::Vec3w cv_pix;
#endif

    int depth = XF_DTPIXELDEPTH(XF_LTM_T, XF_NPPC);
    bool sof = 0;

    for (i = 0; i < img.rows; i++) {
        for (j = 0; j < img.cols / XF_NPPC; j++) { // 4 pixels read per iteration
            AXI_video_strm >> axi;
            if ((i == 0) && (j == 0)) {
                if (axi.user.to_int() == 1) {
                    sof = 1;
                } else {
                    j--;
                }
            }
            if (sof) {
                for (l = 0; l < XF_NPPC; l++) {
                    int num_comp =
                        ((ColorFormat == 0) || (ColorFormat == 1) || (ColorFormat == 4)) ? (img.channels()) : 2;
                    for (k = 0; k < num_comp; k++) {
#if XF_AXI_GBR == 1
                        const int mapComp[5][3] = {
                            {1, 0, 2}, // RGB  //GBR
                            {0, 1, 2}, // 4:4:4
                            {0, 1, 1}, // 4:2:2
                            {0, 1, 1}, // 4:2:0
                            {1, 0, 2},
                        }; // 4:2:0 HDMI
#else
                        const int mapComp[5][3] = {
                            {0, 1, 2}, // RGB  //GBR
                            {0, 1, 2}, // 4:4:4
                            {0, 1, 1}, // 4:2:2
                            {0, 1, 1}, // 4:2:0
                            {1, 0, 2},
                        }; // 4:2:0 HDMI
#endif
                        int kMap = mapComp[ColorFormat][k];
                        switch (depth) {
                            case 10: {
                                unsigned char temp;
                                xf::cv::AXIGetBitFields(axi, (kMap + l * num_comp) * depth, depth, temp);
                                cv_pix.val[k] = temp;
                            } break;
                            case 12: {
                                unsigned char temp;
                                xf::cv::AXIGetBitFields(axi, (kMap + l * num_comp) * depth, depth, temp);
                                cv_pix.val[k] = temp;
                            } break;
                            case 16: {
                                unsigned short temp;
                                xf::cv::AXIGetBitFields(axi, (kMap + l * num_comp) * depth, depth, temp);
                                cv_pix.val[k] = temp;
                            } break;
                            case CV_8U: {
                                unsigned char temp;
                                xf::cv::AXIGetBitFields(axi, (kMap + l * num_comp) * depth, depth, temp);
                                cv_pix.val[k] = temp;
                            } break;
                            default: {
                                unsigned char temp;
                                xf::cv::AXIGetBitFields(axi, (kMap + l * num_comp) * depth, depth, temp);
                                cv_pix.val[k] = temp;
                            } break;
                        }
                    }
#if 1
                    img.at<cv::Vec3b>(i, (XF_NPPC * j + l)) = cv_pix;
#else
                    img.at<cv::Vec3w>(i, (XF_NPPC * j + l)) = cv_pix;
#endif
                }
            } // if(sof)
        }
    }
}
/*********************************************************************************
 * Function:    MultiPixelAXIvideo2Mat
 * Parameters:  96bit stream with 4 pixels packed
 * Return:      None
 * Description: extract pixels from stream and write to open CV Image
 **********************************************************************************/
static void MultiPixelAXIvideo2Mat_yuv(OutVideoStrm_t& AXI_video_strm, cv::Mat& img, unsigned char ColorFormat) {
    int i, j, k, l;
    ap_axiu<AXI_WIDTH_OUT, 1, 1, 1> axi;
    /*
    #if 1
        cv::Vec3b cv_pix;
    #else
        cv::Vec3w cv_pix;
    #endif
    */
    unsigned short cv_pix;
    int depth = XF_DTPIXELDEPTH(XF_16UC1, XF_NPPC);
    bool sof = 0;

    for (i = 0; i < img.rows; i++) {
        for (j = 0; j < img.cols / XF_NPPC; j++) { // 4 pixels read per iteration
            AXI_video_strm >> axi;
            if ((i == 0) && (j == 0)) {
                if (axi.user.to_int() == 1) {
                    sof = 1;
                } else {
                    j--;
                }
            }
            if (sof) {
                for (l = 0; l < XF_NPPC; l++) {
                    cv_pix = axi.data(l * depth + depth - 1, l * depth);

                    img.at<unsigned short>(i, (XF_NPPC * j + l)) = cv_pix;
                }
            } // if(sof)
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

void HDR_merge(cv::Mat& _src1,
               cv::Mat& _src2,
               float& alpha,
               float& optical_black_value,
               float& intersec,
               float& rho,
               float& imax,
               float* t,
               cv::Mat& final_img,
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

    cv::Mat final_w1, final_w2;

    final_img.create(_src1.rows, _src1.cols, CV_16UC1);
    final_w1.create(_src1.rows, _src1.cols, CV_32FC1);
    final_w2.create(_src1.rows, _src1.cols, CV_32FC1);

    FILE* fp1 = fopen("imagevals_ocv.txt", "w");

    for (int i = 0; i < _src1.rows; i++) {
        for (int j = 0; j < _src1.cols; j++) {
            int val1 = _src1.at<unsigned short>(i, j);
            int val2 = _src2.at<unsigned short>(i, j);

            final_w1.at<float>(i, j) = (float)(wr[0][val1]);
            final_w2.at<float>(i, j) = (float)(wr[1][val2]);

            float val_1 = final_w1.at<float>(i, j) *
                          val1; // (g_value_com(_src1.at<unsigned short>(i,j),alpha,optical_black_value)/t[0]);
            float val_2 = final_w2.at<float>(i, j) *
                          val2; //(g_value_com(_src2.at<unsigned short>(i,j),alpha,optical_black_value)/t[1]);

            float sum_wei = final_w1.at<float>(i, j) + final_w2.at<float>(i, j);

            int final_val = (int)((float)(val_1 + val_2) / (float)sum_wei);

            if (final_val > (W_B_SIZE - 1)) {
                final_val = (W_B_SIZE - 1);
            }
            fprintf(fp1, "%d,", final_val);

            final_img.at<unsigned short>(i, j) = (unsigned short)final_val;
        }
        fprintf(fp1, "\n");
    }
    fclose(fp1);
}

int main(int argc, char** argv) {
    if (argc != 3) {
        fprintf(stderr, "Invalid Number of Arguments!\nUsage:\n");
        fprintf(stderr, "<Executable Name> <input image path> \n");
        return -1;
    }

    cv::Mat in_img1, in_img2, interleaved_img, final_ocv, out_img, ocv_ref, in_gray, diff;

    unsigned short in_width, in_height;

/*  reading in the color image  */
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

#if T_8U
    interleaved_img.create(cv::Size(in_img1.cols + NUM_H_BLANK, in_img1.rows * 2), CV_8UC1);

#else
    interleaved_img.create(cv::Size(in_img1.cols + NUM_H_BLANK, in_img1.rows * 2), CV_16UC1);

#endif

    int height = in_img1.rows;
    int width = in_img1.cols;
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

#if T_8U
    int sc = 1;
    int cnt = 0, cnt1 = 0;
    for (int r = 0; r < height * 2; r++) {
        for (int c = 0; c < width + NUM_H_BLANK; c++) {
            if (r < NUM_V_BLANK_LINES) {
                if (c >= NUM_H_BLANK)
                    interleaved_img.at<unsigned char>(r, c) = in_img1.at<unsigned char>(r, c - NUM_H_BLANK);
                else
                    interleaved_img.at<unsigned char>(r, c) = 0;
            }

            if (r >= NUM_V_BLANK_LINES && r <= ((2 * height) - NUM_V_BLANK_LINES)) {
                if (r % 2 == 0) {
                    if (c >= NUM_H_BLANK)
                        interleaved_img.at<unsigned char>(r, c) = in_img2.at<unsigned char>(cnt, c - NUM_H_BLANK);
                    else
                        interleaved_img.at<unsigned char>(r, c) = 0;
                } else {
                    if (c >= NUM_H_BLANK)
                        interleaved_img.at<unsigned char>(r, c) =
                            in_img1.at<unsigned char>(cnt1 + NUM_V_BLANK_LINES - 1, c - NUM_H_BLANK);
                    else
                        interleaved_img.at<unsigned char>(r, c) = 0;
                }
            }
            if (r >= ((2 * height) - NUM_V_BLANK_LINES)) {
                if (c >= NUM_H_BLANK)
                    interleaved_img.at<unsigned char>(r, c) = in_img2.at<unsigned char>(cnt, c - NUM_H_BLANK);
                else
                    interleaved_img.at<unsigned char>(r, c) = 0;
            }
        }
        if (r % 2 == 0 && r >= NUM_V_BLANK_LINES) {
            cnt++;
        }
        if (r % 2 != 0 && r >= NUM_V_BLANK_LINES) {
            cnt1++;
        }
    }
#else
    int sc = 1;
    int cnt = 0, cnt1 = 0;
    for (int r = 0; r < height * 2; r++) {
        for (int c = 0; c < width + NUM_H_BLANK; c++) {
            if (r < NUM_V_BLANK_LINES) {
                if (c >= NUM_H_BLANK)
                    interleaved_img.at<unsigned short>(r, c) = in_img1.at<unsigned short>(r, c - NUM_H_BLANK);
                else
                    interleaved_img.at<unsigned short>(r, c) = 0;
            }

            if (r >= NUM_V_BLANK_LINES && r <= ((2 * height) - NUM_V_BLANK_LINES)) {
                if (r % 2 == 0) {
                    if (c >= NUM_H_BLANK)
                        interleaved_img.at<unsigned short>(r, c) = in_img2.at<unsigned short>(cnt, c - NUM_H_BLANK);
                    else
                        interleaved_img.at<unsigned short>(r, c) = 0;
                } else {
                    if (c >= NUM_H_BLANK)
                        interleaved_img.at<unsigned short>(r, c) =
                            in_img1.at<unsigned short>(cnt1 + NUM_V_BLANK_LINES - 1, c - NUM_H_BLANK);
                    else
                        interleaved_img.at<unsigned short>(r, c) = 0;
                }
            }
            if (r >= ((2 * height) - NUM_V_BLANK_LINES)) {
                if (c >= NUM_H_BLANK)
                    interleaved_img.at<unsigned short>(r, c) = in_img2.at<unsigned short>(cnt, c - NUM_H_BLANK);
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
#endif

    imwrite("interleaved_img.png", interleaved_img);

#if T_8U
    out_img.create(in_img1.rows, in_img1.cols, CV_8UC3);
#else
    out_img.create(in_img1.rows, in_img1.cols, CV_8UC3);
#endif

    // Write input image
    imwrite("input1.png", in_img1);
    imwrite("input2.png", in_img2);

#if T_8U
    final_ocv.create(in_img1.rows, in_img1.cols, CV_8UC1);
#endif
#if T_16U
    final_ocv.create(in_img1.rows, in_img1.cols, CV_16UC1);
#endif

    float alpha = 1.0f;
    float optical_black_value = 0.0f;
    float intersec = 0.25f;
    float rho = 512;
    float imax = (W_B_SIZE - 1);
    float t[NO_EXPS] = {1.0f, 0.25f}; //{1.0f,0.25f,0.0625f};

    short wr_ocv[NO_EXPS][W_B_SIZE];

    HDR_merge(in_img1, in_img2, alpha, optical_black_value, intersec, rho, imax, t, final_ocv, wr_ocv);

    imwrite("final_ocv.png", final_ocv);

    short wr_hls[NO_EXPS * XF_NPPC * W_B_SIZE];

    for (int k = 0; k < XF_NPPC; k++) {
        for (int i = 0; i < NO_EXPS; i++) {
            for (int j = 0; j < (W_B_SIZE); j++) {
                wr_hls[(i + k * NO_EXPS) * W_B_SIZE + j] = wr_ocv[i][j];
            }
        }
    }

    /////////////////////////////////////// CL ////////////////////////
    std::cout << "Input image height : " << height << std::endl;
    std::cout << "Input image width  : " << width << std::endl;
    std::cout << "Input Image Bit Depth:" << XF_DTPIXELDEPTH(XF_SRC_T, XF_NPPC) << std::endl;
    std::cout << "Input Image Channels:" << XF_CHANNELS(XF_SRC_T, XF_NPPC) << std::endl;
    std::cout << "NPPC:" << XF_NPPC << std::endl;

    unsigned short rgain = 256;
    unsigned short bgain = 256;
    unsigned short ggain = 200;

    unsigned char mode_reg = 1;

    unsigned short pawb = 128;

    unsigned char gamma_lut[256 * 3];
    uint32_t hist0_awb[3][HIST_SIZE] = {0};
    uint32_t hist1_awb[3][HIST_SIZE] = {0};

    float gamma_val_r = 0.8f, gamma_val_g = 1.5f, gamma_val_b = 1.5f;

    compute_gamma(gamma_val_r, gamma_val_g, gamma_val_b, gamma_lut);
    InVideoStrm_t src_axi;
    OutVideoStrm_t dst_axi;
    uint16_t bformat = XF_BAYER_PATTERN; // Bayer format BG-0; GB-1; GR-2; RG-3

    for (int i = 0; i < 2; i++) {
        Mat2MultiBayerAXIvideo(interleaved_img, src_axi, 0);
        // Call IP Processing function
        isppipeline(width, height, src_axi, dst_axi, rgain, bgain, gamma_lut, mode_reg, pawb, wr_hls, ccm_matrix_int,
                    offsetarray_int, ggain, bformat);

        MultiPixelAXIvideo2Mat(dst_axi, out_img, 0);
    }
    imwrite("output.png", out_img);

    return 0;
}
