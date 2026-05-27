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

#ifndef VECTOR_FACTORY_HPP
#define VECTOR_FACTORY_HPP

#include <iostream>
#include <vector>
#include <complex>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <cstdio>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

typedef std::complex<float> complex_t;

class VectorFactory {
   public:
    static const int NUM_WAVE_TYPES = 8;
    static constexpr const char* wave_names[NUM_WAVE_TYPES] = {"linear", "sine",  "cosine", "triangular",
                                                               "square", "pulse", "noise",  "random"};

    // --- Waveform generation ---

    static void gen(std::vector<float>& out, int n, const char* type, int freq, float scale) {
        out.clear();
        out.reserve(n);
        int k = (freq > 0 && freq < n) ? freq : 1;
        float period = static_cast<float>(n) / static_cast<float>(k);
        const float twopi = 2.0f * static_cast<float>(M_PI);

        switch (waveIndex(type)) {
            case 0:
                for (int i = 0; i < n; i++) out.push_back(scale * static_cast<float>(i) / static_cast<float>(n));
                break;
            case 1:
                for (int i = 0; i < n; i++)
                    out.push_back(scale * std::sin(twopi * static_cast<float>(k * i) / static_cast<float>(n)));
                break;
            case 2:
                for (int i = 0; i < n; i++)
                    out.push_back(scale * std::cos(twopi * static_cast<float>(k * i) / static_cast<float>(n)));
                break;
            case 3:
                for (int i = 0; i < n; i++) {
                    float ph = wrap01(static_cast<float>(i) / period);
                    out.push_back(scale * ((ph < 0.5f) ? (4.0f * ph - 1.0f) : (3.0f - 4.0f * ph)));
                }
                break;
            case 4:
                for (int i = 0; i < n; i++) {
                    float ph = wrap01(static_cast<float>(i) / period);
                    out.push_back(scale * ((ph < 0.5f) ? 1.0f : -1.0f));
                }
                break;
            case 5:
                for (int i = 0; i < n; i++) {
                    int s = static_cast<int>(std::fmod(static_cast<float>(i), period));
                    out.push_back(scale * ((s == 0) ? 1.0f : 0.0f));
                }
                break;
            case 6:
                std::srand(42);
                for (int i = 0; i < n; i++) out.push_back(scale * (2.0f * randUnit() - 1.0f));
                break;
            case 7:
                std::srand(12345);
                for (int i = 0; i < n; i++) out.push_back(scale * (2.0f * randUnit() - 1.0f));
                break;
            default:
                for (int i = 0; i < n; i++) out.push_back(0.0f);
                break;
        }
    }

    static void genComplex(std::vector<complex_t>& out, const std::vector<float>& re, const std::vector<float>& im) {
        out.clear();
        out.reserve(re.size());
        for (size_t i = 0; i < re.size(); i++) out.push_back(complex_t(re[i], im[i]));
    }

    static void genCounter(std::vector<complex_t>& out, int n) {
        out.clear();
        out.reserve(n);
        for (int i = 0; i < n; i++) out.push_back(complex_t(static_cast<float>(i), 0.0f));
    }

    // --- Golden FFT (radix-2 DIT, prints each stage) ---
    // DIT: bit-reverse input first, then butterflies with increasing stride.
    // Output is in natural order (no bit-reversal needed at the end).

    static int bitReverse(int x, int log2n) {
        int r = 0;
        for (int b = 0; b < log2n; b++) {
            r = (r << 1) | (x & 1);
            x >>= 1;
        }
        return r;
    }

    static void goldenFFT(std::vector<complex_t>& result, const std::vector<complex_t>& input) {
        int n = static_cast<int>(input.size());
        if (n <= 0) return;
        int log2n = 0;
        for (int t = n; t > 1; t >>= 1) log2n++;
        if ((1 << log2n) != n) {
            std::cout << "goldenFFT: size must be power of 2, got " << n << "\n";
            return;
        }

        std::cout << "--- 1D FFT input ---\n";
        printDual(input, "input: ", n);

        std::vector<complex_t> work(n);
        for (int i = 0; i < n; i++) work[bitReverse(i, log2n)] = input[i];

        const float pi = static_cast<float>(M_PI);
        for (int stage = 0; stage < log2n; stage++) {
            int half = 1 << stage;
            int span = half << 1;
            for (int g = 0; g < n; g += span) {
                for (int k = 0; k < half; k++) {
                    float angle = -2.0f * pi * static_cast<float>(k) / static_cast<float>(span);
                    complex_t tw(std::cos(angle), std::sin(angle));
                    int i0 = g + k;
                    int i1 = i0 + half;
                    complex_t t = work[i1] * tw;
                    work[i1] = work[i0] - t;
                    work[i0] = work[i0] + t;
                }
            }
            std::cout << "stage " << stage << ": ";
            printDual(work, "", n);
        }

        result = work;

        std::cout << "--- 1D FFT result ---\n";
        printDual(result, "result: ", n);
    }

