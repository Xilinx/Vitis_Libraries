/*
Copyright (C) 2025-2025, Advanced Micro Devices, Inc.
SPDX-License-Identifier: X11
*/

/**
 * @file hls_test_utils.hpp
 * @brief Common utilities for Phase 2 HLS testing
 * 
 * This header provides reusable utilities for all Phase 2 HLS testbenches:
 *   - Precision metrics calculation
 *   - Test stimulus generation
 *   - Error reporting
 *   - Performance analysis helpers
 */

#pragma once

#include <cmath>
#include <vector>
#include <iostream>
#include <iomanip>
#include <algorithm>

namespace xf {
namespace motorcontrol {
namespace test {

// ============================================================================
// Constants
// ============================================================================

constexpr float PI = 3.14159265359f;
constexpr double PI_D = 3.14159265358979323846;

// Default precision thresholds
constexpr float DEFAULT_FP32_THRESHOLD = 1e-6f;   // FP32 machine epsilon
constexpr double DEFAULT_FP64_THRESHOLD = 1e-12;  // FP64 machine epsilon

// ============================================================================
// Precision Metrics
// ============================================================================

/**
 * @brief Precision metrics for comparing DUT vs Golden
 */
template<typename T>
struct PrecisionMetrics {
    T max_absolute_error;
    T mean_absolute_error;
    T root_mean_square_error;
    double snr_db;  // Signal-to-Noise Ratio in dB
    
    int num_samples;
    int num_passed;
    T threshold;
    
    PrecisionMetrics(T thresh = DEFAULT_FP32_THRESHOLD)
        : max_absolute_error(0), mean_absolute_error(0),
          root_mean_square_error(0), snr_db(0),
          num_samples(0), num_passed(0), threshold(thresh) {}
    
    /**
     * @brief Add a single comparison sample
     */
    void add_sample(T dut_value, T golden_value) {
        T error = std::abs(dut_value - golden_value);
        
        // Update max error
        max_absolute_error = std::max(max_absolute_error, error);
        
        // Accumulate for mean/RMS
        mean_absolute_error += error;
        root_mean_square_error += error * error;
        
        // Check against threshold
        num_samples++;
        if (error < threshold) {
            num_passed++;
        }
    }
    
    /**
     * @brief Compute SNR from accumulated signal and noise power
     */
    void compute_snr(const std::vector<T>& golden_values,
                     const std::vector<T>& dut_values) {
        if (golden_values.size() != dut_values.size() || golden_values.empty()) {
            snr_db = 0;
            return;
        }
        
        double signal_power = 0;
        double noise_power = 0;
        
        for (size_t i = 0; i < golden_values.size(); i++) {
            double signal = static_cast<double>(golden_values[i]);
            double noise = static_cast<double>(golden_values[i] - dut_values[i]);
            
            signal_power += signal * signal;
            noise_power += noise * noise;
        }
        
        if (noise_power > 0 && signal_power > 0) {
            snr_db = 10.0 * std::log10(signal_power / noise_power);
        } else {
            snr_db = 200.0;  // Effectively infinite SNR
        }
    }
    
    /**
     * @brief Finalize metrics (compute means)
     */
    void finalize() {
        if (num_samples > 0) {
            mean_absolute_error /= num_samples;
            root_mean_square_error = std::sqrt(root_mean_square_error / num_samples);
        }
    }
    
    /**
     * @brief Print summary report
     */
    void print(const std::string& metric_name = "Metric") const {
        std::cout << std::fixed;
        std::cout << "  " << std::setw(12) << std::left << metric_name << ": ";
        std::cout << "MAE=" << std::scientific << std::setprecision(3) << mean_absolute_error << " ";
        std::cout << "RMSE=" << std::scientific << std::setprecision(3) << root_mean_square_error << " ";
        std::cout << "MaxErr=" << std::scientific << std::setprecision(3) << max_absolute_error;
        if (snr_db > 0) {
            std::cout << " SNR=" << std::fixed << std::setprecision(2) << snr_db << "dB";
        }
        std::cout << "\n";
    }
    
    /**
     * @brief Check if all samples passed threshold
     */
    bool all_passed() const {
        return num_passed == num_samples;
    }
    
