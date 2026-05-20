/*
Copyright (C) 2025-2025, Advanced Micro Devices, Inc.
SPDX-License-Identifier: X11
*/

/**
 * @file test_svpwm.cpp
 * @brief Phase 1 Precision Test for SVPWM (Space Vector PWM)
 * 
 * Test Focus:
 *   - Sector determination accuracy
 *   - Duty cycle calculation precision
 *   - Sector boundary conditions
 *   - Numerical consistency (no timing/waveform analysis)
 * 
 * Test Modes:
 *   - Sector Sweep: Test all 6 sectors with boundary cases
 *   - Random Voltages: Random three-phase inputs
 *   - Rotating Vector: Circular trajectory in α-β plane
 */

#include <iostream>
#include <fstream>
#include <iomanip>
#include <vector>
#include <string>
#include <cmath>

// Vitis HLS headers
#include "ap_fixed.h"

// DUT header (we'll use the core calculation function)
#include "hw/svpwm.hpp"

// Phase 1 validation utilities
#include "stimulus_generator.hpp"
#include "quantization_utils.hpp"
#include "precision_analyzer.hpp"
#include "compatibility.hpp"

using namespace xf::motorcontrol::test;
using namespace xf::motorcontrol;

// ============================================================================
// Test Configuration
// ============================================================================

// Q-format configuration (matching motor control typical usage)
constexpr int W_IO = 32;
constexpr int I_IO = 16;
typedef ap_fixed<W_IO, I_IO> T_FOC_COM;

// DC Link voltage (typical value)
constexpr float DC_LINK_VOLTAGE = 24.0f;

// Test parameters
constexpr int NUM_RANDOM_TESTS = 1000;
constexpr float VOLTAGE_RANGE = 20.0f;  // Test range ±20V

constexpr float ROTATING_AMPLITUDE = 15.0f;  // 15V amplitude
constexpr float ROTATING_FREQUENCY = 50.0f;  // 50 Hz
constexpr float SAMPLE_RATE = 1000.0f;       // 1 kHz
constexpr float DURATION = 0.06f;             // 60ms (3 full rotations)

// Output files
const std::string SECTOR_OUTPUT_FILE = "svpwm_sector_sweep.csv";
const std::string RANDOM_OUTPUT_FILE = "svpwm_random_tests.csv";
const std::string ROTATING_OUTPUT_FILE = "svpwm_rotating_vector.csv";

// ============================================================================
// Test Data Structure
// ============================================================================

struct TestPoint {
    // Inputs
    float va_cmd;
    float vb_cmd;
    float vc_cmd;
    float dc_link;
    
    // Golden outputs
    float duty_a_golden;
    float duty_b_golden;
    float duty_c_golden;
    int sector_golden;
    float voff_golden;
    
    // DUT outputs (dequantized)
    float duty_a_dut;
    float duty_b_dut;
    float duty_c_dut;
    
    // Errors
    float error_duty_a;
    float error_duty_b;
    float error_duty_c;
    
    // Analysis
    float magnitude;  // sqrt(Valpha^2 + Vbeta^2)
    float angle_deg;  // Angle in degrees
    
    TestPoint() : va_cmd(0), vb_cmd(0), vc_cmd(0), dc_link(DC_LINK_VOLTAGE),
                  duty_a_golden(0), duty_b_golden(0), duty_c_golden(0),
                  sector_golden(0), voff_golden(0),
                  duty_a_dut(0), duty_b_dut(0), duty_c_dut(0),
                  error_duty_a(0), error_duty_b(0), error_duty_c(0),
                  magnitude(0), angle_deg(0) {}
};

// ============================================================================
// Helper Functions
// ============================================================================

/**
 * @brief Run single SVPWM test point
 */
