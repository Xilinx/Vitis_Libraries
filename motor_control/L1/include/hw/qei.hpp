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
#ifndef _QEI_HPP_
#define _QEI_HPP_

#include <math.h>
#include "ap_int.h"
#include <iostream>
#include <hls_stream.h>
#include "common.hpp"

//--------------------------------------------------------------------------
// Argument
//--------------------------------------------------------------------------

enum Encoding_Mode { B_Leading_A = 0, A_Leading_B };
enum Dirction_QEI { clockwise_n = 0, clockwise_p };
struct AxiParameters_QEI { //
    int qei_args_cpr;
    int qei_args_ctrl;
    int qei_stts_RPM_THETA_m;
    int qei_stts_dir;
    int qei_stts_err;
    long qei_args_cnt_trip;
};
/// Constant values
#define QEI_MAX_NO_EDGE_CYCLE (1 << 30)
static RangeDef<ap_uint<32> > RANGE_qei_freq = {COMM_CLOCK_FREQ / 10, COMM_CLOCK_FREQ * 20,
                                                COMM_CLOCK_FREQ}; // min max default
static RangeDef<ap_uint<32> > RANGE_qei_cpr = {400, 40000, 1000}; // min max default

namespace xf {
namespace motorcontrol {
namespace details {

/*
 * @brief The QEI module uses digital noise filters to reject noise on the incoming quadrature phase signals and index
 * pulse.
 * @tparam T_bin ABI's pulse is a binary data type
 * @param A Input signals of a quadrature encoder Channel A.
 * @param B Input signals of a quadrature encoder Channel B. The A and B channels are coded ninety electrical degrees
 * out of phase.
 * @param I Input signals of a quadrature encoder Channel I. This I signal, produced once per complete revolution of the
 * quadrature encoder, is often used to locate a specific position during a 360° revolution.
 * @param trip_cnt input of trip count
 * @param finalABI The output of ABI by filtered
 */
template <class T_bin>
void filterIn(hls::stream<T_bin>& A,
              hls::stream<T_bin>& B,
              hls::stream<T_bin>& I,
              long trip_cnt,
              hls::stream<ap_uint<4> >& finalABI) {
    const ap_uint<5> max_filtercount = 16;
    ap_uint<5> filter_a = 0;
    ap_uint<5> filter_b = 0;
    ap_uint<5> filter_i = 0;
    T_bin f_a = 0;
    T_bin f_b = 0;
    T_bin f_i = 0;
    T_bin a;
    T_bin b;
    T_bin i;
    T_bin ini_A = 0;
    T_bin ini_B = 0;
    T_bin ini_I = 0;
    T_bin pre_a = 0;
    T_bin pre_b = 0;
    T_bin pre_i = 0;

LOOP_QEI_FILTER:
    for (long k = 0; k < trip_cnt; k++) {
#pragma HLS PIPELINE II = 1
        auto read_a = A.read_nb(a);
        auto read_b = B.read_nb(b);
        auto read_i = I.read_nb(i);
        bool is_end = trip_cnt == k + 1 ? true : false;
        if (read_a) {
            ini_A = a;
            ini_B = b;
            ini_I = i;
        }
        if (pre_a ^ ini_A) filter_a = 0;
        if (pre_b ^ ini_B) filter_b = 0;
        if (pre_i ^ ini_I) filter_i = 0;
        pre_a = ini_A;
        pre_b = ini_B;
        pre_i = ini_I;
        if (filter_a < max_filtercount) filter_a++;
        if (filter_b < max_filtercount) filter_b++;
        if (filter_i < max_filtercount) filter_i++;
        if (filter_a == max_filtercount) f_a = ini_A;
        if (filter_b == max_filtercount) f_b = ini_B;
        if (filter_i == max_filtercount) f_i = ini_I;
        ap_uint<4> f_ABI;
        f_ABI[0] = (f_a);
        f_ABI[1] = (f_b);
        f_ABI[2] = (f_i);
        f_ABI[3] = (is_end);
        finalABI.write(f_ABI);
    }
}

/*
 * @brief The structure for ABI' edges information
 * @param edges The variable for saving ABI's status, if there is a change (0->1 or 1->0), edges value is 1, or is 0.
 * @param types The type for marking ABI's status including raining(0->1) or falling(1->0)
 * @param timeStep Record the number of cycles on qei system time
 */
struct QEI_EdgeInfo {
    ap_uint<3> edges; // 2:I,1:B,0:A
    ap_uint<3> types; // 2:I,1:B,0:A, 1: rainsing , 0: falling
    unsigned int timeStep;
    void clone(QEI_EdgeInfo src) {
        edges = src.edges;
        types = src.types;
        timeStep = src.timeStep;
    }
};

/*
 * @brief By catching on the rising and falling edges of the pulse train,
 * we can mark the state and type of the pulse and filter out invalid signals.
 * @tparam T_bin ABI's pulse is a binary data type
 * @param strm_finalABI The input signals are filtered by filterIn
 * @param strm_cntEdge The output signals are marked and filtered by this module
 */
template <class T_bin>
void catchingEdge(hls::stream<ap_uint<4> >& strm_finalABI,
                  hls::stream<QEI_EdgeInfo>& strm_cntEdge,
                  hls::stream<T_bin>& strm_calc_end) {
    const unsigned int max_timeStep = QEI_MAX_NO_EDGE_CYCLE;
    unsigned int timeStep = 0; // increasing by MAX_PERIOD
#pragma HLS BIND_OP variable = timeStep op = add impl = dsp
    QEI_EdgeInfo edgeInfo;
    ap_uint<4> f_ABI;
    T_bin va, vb, vi, is_end;
    T_bin va_pre, vb_pre, vi_pre;
    f_ABI = strm_finalABI.read();
    va = f_ABI[0];
    vb = f_ABI[1];
    vi = f_ABI[2];
    is_end = f_ABI[3];
    timeStep++;
#ifndef __SYNTHESIS__
    unsigned int k = 0;
#endif
LOOP_QEI_CATCH:
    while (!is_end) {
#pragma HLS PIPELINE II = 1
        if (!strm_finalABI.empty()) {
            va_pre = va;
            vb_pre = vb;
            vi_pre = vi;
            f_ABI = strm_finalABI.read();
            va = f_ABI[0];
            vb = f_ABI[1];
            vi = f_ABI[2];
            is_end = f_ABI[3];
            ap_uint<3> edges;
            edgeInfo.edges[0] = va_pre ^ va;
            edgeInfo.edges[1] = vb_pre ^ vb;
            edgeInfo.edges[2] = vi_pre ^ vi;
            edgeInfo.types[0] = va; // 0->1 : raising edge
            edgeInfo.types[1] = vb;
            edgeInfo.types[2] = vi;
            edgeInfo.timeStep = timeStep;
            if (edgeInfo.edges != 0) {
                strm_cntEdge.write_nb(edgeInfo);
                strm_calc_end.write(0);
            }
        }
        if (timeStep < max_timeStep - 1)
            timeStep++;
        else
            timeStep = 0;
#ifndef __SYNTHESIS__
        k++;
#endif
    } // while(1)
    strm_calc_end.write(1);
}

/*
 * @brief Judging that both channel A and channel B have edges
 * @param edges_cur The current signals packet including ABI's edges, types, timeStep
 * @return If both A and B have edges is false, or is true
 */
static bool isNormalEdges(QEI_EdgeInfo edges_cur) {
#pragma HLS INLINE
    const int i_a = 0;
    const int i_b = 1;
    if (edges_cur.edges[i_a] == 1 && edges_cur.edges[i_b] == 1)
        return false;
    else
        return true;
}

/*
 * @brief Judging that both channel A and channel B have no edges
 * @param edges_cur The current signals packet including ABI's edges, types, timeStep
 * @return If A and B have no edges is true, or is false
 */
static bool isNoABEdge(QEI_EdgeInfo edges_cur) {
#pragma HLS INLINE
    const int i_a = 0;
    const int i_b = 1;
    if (edges_cur.edges[i_a] == 0 && edges_cur.edges[i_b] == 0)
        return true;
    else
        return false;
}

/*
 * @brief Determine whether the pulse is a rising edge
 * @param idx Index of pulse type for A, B, I
 * @param edges_cur The current pulse packet including ABI's edges, types, timeStep
 * @return If the pulse is a rising edge is true, or is false
 */
static bool isRaising(int idx, QEI_EdgeInfo edges_cur) {
#pragma HLS INLINE
    if (edges_cur.edges[idx] == 1 && edges_cur.types[idx] == 1)
        return true;
    else
        return false;
}

/*
 * @brief Analyze the direction of a pulse based on its state change and type, and calculate the number of cycles
 * between the two pulses.
 * @param pre The previous status of pulse
 * @param cur The current status of pulse
 * @param mode The mode represents encoding mode, B leading A is represented by 0 and A leading B is represented by 1.
 * @param dir The direction of current pulse, clockwise is represented by 1 and counterclockwise is represented by 0.
 * @param ii_cycles The number of cycles between previous pulse and current pulse
 */
static void processABedges(QEI_EdgeInfo pre, QEI_EdgeInfo cur, ap_uint<1>& mode, bool& dir, unsigned int& ii_cycles) {
#pragma HLS INLINE
    // PRE	CUR	Leader
    // R	    R	PRE
    // F	    F	PRE
    // R	    F	CUR
    // F	    R	CUR
    const unsigned int max_timeStep = QEI_MAX_NO_EDGE_CYCLE;
    const int i_a = 0;
    const int i_b = 1;
    int id_pre = pre.edges(1, 0) / 2;
    int id_cur = cur.edges(1, 0) / 2;
    if (pre.types[id_pre] == cur.types[id_cur]) {
        // leader is pre
        if (id_pre == 0) // A is leader
            dir = 1 ^ mode;
        else
            dir = 0 ^ mode;
    } else {
        // leader is cur
        if (id_cur == 0) // A is leader
            dir = 1 ^ mode;
        else
            dir = 0 ^ mode;
    }
    int diff;
#pragma HLS BIND_OP variable = diff op = add impl = dsp
    diff = cur.timeStep - pre.timeStep;
    if (diff < 0) diff += QEI_MAX_NO_EDGE_CYCLE;
    ii_cycles = diff;
}

/*
 * @brief By counting the leading and trailing edges, the counter monitors the transition in its relationship
 to the state of the opposite channel, and can generate reliable position and speed.
 * @tparam T_bin ABI's pulse is a binary data type
 * @tparam T_err The type for qei's error status
 * @param strm_cntEdge The input stream with a structure including ABI' edges, types and timeStep
 * @param out_speed_angle The output stream with 32 bits saving speed and angle
 * @param out_dirstr The output stream for direction
 * @param out_errstr The output stream for error
 * @param axi_qei_cpr Read for user setting or written back by kernel
 * @param axi_qei_ctrl The lowest bit of this value indicates the encoding mode
 * @param axi_qei_rpm_theta_m Rpm and theta_m written back by kernel on axi_lite port
 * @param axi_qei_dir Dir written back by kernel on axi_lite port
 * @param axi_qei_err Err written back by kernel on axi_lite port
 */
template <class T_bin, class T_err>
void calcCounter(hls::stream<QEI_EdgeInfo>& strm_cntEdge,
                 hls::stream<T_bin>& strm_calc_end,
                 hls::stream<ap_uint<32> >& out_speed_angle,
                 hls::stream<T_bin>& out_dirstr,
                 hls::stream<T_err>& out_errstr,
                 volatile int& axi_qei_cpr,
                 // volatile int& axi_qei_freq,COMM_CLOCK_FREQ
                 volatile int& axi_qei_ctrl,
                 volatile int& axi_qei_rpm_theta_m,
                 volatile int& axi_qei_dir,
                 volatile int& axi_qei_err) {
    axi_qei_rpm_theta_m = 0;
    axi_qei_dir = 0;
    axi_qei_err = 0;

    const int i_a = 0;
    const int i_b = 1;
    const int i_i = 2;

    const unsigned int timeOut_noEdge = QEI_MAX_NO_EDGE_CYCLE;
    unsigned int cnt_noEdge = 0;
    unsigned short counter = 0;
#pragma HLS BIND_OP variable = counter op = add impl = dsp

    bool isPreValid = false;
    bool isNoEdge = true;
    QEI_EdgeInfo edges_cur;
    QEI_EdgeInfo edges_pre;

    T_bin is_end = strm_calc_end.read();
    bool dir = true; // clockwise
    ap_uint<32> ctrl = axi_qei_ctrl;
    ap_uint<1> mode = (ctrl > (ap_uint<32>)0);
    mode = mode ^ 1; // inversing the behavior for default setting of  axi_qei_ctrl '0' means B leading A
//#define DBG_CHIPSCOPE
#ifdef DBG_CHIPSCOPE
    unsigned short freqA = 0;
    unsigned short freqB = 0;
    unsigned short cnt_A = 0;
    unsigned short cnt_B = 0;
    unsigned int cnt_sec = 0;
#endif // DBG_CHIPSCOPE
#ifndef __SYNTHESIS__
    QEI_EdgeInfo edges_pre2; // for printing
    unsigned int k = 0;
#endif
LOOP_QEI_COUNTEDGE:
    while (!is_end) {
#pragma HLS PIPELINE off

        ap_uint<32> cpr = axi_qei_cpr;
        CheckRange(cpr, RANGE_qei_cpr);
        axi_qei_cpr = cpr;
        ap_uint<32> freq = COMM_CLOCK_FREQ;

        unsigned int ii_cycles;
        unsigned short speed_rpm;

#ifdef DBG_CHIPSCOPE
        const int ii_after_synthesis = 4;
        const int div_factor =
            100 *
            ii_after_synthesis; // for simulation, div_factor can be larger value, for running on hardware, it can be 1
        if (cnt_sec == freq / div_factor) { // if(cnt_sec == freq/ div_factor) can also be used with more area
            cnt_sec = 0;
            freqA = cnt_A; // just the rounded KHz of
            freqB = cnt_B;
        } else {
            cnt_sec++;
        }
#endif // DBG_CHIPSCOPE
        if (!strm_cntEdge.empty()) {
            is_end = strm_calc_end.read();
            isNoEdge = false;
            edges_cur = strm_cntEdge.read();
#ifdef DBG_CHIPSCOPE
            if (isRaising(i_a, edges_cur)) {
                if (cnt_sec == 0)
                    cnt_A = 1;
                else
                    cnt_A++;
            }
            if (isRaising(i_b, edges_cur)) {
                if (cnt_sec == 0)
                    cnt_B = 1;
                else
                    cnt_B++;
            }
#endif // DBG_CHIPSCOPE

            if (!isNormalEdges(edges_cur)) {
                ;
#ifdef DBG_CHIPSCOPE
                axi_qei_dir = (freqA + freqB) << 1; // just to keep the registers after synthesis
#endif
            } else { // valid edge(s)
                // interating counter
                if (isRaising(i_i, edges_cur))
                    counter = 0;
                else if (edges_cur.edges & 3) { // 4X mode : using any type of edges of A or B
                    if (dir)
                        counter = counter < ((int)(cpr << 2) - 1) ? counter + 1 : 0;
                    else
                        counter = counter == 0 ? ((int)(cpr << 2) - 1) : counter - 1;
                }
                // No A or B edges
                if (isNoABEdge(edges_cur)) continue;

                if (isPreValid == true) {
                    // iterating dir and getting interval between two edges
                    processABedges(edges_pre, edges_cur, mode, dir, ii_cycles);
                    ap_uint<36> tmp_rpm = freq;
#pragma HLS BIND_OP variable = tmp_rpm op = mul impl = dsp
                    tmp_rpm *= 15; // 60 / 4
                    unsigned int div;
#pragma HLS BIND_OP variable = div op = mul impl = dsp
                    div = ii_cycles * cpr;
#ifndef __SYNTHESIS__
                    if (div == 0) printf("QEI_ERROR DIV=0\n");
                    assert(div != 0);
#endif
                    tmp_rpm = tmp_rpm / div;

                    if (dir == Dirction_QEI::clockwise_p)
                        speed_rpm = tmp_rpm; // clockwise_p
                    else
                        speed_rpm = -tmp_rpm; // clockwise_n
                    ap_uint<32> tmp;
                    tmp.range(15, 0) = speed_rpm;
                    tmp.range(31, 16) = counter >> 2; // 4X mode : using any type of edges of A or B
                    axi_qei_rpm_theta_m = (int)tmp;
                    // outputing
                    if (!out_speed_angle.full()) out_speed_angle.write((int)tmp);
                    if (!out_dirstr.full()) out_dirstr.write((T_bin)dir);
                    if (!out_errstr.full()) out_errstr.write((T_err)0);
                    axi_qei_dir = dir;
                    axi_qei_err = 0;
                }
                isPreValid = true;
#ifndef __SYNTHESIS__
                edges_pre2.clone(edges_pre); // for printing
#endif
                edges_pre.clone(edges_cur);
            } // Normal edge(s)
            cnt_noEdge = 0;
        } else { // no edge

#ifdef DBG_CHIPSCOPE
            if (cnt_sec == 0) {
                cnt_A = 0;
                cnt_B = 0;
            }
#endif // DBG_CHIPSCOPE

            isNoEdge = true;
            if (cnt_noEdge == timeOut_noEdge - 1) {
                // debug tobe refine
                axi_qei_dir = counter >> 2; /// 4X mode : using any type of edges of A or B
                ap_uint<32> tmp_err;
                tmp_err.range(3, 0) = (ap_uint<4>)3;
                tmp_err.range(31, 4) = (ap_uint<28>)cnt_noEdge;

                axi_qei_err = tmp_err;
                // reseting
                counter = 0;
                cnt_noEdge = 0;
                // outputing
                if (!out_speed_angle.full()) out_speed_angle.write(0);
                if (!out_dirstr.full()) out_dirstr.write((T_bin)dir);
                if (!out_errstr.full()) out_errstr.write((T_err)3);

            } else
                cnt_noEdge++;
        } // no edge
//#define _DBG_QEI_WATCH_
#ifndef __SYNTHESIS__

#ifdef _DBG_QEI_WATCH_
        int mode_noedge = timeOut_noEdge / 20;
        if (k == 0)
            printf(
                "DBG_QEI_WATCH:\t k\t noEdge\t PreValid\t pre.edges\t pre.types\t pre.time\t cur.edges\t cur.types\t "
                "cur.time\t counter\t speed_rpm\tii_cycles\tdir\terr \t preA\tcurA\tpreB\tcurB\tpreI\tcurI\n");
#ifdef DBG_CHIPSCOPE
        else if ((k != 1) && (cnt_noEdge % mode_noedge == 0 || isNoEdge == false || (cnt_sec == 0))) {
#else
        else if ((k != 1) && (cnt_noEdge % mode_noedge == 0 || isNoEdge == false)) {
#endif
            printf(
                "DBG_QEI_WATCH:\t %d\t %d\t %d\t%x\t %x\t %d\t %x\t %x\t %d\t %d\t %d\t%d\t%d\t%d  \t %d\t %d\t "
                "%d\t%d\t%d\t%d",
                k, cnt_noEdge, isPreValid, edges_pre2.edges, edges_pre2.types, edges_pre2.timeStep, edges_cur.edges,
                edges_cur.types, edges_cur.timeStep, counter, speed_rpm, ii_cycles, dir, axi_qei_err,
                edges_pre2.edges & 1, edges_cur.edges & 1, (edges_pre2.edges >> 1) & 1, (edges_cur.edges >> 1) & 1,
                (edges_pre2.edges >> 2) & 1, (edges_cur.edges >> 2) & 1);
#ifdef DBG_CHIPSCOPE
            printf("\t %d\t %d\t %d \t%d \t%d", cnt_sec, freqA, freqB, cnt_A, cnt_B);
#endif //#ifdef DBG_CHIPSCOPE
            printf("\n");
        }
#endif //_DBG_QEI_WATCH_
#endif //__SYNTHESIS__

#ifndef __SYNTHESIS__
        k++;
#endif
    }
}
} // details

/**
 * @brief Quadrature Encoder Interface(QEI) control demo top interface and paramters list
 * @tparam T_bin The data type for ABI's signals
 * @tparam T_err The data type for qei's error status
 * @param strm_qei_A The input stream for A signals
 * @param strm_qei_B The input stream for B signals
 * @param strm_qei_I The input stream for I signals
 * @param strm_qei_RPM_THETA_m The output stream for rpm and theta_m
 * @param strm_qei_dir The output stream for direction value
 * @param strm_qei_err The output stream for error status value
 * @param qei_args_cpr Read for user setting or written back by kernel
 * @param qei_args_ctrl The lowest bit of this value indicates the encoding mode
 * @param qei_stts_RPM_THETA_m Rpm and theta_m written back by kernel on axi_lite port
 * @param qei_stts_dir Dir written back by kernel on axi_lite port
 * @param qei_stts_err Err written back by kernel on axi_lite port
 * @param qei_args_cnt_trip input of trip count
 */
template <class T_bin, class T_err>
void hls_qei_axi(hls::stream<T_bin>& strm_qei_A,
                 hls::stream<T_bin>& strm_qei_B,
                 hls::stream<T_bin>& strm_qei_I,
                 hls::stream<ap_uint<32> >& strm_qei_RPM_THETA_m,
                 hls::stream<T_bin>& strm_qei_dir,
                 hls::stream<T_err>& strm_qei_err,
                 volatile int& qei_args_cpr,
                 volatile int& qei_args_ctrl,
                 volatile int& qei_stts_RPM_THETA_m,
                 volatile int& qei_stts_dir,
                 volatile int& qei_stts_err,
                 volatile long& qei_args_cnt_trip) {
#pragma HLS DATAFLOW

    hls::stream<T_bin> strm_qei_A_out("strm_qei_A_out");
#pragma HLS STREAM depth = 16 variable = strm_qei_A_out

    hls::stream<T_bin> strm_qei_B_out("strm_qei_B_out");
#pragma HLS STREAM depth = 16 variable = strm_qei_B_out

    hls::stream<T_bin> strm_qei_I_out("strm_qei_I_out");
#pragma HLS STREAM depth = 16 variable = strm_qei_I_out

    hls::stream<T_bin> strm_gen_quit_end("strm_gen_quit_end");
#pragma HLS STREAM depth = 16 variable = strm_gen_quit_end

    hls::stream<ap_uint<4> > strm_qei_finalABI("strm_qei_finalABI");
#pragma HLS STREAM depth = 16 variable = strm_qei_finalABI

    hls::stream<details::QEI_EdgeInfo> strm_cntEdge("strm_cntEdge");
#pragma HLS STREAM depth = 16 variable = strm_cntEdge

    hls::stream<T_bin> strm_calc_end("strm_calc_end");
#pragma HLS STREAM depth = 16 variable = strm_calc_end

    details::filterIn<T_bin>(strm_qei_A, strm_qei_B, strm_qei_I, qei_args_cnt_trip, strm_qei_finalABI);
    details::catchingEdge<T_bin>(strm_qei_finalABI, strm_cntEdge, strm_calc_end);
    details::calcCounter<T_bin, T_err>(strm_cntEdge, strm_calc_end, strm_qei_RPM_THETA_m, strm_qei_dir, strm_qei_err,
                                       qei_args_cpr, qei_args_ctrl, qei_stts_RPM_THETA_m, qei_stts_dir, qei_stts_err);
}
} // xf
} // motorcontrol

#endif // _QEI_HPP_
