/*
 * Copyright (C) 2026, Advanced Micro Devices, Inc.
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

/**
 * Testbench for fft1d_variant_r4_stages_top (radix-4 DIT FFT).
 * Same token pattern as fft_1d_variant_r2; SSR=4 and BEATS=N/4.
 * Primary golden: radix-4 DIT reference (matches DUT algorithm). Optional cross-check vs radix-2 DIT.
 *
 * argv[1] (optional): check level  0=SNR only (default), 1=point-wise, 2=both
 * argv[2] (optional): verbose      0=summary only (default), 1=per-vector detail
 */
#include "fft_1d_variant_types.hpp"
#include "fft1d_variant_r4_stages.hpp"
#include "vector_factory.hpp"
#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <cmath>

using xf::dsp::fft::Log2;
using xf::dsp::fft::struct_fft_ssr4;
using xf::dsp::fft::TOKEN_FIRST;
using xf::dsp::fft::TOKEN_NORMAL;
using xf::dsp::fft::TOKEN_LAST;

extern void fft1d_variant_r4_stages_top(hls::stream<struct_fft_ssr4>& strm_in,
                                        hls::stream<struct_fft_ssr4>& strm_out,
                                        hls::stream<ap_uint<2> >& strm_token_in,
                                        hls::stream<ap_uint<2> >& strm_token_out);

static const int N = NUM_FFT;
static const int BEATS = N / 4;
static const int LOG2N = Log2<N>::value;
static const int SHOW = (N <= 256) ? N : 16;

#ifndef __SYNTHESIS__
static_assert((N & (N - 1)) == 0 && N >= 4, "NUM_FFT must be power of 2");
static_assert(Log2<N>::value % 2 == 0, "NUM_FFT must be a power of 4 for radix-4 variant");
#endif

// ============================================================================
// Radix-2 DIT reference (cross-check only; mathematically same FFT as radix-4 path)
// ============================================================================
static void goldenFFT_radix2_quiet(std::vector<complex_t>& result, const std::vector<complex_t>& input) {
    int n = (int)input.size();
    int log2n = 0;
    for (int t = n; t > 1; t >>= 1) log2n++;

    std::vector<complex_t> work(n);
    for (int i = 0; i < n; i++) work[VectorFactory::bitReverse(i, log2n)] = input[i];

    const float pi = (float)M_PI;
    for (int s = 0; s < log2n; s++) {
        int half = 1 << s;
        int span = half << 1;
        for (int g = 0; g < n; g += span)
            for (int k = 0; k < half; k++) {
                float angle = -2.0f * pi * (float)k / (float)span;
                complex_t tw(std::cos(angle), std::sin(angle));
                int i0 = g + k, i1 = i0 + half;
                complex_t t = work[i1] * tw;
                work[i1] = work[i0] - t;
                work[i0] = work[i0] + t;
            }
    }
    result = work;
}

static inline complex_t Wn_ref(int nfft, int k) {
    k %= nfft;
    if (k < 0) k += nfft;
    float ang = -2.0f * (float)M_PI * (float)k / (float)nfft;
    return complex_t(std::cos(ang), std::sin(ang));
}

/** Same 4-point DFT + twiddle pattern as fft1d_variant_r4_utils.hpp::calculate_radix4_dit */
static inline void radix4_butterfly_ref(const complex_t& x0,
                                        const complex_t& x1,
                                        const complex_t& x2,
                                        const complex_t& x3,
                                        const complex_t& w1,
                                        const complex_t& w2,
                                        const complex_t& w3,
                                        complex_t& o0,
                                        complex_t& o1,
                                        complex_t& o2,
                                        complex_t& o3) {
    const complex_t y0 = x0;
    const complex_t y1 = w1 * x1;
    const complex_t y2 = w2 * x2;
    const complex_t y3 = w3 * x3;
    const complex_t j(0.0f, 1.0f);
    o0 = y0 + y1 + y2 + y3;
    o1 = y0 - j * y1 - y2 + j * y3;
    o2 = y0 - y1 + y2 - y3;
    o3 = y0 + j * y1 - y2 - j * y3;
}

