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
#ifndef __US_MODEL_MULT_HPP__
#define __US_MODEL_MULT_HPP__
#include "us_model_base.hpp"

template <class T>
void fun_genLineMult(int m_num_element,
                     int m_num_sample,
                     int m_num_upSample,
                     T* p_interpolation,
                     T* p_apodization,
                     T* m_p_data) { // call by LbyL
    for (int e = 0; e < m_num_element; e++) {
        for (int n = 0; n < m_num_sample; n++) {
            T apo = p_apodization[e * m_num_sample + n];
            int k_int_base = m_num_upSample * m_num_sample * e + m_num_upSample * n;
            int k_mult_base = m_num_upSample * n;
            for (int i = 0; i < m_num_upSample; i++) {
                m_p_data[k_mult_base + i] += apo * p_interpolation[k_int_base + i];
            }
        }
    }
}

template <class T>
struct para_Mult {
    int num_element;
    int num_sample;
    int num_upSample;
    para_Mult(int num_ele, int num_dep, int num_upSam) {
        num_element = num_ele;
        num_sample = num_dep;
        num_upSample = num_upSam;
    }
};

template <class T>
void fun_genLineMult(     // call by LbyL //call by UbyU
    T* m_p_data,          // output
    para_Mult<T>* p_para, // Self-iterated data structure
    T* p_interpolation,   // input
    T* p_apodization      // input
    ) {
    int num_element = p_para->num_element;
    int num_sample = p_para->num_sample;
    int num_upSample = p_para->num_upSample;
    fun_genLineMult(num_element, num_sample, num_upSample, p_interpolation, p_apodization, m_p_data);
}

template <class T, int NUM_LINE_t, int NUM_ELEMENT_t, int NUM_SAMPLE_t>
void fun_genLineMult_EbyE( // call by EbyE
    int e,
    T* m_p_data,          // output
    para_Mult<T>* p_para, // Self-iterated data structure
    T* p_interpolation,   // input
    T* p_apodization      // input
    ) {
    int m_num_sample = p_para->num_sample;
    int m_num_upSample = p_para->num_upSample;
    // for(int e = 0; e < m_num_element; e++)
    {
        for (int n = 0; n < m_num_sample; n++) {
            T apo = p_apodization[n];
            int k_mult_base = m_num_upSample * n;
            for (int i = 0; i < m_num_upSample; i++) {
                m_p_data[k_mult_base + i] += apo * p_interpolation[k_mult_base + i];
            }
        }
    }
}
template <class T>
struct DU_inter {
    T s[4];
};

