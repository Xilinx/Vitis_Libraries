/*
 * Copyright 2021 Xilinx, Inc.
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

#define PROFILE

#include <chrono>
#include <common/xf_aie_sw_utils.hpp>
#include <common/xfcvDataMovers.h>
#include <fstream>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "xcl2.hpp"
#include "xf_opencl_wrap.hpp"
#include <experimental/xrt_kernel.h>
#include <experimental/xrt_graph.h>
#include "config.h"
// PL

#include "xf_isp_types.h"
#include "ccm-params.h"

void bayerizeImage(cv::Mat img, cv::Mat& bayer_image, cv::Mat& cfa_output, int code) {
    for (int i = 0; i < img.rows; i++) {
        for (int j = 0; j < img.cols; j++) {
            cv::Vec3b in = img.at<cv::Vec3b>(i, j);
            cv::Vec3b b;
            b[0] = 0;
            b[1] = 0;
            b[2] = 0;

            if (code == 0) {            // BG
                if ((i & 1) == 0) {     // even row
                    if ((j & 1) == 0) { // even col
                        b[0] = in[0];
                        cfa_output.at<unsigned char>(i, j) = in[0];
                    } else { // odd col
                        b[1] = in[1];
                        cfa_output.at<unsigned char>(i, j) = in[1];
                    }
                } else {                // odd row
                    if ((j & 1) == 0) { // even col
                        b[1] = in[1];
                        cfa_output.at<unsigned char>(i, j) = in[1];
                    } else { // odd col
                        b[2] = in[2];
                        cfa_output.at<unsigned char>(i, j) = in[2];
                    }
                }
            }
            if (code == 1) {            // GB
                if ((i & 1) == 0) {     // even row
                    if ((j & 1) == 0) { // even col
                        b[1] = in[1];
                        cfa_output.at<unsigned char>(i, j) = in[1];
                    } else { // odd col
                        b[0] = in[0];
                        cfa_output.at<unsigned char>(i, j) = in[0];
                    }
                } else {                // odd row
                    if ((j & 1) == 0) { // even col
                        b[2] = in[2];
                        cfa_output.at<unsigned char>(i, j) = in[2];
                    } else { // odd col
                        b[1] = in[1];
                        cfa_output.at<unsigned char>(i, j) = in[1];
                    }
                }
            }
            if (code == 2) {            // GR
                if ((i & 1) == 0) {     // even row
                    if ((j & 1) == 0) { // even col
                        b[1] = in[1];
                        cfa_output.at<unsigned char>(i, j) = in[1];
                    } else { // odd col
                        b[2] = in[2];
                        cfa_output.at<unsigned char>(i, j) = in[2];
                    }
                } else {                // odd row
                    if ((j & 1) == 0) { // even col
                        b[0] = in[0];
                        cfa_output.at<unsigned char>(i, j) = in[0];
                    } else { // odd col
                        b[1] = in[1];
                        cfa_output.at<unsigned char>(i, j) = in[1];
                    }
                }
            }
            if (code == 3) {            // RG
                if ((i & 1) == 0) {     // even row
                    if ((j & 1) == 0) { // even col
                        b[2] = in[2];
                        cfa_output.at<unsigned char>(i, j) = in[2];
                    } else { // odd col
                        b[1] = in[1];
                        cfa_output.at<unsigned char>(i, j) = in[1];
                    }
                } else {                // odd row
                    if ((j & 1) == 0) { // even col
                        b[1] = in[1];
                        cfa_output.at<unsigned char>(i, j) = in[1];
                    } else { // odd col
                        b[0] = in[0];
                        cfa_output.at<unsigned char>(i, j) = in[0];
                    }
                }
            }
            bayer_image.at<cv::Vec3b>(i, j) = b;
        }
    }
}
void blacklevel_ref(cv::Mat& input, cv::Mat& output, uint8_t blacklevel, float mul_fact) {
    int height = input.size().height;
    int width = input.size().width;
    typedef uint8_t Pixel_t;

    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            Pixel_t Pixel = input.at<Pixel_t>(i, j);
            output.at<Pixel_t>(i, j) = cv::saturate_cast<unsigned char>((Pixel - blacklevel) * mul_fact);
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

template <bool border_reflect>
void demosaicImage(cv::Mat cfa_output, cv::Mat& output_image, int code) {
    int block[5][5];
    for (int i = 0; i < output_image.rows; i++) {
        for (int j = 0; j < output_image.cols; j++) {
            for (int k = -2, ki = 0; k < 3; k++, ki++) {
                for (int l = -2, li = 0; l < 3; l++, li++) {
                    if (border_reflect) {
                        int x = std::min(std::max((i + k), 0), (output_image.rows - 1));
                        int y = std::min(std::max((j + l), 0), (output_image.cols - 1));
                        if (cfa_output.type() == CV_8UC1)
                            block[ki][li] = (int)cfa_output.at<unsigned char>(x, y);
                        else
                            block[ki][li] = (int)cfa_output.at<unsigned short>(x, y);
                    } else {
                        if (i + k >= 0 && i + k < output_image.rows && j + l >= 0 && j + l < output_image.cols) {
                            if (cfa_output.type() == CV_8UC1)
                                block[ki][li] = (int)cfa_output.at<unsigned char>(i + k, j + l);
                            else
                                block[ki][li] = (int)cfa_output.at<unsigned short>(i + k, j + l);
                        } else {
                            block[ki][li] = 0;
                        }
                    }
                }
            }
            cv::Vec3f out_pix;

            if (code == 0) {                     // BG
                if ((i & 0x00000001) == 0) {     // B row
                    if ((j & 0x00000001) == 0) { // B location
                        out_pix[0] = 8 * (float)block[2][2];
                        out_pix[1] = -1.0 * (float)(block[0][2] + block[2][0] + block[2][4] + block[4][2]) +
                                     2.0 * (float)(block[1][2] + block[2][1] + block[2][3] + block[3][2]) +
                                     4.0 * (float)block[2][2];
                        out_pix[2] = -1.5 * (float)(block[0][2] + block[2][0] + block[2][4] + block[4][2]) +
                                     2.0 * (float)(block[1][1] + block[1][3] + block[3][1] + block[3][3]) +
                                     6.0 * (float)block[2][2];
                    } else { // G location
                        out_pix[0] = 0.5 * (float)(block[0][2] + block[4][2]) +
                                     (-1.0) * (float)(block[1][1] + block[1][3] + block[2][0] + block[2][4] +
                                                      block[3][1] + block[3][3]) +
                                     4.0 * (float)(block[2][1] + block[2][3]) + 5.0 * (float)(block[2][2]);
                        out_pix[1] = 8.0 * (float)block[2][2];
                        out_pix[2] = -1.0 * (float)(block[0][2] + block[1][1] + block[1][3] + block[3][1] +
                                                    block[3][3] + block[4][2]) +
                                     4.0 * (float)(block[1][2] + block[3][2]) +
                                     0.5 * (float)(block[2][0] + block[2][4]) + 5.0 * (float)block[2][2];
                    }
                } else {                         // R row
                    if ((j & 0x00000001) == 0) { // G location
                        out_pix[0] = 0.5 * (float)(block[2][0] + block[2][4]) +
                                     (-1.0) * (float)(block[1][1] + block[1][3] + block[0][2] + block[4][2] +
                                                      block[3][1] + block[3][3]) +
                                     4.0 * (float)(block[1][2] + block[3][2]) + 5.0 * (float)(block[2][2]);
                        out_pix[1] = 8.0 * (float)block[2][2];
                        out_pix[2] = -1.0 * (float)(block[2][0] + block[1][1] + block[1][3] + block[3][1] +
                                                    block[3][3] + block[2][4]) +
                                     4.0 * (float)(block[2][1] + block[2][3]) +
                                     0.5 * (float)(block[0][2] + block[4][2]) + 5.0 * (float)block[2][2];
                    } else { // R location
                        out_pix[0] = -1.5 * (float)(block[0][2] + block[2][0] + block[2][4] + block[4][2]) +
                                     2.0 * (float)(block[1][1] + block[1][3] + block[3][1] + block[3][3]) +
                                     6.0 * (float)block[2][2];
                        out_pix[1] = -1.0 * (float)(block[0][2] + block[2][0] + block[2][4] + block[4][2]) +
                                     2.0 * (float)(block[1][2] + block[2][1] + block[2][3] + block[3][2]) +
                                     4.0 * (float)block[2][2];
                        out_pix[2] = 8.0 * (float)block[2][2];
                    }
                }
            } else if (code == 1) {              // GB
                if ((i & 0x00000001) == 0) {     // B row
                    if ((j & 0x00000001) == 0) { // G location
                        out_pix[0] = 0.5 * (float)(block[0][2] + block[4][2]) +
                                     (-1.0) * (float)(block[1][1] + block[1][3] + block[2][0] + block[2][4] +
                                                      block[3][1] + block[3][3]) +
                                     4.0 * (float)(block[2][1] + block[2][3]) + 5.0 * (float)(block[2][2]);
                        out_pix[1] = 8.0 * (float)block[2][2];
                        out_pix[2] = -1.0 * (float)(block[0][2] + block[1][1] + block[1][3] + block[3][1] +
                                                    block[3][3] + block[4][2]) +
                                     4.0 * (float)(block[1][2] + block[3][2]) +
                                     0.5 * (float)(block[2][0] + block[2][4]) + 5.0 * (float)block[2][2];
                    } else { // B location
                        out_pix[0] = 8 * (float)block[2][2];
                        out_pix[1] = -1.0 * (float)(block[0][2] + block[2][0] + block[2][4] + block[4][2]) +
                                     2.0 * (float)(block[1][2] + block[2][1] + block[2][3] + block[3][2]) +
                                     4.0 * (float)block[2][2];
                        out_pix[2] = -1.5 * (float)(block[0][2] + block[2][0] + block[2][4] + block[4][2]) +
                                     2.0 * (float)(block[1][1] + block[1][3] + block[3][1] + block[3][3]) +
                                     6.0 * (float)block[2][2];
                    }
                } else {                         // R row
                    if ((j & 0x00000001) == 0) { // R location
                        out_pix[0] = -1.5 * (float)(block[0][2] + block[2][0] + block[2][4] + block[4][2]) +
                                     2.0 * (float)(block[1][1] + block[1][3] + block[3][1] + block[3][3]) +
                                     6.0 * (float)block[2][2];
                        out_pix[1] = -1.0 * (float)(block[0][2] + block[2][0] + block[2][4] + block[4][2]) +
                                     2.0 * (float)(block[1][2] + block[2][1] + block[2][3] + block[3][2]) +
                                     4.0 * (float)block[2][2];
                        out_pix[2] = 8.0 * (float)block[2][2];
                    } else { // G location
                        out_pix[0] = 0.5 * (float)(block[2][0] + block[2][4]) +
                                     (-1.0) * (float)(block[1][1] + block[1][3] + block[0][2] + block[4][2] +
                                                      block[3][1] + block[3][3]) +
                                     4.0 * (float)(block[1][2] + block[3][2]) + 5.0 * (float)(block[2][2]);
                        out_pix[1] = 8.0 * (float)block[2][2];
                        out_pix[2] = -1.0 * (float)(block[2][0] + block[1][1] + block[1][3] + block[3][1] +
                                                    block[3][3] + block[2][4]) +
                                     4.0 * (float)(block[2][1] + block[2][3]) +
                                     0.5 * (float)(block[0][2] + block[4][2]) + 5.0 * (float)block[2][2];
                    }
                }
            } else if (code == 2) {              // GR
                if ((i & 0x00000001) == 0) {     // R row
                    if ((j & 0x00000001) == 0) { // G location
                        out_pix[0] = 0.5 * (float)(block[2][0] + block[2][4]) +
                                     (-1.0) * (float)(block[1][1] + block[1][3] + block[0][2] + block[4][2] +
                                                      block[3][1] + block[3][3]) +
                                     4.0 * (float)(block[1][2] + block[3][2]) + 5.0 * (float)(block[2][2]);
                        out_pix[1] = 8.0 * (float)block[2][2];
                        out_pix[2] = -1.0 * (float)(block[2][0] + block[1][1] + block[1][3] + block[3][1] +
                                                    block[3][3] + block[2][4]) +
                                     4.0 * (float)(block[2][1] + block[2][3]) +
                                     0.5 * (float)(block[0][2] + block[4][2]) + 5.0 * (float)block[2][2];
                    } else { // R location
                        out_pix[0] = -1.5 * (float)(block[0][2] + block[2][0] + block[2][4] + block[4][2]) +
                                     2.0 * (float)(block[1][1] + block[1][3] + block[3][1] + block[3][3]) +
                                     6.0 * (float)block[2][2];
                        out_pix[1] = -1.0 * (float)(block[0][2] + block[2][0] + block[2][4] + block[4][2]) +
                                     2.0 * (float)(block[1][2] + block[2][1] + block[2][3] + block[3][2]) +
                                     4.0 * (float)block[2][2];
                        out_pix[2] = 8.0 * (float)block[2][2];
                    }
                } else {                         // B row
                    if ((j & 0x00000001) == 0) { // B location
                        out_pix[0] = 8 * (float)block[2][2];
                        out_pix[1] = -1.0 * (float)(block[0][2] + block[2][0] + block[2][4] + block[4][2]) +
                                     2.0 * (float)(block[1][2] + block[2][1] + block[2][3] + block[3][2]) +
                                     4.0 * (float)block[2][2];
                        out_pix[2] = -1.5 * (float)(block[0][2] + block[2][0] + block[2][4] + block[4][2]) +
                                     2.0 * (float)(block[1][1] + block[1][3] + block[3][1] + block[3][3]) +
                                     6.0 * (float)block[2][2];
                    } else { // G location
                        out_pix[0] = 0.5 * (float)(block[0][2] + block[4][2]) +
                                     (-1.0) * (float)(block[1][1] + block[1][3] + block[2][0] + block[2][4] +
                                                      block[3][1] + block[3][3]) +
                                     4.0 * (float)(block[2][1] + block[2][3]) + 5.0 * (float)(block[2][2]);
                        out_pix[1] = 8.0 * (float)block[2][2];
                        out_pix[2] = -1.0 * (float)(block[0][2] + block[1][1] + block[1][3] + block[3][1] +
                                                    block[3][3] + block[4][2]) +
                                     4.0 * (float)(block[1][2] + block[3][2]) +
                                     0.5 * (float)(block[2][0] + block[2][4]) + 5.0 * (float)block[2][2];
                    }
                }
            } else if (code == 3) {              // RG
                if ((i & 0x00000001) == 0) {     // R row
                    if ((j & 0x00000001) == 0) { // R location
                        out_pix[0] = -1.5 * (float)(block[0][2] + block[2][0] + block[2][4] + block[4][2]) +
                                     2.0 * (float)(block[1][1] + block[1][3] + block[3][1] + block[3][3]) +
                                     6.0 * (float)block[2][2];
                        out_pix[1] = -1.0 * (float)(block[0][2] + block[2][0] + block[2][4] + block[4][2]) +
                                     2.0 * (float)(block[1][2] + block[2][1] + block[2][3] + block[3][2]) +
                                     4.0 * (float)block[2][2];
                        out_pix[2] = 8.0 * (float)block[2][2];
                    } else { // G location
                        out_pix[0] = -1.0 * (float)(block[0][2] + block[1][1] + block[1][3] + block[3][1] +
                                                    block[3][3] + block[4][2]) +
                                     4.0 * (float)(block[1][2] + block[3][2]) +
                                     0.5 * (float)(block[2][0] + block[2][4]) + 5.0 * (float)block[2][2];
                        out_pix[1] = 8.0 * (float)block[2][2];
                        out_pix[2] = 0.5 * (float)(block[0][2] + block[4][2]) +
                                     (-1.0) * (float)(block[1][1] + block[1][3] + block[2][0] + block[2][4] +
                                                      block[3][1] + block[3][3]) +
                                     4.0 * (float)(block[2][1] + block[2][3]) + 5.0 * (float)(block[2][2]);
                    }
                } else {                         // B row
                    if ((j & 0x00000001) == 0) { // G location
                        out_pix[0] = 0.5 * (float)(block[0][2] + block[4][2]) +
                                     (-1.0) * (float)(block[1][1] + block[1][3] + block[2][0] + block[2][4] +
                                                      block[3][1] + block[3][3]) +
                                     4.0 * (float)(block[2][1] + block[2][3]) + 5.0 * (float)(block[2][2]);
                        out_pix[1] = 8.0 * (float)block[2][2];
                        out_pix[2] = -1.0 * (float)(block[0][2] + block[1][1] + block[1][3] + block[3][1] +
                                                    block[3][3] + block[4][2]) +
                                     4.0 * (float)(block[1][2] + block[3][2]) +
                                     0.5 * (float)(block[2][0] + block[2][4]) + 5.0 * (float)block[2][2];
                    } else { // B location
                        out_pix[0] = 8.0 * (float)block[2][2];
                        out_pix[1] = -1.0 * (float)(block[0][2] + block[2][0] + block[2][4] + block[4][2]) +
                                     2.0 * (float)(block[1][2] + block[2][1] + block[2][3] + block[3][2]) +
                                     4.0 * (float)block[2][2];
                        out_pix[2] = -1.5 * (float)(block[0][2] + block[2][0] + block[2][4] + block[4][2]) +
                                     2.0 * (float)(block[1][1] + block[1][3] + block[3][1] + block[3][3]) +
                                     6.0 * (float)block[2][2];
                    }
                }
            }
            out_pix /= 8.0;
            if (output_image.type() == CV_8UC3) {
                output_image.at<cv::Vec3b>(i, j) = (cv::Vec3b)(out_pix);
            } else
                output_image.at<cv::Vec3w>(i, j) = (cv::Vec3w)(out_pix);
        }
    }
}
template <int code>
void gainControlOCV(cv::Mat& input, cv::Mat& output, cv::Mat& output_8u, unsigned char rgain, unsigned char bgain) {
    int height = input.size().height;
    int width = input.size().width;
    typedef uint8_t realSize;
    typedef int16_t maxSize;
    maxSize pixel;

    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            pixel = (maxSize)input.at<realSize>(i, j); // extracting each pixel
            bool cond1, cond2;
            cond1 = (j % 2 == 0);
            cond2 = (j % 2 != 0);
            if (code == XF_BAYER_RG) {
                if (i % 2 == 0 && cond1)
                    pixel = (maxSize)((pixel * rgain) >> 6);
                else if (i % 2 != 0 && cond2)
                    pixel = (maxSize)((pixel * bgain) >> 6);
            } else if (code == XF_BAYER_GR) {
                if (i % 2 == 0 && cond2)
                    pixel = (maxSize)((pixel * rgain) >> 6);
                else if (i % 2 != 0 && cond1)
                    pixel = (maxSize)((pixel * bgain) >> 6);
            } else if (code == XF_BAYER_BG) {
                if (i % 2 == 0 && cond1)
                    pixel = (maxSize)((pixel * bgain) >> 6);
                else if (i % 2 == 0 && cond2)
                    pixel = (maxSize)((pixel * rgain) >> 6);
            } else if (code == XF_BAYER_GB) {
                if (i % 2 == 0 && cond2)
                    pixel = (maxSize)((pixel * bgain) >> 6);
                else if (i % 2 != 0 && cond1)
                    pixel = (maxSize)((pixel * rgain) >> 6);
            }
            output_8u.at<realSize>(i, j) = cv::saturate_cast<realSize>(pixel); // writing each pixel
            output.at<maxSize>(i, j) = pixel;                                  // writing each pixel
        }
    }
}

void awbnorm_colorcorrectionmatrix(cv::Mat& _src, cv::Mat& _dst, uint16_t* coeff_awb, int16_t* coeff_fix) {
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

    for (int i = 0; i < _src.rows; i++) {
        for (int j = 0, k = 0; j < _src.cols; j++, k += 3) {
            float _src_r = _src.data[(i * _src.cols * 3) + k];
            float _src_g = _src.data[(i * _src.cols * 3) + k + 1];
            float _src_b = _src.data[(i * _src.cols * 3) + k + 2];
            _src_r = SAT_U8(((_src_r * coeff_awb[0]) / pow(2, coeff_awb[1])) - coeff_awb[2]);
            _src_g = SAT_U8(((_src_g * coeff_awb[3]) / pow(2, coeff_awb[4])) - coeff_awb[5]);
            _src_b = SAT_U8(((_src_b * coeff_awb[6]) / pow(2, coeff_awb[7])) - coeff_awb[8]);

            float value1 = (_src_r * ccm_matrix[0][0]);
            float value2 = (_src_g * ccm_matrix[0][1]);
            float value3 = (_src_b * ccm_matrix[0][2]);
            float value4 = (_src_r * ccm_matrix[1][0]);
            float value5 = (_src_g * ccm_matrix[1][1]);
            float value6 = (_src_b * ccm_matrix[1][2]);
            float value7 = (_src_r * ccm_matrix[2][0]);
            float value8 = (_src_g * ccm_matrix[2][1]);
            float value9 = (_src_b * ccm_matrix[2][2]);
            int value_r = (int)(value1 + value2 + value3 + offsetarray[0]);
            int value_g = (int)(value4 + value5 + value6 + offsetarray[1]);
            int value_b = (int)(value7 + value8 + value9 + offsetarray[2]);

            if (value_r > 255) {
                value_r = 255;
            }

            if (value_g > 255) {
                value_g = 255;
            }

            if (value_b > 255) {
                value_b = 255;
            }

            if (value_r < 0) {
                value_r = 0;
            }

            if (value_g < 0) {
                value_g = 0;
            }

            if (value_b < 0) {
                value_b = 0;
            }

            _dst.data[(i * _dst.cols * 3) + k] = static_cast<uint8_t>(value_r);     // value_r;
            _dst.data[(i * _dst.cols * 3) + k + 1] = static_cast<uint8_t>(value_g); // value_g;
            _dst.data[(i * _dst.cols * 3) + k + 2] = static_cast<uint8_t>(value_b); // value_b;
        }
    }

    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            coeff_fix[(i * 3) + j] = float2fixed(ccm_matrix[i][j], 12);
        }
    }
    for (int j = 0; j < 3; j++) {
        coeff_fix[j + 9] = float2fixed(offsetarray[0], 12);
    }
}

void gammacorrection_ref(
    cv::Mat in_gray, cv::Mat ocv_ref, uchar gam_r[256], uchar gam_g[256], uchar gam_b[256], int height, int width) {
    uchar in_r = 0, in_g = 0, in_b = 0;
    for (int x = 0; x < width; x++) {      // width = cols = x
        for (int y = 0; y < height; y++) { // height = rows = y
            in_r = in_gray.at<cv::Vec3b>(y, x)[0];
            in_g = in_gray.at<cv::Vec3b>(y, x)[1];
            in_b = in_gray.at<cv::Vec3b>(y, x)[2];
            ocv_ref.at<cv::Vec3b>(y, x)[0] = gam_r[in_r];
            ocv_ref.at<cv::Vec3b>(y, x)[1] = gam_g[in_g];
            ocv_ref.at<cv::Vec3b>(y, x)[2] = gam_b[in_b];
        }
    }
}
int run_opencv_ref(cv::Mat& srcImageR,
                   cv::Mat& dstRefImage,
                   cv::Mat& demRefImage,
                   int16_t blacklevel,
                   float mul_fact,
                   int16_t rgain,
                   int16_t bgain,
                   int16_t* coeffs,
                   unsigned char gamma_lut[256 * 3]) {
    cv::Mat bl_outImage(srcImageR.rows, srcImageR.cols, CV_8UC1);
    cv::Mat gc_outImage(srcImageR.rows, srcImageR.cols, CV_16SC1);
    cv::Mat gc_outImage1(srcImageR.rows, srcImageR.cols, CV_8UC1);
    cv::Mat awb_outImage(srcImageR.rows, srcImageR.cols, CV_8UC3);

    blacklevel_ref(srcImageR, bl_outImage, blacklevel, mul_fact);
    gainControlOCV<XF_BAYER_RG>(bl_outImage, gc_outImage, gc_outImage1, rgain, bgain);
    demosaicImage<true>(gc_outImage1, demRefImage, XF_BAYER_RG);
    uint16_t* coeffs_awb = (uint16_t*)coeffs + 16;
    int16_t* coeffs_fix = coeffs;
    awbnorm_colorcorrectionmatrix(demRefImage, awb_outImage, coeffs_awb, coeffs_fix);
    gammacorrection_ref(awb_outImage, dstRefImage, gamma_lut, gamma_lut + 256, gamma_lut + 512, srcImageR.rows,
                        srcImageR.cols);

    return 0;
}

void compute_awb_params(uint16_t* coeff_awb, float* min, float* max) {
    int out_min = 0, out_max = 255;
    float coeff_fl_r = (out_max - out_min) / (max[0] - min[0]);
    float coeff_fl_g = (out_max - out_min) / (max[1] - min[1]);
    float coeff_fl_b = (out_max - out_min) / (max[2] - min[2]);
    coeff_awb[2] = (coeff_fl_r * min[0] - out_min);
    coeff_awb[5] = (coeff_fl_g * min[1] - out_min);
    coeff_awb[8] = (coeff_fl_b * min[2] - out_min);
    // fixing fp vals to 8
    coeff_awb[1] = 8;
    coeff_awb[4] = 8;
    coeff_awb[7] = 8;
    coeff_awb[0] = (uint16_t)(coeff_fl_r * pow(2, coeff_awb[1]));
    coeff_awb[3] = (uint16_t)(coeff_fl_g * pow(2, coeff_awb[4]));
    coeff_awb[6] = (uint16_t)(coeff_fl_b * pow(2, coeff_awb[7]));
}

/*
 ******************************************************************************
 * Top level executable
 ******************************************************************************
 */

