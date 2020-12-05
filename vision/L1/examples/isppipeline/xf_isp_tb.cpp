/***************************************************************************
 Copyright (c) 2020, Xilinx, Inc.
 All rights reserved.

 Redistribution and use in source and binary forms, with or without modification,
 are permitted provided that the following conditions are met:

 1. Redistributions of source code must retain the above copyright notice,
 this list of conditions and the following disclaimer.

 2. Redistributions in binary form must reproduce the above copyright notice,
 this list of conditions and the following disclaimer in the documentation
 and/or other materials provided with the distribution.

 3. Neither the name of the copyright holder nor the names of its contributors
 may be used to endorse or promote products derived from this software
 without specific prior written permission.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

 ***************************************************************************/

#include "common/xf_headers.hpp"
#include "xf_isp_types.h"

using namespace std;

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
                    case IPL_DEPTH_8U:
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
                            case IPL_DEPTH_8U: {
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

template <typename T>
void balanceWhiteSimple(std::vector<cv::Mat_<T> >& src,
                        cv::Mat& dst,
                        const float inputMin,
                        const float inputMax,
                        const float outputMin,
                        const float outputMax,
                        const float p) {
    /********************* Simple white balance *********************/
    const float s1 = p; // low quantile
    const float s2 = p; // high quantile

    int depth = 2; // depth of histogram tree
    if (src[0].depth() != CV_8U) ++depth;
    int bins = 16; // number of bins at each histogram level

    int nElements = int(pow((float)bins, (float)depth));

    int countval = 0;

    for (size_t i = 0; i < src.size(); ++i) {
        std::vector<int> hist(nElements, 0);

        typename cv::Mat_<T>::iterator beginIt = src[i].begin();
        typename cv::Mat_<T>::iterator endIt = src[i].end();

        for (typename cv::Mat_<T>::iterator it = beginIt; it != endIt; ++it) // histogram filling
        {
            int pos = 0;
            float minValue = inputMin - 0.5f;
            float maxValue = inputMax + 0.5f;
            T val = *it;

            float interval = float(maxValue - minValue) / bins;

            for (int j = 0; j < 1; ++j) {
                int currentBin = int((val - minValue + 1e-4f) / interval);
                ++hist[pos + currentBin];

                pos = (pos + currentBin) * bins;

                minValue = minValue + currentBin * interval;
                maxValue = minValue + interval;

                interval /= bins;
            }

            countval++;
        }
        FILE* fp = fopen("hist_val.txt", "w");

        for (auto ith = hist.begin(); ith != hist.end(); ith++) {
            fprintf(fp, "%d\n", *ith);
        }
        fclose(fp);

        int total = int(src[i].total());

        int p1 = 0, p2 = bins - 1;
        int n1 = 0, n2 = total;

        float minValue = inputMin - 0.5f;
        float maxValue = inputMax + 0.5f;

        float interval = (maxValue - minValue) / float(bins);

        for (int j = 0; j < 1; ++j)
        // searching for s1 and s2
        {
            while (n1 + hist[p1] < s1 * total / 100.0f) {
                n1 += hist[p1++];
                minValue += interval;

                // printf("ocv min vlaues:%f %d\n",minValue,n1);
            }
            p1 *= bins;

            while (n2 - hist[p2] > (100.0f - s2) * total / 100.0f) {
                n2 -= hist[p2--];
                maxValue -= interval;
            }
            p2 = (p2 + 1) * bins - 1;

            interval /= bins;
        }

        printf("%f %f\n", minValue, maxValue);

        src[i] = (outputMax - outputMin) * (src[i] - minValue) / (maxValue - minValue) + outputMin;
    }

    /****************************************************************/

    dst.create(/**/ src[0].size(), CV_MAKETYPE(src[0].depth(), int(src.size())) /**/);
    cv::merge(src, dst);

    printf("\ncountvalue:%d\n", countval);
}

int main(int argc, char** argv) {
    cv::Mat raw_input, final_output;

    InVideoStrm_t src_axi;
    OutVideoStrm_t dst_axi;
    int result = 0;
    unsigned char InColorFormat = 0;
    CvSize imgSize;
    int nrFrames = 1;
    // read input image
    raw_input = cv::imread(argv[1], -1);

    imgSize.height = raw_input.rows;
    imgSize.width = raw_input.cols;

#if T_8U || T_10U || T_12U
    // Allocate memory for final image
    final_output.create(raw_input.rows, raw_input.cols, CV_8UC3);
#else
    final_output.create(raw_input.rows, raw_input.cols, CV_8UC3);
#endif

    imwrite("input.png", raw_input);

    // As we are processing still image H/W kernel needs to be run twice

    for (int i = 0; i < 3; i++) {
        Mat2MultiBayerAXIvideo(raw_input, src_axi, InColorFormat);

        // Call IP Processing function
        ISPPipeline_accel(raw_input.cols, raw_input.rows, 0, src_axi, dst_axi);

        // Convert processed image back to CV image, then to XVID image
        MultiPixelAXIvideo2Mat(dst_axi, final_output, InColorFormat);
    }

    //	cv::Mat final_img;
    //
    //	final_img.create(raw_input.rows,raw_input.cols,CV_8UC3);
    //
    //	std::vector<cv::Mat_<unsigned char> > mv;
    //	split(final_output, mv);
    //
    //	// simple white balancing algorithm
    //	balanceWhiteSimple(mv, final_img, 0, 255, 0, 255, 0.2);

    imwrite("output.png", final_output);
    //
    //	imwrite("output.png", final_img);

    return 0;
}
