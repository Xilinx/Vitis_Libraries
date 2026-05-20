/*
Copyright (C) 2025-2025, Advanced Micro Devices, Inc.
SPDX-License-Identifier: X11
*/

#include <iostream>
#include <iomanip>
#include <cmath>

#include "../src/svpwm_fp32.hpp"
#include "../../../../include/models_fp/svpwm.hpp"

using namespace xf::motorcontrol;

constexpr float PRECISION_THRESHOLD = 1e-5f;  // Slightly relaxed for SVPWM

struct ErrorMetrics {
    float max_error_duty_a, max_error_duty_b, max_error_duty_c;
    int num_tests, num_passed;
    
    ErrorMetrics() : max_error_duty_a(0), max_error_duty_b(0), max_error_duty_c(0),
                     num_tests(0), num_passed(0) {}
    
    void update(float err_a, float err_b, float err_c) {
        max_error_duty_a = std::max(max_error_duty_a, std::abs(err_a));
        max_error_duty_b = std::max(max_error_duty_b, std::abs(err_b));
        max_error_duty_c = std::max(max_error_duty_c, std::abs(err_c));
        num_tests++;
        if (std::abs(err_a) < PRECISION_THRESHOLD &&
            std::abs(err_b) < PRECISION_THRESHOLD &&
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
        std::cout << "Pass rate:       " << std::fixed << std::setprecision(2) 
                  << (100.0f * num_passed / num_tests) << "%\n";
        std::cout << "\nMax Errors:\n";
        std::cout << "  Duty A: " << std::scientific << max_error_duty_a << "\n";
        std::cout << "  Duty B: " << std::scientific << max_error_duty_b << "\n";
        std::cout << "  Duty C: " << std::scientific << max_error_duty_c << "\n";
        std::cout << "\nPrecision threshold: " << PRECISION_THRESHOLD << "\n";
        std::cout << "========================================\n";
    }
    
    bool all_passed() const { return num_passed == num_tests; }
};

void test_balanced_voltages(ErrorMetrics& metrics) {
    std::cout << "\n[TEST 1] Balanced 3-Phase Voltages (360 tests)\n";
    std::cout << "----------------------------------------\n";
    
    const float amplitude = 100.0f;
    const float dc_link = 300.0f;
    const float pi = 3.14159265359f;
    
    for (int i = 0; i < 360; i++) {
        float theta = 2.0f * pi * i / 360.0f;
        
        // Generate balanced 3-phase commands
        float va_cmd = amplitude * std::cos(theta);
        float vb_cmd = amplitude * std::cos(theta - 2.0f * pi / 3.0f);
        float vc_cmd = amplitude * std::cos(theta + 2.0f * pi / 3.0f);
        
        // DUT
        hls::SVPWMOutput_fp32 dut_output;
        hls::SVPWM_fp32(dut_output, va_cmd, vb_cmd, vc_cmd, dc_link);
        
        // Golden
        golden::SVPWMOutput<float> golden_output;
        golden::svpwm_golden(golden_output, va_cmd, vb_cmd, vc_cmd, dc_link);
        
        float err_a = dut_output.duty_a - golden_output.duty_a;
        float err_b = dut_output.duty_b - golden_output.duty_b;
        float err_c = dut_output.duty_c - golden_output.duty_c;
        metrics.update(err_a, err_b, err_c);
        
        if (i == 0 || i == 60 || i == 120 || i == 180 || i == 240 || i == 300) {
            std::cout << "θ=" << i << "°: Va=" << va_cmd << " Vb=" << vb_cmd << " Vc=" << vc_cmd << "\n";
            std::cout << "  DUT Duties:    A=" << dut_output.duty_a << " B=" << dut_output.duty_b 
                      << " C=" << dut_output.duty_c << " Sector=" << dut_output.sector << "\n";
            std::cout << "  Golden Duties: A=" << golden_output.duty_a << " B=" << golden_output.duty_b 
                      << " C=" << golden_output.duty_c << " Sector=" << golden_output.sector << "\n";
            std::cout << "  Errors: A=" << err_a << " B=" << err_b << " C=" << err_c << "\n\n";
        }
    }
    
    std::cout << "Balanced voltage tests completed.\n";
}

void test_boundary_cases(ErrorMetrics& metrics) {
    std::cout << "\n[TEST 2] Boundary Cases\n";
    std::cout << "----------------------------------------\n";
    
    struct TestCase {
        const char* name;
        float va, vb, vc, dc_link;
    };
    
    TestCase cases[] = {
        {"All zeros",           0.0f,   0.0f,   0.0f,   300.0f},
        {"Only Va",           100.0f,   0.0f,   0.0f,   300.0f},
        {"Balanced max",      100.0f, -50.0f, -50.0f,  300.0f},
        {"High modulation",   150.0f, -75.0f, -75.0f,  300.0f},
        {"Low DC link",        10.0f,  -5.0f,  -5.0f,   50.0f},
        {"Negative voltages", -50.0f, -50.0f, -50.0f,  300.0f}
    };
    
    for (const auto& tc : cases) {
        hls::SVPWMOutput_fp32 dut_output;
        hls::SVPWM_fp32(dut_output, tc.va, tc.vb, tc.vc, tc.dc_link);
        
        golden::SVPWMOutput<float> golden_output;
        golden::svpwm_golden(golden_output, tc.va, tc.vb, tc.vc, tc.dc_link);
        
        float err_a = dut_output.duty_a - golden_output.duty_a;
        float err_b = dut_output.duty_b - golden_output.duty_b;
        float err_c = dut_output.duty_c - golden_output.duty_c;
        metrics.update(err_a, err_b, err_c);
        
        std::cout << "Case: " << tc.name << "\n";
        std::cout << "  Input:  Va=" << tc.va << " Vb=" << tc.vb << " Vc=" << tc.vc 
                  << " DC=" << tc.dc_link << "\n";
        std::cout << "  DUT Duties:    A=" << dut_output.duty_a << " B=" << dut_output.duty_b 
                  << " C=" << dut_output.duty_c << "\n";
        std::cout << "  Golden Duties: A=" << golden_output.duty_a << " B=" << golden_output.duty_b 
                  << " C=" << golden_output.duty_c << "\n";
        std::cout << "  Errors: A=" << err_a << " B=" << err_b << " C=" << err_c << "\n\n";
    }
    
    std::cout << "Boundary tests completed.\n";
}

void test_sector_coverage(ErrorMetrics& metrics) {
    std::cout << "\n[TEST 3] Sector Coverage (60 tests, 10 per sector)\n";
    std::cout << "----------------------------------------\n";
    
    const float amplitude = 100.0f;
    const float dc_link = 300.0f;
    const float pi = 3.14159265359f;
    
    int sector_count[7] = {0};  // Index 1-6 for sectors
    
    for (int sector = 1; sector <= 6; sector++) {
        float base_angle = (sector - 1) * pi / 3.0f;
        
        for (int j = 0; j < 10; j++) {
            float theta = base_angle + (pi / 3.0f) * j / 10.0f;
            
            float va_cmd = amplitude * std::cos(theta);
            float vb_cmd = amplitude * std::cos(theta - 2.0f * pi / 3.0f);
            float vc_cmd = amplitude * std::cos(theta + 2.0f * pi / 3.0f);
            
            hls::SVPWMOutput_fp32 dut_output;
            hls::SVPWM_fp32(dut_output, va_cmd, vb_cmd, vc_cmd, dc_link);
            
            golden::SVPWMOutput<float> golden_output;
            golden::svpwm_golden(golden_output, va_cmd, vb_cmd, vc_cmd, dc_link);
            
            sector_count[dut_output.sector]++;
            
            float err_a = dut_output.duty_a - golden_output.duty_a;
            float err_b = dut_output.duty_b - golden_output.duty_b;
            float err_c = dut_output.duty_c - golden_output.duty_c;
            metrics.update(err_a, err_b, err_c);
        }
    }
    
    std::cout << "Sector distribution:\n";
    for (int i = 1; i <= 6; i++) {
        std::cout << "  Sector " << i << ": " << sector_count[i] << " samples\n";
    }
    
    std::cout << "Sector coverage test completed.\n";
}

int main() {
    std::cout << "========================================\n";
    std::cout << "SVPWM FP32 HLS Testbench\n";
    std::cout << "Phase 2: Versal FP32 Implementation\n";
    std::cout << "========================================\n";
    
    ErrorMetrics metrics;
    
    test_balanced_voltages(metrics);
    test_boundary_cases(metrics);
    test_sector_coverage(metrics);
    
    metrics.print_summary();
    
    if (metrics.all_passed()) {
        std::cout << "\n✓ ALL TESTS PASSED\n\n";
        return 0;
    } else {
        std::cout << "\n✗ SOME TESTS FAILED\n\n";
        return 1;
    }
}
