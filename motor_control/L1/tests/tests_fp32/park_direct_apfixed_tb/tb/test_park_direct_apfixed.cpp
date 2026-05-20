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
 * @file test_park_direct.cpp
 * @brief Phase 1 Precision Test for Park Direct Transform
 * 
 * Test Flow:
 *   1. Generate test inputs (α-β components + angle θ) using stimulus_generator
 *   2. Run Golden Model (float32)
 *   3. Quantize inputs to ap_fixed
 *   4. Run DUT (ap_fixed)
 *   5. Dequantize DUT outputs to float
 *   6. Calculate point-by-point errors
 *   7. Analyze precision with precision_analyzer
 * 
 * Test Modes:
 *   - Random: Random α-β values with random angles
 *   - Sine: Rotating α-β vector (circular trajectory) with linearly increasing angle
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

// Q-format configuration for I/O (current values)
constexpr int W_IO = 32;    // Total width
constexpr int I_IO = 16;    // Integer bits (including sign)
typedef ap_fixed<W_IO, I_IO> T_IO;

// Q-format configuration for sin/cos (range [-1, 1])
constexpr int W_SINCOS = 32;  // Total width
constexpr int I_SINCOS = 2;   // Integer bits: 1 for sign, 1 for range
typedef ap_fixed<W_SINCOS, I_SINCOS> T_SINCOS;

// Test parameters
constexpr int NUM_RANDOM_TESTS = 1000;     // Number of random test cases
constexpr float RANDOM_MIN = -100.0f;      // Minimum random current value
constexpr float RANDOM_MAX = 100.0f;       // Maximum random current value

constexpr float SINE_SAMPLE_RATE = 10000.0f;  // 10 kHz sampling
constexpr float SINE_DURATION = 0.02f;         // 20 ms
constexpr float SINE_AMPLITUDE = 10.0f;        // 10 A amplitude
constexpr float SINE_FREQUENCY = 50.0f;        // 50 Hz electrical frequency
constexpr float ROTOR_SPEED_HZ = 50.0f;        // Rotor rotates at 50 Hz

// Output file names
const std::string RANDOM_OUTPUT_FILE = "park_direct_random_errors.csv";
const std::string SINE_OUTPUT_FILE = "park_direct_sine_errors.csv";

// ============================================================================
// Test Data Structure
// ============================================================================

struct TestPoint {
    // Inputs (float32, quantized)
    float ialpha_float;
    float ibeta_float;
    float theta_float;      // Angle in radians
    float cos_theta_float;
    float sin_theta_float;
    
    // Golden outputs (float32)
    float id_golden;
    float iq_golden;
    
    // DUT outputs (float32, dequantized from ap_fixed)
    float id_dut;
    float iq_dut;
    
    // Point-by-point errors
    float error_id;
    float error_iq;
    
    // Magnitude check (should be preserved)
    float mag_input;   // sqrt(Ialpha² + Ibeta²)
    float mag_golden;  // sqrt(Id² + Iq²)
    float mag_dut;
    
    TestPoint() : ialpha_float(0), ibeta_float(0), theta_float(0),
                  cos_theta_float(0), sin_theta_float(0),
                  id_golden(0), iq_golden(0),
                  id_dut(0), iq_dut(0),
                  error_id(0), error_iq(0),
                  mag_input(0), mag_golden(0), mag_dut(0) {}
};

// ============================================================================
// Helper Functions
// ============================================================================

/**
 * @brief Calculate magnitude
 */
inline float magnitude(float a, float b) {
    return std::sqrt(a * a + b * b);
}

/**
 * @brief Run single test point through Golden and DUT
 * 
 * @param ialpha Input α-axis component (float32)
 * @param ibeta Input β-axis component (float32)
 * @param theta Input angle in radians (float32)
 * @return TestPoint Complete test results including errors
 */
