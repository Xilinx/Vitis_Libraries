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

#include "xf_cvt_color_config.h"

#if RGBA2IYUV
void cvtcolor_rgba2iyuv(xf::cv::Mat<XF_8UC4, HEIGHT, WIDTH, NPC1>& imgInput,
                        xf::cv::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1>& imgOutput0,
                        xf::cv::Mat<XF_8UC1, HEIGHT / 4, WIDTH, NPC1>& imgOutput1,
                        xf::cv::Mat<XF_8UC1, HEIGHT / 4, WIDTH, NPC1>& imgOutput2) {
    xf::cv::rgba2iyuv<XF_8UC4, XF_8UC1, HEIGHT, WIDTH, NPC1>(imgInput, imgOutput0, imgOutput1, imgOutput2);
}
#endif
#if RGBA2NV12
void cvtcolor_rgba2nv12(xf::cv::Mat<XF_8UC4, HEIGHT, WIDTH, NPC1>& imgInput,
                        xf::cv::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1>& imgOutput0,
                        xf::cv::Mat<XF_8UC2, HEIGHT / 2, WIDTH / 2, NPC2>& imgOutput1) {
    xf::cv::rgba2nv12<XF_8UC4, XF_8UC1, XF_8UC2, HEIGHT, WIDTH, NPC1, NPC2>(imgInput, imgOutput0, imgOutput1);
}
#endif
#if RGBA2NV21
void cvtcolor_rgba2nv21(xf::cv::Mat<XF_8UC4, HEIGHT, WIDTH, NPC1>& imgInput,
                        xf::cv::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1>& imgOutput0,
                        xf::cv::Mat<XF_8UC2, HEIGHT / 2, WIDTH / 2, NPC2>& imgOutput1) {
    xf::cv::rgba2nv21<XF_8UC4, XF_8UC1, XF_8UC2, HEIGHT, WIDTH, NPC1, NPC2>(imgInput, imgOutput0, imgOutput1);
}
#endif
#if RGBA2YUV4
void cvtcolor_rgba2yuv4(xf::cv::Mat<XF_8UC4, HEIGHT, WIDTH, NPC1>& imgInput,
                        xf::cv::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1>& imgOutput0,
                        xf::cv::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1>& imgOutput1,
                        xf::cv::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1>& imgOutput2) {
    xf::cv::rgba2yuv4<XF_8UC4, XF_8UC1, HEIGHT, WIDTH, NPC1>(imgInput, imgOutput0, imgOutput1, imgOutput2);
}
#endif

#if RGB2IYUV
void cvtcolor_rgb2iyuv(xf::cv::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1>& imgInput,
                       xf::cv::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1>& imgOutput0,
                       xf::cv::Mat<XF_8UC1, HEIGHT / 4, WIDTH, NPC1>& imgOutput1,
                       xf::cv::Mat<XF_8UC1, HEIGHT / 4, WIDTH, NPC1>& imgOutput2) {
    xf::cv::rgb2iyuv<XF_8UC3, XF_8UC1, HEIGHT, WIDTH, NPC1>(imgInput, imgOutput0, imgOutput1, imgOutput2);
}
#endif
#if RGB2NV12
void cvtcolor_rgb2nv12(xf::cv::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1>& imgInput,
                       xf::cv::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1>& imgOutput0,
                       xf::cv::Mat<XF_8UC2, HEIGHT / 2, WIDTH / 2, NPC2>& imgOutput1) {
    xf::cv::rgb2nv12<XF_8UC3, XF_8UC1, XF_8UC2, HEIGHT, WIDTH, NPC1, NPC2>(imgInput, imgOutput0, imgOutput1);
}
#endif
#if RGB2NV21
void cvtcolor_rgb2nv21(xf::cv::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1>& imgInput,
                       xf::cv::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1>& imgOutput0,
                       xf::cv::Mat<XF_8UC2, HEIGHT / 2, WIDTH / 2, NPC2>& imgOutput1) {
    xf::cv::rgb2nv21<XF_8UC3, XF_8UC1, XF_8UC2, HEIGHT, WIDTH, NPC1, NPC2>(imgInput, imgOutput0, imgOutput1);
}
#endif
#if RGB2YUV4
void cvtcolor_rgb2yuv4(xf::cv::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1>& imgInput,
                       xf::cv::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1>& imgOutput0,
                       xf::cv::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1>& imgOutput1,
                       xf::cv::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1>& imgOutput2) {
    xf::cv::rgb2yuv4<XF_8UC3, XF_8UC1, HEIGHT, WIDTH, NPC1>(imgInput, imgOutput0, imgOutput1, imgOutput2);
}
#endif
#if RGB2UYVY
void cvtcolor_rgb2uyvy(xf::cv::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1>& imgInput,
                       xf::cv::Mat<XF_16UC1, HEIGHT, WIDTH, NPC1>& imgOutput0) {
    xf::cv::rgb2uyvy<XF_8UC3, XF_16UC1, HEIGHT, WIDTH, NPC1>(imgInput, imgOutput0);
}
#endif
#if RGB2YUYV
void cvtcolor_rgb2yuyv(xf::cv::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1>& imgInput,
                       xf::cv::Mat<XF_16UC1, HEIGHT, WIDTH, NPC1>& imgOutput0) {
    xf::cv::rgb2yuyv<XF_8UC3, XF_16UC1, HEIGHT, WIDTH, NPC1>(imgInput, imgOutput0);
}
#endif
#if RGB2BGR
void cvtcolor_rgb2bgr(xf::cv::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1>& imgInput,
                      xf::cv::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1>& imgOutput0) {
    xf::cv::rgb2bgr<XF_8UC3, XF_8UC3, HEIGHT, WIDTH, NPC1>(imgInput, imgOutput0);
}
#endif

