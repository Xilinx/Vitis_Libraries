/*
Copyright (C) 2025-2025, Advanced Micro Devices, Inc.
SPDX-License-Identifier: X11

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
X CONSORTIUM BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
IN THE SOFTWARE.

Except as contained in this notice, the name of Advanced Micro Devices
shall not be used in advertising or otherwise to promote the sale,
use or other dealings in this Software without prior written authorization
from Advanced Micro Devices, Inc.
*/

/**
 * @file test_park_inverse.cpp
 * @brief Phase 1 Precision Test for Park Inverse Transform
 * 
 * Test Flow:
 *   1. Generate test inputs (d-q components + angle θ) using stimulus_generator
 *   2. Run Golden Model (float32)
 *   3. Quantize inputs to ap_fixed
 *   4. Run DUT (ap_fixed)
 *   5. Dequantize DUT outputs to float
 *   6. Calculate point-by-point errors
 *   7. Analyze precision with precision_analyzer
 * 
 * Test Modes:
 *   - Random: Random d-q values with random angles
 *   - Sine: Rotating d-q vector with linearly increasing angle
 *   - Boundary: Critical edge cases
 */

#include <iostream>
#include <fstream>
#include <iomanip>
#include <vector>
#include <string>
#include <cmath>

// Vitis HLS headers
#include "ap_fixed.h"

// DUT header
#include "hw/park.hpp"

// Phase 1 validation utilities
#include "stimulus_generator.hpp"
#include "quantization_utils.hpp"
#include "precision_analyzer.hpp"
#include "compatibility.hpp"

using namespace xf::motorcontrol::test;

// ============================================================================
// Test Configuration
// ============================================================================

// Q-format configuration for I/O
constexpr int W_IO = 32;
constexpr int I_IO = 16;
typedef ap_fixed<W_IO, I_IO> T_IO;

// Q-format configuration for sin/cos (range [-1, 1])
constexpr int W_SINCOS = 32;
constexpr int I_SINCOS = 2;
typedef ap_fixed<W_SINCOS, I_SINCOS> T_SINCOS;

// Test parameters
constexpr int NUM_RANDOM_TESTS = 1000;
constexpr float RANDOM_MIN = -100.0f;
constexpr float RANDOM_MAX = 100.0f;

constexpr float SINE_SAMPLE_RATE = 10000.0f;
constexpr float SINE_DURATION = 0.02f;
constexpr float SINE_AMPLITUDE = 10.0f;
constexpr float SINE_FREQUENCY = 50.0f;
constexpr float ROTOR_SPEED_HZ = 50.0f;

// Output file names
const std::string RANDOM_OUTPUT_FILE = "park_inverse_random_errors.csv";
const std::string SINE_OUTPUT_FILE = "park_inverse_sine_errors.csv";
const std::string BOUNDARY_OUTPUT_FILE = "park_inverse_boundary_errors.csv";

// ============================================================================
// Test Data Structure
// ============================================================================

struct TestPoint {
    // Inputs (float32, quantized)
    float vd_float;
    float vq_float;
    float theta_float;
    float cos_theta_float;
    float sin_theta_float;
    
    // Golden outputs (float32)
    float valpha_golden;
    float vbeta_golden;
    
    // DUT outputs (float32, dequantized from ap_fixed)
    float valpha_dut;
    float vbeta_dut;
    
    // Point-by-point errors
    float error_valpha;
    float error_vbeta;
    
    // Magnitude check
    float mag_input;
    float mag_golden;
    float mag_dut;
    
    TestPoint() : vd_float(0), vq_float(0), theta_float(0),
                  cos_theta_float(0), sin_theta_float(0),
                  valpha_golden(0), vbeta_golden(0),
                  valpha_dut(0), vbeta_dut(0),
                  error_valpha(0), error_vbeta(0),
                  mag_input(0), mag_golden(0), mag_dut(0) {}
};

// ============================================================================
// Helper Functions
// ============================================================================

inline float magnitude(float a, float b) {
    return std::sqrt(a * a + b * b);
}

