/*
Copyright (C) 2022-2023, Advanced Micro Devices, Inc.
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
#ifndef _PID_CONTROL_HPP_
#define _PID_CONTROL_HPP_

#include "ap_int.h"
#include "ap_fixed.h"
#include "hls_stream.h"
#include <array>
#include <algorithm>

#define COUNT_X1 0
#define COUNT_X2 1
namespace xf {
namespace motorcontrol {
namespace details {
typedef ap_uint<1> bit;

enum enc_pos_mode {

    DEGREES = 0,
    RADIANS = 1

};

enum enc_count_mode {

    X1 = 0,
    X2 = 1,
    X4 = 2

};

template <typename T_QEI_COUNTER, typename T_ANGLE, typename T_VELOCITY>
class QEI {
   public:
    QEI() : posCount_(0), revCount_(0), QAp_(false), QBp_(false), QIp_(false), countDirection_(true) {}

    void update(bool QAi, bool QBi, bool QIi, bool B_leading_A) {
        // QAi and QBi are the inputs QAp and QBp are the previous inputs.
        //  The following table represents the states and the action to the counter
        // QAi QBi QAp QBp UP DW Er Action
        // 1   1    1   0  1  0  0   Count up
        // 1   0    0   0  1  0  0   Count up
        // 0   1    1   1  1  0  0   Count up
        // 0   0    0   1  1  0  0   Count up

        // 1   1    0   1  0  1  0   Count down
        // 1   0    1   1  0  1  0   Count down
        // 0   1    0   0  0  1  0   Count down
        // 0   0    1   0  0  1  0   Count down

        // 1   1    0   0  0  0  1   Invalid state change; ignore
        // 1   0    0   1  0  0  1   Invalid state change; ignore
        // 0   1    1   0  0  0  1   Invalid state change; ignore
        // 0   0    1   1  0  0  1   Invalid state change; ignore

        // 1   0    1   0  0  0  0   No count or direction change
        // 0   1    0   1  0  0  0   No count or direction change
        // 0   0    0   0  0  0  0   No count or direction change
        // 1   1    1   1  0  0  0   No count or direction change

        // A Leading B Config:           0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15
        static bool def_count_up[16] = {0, 1, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 1, 0};
        static bool def_count_dw[16] = {0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0};
        bool *count_up = def_count_up, *count_dw = def_count_dw;
        if (B_leading_A) {
            std::swap(count_up, count_dw);
        }
        static bool err_qei[16] = {0, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 0};
        static bool idle_qei[16] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};

        // status of the QEI
        T_QEI_COUNTER pnt =
            (T_QEI_COUNTER)QAi << 3 | (T_QEI_COUNTER)QBi << 2 | (T_QEI_COUNTER)QAp_ << 1 | (T_QEI_COUNTER)QBp_;

        // Update position count (from 0 to CPR)
        posCount_ = (!QIp_ & QIi) ? 0 : posCount_ + count_up[pnt] - count_dw[pnt];
        // Update revolution count
        revCount_ = (!QIp_ & QIi) ? (countDirection_ ? revCount_ + 1 : revCount_ - 1) : revCount_;
        // Check whether the spinning direction is positive or negative
        countDirection_ = (countDirection_ & !count_dw[pnt]) | count_up[pnt];

        counting_a = (!QAp_ & QAi) | QAi;

        if (counting_a) {
            if (countDirection_) {
                ++counter_divider_a;
            } else {
                --counter_divider_a;
            }
        }

        divide_enable_a = ((counter_divider_a != 0) && !counting_a) ? true : false;

        if (divide_enable_a) {
            m_internal_velocity_a = RPM_factor_a / counter_divider_a;
            divide_enable_a = false;
            counter_divider_a = 0;
        }

        counting_b = (QAp_ & !QAi) | !QAi;

        if (counting_b) {
            if (countDirection_) {
                ++counter_divider_b;
            } else {
                --counter_divider_b;
            }
        }

        divide_enable_b = ((counter_divider_b != 0) && !counting_b) ? true : false;

        if (divide_enable_b) {
            m_internal_velocity_b = RPM_factor_b / counter_divider_b;
            divide_enable_b = false;
            counter_divider_b = 0;
        }

        // update internal state variables
        QAp_ = QAi;
        QBp_ = QBi;
        QIp_ = QIi;
        m_error = err_qei[pnt];
        m_velocity = (counting_a == true) ? m_internal_velocity_b : m_internal_velocity_a;
        m_revCount_angle = (T_ANGLE)revCount_;
        m_posCount_angle = (T_ANGLE)posCount_;

    } // End update

    // return the counter for encoder position
    T_QEI_COUNTER getCounter() const { return posCount_; }
    // set the counter for encoder position
    void setCounter(T_QEI_COUNTER val) { posCount_ = val; }
    // return the number of revolution of the encoder
    T_QEI_COUNTER getRevolutions() const { return revCount_; }
    // set the number of revolution of the encoder
    void setRevolutions(T_QEI_COUNTER val) { revCount_ = val; }
    // return the spinning direction of the motor
    /*bool getDirection() const {
        return countDirection_;
    }*/
    T_QEI_COUNTER getDirection() const {
        if (countDirection_) {
            return 0;
        } else {
            return 1;
        }
    }
    // set the number of CPR
    void setCPR(T_QEI_COUNTER val) { m_CPR = val; }
    // set whteher to measure in degrees or radians
    void setModeAngle(enc_pos_mode mode) { m_mode = mode; }
    // set the divider to compute the position
    void setAngleDivider(T_QEI_COUNTER divider) {
        if (m_mode == enc_pos_mode::DEGREES) {
            m_divider_degrees = (T_ANGLE)divider;
        } else {
            m_divider_radians = (T_ANGLE)divider;
        }
    }
    // return the angle based on the mode set
    T_ANGLE getAngle() {
        if (m_mode == enc_pos_mode::DEGREES) {
            return m_posCount_angle * m_divider_degrees;
        } else {
            return m_posCount_angle * m_divider_radians;
        }
    }
    // return velocity computed
    T_VELOCITY getRPMVelocity() { return m_velocity; }
    // return if there is an error state
    bool getErrorState() { return m_error; }

   private:
    T_QEI_COUNTER posCount_ = 0;
    T_QEI_COUNTER m_pos_offset = 0;
    T_QEI_COUNTER revCount_ = 0;
    T_QEI_COUNTER m_CPR = 4000;
    T_QEI_COUNTER m_counter_period = 0;

    T_QEI_COUNTER RPM_factor_a = 1500000; // 60s*100MHz/CPR(4000)
    T_QEI_COUNTER RPM_factor_b = 1500000; // 60s*100MHz/CPR(4000)
    T_QEI_COUNTER counter_divider_a = 0, counter_divider_b = 0, internal_counter = 0;
    T_QEI_COUNTER m_velocity = 0, m_internal_velocity_a = 0, m_internal_velocity_b = 0;
    bool divide_enable_a = false;
    bool counting_a = false;
    bool divide_enable_b = false;
    bool counting_b = false;

    bool m_error = false;

    T_ANGLE m_posCount_angle = 0;
    T_ANGLE m_revCount_angle = 0;
    bool QAp_;
    bool QBp_;
    bool QIp_;
    bool countDirection_;
    enc_pos_mode m_mode = enc_pos_mode::DEGREES;
    T_ANGLE m_factor = 1.0;
    T_ANGLE m_divider_degrees = 0.36;
    T_ANGLE m_divider_radians = 0.00628;
    T_ANGLE m_theta = 0.0;
};

