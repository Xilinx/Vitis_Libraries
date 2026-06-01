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
 * @file test_clarke_inverse.cpp
 * @brief Phase 1 Precision Test for Clarke Inverse Transform
 *
 * Test Flow:
 *   1. Generate test inputs using stimulus_generator
 *   2. Run Golden Model (float32)
 *   3. Quantize inputs to ap_fixed
 *   4. Run DUT (ap_fixed)
 *   5. Dequantize DUT outputs to float
 *   6. Save point-by-point errors
 *   7. Analyze precision with precision_analyzer
 *
 * Test Modes:
 *   - Random: Random α-β inputs
 *   - Sine: Rotating vector (circular trajectory in α-β plane)
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
#include "hw/clarke_2p.hpp"

// Phase 1 validation utilities
#include "stimulus_generator.hpp"
#include "quantization_utils.hpp"
#include "precision_analyzer.hpp"
#include "compatibility.hpp"

using namespace xf::motorcontrol::test;

// ============================================================================
// Test Configuration
// ============================================================================

// Q-format configuration (matching typical motor control applications)
constexpr int W = 32; // Total width
constexpr int I = 16; // Integer bits (including sign)
// Fractional bits = W - I = 16
// Format: Q16.15

typedef ap_fixed<W, I> T_FIXED;

// Test parameters
constexpr int NUM_RANDOM_TESTS = 1000; // Number of random test cases
constexpr float RANDOM_MIN = -100.0f;  // Minimum random value
constexpr float RANDOM_MAX = 100.0f;   // Maximum random value

constexpr float SINE_SAMPLE_RATE = 10000.0f; // 10 kHz sampling
constexpr float SINE_DURATION = 0.02f;       // 20 ms (1 cycle at 50 Hz)
constexpr float SINE_AMPLITUDE = 10.0f;      // 10 A/V amplitude
constexpr float SINE_FREQUENCY = 50.0f;      // 50 Hz (rotating vector)

// Output file names
const std::string RANDOM_OUTPUT_FILE = "clarke_inverse_random_errors.csv";
const std::string SINE_OUTPUT_FILE = "clarke_inverse_sine_errors.csv";
const std::string BOUNDARY_OUTPUT_FILE = "clarke_inverse_boundary_errors.csv";

// ============================================================================
// Test Data Structure
// ============================================================================

struct TestPoint {
    // Inputs (float32, quantized)
    float valpha_float;
    float vbeta_float;

    // Golden outputs (float32)
    float va_golden;
    float vb_golden;
    float vc_golden;

    // DUT outputs (float32, dequantized from ap_fixed)
    float va_dut;
    float vb_dut;
    float vc_dut;

    // Point-by-point errors
    float error_va;
    float error_vb;
    float error_vc;

    // Balance check
    float sum_golden; // Va + Vb + Vc (should be ~0)
    float sum_dut;

    TestPoint()
        : valpha_float(0),
          vbeta_float(0),
          va_golden(0),
          vb_golden(0),
          vc_golden(0),
          va_dut(0),
          vb_dut(0),
          vc_dut(0),
          error_va(0),
          error_vb(0),
          error_vc(0),
          sum_golden(0),
          sum_dut(0) {}
};

// ============================================================================
// Test Functions
// ============================================================================

/**
 * @brief Run single test point through Golden and DUT
 *
 * @param valpha Input α-axis component (float32)
 * @param vbeta Input β-axis component (float32)
 * @return TestPoint Complete test results including errors
 */
