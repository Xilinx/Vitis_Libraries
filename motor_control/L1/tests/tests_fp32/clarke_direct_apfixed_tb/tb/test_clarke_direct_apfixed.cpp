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
 * @file test_clarke_direct.cpp
 * @brief Phase 1 Precision Test for Clarke Direct Transform
 *
 * Test Flow:
 *   1. Generate test inputs using stimulus_generator
 *   2. Run Golden Model (float32)
 *   3. Quantize inputs to ap_fixed
 *   4. Run DUT (ap_fixed)
 *   5. Dequantize DUT outputs to float
 *   6. Save point-by-point errors
 *
 * Test Modes:
 *   - Random: Random three-phase inputs
 *   - Sine: Sinusoidal three-phase inputs (50 Hz)
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
#include "hw/clarke_3p.hpp"

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
constexpr float RANDOM_MIN = -100.0f;  // Minimum random current (A)
constexpr float RANDOM_MAX = 100.0f;   // Maximum random current (A)

constexpr float SINE_SAMPLE_RATE = 10000.0f; // 10 kHz sampling
constexpr float SINE_DURATION = 0.02f;       // 20 ms (1 cycle at 50 Hz)
constexpr float SINE_AMPLITUDE = 10.0f;      // 10 A amplitude
constexpr float SINE_FREQUENCY = 50.0f;      // 50 Hz

// Output file names
const std::string RANDOM_OUTPUT_FILE = "clarke_direct_random_errors.csv";
const std::string SINE_OUTPUT_FILE = "clarke_direct_sine_errors.csv";
const std::string BOUNDARY_OUTPUT_FILE = "clarke_direct_boundary_errors.csv";

// ============================================================================
// Test Data Structure
// ============================================================================

struct TestPoint {
    // Inputs (float32, quantized)
    float ia_float;
    float ib_float;
    float ic_float;

    // Golden outputs (float32)
    float ialpha_golden;
    float ibeta_golden;
    float ihomop_golden;

    // DUT outputs (float32, dequantized from ap_fixed)
    float ialpha_dut;
    float ibeta_dut;
    float ihomop_dut;

    // Point-by-point errors
    float error_ialpha;
    float error_ibeta;
    float error_ihomop;

    TestPoint()
        : ia_float(0),
          ib_float(0),
          ic_float(0),
          ialpha_golden(0),
          ibeta_golden(0),
          ihomop_golden(0),
          ialpha_dut(0),
          ibeta_dut(0),
          ihomop_dut(0),
          error_ialpha(0),
          error_ibeta(0),
          error_ihomop(0) {}
};

// ============================================================================
// Test Functions
// ============================================================================

/**
 * @brief Run single test point through Golden and DUT
 *
 * @param ia Input phase A current (float32)
 * @param ib Input phase B current (float32)
 * @param ic Input phase C current (float32)
 * @return TestPoint Complete test results including errors
 */
TestPoint run_test_point(float ia, float ib, float ic) {
    TestPoint result;

    // Store quantized inputs (what DUT actually sees)
    result.ia_float = quantize_float<W, I>(ia);
    result.ib_float = quantize_float<W, I>(ib);
    result.ic_float = quantize_float<W, I>(ic);

    // ========================================================================
    // Step 1: Run Golden Model (float32)
    // ========================================================================
    xf::motorcontrol::golden::clarke_direct_golden(result.ialpha_golden, result.ibeta_golden, result.ihomop_golden,
                                                   result.ia_float, // Use quantized inputs for fair comparison
                                                   result.ib_float, result.ic_float);

    // ========================================================================
    // Step 2: Convert inputs to ap_fixed
    // ========================================================================
    T_FIXED ia_fixed = float_to_fixed<W, I>(ia);
    T_FIXED ib_fixed = float_to_fixed<W, I>(ib);
    T_FIXED ic_fixed = float_to_fixed<W, I>(ic);

    // ========================================================================
    // Step 3: Run DUT (ap_fixed)
    // ========================================================================
    T_FIXED ialpha_fixed, ibeta_fixed, ihomop_fixed;

    Clarke_Direct_3p_ap_fixed<T_FIXED>(ialpha_fixed, ibeta_fixed, ihomop_fixed, ia_fixed, ib_fixed, ic_fixed);

    // ========================================================================
    // Step 4: Convert DUT outputs back to float
    // ========================================================================
    result.ialpha_dut = fixed_to_float(ialpha_fixed);
    result.ibeta_dut = fixed_to_float(ibeta_fixed);
    result.ihomop_dut = fixed_to_float(ihomop_fixed);

    // ========================================================================
    // Step 5: Calculate point-by-point errors
    // ========================================================================
    result.error_ialpha = result.ialpha_dut - result.ialpha_golden;
    result.error_ibeta = result.ibeta_dut - result.ibeta_golden;
    result.error_ihomop = result.ihomop_dut - result.ihomop_golden;

    return result;
}

