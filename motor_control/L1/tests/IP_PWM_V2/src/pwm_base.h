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
#include "common.hpp"

/// \brief template class implementing the pwm inputs

template <typename pwm_data_type>
class pwm_in {
   public:
    pwm_data_type dutycycle;
    pwm_data_type synch_reg_set;
    pwm_data_type flip_counter;
};

class Q16_16 {
   public:
    int32_t value;

    // Default constructor
    constexpr Q16_16() : value(0) {}

    // Constructor from int
    constexpr Q16_16(int32_t val) : value(val << 16) {}

    // Constructor from float
    constexpr Q16_16(float val) : value(static_cast<int32_t>(val * 65536.0f)) {}

    // Constructor from double
    constexpr Q16_16(double val) : value(static_cast<int32_t>(val * 65536.0)) {}

    // Conversion to float
    constexpr operator float() const { return static_cast<float>(value) / 65536.0f; }

    // Conversion to double
    constexpr operator double() const { return static_cast<double>(value) / 65536.0; }

    // Addition operator
    constexpr Q16_16 operator+(const Q16_16& other) const { return Q16_16::fromRaw(value + other.value); }

    // Subtraction operator
    constexpr Q16_16 operator-(const Q16_16& other) const { return Q16_16::fromRaw(value - other.value); }

    // Multiplication operator
    constexpr Q16_16 operator*(const Q16_16& other) const {
        int64_t temp = static_cast<int64_t>(value) * static_cast<int64_t>(other.value);
        return Q16_16::fromRaw(static_cast<int32_t>(temp >> 16));
    }

    // Greater than operator
    constexpr bool operator>(const Q16_16& other) const { return value > other.value; }

    // Less than operator
    constexpr bool operator<(const Q16_16& other) const { return value < other.value; }

    // Factory method to create a Q16_16 from raw value
    static constexpr Q16_16 fromRaw(int32_t rawValue) {
        Q16_16 result;
        result.value = rawValue;
        return result;
    }
};

template <typename sdm_type>
class sdm2ndorder {
   protected:
    sdm_type I1;            // first integrator
    sdm_type I2;            // second integrator
    sdm_type error1;        // error between input and quantized value
    sdm_type error2;        // error between integrator and quantized value
    sdm_type comp;          // quantized value
    sdm_type vref;          // reference voltage
    sdm_type vref_n;        // negative reference voltage
    sdm_type gain;          // Gain of the integrator
    sdm_type offset_error;  // offset error
    uint32_t min_pulse;     // minimum pulse duration
    uint32_t min_pulse_cnt; // minimum pulse duration counter
    bool sdmout;

   public:
    sdm2ndorder(sdm_type vref_i, sdm_type gain_i, uint32_t min_pulse_in) {
        gain = gain_i; // Gain of the integrator
        vref = vref_i;
        vref_n = vref_i * sdm_type(-1);
        min_pulse = min_pulse_in;
        clear();
    }

    sdm2ndorder() { sdm2ndorder(sdm_type(0.32), sdm_type(0.5), 12); }

    bool sd2nd(bool ena, sdm_type in) {
        if (min_pulse_cnt < min_pulse)
            min_pulse_cnt++;
        else {
            min_pulse_cnt = 0;
            error1 = in - comp + offset_error; // error between input and quantized value + offset of the AD7403
            I1 = I1 + gain * error1;           // integration of error1
            error2 = I1 - comp;                // error between integrator and quantized value
            I2 = I2 + gain * error2;           // integration of error2
            comp =
                I2 > sdm_type(0.0) ? vref : vref_n; // quantizer is just a comparator to 0V with +vref or -vref output
            sdmout = I2 > sdm_type(0.0) ? true : false;
        }

        return (sdmout);
    }

    void clear(void) {
        I1 = sdm_type(0);                 // first integrator
        I2 = sdm_type(0);                 // second integrator
        error1 = sdm_type(0);             // error between input and quantized value
        error2 = sdm_type(0);             // error between integrator and quantized value
        comp = sdm_type(1);               // quantized value
        offset_error = sdm_type(0.00002); // offset error typically 1mV (AD7403 Data sheet)
        min_pulse_cnt = 0;
        sdmout = false;
    }

    // Accessor methods
    sdm_type get_I1(void) { return I1; }                     // first integrator
    sdm_type get_I2(void) { return I2; }                     // second integrator
    sdm_type get_error1(void) { return error1; }             // error between input and quantized value
    sdm_type get_error2(void) { return error2; }             // error between integrator and quantized value
    sdm_type get_comp(void) { return comp; }                 // quantized value
    sdm_type get_vref(void) { return vref; }                 // reference voltage
    void set_vref(sdm_type d) { vref = d; }                  // set reference voltage
    sdm_type get_gain(void) { return gain; }                 // Gain of the integrator
    void set_gain(sdm_type d) { gain = d; }                  // set gain voltage
    sdm_type get_offset_error(void) { return offset_error; } // offset_error of the modulator
    void set_offset_error(sdm_type d) { offset_error = d; }  // set offset error
};

/** ---------------------------------------------------------------------------
    /brief Base class for the PWM and PFM modulation

*/

template <class pwm_data_type>
class pwm_base {
   public:
    pwm_data_type counter;     /// PWM & PFM counter for dutycycle
    pwm_data_type counter_pfm; /// PFM counter for synch outputs
    pwm_data_type period;      /// PWM & PFM period expressed in
                               /// number of clock cycles