#if BGR2UYVY
void cvtcolor_bgr2uyvy(xf::cv::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1>& imgInput,
                       xf::cv::Mat<XF_16UC1, HEIGHT, WIDTH, NPC1>& imgOutput0) {
    xf::cv::bgr2uyvy<XF_8UC3, XF_16UC1, HEIGHT, WIDTH, NPC1>(imgInput, imgOutput0);
}
#endif
#if BGR2YUYV
void cvtcolor_bgr2yuyv(xf::cv::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1>& imgInput,
                       xf::cv::Mat<XF_16UC1, HEIGHT, WIDTH, NPC1>& imgOutput0) {
    xf::cv::bgr2yuyv<XF_8UC3, XF_16UC1, HEIGHT, WIDTH, NPC1>(imgInput, imgOutput0);
}
#endif
#if BGR2RGB
void cvtcolor_bgr2rgb(xf::cv::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1>& imgInput,
                      xf::cv::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1>& imgOutput0) {
    xf::cv::bgr2rgb<XF_8UC3, XF_8UC3, HEIGHT, WIDTH, NPC1>(imgInput, imgOutput0);
}
#endif
#if BGR2NV12
void cvtcolor_bgr2nv12(xf::cv::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1>& imgInput,
                       xf::cv::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1>& imgOutput0,
                       xf::cv::Mat<XF_8UC2, HEIGHT / 2, WIDTH / 2, NPC2>& imgOutput1) {
    xf::cv::bgr2nv12<XF_8UC3, XF_8UC1, XF_8UC2, HEIGHT, WIDTH, NPC1, NPC2>(imgInput, imgOutput0, imgOutput1);
}
#endif
#if BGR2NV21
void cvtcolor_bgr2nv21(xf::cv::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1>& imgInput,
                       xf::cv::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1>& imgOutput0,
                       xf::cv::Mat<XF_8UC2, HEIGHT / 2, WIDTH / 2, NPC2>& imgOutput1) {
    xf::cv::bgr2nv21<XF_8UC3, XF_8UC1, XF_8UC2, HEIGHT, WIDTH, NPC1, NPC2>(imgInput, imgOutput0, imgOutput1);
}
#endif