TestPoint run_test_point(float valpha, float vbeta) {
    TestPoint result;

    // Store quantized inputs (what DUT actually sees)
    result.valpha_float = quantize_float<W, I>(valpha);
    result.vbeta_float = quantize_float<W, I>(vbeta);

    // ========================================================================
    // Step 1: Run Golden Model (float32)
    // ========================================================================
    xf::motorcontrol::golden::clarke_inverse_golden(result.va_golden, result.vb_golden, result.vc_golden,
                                                    result.valpha_float, // Use quantized inputs for fair comparison
                                                    result.vbeta_float);

    result.sum_golden = result.va_golden + result.vb_golden + result.vc_golden;

    // ========================================================================
    // Step 2: Convert inputs to ap_fixed
    // ========================================================================
    T_FIXED valpha_fixed = float_to_fixed<W, I>(valpha);
    T_FIXED vbeta_fixed = float_to_fixed<W, I>(vbeta);

    // ========================================================================
    // Step 3: Run DUT (ap_fixed)
    // ========================================================================
    T_FIXED va_fixed, vb_fixed, vc_fixed;

    Clarke_Inverse_2p_ap_fixed<T_FIXED>(va_fixed, vb_fixed, vc_fixed, valpha_fixed, vbeta_fixed);

    // ========================================================================
    // Step 4: Convert DUT outputs back to float
    // ========================================================================
    result.va_dut = fixed_to_float(va_fixed);
    result.vb_dut = fixed_to_float(vb_fixed);
    result.vc_dut = fixed_to_float(vc_fixed);

    result.sum_dut = result.va_dut + result.vb_dut + result.vc_dut;

    // ========================================================================
    // Step 5: Calculate point-by-point errors
    // ========================================================================
    result.error_va = result.va_dut - result.va_golden;
    result.error_vb = result.vb_dut - result.vb_golden;
    result.error_vc = result.vc_dut - result.vc_golden;

    return result;
}

/**
 * @brief Save test results to CSV file
 */
void save_results_to_csv(const std::string& filename, const std::vector<TestPoint>& results, const std::string& mode) {
    std::ofstream file(filename);

    if (!file.is_open()) {
        std::cerr << "Error: Cannot open file " << filename << std::endl;
        return;
    }

    // Write header
    file << "# Clarke Inverse Transform - Phase 1 Precision Test Results\n";
    file << "# Test Mode: " << mode << "\n";
    file << "# Q-format: Q" << I << "." << (W - I) << " (ap_fixed<" << W << "," << I << ">)\n";
    file << "# Number of test points: " << results.size() << "\n";
    file << "#\n";
    file << "# Columns:\n";
    file << "#   Valpha_in, Vbeta_in: Quantized inputs (float32)\n";
    file << "#   Va_golden, Vb_golden, Vc_golden: Golden model outputs\n";
    file << "#   Va_dut, Vb_dut, Vc_dut: DUT outputs (dequantized)\n";
    file << "#   Error_Va, Error_Vb, Error_Vc: Point-by-point errors (DUT - Golden)\n";
    file << "#   Sum_golden, Sum_dut: Va+Vb+Vc (should be ~0 for balanced)\n";
    file << "#\n";

    // Column headers
    file << "Valpha_in,Vbeta_in,";
    file << "Va_golden,Vb_golden,Vc_golden,";
    file << "Va_dut,Vb_dut,Vc_dut,";
    file << "Error_Va,Error_Vb,Error_Vc,";
    file << "Sum_golden,Sum_dut\n";

    // Write data
    file << std::fixed << std::setprecision(10);
    for (const auto& point : results) {
        file << point.valpha_float << "," << point.vbeta_float << ",";
        file << point.va_golden << "," << point.vb_golden << "," << point.vc_golden << ",";
        file << point.va_dut << "," << point.vb_dut << "," << point.vc_dut << ",";
        file << point.error_va << "," << point.error_vb << "," << point.error_vc << ",";
        file << point.sum_golden << "," << point.sum_dut << "\n";
    }

    file.close();
    std::cout << "Results saved to: " << filename << std::endl;
}

/**
 * @brief Print precision analysis summary
 */