TestPoint run_test_point(float vd, float vq, float theta) {
    TestPoint result;
    
    float cos_theta = std::cos(theta);
    float sin_theta = std::sin(theta);
    
    // Store quantized inputs
    result.vd_float = quantize_float<W_IO, I_IO>(vd);
    result.vq_float = quantize_float<W_IO, I_IO>(vq);
    result.theta_float = theta;
    result.cos_theta_float = quantize_float<W_SINCOS, I_SINCOS>(cos_theta);
    result.sin_theta_float = quantize_float<W_SINCOS, I_SINCOS>(sin_theta);
    
    result.mag_input = magnitude(result.vd_float, result.vq_float);
    
    // Run Golden Model
    xf::motorcontrol::golden::park_inverse_golden(
        result.valpha_golden,
        result.vbeta_golden,
        result.vd_float,
        result.vq_float,
        result.cos_theta_float,
        result.sin_theta_float
    );
    
    result.mag_golden = magnitude(result.valpha_golden, result.vbeta_golden);
    
    // Convert to ap_fixed
    T_IO vd_fixed = float_to_fixed<W_IO, I_IO>(vd);
    T_IO vq_fixed = float_to_fixed<W_IO, I_IO>(vq);
    T_SINCOS cos_theta_fixed = float_to_fixed<W_SINCOS, I_SINCOS>(cos_theta);
    T_SINCOS sin_theta_fixed = float_to_fixed<W_SINCOS, I_SINCOS>(sin_theta);
    
    // Run DUT
    T_IO valpha_fixed, vbeta_fixed;
    
    Park_Inverse_ap_fixed<T_IO, T_SINCOS>(
        valpha_fixed,
        vbeta_fixed,
        vd_fixed,
        vq_fixed,
        cos_theta_fixed,
        sin_theta_fixed
    );
    
    // Convert back to float
    result.valpha_dut = fixed_to_float(valpha_fixed);
    result.vbeta_dut = fixed_to_float(vbeta_fixed);
    
    result.mag_dut = magnitude(result.valpha_dut, result.vbeta_dut);
    
    // Calculate errors
    result.error_valpha = result.valpha_dut - result.valpha_golden;
    result.error_vbeta = result.vbeta_dut - result.vbeta_golden;
    
    return result;
}

void save_results_to_csv(const std::string& filename,
                         const std::vector<TestPoint>& results,
                         const std::string& mode) {
    std::ofstream file(filename);
    
    if (!file.is_open()) {
        std::cerr << "Error: Cannot open file " << filename << std::endl;
        return;
    }
    
    file << "# Park Inverse Transform - Phase 1 Precision Test Results\n";
    file << "# Test Mode: " << mode << "\n";
    file << "# Q-format (I/O): Q" << I_IO << "." << (W_IO-I_IO) << " (ap_fixed<" << W_IO << "," << I_IO << ">)\n";
    file << "# Q-format (sin/cos): Q" << I_SINCOS << "." << (W_SINCOS-I_SINCOS) << " (ap_fixed<" << W_SINCOS << "," << I_SINCOS << ">)\n";
    file << "# Number of test points: " << results.size() << "\n";
    file << "#\n";
    file << "# Columns:\n";
    file << "#   Vd_in, Vq_in: Quantized inputs (rotating frame, float32)\n";
    file << "#   Theta_rad: Input angle in radians\n";
    file << "#   cos_theta, sin_theta: Quantized trig values\n";
    file << "#   Valpha_golden, Vbeta_golden: Golden model outputs (stationary frame)\n";
    file << "#   Valpha_dut, Vbeta_dut: DUT outputs (dequantized)\n";
    file << "#   Error_Valpha, Error_Vbeta: Point-by-point errors (DUT - Golden)\n";
    file << "#   Mag_input, Mag_golden, Mag_dut: Magnitude (should be preserved)\n";
    file << "#\n";
    
    file << "Vd_in,Vq_in,Theta_rad,cos_theta,sin_theta,";
    file << "Valpha_golden,Vbeta_golden,";
    file << "Valpha_dut,Vbeta_dut,";
    file << "Error_Valpha,Error_Vbeta,";
    file << "Mag_input,Mag_golden,Mag_dut\n";
    
    file << std::fixed << std::setprecision(10);
    for (const auto& point : results) {
        file << point.vd_float << "," << point.vq_float << ",";
        file << point.theta_float << ",";
        file << point.cos_theta_float << "," << point.sin_theta_float << ",";
        file << point.valpha_golden << "," << point.vbeta_golden << ",";
        file << point.valpha_dut << "," << point.vbeta_dut << ",";
        file << point.error_valpha << "," << point.error_vbeta << ",";
        file << point.mag_input << "," << point.mag_golden << "," << point.mag_dut << "\n";
    }
    
    file.close();
    std::cout << "Results saved to: " << filename << std::endl;
}

