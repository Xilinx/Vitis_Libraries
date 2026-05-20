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
 * @file test_pi_control.cpp
 * @brief Phase 1 Precision Test for PI Controller
 * 
 * Test Flow:
 *   1. Initialize controller states (Golden and DUT)
 *   2. Generate test inputs (step response / sine tracking)
 *   3. Run Golden Model (float32) - step by step
 *   4. Run DUT (ap_fixed) - step by step
 *   5. Compare outputs at each time step
 *   6. Analyze precision with precision_analyzer
 * 
 * Test Modes:
 *   - Step Response: Setpoint step change, analyze settling behavior
 *   - Sine Tracking: Sinusoidal setpoint, analyze tracking performance
 * 
 * CRITICAL: Controller is stateful - state initialization and update 
 *           order must match between Golden and DUT
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
#include "hw/pid_control.hpp"

// Phase 1 validation utilities
#include "stimulus_generator.hpp"
#include "quantization_utils.hpp"
#include "precision_analyzer.hpp"
#include "compatibility.hpp"

using namespace xf::motorcontrol::test;

// ============================================================================
// Test Configuration
// ============================================================================

// Q-format configuration
constexpr int W_IO = 32;      // I/O width
constexpr int I_IO = 16;      // I/O integer bits
constexpr int W_ACC = 48;     // Accumulator width (wider for integral)
constexpr int I_ACC = 24;     // Accumulator integer bits
constexpr int W_ERR = 32;     // Error width
constexpr int I_ERR = 16;     // Error integer bits
constexpr int W_PID = 32;     // Coefficient width
constexpr int I_PID = 16;     // Coefficient integer bits

typedef ap_fixed<W_IO, I_IO> T_IO;
typedef ap_fixed<W_ACC, I_ACC> T_ACC;
typedef ap_fixed<W_ERR, I_ERR> T_ERR;
typedef ap_fixed<W_PID, I_PID> T_PID;

// PI Controller Parameters
constexpr float KP = 0.5f;      // Proportional gain
constexpr float KI = 0.1f;      // Integral gain
constexpr float KD = 0.0f;      // Derivative gain (0 for PI mode)

// Step Response Test Parameters
constexpr float STEP_INITIAL_SETPOINT = 0.0f;
constexpr float STEP_FINAL_SETPOINT = 10.0f;
constexpr int STEP_DURATION_SAMPLES = 500;
constexpr int STEP_CHANGE_TIME = 50;  // Step occurs at sample 50

// Sine Tracking Test Parameters
constexpr float SINE_AMPLITUDE = 10.0f;
constexpr float SINE_FREQUENCY = 1.0f;  // 1 Hz
constexpr float SINE_SAMPLE_RATE = 100.0f;  // 100 Hz
constexpr float SINE_DURATION = 5.0f;  // 5 seconds

// Simulated Plant (simple first-order system for testing)
constexpr float PLANT_TIME_CONSTANT = 0.5f;  // tau

// Output file names
const std::string STEP_OUTPUT_FILE = "pi_control_step_response.csv";
const std::string SINE_OUTPUT_FILE = "pi_control_sine_tracking.csv";

// ============================================================================
// Test Data Structure
// ============================================================================

struct TestPoint {
    int time_step;
    
    // Inputs
    float setpoint;
    float measured_value;  // Process variable (from simulated plant)
    
    // Golden outputs
    float output_golden;
    float error_golden;
    float integral_golden;
    
    // DUT outputs (dequantized)
    float output_dut;
    
    // Errors
    float error_output;
    
    TestPoint() : time_step(0), setpoint(0), measured_value(0),
                  output_golden(0), error_golden(0), integral_golden(0),
                  output_dut(0), error_output(0) {}
};

// ============================================================================
// Simulated Plant Model (for closed-loop testing)
// ============================================================================

/**
 * @brief Simple first-order plant simulation
 * 
 * Transfer function: G(s) = 1 / (tau*s + 1)
 * Discrete: y(k) = alpha * y(k-1) + (1-alpha) * u(k-1)
 * where alpha = exp(-T/tau)
 */
class SimplePlant {
private:
    float state;
    float alpha;
    
public:
    SimplePlant(float tau, float dt) {
        state = 0.0f;
        alpha = std::exp(-dt / tau);
    }
    
    void reset() {
        state = 0.0f;
    }
    
    float update(float input) {
        state = alpha * state + (1.0f - alpha) * input;
        return state;
    }
    
    float get_output() const {
        return state;
    }
};

// ============================================================================
// Test Functions
// ============================================================================

