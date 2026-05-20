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
#include <random>
#include <algorithm>

namespace xf {
namespace motorcontrol {
namespace test {

/**
 * @brief Mathematical constants
 */
constexpr float PI = 3.14159265358979323846f;
constexpr float TWO_PI = 2.0f * PI;
constexpr float PHASE_120_DEG = 2.0f * PI / 3.0f;  // 120 degrees in radians

/**
 * @brief Three-phase signal structure
 * 
 * Represents three-phase electrical signals (Ia, Ib, Ic)
 * commonly used in motor control applications
 */
struct ThreePhaseSignal {
    float ia;  // Phase A current
    float ib;  // Phase B current
    float ic;  // Phase C current
    
    ThreePhaseSignal() : ia(0.0f), ib(0.0f), ic(0.0f) {}
    ThreePhaseSignal(float a, float b, float c) : ia(a), ib(b), ic(c) {}
};

/**
 * @brief Two-phase signal structure (alpha-beta frame)
 * 
 * Represents signals in the stationary orthogonal frame
 */
struct TwoPhaseSignal {
    float alpha;
    float beta;
    
    TwoPhaseSignal() : alpha(0.0f), beta(0.0f) {}
    TwoPhaseSignal(float a, float b) : alpha(a), beta(b) {}
};

/**
 * @brief DQ-frame signal structure (rotating frame)
 * 
 * Represents signals in the synchronous rotating frame
 */
struct DQSignal {
    float d;  // Direct axis
    float q;  // Quadrature axis
    
    DQSignal() : d(0.0f), q(0.0f) {}
    DQSignal(float direct, float quad) : d(direct), q(quad) {}
};

/**
 * @brief Stimulus Generator - Test signal generation for Phase 1 validation
 * 
 * Provides various signal generation patterns for testing motor control algorithms.
 * All signals are generated as float32. Does not call DUT or Golden Model.
 * 
 * Supported signal types:
 * - Random (uniform, normal distribution)
 * - Sinusoidal (single phase, configurable amplitude and frequency)
 * - Three-phase (120° phase shift, balanced system)
 * - Sweep (frequency sweep)
 * - Step / Ramp
 */
class StimulusGenerator {
public:
    /**
     * @brief Constructor with optional seed for reproducibility
     * 
     * @param seed Random seed (0 = use random_device)
     */
    StimulusGenerator(unsigned int seed = 0) {
        if (seed == 0) {
            std::random_device rd;
            rng_.seed(rd());
        } else {
            rng_.seed(seed);
        }
    }
    
    // ========================================================================
    // Random Signal Generation
    // ========================================================================
    
    /**
     * @brief Generate uniform random value in range [min, max]
     * 
     * @param min Minimum value
     * @param max Maximum value
     * @return float Random value
     */
    float random_uniform(float min = -1.0f, float max = 1.0f) {
        std::uniform_real_distribution<float> dist(min, max);
        return dist(rng_);
    }
    
    /**
     * @brief Generate normal distributed random value
     * 
     * @param mean Mean value
     * @param stddev Standard deviation
     * @return float Random value
     */
    float random_normal(float mean = 0.0f, float stddev = 1.0f) {
        std::normal_distribution<float> dist(mean, stddev);
        return dist(rng_);
    }
    
    /**
     * @brief Generate array of uniform random values
     * 
     * @param count Number of samples
     * @param min Minimum value
     * @param max Maximum value
     * @return std::vector<float> Random values
     */
    std::vector<float> random_uniform_array(size_t count, 
                                            float min = -1.0f, 
                                            float max = 1.0f) {
        std::vector<float> result(count);
        for (size_t i = 0; i < count; i++) {
            result[i] = random_uniform(min, max);
        }
        return result;
    }
    
    /**
     * @brief Generate array of normal distributed random values
     * 
     * @param count Number of samples
     * @param mean Mean value
     * @param stddev Standard deviation
     * @return std::vector<float> Random values
     */
    std::vector<float> random_normal_array(size_t count,
                                           float mean = 0.0f,
                                           float stddev = 1.0f) {
        std::vector<float> result(count);
        for (size_t i = 0; i < count; i++) {
            result[i] = random_normal(mean, stddev);
        }
        return result;
    }
    
    // ========================================================================
    // Sinusoidal Signal Generation
    // ========================================================================
    
    /**
     * @brief Generate sinusoidal value at specific time
     * 
     * Formula: y = A * sin(2π * f * t + φ) + offset
     * 
     * @param time Time in seconds
     * @param amplitude Peak amplitude
     * @param frequency Frequency in Hz
     * @param phase Phase offset in radians (default: 0)
     * @param offset DC offset (default: 0)
     * @return float Signal value
     */
    static float sine_wave(float time,
                          float amplitude,
                          float frequency,
                          float phase = 0.0f,
                          float offset = 0.0f) {
        return amplitude * std::sin(TWO_PI * frequency * time + phase) + offset;
    }
    