void print_summary(const std::vector<TestPoint>& results, const std::string& mode) {
    if (results.empty()) return;
    
    std::cout << "\n========================================" << std::endl;
    std::cout << "  " << mode << " - Precision Analysis" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "Test points: " << results.size() << std::endl;
    std::cout << std::endl;
    
    std::vector<float> golden_valpha, golden_vbeta;
    std::vector<float> dut_valpha, dut_vbeta;
    
    golden_valpha.reserve(results.size());
    golden_vbeta.reserve(results.size());
    dut_valpha.reserve(results.size());
    dut_vbeta.reserve(results.size());
    
    for (const auto& point : results) {
        golden_valpha.push_back(point.valpha_golden);
        golden_vbeta.push_back(point.vbeta_golden);
        dut_valpha.push_back(point.valpha_dut);
        dut_vbeta.push_back(point.vbeta_dut);
    }
    
    // Analyze Valpha
    std::cout << "--- Valpha (α-axis) ---" << std::endl;
    auto metrics_valpha = PrecisionAnalyzer::analyze(golden_valpha, dut_valpha);
    std::cout << std::scientific << std::setprecision(6);
    std::cout << "  MAE:       " << metrics_valpha.mae << std::endl;
    std::cout << "  RMSE:      " << metrics_valpha.rmse << std::endl;
    std::cout << "  Max Error: " << metrics_valpha.max_error << std::endl;
    std::cout << "  SNR:       " << std::fixed << std::setprecision(2) 
              << metrics_valpha.snr_db << " dB" << std::endl;
    std::cout << std::endl;
    
    // Analyze Vbeta
    std::cout << "--- Vbeta (β-axis) ---" << std::endl;
    auto metrics_vbeta = PrecisionAnalyzer::analyze(golden_vbeta, dut_vbeta);
    std::cout << std::scientific << std::setprecision(6);
    std::cout << "  MAE:       " << metrics_vbeta.mae << std::endl;
    std::cout << "  RMSE:      " << metrics_vbeta.rmse << std::endl;
    std::cout << "  Max Error: " << metrics_vbeta.max_error << std::endl;
    std::cout << "  SNR:       " << std::fixed << std::setprecision(2) 
              << metrics_vbeta.snr_db << " dB" << std::endl;
    std::cout << std::endl;
    
    // Overall Summary
    std::cout << "--- Overall Summary ---" << std::endl;
    float max_mae = std::max(metrics_valpha.mae, metrics_vbeta.mae);
    float max_rmse = std::max(metrics_valpha.rmse, metrics_vbeta.rmse);
    float max_error = std::max(metrics_valpha.max_error, metrics_vbeta.max_error);
    float min_snr = std::min(metrics_valpha.snr_db, metrics_vbeta.snr_db);
    
    std::cout << std::scientific << std::setprecision(6);
    std::cout << "  Worst MAE:       " << max_mae << std::endl;
    std::cout << "  Worst RMSE:      " << max_rmse << std::endl;
    std::cout << "  Worst Max Error: " << max_error << std::endl;
    std::cout << "  Worst SNR:       " << std::fixed << std::setprecision(2) 
              << min_snr << " dB" << std::endl;
    
    // Magnitude preservation check
    std::cout << "\n--- Magnitude Preservation Check ---" << std::endl;
    float max_mag_error = 0.0f;
    float avg_mag_error = 0.0f;
    for (const auto& r : results) {
        float mag_error = std::abs(r.mag_dut - r.mag_input);
        max_mag_error = std::max(max_mag_error, mag_error);
        avg_mag_error += mag_error;
    }
    avg_mag_error /= results.size();
    
    std::cout << std::scientific << std::setprecision(6);
    std::cout << "  Max magnitude error: " << max_mag_error << std::endl;
    std::cout << "  Avg magnitude error: " << avg_mag_error << std::endl;
    
    std::cout << "========================================\n" << std::endl;
}

// ============================================================================
// Test Mode Implementations
// ============================================================================

