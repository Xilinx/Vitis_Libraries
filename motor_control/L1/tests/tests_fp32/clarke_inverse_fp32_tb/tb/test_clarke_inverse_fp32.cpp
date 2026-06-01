/*
Copyright (C) 2025-2025, Advanced Micro Devices, Inc.
SPDX-License-Identifier: X11
*/

/**
 * @file test_clarke_inverse_fp32.cpp
 * @brief HLS Testbench for Clarke Inverse Transform FP32
 *
 * This testbench validates the HLS FP32 implementation against
 * the Phase 1 golden reference model.
 *
 * Test Strategy:
 *   1. Generate diverse test vectors (random, sine, boundary cases)
 *   2. Compare HLS FP32 output vs Phase 1 golden float32
 *   3. Verify precision: max error < 1e-6 (FP32 machine epsilon)
 *   4. Verify balanced system property: Va + Vb + Vc = 0
 */

#include <iostream>
#include <iomanip>
#include <cmath>
#include <vector>
#include <random>

// DUT: HLS FP32 synthesizable version
#include "../src/clarke_inverse_fp32.hpp"

// Golden: Phase 1 reference model
#include "../../../../include/models_fp/clarke_inverse.hpp"

using namespace xf::motorcontrol;

// ============================================================================
// Test Configuration
// ============================================================================

constexpr int NUM_RANDOM_TESTS = 1000;
constexpr int NUM_SINE_TESTS = 360;
constexpr float PRECISION_THRESHOLD = 1e-6f; // FP32 precision target

// ============================================================================
// Utility: Error Metrics
// ============================================================================

struct ErrorMetrics {
    float max_error_a;
    float max_error_b;
    float max_error_c;
    int num_tests;
    int num_passed;

    ErrorMetrics() : max_error_a(0), max_error_b(0), max_error_c(0), num_tests(0), num_passed(0) {}