TestPoint run_test_point(float va, float vb, float vc, float dc_link) {
    TestPoint result;
    
    // Store inputs
    result.va_cmd = va;
    result.vb_cmd = vb;
    result.vc_cmd = vc;
    result.dc_link = dc_link;
    
    // Calculate magnitude and angle (for analysis)
    const float sqrt3 = 1.732050808f;
    float valpha = va;
    float vbeta = (va + 2.0f * vb) / sqrt3;
    result.magnitude = std::sqrt(valpha * valpha + vbeta * vbeta);
    result.angle_deg = std::atan2(vbeta, valpha) * 180.0f / PI;
    if (result.angle_deg < 0.0f) result.angle_deg += 360.0f;
    
    // ========================================================================
    // Run Golden Model
    // ========================================================================
    xf::motorcontrol::golden::SVPWMOutput<float> golden_output;
    xf::motorcontrol::golden::svpwm_golden(golden_output, va, vb, vc, dc_link);
    
    result.duty_a_golden = golden_output.duty_a;
    result.duty_b_golden = golden_output.duty_b;
    result.duty_c_golden = golden_output.duty_c;
    result.sector_golden = golden_output.sector;
    result.voff_golden = golden_output.voff;
    
    // ========================================================================
    // Run DUT (core calculation function)
    // ========================================================================
    
    // Quantize inputs
    T_FOC_COM va_fixed = float_to_fixed<W_IO, I_IO>(va);
    T_FOC_COM vb_fixed = float_to_fixed<W_IO, I_IO>(vb);
    T_FOC_COM vc_fixed = float_to_fixed<W_IO, I_IO>(vc);
    T_FOC_COM dc_link_fixed = float_to_fixed<W_IO, I_IO>(dc_link);
    
    // Prepare inputs for DUT
    T_FOC_COM Vcmd[3];
    Vcmd[0] = va_fixed;
    Vcmd[1] = vb_fixed;
    Vcmd[2] = vc_fixed;
    
    // Call DUT core function
    ap_ufixed<17, 1> duty_ratio[3];
    
    details::calculate_ratios_core<T_FOC_COM>(
        duty_ratio,
        Vcmd,
        dc_link_fixed,
        static_cast<int>(dc_link),  // dc_link_ref (not used in ADC mode)
        0  // DC_SRC_ADC mode (use measured dc_link)
    );
    
    // Dequantize outputs
    result.duty_a_dut = static_cast<float>(duty_ratio[0]);
    result.duty_b_dut = static_cast<float>(duty_ratio[1]);
    result.duty_c_dut = static_cast<float>(duty_ratio[2]);
    
    // Calculate errors
    result.error_duty_a = result.duty_a_dut - result.duty_a_golden;
    result.error_duty_b = result.duty_b_dut - result.duty_b_golden;
    result.error_duty_c = result.duty_c_dut - result.duty_c_golden;
    
    return result;
}

/**
 * @brief Save results to CSV
 */
void save_results_to_csv(const std::string& filename,
                         const std::vector<TestPoint>& results,
                         const std::string& mode) {
    std::ofstream file(filename);
    
    if (!file.is_open()) {
        std::cerr << "Error: Cannot open file " << filename << std::endl;
        return;
    }
    
    file << "# SVPWM - Phase 1 Precision Test Results\n";
    file << "# Test Mode: " << mode << "\n";
    file << "# Q-format: Q" << I_IO << "." << (W_IO-I_IO) << "\n";
    file << "# DC Link Voltage: " << DC_LINK_VOLTAGE << " V\n";
    file << "# Number of test points: " << results.size() << "\n";
    file << "#\n";
    file << "# Columns:\n";
    file << "#   Va_cmd, Vb_cmd, Vc_cmd: Input voltage commands\n";
    file << "#   Duty_a/b/c_golden: Golden duty cycles [0,1]\n";
    file << "#   Duty_a/b/c_dut: DUT duty cycles (dequantized)\n";
    file << "#   Error_duty_a/b/c: Duty cycle errors\n";
    file << "#   Sector: Determined sector (1-6)\n";
    file << "#   Voff: Offset voltage\n";
    file << "#   Magnitude: Voltage vector magnitude\n";
    file << "#   Angle_deg: Angle in degrees [0, 360)\n";
    file << "#\n";
    
    file << "Va_cmd,Vb_cmd,Vc_cmd,";
    file << "Duty_a_golden,Duty_b_golden,Duty_c_golden,";
    file << "Duty_a_dut,Duty_b_dut,Duty_c_dut,";
    file << "Error_duty_a,Error_duty_b,Error_duty_c,";
    file << "Sector,Voff,Magnitude,Angle_deg\n";
    
    file << std::fixed << std::setprecision(10);
    for (const auto& point : results) {
        file << point.va_cmd << "," << point.vb_cmd << "," << point.vc_cmd << ",";
        file << point.duty_a_golden << "," << point.duty_b_golden << "," << point.duty_c_golden << ",";
        file << point.duty_a_dut << "," << point.duty_b_dut << "," << point.duty_c_dut << ",";
        file << point.error_duty_a << "," << point.error_duty_b << "," << point.error_duty_c << ",";
        file << point.sector_golden << "," << point.voff_golden << ",";
        file << point.magnitude << "," << point.angle_deg << "\n";
    }
    
    file.close();
    std::cout << "Results saved to: " << filename << std::endl;
}