void save_results_to_csv(const std::string& filename,
                         const std::vector<TestPoint>& results,
                         const std::string& mode) {
    std::ofstream file(filename);
    
    if (!file.is_open()) {
        std::cerr << "Error: Cannot open file " << filename << std::endl;
        return;
    }
    
    file << "# PI Controller - Phase 1 Precision Test Results\n";
    file << "# Test Mode: " << mode << "\n";
    file << "# Q-format (I/O): Q" << I_IO << "." << (W_IO-I_IO) << "\n";
    file << "# Q-format (ACC): Q" << I_ACC << "." << (W_ACC-I_ACC) << "\n";
    file << "# Parameters: Kp=" << KP << ", Ki=" << KI << ", Kd=" << KD << "\n";
    file << "# Number of samples: " << results.size() << "\n";
    file << "#\n";
    file << "# Columns:\n";
    file << "#   Time_step: Sample number\n";
    file << "#   Setpoint: Desired value (reference)\n";
    file << "#   Measured: Process variable (plant output)\n";
    file << "#   Output_golden: Golden controller output\n";
    file << "#   Output_dut: DUT controller output (dequantized)\n";
    file << "#   Error_golden: Setpoint - Measured (golden)\n";
    file << "#   Integral_golden: Accumulated error (golden state)\n";
    file << "#   Error_output: Output error (DUT - Golden)\n";
    file << "#\n";
    
    file << "Time_step,Setpoint,Measured,Output_golden,Output_dut,";
    file << "Error_golden,Integral_golden,Error_output\n";
    
    file << std::fixed << std::setprecision(10);
    for (const auto& point : results) {
        file << point.time_step << ",";
        file << point.setpoint << "," << point.measured_value << ",";
        file << point.output_golden << "," << point.output_dut << ",";
        file << point.error_golden << "," << point.integral_golden << ",";
        file << point.error_output << "\n";
    }
    
    file.close();
    std::cout << "Results saved to: " << filename << std::endl;
}

void print_summary(const std::vector<TestPoint>& results, const std::string& mode) {
    if (results.empty()) return;
    
    std::cout << "\n========================================" << std::endl;
    std::cout << "  " << mode << " - Precision Analysis" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "Test samples: " << results.size() << std::endl;
    std::cout << std::endl;
    
    // Extract outputs
    std::vector<float> golden_output, dut_output;
    golden_output.reserve(results.size());
    dut_output.reserve(results.size());
    
    for (const auto& point : results) {
        golden_output.push_back(point.output_golden);
        dut_output.push_back(point.output_dut);
    }
    
    // Analyze
    std::cout << "--- Controller Output ---" << std::endl;
    auto metrics = PrecisionAnalyzer::analyze(golden_output, dut_output);
    std::cout << std::scientific << std::setprecision(6);
    std::cout << "  MAE:       " << metrics.mae << std::endl;
    std::cout << "  RMSE:      " << metrics.rmse << std::endl;
    std::cout << "  Max Error: " << metrics.max_error << std::endl;
    std::cout << "  SNR:       " << std::fixed << std::setprecision(2) 
              << metrics.snr_db << " dB" << std::endl;
    std::cout << std::endl;
    
    // Statistics
    std::cout << "--- Test Statistics ---" << std::endl;
    float final_error = results.back().error_golden;
    float steady_state_output_golden = 0.0f;
    float steady_state_output_dut = 0.0f;
    int steady_start = static_cast<int>(results.size() * 0.8);
    
    for (int i = steady_start; i < static_cast<int>(results.size()); i++) {
        steady_state_output_golden += results[i].output_golden;
        steady_state_output_dut += results[i].output_dut;
    }
    steady_state_output_golden /= (results.size() - steady_start);
    steady_state_output_dut /= (results.size() - steady_start);
    
    std::cout << std::fixed << std::setprecision(6);
    std::cout << "  Final error: " << final_error << std::endl;
    std::cout << "  Steady-state output (golden): " << steady_state_output_golden << std::endl;
    std::cout << "  Steady-state output (DUT):    " << steady_state_output_dut << std::endl;
    std::cout << "  Steady-state difference:      " << (steady_state_output_dut - steady_state_output_golden) << std::endl;
    
    std::cout << "========================================\n" << std::endl;
}

// ============================================================================
// Test Mode 1: Step Response
// ============================================================================

