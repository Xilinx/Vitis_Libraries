# xfOpenCV/examples_sdaccel
Each of the folders inside examples folder aims to evaluate at least one of the xfOpenCV kernels.

Each example folder consists of a data folder, which contains the corresponding test images. Additionally, each example contains the following files -

| File Name | Description |
| :------------- | :------------- |
| xf_headers.h | Contains the headers required for host code (the code that runs on ARM) |
| xf_< example_name >_ config.h, xf_config_params.h | Contains the hardware kernel configuration information and includes the kernel headers |
| xf_< example_name >_ tb.cpp | Contains the main() function and evaluation code for each of the xfOpenCV kernels |
| xf_< example_name >_ accel.cpp | Contains the conversion functions from pointer to xf::Mat and vice-verca along with the function call for the specific xfOpenCV kernel |
| xcl2.cpp, xcl2.hpp | Contains Xilinx CL infrastructure functions |
| Makefile | Makefile to build the example in commandline |


The following table lists which xfOpenCV kernel(s) each example aims to evaluate -

| Example | Function Name |
| :------------- | :------------- |
| boundingbox | boundingbox |
| boxfilter | boxFilter |
| canny | Canny, EdgeTracing |
| channelextract | extractChannel |
| cornertracker | cornerHarris, cornersImgToList, pyrDown, cornerUpdate |
| crop | crop |
| cvtcolor | iyuv2nv12, iyuv2rgba, iyuv2yuv4, nv122iyuv, nv122rgba, nv122yuv4, nv212iyuv, nv212rgba, nv212yuv4, rgba2yuv4, rgba2iyuv, rgba2nv12, rgba2nv21, uyvy2iyuv, uyvy2nv12, uyvy2rgba, yuyv2iyuv, yuyv2nv12, yuyv2rgba, rgb2gray, bgr2gray, gray2rgb, gray2bgr, rgb2xyz, bgr2xyz, xyz2rgb, xyz2bgr, rgb2ycrcb, bgr2ycrcb, ycrcb2rgb, ycrcb2bgr, rgb2hsv, bgr2hsv, hsv2rgb, hsv2bgr, rgb2hls, bgr2hls, hls2rgb, hls2bgr, bgr2uyvy, bgr2yuyv, bgr2rgb, bgr2nv12, bgr2nv21, iyuv2rgb, nv122rgb, nv122bgr, nv122yuyv, nv122uyvy, nv122nv21, nv212rgb, nv212bgr, nv212uyvy, nv212yuyv, nv212nv12, rgb2iyuv, rgb2nv12, rgb2nv21, rgb2yuv4, rgb2uyvy, rgb2yuyv, rgb2bgr, uyvy2nv21, uyvy2rgb, uyvy2bgr, uyvy2yuyv, yuyv2nv21, yuyv2rgb, yuyv2bgr, yuyv2uyvy |
| dilation | dilate |
| gaussianfilter | GaussianBlur |
| harris | cornerHarris |
| histogram | calcHist |
| histequialize | equalizeHist |
| integralimg | integral |
| kalmanfilter | kalmanfilter |
| lkdensepyrof | DensePyrOpticalFlow |
| lknpyroflow | DenseNonPyrLKOpticalFlow |
| magnitude | magnitude |
| meanshifttracking | MeanShift |
| phase | phase |
| pyrdown | pyrDown |
| pyrup | pyrUp |
| resize | resize |
| scharr | Scharr |
| sobelfilter | Sobel |
| stereopipeline | InitUndistortRectifyMapInverse, remap, StereoBM |
| threshold | Threshold |
