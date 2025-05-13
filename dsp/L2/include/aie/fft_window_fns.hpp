/*
 * Copyright (C) 2019-2022, Xilinx, Inc.
 * Copyright (C) 2022-2025, Advanced Micro Devices, Inc.
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
#ifndef _DSPLIB_FFT_WINDOW_FNS_HPP_
#define _DSPLIB_FFT_WINDOW_FNS_HPP_

namespace xf {
namespace dsp {
namespace aie {
namespace fft {
namespace windowfn {

/**
 * @addtogroup  fft_window FFT Window
 *
 * @{
 *
 */

/**
 * @defgroup fft_window_utils FFT Window utils
 *
 * @ingroup fft_window
 *
 * The FFT Window utilities contain helper functions to create windowing data structures.
 *
 */

/**
 * @ingroup fft_window_utils
 * @brief getHammingWindow is utility to create Hamming window.
 * @tparam T_C describes the type of weights in the FFT window.
 * @param[out] weights a pointer to the area where Window will be created
 * @param[in] pointSize size of the window to create
 */
template <typename T_C>
void getHammingWindow(T_C* weights, const unsigned int pointSize) {
    float temp;
    for (int i = 0; i < pointSize; i++) {
        temp = 0.54 -
               0.46 * cos(i * 2 * M_PI /
                          (pointSize - 1)); // why POINT_SIZE-1? From hamming desc in Matlab, window length = N+1
        weights[i] = (T_C)((float)(1 << 14) * temp + 0.5);
    }
}
/**
  * @cond NOCOMMENTS
  */

template <>
void getHammingWindow<int32>(int32* weights, const unsigned int pointSize) {
    float temp;
    for (int i = 0; i < pointSize; i++) {
        temp = 0.54 -
               0.46 * cos(i * 2 * M_PI /
                          (pointSize - 1)); // why POINT_SIZE-1? From hamming desc in Matlab, window length = N+1
        weights[i] = ((int32)(1 << 30) * temp + 0.5);
    }
}

template <>
void getHammingWindow<float>(float* weights, const unsigned int pointSize) {
    float temp;
    for (int i = 0; i < pointSize; i++) {
        weights[i] = 0.54 -
                     0.46 * cos(i * 2 * M_PI /
                                (pointSize - 1)); // why POINT_SIZE-1? From hamming desc in Matlab, window length = N+1
    }
}

/**
 * @endcond
 */

/**
 * @ingroup fft_window_utils
 * @brief getHannWindow is utility to create Hann window.
 * @tparam T_C describes the type of weights in the FFT window.
 * @param[out] weights a pointer to the area where Window will be created
 * @param[in] pointSize size of the window to create
 */
template <typename T_C>
void getHannWindow(T_C* weights, const unsigned int pointSize) {
    float temp;
    for (int i = 0; i < pointSize; i++) {
        temp = 0.5 -
               0.5 * cos(i * 2 * M_PI /
                         (pointSize - 1)); // why POINT_SIZE-1? From description in Matlab, window length = N+1
        weights[i] = (T_C)((float)(1 << 14) * temp + 0.5);
    }
}
/**
  * @cond NOCOMMENTS
  */

template <>
void getHannWindow<int32>(int32* weights, const unsigned int pointSize) {
    float temp;
    for (int i = 0; i < pointSize; i++) {
        temp = 0.5 -
               0.5 * cos(i * 2 * M_PI /
                         (pointSize - 1)); // why POINT_SIZE-1? From description in Matlab, window length = N+1
        weights[i] = ((int32)(1 << 30) * temp + 0.5);
    }
}

template <>
void getHannWindow<float>(float* weights, const unsigned int pointSize) {
    float temp;
    for (int i = 0; i < pointSize; i++) {
        weights[i] = 0.5 -
                     0.5 * cos(i * 2 * M_PI /
                               (pointSize - 1)); // why POINT_SIZE-1? From description in Matlab, window length = N+1
    }
}

/**
 * @endcond
 */

/**
 * @ingroup fft_window_utils
 * @brief getBlackmanWindow is utility to create Hamming window.
 * @tparam T_C describes the type of weights in the FFT window.
 * @param[out] weights a pointer to the area where Window will be created
 * @param[in] pointSize size of the window to create
 */
template <typename T_C>
void getBlackmanWindow(T_C* weights, const unsigned int pointSize) {
    float temp;
    for (int i = 0; i < pointSize; i++) {
        temp = 0.42 - 0.5 * cos(i * 2 * M_PI / (pointSize - 1)) +
               0.08 * cos(i * 4 * M_PI /
                          (pointSize - 1)); // why POINT_SIZE-1? From description in Matlab, window length = N+1
        weights[i] = (T_C)((float)(1 << 14) * temp + 0.5);
    }
}
/**
  * @cond NOCOMMENTS
  */