TestPoint run_test_point(float ialpha, float ibeta, float theta) {
    TestPoint result;
    
    // Calculate sin/cos
    float cos_theta = std::cos(theta);
    float sin_theta = std::sin(theta);
    
    // Store quantized inputs (what DUT actually sees)
    result.ialpha_float = quantize_float<W_IO, I_IO>(ialpha);
    result.ibeta_float = quantize_float<W_IO, I_IO>(ibeta);
    result.theta_float = theta;  // Keep original for reference
    result.cos_theta_float = quantize_float<W_SINCOS, I_SINCOS>(cos_theta);
    result.sin_theta_float = quantize_float<W_SINCOS, I_SINCOS>(sin_theta);
    
    // Calculate input magnitude
    result.mag_input = magnitude(result.ialpha_float, result.ibeta_float);
    
    // ========================================================================
    // Step 1: Run Golden Model (float32)
    // ========================================================================
    xf::motorcontrol::golden::park_direct_golden(
        result.id_golden,
        result.iq_golden,
        result.ialpha_float,  // Use quantized inputs for fair comparison
        result.ibeta_float,
        result.cos_theta_float,
        result.sin_theta_float
    );
    
    result.mag_golden = magnitude(result.id_golden, result.iq_golden);
    
    // ========================================================================
    // Step 2: Convert inputs to ap_fixed
    // ========================================================================
    T_IO ialpha_fixed = float_to_fixed<W_IO, I_IO>(ialpha);
    T_IO ibeta_fixed = float_to_fixed<W_IO, I_IO>(ibeta);
    T_SINCOS cos_theta_fixed = float_to_fixed<W_SINCOS, I_SINCOS>(cos_theta);
    T_SINCOS sin_theta_fixed = float_to_fixed<W_SINCOS, I_SINCOS>(sin_theta);
    
    // ========================================================================
    // Step 3: Run DUT (ap_fixed)
    // ========================================================================
    T_IO id_fixed, iq_fixed;
    
    Park_Direct_ap_fixed<T_IO, T_SINCOS>(
        id_fixed,
        iq_fixed,
        ialpha_fixed,
        ibeta_fixed,
        cos_theta_fixed,
        sin_theta_fixed
    );
    
    // ========================================================================
    // Step 4: Convert DUT outputs back to float
    // ========================================================================
    result.id_dut = fixed_to_float(id_fixed);
    result.iq_dut = fixed_to_float(iq_fixed);
    
    result.mag_dut = magnitude(result.id_dut, result.iq_dut);
    
    // ========================================================================
    // Step 5: Calculate point-by-point errors
    // ========================================================================
    result.error_id = result.id_dut - result.id_golden;
    result.error_iq = result.iq_dut - result.iq_golden;
    
    return result;
}

/**
 * @brief Save test results to CSV file
 */