/**
 * @brief Print precision analysis
 */
void print_summary(const std::vector<TestPoint>& results, const std::string& mode) {
    if (results.empty()) return;
    
    std::cout << "\n========================================" << std::endl;
    std::cout << "  " << mode << " - Precision Analysis" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "Test points: " << results.size() << std::endl;
    std::cout << std::endl;
    
    // Extract duty cycles
    std::vector<float> golden_a, golden_b, golden_c;
    std::vector<float> dut_a, dut_b, dut_c;
    
    for (const auto& point : results) {
        golden_a.push_back(point.duty_a_golden);
        golden_b.push_back(point.duty_b_golden);
        golden_c.push_back(point.duty_c_golden);
        dut_a.push_back(point.duty_a_dut);
        dut_b.push_back(point.duty_b_dut);
        dut_c.push_back(point.duty_c_dut);
    }
    
    // Analyze each phase
    std::cout << "--- Duty Cycle A ---" << std::endl;
    auto metrics_a = PrecisionAnalyzer::analyze(golden_a, dut_a);
    std::cout << std::scientific << std::setprecision(6);
    std::cout << "  MAE:       " << metrics_a.mae << std::endl;
    std::cout << "  RMSE:      " << metrics_a.rmse << std::endl;
    std::cout << "  Max Error: " << metrics_a.max_error << std::endl;
    std::cout << "  SNR:       " << std::fixed << std::setprecision(2) 
              << metrics_a.snr_db << " dB" << std::endl;
    std::cout << std::endl;
    
    std::cout << "--- Duty Cycle B ---" << std::endl;
    auto metrics_b = PrecisionAnalyzer::analyze(golden_b, dut_b);
    std::cout << std::scientific << std::setprecision(6);
    std::cout << "  MAE:       " << metrics_b.mae << std::endl;
    std::cout << "  RMSE:      " << metrics_b.rmse << std::endl;
    std::cout << "  Max Error: " << metrics_b.max_error << std::endl;
    std::cout << "  SNR:       " << std::fixed << std::setprecision(2) 
              << metrics_b.snr_db << " dB" << std::endl;
    std::cout << std::endl;
    
    std::cout << "--- Duty Cycle C ---" << std::endl;
    auto metrics_c = PrecisionAnalyzer::analyze(golden_c, dut_c);
    std::cout << std::scientific << std::setprecision(6);
    std::cout << "  MAE:       " << metrics_c.mae << std::endl;
    std::cout << "  RMSE:      " << metrics_c.rmse << std::endl;
    std::cout << "  Max Error: " << metrics_c.max_error << std::endl;
    std::cout << "  SNR:       " << std::fixed << std::setprecision(2) 
              << metrics_c.snr_db << " dB" << std::endl;
    std::cout << std::endl;
    
    // Overall summary
    std::cout << "--- Overall Summary ---" << std::endl;
    float worst_mae = std::max({metrics_a.mae, metrics_b.mae, metrics_c.mae});
    float worst_rmse = std::max({metrics_a.rmse, metrics_b.rmse, metrics_c.rmse});
    float worst_max = std::max({metrics_a.max_error, metrics_b.max_error, metrics_c.max_error});
    float worst_snr = std::min({metrics_a.snr_db, metrics_b.snr_db, metrics_c.snr_db});
    
    std::cout << std::scientific << std::setprecision(6);
    std::cout << "  Worst MAE:       " << worst_mae << std::endl;
    std::cout << "  Worst RMSE:      " << worst_rmse << std::endl;
    std::cout << "  Worst Max Error: " << worst_max << std::endl;
    std::cout << "  Worst SNR:       " << std::fixed << std::setprecision(2) 
              << worst_snr << " dB" << std::endl;
    
    std::cout << "========================================\n" << std::endl;
}