#if IYUV2NV12
void cvtcolor_iyuv2nv12(xf::cv::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1>& imgInput0,
                        xf::cv::Mat<XF_8UC1, HEIGHT / 4, WIDTH, NPC1>& imgInput1,
                        xf::cv::Mat<XF_8UC1, HEIGHT / 4, WIDTH, NPC1>& imgInput2,
                        xf::cv::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1>& imgOutput0,
                        xf::cv::Mat<XF_8UC2, HEIGHT / 2, WIDTH / 2, NPC2>& imgOutput1) {
    xf::cv::iyuv2nv12<XF_8UC1, XF_8UC2, HEIGHT, WIDTH, NPC1, NPC2>(imgInput0, imgInput1, imgInput2, imgOutput0,
                                                                   imgOutput1);
}
#endif
#if IYUV2RGBA
void cvtcolor_iyuv2rgba(xf::cv::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1>& imgInput0,
                        xf::cv::Mat<XF_8UC1, HEIGHT / 4, WIDTH, NPC1>& imgInput1,
                        xf::cv::Mat<XF_8UC1, HEIGHT / 4, WIDTH, NPC1>& imgInput2,
                        xf::cv::Mat<XF_8UC4, HEIGHT, WIDTH, NPC1>& imgOutput0) {
    xf::cv::iyuv2rgba<XF_8UC1, XF_8UC4, HEIGHT, WIDTH, NPC1>(imgInput0, imgInput1, imgInput2, imgOutput0);
}
#endif
#if IYUV2RGB
void cvtcolor_iyuv2rgb(xf::cv::Mat<XF_8UC1, HEIGHT, WIDTH, XF_NPPC1>& imgInput0,
                       xf::cv::Mat<XF_8UC1, HEIGHT / 4, WIDTH, XF_NPPC1>& imgInput1,
                       xf::cv::Mat<XF_8UC1, HEIGHT / 4, WIDTH, XF_NPPC1>& imgInput2,
                       xf::cv::Mat<XF_8UC3, HEIGHT, WIDTH, XF_NPPC1>& imgOutput0) {
    xf::cv::iyuv2rgb<XF_8UC1, XF_8UC3, HEIGHT, WIDTH, XF_NPPC1>(imgInput0, imgInput1, imgInput2, imgOutput0);
}
#endif

#if IYUV2YUV4
void cvtcolor_iyuv2yuv4(xf::cv::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1>& imgInput0,
                        xf::cv::Mat<XF_8UC1, HEIGHT / 4, WIDTH, NPC1>& imgInput1,
                        xf::cv::Mat<XF_8UC1, HEIGHT / 4, WIDTH, NPC1>& imgInput2,
                        xf::cv::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1>& imgOutput0,
                        xf::cv::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1>& imgOutput1,
                        xf::cv::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1>& imgOutput2) {
    xf::cv::iyuv2yuv4<XF_8UC1, HEIGHT, WIDTH, NPC1>(imgInput0, imgInput1, imgInput2, imgOutput0, imgOutput1,
                                                    imgOutput2);
}
#endif

#if NV122IYUV
void cvtcolor_nv122iyuv(xf::cv::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1>& imgInput0,
                        xf::cv::Mat<XF_8UC2, HEIGHT / 2, WIDTH / 2, NPC2>& imgInput1,
                        xf::cv::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1>& imgOutput0,
                        xf::cv::Mat<XF_8UC1, HEIGHT / 4, WIDTH, NPC1>& imgOutput1,
                        xf::cv::Mat<XF_8UC1, HEIGHT / 4, WIDTH, NPC1>& imgOutput2) {
    xf::cv::nv122iyuv<XF_8UC1, XF_8UC2, HEIGHT, WIDTH, NPC1, NPC2>(imgInput0, imgInput1, imgOutput0, imgOutput1,
                                                                   imgOutput2);
}
#endif

