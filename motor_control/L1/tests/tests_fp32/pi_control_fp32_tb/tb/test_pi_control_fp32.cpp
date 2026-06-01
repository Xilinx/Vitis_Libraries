/*
Copyright (C) 2025-2025, Advanced Micro Devices, Inc.
SPDX-License-Identifier: X11
*/

#include <iostream>
#include <iomanip>
#include <cmath>
#include <vector>

#include "../src/pi_control_fp32.hpp"
#include "../../../../include/models_fp/pi_control.hpp"

using namespace xf::motorcontrol;

constexpr float PRECISION_THRESHOLD = 1e-6f;

struct ErrorMetrics {
    float max_error;
    int num_tests, num_passed;

    ErrorMetrics() : max_error(0), num_tests(0), num_passed(0) {}

    void update(float err) {
        max_error = std::max(max_error, std::abs(err));
        num_tests++;
        if (std::abs(err) < PRECISION_THRESHOLD) {
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
        std::cout << "\nMax Error: " << std::scientific << max_error << "\n";
        std::cout << "Precision threshold: " << PRECISION_THRESHOLD << "\n";
        std::cout << "========================================\n";
    }

    bool all_passed() const { return num_passed == num_tests; }
};

void test_step_response(ErrorMetrics& metrics) {
    std::cout << "\n[TEST 1] Step Response (100 steps)\n";
    std::cout << "----------------------------------------\n";

    hls::PIControllerState_fp32 dut_state;
    golden::PIControllerState<float> golden_state;

    float setpoint = 100.0f;
    float measured = 0.0f;
    float kp = 0.5f;
    float ki = 0.1f;

    for (int i = 0; i < 100; i++) {
        float dut_output;
        hls::PI_Control_fp32(dut_output, setpoint, measured, kp, ki, false, dut_state);

        float golden_output;
        golden::pi_control_golden(golden_output, golden_state, measured, setpoint, kp, ki);

        // Simple plant model: measured moves toward output
        measured += dut_output * 0.01f;

        float err = dut_output - golden_output;
        metrics.update(err);

        if (i < 5 || i % 20 == 0) {
            std::cout << "Step " << i << ": Setpoint=" << setpoint << " Measured=" << measured << "\n";
            std::cout << "  DUT Output:    " << dut_output << "\n";
            std::cout << "  Golden Output: " << golden_output << "\n";
            std::cout << "  Error: " << err << "\n";
            std::cout << "  DUT Integral: " << dut_state.integral_error << "\n\n";
        }
    }

    std::cout << "Step response test completed.\n";
}

void test_mode_change(ErrorMetrics& metrics) {
    std::cout << "\n[TEST 2] Mode Change (Integral Reset)\n";
    std::cout << "----------------------------------------\n";

    hls::PIControllerState_fp32 dut_state;
    golden::PIControllerState<float> golden_state;

    // Build up some integral
    float setpoint = 50.0f;
    float measured = 0.0f;
    float kp = 0.5f;
    float ki = 0.2f;

    for (int i = 0; i < 10; i++) {
        float dut_output, golden_output;
        hls::PI_Control_fp32(dut_output, setpoint, measured, kp, ki, false, dut_state);
        golden::pi_control_golden(golden_output, golden_state, measured, setpoint, kp, ki);

        float err = dut_output - golden_output;
        metrics.update(err);
    }

    std::cout << "Before mode change: Integral = " << dut_state.integral_error << "\n";

    // Mode change - should reset integral
    float dut_output, golden_output;
    hls::PI_Control_fp32(dut_output, setpoint, measured, kp, ki, true, dut_state);
    golden::pi_control_golden(golden_output, golden_state, measured, setpoint, kp, ki, true);

    std::cout << "After mode change:  Integral = " << dut_state.integral_error << "\n";
    std::cout << "DUT Output: " << dut_output << "\n";
    std::cout << "Golden Output: " << golden_output << "\n";

    float err = dut_output - golden_output;
    metrics.update(err);

    std::cout << "Error: " << err << "\n";
    std::cout << "Mode change test completed.\n";
}

void test_boundary_cases(ErrorMetrics& metrics) {
    std::cout << "\n[TEST 3] Boundary Cases\n";
    std::cout << "----------------------------------------\n";

    struct TestCase {
        const char* name;
        float measured, setpoint, kp, ki;
    };

    TestCase cases[] = {{"Zero error", 50.0f, 50.0f, 1.0f, 0.5f},     {"Large error", 0.0f, 100.0f, 1.0f, 0.5f},
                        {"Negative error", 100.0f, 0.0f, 1.0f, 0.5f}, {"Zero gains", 10.0f, 50.0f, 0.0f, 0.0f},
                        {"High Kp", 10.0f, 50.0f, 10.0f, 0.1f},       {"High Ki", 10.0f, 50.0f, 0.1f, 10.0f}};

    for (const auto& tc : cases) {
        hls::PIControllerState_fp32 dut_state;
        golden::PIControllerState<float> golden_state;

        float dut_output, golden_output;
        hls::PI_Control_fp32(dut_output, tc.setpoint, tc.measured, tc.kp, tc.ki, false, dut_state);
        golden::pi_control_golden(golden_output, golden_state, tc.measured, tc.setpoint, tc.kp, tc.ki);

        float err = dut_output - golden_output;
        metrics.update(err);

        std::cout << "Case: " << tc.name << "\n";
        std::cout << "  Measured=" << tc.measured << " Setpoint=" << tc.setpoint << " Kp=" << tc.kp << " Ki=" << tc.ki
                  << "\n";
        std::cout << "  DUT Output:    " << dut_output << "\n";
        std::cout << "  Golden Output: " << golden_output << "\n";
        std::cout << "  Error: " << err << "\n\n";
    }

    std::cout << "Boundary tests completed.\n";
}

int main() {
    std::cout << "========================================\n";
    std::cout << "PI Controller FP32 HLS Testbench\n";
    std::cout << "Phase 2: Versal FP32 Implementation\n";
    std::cout << "========================================\n";

    ErrorMetrics metrics;

    test_step_response(metrics);
    test_mode_change(metrics);
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