void test_step_response() {
    std::cout << "\n=== Step Response Test ===" << std::endl;
    std::cout << "Initial setpoint: " << STEP_INITIAL_SETPOINT << std::endl;
    std::cout << "Final setpoint:   " << STEP_FINAL_SETPOINT << std::endl;
    std::cout << "Step time:        " << STEP_CHANGE_TIME << std::endl;
    std::cout << "Duration:         " << STEP_DURATION_SAMPLES << " samples" << std::endl;
    
    // Initialize states
    xf::motorcontrol::golden::PIControllerState<float> golden_state;
    T_ACC dut_integral_error = 0;
    T_ERR dut_previous_error = 0;
    
    // Initialize plant
    float dt = 0.01f;  // 10ms sample time
    SimplePlant plant(PLANT_TIME_CONSTANT, dt);
    
    // Quantize PI gains
    float kp_quantized = quantize_float<W_PID, I_PID>(KP);
    float ki_quantized = quantize_float<W_PID, I_PID>(KI);
    float kd_quantized = quantize_float<W_PID, I_PID>(KD);
    
    T_PID kp_fixed = float_to_fixed<W_PID, I_PID>(KP);
    T_PID ki_fixed = float_to_fixed<W_PID, I_PID>(KI);
    T_PID kd_fixed = float_to_fixed<W_PID, I_PID>(KD);
    
    std::vector<TestPoint> results;
    results.reserve(STEP_DURATION_SAMPLES);
    
    for (int t = 0; t < STEP_DURATION_SAMPLES; t++) {
        TestPoint point;
        point.time_step = t;
        
        // Generate setpoint (step input)
        float setpoint = (t < STEP_CHANGE_TIME) ? STEP_INITIAL_SETPOINT : STEP_FINAL_SETPOINT;
        point.setpoint = setpoint;
        
        // Get measured value from plant
        float measured = plant.get_output();
        point.measured_value = measured;
        
        // Quantize inputs
        float setpoint_q = quantize_float<W_IO, I_IO>(setpoint);
        float measured_q = quantize_float<W_IO, I_IO>(measured);
        
        T_IO setpoint_fixed = float_to_fixed<W_IO, I_IO>(setpoint);
        T_IO measured_fixed = float_to_fixed<W_IO, I_IO>(measured);
        
        // ====================================================================
        // Run Golden Model
        // ====================================================================
        float output_golden;
        xf::motorcontrol::golden::pi_control_golden(output_golden, golden_state, measured_q, setpoint_q,
                              kp_quantized, ki_quantized, false);
        
        point.output_golden = output_golden;
        point.error_golden = setpoint_q - measured_q;
        point.integral_golden = golden_state.integral_error;
        
        // ====================================================================
        // Run DUT
        // ====================================================================
        T_IO output_dut_fixed;
        PID_Control_ap_fixed<T_IO, T_ACC, T_ERR, T_PID>(
            output_dut_fixed,
            dut_integral_error,
            dut_previous_error,
            measured_fixed,
            setpoint_fixed,
            kp_fixed,
            ki_fixed,
            kd_fixed,
            false  // mode_change = false
        );
        
        float output_dut = fixed_to_float(output_dut_fixed);
        point.output_dut = output_dut;
        
        // Calculate error
        point.error_output = output_dut - output_golden;
        
        results.push_back(point);
        
        // Update plant with controller output
        plant.update(output_golden);  // Use golden for plant simulation
        
        // Progress
        if ((t + 1) % 100 == 0) {
            std::cout << "  Progress: " << (t + 1) << "/" << STEP_DURATION_SAMPLES << std::endl;
        }
    }
    
    save_results_to_csv(STEP_OUTPUT_FILE, results, "Step Response");
    print_summary(results, "Step Response");
}

// ============================================================================
// Test Mode 2: Sine Tracking
// ============================================================================

