/*
Copyright (C) 2022-2022, Xilinx, Inc.
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

#ifndef SVPWM_HPP
#define SVPWM_HPP

#include <hls_stream.h>
#include <ap_axi_sdata.h>
#include <ap_int.h>
#include <stdint.h>
#include "common.hpp"

namespace xf {
namespace motorcontrol {

enum MODE_PWM_DC_SRC { DC_SRC_ADC = 0, DC_SRC_REF };

enum MODE_PWM_PHASE_SHIFT { SHIFT_ZERO = 0, SHIFT_120 };

namespace details {

static RangeDef<int> RANGE_pwm_freq = {1000, 200000, 100000};

static RangeDef<int> RANGE_dead_cycles = {10, 2000, 10};

static RangeDef<t_glb_foc2pwm> RANGE_dc_link_ref = {5, 500, 24};

struct pwmPassedArgs {
    ap_uint<18> pwm_cycle;
    bool phase_shift_mode;
    ap_uint<18> dead_cycles;
    bool isEnd_cond;
};

template <class T_PWM_IO>
struct pwmStrmIO {
    T_PWM_IO Va_cmd;
    T_PWM_IO Vb_cmd;
    T_PWM_IO Vc_cmd;
    T_PWM_IO dc_link;
    bool isEnd_cond;
};

template <class T_FOC_COM>
void sampler_duty(long cnt_trip,
                  int ii_sample,
                  hls::stream<T_FOC_COM>& Va_cmd,
                  hls::stream<T_FOC_COM>& Vb_cmd,
                  hls::stream<T_FOC_COM>& Vc_cmd,
                  hls::stream<T_FOC_COM>& dc_link,
                  hls::stream<pwmStrmIO<T_FOC_COM> >& strm_pwm_bundle_out) {
    pwmStrmIO<T_FOC_COM> pwm_args;

    int cnt_ii = 0;
    long i = 0;
    int local_ii_sample = ii_sample <= 0 ? 1 : ii_sample;

LOOP_SVPWM_SAMPLER:
    while (i < cnt_trip) {
#pragma HLS pipeline II = 1
        if (cnt_ii == 0) {
            pwm_args.isEnd_cond = (i == (cnt_trip - 1));
            if (!Va_cmd.empty()) {
                pwm_args.Va_cmd = Va_cmd.read();
                pwm_args.Vb_cmd = Vb_cmd.read();
                pwm_args.Vc_cmd = Vc_cmd.read();
            }
            if (!dc_link.empty()) pwm_args.dc_link = dc_link.read();

            if (!strm_pwm_bundle_out.full()) {
                strm_pwm_bundle_out.write(pwm_args);
                i++;
            }
        }
        // updating cnt_ii;
        // local_ii_sample = ii_sample <= 0 ? 1 : ii_sample;
        // if (cnt_ii == ii_sample)
        if (cnt_ii == local_ii_sample)
            cnt_ii = 0;
        else
            cnt_ii++;
    }
};

//---------------------------------------------------------------------------
// split version for calculating duty persentage ratio
//---------------------------------------------------------------------------
template <class T>
T GetVoff(T V[3]) {
    T Vmin, Vmax, Voff;
    Vmin = (V[0] < V[1]) ? V[0] : V[1];
    Vmin = (V[2] < Vmin) ? V[2] : Vmin;
    Vmax = (V[0] > V[1]) ? V[0] : V[1];
    Vmax = (V[2] > Vmax) ? V[2] : Vmax;
    Voff = (Vmin + Vmax) >> 1;
    return Voff;
}

template <class t_val, class t_ratio>
void GetRatio(t_val V_saddle, t_val MAXVAL, t_ratio& duty_ratio) {
    t_val MINVAL = -(MAXVAL);
    V_saddle = (V_saddle > MAXVAL) ? MAXVAL : V_saddle;
    V_saddle = (V_saddle < MINVAL) ? MINVAL : V_saddle;
    t_glb_q15q16 Vp_saddle;
#pragma HLS BIND_OP variable = Vp_saddle op = add impl = dsp
    Vp_saddle = V_saddle + MAXVAL;
    t_ratio ratio;
#pragma HLS BIND_OP variable = ratio op = mul impl = dsp
    ratio = ((Vp_saddle >> 1) / MAXVAL);

    if (ratio >= 1) ratio = 0.99999;
    duty_ratio = ratio;
}

template <class T_FOC_COM>
void calculate_ratios_core(
    // output
    ap_ufixed<17, 1> duty_ratio[3],
    // input
    T_FOC_COM Vcmd[3],
    T_FOC_COM dcLink_adc,
    int args_dc_link_ref,
    int args_dc_src_mode) {
    t_glb_q15q16 apx_dc_ref;
    apx_dc_ref(31, 0) = args_dc_link_ref;

    // Selecting reference source from ADC and AXI sources
    T_FOC_COM V_ref = (args_dc_src_mode == MODE_PWM_DC_SRC::DC_SRC_ADC) ? dcLink_adc : (T_FOC_COM)apx_dc_ref;

    // Calculating Triangular Carrier Wave
    T_FOC_COM Voff = GetVoff(Vcmd);

    // Tanslating into saddle ware
    Vcmd[0] = Vcmd[0] - Voff;
    Vcmd[1] = Vcmd[1] - Voff;
    Vcmd[2] = Vcmd[2] - Voff;

    GetRatio<T_FOC_COM, ap_ufixed<17, 1> >(Vcmd[0], V_ref, duty_ratio[0]);
    GetRatio<T_FOC_COM, ap_ufixed<17, 1> >(Vcmd[1], V_ref, duty_ratio[1]);
    GetRatio<T_FOC_COM, ap_ufixed<17, 1> >(Vcmd[2], V_ref, duty_ratio[2]);
}

template <class T_FOC_COM, class T_RATIO_16b>
void calculate_ratios(hls::stream<pwmStrmIO<T_FOC_COM> >& strm_pwm_bundle_in,
                      hls::stream<T_RATIO_16b>& strm_duty_ratio_a,
                      hls::stream<T_RATIO_16b>& strm_duty_ratio_b,
                      hls::stream<T_RATIO_16b>& strm_duty_ratio_c,
                      volatile int& args_dc_link_ref,
                      volatile int& args_dc_src_mode,
                      volatile int& stt_cnt_iter,
                      volatile int& pwm_stt_Va_cmd,
                      volatile int& pwm_stt_Vb_cmd,
                      volatile int& pwm_stt_Vc_cmd) {
    int iter = 0;
    bool isEnd_cond = true;
    T_FOC_COM Vcmd[3];
    ap_ufixed<17, 1> duty_ratio[3];
    T_FOC_COM dcLink_adc;

LOOP_SVPWM_RATIO:
    do {
#pragma HLS PIPELINE off
        pwmStrmIO<T_FOC_COM> io_bundle_args;
        strm_pwm_bundle_in.read(io_bundle_args); // block reading to avoid oversampling

        Vcmd[0] = io_bundle_args.Va_cmd;
        Vcmd[1] = io_bundle_args.Vb_cmd;
        Vcmd[2] = io_bundle_args.Vc_cmd;
        isEnd_cond = io_bundle_args.isEnd_cond;
        dcLink_adc = io_bundle_args.dc_link;

        t_glb_q15q16 pwm_Va_stt_temp = Vcmd[0];
        t_glb_q15q16 pwm_Vb_stt_temp = Vcmd[1];
        t_glb_q15q16 pwm_Vc_stt_temp = Vcmd[2];

        pwm_stt_Va_cmd = pwm_Va_stt_temp.range(31, 0);
        pwm_stt_Vb_cmd = pwm_Vb_stt_temp.range(31, 0);
        pwm_stt_Vc_cmd = pwm_Vc_stt_temp.range(31, 0);

        calculate_ratios_core<T_FOC_COM>(duty_ratio, Vcmd, dcLink_adc, args_dc_link_ref, args_dc_src_mode);
        stt_cnt_iter = iter++;

        ap_ufixed<16, 0> r0 = (ap_ufixed<16, 0>)duty_ratio[0];
        ap_ufixed<16, 0> r1 = (ap_ufixed<16, 0>)duty_ratio[1];
        ap_ufixed<16, 0> r2 = (ap_ufixed<16, 0>)duty_ratio[2];
        T_RATIO_16b r0_out, r1_out, r2_out;
        r0_out.range(15, 0) = r0.range(15, 0);
        r1_out.range(15, 0) = r1.range(15, 0);
        r2_out.range(15, 0) = r2.range(15, 0);

        if (!strm_duty_ratio_a.full()) {
            strm_duty_ratio_a.write(r0_out);
            strm_duty_ratio_b.write(r1_out);
            strm_duty_ratio_c.write(r2_out);
        }

    } while (!isEnd_cond);
}

// clang-format off
/*
 * brief This module is to generate output by channel
 * tparam T_IN in<reg>: input data format
 * param pwm_cycle in<reg>: Dead cycle, the value in test is pwm_args_dead_cycles cycles
 * param pwm_cnt in<reg>: PWM duty cycles counter by each channel 
 * param dead_cycles in<reg>: Dead cycle, the value in test is pwm_args_dead_cycles cycles
 * param shift in<reg>: 0 (0 degree Phase Shift for output), 1 (120 degree Phase Shift for output)
 * param len in<reg>: local registers to hold the duty cycles length
 * param strm_length out<strm>: duty cycles on the target switch
 * param strm_h_pwm out<strm>: duty cycles on upper switch of the channel
 * param strm_l_pwm out<strm>: duty cycles on lower switch of the channel
 * param strm_pwm_sync out<strm>: sync signals by channel to sample the ADCs
 */