void test_random_mode() {
    std::cout << "\n=== Random Input Test ===" << std::endl;
    std::cout << "Generating " << NUM_RANDOM_TESTS << " random test cases..." << std::endl;
    
    StimulusGenerator gen(12345);
    std::vector<TestPoint> results;
    results.reserve(NUM_RANDOM_TESTS);
    
    for (int i = 0; i < NUM_RANDOM_TESTS; i++) {
        float vd = gen.random_uniform(RANDOM_MIN, RANDOM_MAX);
        float vq = gen.random_uniform(RANDOM_MIN, RANDOM_MAX);
        float theta = gen.random_uniform(0.0f, 2.0f * PI);
        
        TestPoint result = run_test_point(vd, vq, theta);
        results.push_back(result);
        
        if ((i + 1) % 100 == 0) {
            std::cout << "  Progress: " << (i + 1) << "/" << NUM_RANDOM_TESTS << std::endl;
        }
    }
    
    save_results_to_csv(RANDOM_OUTPUT_FILE, results, "Random dq Inputs with Random Angles");
    print_summary(results, "Random Mode");
}

void test_sine_mode() {
    std::cout << "\n=== Rotating Vector with Angle Sweep Test ===" << std::endl;
    std::cout << "Generating rotating vector test cases..." << std::endl;
    std::cout << "  Frequency: " << SINE_FREQUENCY << " Hz (d-q rotation)" << std::endl;
    std::cout << "  Rotor speed: " << ROTOR_SPEED_HZ << " Hz (angle θ)" << std::endl;
    std::cout << "  Amplitude: " << SINE_AMPLITUDE << std::endl;
    std::cout << "  Duration: " << SINE_DURATION << " s" << std::endl;
    std::cout << "  Sample rate: " << SINE_SAMPLE_RATE << " Hz" << std::endl;
    
    size_t num_samples = static_cast<size_t>(SINE_SAMPLE_RATE * SINE_DURATION);
    std::vector<TestPoint> results;
    results.reserve(num_samples);
    
    float dt = 1.0f / SINE_SAMPLE_RATE;
    float omega_dq = 2.0f * PI * SINE_FREQUENCY;
    float omega_rotor = 2.0f * PI * ROTOR_SPEED_HZ;
    
    for (size_t i = 0; i < num_samples; i++) {
        float t = i * dt;
        
        float vd = SINE_AMPLITUDE * std::cos(omega_dq * t);
        float vq = SINE_AMPLITUDE * std::sin(omega_dq * t);
        
        float theta = omega_rotor * t;
        theta = std::fmod(theta, 2.0f * PI);
        
        TestPoint result = run_test_point(vd, vq, theta);
        results.push_back(result);
        
        if ((i + 1) % 50 == 0) {
            std::cout << "  Progress: " << (i + 1) << "/" << num_samples << std::endl;
        }
    }
    
    std::cout << "  Generated " << num_samples << " samples" << std::endl;
    
    save_results_to_csv(SINE_OUTPUT_FILE, results, "Rotating dq Vector (50 Hz) with Angle Sweep");
    print_summary(results, "Rotating Vector Mode");
}

