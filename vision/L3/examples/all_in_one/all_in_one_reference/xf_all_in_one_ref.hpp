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
#include <iostream>
#include <vector>
#include "opencv2/opencv.hpp"
#include "common/xf_headers.hpp"
#include <stdlib.h>
#include <ap_int.h>
#include <math.h>
#include "xf_config_params.h"

#if USE_HDR
#include "all_in_one_reference/xf_hdr_decompanding_ref.hpp"
#endif
#if USE_HDR_FUSION
#include "all_in_one_reference/xf_extract_eframes_ref.hpp"
#endif
#if USE_RGBIR
#include "all_in_one_reference/xf_rgbir2bayer_ref.hpp"
#endif
#if USE_AEC
#include "all_in_one_reference/xf_aec_ref.hpp"
#endif
#if USE_DEGAMMA
#include "all_in_one_reference/xf_degamma_ref.hpp"
#endif
#if USE_AWB
#include "all_in_one_reference/xf_awb_ref.hpp"
#endif
#if USE_CCM
#include "all_in_one_reference/xf_ccm_ref.hpp"
#endif
#if USE_LTM
#include "all_in_one_reference/xf_ltm_ref_16bit.hpp"
#endif
#if USE_GTM
#include "all_in_one_reference/xf_gtm_ref.hpp"
#endif
#if USE_QND
#include "all_in_one_reference/xf_qnd_ref.hpp"
#endif
#if USE_3DLUT
#include "all_in_one_reference/xf_3dlut_ref.hpp"
#endif
#if USE_CSC
#include "all_in_one_reference/xf_rgb2yuyv_ref.hpp"
#endif

#include "all_in_one_reference/xf_bpc_ref.hpp"
#include "all_in_one_reference/xf_lsc_ref.hpp"
#include "all_in_one_reference/xf_gain_control_ref.hpp"
#include "all_in_one_reference/xf_demosaicing_ref.hpp"
#include "all_in_one_reference/xf_gamma_ref.hpp"