void print_summary(const std::vector<TestPoint>& results, const std::string& mode) {
    if (results.empty()) return;

    std::cout << "\n========================================" << std::endl;
    std::cout << "  " << mode << " - Precision Analysis" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "Test points: " << results.size() << std::endl;
    std::cout << std::endl;

    // Extract golden and DUT outputs for analysis
    std::vector<float> golden_va, golden_vb, golden_vc;
    std::vector<float> dut_va, dut_vb, dut_vc;

    golden_va.reserve(results.size());
    golden_vb.reserve(results.size());
    golden_vc.reserve(results.size());
    dut_va.reserve(results.size());
    dut_vb.reserve(results.size());
    dut_vc.reserve(results.size());

    for (const auto& point : results) {
        golden_va.push_back(point.va_golden);
        golden_vb.push_back(point.vb_golden);
        golden_vc.push_back(point.vc_golden);
        dut_va.push_back(point.va_dut);
        dut_vb.push_back(point.vb_dut);
        dut_vc.push_back(point.vc_dut);
    }

    // Analyze Va
    std::cout << "--- Va (Phase A) ---" << std::endl;
    auto metrics_va = PrecisionAnalyzer::analyze(golden_va, dut_va);
    std::cout << std::scientific << std::setprecision(6);
    std::cout << "  MAE:       " << metrics_va.mae << std::endl;
    std::cout << "  RMSE:      " << metrics_va.rmse << std::endl;
    std::cout << "  Max Error: " << metrics_va.max_error << std::endl;
    std::cout << "  SNR:       " << std::fixed << std::setprecision(2) << metrics_va.snr_db << " dB" << std::endl;
    std::cout << std::endl;

    // Analyze Vb
    std::cout << "--- Vb (Phase B) ---" << std::endl;
    auto metrics_vb = PrecisionAnalyzer::analyze(golden_vb, dut_vb);
    std::cout << std::scientific << std::setprecision(6);
    std::cout << "  MAE:       " << metrics_vb.mae << std::endl;
    std::cout << "  RMSE:      " << metrics_vb.rmse << std::endl;
    std::cout << "  Max Error: " << metrics_vb.max_error << std::endl;
    std::cout << "  SNR:       " << std::fixed << std::setprecision(2) << metrics_vb.snr_db << " dB" << std::endl;
    std::cout << std::endl;

    // Analyze Vc
    std::cout << "--- Vc (Phase C) ---" << std::endl;
    auto metrics_vc = PrecisionAnalyzer::analyze(golden_vc, dut_vc);
    std::cout << std::scientific << std::setprecision(6);
    std::cout << "  MAE:       " << metrics_vc.mae << std::endl;
    std::cout << "  RMSE:      " << metrics_vc.rmse << std::endl;
    std::cout << "  Max Error: " << metrics_vc.max_error << std::endl;
    std::cout << "  SNR:       " << std::fixed << std::setprecision(2) << metrics_vc.snr_db << " dB" << std::endl;
    std::cout << std::endl;

    // Overall Summary
    std::cout << "--- Overall Summary ---" << std::endl;
    float max_mae = std::max({metrics_va.mae, metrics_vb.mae, metrics_vc.mae});
    float max_rmse = std::max({metrics_va.rmse, metrics_vb.rmse, metrics_vc.rmse});
    float max_error = std::max({metrics_va.max_error, metrics_vb.max_error, metrics_vc.max_error});
    float min_snr = std::min({metrics_va.snr_db, metrics_vb.snr_db, metrics_vc.snr_db});

    std::cout << std::scientific << std::setprecision(6);
    std::cout << "  Worst MAE:       " << max_mae << std::endl;
    std::cout << "  Worst RMSE:      " << max_rmse << std::endl;
    std::cout << "  Worst Max Error: " << max_error << std::endl;
    std::cout << "  Worst SNR:       " << std::fixed << std::setprecision(2) << min_snr << " dB" << std::endl;

    std::cout << "========================================\n" << std::endl;
}

// ============================================================================
// Test Mode Implementations
// ============================================================================

/**
 * @brief Test with random α-β inputs
 */