// clang-format on

template <class T_IN>
void generate_output_chnl(T_IN pwm_cycle,
                          T_IN pwm_cnt,
                          T_IN dead_cycles,
                          T_IN shift,
                          T_IN& len,
                          // hls::stream<ap_ufixed<16,0> >& strm_length,
                          ap_ufixed<16, 0>& duty_cycles_in,
                          hls::stream<ap_uint<1> >& strm_h_pwm,
                          hls::stream<ap_uint<1> >& strm_l_pwm,
                          hls::stream<ap_uint<1> >& strm_pwm_sync) {
#pragma HLS INLINE
    T_IN max_len;
#pragma HLS BIND_OP variable = max_len op = add impl = dsp
    max_len = pwm_cycle - dead_cycles;
    T_IN dead2 = (dead_cycles + 1) >> 1;
    // updating pwm_cnt2
    int pwm_cnt2;
#pragma HLS BIND_OP variable = pwm_cnt2 op = add impl = dsp
    pwm_cnt2 = pwm_cnt - shift;

    if (pwm_cnt2 < 0) pwm_cnt2 += pwm_cycle;

    ap_ufixed<16, 0> tmp_len = 0;
    if (pwm_cnt2 == 0) {
        // strm_length.read_nb(tmp_len);
        tmp_len = duty_cycles_in;
        len = tmp_len * pwm_cycle;
    }
    if (len > max_len) len = max_len;

    // calculating start and end
    T_IN start = (pwm_cycle - len) >> 1;
    T_IN end = start + len;

    // calculating output
    ap_uint<1> h_pwm, l_pwm, pwm_sync;
    if (pwm_cnt2 <= start || pwm_cnt2 > end) // when start==end also be 0
        h_pwm = 0;
    else //(start, end]
        h_pwm = 1;

    if (pwm_cnt2 <= start - dead2 || pwm_cnt2 > end + dead2)
        l_pwm = 1;
    else
        l_pwm = 0;
    if (pwm_cnt2 == (pwm_cycle >> 1)) // always at the central of pwm_cnt
        pwm_sync = 1;
    else
        pwm_sync = 0;
    strm_h_pwm.write_nb(h_pwm);
    strm_l_pwm.write_nb(l_pwm);
    strm_pwm_sync.write_nb(pwm_sync);
}