#if (DEBUG == 0)
void all_in_one_ref(cv::Mat& all_in_one_input,
                    double wr[NO_EXPS][W_B_SIZE],
                    int params_decomand[3][4][3],
                    int rows,
                    int cols,
                    unsigned short pawb,
                    unsigned short rgain,
                    unsigned short bgain,
                    unsigned short ggain,
                    int blk_height,
                    int blk_width,
                    int lut_dim,
                    float* lut,
                    cv::Mat& final_ocv) {
    //////////HDR_REF_SECTION//////////////////
    // cv::Mat HDR_ref_out;
    // HDR_ref_out.create(rows, cols, CV_IN_TYPE);

    cv::Mat HDR_ref_out;
    HDR_ref_out.create(rows, cols, CV_IN_TYPE);

    cv::Mat rggb_out_ref;
    rggb_out_ref.create(rows, cols, CV_IN_TYPE);

    cv::Mat aec_ref_out;
    aec_ref_out.create(rows, cols, CV_IN_TYPE);

    cv::Mat blacklevel_out_ref;
    blacklevel_out_ref.create(rows, cols, CV_IN_TYPE);

    cv::Mat bpc_ref;
    bpc_ref.create(rows, cols, CV_IN_TYPE);

    cv::Mat degamma_out_ref;
    degamma_out_ref.create(rows, cols, CV_IN_TYPE);

    cv::Mat lsc_out_ref;
    lsc_out_ref.create(rows, cols, CV_IN_TYPE);

    cv::Mat demosaic_out_ref(rows, cols, CV_OUT_TYPE);

    cv::Mat awb_out_ref;
    awb_out_ref.create(rows, cols, CV_OUT_TYPE);

    cv::Mat gaincontrol_out_ref;
    gaincontrol_out_ref.create(rows, cols, CV_IN_TYPE);

    cv::Mat ccm_out_ref(rows, cols, CV_OUT_TYPE);

    cv::Mat tm_out_ref;
    tm_out_ref.create(rows, cols, CV_GTM_TYPE);

    cv::Mat gamma_out_ref;
    gamma_out_ref.create(rows, cols, CV_GTM_TYPE);

    cv::Mat lut3d_out_ref;
    lut3d_out_ref.create(rows, cols, CV_GTM_TYPE);

    cv::Mat yuyv_out_ref;

#if USE_CSC
    yuyv_out_ref.create(rows, cols, CV_16UC1);
#else
    yuyv_out_ref.create(rows, cols, CV_8UC3);
#endif

#if USE_HDR
#if USE_HDR_FUSION
    cv::Mat lef_img_ref;
    cv::Mat sef_img_ref;

    lef_img_ref.create(rows, cols, CV_IN_TYPE);
    sef_img_ref.create(rows, cols, CV_IN_TYPE);
    cv::imwrite("all_in_one_input.png", all_in_one_input);

    extractExposureFrames_ref(all_in_one_input, lef_img_ref, sef_img_ref, NUM_V_BLANK_LINES, NUM_H_BLANK);

    cv::imwrite("lef_img_ref.png", lef_img_ref);
    cv::imwrite("sef_img_ref.png", sef_img_ref);

    HDR_merge_ref(lef_img_ref, sef_img_ref, wr, HDR_ref_out);

    cv::imwrite("HDR_extract_merge_ref_out.png", HDR_ref_out);
#else
    cv::imwrite("all_in_one_input.png", all_in_one_input);
    hdr_decompanding_ref(all_in_one_input, HDR_ref_out, params_decomand, XF_BAYER_PATTERN, rows, cols);
    cv::imwrite("out_img_hdr_decompanding.png", HDR_ref_out);
#endif

#else
    all_in_one_input.copyTo(HDR_ref_out);
#endif

//////////RGB-IR_REF_SECTION//////////////////

// cv::Mat rggb_out_ref;
// rggb_out_ref.create(rows, cols, CV_IN_TYPE);
#if USE_RGBIR
    cv::Mat ir_out_ref(rows, cols, CV_IN_TYPE);
    ref_rgb_ir(HDR_ref_out, rggb_out_ref, ir_out_ref, rows, cols);
    cv::imwrite("ref_ir_output.png", ir_out_ref);
    cv::imwrite("ref_rggb_output.png", rggb_out_ref);
#else
    HDR_ref_out.copyTo(rggb_out_ref);
    cv::imwrite("ref_rggb_buffer_output.png", rggb_out_ref);
#endif

//////////AEC_REF_SECTION//////////////////

// cv::Mat aec_ref_out;
// aec_ref_out.create(rows, cols, CV_IN_TYPE);
#if USE_AEC
    aec_ref(rggb_out_ref, aec_ref_out, pawb);
    cv::imwrite("aec_ref_out.png", aec_ref_out);
#else
    rggb_out_ref.copyTo(aec_ref_out);
    cv::imwrite("aec_ref_buffer_out.png", aec_ref_out);
#endif

//////////BLC_REF_SECTION//////////////////

// cv::Mat blacklevel_out_ref;
// blacklevel_out_ref.create(rows, cols, CV_IN_TYPE);
#if T_8U
    float MaxLevel = 255.0f; // Or White Level
#elif T_16U
    float MaxLevel = 65535.0f; // Or White Level
#endif
    float MulValue = (float)(MaxLevel / (MaxLevel - BLACK_LEVEL));
    for (int r = 0; r < rows; r++) {
        for (int c = 0; c < cols; c++) {
            unsigned short Pixel = aec_ref_out.at<unsigned short>(r, c);
            blacklevel_out_ref.at<unsigned short>(r, c) =
                cv::saturate_cast<unsigned short>((Pixel - BLACK_LEVEL) * MulValue);
        }
    }
    cv::imwrite("blacklevel_out_ref.png", blacklevel_out_ref);

    //////////BPC_REF_SECTION//////////////////

    // cv::Mat bpc_ref;
    // bpc_ref.create(rows, cols, CV_IN_TYPE);
    BadPixelCorrection(blacklevel_out_ref, bpc_ref);
    cv::imwrite("bpc_ref.png", bpc_ref);

//////////DEGAMMA_REF_SECTION//////////////////

// cv::Mat degamma_out_ref;
// degamma_out_ref.create(rows, cols, CV_IN_TYPE);
#if USE_DEGAMMA
    degamma_ref(bpc_ref, degamma_out_ref);
    cv::imwrite("degamma_out_ref.png", degamma_out_ref);
#else
    bpc_ref.copyTo(degamma_out_ref);
    cv::imwrite("degamma_buffer_out_ref.png", degamma_out_ref);
#endif

    //////////LSC_REF_SECTION//////////////////

    // cv::Mat lsc_out_ref;
    // lsc_out_ref.create(rows, cols, CV_IN_TYPE);
    LSC_ref(degamma_out_ref, lsc_out_ref);
    cv::imwrite("lsc_out_ref.png", lsc_out_ref);

    //////////GAIN_CONTROL_REF_SECTION//////////////////
    // cv::Mat gaincontrol_out_ref;
    // gaincontrol_out_ref.create(rows, cols, CV_IN_TYPE);
    int bformat = XF_BAYER_PATTERN;
    gainControlOCV(lsc_out_ref, gaincontrol_out_ref, bformat, rgain, bgain, ggain);
    cv::imwrite("gaincontrol_out_ref.png", gaincontrol_out_ref);

    //////////DEMOSAIC_REF_SECTION//////////////////

    // cv::Mat demosaic_out_ref(rows, cols, CV_OUT_TYPE);
    demosaicImage(gaincontrol_out_ref, demosaic_out_ref);
    cv::imwrite("demosaic_out_ref.png", demosaic_out_ref);

//////////AWB_REF_SECTION//////////////////

#if T_8U
    float inputMin = 0.0f;
    float inputMax = 255.0f;
    float outputMin = 0.0f;
    float outputMax = 255.0f;
    std::vector<cv::Mat_<uchar> > awb_input_ref;
    split(demosaic_out_ref, awb_input_ref);
#else T_16U
    float inputMin = 0.0f;
    float inputMax = 65535.0f;
    float outputMin = 0.0f;
    float outputMax = 65535.0f;
    std::vector<cv::Mat_<ushort> > awb_input_ref;
    split(demosaic_out_ref, awb_input_ref);
#endif
    // float thresh = (float)pawb / 256;
    float thresh = 0.9;

// cv::Mat awb_out_ref;
// awb_out_ref.create(rows, cols, CV_OUT_TYPE);

#if USE_AWB
#if (WB_TYPE == 0)
    // gray world white balancing algorithm
    balanceWhiteGW(demosaic_out_ref, awb_out_ref);
    cv::imwrite("awb_gw_out_ref.png", awb_out_ref);

#else
    // simple white balancing algorithm
    balanceWhiteSimple(awb_input_ref, awb_out_ref, inputMin, inputMax, outputMin, outputMax, thresh);
    cv::imwrite("awb_simple_out_ref.png", awb_out_ref);
#endif

#else
    demosaic_out_ref.copyTo(awb_out_ref);
    cv::imwrite("awb_buffer_out_ref.png", awb_out_ref);
#endif

//////////CCM_REF_SECTION//////////////////

// cv::Mat ccm_out_ref(rows, cols, CV_OUT_TYPE);
#if USE_CCM
    colorcorrectionmatrix(awb_out_ref, ccm_out_ref);
    cv::imwrite("ccm_out_ref.png", ccm_out_ref);
#else
    awb_out_ref.copyTo(ccm_out_ref);
    cv::imwrite("ccm_buffer_out_ref.png", ccm_out_ref);
#endif

//////////FUNCTION_TM_REF_SECTION//////////////////

// cv::Mat tm_out_ref;
// tm_out_ref.create(rows, cols, CV_GTM_TYPE);

#if USE_GTM
    global_tone_mapping_ref(ccm_out_ref, tm_out_ref);
    cv::imwrite("gtm_out_ref.png", tm_out_ref);
#elif USE_QND
    tm_out_ref = floyd_steinberg_dithering(ccm_out_ref, SCALEFACTOR);
    cv::imwrite("qnd_out_ref.png", tm_out_ref);
#elif USE_LTM
    ltm_ref(ccm_out_ref, tm_out_ref, rows, cols, blk_height, blk_width);
    cv::imwrite("ltm_out_ref.png", tm_out_ref);
#else
    ccm_out_ref.copyTo(tm_out_ref);
    cv::imwrite("function_tm_out_ref.png", tm_out_ref);
#endif

    //////////GAMMACORRECTION_REF_SECTION//////////////////

    // cv::Mat gamma_out_ref;
    // gamma_out_ref.create(rows, cols, CV_GTM_TYPE);
    unsigned char gamma_lut[256 * 3];
    float gamma_val_r = 0.5f, gamma_val_g = 0.8f, gamma_val_b = 0.8f;

    compute_gamma_ref(gamma_val_r, gamma_val_g, gamma_val_b, gamma_lut);

    unsigned char gam_r[256], gam_g[256], gam_b[256];
    for (int i = 0; i < 256; i++) {
        gam_r[i] = gamma_lut[i];
        gam_g[i] = gamma_lut[i + 256];
        gam_b[i] = gamma_lut[i + 512];
    }

    gammacorrection_ref(tm_out_ref, gamma_out_ref, gam_r, gam_g, gam_b, rows, cols);
    cv::imwrite("gamma_out_ref.png", gamma_out_ref);

//////////3DLUT_REF_SECTION//////////////////
// cv::Mat lut3d_out_ref(rows, cols, CV_GTM_TYPE);

#if USE_3DLUT
    lut3d_ref(rows, cols, lut_dim, gamma_out_ref, lut, lut3d_out_ref);
    cv::imwrite("lut3d_out_ref.png", lut3d_out_ref);

#else
    gamma_out_ref.copyTo(lut3d_out_ref);
    cv::imwrite("3dlut_buffer_out_ref.png", lut3d_out_ref);
#endif

//////////RGB2YUYV_REF_SECTION//////////////////
// cv::Mat yuyv_out_ref(rows,cols, CV_16UC1);
// cvtColor(lut3d_out_ref, lut3d_out_ref, cv::COLOR_BGR2RGB);

#if USE_CSC
    bgr2yuyv_ref(lut3d_out_ref, yuyv_out_ref);
#else
    lut3d_out_ref.copyTo(yuyv_out_ref);
#endif
    cv::imwrite("yuyv_out_ref.png", yuyv_out_ref);

    //////////OUTPUT//////////////////
    yuyv_out_ref.copyTo(final_ocv);
    cv::imwrite("final_ocv.png", final_ocv);
}
#else
void all_in_one_ref(cv::Mat& all_in_one_input,
                    double wr[NO_EXPS][W_B_SIZE],
                    int params_decomand[3][4][3],
                    int rows,
                    int cols,
                    unsigned short pawb,
                    unsigned short rgain,
                    unsigned short bgain,
                    unsigned short ggain,
                    int blk_height,
                    int blk_width,
                    int lut_dim,
                    float* lut,
                    cv::Mat& final_ocv,
                    cv::Mat& HDR_ref_out,
                    cv::Mat& rggb_out_ref,
                    cv::Mat& aec_ref_out,
                    cv::Mat& blacklevel_out_ref,
                    cv::Mat& bpc_ref,
                    cv::Mat& degamma_out_ref,
                    cv::Mat& lsc_out_ref,
                    cv::Mat& gaincontrol_out_ref,
                    cv::Mat& demosaic_out_ref,
                    cv::Mat& awb_out_ref,
                    cv::Mat& ccm_out_ref,
                    cv::Mat& tm_out_ref,
                    cv::Mat& gamma_out_ref,
                    cv::Mat& lut3d_out_ref,
                    cv::Mat& yuyv_out_ref) {
//////////HDR_REF_SECTION//////////////////
// cv::Mat HDR_ref_out;
// HDR_ref_out.create(rows, cols, CV_IN_TYPE);
#if USE_HDR
#if USE_HDR_FUSION
    cv::Mat lef_img_ref;
    cv::Mat sef_img_ref;

    lef_img_ref.create(rows, cols, CV_IN_TYPE);
    sef_img_ref.create(rows, cols, CV_IN_TYPE);
    cv::imwrite("all_in_one_input.png", all_in_one_input);

    extractExposureFrames_ref(all_in_one_input, lef_img_ref, sef_img_ref, NUM_V_BLANK_LINES, NUM_H_BLANK);

    cv::imwrite("lef_img_ref.png", lef_img_ref);
    cv::imwrite("sef_img_ref.png", sef_img_ref);

    HDR_merge_ref(lef_img_ref, sef_img_ref, wr, HDR_ref_out);

    cv::imwrite("HDR_extract_merge_ref_out.png", HDR_ref_out);
#else
    cv::imwrite("all_in_one_input.png", all_in_one_input);
    hdr_decompanding_ref(all_in_one_input, HDR_ref_out, params_decomand, XF_BAYER_PATTERN, rows, cols);
    cv::imwrite("out_img_hdr_decompanding.png", HDR_ref_out);
#endif

#else
    all_in_one_input.copyTo(HDR_ref_out);
#endif

//////////RGB-IR_REF_SECTION//////////////////

// cv::Mat rggb_out_ref;
// rggb_out_ref.create(rows, cols, CV_IN_TYPE);
#if USE_RGBIR
    cv::Mat ir_out_ref(rows, cols, CV_IN_TYPE);
    ref_rgb_ir(HDR_ref_out, rggb_out_ref, ir_out_ref, rows, cols);
    cv::imwrite("ref_ir_output.png", ir_out_ref);
    cv::imwrite("ref_rggb_output.png", rggb_out_ref);
#else
    HDR_ref_out.copyTo(rggb_out_ref);
    cv::imwrite("ref_rggb_buffer_output.png", rggb_out_ref);
#endif

//////////AEC_REF_SECTION//////////////////

// cv::Mat aec_ref_out;
// aec_ref_out.create(rows, cols, CV_IN_TYPE);
#if USE_AEC
    aec_ref(rggb_out_ref, aec_ref_out, pawb);
    cv::imwrite("aec_ref_out.png", aec_ref_out);
#else
    rggb_out_ref.copyTo(aec_ref_out);
    cv::imwrite("aec_ref_buffer_out.png", aec_ref_out);
#endif

//////////BLC_REF_SECTION//////////////////

// cv::Mat blacklevel_out_ref;
// blacklevel_out_ref.create(rows, cols, CV_IN_TYPE);
#if T_8U
    float MaxLevel = 255.0f; // Or White Level
#elif T_16U
    float MaxLevel = 65535.0f; // Or White Level
#endif
    float MulValue = (float)(MaxLevel / (MaxLevel - BLACK_LEVEL));
    for (int r = 0; r < rows; r++) {
        for (int c = 0; c < cols; c++) {
            unsigned short Pixel = aec_ref_out.at<unsigned short>(r, c);
            blacklevel_out_ref.at<unsigned short>(r, c) =
                cv::saturate_cast<unsigned short>((Pixel - BLACK_LEVEL) * MulValue);
        }
    }
    cv::imwrite("blacklevel_out_ref.png", blacklevel_out_ref);

    //////////BPC_REF_SECTION//////////////////

    // cv::Mat bpc_ref;
    // bpc_ref.create(rows, cols, CV_IN_TYPE);
    BadPixelCorrection(blacklevel_out_ref, bpc_ref);
    cv::imwrite("bpc_ref.png", bpc_ref);

//////////DEGAMMA_REF_SECTION//////////////////

// cv::Mat degamma_out_ref;
// degamma_out_ref.create(rows, cols, CV_IN_TYPE);
#if USE_DEGAMMA
    degamma_ref(bpc_ref, degamma_out_ref);
    cv::imwrite("degamma_out_ref.png", degamma_out_ref);
#else
    bpc_ref.copyTo(degamma_out_ref);
    cv::imwrite("degamma_buffer_out_ref.png", degamma_out_ref);
#endif

    //////////LSC_REF_SECTION//////////////////

    // cv::Mat lsc_out_ref;
    // lsc_out_ref.create(rows, cols, CV_IN_TYPE);
    LSC_ref(degamma_out_ref, lsc_out_ref);
    cv::imwrite("lsc_out_ref.png", lsc_out_ref);

    //////////GAIN_CONTROL_REF_SECTION//////////////////
    // cv::Mat gaincontrol_out_ref;
    // gaincontrol_out_ref.create(rows, cols, CV_IN_TYPE);
    int bformat = XF_BAYER_PATTERN;
    gainControlOCV(lsc_out_ref, gaincontrol_out_ref, bformat, rgain, bgain, ggain);
    cv::imwrite("gaincontrol_out_ref.png", gaincontrol_out_ref);

    //////////DEMOSAIC_REF_SECTION//////////////////

    // cv::Mat demosaic_out_ref(rows, cols, CV_OUT_TYPE);
    demosaicImage(gaincontrol_out_ref, demosaic_out_ref);
    cv::imwrite("demosaic_out_ref.png", demosaic_out_ref);

//////////AWB_REF_SECTION//////////////////

#if T_8U
    float inputMin = 0.0f;
    float inputMax = 255.0f;
    float outputMin = 0.0f;
    float outputMax = 255.0f;
    std::vector<cv::Mat_<uchar> > awb_input_ref;
    split(demosaic_out_ref, awb_input_ref);
#else T_16U
    float inputMin = 0.0f;
    float inputMax = 65535.0f;
    float outputMin = 0.0f;
    float outputMax = 65535.0f;
    std::vector<cv::Mat_<ushort> > awb_input_ref;
    split(demosaic_out_ref, awb_input_ref);
#endif
    // float thresh = (float)pawb / 256;
    float thresh = 0.9;

// cv::Mat awb_out_ref;
// awb_out_ref.create(rows, cols, CV_OUT_TYPE);

#if USE_AWB
#if (WB_TYPE == 0)
    // gray world white balancing algorithm
    balanceWhiteGW(demosaic_out_ref, awb_out_ref);
    cv::imwrite("awb_gw_out_ref.png", awb_out_ref);

#else
    // simple white balancing algorithm
    balanceWhiteSimple(awb_input_ref, awb_out_ref, inputMin, inputMax, outputMin, outputMax, thresh);
    cv::imwrite("awb_simple_out_ref.png", awb_out_ref);
#endif

#else
    demosaic_out_ref.copyTo(awb_out_ref);
    cv::imwrite("awb_buffer_out_ref.png", awb_out_ref);
#endif

//////////CCM_REF_SECTION//////////////////

// cv::Mat ccm_out_ref(rows, cols, CV_OUT_TYPE);
#if USE_CCM
    colorcorrectionmatrix(awb_out_ref, ccm_out_ref);
    cv::imwrite("ccm_out_ref.png", ccm_out_ref);
#else
    awb_out_ref.copyTo(ccm_out_ref);
    cv::imwrite("ccm_buffer_out_ref.png", ccm_out_ref);
#endif

//////////FUNCTION_TM_REF_SECTION//////////////////

// cv::Mat tm_out_ref;
// tm_out_ref.create(rows, cols, CV_GTM_TYPE);

#if USE_GTM
    global_tone_mapping_ref(ccm_out_ref, tm_out_ref);
    cv::imwrite("gtm_out_ref.png", tm_out_ref);
#elif USE_QND
    tm_out_ref = floyd_steinberg_dithering(ccm_out_ref, SCALEFACTOR);
    cv::imwrite("qnd_out_ref.png", tm_out_ref);
#elif USE_LTM
    ltm_ref(ccm_out_ref, tm_out_ref, rows, cols, blk_height, blk_width);
    cv::imwrite("ltm_out_ref.png", tm_out_ref);
#else
    ccm_out_ref.copyTo(tm_out_ref);
    cv::imwrite("function_tm_out_ref.png", tm_out_ref);
#endif

    //////////GAMMACORRECTION_REF_SECTION//////////////////

    // cv::Mat gamma_out_ref;
    // gamma_out_ref.create(rows, cols, CV_GTM_TYPE);
    unsigned char gamma_lut[256 * 3];
    float gamma_val_r = 0.5f, gamma_val_g = 0.8f, gamma_val_b = 0.8f;

    compute_gamma_ref(gamma_val_r, gamma_val_g, gamma_val_b, gamma_lut);

    unsigned char gam_r[256], gam_g[256], gam_b[256];
    for (int i = 0; i < 256; i++) {
        gam_r[i] = gamma_lut[i];
        gam_g[i] = gamma_lut[i + 256];
        gam_b[i] = gamma_lut[i + 512];
    }

    gammacorrection_ref(tm_out_ref, gamma_out_ref, gam_r, gam_g, gam_b, rows, cols);
    cv::imwrite("gamma_out_ref.png", gamma_out_ref);

//////////3DLUT_REF_SECTION//////////////////
// cv::Mat lut3d_out_ref(rows, cols, CV_GTM_TYPE);

#if USE_3DLUT
    lut3d_ref(rows, cols, lut_dim, gamma_out_ref, lut, lut3d_out_ref);
    cv::imwrite("lut3d_out_ref.png", lut3d_out_ref);

#else
    gamma_out_ref.copyTo(lut3d_out_ref);
    cv::imwrite("3dlut_buffer_out_ref.png", lut3d_out_ref);
#endif

//////////RGB2YUYV_REF_SECTION//////////////////
// cv::Mat yuyv_out_ref(rows,cols, CV_16UC1);
// cvtColor(lut3d_out_ref, lut3d_out_ref, cv::COLOR_BGR2RGB);

#if USE_CSC
    bgr2yuyv_ref(lut3d_out_ref, yuyv_out_ref);
#else
    lut3d_out_ref.copyTo(yuyv_out_ref);
#endif

    //////////OUTPUT//////////////////
    yuyv_out_ref.copyTo(final_ocv);
    cv::imwrite("final_ocv.png", final_ocv);
}

#endif