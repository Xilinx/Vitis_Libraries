/*
Copyright (C) 2025-2025, Advanced Micro Devices, Inc.
SPDX-License-Identifier: X11
*/

/**
 * @file test_park_direct_fp32.cpp
 * @brief HLS Testbench for Park Direct Transform FP32
 */

#include <iostream>
#include <iomanip>
#include <cmath>
#include <random>

// DUT: HLS FP32 synthesizable version
#include "../src/park_direct_fp32.hpp"

// Golden: Phase 1 reference model
#include "../../../../include/models_fp/park_direct.hpp"

using namespace xf::motorcontrol;

constexpr int NUM_RANDOM_TESTS = 1000;
constexpr int NUM_ANGLE_TESTS = 360;
constexpr float PRECISION_THRESHOLD = 1e-6f;

struct ErrorMetrics {
    float max_error_d, max_error_q;
    int num_tests, num_passed;

    ErrorMetrics() : max_error_d(0), max_error_q(0), num_tests(0), num_passed(0) {}

    void update(float err_d, float err_q) {
        max_error_d = std::max(max_error_d, std::abs(err_d));
        max_error_q = std::max(max_error_q, std::abs(err_q));
        num_tests++;
        if (std::abs(err_d) < PRECISION_THRESHOLD && std::abs(err_q) < PRECISION_THRESHOLD) {
            num_passed++;
        }
    }

    void print_summary() {
        std::cout << "\n========================================\n";
        std::cout << "Test Summary\n";
        std::cout << "========================================\n";
        std::cout << "Total tests:     " << num_tests << "\n";
        std::cout << "Passed:          " << num_passed << "\n";
        std::cout << "Failed:          " << (num_tests - num_passed) << "\n";
        std::cout << "Pass rate:       " << std::fixed << std::setprecision(2) << (100.0f * num_passed / num_tests)
                  << "%\n";
        std::cout << "\nMax Errors:\n";
        std::cout << "  Id: " << std::scientific << max_error_d << "\n";
        std::cout << "  Iq: " << std::scientific << max_error_q << "\n";
        std::cout << "\nPrecision threshold: " << PRECISION_THRESHOLD << "\n";
        std::cout << "========================================\n";
    }

    bool all_passed() const { return num_passed == num_tests; }
};

void test_random_inputs(ErrorMetrics& metrics) {
    std::cout << "\n[TEST 1] Random Inputs (" << NUM_RANDOM_TESTS << " tests)\n";
    std::cout << "----------------------------------------\n";

    std::mt19937 rng(42);
    std::uniform_real_distribution<float> dist_curr(-100.0f, 100.0f);
    std::uniform_real_distribution<float> dist_angle(0.0f, 2.0f * 3.14159265359f);

    for (int i = 0; i < NUM_RANDOM_TESTS; i++) {
        float alpha = dist_curr(rng);
        float beta = dist_curr(rng);
        float theta = dist_angle(rng);
        float cos_theta = std::cos(theta);
        float sin_theta = std::sin(theta);

        // DUT
        float dut_d, dut_q;
        hls::Park_Direct_fp32(dut_d, dut_q, alpha, beta, cos_theta, sin_theta);

        // Golden
        float golden_d, golden_q;
        golden::park_direct_golden(golden_d, golden_q, alpha, beta, cos_theta, sin_theta);

        float err_d = dut_d - golden_d;
        float err_q = dut_q - golden_q;
        metrics.update(err_d, err_q);

        if (i < 5) {
            std::cout << "Test " << i << ": α=" << alpha << " β=" << beta << " θ=" << theta << "\n";
            std::cout << "  DUT:    Id=" << dut_d << " Iq=" << dut_q << "\n";
            std::cout << "  Golden: Id=" << golden_d << " Iq=" << golden_q << "\n";
            std::cout << "  Error:  Id=" << err_d << " Iq=" << err_q << "\n\n";
        }
    }

    std::cout << "Random tests completed.\n";
}