/**
 * @brief Save test results to CSV file
 *
 * @param filename Output CSV file name
 * @param results Vector of test results
 * @param mode Test mode description
 */
void save_results_to_csv(const std::string& filename, const std::vector<TestPoint>& results, const std::string& mode) {
    std::ofstream file(filename);

    if (!file.is_open()) {
        std::cerr << "Error: Cannot open file " << filename << std::endl;
        return;
    }

    // Write header
    file << "# Clarke Direct Transform - Phase 1 Precision Test Results\n";
    file << "# Test Mode: " << mode << "\n";
    file << "# Q-format: Q" << I << "." << (W - I) << " (ap_fixed<" << W << "," << I << ">)\n";
    file << "# Number of test points: " << results.size() << "\n";
    file << "#\n";
    file << "# Columns:\n";
    file << "#   Ia_in, Ib_in, Ic_in: Quantized inputs (float32)\n";
    file << "#   Ialpha_golden, Ibeta_golden, Ihomop_golden: Golden model outputs (float32)\n";
    file << "#   Ialpha_dut, Ibeta_dut, Ihomop_dut: DUT outputs (float32, dequantized)\n";
    file << "#   Error_Ialpha, Error_Ibeta, Error_Ihomop: Point-by-point errors (DUT - Golden)\n";
    file << "#\n";

    // Column headers
    file << "Ia_in,Ib_in,Ic_in,";
    file << "Ialpha_golden,Ibeta_golden,Ihomop_golden,";
    file << "Ialpha_dut,Ibeta_dut,Ihomop_dut,";
    file << "Error_Ialpha,Error_Ibeta,Error_Ihomop\n";

    // Write data
    file << std::fixed << std::setprecision(10);
    for (const auto& point : results) {
        file << point.ia_float << "," << point.ib_float << "," << point.ic_float << ",";

        file << point.ialpha_golden << "," << point.ibeta_golden << "," << point.ihomop_golden << ",";

        file << point.ialpha_dut << "," << point.ibeta_dut << "," << point.ihomop_dut << ",";

        file << point.error_ialpha << "," << point.error_ibeta << "," << point.error_ihomop << "\n";
    }

    file.close();
    std::cout << "Results saved to: " << filename << std::endl;
}

/**
 * @brief Print summary statistics (for user information only)
 *
 * @param results Test results
 * @param mode Test mode description
 */