#if NV122RGBA
void cvtcolor_nv122rgba(xf::cv::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1>& imgInput0,
                        xf::cv::Mat<XF_8UC2, HEIGHT / 2, WIDTH / 2, NPC2>& imgInput1,
                        xf::cv::Mat<XF_8UC4, HEIGHT, WIDTH, NPC1>& imgOutput0) {
    xf::cv::nv122rgba<XF_8UC1, XF_8UC2, XF_8UC4, HEIGHT, WIDTH, NPC1, NPC2>(imgInput0, imgInput1, imgOutput0);
}
#endif
#if NV122YUV4
void cvtcolor_nv122yuv4(xf::cv::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1>& imgInput0,
                        xf::cv::Mat<XF_8UC2, HEIGHT / 2, WIDTH / 2, NPC2>& imgInput1,
                        xf::cv::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1>& imgOutput0,
                        xf::cv::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1>& imgOutput1,
                        xf::cv::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1>& imgOutput2) {
    xf::cv::nv122yuv4<XF_8UC1, XF_8UC2, HEIGHT, WIDTH, NPC1, NPC2>(imgInput0, imgInput1, imgOutput0, imgOutput1,
                                                                   imgOutput2);
}
#endif
#if NV122RGB
void cvtcolor_nv122rgb(xf::cv::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1>& imgInput0,
                       xf::cv::Mat<XF_8UC2, HEIGHT / 2, WIDTH / 2, NPC2>& imgInput1,
                       xf::cv::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1>& imgOutput0) {
    xf::cv::nv122rgb<XF_8UC1, XF_8UC2, XF_8UC3, HEIGHT, WIDTH, NPC1, NPC2>(imgInput0, imgInput1, imgOutput0);
}
#endif
#if NV122BGR
void cvtcolor_nv122bgr(xf::cv::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1>& imgInput0,
                       xf::cv::Mat<XF_8UC2, HEIGHT / 2, WIDTH / 2, NPC2>& imgInput1,
                       xf::cv::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1>& imgOutput0) {
    xf::cv::nv122bgr<XF_8UC1, XF_8UC2, XF_8UC3, HEIGHT, WIDTH, NPC1, NPC2>(imgInput0, imgInput1, imgOutput0);
}
#endif
#if NV122UYVY
void cvtcolor_nv122uyvy(xf::cv::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1>& imgInput0,
                        xf::cv::Mat<XF_8UC2, HEIGHT / 2, WIDTH / 2, NPC2>& imgInput1,
                        xf::cv::Mat<XF_16UC1, HEIGHT, WIDTH, NPC1>& imgOutput0) {
    xf::cv::nv122uyvy<XF_8UC1, XF_8UC2, XF_16UC1, HEIGHT, WIDTH, NPC1, NPC2>(imgInput0, imgInput1, imgOutput0);
}
#endif
#if NV122YUYV
void cvtcolor_nv122yuyv(xf::cv::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1>& imgInput0,
                        xf::cv::Mat<XF_8UC2, HEIGHT / 2, WIDTH / 2, NPC2>& imgInput1,
                        xf::cv::Mat<XF_16UC1, HEIGHT, WIDTH, NPC1>& imgOutput0) {
    xf::cv::nv122yuyv<XF_8UC1, XF_8UC2, XF_16UC1, HEIGHT, WIDTH, NPC1, NPC2>(imgInput0, imgInput1, imgOutput0);
}
#endif
#if NV122NV21
void cvtcolor_nv122nv21(xf::cv::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1>& imgInput0,
                        xf::cv::Mat<XF_8UC2, HEIGHT / 2, WIDTH / 2, NPC2>& imgInput1,
                        xf::cv::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1>& imgOutput0,
                        xf::cv::Mat<XF_8UC2, HEIGHT / 2, WIDTH / 2, NPC2>& imgOutput1) {
    xf::cv::nv122nv21<XF_8UC1, XF_8UC2, HEIGHT, WIDTH, NPC1, NPC2>(imgInput0, imgInput1, imgOutput0, imgOutput1);
}
#endif