template <class T_IN>
struct gen_sampler_pkg {
    T_IN duty_ratio_a;
    T_IN duty_ratio_b;
    T_IN duty_ratio_c;
    int pwm_freq;
    int dead_cycles;
    int phase_shift;
    long trip_cnt;
};

template <class T_RATIO_16b, class T_OUT>
void sampler_gen(hls::stream<T_RATIO_16b>& strm_duty_ratio_a,
                 hls::stream<T_RATIO_16b>& strm_duty_ratio_b,
                 hls::stream<T_RATIO_16b>& strm_duty_ratio_c,
                 hls::stream<gen_sampler_pkg<T_OUT> >& strm_sampler_pkg_out,
                 volatile int& pwm_args_pwm_freq,
                 volatile int& pwm_args_dead_cycles,
                 volatile int& pwm_args_phase_shift,
                 volatile long& pwm_args_cnt_trip,
                 int ii_sample) {
    long iter = 0;
    int cnt_ii = 0;
    T_RATIO_16b local_duty_ratio_a;
    T_RATIO_16b local_duty_ratio_b;
    T_RATIO_16b local_duty_ratio_c;
    gen_sampler_pkg<T_OUT> local_sampler_pkg;

LOOP_GEN_SAMPLER:
    do {
#pragma HLS pipeline II = 1
        // if(!strm_duty_ratio_a.empty()){
        if (cnt_ii == 0) {
            strm_duty_ratio_a.read(local_duty_ratio_a);
            strm_duty_ratio_b.read(local_duty_ratio_b);
            strm_duty_ratio_c.read(local_duty_ratio_c);

            local_sampler_pkg.duty_ratio_a.range(15, 0) = local_duty_ratio_a.range(15, 0);
            local_sampler_pkg.duty_ratio_b.range(15, 0) = local_duty_ratio_b.range(15, 0);
            local_sampler_pkg.duty_ratio_c.range(15, 0) = local_duty_ratio_c.range(15, 0);

            local_sampler_pkg.pwm_freq = pwm_args_pwm_freq;
            local_sampler_pkg.dead_cycles = pwm_args_dead_cycles;
            local_sampler_pkg.phase_shift = pwm_args_phase_shift;
            local_sampler_pkg.trip_cnt = pwm_args_cnt_trip;

            strm_sampler_pkg_out.write(local_sampler_pkg);

            iter++;
        }

        // updating cnt_ii;
        if (cnt_ii == ii_sample)
            cnt_ii = 0;
        else
            cnt_ii++;

    } while (iter < pwm_args_cnt_trip);
}