// ============================================================================
// Test Modes
// ============================================================================

/**
 * @brief Test all 6 sectors with boundary conditions
 */
void test_sector_sweep() {
    std::cout << "\n=== Sector Sweep Test ===" << std::endl;
    std::cout << "Testing all 6 sectors with boundary angles..." << std::endl;
    
    std::vector<TestPoint> results;
    
    float amplitude = 15.0f;  // 15V amplitude
    
    // Test each sector with 5 angles: start, 1/4, center, 3/4, end
    for (int sector = 1; sector <= 6; sector++) {
        float sector_start = (sector - 1) * 60.0f;  // degrees
        
        std::cout << "  Sector " << sector << ": ";
        
        for (int i = 0; i <= 10; i++) {  // 11 points per sector
            float angle_deg = sector_start + i * 6.0f;  // 6° steps
            float angle_rad = angle_deg * PI / 180.0f;
            
            // Convert to α-β
            float valpha = amplitude * std::cos(angle_rad);
            float vbeta = amplitude * std::sin(angle_rad);
            
            // Convert to three-phase
            const float sqrt3 = 1.732050808f;
            float va = valpha;
            float vb = (-valpha + sqrt3 * vbeta) / 2.0f;
            float vc = (-valpha - sqrt3 * vbeta) / 2.0f;
            
            TestPoint result = run_test_point(va, vb, vc, DC_LINK_VOLTAGE);
            results.push_back(result);
        }
        std::cout << "tested " << 11 << " angles" << std::endl;
    }
    
    std::cout << "  Total test points: " << results.size() << std::endl;
    
    save_results_to_csv(SECTOR_OUTPUT_FILE, results, "Sector Sweep (6 sectors, boundary angles)");
    print_summary(results, "Sector Sweep");
    
    // Sector verification
    std::cout << "--- Sector Verification ---" << std::endl;
    int sector_counts[7] = {0};  // Index 0 unused, 1-6 for sectors
    for (const auto& r : results) {
        if (r.sector_golden >= 1 && r.sector_golden <= 6) {
            sector_counts[r.sector_golden]++;
        }
    }
    for (int i = 1; i <= 6; i++) {
        std::cout << "  Sector " << i << ": " << sector_counts[i] << " points" << std::endl;
    }
    std::cout << "========================================\n" << std::endl;
}

/**
 * @brief Random voltage inputs
 */
void test_random() {
    std::cout << "\n=== Random Voltage Test ===" << std::endl;
    std::cout << "Generating " << NUM_RANDOM_TESTS << " random test cases..." << std::endl;
    
    StimulusGenerator gen(54321);
    std::vector<TestPoint> results;
    results.reserve(NUM_RANDOM_TESTS);
    
    for (int i = 0; i < NUM_RANDOM_TESTS; i++) {
        // Generate random three-phase voltages
        float va = gen.random_uniform(-VOLTAGE_RANGE, VOLTAGE_RANGE);
        float vb = gen.random_uniform(-VOLTAGE_RANGE, VOLTAGE_RANGE);
        float vc = gen.random_uniform(-VOLTAGE_RANGE, VOLTAGE_RANGE);
        
        TestPoint result = run_test_point(va, vb, vc, DC_LINK_VOLTAGE);
        results.push_back(result);
        
        if ((i + 1) % 100 == 0) {
            std::cout << "  Progress: " << (i + 1) << "/" << NUM_RANDOM_TESTS << std::endl;
        }
    }
    
    save_results_to_csv(RANDOM_OUTPUT_FILE, results, "Random Three-Phase Voltages");
    print_summary(results, "Random Voltages");
}