    /**
     * @brief Generate sinusoidal waveform array
     * 
     * @param sample_rate Sampling rate in Hz
     * @param duration Duration in seconds
     * @param amplitude Peak amplitude
     * @param frequency Signal frequency in Hz
     * @param phase Phase offset in radians (default: 0)
     * @param offset DC offset (default: 0)
     * @return std::vector<float> Signal samples
     */
    static std::vector<float> sine_wave_array(float sample_rate,
                                               float duration,
                                               float amplitude,
                                               float frequency,
                                               float phase = 0.0f,
                                               float offset = 0.0f) {
        size_t num_samples = static_cast<size_t>(sample_rate * duration);
        std::vector<float> result(num_samples);
        
        float dt = 1.0f / sample_rate;
        for (size_t i = 0; i < num_samples; i++) {
            float t = i * dt;
            result[i] = sine_wave(t, amplitude, frequency, phase, offset);
        }
        return result;
    }
    
    // ========================================================================
    // Three-Phase Signal Generation
    // ========================================================================
    
    /**
     * @brief Generate balanced three-phase signals at specific time
     * 
     * Phase relationships:
     * - Phase A: 0°
     * - Phase B: -120° (lagging)
     * - Phase C: +120° (leading) or -240°
     * 
     * Satisfies: Ia + Ib + Ic = 0 (balanced system)
     * 
     * @param time Time in seconds
     * @param amplitude Peak amplitude
     * @param frequency Frequency in Hz
     * @param phase_offset Phase offset for phase A in radians (default: 0)
     * @return ThreePhaseSignal Three-phase currents
     */
    static ThreePhaseSignal three_phase_sine(float time,
                                             float amplitude,
                                             float frequency,
                                             float phase_offset = 0.0f) {
        ThreePhaseSignal signal;
        
        // Phase A: reference phase
        signal.ia = sine_wave(time, amplitude, frequency, phase_offset);
        
        // Phase B: -120° (2π/3 radians behind phase A)
        signal.ib = sine_wave(time, amplitude, frequency, 
                             phase_offset - PHASE_120_DEG);
        
        // Phase C: +120° (2π/3 radians ahead of phase A, or -240°)
        signal.ic = sine_wave(time, amplitude, frequency, 
                             phase_offset + PHASE_120_DEG);
        
        return signal;
    }
    
    /**
     * @brief Generate balanced three-phase waveform arrays
     * 
     * @param sample_rate Sampling rate in Hz
     * @param duration Duration in seconds
     * @param amplitude Peak amplitude
     * @param frequency Signal frequency in Hz
     * @param phase_offset Phase offset in radians (default: 0)
     * @return std::vector<ThreePhaseSignal> Three-phase signal samples
     */
    static std::vector<ThreePhaseSignal> three_phase_sine_array(
        float sample_rate,
        float duration,
        float amplitude,
        float frequency,
        float phase_offset = 0.0f) {
        
        size_t num_samples = static_cast<size_t>(sample_rate * duration);
        std::vector<ThreePhaseSignal> result(num_samples);
        
        float dt = 1.0f / sample_rate;
        for (size_t i = 0; i < num_samples; i++) {
            float t = i * dt;
            result[i] = three_phase_sine(t, amplitude, frequency, phase_offset);
        }
        return result;
    }
    
    /**
     * @brief Generate random three-phase signal (balanced)
     * 
     * Uses random amplitude and phase, maintains 120° phase relationships
     * 
     * @param amplitude_min Minimum amplitude
     * @param amplitude_max Maximum amplitude
     * @return ThreePhaseSignal Random three-phase signal
     */
    ThreePhaseSignal random_three_phase(float amplitude_min = -100.0f,
                                        float amplitude_max = 100.0f) {
        float amp = random_uniform(amplitude_min, amplitude_max);
        float phase = random_uniform(0.0f, TWO_PI);
        return three_phase_sine(0.0f, amp, 50.0f, phase);  // Arbitrary frequency
    }
    
    // ========================================================================
    // Step and Ramp Signals
    // ========================================================================
    
    /**
     * @brief Generate step signal
     * 
     * @param time Current time
     * @param step_time Time of step
     * @param initial_value Value before step
     * @param final_value Value after step
     * @return float Signal value
     */
    static float step_signal(float time,
                            float step_time,
                            float initial_value = 0.0f,
                            float final_value = 1.0f) {
        return (time < step_time) ? initial_value : final_value;
    }
    