template <class T, int NUM_LINE_t, int NUM_ELEMENT_t, int NUM_SAMPLE_t, int NUM_SEG_t, int ID_SEG_t>
void fun_genLineMult_UbyU_test( // call by EbyE
    int e,
    T* m_p_data,                                   // output
    para_Mult<T>* p_para,                          // Self-iterated data structure
    hls::stream<DU_inter<T> >& strm_interpolation, // input
    hls::stream<T>& strm_apodization               // input
    ) {
    int m_num_sample = p_para->num_sample;
    int m_num_upSample = p_para->num_upSample;

    hls::stream<T> strm_apodization_1;
    hls::stream<T> strm_apodization_2;
    hls::stream<T> strm_apodization_3;

    hls::stream<DU_inter<T> > strm_interpolation_1;
    hls::stream<DU_inter<T> > strm_interpolation_2;
    hls::stream<DU_inter<T> > strm_interpolation_3;
    {
        for (int n = 0; n < m_num_sample / NUM_SEG_t; n++) {
            T apo = strm_apodization.read(); // p_apodization[n];
            int k_mult_base = m_num_upSample * n;
            DU_inter<float> dint = strm_interpolation.read();
            strm_apodization_1.write(apo);
            strm_interpolation_1.write(dint);
            m_p_data[k_mult_base + ID_SEG_t + 0] +=
                apo * dint.s[ID_SEG_t + 0]; // strm_interpolation.read();//p_interpolation[k_mult_base + i];
        }
        for (int n = 0; n < m_num_sample / NUM_SEG_t; n++) {
            T apo = strm_apodization_1.read(); // p_apodization[n];
            int k_mult_base = m_num_upSample * n;
            DU_inter<float> dint = strm_interpolation_1.read();
            strm_apodization_2.write(apo);
            strm_interpolation_2.write(dint);
            m_p_data[k_mult_base + ID_SEG_t + 1] +=
                apo * dint.s[ID_SEG_t + 1]; // strm_interpolation.read();//p_interpolation[k_mult_base + i];
        }
        for (int n = 0; n < m_num_sample / NUM_SEG_t; n++) {
            T apo = strm_apodization_2.read(); // p_apodization[n];
            int k_mult_base = m_num_upSample * n;
            DU_inter<float> dint = strm_interpolation_2.read();
            ;
            strm_apodization_3.write(apo);
            strm_interpolation_3.write(dint);
            m_p_data[k_mult_base + ID_SEG_t + 2] +=
                apo * dint.s[ID_SEG_t + 2]; // strm_interpolation.read();//p_interpolation[k_mult_base + i];
        }
        for (int n = 0; n < m_num_sample / NUM_SEG_t; n++) {
            T apo = strm_apodization_3.read(); // p_apodization[n];
            int k_mult_base = m_num_upSample * n;
            DU_inter<float> dint = strm_interpolation_3.read();
            m_p_data[k_mult_base + ID_SEG_t + 3] +=
                apo * dint.s[ID_SEG_t + 3]; // strm_interpolation.read();//p_interpolation[k_mult_base + i];
        }
    }
}
template <class T, int NUM_LINE_t, int NUM_ELEMENT_t, int NUM_SAMPLE_t, int NUM_SEG_t, int ID_SEG_t>
void fun_genLineMult_UbyU( // call by EbyE
    int e,
    int seg,
    T* m_p_data,                                   // output
    para_Mult<T>* p_para,                          // Self-iterated data structure
    hls::stream<DU_inter<T> >& strm_interpolation, // input
    hls::stream<T>& strm_apodization,              // input
    hls::stream<DU_inter<T> >& strm_interpolation_out,
    hls::stream<T>& strm_apodization_out) {
    int m_num_sample = p_para->num_sample;
    int m_num_upSample = p_para->num_upSample;

    for (int n = 0; n < m_num_sample / NUM_SEG_t; n++) {
        T apo = strm_apodization.read(); // p_apodization[n];

        DU_inter<float> dint = strm_interpolation.read();
        if (ID_SEG_t != NUM_SEG_t - 1) {
            strm_apodization_out.write(apo);
            strm_interpolation_out.write(dint);
        }
        int base = m_num_sample / NUM_SEG_t * seg;
        m_p_data[base + n] += apo * dint.s[ID_SEG_t]; // strm_interpolation.read();//p_interpolation[k_mult_base + i];
        // m_p_data[k_mult_base + ID_SEG_t] += apo *
        // dint.s[ID_SEG_t];//strm_interpolation.read();//p_interpolation[k_mult_base + i];
    }
}

template <class T>
class us_op_mult : public us_model_base {
   public:
    T* m_p_data;

    us_op_mult() : us_model_base(7, "mult") {
        m_type = MODEL_MULT;
        genFullName();
    }

    us_op_mult(int id, const char* nm) : us_model_base(id, nm) {
        m_type = MODEL_MULT;
        genFullName();
    }

    void rstMem() { memset(m_p_data, 0, m_num_line * m_num_sample * m_num_upSample * sizeof(T)); }

    void init(us_device* p_dev) {
        us_model_base::copyPara(p_dev);
        m_p_data = (T*)malloc(m_num_line * m_num_sample * m_num_upSample * sizeof(T));
        rstMem();
    }
    ///////////////
    // call by MbyM
    ///////////////
    void genLineMult(int idx_line, T* p_interpolation, unsigned* p_inside, T* p_apodization) { // call by MbyM
        for (int e = 0; e < m_num_element; e++) {
            for (int n = 0; n < m_num_sample; n++) {
                T apo = p_apodization[m_num_element * m_num_sample * idx_line + e * m_num_sample + n];
                T inside = p_inside[m_num_element * m_num_sample * idx_line + e * m_num_sample + n];
                int k_int_base = m_num_element * m_num_sample * m_num_upSample * idx_line +
                                 m_num_upSample * m_num_sample * e + m_num_upSample * n;
                int k_mult_base = m_num_sample * m_num_upSample * idx_line + m_num_upSample * n;
                for (int i = 0; i < m_num_upSample; i++) {
                    m_p_data[k_mult_base + i] += apo * p_interpolation[k_int_base + i] * inside;
                }
            }
        }
    }
    ///////////////
    // call by MbyM
    ///////////////
    void runAll_Mult(T* p_interpolation, unsigned* p_inside, T* p_apodization) { // call by MbyM
        for (int line = 0; line < m_num_line; line++) genLineMult(line, p_interpolation, p_inside, p_apodization);
    }