#if NV212IYUV
void cvtcolor_nv212iyuv(xf::cv::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1>& imgInput0,
                        xf::cv::Mat<XF_8UC2, HEIGHT / 2, WIDTH / 2, NPC2>& imgInput1,
                        xf::cv::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1>& imgOutput0,
                        xf::cv::Mat<XF_8UC1, HEIGHT / 4, WIDTH, NPC1>& imgOutput1,
                        xf::cv::Mat<XF_8UC1, HEIGHT / 4, WIDTH, NPC1>& imgOutput2) {
    xf::cv::nv212iyuv<XF_8UC1, XF_8UC2, HEIGHT, WIDTH, NPC1, NPC2>(imgInput0, imgInput1, imgOutput0, imgOutput1,
                                                                   imgOutput2);
}
#endif
#if NV212RGBA
void cvtcolor_nv212rgba(xf::cv::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1>& imgInput0,
                        xf::cv::Mat<XF_8UC2, HEIGHT / 2, WIDTH / 2, NPC2>& imgInput1,
                        xf::cv::Mat<XF_8UC4, HEIGHT, WIDTH, NPC1>& imgOutput0) {
    xf::cv::nv212rgba<XF_8UC1, XF_8UC2, XF_8UC4, HEIGHT, WIDTH, NPC1, NPC2>(imgInput0, imgInput1, imgOutput0);
}
#endif
#if NV212RGB
void cvtcolor_nv212rgb(xf::cv::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1>& imgInput0,
                       xf::cv::Mat<XF_8UC2, HEIGHT / 2, WIDTH / 2, NPC2>& imgInput1,
                       xf::cv::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1>& imgOutput0) {
    xf::cv::nv212rgb<XF_8UC1, XF_8UC2, XF_8UC3, HEIGHT, WIDTH, NPC1, NPC2>(imgInput0, imgInput1, imgOutput0);
}
#endif
#if NV212BGR
void cvtcolor_nv212bgr(xf::cv::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1>& imgInput0,
                       xf::cv::Mat<XF_8UC2, HEIGHT / 2, WIDTH / 2, NPC2>& imgInput1,
                       xf::cv::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1>& imgOutput0) {
    xf::cv::nv212bgr<XF_8UC1, XF_8UC2, XF_8UC3, HEIGHT, WIDTH, NPC1, NPC2>(imgInput0, imgInput1, imgOutput0);
}
#endif
#if NV212YUV4
void cvtcolor_nv212yuv4(xf::cv::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1>& imgInput0,
                        xf::cv::Mat<XF_8UC2, HEIGHT / 2, WIDTH / 2, NPC2>& imgInput1,
                        xf::cv::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1>& imgOutput0,
                        xf::cv::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1>& imgOutput1,
                        xf::cv::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1>& imgOutput2) {
    xf::cv::nv212yuv4<XF_8UC1, XF_8UC2, HEIGHT, WIDTH, NPC1, NPC2>(imgInput0, imgInput1, imgOutput0, imgOutput1,
                                                                   imgOutput2);
}
#endif
#if NV212UYVY
void cvtcolor_nv212uyvy(xf::cv::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1>& imgInput0,
                        xf::cv::Mat<XF_8UC2, HEIGHT / 2, WIDTH / 2, NPC2>& imgInput1,
                        xf::cv::Mat<XF_16UC1, HEIGHT, WIDTH, NPC1>& imgOutput0) {
    xf::cv::nv212uyvy<XF_8UC1, XF_8UC2, XF_16UC1, HEIGHT, WIDTH, NPC1, NPC2>(imgInput0, imgInput1, imgOutput0);
}
#endif
#if NV212YUYV
void cvtcolor_nv212yuyv(xf::cv::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1>& imgInput0,
                        xf::cv::Mat<XF_8UC2, HEIGHT / 2, WIDTH / 2, NPC2>& imgInput1,
                        xf::cv::Mat<XF_16UC1, HEIGHT, WIDTH, NPC1>& imgOutput0) {
    xf::cv::nv212yuyv<XF_8UC1, XF_8UC2, XF_16UC1, HEIGHT, WIDTH, NPC1, NPC2>(imgInput0, imgInput1, imgOutput0);
}
#endif
#if NV212NV12
void cvtcolor_nv212nv12(xf::cv::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1>& imgInput0,
                        xf::cv::Mat<XF_8UC2, HEIGHT / 2, WIDTH / 2, NPC2>& imgInput1,
                        xf::cv::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1>& imgOutput0,
                        xf::cv::Mat<XF_8UC2, HEIGHT / 2, WIDTH / 2, NPC2>& imgOutput1) {
    xf::cv::nv212nv12<XF_8UC1, XF_8UC2, HEIGHT, WIDTH, NPC1, NPC2>(imgInput0, imgInput1, imgOutput0, imgOutput1);
}
#endif