    // --- Golden FFT with per-stage results ---
    // DIT: bit-reverse input, then butterflies with increasing stride.
    // stages[s] = state of the array after stage s.
    // result = stages[log2n-1] = final FFT in natural order.

    static void goldenFFT_with_stage(std::vector<std::vector<complex_t> >& stages,
                                     std::vector<complex_t>& result,
                                     const std::vector<complex_t>& input) {
        int n = static_cast<int>(input.size());
        stages.clear();
        result.clear();
        if (n <= 0) return;
        int log2n = 0;
        for (int t = n; t > 1; t >>= 1) log2n++;
        if ((1 << log2n) != n) {
            std::cout << "goldenFFT_with_stage: size must be power of 2, got " << n << "\n";
            return;
        }

        std::cout << "--- 1D FFT input ---\n";
        printDual(input, "", n);

        std::vector<complex_t> work(n);
        for (int i = 0; i < n; i++) work[bitReverse(i, log2n)] = input[i];

        const float pi = static_cast<float>(M_PI);
        stages.resize(log2n);
        for (int stage = 0; stage < log2n; stage++) {
            int half = 1 << stage;
            int span = half << 1;
            for (int g = 0; g < n; g += span) {
                for (int k = 0; k < half; k++) {
                    float angle = -2.0f * pi * static_cast<float>(k) / static_cast<float>(span);
                    complex_t tw(std::cos(angle), std::sin(angle));
                    int i0 = g + k;
                    int i1 = i0 + half;
                    complex_t t = work[i1] * tw;
                    work[i1] = work[i0] - t;
                    work[i0] = work[i0] + t;
                }
            }
            stages[stage] = work;
            std::cout << "stage " << stage << ":\n";
            printDual(work, "", n);
        }

        result = work;

        std::cout << "--- 1D FFT result ---\n";
        printDual(result, "result: ", n);
    }

    // --- Comparison ---

    static bool compare(const std::vector<complex_t>& got, const std::vector<complex_t>& expected, float tol = 1e-5f) {
        bool pass = true;
        int n = static_cast<int>(expected.size());
        for (int i = 0; i < n; i++) {
            if (std::abs(got[i] - expected[i]) > tol) {
                pass = false;
                std::cout << "  [" << i << "] got " << got[i] << " expected " << expected[i] << "\n";
            }
        }
        return pass;
    }

    // --- SNR (Signal-to-Noise Ratio) in dB ---
    // signal_power = sum(|expected[i]|^2), error_power = sum(|got[i]-expected[i]|^2)
    // SNR_dB = 10*log10(signal_power / error_power). Returns a large value if error_power == 0.
    static float computeSNR(const std::vector<complex_t>& got, const std::vector<complex_t>& expected) {
        int n = static_cast<int>(expected.size());
        if (n == 0) return 0.0f;
        double sig_power = 0.0, err_power = 0.0;
        for (int i = 0; i < n; i++) {
            float re = expected[i].real(), im = expected[i].imag();
            sig_power += static_cast<double>(re * re + im * im);
            complex_t e = got[i] - expected[i];
            err_power += static_cast<double>(e.real() * e.real() + e.imag() * e.imag());
        }
        if (err_power <= 0.0) return 200.0f; // no error
        if (sig_power <= 0.0) return 0.0f;
        return static_cast<float>(10.0 * std::log10(sig_power / err_power));
    }

    // Pass if SNR >= min_snr_dB. Prints SNR to stdout.
    static bool compareSNR(const std::vector<complex_t>& got,
                           const std::vector<complex_t>& expected,
                           float min_snr_dB) {
        float snr = computeSNR(got, expected);
        std::cout << "  SNR = " << snr << " dB (min " << min_snr_dB << " dB)\n";
        return snr >= min_snr_dB;
    }