// ============================================================================
// Primary golden: Cooley–Tukey radix-4 DIT (same stage indexing as DUT)
// ============================================================================
static void goldenFFT_radix4_quiet(std::vector<complex_t>& result, const std::vector<complex_t>& input) {
    int n = (int)input.size();
    int log2n = 0;
    for (int t = n; t > 1; t >>= 1) log2n++;
    const int L4 = log2n / 2;

    std::vector<complex_t> buf0(n), buf1(n);
    for (int i = 0; i < n; i++) buf0[VectorFactory::bitReverse(i, log2n)] = input[i];

    complex_t* src = buf0.data();
    complex_t* dst = buf1.data();

    for (int s = 0; s < L4; s++) {
        const int m = 1 << (2 * (s + 1));
        const int m4 = m >> 2;
        const int num_beats = n / 4;
        const int step = n / m;
        for (int tt = 0; tt < num_beats; tt++) {
            const int r = tt / m4;
            const int k = tt % m4;
            const int idx0 = r * m + k;
            const int idx1 = idx0 + (m >> 2);
            const int idx2 = idx0 + (m >> 1);
            const int idx3 = idx0 + 3 * (m >> 2);

            complex_t x0 = src[idx0];
            complex_t x1 = src[idx1];
            complex_t x2 = src[idx2];
            complex_t x3 = src[idx3];
            complex_t w1 = Wn_ref(n, k * step);
            complex_t w2 = Wn_ref(n, 2 * k * step);
            complex_t w3 = Wn_ref(n, 3 * k * step);
            complex_t o0, o1, o2, o3;
            radix4_butterfly_ref(x0, x1, x2, x3, w1, w2, w3, o0, o1, o2, o3);
            dst[idx0] = o0;
            dst[idx1] = o1;
            dst[idx2] = o2;
            dst[idx3] = o3;
        }
        std::swap(src, dst);
    }

    result.resize(n);
    for (int i = 0; i < n; i++) result[i] = src[i];
}

// ============================================================================
// Test case descriptor
// ============================================================================
struct TestCase {
    char name[32];
    std::vector<complex_t> input;
    std::vector<complex_t> golden;
};

static TestCase makeTest(const char* name,
                         const char* re_type,
                         int re_freq,
                         float re_scale,
                         const char* im_type,
                         int im_freq,
                         float im_scale) {
    TestCase tc;
    strncpy(tc.name, name, sizeof(tc.name) - 1);
    tc.name[sizeof(tc.name) - 1] = '\0';
    std::vector<float> re, im;
    VectorFactory::gen(re, N, re_type, re_freq, re_scale);
    VectorFactory::gen(im, N, im_type, im_freq, im_scale);
    VectorFactory::genComplex(tc.input, re, im);
    goldenFFT_radix4_quiet(tc.golden, tc.input);
    return tc;
}

static TestCase makeDC() {
    TestCase tc;
    strncpy(tc.name, "DC", sizeof(tc.name));
    tc.input.assign(N, complex_t(1.0f, 0.0f));
    goldenFFT_radix4_quiet(tc.golden, tc.input);
    return tc;
}

static TestCase makeCounter() {
    TestCase tc;
    strncpy(tc.name, "counter", sizeof(tc.name));
    tc.input.resize(N);
    for (int i = 0; i < N; i++) tc.input[i] = complex_t((float)i, 0.0f);
    goldenFFT_radix4_quiet(tc.golden, tc.input);
    return tc;
}

// ============================================================================
struct Result {
    char name[32];
    float snr_dB;
    float max_err;
    int max_err_idx;
    float mean_err;
    bool pass;
};