#if UYVY2IYUV
void cvtcolor_uyvy2iyuv(xf::cv::Mat<XF_16UC1, HEIGHT, WIDTH, NPC1>& imgInput,
                        xf::cv::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1>& imgOutput0,
                        xf::cv::Mat<XF_8UC1, HEIGHT / 4, WIDTH, NPC1>& imgOutput1,
                        xf::cv::Mat<XF_8UC1, HEIGHT / 4, WIDTH, NPC1>& imgOutput2) {
    xf::cv::uyvy2iyuv<XF_16UC1, XF_8UC1, HEIGHT, WIDTH, NPC1>(imgInput, imgOutput0, imgOutput1, imgOutput2);
}
#endif
#if UYVY2NV12
void cvtcolor_uyvy2nv12(xf::cv::Mat<XF_16UC1, HEIGHT, WIDTH, NPC1>& imgInput,
                        xf::cv::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1>& imgOutput0,
                        xf::cv::Mat<XF_8UC2, HEIGHT / 2, WIDTH / 2, NPC2>& imgOutput1) {
    xf::cv::uyvy2nv12<XF_16UC1, XF_8UC1, XF_8UC2, HEIGHT, WIDTH, NPC1, NPC2>(imgInput, imgOutput0, imgOutput1);
}
#endif
#if UYVY2NV21
void cvtcolor_uyvy2nv21(xf::cv::Mat<XF_16UC1, HEIGHT, WIDTH, NPC1>& imgInput,
                        xf::cv::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1>& imgOutput0,
                        xf::cv::Mat<XF_8UC2, HEIGHT / 2, WIDTH / 2, NPC2>& imgOutput1) {
    xf::cv::uyvy2nv21<XF_16UC1, XF_8UC1, XF_8UC2, HEIGHT, WIDTH, NPC1, NPC2>(imgInput, imgOutput0, imgOutput1);
}
#endif
#if UYVY2RGBA
void cvtcolor_uyvy2rgba(xf::cv::Mat<XF_16UC1, HEIGHT, WIDTH, NPC1>& imgInput,
                        xf::cv::Mat<XF_8UC4, HEIGHT, WIDTH, NPC1>& imgOutput0) {
    xf::cv::uyvy2rgba<XF_16UC1, XF_8UC4, HEIGHT, WIDTH, NPC1>(imgInput, imgOutput0);
}
#endif
#if UYVY2RGB
void cvtcolor_uyvy2rgb(xf::cv::Mat<XF_16UC1, HEIGHT, WIDTH, NPC1>& imgInput,
                       xf::cv::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1>& imgOutput0) {
    xf::cv::uyvy2rgb<XF_16UC1, XF_8UC3, HEIGHT, WIDTH, NPC1>(imgInput, imgOutput0);
}
#endif
#if UYVY2BGR
void cvtcolor_uyvy2bgr(xf::cv::Mat<XF_16UC1, HEIGHT, WIDTH, NPC1>& imgInput,
                       xf::cv::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1>& imgOutput0) {
    xf::cv::uyvy2bgr<XF_16UC1, XF_8UC3, HEIGHT, WIDTH, NPC1>(imgInput, imgOutput0);
}
#endif
#if UYVY2YUYV
void cvtcolor_uyvy2yuyv(xf::cv::Mat<XF_16UC1, HEIGHT, WIDTH, NPC1>& imgInput,
                        xf::cv::Mat<XF_16UC1, HEIGHT, WIDTH, NPC1>& imgOutput0) {
    xf::cv::uyvy2yuyv<XF_16UC1, XF_16UC1, HEIGHT, WIDTH, NPC1>(imgInput, imgOutput0);
}
#endif

