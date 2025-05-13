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
//================================== End Lic =================================================
#ifndef DSP_UTILITIES_H_
#define DSP_UTILITIES_H_
#include <math.h>
#include <complex>

template <int DIM1, int DIM2>
double snr(double signal[DIM1][DIM2], double noisy_signal[DIM1][DIM2]) {
    // SNR = 10 log10 (  mean(singal^2)/ mean(noise^2)   )
    double noise_mean = 0;
    double signal_mean = 0;
    for (int i = 0; i < DIM1; ++i) {
        for (int j = 0; j < DIM2; ++j) {
            double temp = signal[i][j] - noisy_signal[i][j];
            noise_mean += temp * temp;
            signal_mean += signal[i][j] * signal[i][j];
        }
    }
    double snr = signal_mean / noise_mean;
    snr = 10 * log10(snr);

    return snr;
}

template <int DIM1, int DIM2>
double snr(float signal[DIM1][DIM2], float noisy_signal[DIM1][DIM2]) {
    // SNR = 10 log10 (  mean(singal^2)/ mean(noise^2)   )
    float noise_mean = 0;
    float signal_mean = 0;
    for (int i = 0; i < DIM1; ++i) {
        for (int j = 0; j < DIM2; ++j) {
            float temp = signal[i][j] - noisy_signal[i][j];
            noise_mean += temp * temp;
            signal_mean += signal[i][j] * signal[i][j];
        }
    }
    double snr = signal_mean / noise_mean;
    snr = 10 * log10(snr);

    return snr;
}
template <int DIM1, int DIM2>
double snr(complex_wrapper<double> signal[DIM1][DIM2], complex_wrapper<double> noisy_signal[DIM1][DIM2]) {
    // SNR = 10 log10 (  mean(singal^2)/ mean(noise^2)   )
    double noise_mean = 0;
    double signal_mean = 0;
    for (int i = 0; i < DIM1; ++i) {
        for (int j = 0; j < DIM2; ++j) {
            complex_wrapper<double> temp = signal[i][j] - noisy_signal[i][j];
            noise_mean += (temp.real() * temp.real() + temp.imag() * temp.imag());
            temp = signal[i][j];
            signal_mean += (temp.real() * temp.real() + temp.imag() * temp.imag());
        }
    }
    double snr = signal_mean / noise_mean;
    snr = 10 * log10(snr);

    return snr;
}

template <int DIM1, int DIM2>
double snr(complex_wrapper<float> signal[DIM1][DIM2], complex_wrapper<float> noisy_signal[DIM1][DIM2]) {
    double noise_mean = 0;
    double signal_mean = 0;
    for (int i = 0; i < DIM1; ++i) {
        for (int j = 0; j < DIM2; ++j) {
            complex_wrapper<float> temp = signal[i][j] - noisy_signal[i][j];
            noise_mean += (temp.real() * temp.real() + temp.imag() * temp.imag());
            temp = signal[i][j];
            signal_mean += (temp.real() * temp.real() + temp.imag() * temp.imag());
        }
    }
    double snr = signal_mean / noise_mean;
    snr = 10 * log10(snr);

    return snr;
}
template <int DIM1, int DIM2, typename T>
void cast_to_double(complex_wrapper<T> inData[DIM1][DIM2], complex_wrapper<double> outData[DIM1][DIM2]) {
    for (int i = 0; i < DIM1; ++i) {
        for (int j = 0; j < DIM2; ++j) {
            double temp_r = inData[i][j].real();
            double temp_i = inData[i][j].imag();
            outData[i][j].real(temp_r);
            outData[i][j].imag(temp_i);
        }
    }
}

template <int DIM1, int DIM2, typename T>
void cast_to_double(complex_wrapper<T> inData[DIM1][DIM2], complex_wrapper<float> outData[DIM1][DIM2]) {
    for (int i = 0; i < DIM1; ++i) {
        for (int j = 0; j < DIM2; ++j) {
            float temp_r = inData[i][j].real();
            float temp_i = inData[i][j].imag();
            outData[i][j].real(temp_r);
            outData[i][j].imag(temp_i);
        }
    }
}

template <int R, int L, typename DT>
void convert2Array(hls::stream<DT> strm[R], DT arr[R][L / R]) {
    for (int i = 0; i < (L / R); i++) {
        for (int j = 0; j < R; j++) {
            arr[j][i] = strm[j].read();
        }
    }
}

#endif // DSP_UTILITIES_H_