static void PWM_gen_wave(hls::stream<gen_sampler_pkg<ap_ufixed<16, 0> > >& strm_sampler_pkg,
                         hls::stream<ap_uint<1> >& strm_h_a,
                         hls::stream<ap_uint<1> >& strm_h_b,
                         hls::stream<ap_uint<1> >& strm_h_c,
                         hls::stream<ap_uint<1> >& strm_l_a,
                         hls::stream<ap_uint<1> >& strm_l_b,
                         hls::stream<ap_uint<1> >& strm_l_c,
                         hls::stream<ap_uint<1> >& strm_sync_a,
                         hls::stream<ap_uint<1> >& strm_sync_b,
                         hls::stream<ap_uint<1> >& strm_sync_c,
                         volatile int& pwm_stt_pwm_cycle) {
    gen_sampler_pkg<ap_ufixed<16, 0> > local_args;
    strm_sampler_pkg.read(local_args);

    int clk_freq = COMM_CLOCK_FREQ; // pwm_args[SVPWM_ARGS_IDX::CLK_FQ];

    int pwm_freq = local_args.pwm_freq;
    CheckRange<int>(pwm_freq, RANGE_pwm_freq);
    int pwm_args_pwm_freq = pwm_freq; // write back the default value if using default value
    long pwm_args_cnt_trip = local_args.trip_cnt;

    ap_uint<18> pwm_cycle;
#pragma HLS BIND_OP variable = pwm_cycle op = mul impl = dsp
    pwm_cycle = clk_freq / pwm_freq;

    bool phase_shift_mode;

    ap_uint<18> pwm_cnt = 0;
    ap_uint<1> pwm_h[3] = {0, 0, 0};
    ap_uint<1> pwm_l[3] = {0, 0, 0};
    ap_uint<1> pwm_sync_temp[3] = {0, 0, 0};
    long iter = 0;

    ap_uint<18> len[3] = {0, 0, 0};
    ap_uint<18> start[3] = {0, 0, 0};
    ap_uint<18> end[3] = {0, 0, 0};
    ap_uint<18> pwm_cnt2[3] = {0, 0, 0};
    ap_uint<18> shift[3] = {0, 0, 0};
    ap_uint<18> dead_cycles;
    bool toBeEnd = false;
    bool alreadEnd = false;

    dead_cycles = local_args.dead_cycles;                                           // interface
    phase_shift_mode = (local_args.phase_shift == MODE_PWM_PHASE_SHIFT::SHIFT_120); // interface

    if (phase_shift_mode) {
        int phase2_3;
#pragma HLS BIND_OP variable = phase2_3 op = mul impl = dsp
        phase2_3 = ((pwm_cycle << 1) + 1) / 3;
        shift[0] = 0;
        shift[1] = phase2_3 >> 1;
        shift[2] = phase2_3;
    } else {
        shift[0] = 0;
        shift[1] = 0;
        shift[2] = 0;
    }

LOOP_GEN_WAVE:
    do {
#pragma HLS pipeline II = 1

        pwm_stt_pwm_cycle = pwm_cycle;

        generate_output_chnl<ap_uint<18> >(pwm_cycle, pwm_cnt, dead_cycles, shift[0], len[0], local_args.duty_ratio_a,
                                           strm_h_a, strm_l_a, strm_sync_a);
        generate_output_chnl<ap_uint<18> >(pwm_cycle, pwm_cnt, dead_cycles, shift[1], len[1], local_args.duty_ratio_b,
                                           strm_h_b, strm_l_b, strm_sync_b);
        generate_output_chnl<ap_uint<18> >(pwm_cycle, pwm_cnt, dead_cycles, shift[2], len[2], local_args.duty_ratio_c,
                                           strm_h_c, strm_l_c, strm_sync_c);

        if (pwm_cnt + 1 == pwm_cycle) {
            if (toBeEnd)
                alreadEnd = true;
            else {
                pwm_cnt = 0;
                // if(!strm_sampler_pkg.empty())
                {
                    // strm_sampler_pkg.read(local_args);
                    iter++;
                    toBeEnd = (iter == (pwm_args_cnt_trip - 1));
                    dead_cycles = local_args.dead_cycles;
                    phase_shift_mode = (local_args.phase_shift == MODE_PWM_PHASE_SHIFT::SHIFT_120);
                    // toBeEnd = args.isEnd_cond;
                    if (phase_shift_mode) {
                        int phase2_3;
#pragma HLS BIND_OP variable = phase2_3 op = mul impl = dsp
                        phase2_3 = ((pwm_cycle << 1) + 1) / 3;
                        shift[0] = 0;
                        shift[1] = phase2_3 >> 1;
                        shift[2] = phase2_3;
                    } else {
                        shift[0] = 0;
                        shift[1] = 0;
                        shift[2] = 0;
                    }
                }
            }
        } else {
            pwm_cnt++;
#ifdef __SYNTHESIS__
            if (!strm_sampler_pkg.empty()) // always try to consume out the intput stream and only keep the latest
                                           // ratios
                strm_sampler_pkg.read(local_args);
#else
            if ((pwm_cnt + 2 == pwm_cycle) && (!strm_sampler_pkg.empty())) strm_sampler_pkg.read(local_args);
#endif
        }

    } while (!alreadEnd);
}
template <class T_RATIO_16b>
void PWM_gen_wave(
    // hls::stream<gen_sampler_pkg<ap_ufixed<16,0>> > &strm_sampler_pkg,
    hls::stream<T_RATIO_16b>& strm_duty_ratio_a,
    hls::stream<T_RATIO_16b>& strm_duty_ratio_b,
    hls::stream<T_RATIO_16b>& strm_duty_ratio_c,
    volatile int& pwm_args_pwm_freq,
    volatile int& pwm_args_dead_cycles,
    volatile int& pwm_args_phase_shift,
    volatile long& pwm_args_cnt_trip,
    volatile int& pwm_args_ii_sample,
    hls::stream<ap_uint<1> >& strm_h_a,
    hls::stream<ap_uint<1> >& strm_h_b,
    hls::stream<ap_uint<1> >& strm_h_c,
    hls::stream<ap_uint<1> >& strm_l_a,
    hls::stream<ap_uint<1> >& strm_l_b,
    hls::stream<ap_uint<1> >& strm_l_c,
    hls::stream<ap_uint<1> >& strm_sync_a,
    hls::stream<ap_uint<1> >& strm_sync_b,
    hls::stream<ap_uint<1> >& strm_sync_c,
    volatile int& pwm_stt_pwm_cycle,
    volatile int& pwm_stt_duty_ratio_a,
    volatile int& pwm_stt_duty_ratio_b,
    volatile int& pwm_stt_duty_ratio_c) {
    T_RATIO_16b local_duty_ratio_a_next;
    T_RATIO_16b local_duty_ratio_b_next;
    T_RATIO_16b local_duty_ratio_c_next;
    T_RATIO_16b local_duty_ratio_a_inuse;
    T_RATIO_16b local_duty_ratio_b_inuse;
    T_RATIO_16b local_duty_ratio_c_inuse;

    strm_duty_ratio_a.read(local_duty_ratio_a_inuse);
    strm_duty_ratio_b.read(local_duty_ratio_b_inuse);
    strm_duty_ratio_c.read(local_duty_ratio_c_inuse);

    t_glb_q15q16 pwm_duty_ratio_a_temp = local_duty_ratio_a_inuse;
    t_glb_q15q16 pwm_duty_ratio_b_temp = local_duty_ratio_b_inuse;
    t_glb_q15q16 pwm_duty_ratio_c_temp = local_duty_ratio_c_inuse;

    pwm_stt_duty_ratio_a = pwm_duty_ratio_a_temp.range(31, 0);
    pwm_stt_duty_ratio_b = pwm_duty_ratio_b_temp.range(31, 0);
    pwm_stt_duty_ratio_c = pwm_duty_ratio_c_temp.range(31, 0);

    int ii_sample = pwm_args_ii_sample <= 0 ? 1 : pwm_args_ii_sample;
    int pwm_freq_inuse = pwm_args_pwm_freq;
    CheckRange<int>(pwm_freq_inuse, RANGE_pwm_freq);

    ap_uint<18> pwm_cycle;
#pragma HLS BIND_OP variable = pwm_cycle op = mul impl = dsp
    pwm_cycle = COMM_CLOCK_FREQ / pwm_freq_inuse;

    bool phase_shift_mode_inuse;
    bool phase_shift_mode_next;

    ap_uint<18> pwm_cnt = 0;
    ap_uint<1> pwm_h[3] = {0, 0, 0};
    ap_uint<1> pwm_l[3] = {0, 0, 0};
    ap_uint<1> pwm_sync_temp[3] = {0, 0, 0};
    long iter = 0;

    ap_uint<18> len[3] = {0, 0, 0};
    ap_uint<18> start[3] = {0, 0, 0};
    ap_uint<18> end[3] = {0, 0, 0};
    ap_uint<18> pwm_cnt2[3] = {0, 0, 0};
    ap_uint<18> shift[3] = {0, 0, 0};
    ap_uint<18> dead_cycles_inuse;
    ap_uint<18> dead_cycles_next;
    bool toBeEnd = false;
    bool alreadEnd = false;

    dead_cycles_inuse = pwm_args_dead_cycles;
    phase_shift_mode_inuse = (pwm_args_phase_shift == MODE_PWM_PHASE_SHIFT::SHIFT_120);

    if (phase_shift_mode_inuse) {
        int phase2_3;
#pragma HLS BIND_OP variable = phase2_3 op = mul impl = dsp
        phase2_3 = ((pwm_cycle << 1) + 1) / 3;
        shift[0] = 0;
        shift[1] = phase2_3 >> 1;
        shift[2] = phase2_3;
    } else {
        shift[0] = 0;
        shift[1] = 0;
        shift[2] = 0;
    }
    int cnt_ii = 1; // alwasy larger than latency of LOOP_GEN_WAVE
LOOP_GEN_WAVE:
    do {
#pragma HLS pipeline II = 1

        pwm_stt_pwm_cycle = pwm_cycle;

        pwm_duty_ratio_a_temp = (t_glb_q15q16)local_duty_ratio_a_inuse;
        pwm_duty_ratio_b_temp = (t_glb_q15q16)local_duty_ratio_b_inuse;
        pwm_duty_ratio_c_temp = (t_glb_q15q16)local_duty_ratio_c_inuse;

        pwm_stt_duty_ratio_a = pwm_duty_ratio_a_temp.range(31, 0);
        pwm_stt_duty_ratio_b = pwm_duty_ratio_b_temp.range(31, 0);
        pwm_stt_duty_ratio_c = pwm_duty_ratio_c_temp.range(31, 0);

        generate_output_chnl<ap_uint<18> >(pwm_cycle, pwm_cnt, dead_cycles_inuse, shift[0], len[0],
                                           local_duty_ratio_a_inuse, strm_h_a, strm_l_a, strm_sync_a);
        generate_output_chnl<ap_uint<18> >(pwm_cycle, pwm_cnt, dead_cycles_inuse, shift[1], len[1],
                                           local_duty_ratio_b_inuse, strm_h_b, strm_l_b, strm_sync_b);
        generate_output_chnl<ap_uint<18> >(pwm_cycle, pwm_cnt, dead_cycles_inuse, shift[2], len[2],
                                           local_duty_ratio_c_inuse, strm_h_c, strm_l_c, strm_sync_c);

        if (pwm_cnt + 1 == pwm_cycle) {
            if (toBeEnd)
                alreadEnd = true;
            else {
                pwm_cnt = 0;
                // if(!strm_sampler_pkg.empty())
                {
                    iter++;
                    toBeEnd = (iter == (pwm_args_cnt_trip - 1));
                    dead_cycles_inuse = dead_cycles_next;
                    phase_shift_mode_inuse = (phase_shift_mode_next == MODE_PWM_PHASE_SHIFT::SHIFT_120);
                    // toBeEnd = args.isEnd_cond;
                    if (phase_shift_mode_inuse) {
                        int phase2_3;
#pragma HLS BIND_OP variable = phase2_3 op = mul impl = dsp
                        phase2_3 = ((pwm_cycle << 1) + 1) / 3;
                        shift[0] = 0;
                        shift[1] = phase2_3 >> 1;
                        shift[2] = phase2_3;
                    } else {
                        shift[0] = 0;
                        shift[1] = 0;
                        shift[2] = 0;
                    }
                    local_duty_ratio_a_inuse = local_duty_ratio_a_next;
                    local_duty_ratio_b_inuse = local_duty_ratio_b_next;
                    local_duty_ratio_c_inuse = local_duty_ratio_c_next;
                }
            }
        } else {
            pwm_cnt++;

            if (!strm_duty_ratio_a.empty() &&
                (cnt_ii == 1)) // always try to consume out the intput stream and only keep the latest ratios
            {
                strm_duty_ratio_a.read(local_duty_ratio_a_next);
                strm_duty_ratio_b.read(local_duty_ratio_b_next);
                strm_duty_ratio_c.read(local_duty_ratio_c_next);

                dead_cycles_next = pwm_args_dead_cycles;
                phase_shift_mode_next = pwm_args_phase_shift;
            }
            if (cnt_ii == ii_sample)
                cnt_ii = 1;
            else
                cnt_ii++;
        }

    } while (!alreadEnd);
}

} /*end of namespace details*/