void test_random_mode() {
    std::cout << "\n=== Random Input Test ===" << std::endl;
    std::cout << "Generating " << NUM_RANDOM_TESTS << " random test cases..." << std::endl;

    StimulusGenerator gen(12345); // Fixed seed for reproducibility
    std::vector<TestPoint> results;
    results.reserve(NUM_RANDOM_TESTS);

    for (int i = 0; i < NUM_RANDOM_TESTS; i++) {
        // Generate random α-β inputs
        float valpha = gen.random_uniform(RANDOM_MIN, RANDOM_MAX);
        float vbeta = gen.random_uniform(RANDOM_MIN, RANDOM_MAX);

        // Run test
        TestPoint result = run_test_point(valpha, vbeta);
        results.push_back(result);

        // Progress indicator
        if ((i + 1) % 100 == 0) {
            std::cout << "  Progress: " << (i + 1) << "/" << NUM_RANDOM_TESTS << std::endl;
        }
    }

    // Save results
    save_results_to_csv(RANDOM_OUTPUT_FILE, results, "Random Alpha-Beta Inputs");

    // Print summary
    print_summary(results, "Random Mode");
}

/**
 * @brief Test with rotating vector (circular trajectory in α-β plane)
 */
void test_sine_mode() {
    std::cout << "\n=== Rotating Vector Test ===" << std::endl;
    std::cout << "Generating rotating vector test cases..." << std::endl;
    std::cout << "  Frequency: " << SINE_FREQUENCY << " Hz (rotation rate)" << std::endl;
    std::cout << "  Amplitude: " << SINE_AMPLITUDE << " (radius)" << std::endl;
    std::cout << "  Duration: " << SINE_DURATION << " s" << std::endl;
    std::cout << "  Sample rate: " << SINE_SAMPLE_RATE << " Hz" << std::endl;

    size_t num_samples = static_cast<size_t>(SINE_SAMPLE_RATE * SINE_DURATION);
    std::vector<TestPoint> results;
    results.reserve(num_samples);

    float dt = 1.0f / SINE_SAMPLE_RATE;
    float omega = 2.0f * PI * SINE_FREQUENCY;

    for (size_t i = 0; i < num_samples; i++) {
        float t = i * dt;

        // Rotating vector: Valpha = R*cos(ωt), Vbeta = R*sin(ωt)
        float valpha = SINE_AMPLITUDE * std::cos(omega * t);
        float vbeta = SINE_AMPLITUDE * std::sin(omega * t);

        // Run test
        TestPoint result = run_test_point(valpha, vbeta);
        results.push_back(result);

        // Progress indicator
        if ((i + 1) % 50 == 0) {
            std::cout << "  Progress: " << (i + 1) << "/" << num_samples << std::endl;
        }
    }

    std::cout << "  Generated " << num_samples << " samples" << std::endl;

    // Save results
    save_results_to_csv(SINE_OUTPUT_FILE, results, "Rotating Vector (50 Hz circular trajectory)");

    // Print summary
    print_summary(results, "Rotating Vector Mode");
}

/**
 * @brief Boundary and corner case tests
 */
