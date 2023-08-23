/*
 * Copyright (C) 2019-2022, Xilinx, Inc.
 * Copyright (C) 2022-2023, Advanced Micro Devices, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef __US_OP_INTERPOLATION_HPP__
#define __US_OP_INTERPOLATION_HPP__
#include "us_model_base.hpp"
#include <ap_int.h>
#include <hls_stream.h>

template <class T>
struct Window_inter {
    T s[4];
    void push(T v) {
        for (int i = 0; i < 3; i++) s[i] = s[i + 1];
        s[3] = v;
    }
};

template <class T>
struct para_Interpolation {
    int iter;
    int num_element;
    int num_sample;
    int num_upSample;
    Window_inter<T> window;

    para_Interpolation(int n_ele, int n_dep, int n_Samp, const unsigned short* rf_dat_dim) {
        iter = 0;
        num_element = n_ele;
        num_sample = n_dep;
        num_upSample = n_Samp;
    }
    void selfUpdate() { iter++; }
};
///////////////
// call by MbyM
///////////////
template <class T>
void fun_genLineInterpolation(int m_num_element,
                              int m_num_sample,
                              int m_num_upSample,
                              T* p_rf,
                              unsigned short rf_data_dim,
                              unsigned* m_p_data_ins,
                              T* m_p_data_smp,
                              T* p_sample,
                              T* m_p_data_int) { // call by MbyM
    for (int e = 0; e < m_num_element; e++) {
        for (int n = 0; n < m_num_sample; n++) {
            int k = e * m_num_sample + n;
            m_p_data_ins[k] = (p_sample[k] >= 1) && (p_sample[k] < rf_data_dim);
            m_p_data_smp[k] = p_sample[k] * m_p_data_ins[k] + 1 - m_p_data_ins[k];
        }

        for (int n = 0; n < m_num_sample; n++) {
            T s[6];
            int k_sample_base = e * m_num_sample + n;
            s[0] = p_rf[(int)m_p_data_smp[k_sample_base] * m_num_element + e];
            s[1] = (n + 1) < m_num_sample ? p_rf[(int)m_p_data_smp[k_sample_base + 1] * m_num_element + e] : s[0];
            s[2] = (n + 1) < m_num_sample ? p_rf[(int)m_p_data_smp[k_sample_base + 1] * m_num_element + e] : s[0];
            s[3] = (n + 2) < m_num_sample ? p_rf[(int)m_p_data_smp[k_sample_base + 2] * m_num_element + e] : s[1];
            s[4] = (n + 2) < m_num_sample ? p_rf[(int)m_p_data_smp[k_sample_base + 2] * m_num_element + e] : s[1];
            s[5] = (n + 3) < m_num_sample ? p_rf[(int)m_p_data_smp[k_sample_base + 3] * m_num_element + e] : s[2];

            int k_intep_base = m_num_upSample * m_num_sample * e + m_num_upSample * n;
            T vec[4] = {1, 1.33333333, 1.66666667, 2};
            for (int i = 0; i < m_num_upSample; i++) {
                T A1 = (1 - vec[i]) * s[0] + vec[i] * s[1];
                T A2 = (2 - vec[i]) * s[2] + (vec[i] - 1) * s[3];
                T A3 = (3 - vec[i]) * s[4] + (vec[i] - 2) * s[5];

                T B1 = (2 - vec[i]) * 0.5 * A1 + vec[i] * 0.5 * A2;
                T B2 = (3 - vec[i]) * 0.5 * A2 + (vec[i] - 1) * 0.5 * A3;

                T C = (2 - vec[i]) * B1 + (vec[i] - 1) * B2;

                m_p_data_int[k_intep_base + i] = C;
            }
        }
    }
}

template <class T>
void fun_genLineInterpolation2( // call by LbyL
    T* m_p_data_int,
    para_Interpolation<T>* p_para,
    T* p_rf, // input
    T* p_sample,
    unsigned* m_p_data_ins) {
    int m_num_element = p_para->num_element;
    int m_num_sample = p_para->num_sample;
    int m_num_upSample = p_para->num_upSample;
    for (int e = 0; e < m_num_element; e++) {
        for (int n = 0; n < m_num_sample; n++) {
            T s[6];
            int k_sample_base = e * m_num_sample + n;
            s[0] = p_rf[(int)p_sample[k_sample_base] * m_num_element + e];

            s[1] = (n + 1) < m_num_sample ? p_rf[(int)p_sample[k_sample_base + 1] * m_num_element + e] : s[0];
            s[2] = (n + 1) < m_num_sample ? p_rf[(int)p_sample[k_sample_base + 1] * m_num_element + e] : s[0];
            s[3] = (n + 2) < m_num_sample ? p_rf[(int)p_sample[k_sample_base + 2] * m_num_element + e] : s[1];
            s[4] = (n + 2) < m_num_sample ? p_rf[(int)p_sample[k_sample_base + 2] * m_num_element + e] : s[1];
            s[5] = (n + 3) < m_num_sample ? p_rf[(int)p_sample[k_sample_base + 3] * m_num_element + e] : s[2];

            T vec[4] = {1, 1.33333333, 1.66666667, 2};
            for (int i = 0; i < m_num_upSample; i++) {
                T A1 = (1 - vec[i]) * s[0] + vec[i] * s[1];
                T A2 = (2 - vec[i]) * s[2] + (vec[i] - 1) * s[3];
                T A3 = (3 - vec[i]) * s[4] + (vec[i] - 2) * s[5];

                T B1 = (2 - vec[i]) * 0.5 * A1 + vec[i] * 0.5 * A2;
                T B2 = (3 - vec[i]) * 0.5 * A2 + (vec[i] - 1) * 0.5 * A3;

                T C = (2 - vec[i]) * B1 + (vec[i] - 1) * B2;

                m_p_data_int[k_sample_base * m_num_upSample + i] = C * m_p_data_ins[k_sample_base];
            }
        }
    }
}

template <class T, int NUM_LINE_t, int NUM_ELEMENT_t, int NUM_SAMPLE_t>
void fun_genLineInterpolation_EbyE( // call by EbyE
    int line,
    int e,
    T* m_p_data_int,
    para_Interpolation<T>* p_para,
    T* p_rf, // input
    T* p_sample,
    unsigned* m_p_data_ins) {
    int m_num_sample = p_para->num_sample;
    int m_num_upSample = p_para->num_upSample;

    Window_inter<T> win_w;
    T v_resmp = 0;
    T* s; //[4];
    hls::stream<Window_inter<T> > strm_win;
    for (int n = 0; n < m_num_sample + m_num_upSample - 1; n++) {
        if (n < m_num_sample) v_resmp = p_rf[(int)p_sample[n]];
        win_w.push(v_resmp);
        if (n < m_num_upSample - 1) continue;
        strm_win.write(win_w);
    }

    for (int n = 0; n < m_num_sample; n++) {
        Window_inter<T> win_r = strm_win.read();
        s = win_r.s;
        T vec[4] = {1, 1.33333333, 1.66666667, 2};
        for (int i = 0; i < m_num_upSample; i++) {
            T A1 = (1 - vec[i]) * s[0] + vec[i] * s[1];
            T A2 = (2 - vec[i]) * s[1] + (vec[i] - 1) * s[2];
            T A3 = (3 - vec[i]) * s[2] + (vec[i] - 2) * s[3];
            T B1 = (2 - vec[i]) * 0.5 * A1 + vec[i] * 0.5 * A2;
            T B2 = (3 - vec[i]) * 0.5 * A2 + (vec[i] - 1) * 0.5 * A3;
            T C = (2 - vec[i]) * B1 + (vec[i] - 1) * B2;
            m_p_data_int[(n)*m_num_upSample + i] = C * m_p_data_ins[(n)];
        }
    }
}

template <class T, int NUM_LINE_t, int NUM_ELEMENT_t, int NUM_SAMPLE_t, int NUM_SEG_t>
void fun_genLineInterpolation_UbyU_resampling( // call by UbyU
    para_Interpolation<T>* p_para,
    T* p_rf,     // the rf_data line can't be partitioned as the resampling address may cross the edges of segments
    T* p_sample, // divided by NUM_SEG_t
    hls::stream<T>& strm_resmp) {
    int m_num_sample = p_para->num_sample;
    int m_num_upSample = p_para->num_upSample;

    // hls::stream< T > strm_resmp;
    for (int n = 0; n < m_num_sample / NUM_SEG_t; n++) {
        T v_resmp = p_rf[(int)p_sample[n]];
        strm_resmp.write(v_resmp);
    }
}

template <class T, int NUM_LINE_t, int NUM_ELEMENT_t, int NUM_SAMPLE_t, int NUM_SEG_t>
void fun_genLineInterpolation_UbyU_window( // call by UbyU
    int seg,
    para_Interpolation<T>* p_para,
    hls::stream<T>& strm_resmp,
    hls::stream<Window_inter<T> >& strm_win) {
    int m_num_sample = p_para->num_sample;
    int m_num_upSample = p_para->num_upSample;

    T v_resmp;
    // Window_inter<T> win_w;
    int tripcount = seg != 3 ? m_num_sample / NUM_SEG_t : m_num_sample / NUM_SEG_t + m_num_upSample - 1;
    for (int n = 0; n < tripcount; n++) {
        if (n < m_num_sample / NUM_SEG_t) v_resmp = strm_resmp.read(); // p_rf[(int)p_sample[n]];
        p_para->window.push(v_resmp);
        if ((seg == 0) && (n < m_num_upSample - 1)) continue;
        strm_win.write(p_para->window);
    }
}

template <class T, int NUM_LINE_t, int NUM_ELEMENT_t, int NUM_SAMPLE_t, int NUM_SEG_t>
void fun_genLineInterpolation_UbyU_window_sync( // call by UbyU
    int seg,
    para_Interpolation<T>* p_para,
    hls::stream<T>& strm_resmp,
    hls::stream<Window_inter<T> >& strm_win) {
    int m_num_sample = p_para->num_sample;
    int m_num_upSample = p_para->num_upSample;

    T v_resmp;
    // Window_inter<T> win_w;
    for (int n = 0; n < m_num_sample / NUM_SEG_t; n++) {
        v_resmp = strm_resmp.read();
        p_para->window.push(v_resmp);
        if (seg == 0) {
            p_para->window.push(v_resmp);
            p_para->window.push(v_resmp);
            p_para->window.push(v_resmp);
        }
        strm_win.write(p_para->window);
    }
}

template <class T, int NUM_LINE_t, int NUM_ELEMENT_t, int NUM_SAMPLE_t, int NUM_SEG_t>
void fun_genLineInterpolation_UbyU_window_sync2( // call by UbyU
    int seg,
    para_Interpolation<T>* p_para,
    hls::stream<T>& strm_resmp,
    hls::stream<Window_inter<T> >& strm_win) {
    int m_num_sample = p_para->num_sample;
    int m_num_upSample = p_para->num_upSample;

    T v_resmp = strm_resmp.read();
    Window_inter<T> win_w;
    win_w.push(v_resmp);
    win_w.push(v_resmp);
    win_w.push(v_resmp);
    win_w.push(v_resmp);
    strm_win.write(win_w);
    for (int n = 1; n < m_num_sample / NUM_SEG_t; n++) {
        v_resmp = strm_resmp.read();
        win_w.push(v_resmp);
        strm_win.write(win_w);
    }
}

template <class T, int NUM_LINE_t, int NUM_ELEMENT_t, int NUM_SAMPLE_t, int NUM_SEG_t>
void fun_genLineInterpolation_UbyU_window_sync3( // call by UbyU
    int seg,
    para_Interpolation<T>* p_para,
    hls::stream<T>& strm_resmp,
    hls::stream<Window_inter<T> >& strm_win) {
    int m_num_sample = p_para->num_sample;
    int m_num_upSample = p_para->num_upSample;

    T v_resmp;
    Window_inter<T> win_w;
    win_w.push(strm_resmp.read());
    win_w.push(strm_resmp.read());
    win_w.push(strm_resmp.read());

    for (int n = 3; n < m_num_sample / NUM_SEG_t; n++) {
        v_resmp = strm_resmp.read();
        win_w.push(v_resmp);
        strm_win.write(win_w);
    }
    win_w.push(v_resmp);
    strm_win.write(win_w);
    win_w.push(v_resmp);
    strm_win.write(win_w);
    win_w.push(v_resmp);
    strm_win.write(win_w);
}

template <class T, int NUM_LINE_t, int NUM_ELEMENT_t, int NUM_SAMPLE_t, int NUM_SEG_t>
void fun_genLineInterpolation_UbyU_bSpline( // call by UbyU
    T* m_p_data_int,                        // divided by NUM_SEG_
    para_Interpolation<T>* p_para,
    hls::stream<unsigned>& strm_ins,
    hls::stream<Window_inter<T> >& strm_win) {
    int m_num_sample = p_para->num_sample;
    int m_num_upSample = p_para->num_upSample;

    T* s; //[4];
    T v_resmp;
    Window_inter<T> win_w;
    for (int n = 0; n < m_num_sample / NUM_SEG_t; n++) {
        Window_inter<T> win_r = strm_win.read();
        s = win_r.s;
        T vec[4] = {1, 1.33333333, 1.66666667, 2};
        // unsigned isIns = m_p_data_ins[(n)];
        unsigned isIns = strm_ins.read();
        for (int i = 0; i < m_num_upSample; i++) {
            T A1 = (1 - vec[i]) * s[0] + vec[i] * s[1];
            T A2 = (2 - vec[i]) * s[1] + (vec[i] - 1) * s[2];
            T A3 = (3 - vec[i]) * s[2] + (vec[i] - 2) * s[3];
            T B1 = (2 - vec[i]) * 0.5 * A1 + vec[i] * 0.5 * A2;
            T B2 = (3 - vec[i]) * 0.5 * A2 + (vec[i] - 1) * 0.5 * A3;
            T C = (2 - vec[i]) * B1 + (vec[i] - 1) * B2;
            m_p_data_int[(n)*m_num_upSample + i] = C * isIns;
        }
    }
}

template <class T>
void fun_genLineInterpolation_strm( // call by UbyU
    T* m_p_data_int,
    para_Interpolation<T>* p_para,
    hls::stream<unsigned>& strm_inside,
    hls::stream<T>& strm_sample,
    hls::stream<T>& strm_rfdata) {
    Window_inter<T> win_w;

    int m_num_element = p_para->num_element;
    int m_num_sample = p_para->num_sample;
    int m_num_upSample = p_para->num_upSample;
    T p_rfdata[m_num_sample];

    for (int e = 0; e < m_num_element; e++) {
        ////////////////////////////////////////
        // re-sample sol-1
        ////////////////////////////////////////
        hls::stream<Window_inter<T> > strm_win("strm_win");
        hls::stream<T> strm_resample("strm_resample");

        for (int n = 0; n < m_num_sample; n++) {
            p_rfdata[n] = strm_rfdata.read();
        }

        for (int n = 0; n < m_num_sample; n++) {
            int v_add = (int)strm_sample.read();
            T v_resmp = p_rfdata[v_add];
            strm_resample.write(v_resmp);
        }
        ////////////////////////////////////////
        // kernel core for gen_window
        ////////////////////////////////////////
        T v_resmp = 0;
        for (int n = 0; n < m_num_sample + 3; n++) {
            if (n < m_num_sample) v_resmp = strm_resample.read();
            win_w.push(v_resmp);
            if (n >= 3) strm_win.write(win_w);
        }
        /////////////////////////////////////////
        // kernel for pure interpolation
        /////////////////////////////////////////
        for (int n = 0; n < m_num_sample; n++) {
            unsigned isInside = strm_inside.read();
            Window_inter<T> win_r = strm_win.read();
            T s[6];
            int k_sample_base = e * m_num_sample + n;

            s[0] = win_r.s[0];
            s[1] = win_r.s[1];
            s[2] = win_r.s[2];
            s[3] = win_r.s[3];

            T vec[4] = {1, 1.33333333, 1.66666667, 2};
            for (int i = 0; i < m_num_upSample; i++) {
                T A1 = (1 - vec[i]) * s[0] + vec[i] * s[1];
                T A2 = (2 - vec[i]) * s[1] + (vec[i] - 1) * s[2];
                T A3 = (3 - vec[i]) * s[2] + (vec[i] - 2) * s[3];
                T B1 = (2 - vec[i]) * 0.5 * A1 + vec[i] * 0.5 * A2;
                T B2 = (3 - vec[i]) * 0.5 * A2 + (vec[i] - 1) * 0.5 * A3;
                T C = (2 - vec[i]) * B1 + (vec[i] - 1) * B2;

                m_p_data_int[k_sample_base * m_num_upSample + i] = C * isInside; // m_p_data_ins[k_sample_base];
            }
        } // printf(" \n");
    }
}

template <class T>
void fun_genLineInterpolation_warpper(
    T* p_interpolation, para_Interpolation<T>* p_para, T* p_rf, T* p_sample, unsigned* p_inside) { // call by LbyL
    fun_genLineInterpolation2<T>(p_interpolation, p_para, p_rf, p_sample, p_inside);
    p_para->selfUpdate();
}

template <class T>
void fun_genLineInterpolation_warpper_strm(T* p_interpolation,
                                           para_Interpolation<T>* p_para,
                                           hls::stream<unsigned>& strm_inside,
                                           hls::stream<T>& strm_sample,
                                           hls::stream<T>& strm_rfdata) { // call by UbyU
    // printf("MODEL_TEST_SCANLINE_LbyL: fun_genLineInterpolation line: %d \t\n", line);
    fun_genLineInterpolation_strm<T>(p_interpolation, //&(m_p_data_int[n_ele * n_dep * n_Samp * line]),
                                     p_para, strm_inside, strm_sample, strm_rfdata);
    p_para->selfUpdate();
}
//---------------------------done----------------------------//
///////////////
// call by MbyM
///////////////
template <class T>
void fun_runAll_Interpolation(int m_num_line,
                              int m_num_element,
                              int m_num_sample,
                              int m_num_upSample,
                              T* p_rf,
                              unsigned* m_p_data_ins,
                              T* m_p_data_smp,
                              T* p_sample,
                              T* m_p_data_int) { // call by MbyM
    for (int line = 0; line < m_num_line; line++) {
        unsigned short rf_data_dim = example_1_rf_data_dim[line];
        fun_genLineInterpolation(
            m_num_element, m_num_sample, m_num_upSample, &(p_rf[m_num_element * m_num_sample * line]), rf_data_dim,
            &(m_p_data_ins[m_num_element * m_num_sample * line]), &(m_p_data_smp[m_num_element * m_num_sample * line]),
            &(p_sample[m_num_element * m_num_sample * line]),
            &(m_p_data_int[m_num_element * m_num_sample * m_num_upSample * line]));
    }
}

template <class T>
class us_op_interpolation : public us_model_base {
   public:
    T* m_p_data_int;
    T* m_p_data_smp;
    unsigned* m_p_data_ins;

    us_op_interpolation() : us_model_base(6, "Interpolation") {
        m_type = MODEL_INTERPOLATION;
        genFullName();
    }
    us_op_interpolation(int id, const char* nm) : us_model_base(id, nm) {
        m_type = MODEL_INTERPOLATION;
        genFullName();
    }

    void init(us_device* p_dev) {
        us_model_base::copyPara(p_dev);
        m_p_data_int = (T*)malloc(m_num_line * m_num_sample * m_num_element * m_num_upSample * sizeof(T));
        m_p_data_ins = (unsigned*)malloc(m_num_line * m_num_sample * m_num_element * sizeof(unsigned));
        m_p_data_smp = (T*)malloc(m_num_line * m_num_element * (m_num_sample + 3) * sizeof(T));
        memset(m_p_data_smp, 0, m_num_line * m_num_element * (m_num_sample + 3) * sizeof(T));
    }

    hls::stream<short> strm_resmp;
    ///////////////
    // call by MbyM
    ///////////////
    void runAll_Interpolation(T* p_rf, T* p_sample) { // call by MbyM
        fun_runAll_Interpolation<float>(m_num_line, m_num_element, m_num_sample, m_num_upSample, p_rf, m_p_data_ins,
                                        m_p_data_smp, p_sample, m_p_data_int);
    }

    void print_member(FILE* fp) {
        fprintf(fp, "%s m_num_line      = %d\n", m_name_full, m_num_line);
        fprintf(fp, "%s m_num_element   = %d\n", m_name_full, m_num_element);
        fprintf(fp, "%s m_num_sample    = %lx\n", m_name_full, m_num_sample);
    }

    void print_data(FILE* fp) { print_data(fp, 1); }

    void print_data(FILE* fp, int control) { printInterpolation(fp, control); }

    void printInterpolation(FILE* fp, int control) {
        assert(fp);
        fprintf(fp,
                "%s DATA FORMAT of All Interpolation: TxtPage:m_num_line(%d) by TxtLine:m_num_element(%d) by "
                "TxtCol:m_num_sample(%d) \n",
                m_name_full, m_num_line, m_num_element, m_num_sample);
        for (int line = 0; line < m_num_line; line++) {
            printInterpolation_line(fp, line, control);
            fprintf(fp, "\n");
        }
    }

    void printInterpolation_line(FILE* fp, int idx_line, int control) {
        assert(fp);
        if (control) fprintf(fp, "%s_line_%d\n", m_name_full, idx_line);
        char print_format[128];
        if (control == 1)
            strcpy(print_format, "\t%.9f");
        else
            strcpy(print_format, "%.15f\n");
        for (int e = 0; e < m_num_element; e++) {
            for (int n = 0; n < m_num_sample; n++) {
                for (int i = 0; i < m_num_upSample; i++) {
                    int k = m_num_element * m_num_sample * m_num_upSample * idx_line +
                            m_num_upSample * m_num_sample * e + m_num_upSample * n + i;
                    fprintf(fp, print_format, m_p_data_int[k]);
                }
            }
            if (control == 1) fprintf(fp, "\n");
        }
    }

    void printInside(FILE* fp, int control) {
        assert(fp);
        fprintf(fp,
                "%s DATA FORMAT of All Inside: TxtPage:m_num_line(%d) by TxtLine:m_num_element(%d) by "
                "TxtCol:m_num_sample(%d) \n",
                m_name_full, m_num_line, m_num_element, m_num_sample);
        for (int line = 0; line < m_num_line; line++) {
            printInside_line(fp, line, control);
            fprintf(fp, "\n");
        }
    }

    void printInside_line(FILE* fp, int idx_line, int control) {
        assert(fp);
        if (control) fprintf(fp, "%s_line_%d\n", m_name_full, idx_line);
        char print_format[128];
        if (control == 1)
            strcpy(print_format, "\t%d");
        else
            strcpy(print_format, "%d\n");
        for (int e = 0; e < m_num_element; e++) {
            for (int n = 0; n < m_num_sample; n++) {
                int k = m_num_element * m_num_sample * idx_line + m_num_sample * e + n;
                fprintf(fp, print_format, m_p_data_ins[k]);
            }
            if (control == 1) fprintf(fp, "\n");
        }
    }

    ~us_op_interpolation() {
        if (m_p_data_int) free(m_p_data_int);
        if (m_p_data_smp) free(m_p_data_smp);
        if (m_p_data_ins) free(m_p_data_ins);
    }

    char* save4aie_int(int format, const char* nm, int line, int element, int sample) {
        // char fname[DATA_INT][128];
        // sprintf(fname[DATA_INT], "%s_format%d_line%dof%d_ele%dof%d_smp_%dof%d.int", nm, format, line, m_num_line,
        // element, m_num_element, sample, m_num_sample);
        // sprintf(fname[DATA_INT], "%s_format%d_L%d_E%d_S%d.int", nm, format, line, element, sample);
        const char* str_f[2] = {"col", "mtx"};
        sprintf(fname[DATA_INT], "%s_L%d_E%d_S%d.int.%s", nm, line, element, sample, str_f[format & PRINT_MTX]);
        printf("MODEL_TEST_SCANLINE: Saving int data in file %s\n", fname[DATA_INT]);
        FILE* fp = fopen(fname[DATA_INT], "w");
        assert(line <= m_num_line);
        assert(sample <= m_num_sample);
        assert(fp);
        for (int l = 0; l < line; l++) {
            for (int e = 0; e < element; e++) {
                if ((format & PRINT_MTX) && (format & PRINT_IDX)) fprintf(fp, "line:%d:ele:%d\t", l, e);
                for (int s = 0; s < sample * m_num_upSample; s++) {
                    fprintf(fp, "%.9f", this->m_p_data_int[l * m_num_sample * m_num_element * m_num_upSample +
                                                           e * m_num_sample * m_num_upSample + s]);
                    if (!(format & PRINT_MTX))
                        fprintf(fp, "\n");
                    else
                        fprintf(fp, "\t");
                }
                if ((format & PRINT_MTX)) fprintf(fp, "\n");
            } // for e
        }     // for l
        fclose(fp);
        return fname[DATA_INT];
    }
    char* save4aie_ins(int format, const char* nm, int line, int element, int sample) {
        // char fname[DATA_INS][300];
        // sprintf(fname[DATA_INS], "%s_format%d_line%dof%d_ele%dof%d_smp_%dof%d.ins", nm, format, line, m_num_line,
        // element, m_num_element, sample, m_num_sample);
        // sprintf(fname[DATA_INS], "%s_format%d_L%d_E%d_S%d.ins", nm, format, line,  element, sample);
        const char* str_f[2] = {"col", "mtx"};
        sprintf(fname[DATA_INS], "%s_L%d_E%d_S%d.ins.%s", nm, line, element, sample, str_f[format & PRINT_MTX]);
        printf("MODEL_TEST_SCANLINE: Saving inside data in file %s\n", fname[DATA_INS]);
        FILE* fp = fopen(fname[DATA_INS], "w");
        assert(line <= m_num_line);
        assert(sample <= m_num_sample);
        assert(fp);
        for (int l = 0; l < line; l++) {
            for (int e = 0; e < element; e++) {
                if ((format & PRINT_MTX) && (format & PRINT_IDX)) fprintf(fp, "line:%d:ele:%d\t", l, e);
                for (int s = 0; s < sample; s++) {
                    fprintf(fp, "%d", this->m_p_data_ins[l * m_num_sample * m_num_element + e * m_num_sample + s]);
                    if (!(format & PRINT_MTX))
                        fprintf(fp, "\n");
                    else
                        fprintf(fp, "\t");
                }
                if ((format & PRINT_MTX)) fprintf(fp, "\n");
            } // for e
        }     // for l
        fclose(fp);
        return fname[DATA_INS];
    }
    char* save4aie(int format, const char* nm, int line, int element, int sample) {
        save4aie_ins(format, nm, line, element, sample);
        save4aie_int(format, nm, line, element, sample);
        return fname[DATA_INT];
    }
    char* save4aie(const char* nm) {
        save4aie(0, nm, m_num_line, m_num_element, m_num_sample);
        return fname[DATA_INT];
    }
};
#endif