void save_results_to_csv(const std::string& filename,
                         const std::vector<TestPoint>& results,
                         const std::string& mode) {
    std::ofstream file(filename);
    
    if (!file.is_open()) {
        std::cerr << "Error: Cannot open file " << filename << std::endl;
        return;
    }
    
    // Write header
    file << "# Park Direct Transform - Phase 1 Precision Test Results\n";
    file << "# Test Mode: " << mode << "\n";
    file << "# Q-format (I/O): Q" << I_IO << "." << (W_IO-I_IO) << " (ap_fixed<" << W_IO << "," << I_IO << ">)\n";
    file << "# Q-format (sin/cos): Q" << I_SINCOS << "." << (W_SINCOS-I_SINCOS) << " (ap_fixed<" << W_SINCOS << "," << I_SINCOS << ">)\n";
    file << "# Number of test points: " << results.size() << "\n";
    file << "#\n";
    file << "# Columns:\n";
    file << "#   Ialpha_in, Ibeta_in: Quantized inputs (stationary frame, float32)\n";
    file << "#   Theta_rad: Input angle in radians\n";
    file << "#   cos_theta, sin_theta: Quantized trig values\n";
    file << "#   Id_golden, Iq_golden: Golden model outputs (rotating frame)\n";
    file << "#   Id_dut, Iq_dut: DUT outputs (dequantized)\n";
    file << "#   Error_Id, Error_Iq: Point-by-point errors (DUT - Golden)\n";
    file << "#   Mag_input, Mag_golden, Mag_dut: Magnitude (should be preserved)\n";
    file << "#\n";
    
    // Column headers
    file << "Ialpha_in,Ibeta_in,Theta_rad,cos_theta,sin_theta,";
    file << "Id_golden,Iq_golden,";
    file << "Id_dut,Iq_dut,";
    file << "Error_Id,Error_Iq,";
    file << "Mag_input,Mag_golden,Mag_dut\n";
    
    // Write data
    file << std::fixed << std::setprecision(10);
    for (const auto& point : results) {
        file << point.ialpha_float << "," << point.ibeta_float << ",";
        file << point.theta_float << ",";
        file << point.cos_theta_float << "," << point.sin_theta_float << ",";
        file << point.id_golden << "," << point.iq_golden << ",";
        file << point.id_dut << "," << point.iq_dut << ",";
        file << point.error_id << "," << point.error_iq << ",";
        file << point.mag_input << "," << point.mag_golden << "," << point.mag_dut << "\n";
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
    std::vector<float> golden_id, golden_iq;
    std::vector<float> dut_id, dut_iq;
    
    golden_id.reserve(results.size());
    golden_iq.reserve(results.size());
    dut_id.reserve(results.size());
    dut_iq.reserve(results.size());
    
    for (const auto& point : results) {
        golden_id.push_back(point.id_golden);
        golden_iq.push_back(point.iq_golden);
        dut_id.push_back(point.id_dut);
        dut_iq.push_back(point.iq_dut);
    }
    
    // Analyze Id
    std::cout << "--- Id (d-axis) ---" << std::endl;
    auto metrics_id = PrecisionAnalyzer::analyze(golden_id, dut_id);
    std::cout << std::scientific << std::setprecision(6);
    std::cout << "  MAE:       " << metrics_id.mae << std::endl;
    std::cout << "  RMSE:      " << metrics_id.rmse << std::endl;
    std::cout << "  Max Error: " << metrics_id.max_error << std::endl;
    std::cout << "  SNR:       " << std::fixed << std::setprecision(2) 
              << metrics_id.snr_db << " dB" << std::endl;
    std::cout << std::endl;
    
    // Analyze Iq
    std::cout << "--- Iq (q-axis) ---" << std::endl;
    auto metrics_iq = PrecisionAnalyzer::analyze(golden_iq, dut_iq);
    std::cout << std::scientific << std::setprecision(6);
    std::cout << "  MAE:       " << metrics_iq.mae << std::endl;
    std::cout << "  RMSE:      " << metrics_iq.rmse << std::endl;
    std::cout << "  Max Error: " << metrics_iq.max_error << std::endl;
    std::cout << "  SNR:       " << std::fixed << std::setprecision(2) 
              << metrics_iq.snr_db << " dB" << std::endl;
    std::cout << std::endl;
    
    // Overall Summary
    std::cout << "--- Overall Summary ---" << std::endl;
    float max_mae = std::max(metrics_id.mae, metrics_iq.mae);
    float max_rmse = std::max(metrics_id.rmse, metrics_iq.rmse);
    float max_error = std::max(metrics_id.max_error, metrics_iq.max_error);
    float min_snr = std::min(metrics_id.snr_db, metrics_iq.snr_db);
    
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
    std::cout << "  (Park Transform should preserve magnitude)" << std::endl;
    
    std::cout << "========================================\n" << std::endl;
}

// ============================================================================
// Test Mode Implementations
// ============================================================================

/**
 * @brief Test with random α-β inputs and random angles
 */
void test_random_mode() {
    std::cout << "\n=== Random Input Test ===" << std::endl;
    std::cout << "Generating " << NUM_RANDOM_TESTS << " random test cases..." << std::endl;
    
    StimulusGenerator gen(12345);  // Fixed seed for reproducibility
    std::vector<TestPoint> results;
    results.reserve(NUM_RANDOM_TESTS);
    
    for (int i = 0; i < NUM_RANDOM_TESTS; i++) {
        // Generate random α-β inputs
        float ialpha = gen.random_uniform(RANDOM_MIN, RANDOM_MAX);
        float ibeta = gen.random_uniform(RANDOM_MIN, RANDOM_MAX);
        
        // Generate random angle [0, 2π)
        float theta = gen.random_uniform(0.0f, 2.0f * PI);
        
        // Run test
        TestPoint result = run_test_point(ialpha, ibeta, theta);
        results.push_back(result);
        
        // Progress indicator
        if ((i + 1) % 100 == 0) {
            std::cout << "  Progress: " << (i + 1) << "/" << NUM_RANDOM_TESTS << std::endl;
        }
    }
    
    // Save results
    save_results_to_csv(RANDOM_OUTPUT_FILE, results, "Random Alpha-Beta Inputs with Random Angles");
    
    // Print summary
    print_summary(results, "Random Mode");
}

/**
 * @brief Test with rotating α-β vector and synchronous angle sweep
 */
void test_sine_mode() {
    std::cout << "\n=== Rotating Vector with Angle Sweep Test ===" << std::endl;
    std::cout << "Generating rotating vector test cases..." << std::endl;
    std::cout << "  Frequency: " << SINE_FREQUENCY << " Hz (α-β rotation)" << std::endl;
    std::cout << "  Rotor speed: " << ROTOR_SPEED_HZ << " Hz (angle θ)" << std::endl;
    std::cout << "  Amplitude: " << SINE_AMPLITUDE << " (radius)" << std::endl;
    std::cout << "  Duration: " << SINE_DURATION << " s" << std::endl;
    std::cout << "  Sample rate: " << SINE_SAMPLE_RATE << " Hz" << std::endl;
    
    size_t num_samples = static_cast<size_t>(SINE_SAMPLE_RATE * SINE_DURATION);
    std::vector<TestPoint> results;
    results.reserve(num_samples);
    
    float dt = 1.0f / SINE_SAMPLE_RATE;
    float omega_ab = 2.0f * PI * SINE_FREQUENCY;      // α-β rotation rate
    float omega_rotor = 2.0f * PI * ROTOR_SPEED_HZ;   // Rotor angle rate
    
    for (size_t i = 0; i < num_samples; i++) {
        float t = i * dt;
        
        // Rotating α-β vector: Ialpha = R*cos(ωt), Ibeta = R*sin(ωt)
        float ialpha = SINE_AMPLITUDE * std::cos(omega_ab * t);
        float ibeta = SINE_AMPLITUDE * std::sin(omega_ab * t);
        
        // Rotor angle increases linearly
        float theta = omega_rotor * t;
        theta = std::fmod(theta, 2.0f * PI);  // Wrap to [0, 2π)
        
        // Run test
        TestPoint result = run_test_point(ialpha, ibeta, theta);
        results.push_back(result);
        
        // Progress indicator
        if ((i + 1) % 50 == 0) {
            std::cout << "  Progress: " << (i + 1) << "/" << num_samples << std::endl;
        }
    }
    
    std::cout << "  Generated " << num_samples << " samples" << std::endl;
    
    // Save results
    save_results_to_csv(SINE_OUTPUT_FILE, results, "Rotating Vector (50 Hz) with Angle Sweep (50 Hz)");
    
    // Print summary
    print_summary(results, "Rotating Vector Mode");
}

// ============================================================================
// Main
// ============================================================================

int main(int argc, char* argv[]) {
    (void)argc;  // Suppress unused parameter warning
    (void)argv;
    
    std::cout << "======================================================" << std::endl;
    std::cout << "  Park Direct Transform - Phase 1 Precision Test" << std::endl;
    std::cout << "======================================================" << std::endl;
    std::cout << "\nConfiguration:" << std::endl;
    std::cout << "  Q-format (I/O): Q" << I_IO << "." << (W_IO-I_IO) 
              << " (ap_fixed<" << W_IO << "," << I_IO << ">)" << std::endl;
    std::cout << "  Q-format (sin/cos): Q" << I_SINCOS << "." << (W_SINCOS-I_SINCOS)
              << " (ap_fixed<" << W_SINCOS << "," << I_SINCOS << ">)" << std::endl;
    std::cout << "  DUT: Park_Direct_ap_fixed (park.hpp)" << std::endl;
    std::cout << "  Golden: park_direct_golden (models_fp/park_direct.hpp)" << std::endl;
    std::cout << std::endl;
    
    // Run all test modes
    test_random_mode();
    test_sine_mode();
    
    std::cout << "\n======================================================" << std::endl;
    std::cout << "  All tests completed" << std::endl;
    std::cout << "======================================================" << std::endl;
    std::cout << "\nOutput files:" << std::endl;
    std::cout << "  - " << RANDOM_OUTPUT_FILE << std::endl;
    std::cout << "  - " << SINE_OUTPUT_FILE << std::endl;
    std::cout << "\nTest Summary:" << std::endl;
    std::cout << "  - Random mode: " << NUM_RANDOM_TESTS << " test cases" << std::endl;
    std::cout << "  - Rotating vector mode: ~200 samples (1 rotation @ 50 Hz)" << std::endl;
    std::cout << "\nNote: Point-by-point errors saved in CSV files." << std::endl;
    std::cout << "      Precision analysis displayed in console output." << std::endl;
    std::cout << std::endl;
    
    return 0;
}
