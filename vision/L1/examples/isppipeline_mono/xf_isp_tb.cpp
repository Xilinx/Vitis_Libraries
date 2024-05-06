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
#include "config/xf_config_params.h"
#include "common/xf_axi.hpp"
// #include <iostream>
// #include <fstream>
// #include <strstream>

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
void compute_gamma_lum(float l_g, uchar gamma_lut[256]) {
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

    unsigned char gam_r = 0; // gam_g = 0, gam_b = 0;

    for (int i = 0; i < 256; ++i) {
        float r_inv = (float)1 / l_g;

        float powval_r = (float)std::pow(gamma_inv[i], r_inv);
        short tempgamma_r = (powval_r * 255.0);

        if (tempgamma_r > 255) {
            gam_r = 255;
        } else {
            gam_r = tempgamma_r;
        }
        gamma_lut[i] = gam_r;
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

    int depth = XF_DTPIXELDEPTH(OUT_TYPE, XF_NPPCX);
    bool sof = 0;

    for (i = 0; i < img.rows; i++) {
        for (j = 0; j < img.cols / XF_NPPCX; j++) { // 4 pixels read per iteration
            AXI_video_strm >> axi;
            if ((i == 0) && (j == 0)) {
                if (axi.user.to_int() == 1) {
                    sof = 1;
                } else {
                    j--;
                }
            }
            if (sof) {
                for (l = 0; l < XF_NPPCX; l++) {
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
                    img.at<cv::Vec3b>(i, (XF_NPPCX * j + l)) = cv_pix;
#else
                    img.at<cv::Vec3w>(i, (XF_NPPCX * j + l)) = cv_pix;
#endif
                }
            } // if(sof)
        }
    }
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
    int depth = XF_DTPIXELDEPTH(IN_TYPE, XF_NPPCX);

    for (i = 0; i < img.rows; i++) {
        for (j = 0; j < img.cols; j += XF_NPPCX) {
            if ((i == 0) && (j == 0)) {
                axi.user = 1;
            } else {
                axi.user = 0;
            }
            if (j == (img.cols - XF_NPPCX)) {
                axi.last = 1;
            } else {
                axi.last = 0;
            }
            axi.data = -1;
            for (l = 0; l < XF_NPPCX; l++) {
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

int main(int argc, char** argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <INPUT IMAGE PATH 1>", argv[0]);
        return EXIT_FAILURE;
    }

    cv::Mat in_img, out_img, bayer_img, ocv_ref, in_gray, diff;

/*  reading in the color image  */
#if T_8U
    in_img = cv::imread(argv[1], 0); // read image
#else
    in_img = cv::imread(argv[1], -1); // read image
#endif

    if (in_img.data == NULL) {
        fprintf(stderr, "Cannot open image at %s\n", argv[1]);
        return 0;
    }

    unsigned int new_height = XF_NEWHEIGHT;
    unsigned int new_width = XF_NEWWIDTH;

    out_img.create(new_height, new_width, CV_8UC1);

    // Write input image
    imwrite("input.png", in_img);

    /////////////////////////////////////// CL ////////////////////////

    unsigned int height = in_img.rows;
    unsigned int width = in_img.cols;
    std::cout << "Input image height : " << height << std::endl;
    std::cout << "Input image width  : " << width << std::endl;

    // driver changes

    uint32_t common_config;
    common_config = (height << 16 | width);

    uint32_t resize_config;
    resize_config = (new_height << 16 | new_width);

    unsigned short gain_control_config_3 = 128; // gain_lum

    unsigned char gamma_correct_config[256]; // gamma_lut
    float gamma_val_lum = 1.2f;
    compute_gamma_lum(gamma_val_lum, gamma_correct_config);

    // int clahe_config_1 = 3;                    //clip
    // int clahe_config_2;
    // clahe_config_2 = ((int)TILES_Y_MAX << 16 | TILES_X_MAX);     // tiles_y_max(msb) tiles_x_max(lsb)

    // blc params

    float inputMax = (1 << (XF_DTPIXELDEPTH(IN_TYPE, XF_NPPCX))) - 1; // 65535.0f;
    float mul_fact = (float)(inputMax / (inputMax - BLACK_LEVEL));
    int blc_config_1 = (int)(mul_fact * 65536); // mul_fact int Q16_16 format
    int blc_config_2 = BLACK_LEVEL;

    unsigned short bformat = XF_BAYER_GR; // Bayer format BG-0; GB-1; GR-2; RG-3
    bayer_img.create(in_img.rows, in_img.cols, CV_16UC1);

    // clahe params

    int clip = 3;
    int tilesY = TILES_Y_MAX;
    int tilesX = TILES_X_MAX;

    int clahe_config_1 = clip; // clip
    int clahe_config_2;
    clahe_config_2 = ((int)TILES_Y_MAX << 16 | TILES_X_MAX); // tiles_y_max(msb) tiles_x_max(lsb)

    bayerizeImage(in_img, bayer_img, bformat);

    unsigned int pipeline_config_info;
    unsigned int max_supported_size;
    unsigned int funcs_available;
    unsigned int funcs_bypassable;
    unsigned int funcs_bypassable_config = 0b00000000000000000000000000000000;

    InVideoStrm_t src_axi;
    OutVideoStrm_t dst_axi;

    for (int i = 0; i < 2; i++) {
        Mat2MultiBayerAXIvideo(bayer_img, src_axi, 0);

        // Call IP Processing function
        ISPPipeline_accel(src_axi, dst_axi, common_config,
#if XF_BLC_EN
                          blc_config_1, blc_config_2,
#endif
                          resize_config,

#if XF_GAIN_EN
                          gain_control_config_3,
#endif
#if XF_GAMMA_EN
                          gamma_correct_config,
#endif
#if XF_CLAHE_EN
                          clahe_config_1, clahe_config_2,
#endif
                          pipeline_config_info, max_supported_size, funcs_available, funcs_bypassable,
                          funcs_bypassable_config);

        AXIvideo2cvMatxf<XF_NPPCX, XF_DTPIXELDEPTH(XF_GTM_T, XF_NPPCX) * XF_NPPCX>(dst_axi, out_img);
    }

    // Write output image
    imwrite("hls_out.png", out_img);
    std::cout << "Test Finished" << std::endl;

    return 0;
}
