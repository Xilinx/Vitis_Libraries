/*
Copyright (C) 2025-2025, Advanced Micro Devices, Inc.
SPDX-License-Identifier: X11
*/

#include <iostream>
#include <iomanip>
#include <cmath>
#include <random>

#include "../src/park_inverse_fp32.hpp"
#include "../../../../include/models_fp/park_inverse.hpp"

using namespace xf::motorcontrol;

constexpr int NUM_RANDOM_TESTS = 1000;
constexpr int NUM_ANGLE_TESTS = 360;
constexpr float PRECISION_THRESHOLD = 1e-6f;

struct ErrorMetrics {
    float max_error_alpha, max_error_beta;
    int num_tests, num_passed;
    
    ErrorMetrics() : max_error_alpha(0), max_error_beta(0), num_tests(0), num_passed(0) {}
    
    void update(float err_a, float err_b) {
        max_error_alpha = std::max(max_error_alpha, std::abs(err_a));
        max_error_beta = std::max(max_error_beta, std::abs(err_b));
        num_tests++;
        if (std::abs(err_a) < PRECISION_THRESHOLD && std::abs(err_b) < PRECISION_THRESHOLD) {
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
        std::cout << "  Valpha: " << std::scientific << max_error_alpha << "\n";
        std::cout << "  Vbeta:  " << std::scientific << max_error_beta << "\n";
        std::cout << "\nPrecision threshold: " << PRECISION_THRESHOLD << "\n";
        std::cout << "========================================\n";
    }
    
    bool all_passed() const { return num_passed == num_tests; }
};

void test_random_inputs(ErrorMetrics& metrics) {
    std::cout << "\n[TEST 1] Random Inputs (" << NUM_RANDOM_TESTS << " tests)\n";
    std::cout << "----------------------------------------\n";
    
    std::mt19937 rng(42);
    std::uniform_real_distribution<float> dist_dq(-100.0f, 100.0f);
    std::uniform_real_distribution<float> dist_angle(0.0f, 2.0f * 3.14159265359f);
    
    for (int i = 0; i < NUM_RANDOM_TESTS; i++) {
        float d = dist_dq(rng);
        float q = dist_dq(rng);
        float theta = dist_angle(rng);
        float cos_theta = std::cos(theta);
        float sin_theta = std::sin(theta);
        
        float dut_alpha, dut_beta;
        hls::Park_Inverse_fp32(dut_alpha, dut_beta, d, q, cos_theta, sin_theta);
        
        float golden_alpha, golden_beta;
        golden::park_inverse_golden(golden_alpha, golden_beta, d, q, cos_theta, sin_theta);
        
        float err_alpha = dut_alpha - golden_alpha;
        float err_beta = dut_beta - golden_beta;
        metrics.update(err_alpha, err_beta);
        
        if (i < 5) {
            std::cout << "Test " << i << ": Id=" << d << " Iq=" << q << " θ=" << theta << "\n";
            std::cout << "  DUT:    α=" << dut_alpha << " β=" << dut_beta << "\n";
            std::cout << "  Golden: α=" << golden_alpha << " β=" << golden_beta << "\n";
            std::cout << "  Error:  α=" << err_alpha << " β=" << err_beta << "\n\n";
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
        
        float d = amplitude;
        float q = 0.0f;
        
        float dut_alpha, dut_beta;
        hls::Park_Inverse_fp32(dut_alpha, dut_beta, d, q, cos_theta, sin_theta);
        
        float golden_alpha, golden_beta;
        golden::park_inverse_golden(golden_alpha, golden_beta, d, q, cos_theta, sin_theta);
        
        float err_alpha = dut_alpha - golden_alpha;
        float err_beta = dut_beta - golden_beta;
        metrics.update(err_alpha, err_beta);
    }
    
    std::cout << "Angle sweep tests completed.\n";
}

void test_boundary_cases(ErrorMetrics& metrics) {
    std::cout << "\n[TEST 3] Boundary Cases\n";
    std::cout << "----------------------------------------\n";
    
    struct TestCase {
        const char* name;
        float d, q, theta;
    };
    
    const float pi = 3.14159265359f;
    TestCase cases[] = {
        {"All zeros",       0.0f,   0.0f,   0.0f},
        {"θ=0°",           10.0f,   5.0f,   0.0f},
        {"θ=90°",          10.0f,   5.0f,   pi/2.0f},
        {"θ=180°",         10.0f,   5.0f,   pi},
        {"θ=270°",         10.0f,   5.0f,   3.0f*pi/2.0f},
        {"Large amplitude", 100.0f, 100.0f, pi/4.0f},
        {"Small amplitude", 0.001f, 0.001f, pi/4.0f}
    };
    
    for (const auto& tc : cases) {
        float cos_theta = std::cos(tc.theta);
        float sin_theta = std::sin(tc.theta);
        
        float dut_alpha, dut_beta;
        hls::Park_Inverse_fp32(dut_alpha, dut_beta, tc.d, tc.q, cos_theta, sin_theta);
        
        float golden_alpha, golden_beta;
        golden::park_inverse_golden(golden_alpha, golden_beta, tc.d, tc.q, cos_theta, sin_theta);
        
        float err_alpha = dut_alpha - golden_alpha;
        float err_beta = dut_beta - golden_beta;
        metrics.update(err_alpha, err_beta);
        
        std::cout << "Case: " << tc.name << "\n";
        std::cout << "  Input:  Id=" << tc.d << " Iq=" << tc.q << " θ=" << tc.theta << "\n";
        std::cout << "  DUT:    α=" << dut_alpha << " β=" << dut_beta << "\n";
        std::cout << "  Golden: α=" << golden_alpha << " β=" << golden_beta << "\n";
        std::cout << "  Error:  α=" << err_alpha << " β=" << err_beta << "\n\n";
    }
    
    std::cout << "Boundary tests completed.\n";
}

int main() {
    std::cout << "========================================\n";
    std::cout << "Park Inverse FP32 HLS Testbench\n";
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