void print_summary(const std::vector<TestPoint>& results, const std::string& mode) {
    if (results.empty()) return;

    std::cout << "\n========================================" << std::endl;
    std::cout << "  " << mode << " - Precision Analysis" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "Test points: " << results.size() << std::endl;
    std::cout << std::endl;

    // Extract golden and DUT outputs for analysis
    std::vector<float> golden_ialpha, golden_ibeta, golden_ihomop;
    std::vector<float> dut_ialpha, dut_ibeta, dut_ihomop;

    golden_ialpha.reserve(results.size());
    golden_ibeta.reserve(results.size());
    golden_ihomop.reserve(results.size());
    dut_ialpha.reserve(results.size());
    dut_ibeta.reserve(results.size());
    dut_ihomop.reserve(results.size());

    for (const auto& point : results) {
        golden_ialpha.push_back(point.ialpha_golden);
        golden_ibeta.push_back(point.ibeta_golden);
        golden_ihomop.push_back(point.ihomop_golden);
        dut_ialpha.push_back(point.ialpha_dut);
        dut_ibeta.push_back(point.ibeta_dut);
        dut_ihomop.push_back(point.ihomop_dut);
    }

    // ========================================================================
    // Analyze Ialpha
    // ========================================================================
    std::cout << "--- Ialpha (α-axis component) ---" << std::endl;
    auto metrics_ialpha = PrecisionAnalyzer::analyze(golden_ialpha, dut_ialpha);

    std::cout << std::scientific << std::setprecision(6);
    std::cout << "  MAE:       " << metrics_ialpha.mae << std::endl;
    std::cout << "  RMSE:      " << metrics_ialpha.rmse << std::endl;
    std::cout << "  Max Error: " << metrics_ialpha.max_error << std::endl;
    std::cout << "  SNR:       " << std::fixed << std::setprecision(2) << metrics_ialpha.snr_db << " dB" << std::endl;
    std::cout << std::endl;

    // ========================================================================
    // Analyze Ibeta
    // ========================================================================
    std::cout << "--- Ibeta (β-axis component) ---" << std::endl;
    auto metrics_ibeta = PrecisionAnalyzer::analyze(golden_ibeta, dut_ibeta);

    std::cout << std::scientific << std::setprecision(6);
    std::cout << "  MAE:       " << metrics_ibeta.mae << std::endl;
    std::cout << "  RMSE:      " << metrics_ibeta.rmse << std::endl;
    std::cout << "  Max Error: " << metrics_ibeta.max_error << std::endl;
    std::cout << "  SNR:       " << std::fixed << std::setprecision(2) << metrics_ibeta.snr_db << " dB" << std::endl;
    std::cout << std::endl;

    // ========================================================================
    // Analyze Ihomop
    // ========================================================================
    std::cout << "--- Ihomop (Zero-sequence component) ---" << std::endl;
    auto metrics_ihomop = PrecisionAnalyzer::analyze(golden_ihomop, dut_ihomop);

    std::cout << std::scientific << std::setprecision(6);
    std::cout << "  MAE:       " << metrics_ihomop.mae << std::endl;
    std::cout << "  RMSE:      " << metrics_ihomop.rmse << std::endl;
    std::cout << "  Max Error: " << metrics_ihomop.max_error << std::endl;
    std::cout << "  SNR:       " << std::fixed << std::setprecision(2) << metrics_ihomop.snr_db << " dB" << std::endl;
    std::cout << std::endl;

    // ========================================================================
    // Overall Summary
    // ========================================================================
    std::cout << "--- Overall Summary ---" << std::endl;
    float max_mae = std::max({metrics_ialpha.mae, metrics_ibeta.mae, metrics_ihomop.mae});
    float max_rmse = std::max({metrics_ialpha.rmse, metrics_ibeta.rmse, metrics_ihomop.rmse});
    float max_error = std::max({metrics_ialpha.max_error, metrics_ibeta.max_error, metrics_ihomop.max_error});
    float min_snr = std::min({metrics_ialpha.snr_db, metrics_ibeta.snr_db, metrics_ihomop.snr_db});

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
 * @brief Test with random three-phase inputs
 */
void test_random_mode() {
    std::cout << "\n=== Random Input Test ===" << std::endl;
    std::cout << "Generating " << NUM_RANDOM_TESTS << " random test cases..." << std::endl;

    StimulusGenerator gen(12345); // Fixed seed for reproducibility
    std::vector<TestPoint> results;
    results.reserve(NUM_RANDOM_TESTS);

    for (int i = 0; i < NUM_RANDOM_TESTS; i++) {
        // Generate random three-phase input (balanced)
        auto input = gen.random_three_phase(RANDOM_MIN, RANDOM_MAX);

        // Run test
        TestPoint result = run_test_point(input.ia, input.ib, input.ic);
        results.push_back(result);

        // Progress indicator
        if ((i + 1) % 100 == 0) {
            std::cout << "  Progress: " << (i + 1) << "/" << NUM_RANDOM_TESTS << std::endl;
        }
    }

    // Save results
    save_results_to_csv(RANDOM_OUTPUT_FILE, results, "Random Three-Phase Inputs");

    // Print summary
    print_summary(results, "Random Mode");
}

/**
 * @brief Test with sinusoidal three-phase inputs
 */
void test_sine_mode() {
    std::cout << "\n=== Sinusoidal Input Test ===" << std::endl;
    std::cout << "Generating sinusoidal test cases..." << std::endl;
    std::cout << "  Frequency: " << SINE_FREQUENCY << " Hz" << std::endl;
    std::cout << "  Amplitude: " << SINE_AMPLITUDE << " A" << std::endl;
    std::cout << "  Duration: " << SINE_DURATION << " s" << std::endl;
    std::cout << "  Sample rate: " << SINE_SAMPLE_RATE << " Hz" << std::endl;

    // Generate three-phase sinusoidal inputs
    auto inputs =
        StimulusGenerator::three_phase_sine_array(SINE_SAMPLE_RATE, SINE_DURATION, SINE_AMPLITUDE, SINE_FREQUENCY);

    std::cout << "  Generated " << inputs.size() << " samples" << std::endl;

    std::vector<TestPoint> results;
    results.reserve(inputs.size());

    for (size_t i = 0; i < inputs.size(); i++) {
        // Run test
        TestPoint result = run_test_point(inputs[i].ia, inputs[i].ib, inputs[i].ic);
        results.push_back(result);

        // Progress indicator
        if ((i + 1) % 50 == 0) {
            std::cout << "  Progress: " << (i + 1) << "/" << inputs.size() << std::endl;
        }
    }

    // Save results
    save_results_to_csv(SINE_OUTPUT_FILE, results, "Sinusoidal Three-Phase Inputs (50 Hz)");

    // Print summary
    print_summary(results, "Sine Mode");
}

// ============================================================================
// Boundary Test Cases
// ============================================================================

/**
 * @brief Boundary and corner case tests
 *
 * Tests critical boundary conditions:
 * - Maximum/minimum input values
 * - Near-saturation cases
 * - Unbalanced three-phase
 * - Zero inputs
 * - Single-phase non-zero
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

    // ========================================================================
    // Test Case 1: All Zero Inputs
    // ========================================================================
    std::cout << "  Test 1: All zero inputs" << std::endl;
    results.push_back(run_test_point(0.0f, 0.0f, 0.0f));

    // ========================================================================
    // Test Case 2: Maximum Positive Balanced
    // ========================================================================
    std::cout << "  Test 2: Maximum positive (balanced)" << std::endl;
    float max_balanced = max_val * 0.9f; // 90% of max to avoid saturation
    float ia_max = max_balanced;
    float ib_max = -max_balanced / 2.0f;
    float ic_max = -max_balanced / 2.0f;
    results.push_back(run_test_point(ia_max, ib_max, ic_max));

    // ========================================================================
    // Test Case 3: Maximum Negative Balanced
    // ========================================================================
    std::cout << "  Test 3: Maximum negative (balanced)" << std::endl;
    float ia_min = min_val * 0.9f;
    float ib_min = -ia_min / 2.0f;
    float ic_min = -ia_min / 2.0f;
    results.push_back(run_test_point(ia_min, ib_min, ic_min));

    // ========================================================================
    // Test Case 4: Near Saturation (Positive)
    // ========================================================================
    std::cout << "  Test 4: Near saturation (positive, 99% of max)" << std::endl;
    float near_sat_pos = max_val * 0.99f;
    float ia_sat_pos = near_sat_pos;
    float ib_sat_pos = -near_sat_pos / 2.0f;
    float ic_sat_pos = -near_sat_pos / 2.0f;
    results.push_back(run_test_point(ia_sat_pos, ib_sat_pos, ic_sat_pos));

    // ========================================================================
    // Test Case 5: Near Saturation (Negative)
    // ========================================================================
    std::cout << "  Test 5: Near saturation (negative, 99% of min)" << std::endl;
    float near_sat_neg = min_val * 0.99f;
    float ia_sat_neg = near_sat_neg;
    float ib_sat_neg = -near_sat_neg / 2.0f;
    float ic_sat_neg = -near_sat_neg / 2.0f;
    results.push_back(run_test_point(ia_sat_neg, ib_sat_neg, ic_sat_neg));

    // ========================================================================
    // Test Case 6: Unbalanced - Slight Imbalance
    // ========================================================================
    std::cout << "  Test 6: Slight three-phase imbalance (1% error)" << std::endl;
    float ia_unbal1 = 10.0f;
    float ib_unbal1 = -5.0f;
    float ic_unbal1 = -4.9f; // Sum = 0.1 (1% imbalance)
    results.push_back(run_test_point(ia_unbal1, ib_unbal1, ic_unbal1));

    // ========================================================================
    // Test Case 7: Unbalanced - Moderate Imbalance
    // ========================================================================
    std::cout << "  Test 7: Moderate three-phase imbalance (10% error)" << std::endl;
    float ia_unbal2 = 10.0f;
    float ib_unbal2 = -5.0f;
    float ic_unbal2 = -4.0f; // Sum = 1.0 (10% imbalance)
    results.push_back(run_test_point(ia_unbal2, ib_unbal2, ic_unbal2));

    // ========================================================================
    // Test Case 8: Unbalanced - Large Imbalance
    // ========================================================================
    std::cout << "  Test 8: Large three-phase imbalance (50% error)" << std::endl;
    float ia_unbal3 = 10.0f;
    float ib_unbal3 = -5.0f;
    float ic_unbal3 = 0.0f; // Sum = 5.0 (50% imbalance)
    results.push_back(run_test_point(ia_unbal3, ib_unbal3, ic_unbal3));

    // ========================================================================
    // Test Case 9: Single Phase A Non-Zero
    // ========================================================================
    std::cout << "  Test 9: Only Phase A non-zero" << std::endl;
    results.push_back(run_test_point(10.0f, 0.0f, 0.0f));

    // ========================================================================
    // Test Case 10: Single Phase B Non-Zero
    // ========================================================================
    std::cout << "  Test 10: Only Phase B non-zero" << std::endl;
    results.push_back(run_test_point(0.0f, 10.0f, 0.0f));

    // ========================================================================
    // Test Case 11: Single Phase C Non-Zero
    // ========================================================================
    std::cout << "  Test 11: Only Phase C non-zero" << std::endl;
    results.push_back(run_test_point(0.0f, 0.0f, 10.0f));

    // ========================================================================
    // Test Case 12: Very Small Values (near LSB)
    // ========================================================================
    std::cout << "  Test 12: Very small values (near LSB)" << std::endl;
    float lsb = fmt_info.lsb;
    float ia_tiny = lsb * 2.0f;
    float ib_tiny = -lsb;
    float ic_tiny = -lsb;
    results.push_back(run_test_point(ia_tiny, ib_tiny, ic_tiny));

    // ========================================================================
    // Test Case 13: Asymmetric Large Values
    // ========================================================================
    std::cout << "  Test 13: Asymmetric large values" << std::endl;
    float ia_asym = max_val * 0.8f;
    float ib_asym = max_val * 0.3f;
    float ic_asym = -(ia_asym + ib_asym); // Balanced
    results.push_back(run_test_point(ia_asym, ib_asym, ic_asym));

    // ========================================================================
    // Test Case 14: All Same Value (Unbalanced)
    // ========================================================================
    std::cout << "  Test 14: All phases same value (highly unbalanced)" << std::endl;
    results.push_back(run_test_point(5.0f, 5.0f, 5.0f));

    // ========================================================================
    // Test Case 15: Mixed Sign Pattern
    // ========================================================================
    std::cout << "  Test 15: Mixed sign pattern" << std::endl;
    results.push_back(run_test_point(15.0f, -20.0f, 5.0f)); // Balanced

    std::cout << "  Total boundary test cases: " << results.size() << std::endl;

    // Save results
    save_results_to_csv(BOUNDARY_OUTPUT_FILE, results, "Boundary and Corner Cases");

    // Print summary
    print_summary(results, "Boundary Cases");

    // ========================================================================
    // Additional Analysis for Boundary Cases
    // ========================================================================
    std::cout << "\n--- Detailed Boundary Case Analysis ---" << std::endl;

    struct BoundaryCase {
        int case_num;
        std::string description;
        float ia, ib, ic;
        float imbalance; // Ia + Ib + Ic
    };

    std::vector<BoundaryCase> cases = {
        {1, "All zero", 0.0f, 0.0f, 0.0f, 0.0f},
        {2, "Max positive balanced", ia_max, ib_max, ic_max, ia_max + ib_max + ic_max},
        {3, "Max negative balanced", ia_min, ib_min, ic_min, ia_min + ib_min + ic_min},
        {4, "Near sat positive", ia_sat_pos, ib_sat_pos, ic_sat_pos, ia_sat_pos + ib_sat_pos + ic_sat_pos},
        {5, "Near sat negative", ia_sat_neg, ib_sat_neg, ic_sat_neg, ia_sat_neg + ib_sat_neg + ic_sat_neg},
        {6, "Slight imbalance", ia_unbal1, ib_unbal1, ic_unbal1, ia_unbal1 + ib_unbal1 + ic_unbal1},
        {7, "Moderate imbalance", ia_unbal2, ib_unbal2, ic_unbal2, ia_unbal2 + ib_unbal2 + ic_unbal2},
        {8, "Large imbalance", ia_unbal3, ib_unbal3, ic_unbal3, ia_unbal3 + ib_unbal3 + ic_unbal3},
        {9, "Only phase A", 10.0f, 0.0f, 0.0f, 10.0f},
        {10, "Only phase B", 0.0f, 10.0f, 0.0f, 10.0f},
        {11, "Only phase C", 0.0f, 0.0f, 10.0f, 10.0f},
        {12, "Near LSB", ia_tiny, ib_tiny, ic_tiny, ia_tiny + ib_tiny + ic_tiny},
        {13, "Asymmetric large", ia_asym, ib_asym, ic_asym, ia_asym + ib_asym + ic_asym},
        {14, "All same", 5.0f, 5.0f, 5.0f, 15.0f},
        {15, "Mixed sign", 15.0f, -20.0f, 5.0f, 0.0f}};

    std::cout << std::fixed << std::setprecision(6);
    for (size_t i = 0; i < cases.size() && i < results.size(); i++) {
        const auto& c = cases[i];
        const auto& r = results[i];

        std::cout << "\nCase " << c.case_num << ": " << c.description << std::endl;
        std::cout << "  Input: Ia=" << c.ia << ", Ib=" << c.ib << ", Ic=" << c.ic << std::endl;
        std::cout << "  Imbalance (Ia+Ib+Ic): " << c.imbalance << std::endl;
        std::cout << "  Ihomop (should reflect imbalance): " << r.ihomop_golden << std::endl;
        std::cout << "  Max error: " << std::scientific
                  << std::max({std::abs(r.error_ialpha), std::abs(r.error_ibeta), std::abs(r.error_ihomop)})
                  << std::endl;
    }

    std::cout << "\n========================================\n" << std::endl;
}

// ============================================================================
// Main
// ============================================================================

int main(int argc, char* argv[]) {
    std::cout << "======================================================" << std::endl;
    std::cout << "  Clarke Direct Transform - Phase 1 Precision Test" << std::endl;
    std::cout << "======================================================" << std::endl;
    std::cout << "\nConfiguration:" << std::endl;
    std::cout << "  Q-format: Q" << I << "." << (W - I) << " (ap_fixed<" << W << "," << I << ">)" << std::endl;
    std::cout << "  DUT: Clarke_Direct_3p_ap_fixed (clarke_3p.hpp)" << std::endl;
    std::cout << "  Golden: clarke_direct_golden<float> (models_fp/clarke_direct.hpp)" << std::endl;
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
    std::cout << "  - Sine mode: ~200 samples (1 cycle @ 50 Hz)" << std::endl;
    std::cout << "  - Boundary cases: 15 critical test cases" << std::endl;
    std::cout << "\nNote: Point-by-point errors saved in CSV files." << std::endl;
    std::cout << "      Precision analysis displayed in console output." << std::endl;
    std::cout << std::endl;

    return 0;
}