// clang-format off
/**
 * @brief The function hls_svpwm_duty calculates the duty cycles from the input three-phase voltages.
 * @tparam T_FOC_COM The data type for input voltages
 * @tparam T_RATIO_16b The data type for output duty cycles
 * @param strm_Va_cmd in<strm>: Every pwm_args_sample_ii cycles, one output of FOC can be consumed.
 * @param strm_Vb_cmd in<strm>: Every pwm_args_sample_ii cycles, one output of FOC can be consumed.
 * @param strm_Vc_cmd in<strm>: Every pwm_args_sample_ii cycles, one output of FOC can be consumed.
 * @param strm_dc_link in<strm>: Every pwm_args_sample_ii cycles, one output of FOC can be consumed.
 * @param strm_duty_ratio_a out<strm>: the duty ratio of a, within every pwm cycle.
 * @param strm_duty_ratio_b out<strm>: the duty ratio of b, within every pwm cycle.
 * @param strm_duty_ratio_c out<strm>: the duty ratio of c, within every pwm cycle.
 * @param pwm_args_dc_link_ref in<reg>: Q15Q16 representation for dc_link_ref format, Eg.  0x180000: 24.00000(q15q16)
 * @param pwm_stt_cnt_iter out<reg>: constantly monitoring how many pwm command sent.
 * @param pwm_args_dc_src_mode in<reg>: 0 - PWM voltage reference based on ADC measured DC link; 1 - PWM voltage reference uses static register value.
 * @param pwm_args_sample_ii in<reg>: Sampling interval for more real co-sim. 
 * @param pwm_args_cnt_trip in<reg>: Inner trip counter.
 * @param pwm_stt_Va_cmd out<reg>: contantly monitoring the Va_cmd inside the kernel calculate_ratios.
 * @param pwm_stt_Vb_cmd out<reg>: contantly monitoring the Vb_cmd inside the kernel calculate_ratios.
 * @param pwm_stt_Vc_cmd out<reg>: contantly monitoring the Vc_cmd inside the kernel calculate_ratios.
 */