    pwm_data_type shadow_reg; /// Period shadow register to load the
                              /// new period value in synch with the
                              /// start of the period

    pwm_data_type dutycycle; /// dutycycle for PMW or repetition rate
                             /// for PFM expressed in number of clock
                             /// cycles

    pwm_data_type synch_reg; /// hold a synchronization value that is
                             /// loaded into the counter from an
                             /// external input

    pwm_data_type min_pfm_pulse;   /// minimum pulse duration for PFM in clock cycles
    pwm_data_type min_pfm_counter; /// PFM counter

    pwm_data_type flip_counter; /// counts the flip edges for switching losses

    bool flip;
    bool flip_o;
    bool flip_pfm;
    bool flag;
    bool synch_in;
    bool synchcenter;
    bool synchstart;

    pwm_base() { pwm_clear(); };

    pwm_base(pwm_data_type dutycycle_in) {
        pwm_clear();
        dutycycle = dutycycle_in;
    };

    void pwm_clear() {
        pwm_flush();
        period = 0;
        dutycycle = 0;
        shadow_reg = 0;
        synch_reg = 0;
        min_pfm_pulse = 12;
    };

    void pwm_flush() {
        counter = 0;
        counter_pfm = 0;
        min_pfm_counter = 0;
    };

    void clear_flip_counter() {
        flip_counter = 0;
        flip_o = 0;
    }

    void increment_flip_counter(bool outflip) {
        if (outflip ^ flip_o) {
            flip_o = outflip;
            flip_counter++;
        }
    }

    /**
        Computes the PWM modulation
        @param period determines the period
        @param dutycycle determines the dutycycle
        @param shadow_reg value is loaded into period in synch with the start of the PWM period
        @return bool the PWM output
    */
    void pwm_compute() {
        synchcenter = false;
        synchstart = false;

        if (++counter >= period) {
            counter = 0;
            synchstart = true;
            period = shadow_reg;
        }

        if (synch_in) counter = synch_reg;

        flip = dutycycle > counter;
    }

    /**
        Computes the PFM modulation
        @param period determines the cycle
        @param dutycycle determines the ratio of pulses per period
        @param shadow_reg value is loaded into period in synch with the start of the PFM period
        @return bool the PWM output
    */
    void pfm_compute() {
        pwm_symmetric(); // for synch start and center

        if (min_pfm_counter < min_pfm_pulse)
            min_pfm_counter++;
        else {
            min_pfm_counter = 0;
            counter_pfm += dutycycle;
            if (counter_pfm < period) {
                flip_pfm = false;
            } else {
                counter_pfm -= period;
                period = shadow_reg;
                flip_pfm = true;
            }
        }

        if (synch_in) counter_pfm = synch_reg;

        // return flip_pfm;
    }

    /**
         Computes the symmetric PWM modulation
         @param PWM period is 2 times the value contained in the period variable
         @param dutycycle determines the dutycycle
         @param shadow_reg value is loaded into period in synch with the start of the PWM period
         @return bool the PWM output
     */
    void pwm_symmetric() {
        synchcenter = false;
        synchstart = false;

        if (!flag) {
            if (++counter >= period) {
                synchcenter = true;
                flag = true;
            }
        } else {
            if (--counter == 0) {
                period = shadow_reg;
                synchstart = true;
                flag = false;
            }
        }

        if (synch_in) counter = synch_reg;

        flip = dutycycle < counter;
    }
};

/** ---------------------------------------------------------------------------
   \brief Primary interface class for the PWM and PFM modulation

*/
enum SELECT { NONE_MODE = 0, NORM = 1, CENTER = 2, PFM = 3, SIGMADELTA = 4, STREAM = 8, FLUSH = 16, FLIPCNTCLEAR = 32 };

template <class pwm_data_type>
class pwm_engine {
   public:
    pwm_base<pwm_data_type> pwm_primary;
    sdm2ndorder<Q16_16> modulator;
    uint32_t control; /// Control register
                      /// Bit 0
                      ///   0  pwm
                      ///   1  pseudo random pwm
    bool flip;

    pwm_engine()
        : modulator(Q16_16(0.32), Q16_16(0.5), 12),

          control(NONE_MODE),
          flip(false)

    {}

    /**
        Main PWM function
        @param tbtcl bit 0 = 0 standard PWM; bit 0 = 1 pulse frequency modulator (PFM).
        @return bool the PWM output
        The prescaler variable determines the timebase (how frequent) the PWM is called
    */
    void pwm(void) {
        switch (control & 0x7) {
            case NORM:
                pwm_primary.pwm_compute();
                flip = pwm_primary.flip;
                pwm_primary.increment_flip_counter(flip);
                break;
            case CENTER:
                pwm_primary.pwm_symmetric();
                flip = pwm_primary.flip;
                pwm_primary.increment_flip_counter(flip);
                break;
            case PFM:
                pwm_primary.pfm_compute();
                flip = pwm_primary.flip;
                pwm_primary.increment_flip_counter(flip);
                break;
            case SIGMADELTA:
                pwm_primary.pwm_symmetric();
                flip = modulator.sd2nd(true, pwm_primary.dutycycle);
                pwm_primary.increment_flip_counter(flip);
                break;
            default:
                pwm_primary.pwm_clear();
                break;
        }
    }
};