template <typename deglitch_data_type>
class deglitcher {
   public:
    // Constructor
    deglitcher() {
        filter = 0;          // Initialize filter count to 0
        max_filtercount = 4; // Set the default maximum filter count
    };

    // Function to get filtered input
    ap_uint<1> getdeglitched_input(ap_uint<1> noisy_in) {
        // The filter count is reset if the input has changed, otherwise it is incremented up to max_filtercount
        filter = (noisy_in_old ^ noisy_in) ? 0 : (filter < max_filtercount ? filter + 1 : max_filtercount);

        // If the filter count has reached max_filtercount, the output becomes the new input value
        // Otherwise, the output remains the same
        deglitched_out_data = filter == max_filtercount ? noisy_in : deglitched_out_data;

        // Store the current input for the next comparison
        noisy_in_old = noisy_in;

        // Return the filtered output
        return deglitched_out_data;
    }

    // Function to set the maximum filter count
    void set_max_filtercount(deglitch_data_type filter_ctr) { max_filtercount = filter_ctr; }

   private:
    // Store the previous input
    ap_uint<1> noisy_in_old;

    // Store the filtered output
    ap_uint<1> deglitched_out_data;

    // The current count of the filter
    deglitch_data_type filter;

    // The maximum count of the filter
    deglitch_data_type max_filtercount;
};

} // namespace details

/**
 * @brief The function hls_qei_axi is Quadrature Encoder Interface(QEI) control demo top
 * @tparam T_bin The data type for ABI's signals
 * @param strm_qei_A The input stream for A signals
 * @param strm_qei_B The input stream for B signals
 * @param strm_qei_I The input stream for I signals
 * @param strm_qei_RPM_THETA_m The output stream for rpm and theta_m
 * @param logger The output stream for status
 * @param qei_args_cpr Read for user setting or written back by kernel
 * @param qei_args_ctrl The lowest bit of this value indicates the encoding mode
 * @param qei_stts_RPM_THETA_m Rpm and theta_m written back by kernel on axi_lite port
 * @param qei_stts_dir Dir written back by kernel on axi_lite port
 * @param qei_stts_err Err written back by kernel on axi_lite port
 * @param qei_args_flt_size size of filter
 * @param qei_args_cnt_trip input of trip count used when doing csim
 * @param qei_debug_rpm output of rpm for debug
 * @param qei_count_mode Reserved s_axilite interface
 * @param qei_args_flt_size_i Reserved s_axilite interface
 */