    /**
     * @brief Generate ramp signal
     * 
     * @param time Current time
     * @param start_time Ramp start time
     * @param end_time Ramp end time
     * @param initial_value Value at start
     * @param final_value Value at end
     * @return float Signal value
     */
    static float ramp_signal(float time,
                            float start_time,
                            float end_time,
                            float initial_value = 0.0f,
                            float final_value = 1.0f) {
        if (time < start_time) {
            return initial_value;
        } else if (time > end_time) {
            return final_value;
        } else {
            float t_norm = (time - start_time) / (end_time - start_time);
            return initial_value + t_norm * (final_value - initial_value);
        }
    }
    
    // ========================================================================
    // Utility Functions
    // ========================================================================
    
    /**
     * @brief Generate DC constant value array
     * 
     * @param count Number of samples
     * @param value Constant value
     * @return std::vector<float> Constant array
     */
    static std::vector<float> constant_array(size_t count, float value) {
        return std::vector<float>(count, value);
    }
    
    /**
     * @brief Generate linearly spaced array
     * 
     * @param start Start value
     * @param end End value
     * @param count Number of samples
     * @return std::vector<float> Linearly spaced values
     */
    static std::vector<float> linspace(float start, float end, size_t count) {
        std::vector<float> result(count);
        if (count == 1) {
            result[0] = start;
        } else {
            float step = (end - start) / (count - 1);
            for (size_t i = 0; i < count; i++) {
                result[i] = start + i * step;
            }
        }
        return result;
    }
    
    /**
     * @brief Add noise to signal
     * 
     * @param signal Input signal
     * @param noise_stddev Standard deviation of Gaussian noise
     * @return std::vector<float> Noisy signal
     */
    std::vector<float> add_noise(const std::vector<float>& signal,
                                 float noise_stddev) {
        std::vector<float> result(signal.size());
        for (size_t i = 0; i < signal.size(); i++) {
            result[i] = signal[i] + random_normal(0.0f, noise_stddev);
        }
        return result;
    }
    
    /**
     * @brief Clip signal to range
     * 
     * @param signal Input signal
     * @param min_val Minimum value
     * @param max_val Maximum value
     * @return std::vector<float> Clipped signal
     */
    static std::vector<float> clip_signal(const std::vector<float>& signal,
                                          float min_val,
                                          float max_val) {
        std::vector<float> result(signal.size());
        for (size_t i = 0; i < signal.size(); i++) {
            result[i] = std::max(min_val, std::min(max_val, signal[i]));
        }
        return result;
    }
    
    /**
     * @brief Scale signal by factor
     * 
     * @param signal Input signal
     * @param scale Scale factor
     * @return std::vector<float> Scaled signal
     */
    static std::vector<float> scale_signal(const std::vector<float>& signal,
                                           float scale) {
        std::vector<float> result(signal.size());
        for (size_t i = 0; i < signal.size(); i++) {
            result[i] = signal[i] * scale;
        }
        return result;
    }

private:
    std::mt19937 rng_;  // Random number generator
};

/**
 * @brief Preset signal configurations for common test scenarios
 */
namespace presets {

/**
 * @brief Typical motor rated current test scenario
 * 
 * - Amplitude: 10 A (typical rated current)
 * - Frequency: 50 Hz (grid frequency)
 */
struct MotorRatedCurrent {
    static constexpr float amplitude = 10.0f;
    static constexpr float frequency = 50.0f;
    
    static ThreePhaseSignal generate(float time, float phase = 0.0f) {
        return StimulusGenerator::three_phase_sine(time, amplitude, frequency, phase);
    }
};

/**
 * @brief Low speed operation test scenario
 * 
 * - Amplitude: 5 A
 * - Frequency: 10 Hz (low speed)
 */
struct LowSpeed {
    static constexpr float amplitude = 5.0f;
    static constexpr float frequency = 10.0f;
    
    static ThreePhaseSignal generate(float time, float phase = 0.0f) {
        return StimulusGenerator::three_phase_sine(time, amplitude, frequency, phase);
    }
};

/**
 * @brief High speed operation test scenario
 * 
 * - Amplitude: 15 A
 * - Frequency: 200 Hz (high speed)
 */
struct HighSpeed {
    static constexpr float amplitude = 15.0f;
    static constexpr float frequency = 200.0f;
    
    static ThreePhaseSignal generate(float time, float phase = 0.0f) {
        return StimulusGenerator::three_phase_sine(time, amplitude, frequency, phase);
    }
};

} // namespace presets

} // namespace test
} // namespace motorcontrol
} // namespace xf