// clang-format on
template <class T_FOC_COM, class T_RATIO_16b>
void hls_svpwm_duty_axi(hls::stream<T_FOC_COM>& strm_Va_cmd,
                        hls::stream<T_FOC_COM>& strm_Vb_cmd,
                        hls::stream<T_FOC_COM>& strm_Vc_cmd,
                        hls::stream<T_FOC_COM>& strm_dc_link,
                        hls::stream<T_RATIO_16b>& strm_duty_ratio_a,
                        hls::stream<T_RATIO_16b>& strm_duty_ratio_b,
                        hls::stream<T_RATIO_16b>& strm_duty_ratio_c,
                        volatile int& pwm_args_dc_link_ref,
                        volatile int& pwm_stt_cnt_iter,
                        volatile int& pwm_args_dc_src_mode,
                        volatile int& pwm_args_sample_ii,
                        volatile long& pwm_args_cnt_trip,
                        volatile int& pwm_stt_Va_cmd,
                        volatile int& pwm_stt_Vb_cmd,
                        volatile int& pwm_stt_Vc_cmd) {
    hls::stream<details::pwmStrmIO<T_FOC_COM> > strm_pwm_io_bundle;
#pragma HLS STREAM depth = 2 variable = strm_pwm_io_bundle

#pragma HLS DATAFLOW
    details::sampler_duty<T_FOC_COM>(pwm_args_cnt_trip, pwm_args_sample_ii, strm_Va_cmd, strm_Vb_cmd, strm_Vc_cmd,
                                     strm_dc_link, strm_pwm_io_bundle);
    details::calculate_ratios<T_FOC_COM, T_RATIO_16b>(strm_pwm_io_bundle, strm_duty_ratio_a, strm_duty_ratio_b,
                                                      strm_duty_ratio_c, pwm_args_dc_link_ref, pwm_args_dc_src_mode,
                                                      pwm_stt_cnt_iter, pwm_stt_Va_cmd, pwm_stt_Vb_cmd, pwm_stt_Vc_cmd);
}