    /**
     * @brief Get pass rate percentage
     */
    float pass_rate() const {
        return num_samples > 0 ? (100.0f * num_passed / num_samples) : 0.0f;
    }
};

// ============================================================================
// Multi-Output Metrics (for transforms with multiple outputs)
// ============================================================================

/**
 * @brief Metrics aggregator for functions with multiple outputs
 */
template<typename T>
struct MultiOutputMetrics {
    std::vector<PrecisionMetrics<T>> output_metrics;
    std::vector<std::string> output_names;
    
    MultiOutputMetrics(const std::vector<std::string>& names, T threshold = DEFAULT_FP32_THRESHOLD)
        : output_names(names) {
        for (size_t i = 0; i < names.size(); i++) {
            output_metrics.emplace_back(threshold);
        }
    }
    
    void finalize() {
        for (auto& m : output_metrics) {
            m.finalize();
        }
    }
    
    void print_summary() const {
        std::cout << "\n========================================\n";
        std::cout << "Precision Analysis Summary\n";
        std::cout << "========================================\n";
        
        for (size_t i = 0; i < output_metrics.size(); i++) {
            output_metrics[i].print(output_names[i]);
        }
        
        std::cout << "\nOverall:\n";
        int total_samples = output_metrics[0].num_samples * output_metrics.size();
        int total_passed = 0;
        for (const auto& m : output_metrics) {
            total_passed += m.num_passed;
        }
        
        std::cout << "  Tests:      " << output_metrics[0].num_samples << "\n";
        std::cout << "  Outputs:    " << output_metrics.size() << "\n";
        std::cout << "  Pass rate:  " << std::fixed << std::setprecision(2)
                  << (100.0f * total_passed / total_samples) << "%\n";
        std::cout << "  Threshold:  " << std::scientific << output_metrics[0].threshold << "\n";
        std::cout << "========================================\n";
    }
    
    bool all_passed() const {
        for (const auto& m : output_metrics) {
            if (!m.all_passed()) return false;
        }
        return true;
    }
};

// ============================================================================
// Stimulus Generators
// ============================================================================

/**
 * @brief Generate balanced 3-phase currents for a given angle
 */
template<typename T>
void generate_3phase_balanced(T theta, T amplitude, T& ia, T& ib, T& ic) {
    ia = amplitude * std::cos(theta);
    ib = amplitude * std::cos(theta - static_cast<T>(2.0 * PI / 3.0));
    ic = amplitude * std::cos(theta + static_cast<T>(2.0 * PI / 3.0));
}

/**
 * @brief Generate rotating dq vector for a given angle
 */
template<typename T>
void generate_dq_rotating(T theta, T amplitude, T& id, T& iq) {
    id = amplitude * std::cos(theta);
    iq = amplitude * std::sin(theta);
}

/**
 * @brief Generate sine and cosine for Park transform
 */
template<typename T>
void generate_sin_cos(T theta, T& sin_theta, T& cos_theta) {
    sin_theta = std::sin(theta);
    cos_theta = std::cos(theta);
}

// ============================================================================
// Test Reporting
// ============================================================================

/**
 * @brief Print test header
 */
inline void print_test_header(const std::string& test_name, const std::string& description = "") {
    std::cout << "\n========================================\n";
    std::cout << test_name << "\n";
    if (!description.empty()) {
        std::cout << description << "\n";
    }
    std::cout << "========================================\n\n";
}

/**
 * @brief Print test footer with pass/fail status
 */
inline void print_test_result(bool passed, int num_tests = 0) {
    std::cout << "\n----------------------------------------\n";
    if (passed) {
        std::cout << "✓ PASSED";
    } else {
        std::cout << "✗ FAILED";
    }
    if (num_tests > 0) {
        std::cout << " (" << num_tests << " tests)";
    }
    std::cout << "\n----------------------------------------\n";
}

/**
 * @brief Print final summary
 */
inline void print_final_summary(bool all_passed) {
    std::cout << "\n========================================\n";
    if (all_passed) {
        std::cout << "✓✓✓ ALL TESTS PASSED ✓✓✓\n";
    } else {
        std::cout << "✗✗✗ SOME TESTS FAILED ✗✗✗\n";
    }
    std::cout << "========================================\n\n";
}

} // namespace test
} // namespace motorcontrol
} // namespace xf
