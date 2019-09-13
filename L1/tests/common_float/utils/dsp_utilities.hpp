#ifndef DSP_UTILITIES_H_
#define DSP_UTILITIES_H_
#include <math.h>
#include <complex>

template <int DIM1, int DIM2>
double snr(double signal[DIM1][DIM2], double noisy_signal[DIM1][DIM2]) {
    // SNR = 10 log10 (  mean(singal^2)/ mean(noise^2)   )
    // double *noise = new double[dim1*dim2];
    double noise_mean = 0;
    double signal_mean = 0;
    for (int i = 0; i < DIM1; ++i) {
        for (int j = 0; j < DIM2; ++j) {
            double temp = signal[i][j] - noisy_signal[i][j];
            noise_mean += temp * temp;
            signal_mean += signal[i][j] * signal[i][j];
        }
    }
    noise_mean = noise_mean / (DIM1);
    signal_mean = signal_mean / (DIM1);
    double snr = signal_mean / noise_mean;
    snr = 10 * log10(snr);

    return snr;
}
template <int DIM1, int DIM2>
double snr(complex_wrapper<double> signal[DIM1][DIM2], complex_wrapper<double> noisy_signal[DIM1][DIM2]) {
    // SNR = 10 log10 (  mean(singal^2)/ mean(noise^2)   )
    // double *noise = new double[dim1*dim2];
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
    noise_mean = noise_mean / (DIM1);
    signal_mean = signal_mean / (DIM1);
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

#endif // DSP_UTILITIES_H_