int main(int argc, char** argv) {
    try {
        if (argc < 3) {
            std::stringstream errorMessage;
            errorMessage << argv[0] << " <xclbin> <inputImage> [width] [height] [iterations] <r_g> <g_g> <b_g>";
            std::cerr << errorMessage.str();
            throw std::invalid_argument(errorMessage.str());
        }
        const char* xclBinName = argv[1];
        //////////////////////////////////////////
        // Read image from file and resize
        //////////////////////////////////////////
        cv::Mat inputR;
        inputR = cv::imread(argv[2], 1);
        cv::Mat srcImageR(inputR.rows, inputR.cols, CV_8UC1); // simulate the Bayer pattern CFA output
        cv::Mat color_cfa_bayer_output(inputR.rows, inputR.cols, CV_8UC3);
        bayerizeImage(inputR, color_cfa_bayer_output, srcImageR, XF_BAYER_RG);
        cv::imwrite("input_grey.png", srcImageR);

        int width = srcImageR.cols;
        if (argc >= 4) width = atoi(argv[3]);
        int height = srcImageR.rows;
        if (argc >= 5) height = atoi(argv[4]);

        if ((width != srcImageR.cols) || (height != srcImageR.rows))
            cv::resize(srcImageR, srcImageR, cv::Size(width, height));

        int iterations = 1;
        if (argc >= 6) iterations = atoi(argv[5]);

        float gamma_val_r = 0.5f;
        if (argc >= 7) gamma_val_r = atof(argv[6]);
        float gamma_val_g = 0.8f;
        if (argc >= 8) gamma_val_g = atof(argv[7]);
        float gamma_val_b = 0.8f;
        if (argc >= 9) gamma_val_b = atof(argv[8]);

        std::cout << "Image size" << std::endl;
        std::cout << srcImageR.rows << std::endl;
        std::cout << srcImageR.cols << std::endl;
        std::cout << srcImageR.elemSize() << std::endl;
        std::cout << "Image size (end)" << std::endl;
        int op_width = srcImageR.cols;
        int op_height = srcImageR.rows;

        //////////////////////////////////////////
        /* AIE Related*/
        //////////////////////////////////////////
        uint8_t rgain = 64;
        uint8_t bgain = 64;
        uint8_t black_level = 32;
        const int MaxLevel = 255; // 8b input value
        float MulValue1 = (float)((float)MaxLevel / (MaxLevel - black_level));
        uint16_t MulValue = 37470; // Q(1.15)

        int16_t coeffs[25];

        float minVal_f[3], maxVal_f[3];
        minVal_f[0] = -0.5;
        minVal_f[1] = -0.5;
        minVal_f[2] = -0.5;
        maxVal_f[0] = 223.5;
        maxVal_f[1] = 230.5;
        maxVal_f[2] = 250.5;
        compute_awb_params((uint16_t*)coeffs + 16, minVal_f, maxVal_f);
        ccmparams<0>(coeffs);

        float thresh = 0.5;

        ///////////////////////////////////////////////
        /*HLS Related*/
        //////////////////////////////////////////////
        cv::Mat in_img, out_img, ocv_ref, in_gray, diff;

        out_img.create(srcImageR.rows, srcImageR.cols, CV_8UC3);
        size_t vec_in_size_bytes = 256 * 3 * sizeof(unsigned char);
        size_t image_in_size_bytes = srcImageR.rows * srcImageR.cols * 3 * sizeof(unsigned char);
        size_t rgba_image_in_size_bytes = srcImageR.rows * srcImageR.cols * 4 * sizeof(unsigned char);
        size_t image_out_size_bytes = srcImageR.rows * srcImageR.cols * 3 * sizeof(unsigned char);

        // Write input image
        imwrite("input.png", srcImageR);

        int height1 = srcImageR.rows;
        int width1 = srcImageR.cols;

        std::cout << "Input image height : " << height1 << std::endl;
        std::cout << "Input image width  : " << width1 << std::endl;

        unsigned char gamma_lut[256 * 3];

        compute_gamma(gamma_val_r, gamma_val_g, gamma_val_b, gamma_lut);

        size_t minVal_vec_bytes = 3 * sizeof(int);
        size_t maxVal_vec_bytes = 3 * sizeof(int);

        cv::Mat demRefImage(srcImageR.rows, srcImageR.cols, CV_8UC3);
        cv::Mat dstRefImage(srcImageR.rows, srcImageR.cols, CV_8UC3);
        run_opencv_ref(srcImageR, dstRefImage, demRefImage, black_level, MulValue1, rgain, bgain, coeffs, gamma_lut);

        cv::Mat dstRefBGR[3];
        cv::split(dstRefImage, dstRefBGR);

        //////////////////////////////////////////////////////////////////////////////////////////////
        /* AIE Host */
        ////////////////////////////////////////////////////////////////////////////////////////////
        xF::deviceInit(xclBinName);

        void* srcData = nullptr;
        xrt::bo src_hndl = xrt::bo(xF::gpDhdl, (srcImageR.total() * srcImageR.elemSize()), 0, 0);
        srcData = src_hndl.map();
        memcpy(srcData, srcImageR.data, (srcImageR.total() * srcImageR.elemSize()));

        // Allocate output buffer for AWB-CCM
        void* dstData = nullptr;
        xrt::bo dst_hndl = xrt::bo(xF::gpDhdl, (op_height * op_width * 4), 0, 0);
        dstData = dst_hndl.map();
        cv::Mat dst(op_height, op_width, CV_8UC4, dstData);

        // Allocate output buffer for Demosaicing
        void* demData = nullptr;
        xrt::bo dem_hndl = xrt::bo(xF::gpDhdl, (op_height * op_width * 4), 0, 0);
        demData = dem_hndl.map();
        cv::Mat dstDem(op_height, op_width, CV_8UC4, demData);

        xF::xfcvDataMovers<xF::TILER, uint8_t, TILE_HEIGHT, TILE_WIDTH, VECTORIZATION_FACTOR, CORES> tiler(2, 2, false,
                                                                                                           4);
        xF::xfcvDataMovers<xF::STITCHER, uint8_t, TILE_HEIGHT, (TILE_WIDTH << 2), VECTORIZATION_FACTOR, CORES>
            stitcherDem;
        xF::xfcvDataMovers<xF::STITCHER, uint8_t, TILE_HEIGHT, (TILE_WIDTH << 2), VECTORIZATION_FACTOR, CORES> stitcher;

#if !__X86_DEVICE__
        std::cout << "Graph init. This does nothing because CDO in boot PDI "
                     "already configures AIE.\n";

        std::vector<xrt::graph> gHndl;
        std::string graph_name_RTP[6];

        for (int i = 0; i < CORES; i++) {
            std::string graph_name = "TOP[" + std::to_string(i) + "]";
            std::cout << graph_name << std::endl;

            gHndl.push_back(xrt::graph(xF::gpDhdl, xF::xclbin_uuid, graph_name));
            std::cout << "XRT graph opened" << std::endl;
            gHndl.back().reset();
            std::cout << "Graph reset done" << std::endl;
            for (int j = 1; j < 6; j++) {
                graph_name_RTP[j] = graph_name + ".k[0]." + "in[" + std::to_string(j) + "]";
            }

            gHndl[i].update(graph_name_RTP[1], coeffs);
            gHndl[i].update(graph_name_RTP[2], black_level);
            gHndl[i].update(graph_name_RTP[3], MulValue);
            gHndl[i].update(graph_name_RTP[4], rgain);
            gHndl[i].update(graph_name_RTP[5], bgain);
        }
#endif

        START_TIMER
        tiler.compute_metadata(srcImageR.size());
        STOP_TIMER("Meta data compute time")

        std::cout << "AWB_accel " << std::endl;
        cl_int err;
        std::vector<cl::Device> devices = xcl::get_xil_devices();
        cl::Device device = devices[0];

        // Context, command queue and device name:
        OCL_CHECK(err, cl::Context context(device, NULL, NULL, NULL, &err));
        OCL_CHECK(err, cl::CommandQueue queue(context, device, CL_QUEUE_PROFILING_ENABLE, &err));
        OCL_CHECK(err, std::string device_name = device.getInfo<CL_DEVICE_NAME>(&err));

        std::string binaryFile = xcl::find_binary_file(device_name, "kernel");
        cl::Program::Binaries bins = xcl::import_binary_file(binaryFile);
        devices.resize(1);

        std::cout << "INFO: Device found - " << device_name << std::endl;

        OCL_CHECK(err, cl::Program program(context, devices, bins, NULL, &err));

        OCL_CHECK(err, cl::Kernel kernel(program, "AWB_accel", &err));

        OCL_CHECK(err, cl::Buffer buffer_inImage(context, CL_MEM_READ_ONLY, rgba_image_in_size_bytes, NULL, &err));
        OCL_CHECK(err, cl::Buffer buffer_minValues(context, CL_MEM_WRITE_ONLY, minVal_vec_bytes, NULL, &err));
        OCL_CHECK(err, cl::Buffer buffer_maxValues(context, CL_MEM_WRITE_ONLY, maxVal_vec_bytes, NULL, &err));

        std::chrono::microseconds tt(0);
        for (int i = 0; i < iterations; i++) {
            START_TIMER
            auto tiles_sz = tiler.host2aie_nb(&src_hndl, srcImageR.size());
            stitcherDem.aie2host_nb(&dem_hndl, cv::Size((srcImageR.cols << 2), srcImageR.rows), tiles_sz);
            stitcher.aie2host_nb(&dst_hndl, cv::Size((srcImageR.cols << 2), srcImageR.rows), tiles_sz);

#if !__X86_DEVICE__
            for (int i = 0; i < CORES; i++) {
                std::cout << "Graph run(" << (tiler.tilesPerCore(i)) << ")\n";
                gHndl[i].run(tiler.tilesPerCore(i));
            }

            for (int i = 0; i < CORES; i++) {
                gHndl[i].wait();
            }
#endif

            tiler.wait();
            stitcherDem.wait();
            stitcher.wait();
            STOP_TIMER("AIE Pipeline")
            tt += tdiff;

            ////////////////////////////////////////////////////////PL
            /// host////////////////////////////////////////////////

            OCL_CHECK(err, err = kernel.setArg(0, buffer_inImage));
            OCL_CHECK(err, err = kernel.setArg(1, height));
            OCL_CHECK(err, err = kernel.setArg(2, width));
            OCL_CHECK(err, err = kernel.setArg(3, thresh));
            OCL_CHECK(err, err = kernel.setArg(4, buffer_minValues));
            OCL_CHECK(err, err = kernel.setArg(5, buffer_maxValues));

            // Profiling Objects
            cl_ulong start = 0;
            cl_ulong end = 0;
            double diff_prof = 0.0f;
            cl::Event event;

            OCL_CHECK(err,
                      queue.enqueueWriteBuffer(buffer_inImage,           // buffer on the FPGA
                                               CL_TRUE,                  // blocking call
                                               0,                        // buffer offset in bytes
                                               rgba_image_in_size_bytes, // Size in bytes
                                               demData,                  // Pointer to the data to copy
                                               nullptr, &event));

            // Execute the kernel:
            OCL_CHECK(err, err = queue.enqueueTask(kernel));

            // Copy Result from Device Global Memory to Host Local Memory
            queue.enqueueReadBuffer(buffer_minValues, // This buffers data will be read
                                    CL_TRUE,          // blocking call
                                    0,                // offset
                                    minVal_vec_bytes,
                                    // minVal, // Data will be stored here
                                    minVal_f, // Data will be stored here
                                    nullptr, &event);

            queue.enqueueReadBuffer(buffer_maxValues, // This buffers data will be read
                                    CL_TRUE,          // blocking call
                                    0,                // offset
                                    maxVal_vec_bytes,
                                    // maxVal, // Data will be stored here
                                    maxVal_f, // Data will be stored here
                                    nullptr, &event);
            event.getProfilingInfo(CL_PROFILING_COMMAND_START, &start);
            event.getProfilingInfo(CL_PROFILING_COMMAND_END, &end);
            diff_prof = end - start;
            std::cout << (diff_prof / 1000000) << "ms" << std::endl;
            // Clean up:
            queue.finish();

            // Execute AWB Histogram
            std::cout << minVal_f[0] << " " << minVal_f[1] << " " << minVal_f[2] << std::endl;
            std::cout << maxVal_f[0] << " " << maxVal_f[1] << " " << maxVal_f[2] << std::endl;
            // Read the calculated awb params from PL
            compute_awb_params((uint16_t*)coeffs + 16, minVal_f, maxVal_f);

            (void)cl_kernel_mgr::registerKernel("ISPPipeline_accel", "kernel", XCLIN(dstData, rgba_image_in_size_bytes),
                                                XCLOUT(out_img.data, image_out_size_bytes), XCLIN(srcImageR.rows),
                                                XCLIN(srcImageR.cols), XCLIN(gamma_lut, vec_in_size_bytes));
            // Execute ISP
            cl_kernel_mgr::exec_all();
        }

        cv::cvtColor(dstDem, dstDem, cv::COLOR_BGRA2BGR);
        cv::cvtColor(dst, dst, cv::COLOR_BGRA2BGR);
        cv::imwrite("ref.png", dstRefImage);
        cv::imwrite("ref_dem.png", demRefImage);
        cv::imwrite("demosaic.png", dstDem);
        cv::imwrite("awb_ccm.png", dst);
        cv::imwrite("Final-output.png", out_img);

        std::cout << "AIE PIPELINE passed" << std::endl;
        std::cout << "Average time to process frame : " << (((float)tt.count() * 0.001) / (float)iterations) << " ms"
                  << std::endl;
        std::cout << "Average frames per second : " << (((float)1000000 / (float)tt.count()) * (float)iterations)
                  << " fps" << std::endl;

#if !__X86_DEVICE__
        for (int i = 0; i < CORES; i++) {
            gHndl[i].end();
        }
#endif

        return 0;
    } catch (std::exception& e) {
        const char* errorMessage = e.what();
        std::cerr << "Exception caught: " << errorMessage << std::endl;
        exit(-1);
    }
}