// clang-format off
/**
 * @brief The function hls_pwm_gen generates the gating bitstream of each switch according to the duty cycles.
 * @tparam T_RATIO_16b The data type of input duty cycles.
 * @param strm_duty_ratio_a in<strm>: the duty ratio of switch bridge pair a, within every pwm cycle.
 * @param strm_duty_ratio_b in<strm>: the duty ratio of switch bridge pair b, within every pwm cycle.
 * @param strm_duty_ratio_c in<strm>: the duty ratio of switch bridge pair c, within every pwm cycle.
 * @param strm_h_a out<strm>: controls the gating of upper switch at bridge pair a.
 * @param strm_h_b out<strm>: controls the gating of upper switch at bridge pair b.
 * @param strm_h_c out<strm>: controls the gating of upper switch at bridge pair c.
 * @param strm_l_a out<strm>: controls the gating of lower switch at bridge pair a.
 * @param strm_l_b out<strm>: controls the gating of lower switch at bridge pair b.
 * @param strm_l_c out<strm>: controls the gating of lower switch at bridge pair c.
 * @param strm_sync_a out<strm>: send sync sampling signal to the ADC a.
 * @param strm_sync_b out<strm>: send sync sampling signal to the ADC b.
 * @param strm_sync_c out<strm>: send sync sampling signal to the ADC c.
 * @param pwm_args_pwm_freq in<reg>: pwm cycle, the value in test is 100,000 Hz.
 * @param pwm_args_dead_cycles in<reg>: dead cycle, the value in test is pwm_args_dead_cycles<10> cycles, with global clk freq 100MHz.
 * @param pwm_args_phase_shift in<reg>: 0 - No phase shift for output; 1 - 120 degree phase shift for output.
 * @param pwm_stt_pwm_cycle out<reg>: constantly monitoring the integer value of pwm_factor=COMM_CLOCK_FREQ/pwm_freq.
 * @param pwm_args_cnt_trip in<reg>: inner trip count.
 * @param pwm_args_sample_ii in<reg>: sampling the AXIS input at [-ii] rate.
 * @param pwm_stt_duty_ratio_a out<reg>: constantly monitoring the duty_ratio_a value.
 * @param pwm_stt_duty_ratio_b out<reg>: constantly monitoring the duty_ratio_b value.
 * @param pwm_stt_duty_ratio_c out<reg>: constantly monitoring the duty_ratio_c value.
 */