#if YUYV2IYUV
void cvtcolor_yuyv2iyuv(xf::cv::Mat<XF_16UC1, HEIGHT, WIDTH, NPC1>& imgInput,
                        xf::cv::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1>& imgOutput0,
                        xf::cv::Mat<XF_8UC1, HEIGHT / 4, WIDTH, NPC1>& imgOutput1,
                        xf::cv::Mat<XF_8UC1, HEIGHT / 4, WIDTH, NPC1>& imgOutput2) {
    xf::cv::yuyv2iyuv<XF_16UC1, XF_8UC1, HEIGHT, WIDTH, NPC1>(imgInput, imgOutput0, imgOutput1, imgOutput2);
}
#endif
#if YUYV2NV12
void cvtcolor_yuyv2nv12(xf::cv::Mat<XF_16UC1, HEIGHT, WIDTH, NPC1>& imgInput,
                        xf::cv::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1>& imgOutput0,
                        xf::cv::Mat<XF_8UC2, HEIGHT / 2, WIDTH / 2, NPC2>& imgOutput1) {
    xf::cv::yuyv2nv12<XF_16UC1, XF_8UC1, XF_8UC2, HEIGHT, WIDTH, NPC1, NPC2>(imgInput, imgOutput0, imgOutput1);
}
#endif
#if YUYV2NV21
void cvtcolor_yuyv2nv21(xf::cv::Mat<XF_16UC1, HEIGHT, WIDTH, NPC1>& imgInput,
                        xf::cv::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1>& imgOutput0,
                        xf::cv::Mat<XF_8UC2, HEIGHT / 2, WIDTH / 2, NPC2>& imgOutput1) {
    xf::cv::yuyv2nv21<XF_16UC1, XF_8UC1, XF_8UC2, HEIGHT, WIDTH, NPC1, NPC2>(imgInput, imgOutput0, imgOutput1);
}
#endif
#if YUYV2RGBA
void cvtcolor_yuyv2rgba(xf::cv::Mat<XF_16UC1, HEIGHT, WIDTH, NPC1>& imgInput,
                        xf::cv::Mat<XF_8UC4, HEIGHT, WIDTH, NPC1>& imgOutput0) {
    xf::cv::yuyv2rgba<XF_16UC1, XF_8UC4, HEIGHT, WIDTH, NPC1>(imgInput, imgOutput0);
}
#endif
#if YUYV2RGB
void cvtcolor_yuyv2rgb(xf::cv::Mat<XF_16UC1, HEIGHT, WIDTH, NPC1>& imgInput,
                       xf::cv::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1>& imgOutput0) {
    xf::cv::yuyv2rgb<XF_16UC1, XF_8UC3, HEIGHT, WIDTH, NPC1>(imgInput, imgOutput0);
}
#endif
#if YUYV2BGR
void cvtcolor_yuyv2bgr(xf::cv::Mat<XF_16UC1, HEIGHT, WIDTH, NPC1>& imgInput,
                       xf::cv::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1>& imgOutput0) {
    xf::cv::yuyv2bgr<XF_16UC1, XF_8UC3, HEIGHT, WIDTH, NPC1>(imgInput, imgOutput0);
}
#endif
#if YUYV2UYVY
void cvtcolor_yuyv2uyvy(xf::cv::Mat<XF_16UC1, HEIGHT, WIDTH, NPC1>& imgInput,
                        xf::cv::Mat<XF_16UC1, HEIGHT, WIDTH, NPC1>& imgOutput0) {
    xf::cv::yuyv2uyvy<XF_16UC1, XF_16UC1, HEIGHT, WIDTH, NPC1>(imgInput, imgOutput0);
}
#endif

#if RGB2GRAY
void cvtcolor_rgb2gray(xf::cv::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1>& imgInput,
                       xf::cv::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1>& imgOutput) {
    xf::cv::rgb2gray<XF_8UC3, XF_8UC1, HEIGHT, WIDTH, NPC1>(imgInput, imgOutput);
}
#endif
#if BGR2GRAY
void cvtcolor_bgr2gray(xf::cv::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1>& imgInput,
                       xf::cv::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1>& imgOutput) {
    xf::cv::bgr2gray<XF_8UC3, XF_8UC1, HEIGHT, WIDTH, NPC1>(imgInput, imgOutput);
}
#endif
#if GRAY2RGB
void cvtcolor_gray2rgb(xf::cv::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1>& imgInput,
                       xf::cv::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1>& imgOutput) {
    xf::cv::gray2rgb<XF_8UC1, XF_8UC3, HEIGHT, WIDTH, NPC1>(imgInput, imgOutput);
}
#endif
#if GRAY2BGR
void cvtcolor_gray2bgr(xf::cv::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1>& imgInput,
                       xf::cv::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1>& imgOutput) {
    xf::cv::gray2bgr<XF_8UC1, XF_8UC3, HEIGHT, WIDTH, NPC1>(imgInput, imgOutput);
}
#endif
#if RGB2XYZ
void cvtcolor_rgb2xyz(xf::cv::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1>& imgInput,
                      xf::cv::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1>& imgOutput) {
    xf::cv::rgb2xyz<XF_8UC3, XF_8UC3, HEIGHT, WIDTH, NPC1>(imgInput, imgOutput);
}
#endif
#if BGR2XYZ
void cvtcolor_bgr2xyz(xf::cv::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1>& imgInput,
                      xf::cv::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1>& imgOutput) {
    xf::cv::bgr2xyz<XF_8UC3, XF_8UC3, HEIGHT, WIDTH, NPC1>(imgInput, imgOutput);
}
#endif
#if XYZ2RGB
void cvtcolor_xyz2rgb(xf::cv::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1>& imgInput,
                      xf::cv::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1>& imgOutput) {
    xf::cv::xyz2rgb<XF_8UC3, XF_8UC3, HEIGHT, WIDTH, NPC1>(imgInput, imgOutput);
}
#endif
#if XYZ2BGR
void cvtcolor_xyz2bgr(xf::cv::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1>& imgInput,
                      xf::cv::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1>& imgOutput) {
    xf::cv::xyz2bgr<XF_8UC3, XF_8UC3, HEIGHT, WIDTH, NPC1>(imgInput, imgOutput);
}
#endif
#if RGB2YCrCb
void cvtcolor_rgb2ycrcb(xf::cv::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1>& imgInput,
                        xf::cv::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1>& imgOutput) {
    xf::cv::rgb2ycrcb<XF_8UC3, XF_8UC3, HEIGHT, WIDTH, NPC1>(imgInput, imgOutput);
}
#endif

