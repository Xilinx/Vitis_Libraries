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

/**
 * Comprehensive testbench for fft1d_variant_r2_stages_top (Stockham DIT FFT).
 * Official test: fft_1d_hls_radix2 (aligned with xf_dsp_ryanw test_pingpong).
 *
 * Tests multiple waveform types streamed back-to-back through the DUT,
 * verifying each output against a golden radix-2 DIT FFT reference.
 *
 * argv[1] (optional): check level  0=SNR only (default), 1=point-wise, 2=both
 * argv[2] (optional): verbose      0=summary only (default), 1=per-vector detail
 */
#include "fft_1d_variant_types.hpp"
#include "fft1d_variant_r2_stages.hpp"
#include "vector_factory.hpp"
#include <cstdlib>
#include <cstring>
#include <vector>
#include <cmath>

using xf::dsp::fft::struct_fft_ssr2;
using xf::dsp::fft::token_t;
using xf::dsp::fft::TOKEN_IDLE;
using xf::dsp::fft::TOKEN_FIRST;
using xf::dsp::fft::TOKEN_NORMAL;
using xf::dsp::fft::TOKEN_LAST;
using xf::dsp::fft::Log2;

extern void fft1d_variant_r2_stages_top(hls::stream<struct_fft_ssr2>& strm_in,
                                        hls::stream<struct_fft_ssr2>& strm_out,
                                        hls::stream<ap_uint<2> >& strm_token_in,
                                        hls::stream<ap_uint<2> >& strm_token_out);

static const int N = NUM_FFT;
static const int BEATS = N / 2;
static const int LOG2N = Log2<N>::value;
static const int SHOW = (N <= 256) ? N : 16;

// ============================================================================
// Quiet golden FFT — no per-stage printing, suitable for multi-vector tests.
// ============================================================================
static void goldenFFT_quiet(std::vector<complex_t>& result, const std::vector<complex_t>& input) {
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
    goldenFFT_quiet(tc.golden, tc.input);
    return tc;
}

static TestCase makeDC() {
    TestCase tc;
    strncpy(tc.name, "DC", sizeof(tc.name));
    tc.input.assign(N, complex_t(1.0f, 0.0f));
    goldenFFT_quiet(tc.golden, tc.input);
    return tc;
}

static TestCase makeCounter() {
    TestCase tc;
    strncpy(tc.name, "counter", sizeof(tc.name));
    tc.input.resize(N);
    for (int i = 0; i < N; i++) tc.input[i] = complex_t((float)i, 0.0f);
    goldenFFT_quiet(tc.golden, tc.input);
    return tc;
}

// ============================================================================
// Per-vector result record
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
// main
// ============================================================================
int main(int argc, char* argv[]) {
    int check_level = 0;
    int verbose = 0;
    if (argc >= 2) check_level = std::atoi(argv[1]);
    if (argc >= 3) verbose = std::atoi(argv[2]);
    if (check_level < 0 || check_level > 2) check_level = 0;

    float min_snr = (N <= 1024) ? 80.0f : ((N <= 8192) ? 60.0f : 50.0f);
    float tol = (N <= 1024) ? 1e-5f : ((N <= 8192) ? 1e-4f : 1e-3f);

    // ------------------------------------------------------------------
    // Build test suite
    // ------------------------------------------------------------------
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
    printf("  Stockham DIT FFT Testbench (fft_1d_hls_radix2)\n");
    printf("  N = %d,  log2N = %d,  SSR = 2\n", N, LOG2N);
    printf("  Test vectors: %d (streamed back-to-back)\n", K);
    printf("  Check: %s  |  SNR min %.0f dB  |  tol %.1e\n",
           check_level == 0 ? "SNR only" : check_level == 1 ? "point-wise" : "SNR + point-wise", min_snr, tol);
    printf("========================================================\n\n");

    // ------------------------------------------------------------------
    // Feed all vectors into DUT via ping-pong token protocol
    // ------------------------------------------------------------------
    hls::stream<struct_fft_ssr2> strm_in("in"), strm_out("out");
    hls::stream<ap_uint<2> > strm_token_in("tok_in"), strm_token_out("tok_out");

    for (int v = 0; v < K; v++) {
        strm_token_in.write((v == 0) ? TOKEN_FIRST : TOKEN_NORMAL);
        for (int i = 0; i < BEATS; i++) {
            struct_fft_ssr2 d(tests[v].input[i * 2], tests[v].input[i * 2 + 1]);
            strm_in.write(d);
        }
    }
    strm_token_in.write(TOKEN_LAST);

    printf("[DUT] Running %d vectors through %d-stage pipeline ...\n\n", K, LOG2N + 1);
    fft1d_variant_r2_stages_top(strm_in, strm_out, strm_token_in, strm_token_out);

    // ------------------------------------------------------------------
    // Collect outputs and compare
    // ------------------------------------------------------------------
    std::vector<Result> results;
    bool all_pass = true;
    int num_out = 0;

    while (!strm_token_out.empty()) {
        ap_uint<2> token = strm_token_out.read();
        bool has_data = (token == TOKEN_NORMAL || token == TOKEN_LAST);

        if (has_data && num_out < K) {
            std::vector<complex_t> got(N);
            for (int i = 0; i < BEATS; i++) {
                struct_fft_ssr2 d = strm_out.read();
                got[i * 2] = d.data[0];
                got[i * 2 + 1] = d.data[1];
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

    // ------------------------------------------------------------------
    // Summary table
    // ------------------------------------------------------------------
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