// clang-format on
template <class T_RATIO_16b>
void hls_pwm_gen_axi(hls::stream<T_RATIO_16b>& strm_duty_ratio_a,
                     hls::stream<T_RATIO_16b>& strm_duty_ratio_b,
                     hls::stream<T_RATIO_16b>& strm_duty_ratio_c,
                     hls::stream<ap_uint<1> >& strm_h_a,
                     hls::stream<ap_uint<1> >& strm_h_b,
                     hls::stream<ap_uint<1> >& strm_h_c,
                     hls::stream<ap_uint<1> >& strm_l_a,
                     hls::stream<ap_uint<1> >& strm_l_b,
                     hls::stream<ap_uint<1> >& strm_l_c,
                     hls::stream<ap_uint<1> >& strm_sync_a,
                     hls::stream<ap_uint<1> >& strm_sync_b,
                     hls::stream<ap_uint<1> >& strm_sync_c,
                     volatile int& pwm_args_pwm_freq,
                     volatile int& pwm_args_dead_cycles,
                     volatile int& pwm_args_phase_shift,
                     volatile int& pwm_stt_pwm_cycle,
                     volatile long& pwm_args_cnt_trip,
                     volatile int& pwm_args_sample_ii,
                     volatile int& pwm_stt_duty_ratio_a,
                     volatile int& pwm_stt_duty_ratio_b,
                     volatile int& pwm_stt_duty_ratio_c) {
    details::PWM_gen_wave<T_RATIO_16b>(
        strm_duty_ratio_a, strm_duty_ratio_b, strm_duty_ratio_c,

        pwm_args_pwm_freq, pwm_args_dead_cycles, pwm_args_phase_shift, pwm_args_cnt_trip, pwm_args_sample_ii,

        strm_h_a, strm_h_b, strm_h_c, strm_l_a, strm_l_b, strm_l_c, strm_sync_a, strm_sync_b, strm_sync_c,
        pwm_stt_pwm_cycle, pwm_stt_duty_ratio_a, pwm_stt_duty_ratio_b, pwm_stt_duty_ratio_c);
}
} /*end of namespace motorcontrol*/
} /*end of namespace xf*/

#endif // SVPWM_H
