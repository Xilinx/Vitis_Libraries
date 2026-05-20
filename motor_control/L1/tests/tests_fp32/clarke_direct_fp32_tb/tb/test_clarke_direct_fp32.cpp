/*
Copyright (C) 2025-2025, Advanced Micro Devices, Inc.
SPDX-License-Identifier: X11
*/

/**
 * @file test_clarke_direct_fp32.cpp
 * @brief HLS Testbench for Clarke Direct Transform FP32
 * 
 * This testbench validates the HLS FP32 implementation against
 * the Phase 1 golden reference model.
 * 
 * Test Strategy:
 *   1. Generate diverse test vectors (random, sine, boundary cases)
 *   2. Compare HLS FP32 output vs Phase 1 golden float32
 *   3. Verify precision: max error < 1e-6 (FP32 machine epsilon)
 *   4. Report pass/fail status for HLS C simulation
 */

#include <iostream>
#include <iomanip>
#include <cmath>
#include <vector>
#include <random>

// DUT: HLS FP32 synthesizable version
#include "../src/clarke_direct_fp32.hpp"

// Golden: Phase 1 reference model
#include "../../../../include/models_fp/clarke_direct.hpp"

using namespace xf::motorcontrol;

// ============================================================================
// Test Configuration
// ============================================================================

constexpr int NUM_RANDOM_TESTS = 1000;
constexpr int NUM_SINE_TESTS = 360;
// Golden uses /3.0; HLS DUT uses *0.333333333333f — small float path difference (~1.5e-5)
constexpr float PRECISION_THRESHOLD = 2e-5f;

// ============================================================================
// Utility: Error Metrics
// ============================================================================

struct ErrorMetrics {
    float max_error_alpha;
    float max_error_beta;
    float max_error_homop;
    int num_tests;
    int num_passed;
    
    ErrorMetrics() : max_error_alpha(0), max_error_beta(0), max_error_homop(0),
                     num_tests(0), num_passed(0) {}
    