#if BGR2YCrCb
void cvtcolor_bgr2ycrcb(xf::cv::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1>& imgInput,
                        xf::cv::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1>& imgOutput) {
    xf::cv::bgr2ycrcb<XF_8UC3, XF_8UC3, HEIGHT, WIDTH, NPC1>(imgInput, imgOutput);
}
#endif

#if YCrCb2RGB
void cvtcolor_ycrcb2rgb(xf::cv::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1>& imgInput,
                        xf::cv::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1>& imgOutput) {
    xf::cv::ycrcb2rgb<XF_8UC3, XF_8UC3, HEIGHT, WIDTH, NPC1>(imgInput, imgOutput);
}
#endif
#if YCrCb2BGR
void cvtcolor_ycrcb2bgr(xf::cv::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1>& imgInput,
                        xf::cv::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1>& imgOutput) {
    xf::cv::ycrcb2bgr<XF_8UC3, XF_8UC3, HEIGHT, WIDTH, NPC1>(imgInput, imgOutput);
}
#endif
#if RGB2HLS
void cvtcolor_rgb2hls(xf::cv::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1>& imgInput,
                      xf::cv::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1>& imgOutput) {
    xf::cv::rgb2hls<XF_8UC3, XF_8UC3, HEIGHT, WIDTH, NPC1>(imgInput, imgOutput);
}
#endif
#if BGR2HLS
void cvtcolor_bgr2hls(xf::cv::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1>& imgInput,
                      xf::cv::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1>& imgOutput) {
    xf::cv::bgr2hls<XF_8UC3, XF_8UC3, HEIGHT, WIDTH, NPC1>(imgInput, imgOutput);
}
#endif
#if HLS2RGB
void cvtcolor_hls2rgb(xf::cv::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1>& imgInput,
                      xf::cv::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1>& imgOutput) {
    xf::cv::hls2rgb<XF_8UC3, XF_8UC3, HEIGHT, WIDTH, NPC1>(imgInput, imgOutput);
}
#endif
#if HLS2BGR
void cvtcolor_hls2bgr(xf::cv::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1>& imgInput,
                      xf::cv::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1>& imgOutput) {
    xf::cv::hls2bgr<XF_8UC3, XF_8UC3, HEIGHT, WIDTH, NPC1>(imgInput, imgOutput);
}
#endif
#if RGB2HSV
void cvtcolor_rgb2hsv(xf::cv::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1>& imgInput,
                      xf::cv::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1>& imgOutput) {
    xf::cv::rgb2hsv<XF_8UC3, HEIGHT, WIDTH, NPC1>(imgInput, imgOutput);
}
#endif
#if BGR2HSV
void cvtcolor_bgr2hsv(xf::cv::Mat<XF_8UC3, HEIGHT, WIDTH, XF_NPPC1>& imgInput,
                      xf::cv::Mat<XF_8UC3, HEIGHT, WIDTH, XF_NPPC1>& imgOutput) {
    xf::cv::bgr2hsv<XF_8UC3, HEIGHT, WIDTH, XF_NPPC1>(imgInput, imgOutput);
}
#endif
#if HSV2RGB
void cvtcolor_hsv2rgb(xf::cv::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1>& imgInput,
                      xf::cv::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1>& imgOutput) {
    xf::cv::hsv2rgb<XF_8UC3, XF_8UC3, HEIGHT, WIDTH, NPC1>(imgInput, imgOutput);
}
#endif
#if HSV2BGR
void cvtcolor_hsv2bgr(xf::cv::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1>& imgInput,
                      xf::cv::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1>& imgOutput) {
    xf::cv::hsv2bgr<XF_8UC3, XF_8UC3, HEIGHT, WIDTH, NPC1>(imgInput, imgOutput);
}
#endif