    // --- Error statistics: distribution, max absolute error and its index ---
    static void reportErrorStats(const std::vector<complex_t>& got, const std::vector<complex_t>& expected) {
        int n = static_cast<int>(expected.size());
        if (n == 0) return;

        float max_abs_err = 0.0f;
        int max_idx = 0;
        double sum_abs_err = 0.0;
        // Buckets: [0,1e-6), [1e-6,1e-5), [1e-5,1e-4), [1e-4,1e-3), [1e-3,1e-2), [1e-2,inf)
        int hist[6] = {0, 0, 0, 0, 0, 0};
        const float bounds[] = {1e-6f, 1e-5f, 1e-4f, 1e-3f, 1e-2f};

        for (int i = 0; i < n; i++) {
            float e = std::abs(got[i] - expected[i]);
            sum_abs_err += static_cast<double>(e);
            if (e > max_abs_err) {
                max_abs_err = e;
                max_idx = i;
            }
            int b = 0;
            while (b < 5 && e >= bounds[b]) b++;
            hist[b]++;
        }

        std::cout << "  --- Error statistics ---\n";
        std::cout << "  Max absolute error = " << max_abs_err << " at index " << max_idx << "  got " << got[max_idx]
                  << "  expected " << expected[max_idx] << "\n";
        std::cout << "  Mean absolute error = " << (sum_abs_err / n) << "\n";
        std::cout << "  Distribution (count by magnitude):\n";
        std::cout << "    [0, 1e-6)     : " << hist[0] << "\n";
        std::cout << "    [1e-6, 1e-5)  : " << hist[1] << "\n";
        std::cout << "    [1e-5, 1e-4)  : " << hist[2] << "\n";
        std::cout << "    [1e-4, 1e-3)  : " << hist[3] << "\n";
        std::cout << "    [1e-3, 1e-2)  : " << hist[4] << "\n";
        std::cout << "    [1e-2, +inf)  : " << hist[5] << "\n";
    }

    // --- Print ---

    static void print(const std::vector<float>& v) {
        for (size_t i = 0; i < v.size(); i++) printf("%4.2f, ", v[i]);
        printf("\n");
    }

    static void print(const std::vector<complex_t>& v, const char* prefix = "", int max_show = 8) {
        std::cout << prefix;
        int n = static_cast<int>(v.size());
        for (int i = 0; i < n && i < max_show; i++) std::cout << "(" << v[i].real() << "," << v[i].imag() << ") ";
        if (n > max_show) std::cout << "... [" << n << " pts]";
        std::cout << "\n";
    }

    static void printDual(const std::vector<complex_t>& v, const char* prefix = "", int max_show = 8) {
        std::cout << prefix;
        int n = static_cast<int>(v.size());
        printf("real, \t");
        for (int i = 0; i < n && i < max_show; i++) printf("%4.2f, ", v[i].real());
        if (n > max_show) std::cout << "... [" << n << " pts]";
        std::cout << "\n";
        std::cout << prefix;
        printf("imag, \t");
        for (int i = 0; i < n && i < max_show; i++) printf("%4.2f, ", v[i].imag());
        if (n > max_show) std::cout << "... [" << n << " pts]";
        std::cout << "\n";
    }

    // --- Self-test: sweep all waveform types and frequencies ---

    static void testAllWaveforms(int n) {
        for (int i = 0; i < NUM_WAVE_TYPES; i++) {
            for (int freq = 0; freq < n; freq++) {
                std::vector<float> v;
                gen(v, n, wave_names[i], freq, 1.0f);
                printf("type: %s freq: %d result: \t", wave_names[i], freq);
                print(v);
            }
        }
    }

   private:
    static int waveIndex(const char* name) {
        for (int i = 0; i < NUM_WAVE_TYPES; i++)
            if (strcmp(name, wave_names[i]) == 0) return i;
        std::cout << "wave type not found: " << name << std::endl;
        return -1;
    }
    static float wrap01(float x) {
        float p = std::fmod(x, 1.0f);
        return (p < 0.0f) ? p + 1.0f : p;
    }
    static float randUnit() { return static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX); }
};
constexpr const char* VectorFactory::wave_names[];

#endif