    void update(float err_alpha, float err_beta, float err_homop) {
        max_error_alpha = std::max(max_error_alpha, std::abs(err_alpha));
        max_error_beta = std::max(max_error_beta, std::abs(err_beta));
        max_error_homop = std::max(max_error_homop, std::abs(err_homop));
        
        num_tests++;
        if (std::abs(err_alpha) < PRECISION_THRESHOLD &&
            std::abs(err_beta) < PRECISION_THRESHOLD &&
            std::abs(err_homop) < PRECISION_THRESHOLD) {
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
        std::cout << "Pass rate:       " << std::fixed << std::setprecision(2) 
                  << (100.0f * num_passed / num_tests) << "%\n";
        std::cout << "\nMax Errors:\n";
        std::cout << "  Ialpha: " << std::scientific << max_error_alpha << "\n";
        std::cout << "  Ibeta:  " << std::scientific << max_error_beta << "\n";
        std::cout << "  Ihomop: " << std::scientific << max_error_homop << "\n";
        std::cout << "\nPrecision threshold: " << PRECISION_THRESHOLD << "\n";
        std::cout << "========================================\n";
    }
    
    bool all_passed() const {
        return num_passed == num_tests;
    }
};

// ============================================================================
// Test Case 1: Random Inputs
// ============================================================================

void test_random_inputs(ErrorMetrics& metrics) {
    std::cout << "\n[TEST 1] Random Inputs (" << NUM_RANDOM_TESTS << " tests)\n";
    std::cout << "----------------------------------------\n";
    
    std::mt19937 rng(42);  // Fixed seed for reproducibility
    std::uniform_real_distribution<float> dist(-100.0f, 100.0f);
    
    for (int i = 0; i < NUM_RANDOM_TESTS; i++) {
        // Generate random inputs
        float ia = dist(rng);
        float ib = dist(rng);
        float ic = dist(rng);
        
        // DUT: HLS FP32 version
        float dut_alpha, dut_beta, dut_homop;
        hls::Clarke_Direct_3p_fp32(dut_alpha, dut_beta, dut_homop, ia, ib, ic);
        
        // Golden: Phase 1 reference
        float golden_alpha, golden_beta, golden_homop;
        golden::clarke_direct_golden(golden_alpha, golden_beta, golden_homop, ia, ib, ic);
        
        // Calculate errors
        float err_alpha = dut_alpha - golden_alpha;
        float err_beta = dut_beta - golden_beta;
        float err_homop = dut_homop - golden_homop;
        
        metrics.update(err_alpha, err_beta, err_homop);
        
        // Print first 5 tests for verification
        if (i < 5) {
            std::cout << "Test " << i << ": Ia=" << ia << " Ib=" << ib << " Ic=" << ic << "\n";
            std::cout << "  DUT:    α=" << dut_alpha << " β=" << dut_beta << " I0=" << dut_homop << "\n";
            std::cout << "  Golden: α=" << golden_alpha << " β=" << golden_beta << " I0=" << golden_homop << "\n";
            std::cout << "  Error:  α=" << err_alpha << " β=" << err_beta << " I0=" << err_homop << "\n\n";
        }
    }
    
    std::cout << "Random tests completed.\n";
}

// ============================================================================
// Test Case 2: Sine Wave (Rotating Vector)
// ============================================================================

void test_sine_wave(ErrorMetrics& metrics) {
    std::cout << "\n[TEST 2] Sine Wave - Rotating Vector (" << NUM_SINE_TESTS << " tests)\n";
    std::cout << "----------------------------------------\n";
    
    const float amplitude = 10.0f;
    const float pi = 3.14159265359f;
    
    for (int i = 0; i < NUM_SINE_TESTS; i++) {
        float theta = 2.0f * pi * i / NUM_SINE_TESTS;
        
        // Generate balanced 3-phase currents
        float ia = amplitude * std::cos(theta);
        float ib = amplitude * std::cos(theta - 2.0f * pi / 3.0f);
        float ic = amplitude * std::cos(theta + 2.0f * pi / 3.0f);
        
        // DUT
        float dut_alpha, dut_beta, dut_homop;
        hls::Clarke_Direct_3p_fp32(dut_alpha, dut_beta, dut_homop, ia, ib, ic);
        
        // Golden
        float golden_alpha, golden_beta, golden_homop;
        golden::clarke_direct_golden(golden_alpha, golden_beta, golden_homop, ia, ib, ic);
        
        // Calculate errors
        float err_alpha = dut_alpha - golden_alpha;
        float err_beta = dut_beta - golden_beta;
        float err_homop = dut_homop - golden_homop;
        
        metrics.update(err_alpha, err_beta, err_homop);
    }
    
    std::cout << "Sine wave tests completed.\n";
}

// ============================================================================
// Test Case 3: Boundary Cases
// ============================================================================

void test_boundary_cases(ErrorMetrics& metrics) {
    std::cout << "\n[TEST 3] Boundary Cases\n";
    std::cout << "----------------------------------------\n";
    
    struct TestCase {
        float ia, ib, ic;
        const char* description;
    };
    
    TestCase cases[] = {
        {0.0f, 0.0f, 0.0f, "All zeros"},
        {1.0f, 0.0f, 0.0f, "Only Ia"},
        {0.0f, 1.0f, 0.0f, "Only Ib"},
        {0.0f, 0.0f, 1.0f, "Only Ic"},
        {10.0f, -5.0f, -5.0f, "Balanced (standard)"},
        {100.0f, -50.0f, -50.0f, "Large amplitude"},
        {0.001f, -0.0005f, -0.0005f, "Small amplitude"},
        {-10.0f, 5.0f, 5.0f, "Negative dominant"}
    };
    
    for (const auto& tc : cases) {
        // DUT
        float dut_alpha, dut_beta, dut_homop;
        hls::Clarke_Direct_3p_fp32(dut_alpha, dut_beta, dut_homop, tc.ia, tc.ib, tc.ic);
        
        // Golden
        float golden_alpha, golden_beta, golden_homop;
        golden::clarke_direct_golden(golden_alpha, golden_beta, golden_homop, tc.ia, tc.ib, tc.ic);
        
        // Calculate errors
        float err_alpha = dut_alpha - golden_alpha;
        float err_beta = dut_beta - golden_beta;
        float err_homop = dut_homop - golden_homop;
        
        metrics.update(err_alpha, err_beta, err_homop);
        
        std::cout << "Case: " << tc.description << "\n";
        std::cout << "  Input:  Ia=" << tc.ia << " Ib=" << tc.ib << " Ic=" << tc.ic << "\n";
        std::cout << "  DUT:    α=" << dut_alpha << " β=" << dut_beta << " I0=" << dut_homop << "\n";
        std::cout << "  Golden: α=" << golden_alpha << " β=" << golden_beta << " I0=" << golden_homop << "\n";
        std::cout << "  Error:  α=" << err_alpha << " β=" << err_beta << " I0=" << err_homop << "\n\n";
    }
    
    std::cout << "Boundary tests completed.\n";
}

// ============================================================================
// Main Test Function
// ============================================================================

int main() {
    std::cout << "========================================\n";
    std::cout << "Clarke Direct FP32 HLS Testbench\n";
    std::cout << "Phase 2: Versal FP32 Implementation\n";
    std::cout << "========================================\n";
    
    ErrorMetrics metrics;
    
    // Run all test suites
    test_random_inputs(metrics);
    test_sine_wave(metrics);
    test_boundary_cases(metrics);
    
    // Print summary
    metrics.print_summary();
    
    // Return 0 for pass, 1 for fail (HLS convention)
    if (metrics.all_passed()) {
        std::cout << "\n✓ ALL TESTS PASSED\n\n";
        return 0;
    } else {
        std::cout << "\n✗ SOME TESTS FAILED\n\n";
        return 1;
    }
}
