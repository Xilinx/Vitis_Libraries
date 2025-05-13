/*
 * Copyright (C) 2024-2025, Advanced Micro Devices, Inc.
 * SPDX-License-Identifier: X11
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * X CONSORTIUM BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 * Except as contained in this notice, the name of Advanced Micro Devices
 * shall not be used in advertising or otherwise to promote the sale,
 * use or other dealings in this Software without prior written authorization
 * from Advanced Micro Devices, Inc.
 * */
#include "angle_generation.hpp"
#include "clarke_direct.hpp"
#include "park_direct.hpp"
#include "demuxer_pi.hpp"
#include "pi_control.hpp"
#include "muxer_pi.hpp"
#include "park_inverse.hpp"
#include "clarke_inverse.hpp"
#include "svpwm.hpp"
#include "voltage_modulation.hpp"
#include "common_vars.hpp"

void angle_generation_inst(volatile int& period, volatile int& increment, hls::stream<int32_t>& output_stream);
void Clarke_Direct_Inst(hls::stream<ap_uint<BIT_WIDTH_STREAM_FOC> >& s_axis,
                        hls::stream<ap_uint<BIT_WIDTH_STREAM_FOC> >& m_axis,
                        hls::stream<ap_uint<BIT_WIDTH_LOG_STREAM_FOC> >& logger_stream_in,
                        hls::stream<ap_uint<BIT_WIDTH_LOG_STREAM_FOC> >& logger_stream_out);
void Park_Direct_Inst(hls::stream<ap_uint<BIT_WIDTH_STREAM_FOC> >& s_axis,
                      hls::stream<ap_uint<BIT_WIDTH_STREAM_FOC> >& m_axis,
                      hls::stream<ap_uint<BIT_WIDTH_LOG_STREAM_FOC> >& logger_stream_in,
                      hls::stream<ap_uint<BIT_WIDTH_LOG_STREAM_FOC> >& logger_stream_out);
void Park_Inverse_Inst(hls::stream<ap_uint<BIT_WIDTH_STREAM_FOC> >& s_axis,
                       hls::stream<ap_uint<BIT_WIDTH_STREAM_FOC> >& m_axis,
                       hls::stream<ap_uint<BIT_WIDTH_LOG_STREAM_FOC> >& logger_stream_in,
                       hls::stream<ap_uint<BIT_WIDTH_LOG_STREAM_FOC> >& logger_stream_out);
void Clarke_Inverse_Inst(hls::stream<ap_uint<BIT_WIDTH_STREAM_FOC> >& s_axis,
                         hls::stream<ap_uint<BIT_WIDTH_STREAM_FOC> >& m_axis,
                         hls::stream<ap_uint<BIT_WIDTH_LOG_STREAM_FOC> >& logger_stream_in,
                         hls::stream<ap_uint<BIT_WIDTH_LOG_STREAM_FOC> >& logger_stream_out);
void SVPWM_Inst(hls::stream<ap_uint<BIT_WIDTH_STREAM_FOC> >& s_axis,
                hls::stream<ap_uint<BIT_WIDTH_STREAM_FOC> >& m_axis,
                hls::stream<ap_uint<BIT_WIDTH_LOG_STREAM_FOC> >& logger_stream_in,
                hls::stream<ap_uint<BIT_WIDTH_LOG_STREAM_FOC> >& logger_stream_out,
                volatile int& mode_,
                volatile int& phase_a_,
                volatile int& phase_b_,
                volatile int& phase_c_);
void demuxer_pi_inst(hls::stream<ap_uint<BIT_WIDTH_STREAM_FOC> >& s_axis,
                     hls::stream<ap_int<BIT_WIDTH_DATA> >& angle_gen,
                     hls::stream<ap_int<BIT_WIDTH_DATA> >& to_Iq_PI,
                     hls::stream<ap_int<BIT_WIDTH_DATA> >& to_Id_PI,
                     hls::stream<ap_int<BIT_WIDTH_DATA> >& to_RPM_PI,
                     hls::stream<ap_int<BIT_WIDTH_DATA> >& to_muxer,
                     hls::stream<ap_uint<BIT_WIDTH_LOG_STREAM_FOC> >& logger_stream_in,
                     hls::stream<ap_uint<BIT_WIDTH_LOG_STREAM_FOC> >& logger_stream_out,
                     volatile int& control_mode,
                     volatile int& Id_,
                     volatile int& Iq_,
                     volatile int& theta_);
void ps_iir_filter_inst(hls::stream<t_glb_foc2pwm>& Ia,
                        hls::stream<t_glb_foc2pwm>& Ib,
                        hls::stream<t_glb_foc2pwm>& Ic,
                        hls::stream<t_glb_speed_theta>& SPEED_THETA_m,
                        hls::stream<ap_uint<BIT_WIDTH_STREAM_FOC> >& output_stream,
                        hls::stream<ap_uint<BIT_WIDTH_LOG_STREAM_FOC> >& logger_stream,
                        int32_t& filt_a,
                        int32_t& filt_b,
                        int32_t& angle_shift);
void muxer_pi_inst(hls::stream<ap_int<BIT_WIDTH_DATA> >& from_torque,
                   hls::stream<ap_int<BIT_WIDTH_DATA> >& from_flux,
                   hls::stream<ap_int<BIT_WIDTH_DATA> >& from_demux,
                   hls::stream<ap_int<BIT_WIDTH_DATA> >& from_gen,
                   hls::stream<ap_uint<BIT_WIDTH_STREAM_FOC> >& m_axis,
                   hls::stream<ap_uint<BIT_WIDTH_LOG_STREAM_FOC> >& logger_stream_in,
                   hls::stream<ap_uint<BIT_WIDTH_LOG_STREAM_FOC> >& logger_stream_out,
                   volatile int& control_mode,
                   volatile int& Vd_ps,
                   volatile int& Vq_ps);
void PI_Control_Inst(hls::stream<ap_int<BIT_WIDTH_DATA> >& s_axis,
                     hls::stream<ap_int<BIT_WIDTH_DATA> >& m_axis,
                     ap_int<BIT_WIDTH_DATA> Sp,
                     ap_int<BIT_WIDTH_DATA> Kp,
                     ap_int<BIT_WIDTH_DATA> Ki,
                     ap_int<BIT_WIDTH_DATA> mode);
void PI_Control_stream_Inst(hls::stream<ap_int<BIT_WIDTH_DATA> >& s_axis,
                            hls::stream<ap_int<BIT_WIDTH_DATA> >& s_axis_2,
                            hls::stream<ap_int<BIT_WIDTH_DATA> >& m_axis,
                            ap_int<BIT_WIDTH_DATA> Sp,
                            ap_int<BIT_WIDTH_DATA> Kp,
                            ap_int<BIT_WIDTH_DATA> Ki,
                            ap_int<BIT_WIDTH_DATA> mode);
void voltage_modulation_inst(hls::stream<ap_uint<BIT_WIDTH_STREAM_FOC> >& s_axis,
                             hls::stream<ap_int<BIT_WIDTH_DATA> >& voltage_in,
                             hls::stream<ap_uint<96> >& output_s,
                             hls::stream<ap_uint<BIT_WIDTH_LOG_STREAM_FOC> >& logger_stream_in,
                             hls::stream<ap_uint<BIT_WIDTH_LOG_STREAM_FOC> >& logger_stream_out,
                             volatile unsigned int& mode,
                             volatile int& max_sym_interval,
                             volatile int& double_interval,
                             volatile int& scaling_interval_pwm,
                             volatile int& phase_a,
                             volatile int& phase_b,
                             volatile int& phase_c);