static void computeErrors(const std::vector<complex_t>& got,
                          const std::vector<complex_t>& golden,
                          float& snr,
                          float& max_err,
                          int& max_idx,
                          float& mean_err) {
    int n = (int)golden.size();
    double sig_pow = 0.0, err_pow = 0.0, sum_abs = 0.0;
    max_err = 0.0f;
    max_idx = 0;
    for (int i = 0; i < n; i++) {
        float re = golden[i].real(), im = golden[i].imag();
        sig_pow += (double)(re * re + im * im);
        complex_t e = got[i] - golden[i];
        float ae = std::abs(e);
        err_pow += (double)(e.real() * e.real() + e.imag() * e.imag());
        sum_abs += (double)ae;
        if (ae > max_err) {
            max_err = ae;
            max_idx = i;
        }
    }
    mean_err = (float)(sum_abs / n);
    if (err_pow <= 0.0)
        snr = 200.0f;
    else if (sig_pow <= 0.0)
        snr = 0.0f;
    else
        snr = (float)(10.0 * std::log10(sig_pow / err_pow));
}

// ============================================================================
int main(int argc, char* argv[]) {
    int check_level = 0;
    int verbose = 0;
    if (argc >= 2) check_level = std::atoi(argv[1]);
    if (argc >= 3) verbose = std::atoi(argv[2]);
    if (check_level < 0 || check_level > 2) check_level = 0;

    float min_snr = (N <= 1024) ? 80.0f : ((N <= 8192) ? 60.0f : 50.0f);
    float tol = (N <= 1024) ? 1e-5f : ((N <= 8192) ? 1e-4f : 1e-3f);

    std::vector<TestCase> tests;
    tests.push_back(makeTest("impulse", "pulse", 1, 1.0f, "linear", 0, 0.0f));
    tests.push_back(makeDC());
    tests.push_back(makeTest("sine_k1", "sine", 1, 1.0f, "linear", 0, 0.0f));
    tests.push_back(makeTest("cosine_k4", "cosine", 4, 1.0f, "linear", 0, 0.0f));
    tests.push_back(makeTest("square_k8", "square", 8, 1.0f, "linear", 0, 0.0f));
    tests.push_back(makeTest("triangular", "triangular", 3, 1.0f, "linear", 0, 0.0f));
    tests.push_back(makeCounter());
    tests.push_back(makeTest("random", "random", 0, 1.0f, "noise", 0, 0.5f));

    int K = (int)tests.size();

    printf("========================================================\n");
    printf("  Radix-4 DIT FFT Testbench (fft1d_variant_r4)\n");
    printf("  N = %d,  log2N = %d,  SSR = 4,  BEATS = %d\n", N, LOG2N, BEATS);
    printf("  Test vectors: %d (streamed back-to-back)\n", K);
    printf("  Check: %s  |  SNR min %.0f dB  |  tol %.1e\n",
           check_level == 0 ? "SNR only" : check_level == 1 ? "point-wise" : "SNR + point-wise", min_snr, tol);
    printf("========================================================\n\n");

    // Cross-check: radix-4 golden vs radix-2 golden (should match within float tolerance)
    {
        std::vector<complex_t> probe(N);
        for (int i = 0; i < N; i++) probe[i] = complex_t((float)(i % 17), (float)((i * 3) % 11));
        std::vector<complex_t> gr2, gr4;
        goldenFFT_radix2_quiet(gr2, probe);
        goldenFFT_radix4_quiet(gr4, probe);
        float mx = 0.0f;
        for (int i = 0; i < N; i++) mx = std::max(mx, std::abs(gr2[i] - gr4[i]));
        printf("[ref] max |FFT_r2 - FFT_r4| on probe vector = %.3e (expect ~1e-6..1e-5)\n", mx);
        if (mx > 5e-4f) printf("[ref] WARNING: radix-2 vs radix-4 golden mismatch is large — check references.\n");
        printf("\n");
    }

    hls::stream<struct_fft_ssr4> strm_in("in"), strm_out("out");
    hls::stream<ap_uint<2> > strm_token_in("tok_in"), strm_token_out("tok_out");

    for (int v = 0; v < K; v++) {
        strm_token_in.write((v == 0) ? TOKEN_FIRST : TOKEN_NORMAL);
        for (int i = 0; i < BEATS; i++) {
            struct_fft_ssr4 d(tests[v].input[i * 4 + 0], tests[v].input[i * 4 + 1], tests[v].input[i * 4 + 2],
                              tests[v].input[i * 4 + 3]);
            strm_in.write(d);
        }
    }
    strm_token_in.write(TOKEN_LAST);

    printf("[DUT] Running %d vectors ...\n\n", K);
    fft1d_variant_r4_stages_top(strm_in, strm_out, strm_token_in, strm_token_out);

    std::vector<Result> results;
    bool all_pass = true;
    int num_out = 0;

    while (!strm_token_out.empty()) {
        ap_uint<2> token = strm_token_out.read();
        bool has_data = (token == TOKEN_NORMAL || token == TOKEN_LAST);

        if (has_data && num_out < K) {
            std::vector<complex_t> got(N);
            for (int i = 0; i < BEATS; i++) {
                struct_fft_ssr4 d = strm_out.read();
                got[i * 4 + 0] = d.data[0];
                got[i * 4 + 1] = d.data[1];
                got[i * 4 + 2] = d.data[2];
                got[i * 4 + 3] = d.data[3];
            }

            const TestCase& tc = tests[num_out];

            Result r;
            strncpy(r.name, tc.name, sizeof(r.name));
            computeErrors(got, tc.golden, r.snr_dB, r.max_err, r.max_err_idx, r.mean_err);

            if (check_level == 0)
                r.pass = (r.snr_dB >= min_snr);
            else if (check_level == 1)
                r.pass = (r.max_err <= tol);
            else
                r.pass = (r.snr_dB >= min_snr) && (r.max_err <= tol);

            if (!r.pass) all_pass = false;
            results.push_back(r);

            if (verbose) {
                printf("--- [%d/%d] %s ---\n", num_out + 1, K, tc.name);
                VectorFactory::printDual(tc.input, "  in:  ", SHOW);
                VectorFactory::printDual(tc.golden, "  ref: ", SHOW);
                VectorFactory::printDual(got, "  dut: ", SHOW);
                printf("  SNR = %.2f dB,  max_err = %.3e (idx %d),  mean_err = %.3e\n", r.snr_dB, r.max_err,
                       r.max_err_idx, r.mean_err);
                if (r.max_err > 0.0f)
                    printf("  worst: got (%g,%g)  ref (%g,%g)\n", got[r.max_err_idx].real(), got[r.max_err_idx].imag(),
                           tc.golden[r.max_err_idx].real(), tc.golden[r.max_err_idx].imag());
                printf("  => %s\n\n", r.pass ? "PASS" : "** FAIL **");
            }

            num_out++;
        }
        if (token == TOKEN_LAST) break;
    }

    int num_pass = 0;
    for (int i = 0; i < (int)results.size(); i++)
        if (results[i].pass) num_pass++;

    printf("========================================================\n");
    printf("  Summary:  %d / %d passed    (N = %d)\n", num_pass, K, N);
    printf("========================================================\n");
    printf("  %-16s %10s %12s %12s %6s\n", "Test", "SNR(dB)", "Max Err", "Mean Err", "Result");
    printf("  %-16s %10s %12s %12s %6s\n", "----", "------", "-------", "--------", "------");
    for (int i = 0; i < (int)results.size(); i++) {
        const Result& r = results[i];
        printf("  %-16s %10.2f %12.3e %12.3e %6s\n", r.name, r.snr_dB, r.max_err, r.mean_err, r.pass ? "PASS" : "FAIL");
    }
    printf("========================================================\n");
    printf("Overall: %s\n", all_pass ? "PASS" : "FAIL");

    return all_pass ? 0 : 1;
}