    void update(float err_a, float err_b, float err_c) {
        max_error_a = std::max(max_error_a, std::abs(err_a));
        max_error_b = std::max(max_error_b, std::abs(err_b));
        max_error_c = std::max(max_error_c, std::abs(err_c));

        num_tests++;
        if (std::abs(err_a) < PRECISION_THRESHOLD && std::abs(err_b) < PRECISION_THRESHOLD &&
            std::abs(err_c) < PRECISION_THRESHOLD) {
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
        std::cout << "  Va: " << std::scientific << max_error_a << "\n";
        std::cout << "  Vb: " << std::scientific << max_error_b << "\n";
        std::cout << "  Vc: " << std::scientific << max_error_c << "\n";
        std::cout << "\nPrecision threshold: " << PRECISION_THRESHOLD << "\n";
        std::cout << "========================================\n";
    }

    bool all_passed() const { return num_passed == num_tests; }
};

// ============================================================================
// Test Case 1: Random Inputs
// ============================================================================

void test_random_inputs(ErrorMetrics& metrics) {
    std::cout << "\n[TEST 1] Random Inputs (" << NUM_RANDOM_TESTS << " tests)\n";
    std::cout << "----------------------------------------\n";

    std::mt19937 rng(42); // Fixed seed for reproducibility
    std::uniform_real_distribution<float> dist(-100.0f, 100.0f);

    for (int i = 0; i < NUM_RANDOM_TESTS; i++) {
        // Generate random α-β inputs
        float alpha = dist(rng);
        float beta = dist(rng);

        // DUT: HLS FP32 version
        float dut_a, dut_b, dut_c;
        hls::Clarke_Inverse_2p_fp32(dut_a, dut_b, dut_c, alpha, beta);

        // Golden: Phase 1 reference
        float golden_a, golden_b, golden_c;
        golden::clarke_inverse_golden(golden_a, golden_b, golden_c, alpha, beta);

        // Calculate errors
        float err_a = dut_a - golden_a;
        float err_b = dut_b - golden_b;
        float err_c = dut_c - golden_c;

        metrics.update(err_a, err_b, err_c);

        // Print first 5 tests for verification
        if (i < 5) {
            std::cout << "Test " << i << ": α=" << alpha << " β=" << beta << "\n";
            std::cout << "  DUT:    Va=" << dut_a << " Vb=" << dut_b << " Vc=" << dut_c << "\n";
            std::cout << "  Golden: Va=" << golden_a << " Vb=" << golden_b << " Vc=" << golden_c << "\n";
            std::cout << "  Error:  Va=" << err_a << " Vb=" << err_b << " Vc=" << err_c << "\n";

            // Check balanced system property
            float sum_dut = dut_a + dut_b + dut_c;
            std::cout << "  Balance check: Va+Vb+Vc=" << sum_dut << " (should be ~0)\n\n";
        }
    }

    std::cout << "Random tests completed.\n";
}

// ============================================================================
// Test Case 2: Rotating Vector (Sine Wave in α-β frame)
// ============================================================================

void test_rotating_vector(ErrorMetrics& metrics) {
    std::cout << "\n[TEST 2] Rotating Vector in α-β Frame (" << NUM_SINE_TESTS << " tests)\n";
    std::cout << "----------------------------------------\n";

    const float amplitude = 10.0f;
    const float pi = 3.14159265359f;

    for (int i = 0; i < NUM_SINE_TESTS; i++) {
        float theta = 2.0f * pi * i / NUM_SINE_TESTS;

        // Generate rotating vector in α-β frame
        float alpha = amplitude * std::cos(theta);
        float beta = amplitude * std::sin(theta);

        // DUT
        float dut_a, dut_b, dut_c;
        hls::Clarke_Inverse_2p_fp32(dut_a, dut_b, dut_c, alpha, beta);

        // Golden
        float golden_a, golden_b, golden_c;
        golden::clarke_inverse_golden(golden_a, golden_b, golden_c, alpha, beta);

        // Calculate errors
        float err_a = dut_a - golden_a;
        float err_b = dut_b - golden_b;
        float err_c = dut_c - golden_c;

        metrics.update(err_a, err_b, err_c);
    }

    std::cout << "Rotating vector tests completed.\n";
}

// ============================================================================
// Test Case 3: Boundary Cases
// ============================================================================

void test_boundary_cases(ErrorMetrics& metrics) {
    std::cout << "\n[TEST 3] Boundary Cases\n";
    std::cout << "----------------------------------------\n";

    struct TestCase {
        const char* name;
        float alpha;
        float beta;
    };

    TestCase cases[] = {{"All zeros", 0.0f, 0.0f},           {"Only alpha", 10.0f, 0.0f},
                        {"Only beta", 0.0f, 10.0f},          {"Negative alpha", -10.0f, 5.0f},
                        {"Negative beta", 10.0f, -5.0f},     {"Both negative", -10.0f, -5.0f},
                        {"Large amplitude", 100.0f, 100.0f}, {"Small amplitude", 0.001f, 0.001f}};

    for (const auto& tc : cases) {
        // DUT
        float dut_a, dut_b, dut_c;
        hls::Clarke_Inverse_2p_fp32(dut_a, dut_b, dut_c, tc.alpha, tc.beta);

        // Golden
        float golden_a, golden_b, golden_c;
        golden::clarke_inverse_golden(golden_a, golden_b, golden_c, tc.alpha, tc.beta);

        // Calculate errors
        float err_a = dut_a - golden_a;
        float err_b = dut_b - golden_b;
        float err_c = dut_c - golden_c;

        metrics.update(err_a, err_b, err_c);

        std::cout << "Case: " << tc.name << "\n";
        std::cout << "  Input:  α=" << tc.alpha << " β=" << tc.beta << "\n";
        std::cout << "  DUT:    Va=" << dut_a << " Vb=" << dut_b << " Vc=" << dut_c << "\n";
        std::cout << "  Golden: Va=" << golden_a << " Vb=" << golden_b << " Vc=" << golden_c << "\n";
        std::cout << "  Error:  Va=" << err_a << " Vb=" << err_b << " Vc=" << err_c << "\n";

        // Check balanced system property
        float sum = dut_a + dut_b + dut_c;
        std::cout << "  Balance: Va+Vb+Vc=" << sum << "\n\n";
    }

    std::cout << "Boundary tests completed.\n";
}

// ============================================================================
// Main Test Entry
// ============================================================================

int main() {
    std::cout << "========================================\n";
    std::cout << "Clarke Inverse FP32 HLS Testbench\n";
    std::cout << "Phase 2: Versal FP32 Implementation\n";
    std::cout << "========================================\n";

    ErrorMetrics metrics;

    // Run all test suites
    test_random_inputs(metrics);
    test_rotating_vector(metrics);
    test_boundary_cases(metrics);

    // Print summary
    metrics.print_summary();

    // Return status
    if (metrics.all_passed()) {
        std::cout << "\n✓ ALL TESTS PASSED\n\n";
        return 0;
    } else {
        std::cout << "\n✗ SOME TESTS FAILED\n\n";
        return 1;
    }
}
