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

#include "precision_analyzer.hpp"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <cassert>

namespace xf {
namespace motorcontrol {
namespace test {

// ============================================================================
// Main Analysis Functions
// ============================================================================

PrecisionMetrics PrecisionAnalyzer::analyze(const float* reference,
                                            const float* dut,
                                            size_t length) {
    assert(reference != nullptr && "Reference pointer is null");
    assert(dut != nullptr && "DUT pointer is null");
    assert(length > 0 && "Length must be positive");
    
    PrecisionMetrics metrics;
    metrics.num_samples = length;
    
    // Calculate basic error metrics
    metrics.mae = calculate_mae(reference, dut, length);
    metrics.rmse = calculate_rmse(reference, dut, length);
    metrics.max_error = calculate_max_error(reference, dut, length);
    metrics.snr_db = calculate_snr_db(reference, dut, length);
    
    // Calculate relative errors
    metrics.max_rel_error = calculate_max_relative_error(reference, dut, length);
    
    // Calculate mean relative error
    float sum_rel_error = 0.0f;
    size_t valid_samples = 0;
    for (size_t i = 0; i < length; i++) {
        if (std::abs(reference[i]) > 1e-6f) {
            float rel_err = std::abs(reference[i] - dut[i]) / std::abs(reference[i]);
            sum_rel_error += rel_err * 100.0f;  // Convert to percentage
            valid_samples++;
        }
    }
    metrics.mean_rel_error = (valid_samples > 0) ? 
                             (sum_rel_error / valid_samples) : 0.0f;
    
    // Calculate minimum error
    float min_err = std::numeric_limits<float>::max();
    for (size_t i = 0; i < length; i++) {
        float err = std::abs(reference[i] - dut[i]);
        if (err < min_err) {
            min_err = err;
        }
    }
    metrics.min_error = min_err;
    
    // Find min/max values
    find_min_max(reference, length, metrics.ref_min, metrics.ref_max);
    find_min_max(dut, length, metrics.dut_min, metrics.dut_max);
    
    return metrics;
}

PrecisionMetrics PrecisionAnalyzer::analyze(const std::vector<float>& reference,
                                            const std::vector<float>& dut) {
    assert(reference.size() == dut.size() && "Array size mismatch");
    return analyze(reference.data(), dut.data(), reference.size());
}

PrecisionMetrics PrecisionAnalyzer::analyze_with_thresholds(
    const float* reference,
    const float* dut,
    size_t length,
    const PrecisionThresholds& thresholds) {
    
    PrecisionMetrics metrics = analyze(reference, dut, length);
    
    // Evaluate pass/fail criteria
    bool mae_pass = metrics.mae <= thresholds.max_mae;
    bool rmse_pass = metrics.rmse <= thresholds.max_rmse;
    bool max_err_pass = metrics.max_error <= thresholds.max_error;
    bool rel_err_pass = metrics.max_rel_error <= thresholds.max_rel_error;
    bool snr_pass = metrics.snr_db >= thresholds.min_snr_db;
    
    metrics.passed = mae_pass && rmse_pass && max_err_pass && 
                     rel_err_pass && snr_pass;
    
    // Generate status message
    std::stringstream ss;
    if (metrics.passed) {
        ss << "PASS - All metrics within thresholds";
    } else {
        ss << "FAIL - ";
        std::vector<std::string> failures;
        if (!mae_pass) failures.push_back("MAE");
        if (!rmse_pass) failures.push_back("RMSE");
        if (!max_err_pass) failures.push_back("MaxError");
        if (!rel_err_pass) failures.push_back("RelError");
        if (!snr_pass) failures.push_back("SNR");
        
        for (size_t i = 0; i < failures.size(); i++) {
            ss << failures[i];
            if (i < failures.size() - 1) ss << ", ";
        }
        ss << " exceeded threshold(s)";
    }
    metrics.status_msg = ss.str();
    
    return metrics;
}

// ============================================================================
// Individual Metric Calculations
// ============================================================================

float PrecisionAnalyzer::calculate_mae(const float* reference,
                                       const float* dut,
                                       size_t length) {
    if (length == 0) return 0.0f;
    
    float sum_abs_error = 0.0f;
    for (size_t i = 0; i < length; i++) {
        sum_abs_error += std::abs(reference[i] - dut[i]);
    }
    
    return sum_abs_error / static_cast<float>(length);
}

float PrecisionAnalyzer::calculate_rmse(const float* reference,
                                        const float* dut,
                                        size_t length) {
    if (length == 0) return 0.0f;
    
    float sum_squared_error = 0.0f;
    for (size_t i = 0; i < length; i++) {
        float error = reference[i] - dut[i];
        sum_squared_error += error * error;
    }
    
    float mse = sum_squared_error / static_cast<float>(length);
    return std::sqrt(mse);
}

float PrecisionAnalyzer::calculate_max_error(const float* reference,
                                             const float* dut,
                                             size_t length) {
    if (length == 0) return 0.0f;
    
    float max_err = 0.0f;
    for (size_t i = 0; i < length; i++) {
        float err = std::abs(reference[i] - dut[i]);
        if (err > max_err) {
            max_err = err;
        }
    }
    
    return max_err;
}

float PrecisionAnalyzer::calculate_snr_db(const float* reference,
                                          const float* dut,
                                          size_t length) {
    if (length == 0) return 0.0f;
    
    float signal_power = calculate_signal_power(reference, length);
    float noise_power = calculate_noise_power(reference, dut, length);
    
    // Avoid division by zero or log of zero
    if (noise_power < 1e-30f) {
        return 200.0f;  // Very high SNR (essentially perfect match)
    }
    
    if (signal_power < 1e-30f) {
        return 0.0f;  // No signal
    }
    
    float snr_linear = signal_power / noise_power;
    return 10.0f * std::log10(snr_linear);
}

float PrecisionAnalyzer::calculate_max_relative_error(const float* reference,
                                                       const float* dut,
                                                       size_t length,
                                                       float min_threshold) {
    if (length == 0) return 0.0f;
    
    float max_rel_err = 0.0f;
    for (size_t i = 0; i < length; i++) {
        float ref_abs = std::abs(reference[i]);
        
        // Skip near-zero reference values to avoid numerical issues
        if (ref_abs < min_threshold) {
            continue;
        }
        
        float rel_err = std::abs(reference[i] - dut[i]) / ref_abs;
        float rel_err_pct = rel_err * 100.0f;
        
        if (rel_err_pct > max_rel_err) {
            max_rel_err = rel_err_pct;
        }
    }
    
    return max_rel_err;
}

// ============================================================================
// Helper Functions
// ============================================================================

float PrecisionAnalyzer::calculate_signal_power(const float* signal, 
                                                 size_t length) {
    if (length == 0) return 0.0f;
    
    float sum_squared = 0.0f;
    for (size_t i = 0; i < length; i++) {
        sum_squared += signal[i] * signal[i];
    }
    
    return sum_squared / static_cast<float>(length);
}

float PrecisionAnalyzer::calculate_noise_power(const float* reference,
                                                const float* dut,
                                                size_t length) {
    if (length == 0) return 0.0f;
    
    float sum_squared_error = 0.0f;
    for (size_t i = 0; i < length; i++) {
        float error = reference[i] - dut[i];
        sum_squared_error += error * error;
    }
    
    return sum_squared_error / static_cast<float>(length);
}

void PrecisionAnalyzer::find_min_max(const float* data, size_t length,
                                     float& min_val, float& max_val) {
    if (length == 0) {
        min_val = 0.0f;
        max_val = 0.0f;
        return;
    }
    
    min_val = data[0];
    max_val = data[0];
    
    for (size_t i = 1; i < length; i++) {
        if (data[i] < min_val) min_val = data[i];
        if (data[i] > max_val) max_val = data[i];
    }
}

// ============================================================================
// Reporting Functions
// ============================================================================

std::string PrecisionAnalyzer::generate_report(const PrecisionMetrics& metrics,
                                                bool detailed) {
    std::stringstream ss;
    ss << std::fixed << std::setprecision(6);
    
    ss << "=== Precision Analysis Report ===" << std::endl;
    ss << std::endl;
    
    // Basic error metrics
    ss << "Error Metrics:" << std::endl;
    ss << "  MAE  (Mean Absolute Error):  " << std::setw(12) << metrics.mae << std::endl;
    ss << "  RMSE (Root Mean Square Err): " << std::setw(12) << metrics.rmse << std::endl;
    ss << "  Max Error:                   " << std::setw(12) << metrics.max_error << std::endl;
    
    if (detailed) {
        ss << "  Min Error:                   " << std::setw(12) << metrics.min_error << std::endl;
    }
    
    ss << std::endl;
    
    // Relative error metrics
    ss << "Relative Error Metrics:" << std::endl;
    ss << "  Max Relative Error:          " << std::setw(10) << metrics.max_rel_error << " %" << std::endl;
    ss << "  Mean Relative Error:         " << std::setw(10) << metrics.mean_rel_error << " %" << std::endl;
    ss << std::endl;
    
    // Signal quality
    ss << "Signal Quality:" << std::endl;
    ss << "  SNR:                         " << std::setw(10) << metrics.snr_db << " dB" << std::endl;
    ss << std::endl;
    
    if (detailed) {
        // Sample statistics
        ss << "Sample Statistics:" << std::endl;
        ss << "  Number of samples:           " << metrics.num_samples << std::endl;
        if (metrics.num_saturated > 0) {
            ss << "  Saturated samples:           " << metrics.num_saturated << std::endl;
        }
        ss << std::endl;
        
        // Value ranges
        ss << "Value Ranges:" << std::endl;
        ss << "  Reference: [" << std::setw(10) << metrics.ref_min 
           << ", " << std::setw(10) << metrics.ref_max << "]" << std::endl;
        ss << "  DUT:       [" << std::setw(10) << metrics.dut_min 
           << ", " << std::setw(10) << metrics.dut_max << "]" << std::endl;
        ss << std::endl;
    }
    
    // Pass/Fail status
    if (!metrics.status_msg.empty() && metrics.status_msg != "Not evaluated") {
        ss << "Status: " << metrics.status_msg << std::endl;
    }
    
    return ss.str();
}

void PrecisionAnalyzer::print_metrics(const PrecisionMetrics& metrics,
                                      bool detailed) {
    std::cout << generate_report(metrics, detailed);
}

} // namespace test
} // namespace motorcontrol
} // namespace xf
