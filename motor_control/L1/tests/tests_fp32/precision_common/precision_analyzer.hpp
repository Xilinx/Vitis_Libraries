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

#pragma once

#include <vector>
#include <cmath>
#include <limits>
#include <string>
#include <algorithm>

namespace xf {
namespace motorcontrol {
namespace test {

/**
 * @brief Precision metrics structure
 * 
 * Contains all statistical error metrics for DUT vs Golden comparison
 */
struct PrecisionMetrics {
    // Basic error metrics
    float mae;              // Mean Absolute Error
    float rmse;             // Root Mean Square Error
    float max_error;        // Maximum absolute error
    float min_error;        // Minimum absolute error
    
    // Signal quality metrics
    float snr_db;           // Signal-to-Noise Ratio in dB
    
    // Relative error metrics
    float max_rel_error;    // Maximum relative error (%)
    float mean_rel_error;   // Mean relative error (%)
    
    // Sample statistics
    size_t num_samples;     // Number of samples analyzed
    size_t num_saturated;   // Number of saturated samples (if applicable)
    
    // Range information
    float ref_min;          // Minimum reference value
    float ref_max;          // Maximum reference value
    float dut_min;          // Minimum DUT value
    float dut_max;          // Maximum DUT value
    
    // Pass/Fail status
    bool passed;            // Overall pass/fail based on thresholds
    std::string status_msg; // Status message
    
    /**
     * @brief Default constructor
     */
    PrecisionMetrics() 
        : mae(0.0f), rmse(0.0f), max_error(0.0f), min_error(0.0f),
          snr_db(0.0f), max_rel_error(0.0f), mean_rel_error(0.0f),
          num_samples(0), num_saturated(0),
          ref_min(0.0f), ref_max(0.0f), dut_min(0.0f), dut_max(0.0f),
          passed(false), status_msg("Not evaluated") {}
};

/**
 * @brief Threshold configuration for pass/fail criteria
 */
struct PrecisionThresholds {
    float max_mae;          // Maximum acceptable MAE
    float max_rmse;         // Maximum acceptable RMSE
    float max_error;        // Maximum acceptable absolute error
    float max_rel_error;    // Maximum acceptable relative error (%)
    float min_snr_db;       // Minimum acceptable SNR (dB)
    
    /**
     * @brief Default constructor with typical values for motor control
     */
    PrecisionThresholds()
        : max_mae(0.01f), max_rmse(0.01f), max_error(0.1f),
          max_rel_error(1.0f), min_snr_db(60.0f) {}
    
    /**
     * @brief Custom constructor
     */
    PrecisionThresholds(float mae, float rmse, float max_err, 
                       float rel_err, float snr)
        : max_mae(mae), max_rmse(rmse), max_error(max_err),
          max_rel_error(rel_err), min_snr_db(snr) {}
};

/**
 * @brief Precision Analyzer - Algorithm-agnostic error analysis
 * 
 * Provides comprehensive statistical comparison between reference (golden)
 * and DUT output values. Suitable for all motor control IP blocks.
 */
class PrecisionAnalyzer {
public:
    /**
     * @brief Analyze precision between reference and DUT values
     * 
     * @param reference Golden model output values (float array)
     * @param dut DUT output values (float array)
     * @param length Number of samples
     * @return PrecisionMetrics Complete set of error metrics
     * 
     * Example:
     *   float ref[] = {1.0, 2.0, 3.0};
     *   float dut[] = {1.01, 1.99, 3.02};
     *   auto metrics = PrecisionAnalyzer::analyze(ref, dut, 3);
     */
    static PrecisionMetrics analyze(const float* reference, 
                                    const float* dut, 
                                    size_t length);
    
    /**
     * @brief Analyze precision with vector interface
     * 
     * @param reference Golden model output values
     * @param dut DUT output values
     * @return PrecisionMetrics Complete set of error metrics
     */
    static PrecisionMetrics analyze(const std::vector<float>& reference,
                                    const std::vector<float>& dut);
    
    /**
     * @brief Analyze and apply pass/fail thresholds
     * 
     * @param reference Golden model output values
     * @param dut DUT output values
     * @param length Number of samples
     * @param thresholds Pass/fail criteria
     * @return PrecisionMetrics Metrics with pass/fail evaluation
     */
    static PrecisionMetrics analyze_with_thresholds(
        const float* reference, 
        const float* dut,
        size_t length,
        const PrecisionThresholds& thresholds = PrecisionThresholds());
    
    /**
     * @brief Calculate Mean Absolute Error (MAE)
     * 
     * MAE = (1/N) * Σ|reference[i] - dut[i]|
     * 
     * @param reference Reference values
     * @param dut DUT values
     * @param length Number of samples
     * @return float MAE value
     */
    static float calculate_mae(const float* reference, 
                               const float* dut, 
                               size_t length);
    
    /**
     * @brief Calculate Root Mean Square Error (RMSE)
     * 
     * RMSE = sqrt((1/N) * Σ(reference[i] - dut[i])²)
     * 
     * @param reference Reference values
     * @param dut DUT values
     * @param length Number of samples
     * @return float RMSE value
     */
    static float calculate_rmse(const float* reference, 
                                const float* dut, 
                                size_t length);
    
    /**
     * @brief Calculate Maximum Absolute Error
     * 
     * Max Error = max(|reference[i] - dut[i]|)
     * 
     * @param reference Reference values
     * @param dut DUT values
     * @param length Number of samples
     * @return float Maximum error value
     */
    static float calculate_max_error(const float* reference, 
                                     const float* dut, 
                                     size_t length);
    
    /**
     * @brief Calculate Signal-to-Noise Ratio (SNR) in dB
     * 
     * SNR = 10 * log10(Σ(reference²) / Σ(error²))
     * 
     * @param reference Reference values (signal)
     * @param dut DUT values
     * @param length Number of samples
     * @return float SNR in decibels
     */
    static float calculate_snr_db(const float* reference, 
                                   const float* dut, 
                                   size_t length);
    
    /**
     * @brief Calculate maximum relative error percentage
     * 
     * Rel Error = 100 * |reference - dut| / |reference|
     * 
     * @param reference Reference values
     * @param dut DUT values
     * @param length Number of samples
     * @param min_threshold Minimum reference value to consider (avoid divide by zero)
     * @return float Maximum relative error in percentage
     */
    static float calculate_max_relative_error(const float* reference,
                                               const float* dut,
                                               size_t length,
                                               float min_threshold = 1e-6f);
    
    /**
     * @brief Generate human-readable report string
     * 
     * @param metrics Metrics to report
     * @param detailed Include detailed statistics
     * @return std::string Formatted report
     */
    static std::string generate_report(const PrecisionMetrics& metrics,
                                       bool detailed = true);
    
    /**
     * @brief Print metrics to console
     * 
     * @param metrics Metrics to print
     * @param detailed Include detailed statistics
     */
    static void print_metrics(const PrecisionMetrics& metrics,
                             bool detailed = true);

private:
    // Helper: Calculate signal power
    static float calculate_signal_power(const float* signal, size_t length);
    
    // Helper: Calculate noise power (error power)
    static float calculate_noise_power(const float* reference, 
                                       const float* dut, 
                                       size_t length);
    
    // Helper: Find min/max in array
    static void find_min_max(const float* data, size_t length,
                            float& min_val, float& max_val);
};

} // namespace test
} // namespace motorcontrol
} // namespace xf