    void print_member(FILE* fp) {
        fprintf(fp, "%s m_num_line  = %d\n", m_name_full, m_num_line);
        fprintf(fp, "%s m_num_element   = %d\n", m_name_full, m_num_element);
        fprintf(fp, "%s m_num_sample   = %d\n", m_name_full, m_num_sample);
    }

    void print_data(FILE* fp) { print_data(fp, 1); }

    void print_data(FILE* fp, int control) { printMult(fp, control); }

    void printMult(FILE* fp, int control) {
        assert(fp);
        if (control != 0)
            fprintf(fp,
                    "%s DATA FORMAT of All Mult: TxtPage:m_num_line(%d) by TxtLine:m_num_element(%d) by "
                    "TxtCol:m_num_sample(%d) \n",
                    m_name_full, m_num_line, m_num_element, m_num_sample);
        for (int line = 0; line < m_num_line; line++) {
            if ((control & PRINT_MTX) && (control & PRINT_IDX)) fprintf(fp, "%s_line_%d", m_name_full, line);
            printMult_line(fp, line, control);
            if (control & PRINT_MTX) fprintf(fp, "\n");
        }
    }

    void printMult_line(FILE* fp, int idx_line, int control) {
        assert(fp);

        for (int n = 0; n < m_num_sample; n++) {
            for (int i = 0; i < m_num_upSample; i++) {
                int k = m_num_sample * m_num_upSample * idx_line + m_num_upSample * n + i;
                if (control & PRINT_MTX) {
                    fprintf(fp, "%.9f\t", m_p_data[k]);
                } else {
                    if (control & PRINT_IDX)
                        fprintf(fp, "Line:%d\tsample:%d\tup:%d\t%.9f\n", idx_line, n, i, m_p_data[k]);
                    else
                        fprintf(fp, "%.9f\n", m_p_data[k]);
                }
            }
        }
        // if (control==1) fprintf(fp,"\n");
    }
    void save(int control, const char* surfix) {
        char fname[128];
        sprintf(fname, "%s_line%d_upSampled%d_%s", m_name_full, m_num_line, m_num_sample * m_num_upSample, surfix);
        FILE* fp = fopen(fname, "wb");
        assert(fp);
        print_data(fp, control);
        fclose(fp);
    }
    ~us_op_mult() {
        if (m_p_data) free(m_p_data);
    }

    void save4aie(int format, const char* nm, int line, int sample) {
        // char fname[300];
        // sprintf(fname, "%s_format%d_line%dof%d_smp_%dof%d.mul", nm, format, line, m_num_line, sample, m_num_sample);
        // sprintf(fname, "%s_format%d_L%d_E%d.mul", nm, format, line, sample);
        const char* str_f[2] = {"col", "mtx"};
        sprintf(fname[DATA_MUL], "%s_L%d_S%d.mul.%s", nm, line, sample, str_f[format & PRINT_MTX]);
        printf("MODEL_TEST_SCANLINE: Saving mult data in file %s\n", fname[DATA_MUL]);
        FILE* fp = fopen(fname[DATA_MUL], "w");
        assert(line <= m_num_line);
        assert(sample <= m_num_sample);
        assert(fp);
        for (int l = 0; l < line; l++) {
            if ((format & PRINT_MTX) && (format & PRINT_IDX)) fprintf(fp, "line:%d:\t", l);
            for (int s = 0; s < sample * m_num_upSample; s++) {
                fprintf(fp, "%.9f", this->m_p_data[l * m_num_sample * m_num_upSample + s]);
                if (!(format & PRINT_MTX))
                    fprintf(fp, "\n");
                else
                    fprintf(fp, "\t");
            }
            if ((format & PRINT_MTX)) fprintf(fp, "\n");
        } // for l
        fclose(fp);
    }
    void save4aie(const char* nm) { save4aie(0, nm, m_num_line, m_num_sample); }
};
#endif