/**
 * @brief Rotating vector (circular trajectory)
 */
void test_rotating_vector() {
    std::cout << "\n=== Rotating Vector Test ===" << std::endl;
    std::cout << "Amplitude: " << ROTATING_AMPLITUDE << " V" << std::endl;
    std::cout << "Frequency: " << ROTATING_FREQUENCY << " Hz" << std::endl;
    std::cout << "Duration:  " << DURATION << " s" << std::endl;
    std::cout << "Sample rate: " << SAMPLE_RATE << " Hz" << std::endl;
    
    int num_samples = static_cast<int>(SAMPLE_RATE * DURATION);
    float dt = 1.0f / SAMPLE_RATE;
    float omega = 2.0f * PI * ROTATING_FREQUENCY;
    
    std::vector<TestPoint> results;
    results.reserve(num_samples);
    
    const float sqrt3 = 1.732050808f;
    
    for (int i = 0; i < num_samples; i++) {
        float t = i * dt;
        float angle = omega * t;
        
        // α-β coordinates (rotating vector)
        float valpha = ROTATING_AMPLITUDE * std::cos(angle);
        float vbeta = ROTATING_AMPLITUDE * std::sin(angle);
        
        // Convert to three-phase
        float va = valpha;
        float vb = (-valpha + sqrt3 * vbeta) / 2.0f;
        float vc = (-valpha - sqrt3 * vbeta) / 2.0f;
        
        TestPoint result = run_test_point(va, vb, vc, DC_LINK_VOLTAGE);
        results.push_back(result);
        
        if ((i + 1) % 20 == 0) {
            std::cout << "  Progress: " << (i + 1) << "/" << num_samples << std::endl;
        }
    }
    
    std::cout << "  Generated " << num_samples << " samples" << std::endl;
    
    save_results_to_csv(ROTATING_OUTPUT_FILE, results, "Rotating Vector (50 Hz, 3 rotations)");
    print_summary(results, "Rotating Vector");
}

// ============================================================================
// Main
// ============================================================================

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;
    
    std::cout << "======================================================" << std::endl;
    std::cout << "  SVPWM - Phase 1 Precision Test" << std::endl;
    std::cout << "======================================================" << std::endl;
    std::cout << "\nConfiguration:" << std::endl;
    std::cout << "  Q-format: Q" << I_IO << "." << (W_IO-I_IO) 
              << " (ap_fixed<" << W_IO << "," << I_IO << ">)" << std::endl;
    std::cout << "  DC Link Voltage: " << DC_LINK_VOLTAGE << " V" << std::endl;
    std::cout << "  DUT: calculate_ratios_core (svpwm.hpp)" << std::endl;
    std::cout << "  Golden: svpwm_golden (models_fp/svpwm.hpp)" << std::endl;
    std::cout << std::endl;
    
    test_sector_sweep();
    test_random();
    test_rotating_vector();
    
    std::cout << "\n======================================================" << std::endl;
    std::cout << "  All tests completed" << std::endl;
    std::cout << "======================================================" << std::endl;
    std::cout << "\nOutput files:" << std::endl;
    std::cout << "  - " << SECTOR_OUTPUT_FILE << std::endl;
    std::cout << "  - " << RANDOM_OUTPUT_FILE << std::endl;
    std::cout << "  - " << ROTATING_OUTPUT_FILE << std::endl;
    std::cout << "\nTest Summary:" << std::endl;
    std::cout << "  - Sector sweep: 66 test points (11 per sector)" << std::endl;
    std::cout << "  - Random voltages: " << NUM_RANDOM_TESTS << " test cases" << std::endl;
    std::cout << "  - Rotating vector: ~60 samples (3 rotations @ 50 Hz)" << std::endl;
    std::cout << std::endl;
    
    return 0;
}