void test_sine_tracking() {
    std::cout << "\n=== Sine Tracking Test ===" << std::endl;
    std::cout << "Sine amplitude:   " << SINE_AMPLITUDE << std::endl;
    std::cout << "Sine frequency:   " << SINE_FREQUENCY << " Hz" << std::endl;
    std::cout << "Sample rate:      " << SINE_SAMPLE_RATE << " Hz" << std::endl;
    std::cout << "Duration:         " << SINE_DURATION << " s" << std::endl;
    
    int num_samples = static_cast<int>(SINE_SAMPLE_RATE * SINE_DURATION);
    float dt = 1.0f / SINE_SAMPLE_RATE;
    
    // Initialize states
    xf::motorcontrol::golden::PIControllerState<float> golden_state;
    T_ACC dut_integral_error = 0;
    T_ERR dut_previous_error = 0;
    
    // Initialize plant
    SimplePlant plant(PLANT_TIME_CONSTANT, dt);
    
    // Quantize gains
    float kp_quantized = quantize_float<W_PID, I_PID>(KP);
    float ki_quantized = quantize_float<W_PID, I_PID>(KI);
    
    T_PID kp_fixed = float_to_fixed<W_PID, I_PID>(KP);
    T_PID ki_fixed = float_to_fixed<W_PID, I_PID>(KI);
    T_PID kd_fixed = float_to_fixed<W_PID, I_PID>(KD);
    
    std::vector<TestPoint> results;
    results.reserve(num_samples);
    
    float omega = 2.0f * PI * SINE_FREQUENCY;
    
    for (int t = 0; t < num_samples; t++) {
        TestPoint point;
        point.time_step = t;
        
        // Generate sinusoidal setpoint
        float time = t * dt;
        float setpoint = SINE_AMPLITUDE * std::sin(omega * time);
        point.setpoint = setpoint;
        
        // Get measured value from plant
        float measured = plant.get_output();
        point.measured_value = measured;
        
        // Quantize inputs
        float setpoint_q = quantize_float<W_IO, I_IO>(setpoint);
        float measured_q = quantize_float<W_IO, I_IO>(measured);
        
        T_IO setpoint_fixed = float_to_fixed<W_IO, I_IO>(setpoint);
        T_IO measured_fixed = float_to_fixed<W_IO, I_IO>(measured);
        
        // Run Golden Model
        float output_golden;
        xf::motorcontrol::golden::pi_control_golden(output_golden, golden_state, measured_q, setpoint_q,
                              kp_quantized, ki_quantized, false);
        
        point.output_golden = output_golden;
        point.error_golden = setpoint_q - measured_q;
        point.integral_golden = golden_state.integral_error;
        
        // Run DUT
        T_IO output_dut_fixed;
        PID_Control_ap_fixed<T_IO, T_ACC, T_ERR, T_PID>(
            output_dut_fixed,
            dut_integral_error,
            dut_previous_error,
            measured_fixed,
            setpoint_fixed,
            kp_fixed,
            ki_fixed,
            kd_fixed,
            false
        );
        
        float output_dut = fixed_to_float(output_dut_fixed);
        point.output_dut = output_dut;
        
        point.error_output = output_dut - output_golden;
        
        results.push_back(point);
        
        // Update plant
        plant.update(output_golden);
        
        if ((t + 1) % 100 == 0) {
            std::cout << "  Progress: " << (t + 1) << "/" << num_samples << std::endl;
        }
    }
    
    save_results_to_csv(SINE_OUTPUT_FILE, results, "Sine Tracking");
    print_summary(results, "Sine Tracking");
}

// ============================================================================
// Main
// ============================================================================

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;
    
    std::cout << "======================================================" << std::endl;
    std::cout << "  PI Controller - Phase 1 Precision Test" << std::endl;
    std::cout << "======================================================" << std::endl;
    std::cout << "\nConfiguration:" << std::endl;
    std::cout << "  Q-format (I/O):  Q" << I_IO << "." << (W_IO-I_IO) << "\n";
    std::cout << "  Q-format (ACC):  Q" << I_ACC << "." << (W_ACC-I_ACC) << "\n";
    std::cout << "  Q-format (ERR):  Q" << I_ERR << "." << (W_ERR-I_ERR) << "\n";
    std::cout << "  Q-format (PID):  Q" << I_PID << "." << (W_PID-I_PID) << "\n";
    std::cout << "  Kp = " << KP << ", Ki = " << KI << ", Kd = " << KD << " (PI mode)" << std::endl;
    std::cout << "  DUT: PID_Control_ap_fixed (pid_control.hpp)" << std::endl;
    std::cout << "  Golden: pi_control_golden (models_fp/pi_control.hpp)" << std::endl;
    std::cout << std::endl;
    
    test_step_response();
    test_sine_tracking();
    
    std::cout << "\n======================================================" << std::endl;
    std::cout << "  All tests completed" << std::endl;
    std::cout << "======================================================" << std::endl;
    std::cout << "\nOutput files:" << std::endl;
    std::cout << "  - " << STEP_OUTPUT_FILE << std::endl;
    std::cout << "  - " << SINE_OUTPUT_FILE << std::endl;
    std::cout << "\nTest Summary:" << std::endl;
    std::cout << "  - Step response: " << STEP_DURATION_SAMPLES << " samples" << std::endl;
    std::cout << "  - Sine tracking: ~500 samples (5s @ 100 Hz)" << std::endl;
    std::cout << std::endl;
    
    return 0;
}