void test_boundary_cases() {
    std::cout << "\n=== Boundary & Corner Case Test ===" << std::endl;
    std::cout << "Testing critical boundary conditions..." << std::endl;

    std::vector<TestPoint> results;

    // Get Q-format information
    auto fmt_info = FixedPointInfo::create<W, I>();
    float max_val = fmt_info.max_value;
    float min_val = fmt_info.min_value;

    std::cout << "  Q-format range: [" << min_val << ", " << max_val << "]" << std::endl;
    std::cout << std::endl;

    // Test Case 1: All Zero
    std::cout << "  Test 1: All zero inputs" << std::endl;
    results.push_back(run_test_point(0.0f, 0.0f));

    // Test Case 2: Alpha only
    std::cout << "  Test 2: Only α non-zero" << std::endl;
    results.push_back(run_test_point(10.0f, 0.0f));

    // Test Case 3: Beta only
    std::cout << "  Test 3: Only β non-zero" << std::endl;
    results.push_back(run_test_point(0.0f, 10.0f));

    // Test Case 4: Maximum positive
    std::cout << "  Test 4: Maximum positive (90%)" << std::endl;
    float max_safe = max_val * 0.9f;
    results.push_back(run_test_point(max_safe, max_safe));

    // Test Case 5: Maximum negative
    std::cout << "  Test 5: Maximum negative (90%)" << std::endl;
    float min_safe = min_val * 0.9f;
    results.push_back(run_test_point(min_safe, min_safe));

    // Test Case 6: Near saturation positive
    std::cout << "  Test 6: Near saturation positive (99%)" << std::endl;
    float near_sat_pos = max_val * 0.99f;
    results.push_back(run_test_point(near_sat_pos, near_sat_pos));

    // Test Case 7: Near saturation negative
    std::cout << "  Test 7: Near saturation negative (99%)" << std::endl;
    float near_sat_neg = min_val * 0.99f;
    results.push_back(run_test_point(near_sat_neg, near_sat_neg));

    // Test Case 8: Asymmetric large
    std::cout << "  Test 8: Asymmetric large values" << std::endl;
    results.push_back(run_test_point(max_val * 0.8f, max_val * 0.3f));

    // Test Case 9: Opposite signs
    std::cout << "  Test 9: Opposite signs" << std::endl;
    results.push_back(run_test_point(50.0f, -50.0f));

    // Test Case 10: Near LSB
    std::cout << "  Test 10: Very small values (near LSB)" << std::endl;
    float lsb = fmt_info.lsb;
    results.push_back(run_test_point(lsb * 2.0f, lsb * 3.0f));

    // Test Case 11-14: Quadrant tests
    std::cout << "  Test 11: Quadrant I (+α, +β)" << std::endl;
    results.push_back(run_test_point(10.0f, 10.0f));

    std::cout << "  Test 12: Quadrant II (-α, +β)" << std::endl;
    results.push_back(run_test_point(-10.0f, 10.0f));

    std::cout << "  Test 13: Quadrant III (-α, -β)" << std::endl;
    results.push_back(run_test_point(-10.0f, -10.0f));

    std::cout << "  Test 14: Quadrant IV (+α, -β)" << std::endl;
    results.push_back(run_test_point(10.0f, -10.0f));

    // Test Case 15: Mixed magnitude
    std::cout << "  Test 15: Mixed magnitude" << std::endl;
    results.push_back(run_test_point(100.0f, 1.0f));

    std::cout << "  Total boundary test cases: " << results.size() << std::endl;

    // Save results
    save_results_to_csv(BOUNDARY_OUTPUT_FILE, results, "Boundary and Corner Cases");

    // Print summary
    print_summary(results, "Boundary Cases");

    // Detailed analysis
    std::cout << "\n--- Balance Check (Va + Vb + Vc should ≈ 0) ---" << std::endl;
    std::cout << std::fixed << std::setprecision(8);
    for (size_t i = 0; i < results.size(); i++) {
        const auto& r = results[i];
        std::cout << "Case " << (i + 1) << ": Sum_golden=" << r.sum_golden << ", Sum_dut=" << r.sum_dut << std::endl;
    }
    std::cout << "========================================\n" << std::endl;
}

// ============================================================================
// Main
// ============================================================================

int main(int argc, char* argv[]) {
    std::cout << "======================================================" << std::endl;
    std::cout << "  Clarke Inverse Transform - Phase 1 Precision Test" << std::endl;
    std::cout << "======================================================" << std::endl;
    std::cout << "\nConfiguration:" << std::endl;
    std::cout << "  Q-format: Q" << I << "." << (W - I) << " (ap_fixed<" << W << "," << I << ">)" << std::endl;
    std::cout << "  DUT: Clarke_Inverse_2p_ap_fixed (clarke_2p.hpp)" << std::endl;
    std::cout << "  Golden: clarke_inverse_golden (models_fp/clarke_inverse.hpp)" << std::endl;
    std::cout << std::endl;

    // Run all test modes
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
    std::cout << "  - Rotating vector mode: ~200 samples (1 rotation @ 50 Hz)" << std::endl;
    std::cout << "  - Boundary cases: 15 critical test cases" << std::endl;
    std::cout << "\nNote: Point-by-point errors saved in CSV files." << std::endl;
    std::cout << "      Precision analysis displayed in console output." << std::endl;
    std::cout << std::endl;

    return 0;
}