template <class T_bin = ap_uint<1> >
void hls_qei_axi(hls::stream<T_bin>& strm_qei_A,
                 hls::stream<T_bin>& strm_qei_B,
                 hls::stream<T_bin>& strm_qei_I,
                 hls::stream<ap_uint<32> >& strm_qei_RPM_THETA_m,
                 hls::stream<ap_uint<256> >& logger,
                 volatile int& qei_args_cpr,
                 volatile int& qei_args_ctrl,
                 volatile int& qei_stts_RPM_THETA_m,
                 volatile int& qei_stts_dir,
                 volatile int& qei_stts_err,
                 volatile int& qei_args_flt_size,
                 volatile int& qei_args_cnt_trip,
                 volatile int& qei_debug_rpm,
                 volatile int& qei_count_mode,
                 volatile int& qei_args_flt_size_i) {
    // values for module initialization
    const unsigned int BIT_DEPTH = 32;
    const unsigned int INT_WORD = 15;
    const int CPR = qei_args_cpr;
    const bool B_Leading_A =
        not qei_args_ctrl; // qei_args_ctrl=b0 -> B is leading A when motor is in electrical phase rotation A->B->C
    const details::enc_pos_mode mode_qei = details::enc_pos_mode::DEGREES;
    const details::enc_count_mode mode_qei_count = details::enc_count_mode::X4;

    unsigned int cont = 0;

    bool has_finish_log = false;

    details::deglitcher<unsigned int> filter_a;
    details::deglitcher<unsigned int> filter_b;
    details::deglitcher<unsigned int> filter_i;
    // init QEI
    details::QEI<int, ap_fixed<BIT_DEPTH, INT_WORD>, int> qei_interface_;
    qei_interface_.setCPR(CPR);
    qei_interface_.setModeAngle(mode_qei);

    // Encoder frequency 360KHz, system clock 100MHz -> 100M/360K = 277,77
    const unsigned int QEI_freq_ab = qei_args_flt_size;
    const unsigned int QEI_freq_i = 8;

    filter_a.set_max_filtercount(QEI_freq_ab);
    filter_b.set_max_filtercount(QEI_freq_ab);
    filter_i.set_max_filtercount(QEI_freq_i);

    T_bin a_strm, a_filt, b_strm, b_filt, i_strm, i_filt;

    ap_int<32> angle_, RPM_pack;
    int angle_counter, velocity_, index_counter;
    // unsigned int packet_;
    ap_uint<32> packet_;

    ap_uint<256> logger_var;

    bool has_read = false;
#ifndef __SYNTHESIS__
    // Simulation purposes
    int cnt = qei_args_cnt_trip;
#endif
    while (1) {
#ifndef __SYNTHESIS__
        // Simulation purposes
        cnt--;
        if (cnt <= 0) {
            break;
        }
#endif
        // Take A - B - I signals from encoder
        has_read = strm_qei_A.read_nb(a_strm);
        if (!has_read) {
            continue;
        }
        has_read = strm_qei_B.read_nb(b_strm);
        if (!has_read) {
            continue;
        }
        has_read = strm_qei_I.read_nb(i_strm);
        if (!has_read) {
            continue;
        }
        // Filter encoder inputs
        a_filt = filter_a.getdeglitched_input(a_strm);
        b_filt = filter_b.getdeglitched_input(b_strm);
        i_filt = filter_i.getdeglitched_input(i_strm);

        bool A_ = a_filt == (T_bin)0 ? false : true;
        bool B_ = b_filt == (T_bin)0 ? false : true;
        bool I_ = i_filt == (T_bin)0 ? false : true;
        bool qei_args_ctrl;

        // Update encoder Status
        qei_interface_.update(A_, B_, I_, B_Leading_A);
        // Get counter and velocity
        angle_counter = qei_interface_.getCounter();
        velocity_ = qei_interface_.getRPMVelocity();

        if (angle_counter < 0) {
            angle_counter = angle_counter + 4000;
        }

        switch (mode_qei_count) {
            case details::enc_count_mode::X4:
                velocity_ = velocity_ >> 4;
                angle_counter = angle_counter >> 2;
                break;

            case details::enc_count_mode::X2:
                velocity_ = velocity_ >> 2;
                angle_counter = angle_counter >> 1;
                break;

            default:
                break;
        }

        packet_(31, 16) = angle_counter;
        packet_(15, 0) = velocity_;

        strm_qei_RPM_THETA_m.write(packet_);

        // LOGGER
        logger_var.range(255, 224) = packet_.range(31, 0);
        logger_var.range(223, 192) = b_filt.to_int();
        logger_var.range(191, 160) = a_filt.to_int();
        logger_var.range(159, 128) = I_;
        logger_var.range(127, 96) = B_;
        logger_var.range(95, 64) = A_;
        logger_var.range(63, 32) = velocity_;
        logger_var.range(31, 0) = angle_counter;

        logger.write_nb(logger_var);
        // LOGGER

        // update status and error
        qei_stts_RPM_THETA_m = packet_;
        qei_debug_rpm = velocity_;
        qei_stts_err = qei_interface_.getErrorState();
        qei_stts_dir = qei_interface_.getDirection();
    }
}

} // namespace motorcontrol
} // namespace xf

#endif // _QEI_HPP_