void test_angle_sweep(ErrorMetrics& metrics) {
    std::cout << "\n[TEST 2] Angle Sweep (" << NUM_ANGLE_TESTS << " tests)\n";
    std::cout << "----------------------------------------\n";

    const float amplitude = 10.0f;
    const float pi = 3.14159265359f;

    for (int i = 0; i < NUM_ANGLE_TESTS; i++) {
        float theta = 2.0f * pi * i / NUM_ANGLE_TESTS;
        float cos_theta = std::cos(theta);
        float sin_theta = std::sin(theta);

        // Fixed α-β input
        float alpha = amplitude;
        float beta = 0.0f;

        // DUT
        float dut_d, dut_q;
        hls::Park_Direct_fp32(dut_d, dut_q, alpha, beta, cos_theta, sin_theta);

        // Golden
        float golden_d, golden_q;
        golden::park_direct_golden(golden_d, golden_q, alpha, beta, cos_theta, sin_theta);

        float err_d = dut_d - golden_d;
        float err_q = dut_q - golden_q;
        metrics.update(err_d, err_q);
    }

    std::cout << "Angle sweep tests completed.\n";
}

void test_boundary_cases(ErrorMetrics& metrics) {
    std::cout << "\n[TEST 3] Boundary Cases\n";
    std::cout << "----------------------------------------\n";

    struct TestCase {
        const char* name;
        float alpha, beta, theta;
    };

    const float pi = 3.14159265359f;
    TestCase cases[] = {{"All zeros", 0.0f, 0.0f, 0.0f},
                        {"θ=0°", 10.0f, 5.0f, 0.0f},
                        {"θ=90°", 10.0f, 5.0f, pi / 2.0f},
                        {"θ=180°", 10.0f, 5.0f, pi},
                        {"θ=270°", 10.0f, 5.0f, 3.0f * pi / 2.0f},
                        {"Large amplitude", 100.0f, 100.0f, pi / 4.0f},
                        {"Small amplitude", 0.001f, 0.001f, pi / 4.0f}};

    for (const auto& tc : cases) {
        float cos_theta = std::cos(tc.theta);
        float sin_theta = std::sin(tc.theta);

        // DUT
        float dut_d, dut_q;
        hls::Park_Direct_fp32(dut_d, dut_q, tc.alpha, tc.beta, cos_theta, sin_theta);

        // Golden
        float golden_d, golden_q;
        golden::park_direct_golden(golden_d, golden_q, tc.alpha, tc.beta, cos_theta, sin_theta);

        float err_d = dut_d - golden_d;
        float err_q = dut_q - golden_q;
        metrics.update(err_d, err_q);

        std::cout << "Case: " << tc.name << "\n";
        std::cout << "  Input:  α=" << tc.alpha << " β=" << tc.beta << " θ=" << tc.theta << "\n";
        std::cout << "  DUT:    Id=" << dut_d << " Iq=" << dut_q << "\n";
        std::cout << "  Golden: Id=" << golden_d << " Iq=" << golden_q << "\n";
        std::cout << "  Error:  Id=" << err_d << " Iq=" << err_q << "\n\n";
    }

    std::cout << "Boundary tests completed.\n";
}

int main() {
    std::cout << "========================================\n";
    std::cout << "Park Direct FP32 HLS Testbench\n";
    std::cout << "Phase 2: Versal FP32 Implementation\n";
    std::cout << "========================================\n";

    ErrorMetrics metrics;

    test_random_inputs(metrics);
    test_angle_sweep(metrics);
    test_boundary_cases(metrics);

    metrics.print_summary();

    if (metrics.all_passed()) {
        std::cout << "\n✓ ALL TESTS PASSED\n\n";
        return 0;
    } else {
        std::cout << "\n✗ SOME TESTS FAILED\n\n";
        return 1;
    }
}