template <>
void getBlackmanWindow<int32>(int32* weights, const unsigned int pointSize) {
    float temp;
    for (int i = 0; i < pointSize; i++) {
        temp = 0.42 - 0.5 * cos(i * 2 * M_PI / (pointSize - 1)) +
               0.08 * cos(i * 4 * M_PI /
                          (pointSize - 1)); // why POINT_SIZE-1? From description in Matlab, window length = N+1
        weights[i] = ((int32)(1 << 30) * temp + 0.5);
    }
}

template <>
void getBlackmanWindow<float>(float* weights, const unsigned int pointSize) {
    float temp;
    for (int i = 0; i < pointSize; i++) {
        weights[i] = 0.42 - 0.5 * cos(i * 2 * M_PI / (pointSize - 1)) +
                     0.08 * cos(i * 4 * M_PI /
                                (pointSize - 1)); // why POINT_SIZE-1? From description in Matlab, window length = N+1
    }
}

// MODIFIED bessel function first kind, order 0.
float fn_io(float x) {
    float retVal = 0.0;
    float term = 0.0;
    float d_sum = 1.0;
    float overall_sum = 1.0;
    int k = 1; // theoretically, this starts from 0, but this is aborbed in the initial sum as 1
    do {
        d_sum = (0.25 * d_sum * x * x) / (k * k);
        k++;
        overall_sum += d_sum;
    } while (d_sum > 0.00000000001); // terminate once d_sum would not change LSB of int32
    return overall_sum;
}

/**
 * @endcond
 */

/**
 * @ingroup fft_window_utils
 * @brief geKeiserWindow is utility to create Hamming window.
 * @tparam T_C describes the type of weights in the FFT window.
 * @param[out] weights a pointer to the area where Window will be created
 * @param[in] pointSize size of the window to create
 * @param[in] alpha  a non-negative real number that determines the shape of the window
 */
template <typename T_C>
void getKaiserWindow(T_C* weights, const unsigned int pointSize, const float alpha = 1.27) {
    float temp;
    float x;
    float arg;
    float divFactor = fn_io(M_PI * alpha);
    for (int i = 0; i < pointSize; i++) {
        x = (float)(i);
        arg = 2.0 * x / (float)(pointSize - 1) - 1.0;
        arg = pow(arg, 2.0);
        arg = M_PI * alpha * sqrt(1.0 - arg);
        temp = fn_io(arg) / divFactor;
        weights[i] = (T_C)((float)(1 << 14) * temp + 0.5);
    }
}
/**
  * @cond NOCOMMENTS
  */

template <>
void getKaiserWindow<int32>(int32* weights, const unsigned int pointSize, const float alpha) {
    float temp;
    float x;
    float arg;
    float divFactor = fn_io(M_PI * alpha);
    for (int i = 0; i < pointSize; i++) {
        x = (float)(i);
        arg = 2.0 * x / (float)(pointSize - 1) - 1.0;
        arg = pow(arg, 2.0);
        arg = M_PI * alpha * sqrt(1.0 - arg);
        temp = fn_io(arg) / divFactor;
        weights[i] = (int32)((float)(1 << 30) * temp + 0.5);
    }
}

template <>
void getKaiserWindow<float>(float* weights, const unsigned int pointSize, const float alpha) {
    float temp;
    float x;
    float arg;
    float divFactor = fn_io(M_PI * alpha);
    for (int i = 0; i < pointSize; i++) {
        x = (float)(i);
        arg = 2.0 * x / (float)(pointSize - 1) - 1.0;
        arg = pow(arg, 2.0);
        arg = M_PI * alpha * sqrt(1.0 - arg);
        weights[i] = fn_io(arg) / divFactor;
    }
}

/**
 * @endcond
 */

/**
 * @ingroup fft_window_utils
 * @brief getRotationVector is utility to populate the window with rotation value for the 2D-like implementation of FFT.
 * @tparam T_C describes the type of weights in the FFT window.
 * @param[out] weights a pointer to the area where Window will be created
 * @param[in] pointSize size of the window to create
 */
template <typename T_C>
void getRotationMatrix(T_C* weights, const unsigned int pointSize) {
    float temp;
    float x;
    float arg;
    int rr, cc;
    int sqrt_pt_size = sqrt(pointSize);
    int scalingfactor = (1 << ((sizeof(T_C) / 2 * 8) - 1)) - 1;
    for (int i = 0; i < pointSize; i++) {
        x = (float)(i);
        rr = i / sqrt_pt_size;
        cc = i % sqrt_pt_size;
        weights[i].real = ((float)(cos((-1 * cc * rr * 2 * M_PI) / pointSize))) * (float)scalingfactor;
        weights[i].imag = ((float)(sin((-1 * cc * rr * 2 * M_PI) / pointSize))) * (float)scalingfactor;
    }
}
}
}
}
}
}

#endif