void test_boundary_cases() {
    std::cout << "\n=== Boundary & Corner Case Test ===" << std::endl;
    std::cout << "Testing critical boundary conditions..." << std::endl;
    
    std::vector<TestPoint> results;
    
    auto fmt_info = FixedPointInfo::create<W_IO, I_IO>();
    float max_val = fmt_info.max_value;
    float min_val = fmt_info.min_value;
    
    std::cout << "  Q-format range: [" << min_val << ", " << max_val << "]" << std::endl;
    std::cout << std::endl;
    
    // Test Case 1: All Zero
    std::cout << "  Test 1: All zero inputs" << std::endl;
    results.push_back(run_test_point(0.0f, 0.0f, 0.0f));
    
    // Test Case 2: Vd only
    std::cout << "  Test 2: Only Vd non-zero" << std::endl;
    results.push_back(run_test_point(10.0f, 0.0f, 0.0f));
    
    // Test Case 3: Vq only
    std::cout << "  Test 3: Only Vq non-zero" << std::endl;
    results.push_back(run_test_point(0.0f, 10.0f, 0.0f));
    
    // Test Case 4-7: Different angles with same dq
    std::cout << "  Test 4-7: Same dq, different angles (0°, 90°, 180°, 270°)" << std::endl;
    results.push_back(run_test_point(10.0f, 10.0f, 0.0f));           // 0°
    results.push_back(run_test_point(10.0f, 10.0f, PI / 2.0f));      // 90°
    results.push_back(run_test_point(10.0f, 10.0f, PI));             // 180°
    results.push_back(run_test_point(10.0f, 10.0f, 3.0f * PI / 2.0f)); // 270°
    
    // Test Case 8: Maximum positive
    std::cout << "  Test 8: Maximum positive (90%)" << std::endl;
    float max_safe = max_val * 0.9f;
    results.push_back(run_test_point(max_safe, max_safe, PI / 4.0f));
    
    // Test Case 9: Maximum negative
    std::cout << "  Test 9: Maximum negative (90%)" << std::endl;
    float min_safe = min_val * 0.9f;
    results.push_back(run_test_point(min_safe, min_safe, PI / 4.0f));
    
    // Test Case 10: Near saturation positive
    std::cout << "  Test 10: Near saturation positive (99%)" << std::endl;
    float near_sat_pos = max_val * 0.99f;
    results.push_back(run_test_point(near_sat_pos, near_sat_pos, PI / 6.0f));
    
    // Test Case 11: Near saturation negative
    std::cout << "  Test 11: Near saturation negative (99%)" << std::endl;
    float near_sat_neg = min_val * 0.99f;
    results.push_back(run_test_point(near_sat_neg, near_sat_neg, PI / 6.0f));
    
    // Test Case 12: Opposite signs
    std::cout << "  Test 12: Opposite signs (Vd+, Vq-)" << std::endl;
    results.push_back(run_test_point(50.0f, -50.0f, PI / 3.0f));
    
    // Test Case 13: Near LSB
    std::cout << "  Test 13: Very small values (near LSB)" << std::endl;
    float lsb = fmt_info.lsb;
    results.push_back(run_test_point(lsb * 2.0f, lsb * 3.0f, PI / 4.0f));
    
    // Test Case 14-15: Asymmetric large
    std::cout << "  Test 14-15: Asymmetric large values" << std::endl;
    results.push_back(run_test_point(max_val * 0.8f, max_val * 0.3f, 0.5f));
    results.push_back(run_test_point(100.0f, 1.0f, 1.0f));
    
    std::cout << "  Total boundary test cases: " << results.size() << std::endl;
    
    save_results_to_csv(BOUNDARY_OUTPUT_FILE, results, "Boundary and Corner Cases");
    print_summary(results, "Boundary Cases");
}

// ============================================================================
// Main
// ============================================================================

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;
    
    std::cout << "======================================================" << std::endl;
    std::cout << "  Park Inverse Transform - Phase 1 Precision Test" << std::endl;
    std::cout << "======================================================" << std::endl;
    std::cout << "\nConfiguration:" << std::endl;
    std::cout << "  Q-format (I/O): Q" << I_IO << "." << (W_IO-I_IO) 
              << " (ap_fixed<" << W_IO << "," << I_IO << ">)" << std::endl;
    std::cout << "  Q-format (sin/cos): Q" << I_SINCOS << "." << (W_SINCOS-I_SINCOS)
              << " (ap_fixed<" << W_SINCOS << "," << I_SINCOS << ">)" << std::endl;
    std::cout << "  DUT: Park_Inverse_ap_fixed (park.hpp)" << std::endl;
    std::cout << "  Golden: park_inverse_golden (models_fp/park_inverse.hpp)" << std::endl;
    std::cout << std::endl;
    
    test_random_mode();
    test_sine_mode();
    test_boundary_cases();
    
    std::cout << "\n======================================================" << std::endl;
    std::cout << "  All tests completed" << std::endl;
    std::cout << "======================================================" << std::endl;
    std::cout << "\nOutput files:" << std::endl;
    std::cout << "  - " << RANDOM_OUTPUT_FILE << std::endl;
    std::cout << "  - " << SINE_OUTPUT_FILE << std::endl;
    std::cout << "  - " << BOUNDARY_OUTPUT_FILE << std::endl;
    std::cout << "\nTest Summary:" << std::endl;
    std::cout << "  - Random mode: " << NUM_RANDOM_TESTS << " test cases" << std::endl;
    std::cout << "  - Rotating vector mode: ~200 samples" << std::endl;
    std::cout << "  - Boundary cases: 15 critical test cases" << std::endl;
    std::cout << std::endl;
    
    return 0;
}
