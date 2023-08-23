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
#ifndef __US_MODEL_SAMPLE_HPP__
#define __US_MODEL_SAMPLE_HPP__
#include "us_model_base.hpp"

///////////////
// call by MbyM
///////////////
template <class T>
void fun_genLineSample(int m_num_sample,
                       int m_num_element,
                       int m_freq_sampling,
                       T m_inv_speed_of_sound,
                       T p_xdc_def_pos[][4],
                       T* p_points_x,
                       T* p_points_z,
                       T* p_delay,
                       T* m_p_data) { // call by MbyM
    for (int e = 0; e < m_num_element; e++) {
        T xdc_x = p_xdc_def_pos[e][DIM_X];
        T xdc_z = p_xdc_def_pos[e][DIM_Z];
        for (int n = 0; n < m_num_sample; n++) {
            T x = p_points_x[n] - xdc_x;
            T z = p_points_z[n] - xdc_z;
            T rec_delay = sqrt(x * x + z * z) * m_inv_speed_of_sound;
            T total_delay = rec_delay + p_delay[n];
            int k = e * m_num_sample + n;
            m_p_data[k] = total_delay * m_freq_sampling + 1;
        }
    }
}

template <class T>
struct para_Sample {
    int iter;
    int num_sample;
    int num_element;
    int freq_sampling;
    T inverse_speed_of_sound;
    para_Sample(int n_dep, int n_ele, int freq, T inv_speed) {
        num_sample = n_dep;
        num_element = n_ele;
        freq_sampling = freq;
        inverse_speed_of_sound = inv_speed;
    }
    void selfUpdate() { iter++; }
};

template <class T>
struct para_Sample2 {
    int iter;
    int num_sample;
    int num_element;
    int freq_sampling;
    T inverse_speed_of_sound;
    const unsigned short* p_rf_data_dim;
    para_Sample2(int n_dep, int n_ele, int freq, T inv_speed, const unsigned short* p_dim) {
        iter = 0;
        num_sample = n_dep;
        num_element = n_ele;
        freq_sampling = freq;
        inverse_speed_of_sound = inv_speed;
        p_rf_data_dim = p_dim;
    }
    T getsDim() { return p_rf_data_dim[iter]; }
    void selfUpdate() { iter++; }
};

template <class T>
void fun_genLineSample( // call by LbyL //call by UbyU
    T* m_p_data,        // output
    unsigned int* m_p_inside,
    para_Sample2<T>* p_para, // Self-iterated data structure
    T* p_xdc_def_pos_4d,     // const array: up to 256(m_num_element) * 4 * sizeof(float)
    T* p_points_x,           // input
    T* p_points_z,           // input
    T* p_delay               // input
    ) {
    int num_sample = p_para->num_sample;
    int num_element = p_para->num_element;
    int freq_sampling = p_para->freq_sampling;
    T inv_speed_of_sound = p_para->inverse_speed_of_sound;
    unsigned short rf_data_dim = p_para->getsDim();
    for (int e = 0; e < num_element; e++) {
        T xdc_x = p_xdc_def_pos_4d[e * 4 + DIM_X];
        T xdc_z = p_xdc_def_pos_4d[e * 4 + DIM_Z];
        for (int n = 0; n < num_sample; n++) {
            T x = p_points_x[n] - xdc_x;
            T z = p_points_z[n] - xdc_z;
            T rec_delay = sqrt(x * x + z * z) * inv_speed_of_sound;
            T total_delay = rec_delay + p_delay[n];
            int k = e * num_sample + n;
            T t_sample = total_delay * freq_sampling + 1;

            // m_p_inside[k] = (t_sample >= 1) && (t_sample < rf_data_dim);
            // m_p_data[k] = t_sample * m_p_inside[k] + 1 - m_p_inside[k];

            m_p_inside[k] = (t_sample >= 1) && (t_sample < rf_data_dim);
            m_p_data[k] = t_sample * m_p_inside[k] + 1 - m_p_inside[k];
        }
    }
    p_para->selfUpdate();
}
template <class T, int NUM_LINE_t, int NUM_ELEMENT_t, int NUM_SAMPLE_t>
void fun_genLineSample_EbyE2( // call by LbyL //call by UbyU
    int line,
    int e,
    T* m_p_data,              // output
    unsigned int* m_p_inside, // output
    para_Sample2<T>* p_para,  // Self-iterated data structure
    T* p_xdc_def_pos_4d,      // const array: up to 256(m_num_element) * 4 * sizeof(float)
    T* p_points_x,            // input
    T* p_points_z,            // input
    T* p_delay                // input
    ) {
    int num_sample = p_para->num_sample;
    int num_element = p_para->num_element;
    int freq_sampling = p_para->freq_sampling;
    T inv_speed_of_sound = p_para->inverse_speed_of_sound;
    unsigned short rf_data_dim = p_para->p_rf_data_dim[line]; // getsDim();
    // for(int e = 0; e < num_element; e++)
    {
        T xdc_x = p_xdc_def_pos_4d[e * 4 + DIM_X];
        T xdc_z = p_xdc_def_pos_4d[e * 4 + DIM_Z];
        for (int n = 0; n < num_sample; n++) {
            T x = p_points_x[n] - xdc_x;
            T z = p_points_z[n] - xdc_z;
            T rec_delay = sqrt(x * x + z * z) * inv_speed_of_sound;
            T total_delay = rec_delay + p_delay[n];
            int k = e * num_sample + n;
            T t_sample = total_delay * freq_sampling + 1;
            m_p_inside[k] = (t_sample >= 1) && (t_sample < rf_data_dim);
            m_p_data[k] = t_sample * m_p_inside[k] + 1 - m_p_inside[k];
        }
    }
    p_para->selfUpdate();
}
template <class T, int NUM_LINE_t, int NUM_ELEMENT_t, int NUM_SAMPLE_t>
void fun_genLineSample_EbyE( // call by LbyL //call by UbyU
    int line,
    int e,
    T* m_p_data,              // output
    unsigned int* m_p_inside, // output
    para_Sample2<T>* p_para,  // Self-iterated data structure
    T* p_xdc_def_pos_4d,      // const array: up to 256(m_num_element) * 4 * sizeof(float)
    T* p_points_x,            // input
    T* p_points_z,            // input
    T* p_delay                // input
    ) {
    int num_sample = p_para->num_sample;
    int num_element = p_para->num_element;
    int freq_sampling = p_para->freq_sampling;
    T inv_speed_of_sound = p_para->inverse_speed_of_sound;
    unsigned short rf_data_dim = p_para->p_rf_data_dim[line]; // getsDim();
    // for(int e = 0; e < num_element; e++)
    {
        T xdc_x = p_xdc_def_pos_4d[e * 4 + DIM_X];
        T xdc_z = p_xdc_def_pos_4d[e * 4 + DIM_Z];
        for (int n = 0; n < num_sample; n++) {
            T x = p_points_x[n] - xdc_x;
            T z = p_points_z[n] - xdc_z;
            T rec_delay = sqrt(x * x + z * z) * inv_speed_of_sound;
            T total_delay = rec_delay + p_delay[n];
            int k = e * num_sample + n;
            T t_sample = total_delay * freq_sampling + 1;
            m_p_inside[n] = (t_sample >= 1) && (t_sample < rf_data_dim);
            m_p_data[n] = t_sample * m_p_inside[n] + 1 - m_p_inside[n];
        }
    }
    p_para->selfUpdate();
}

template <class T, int NUM_LINE_t, int NUM_ELEMENT_t, int NUM_SAMPLE_t, int NUM_SEG_t>
void fun_genLineSample_UbyU( // call by LbyL //call by UbyU
    int line,
    int e,
    int seg,
    T* m_p_data,              // output
    unsigned int* m_p_inside, // output
    para_Sample2<T>* p_para,  // Self-iterated data structure
    T* p_xdc_def_pos_4d,      // const array: up to 256(m_num_element) * 4 * sizeof(float)
    T* p_points_x,            // input
    T* p_points_z,            // input
    T* p_delay                // input
    ) {
    int num_sample = p_para->num_sample;
    int freq_sampling = p_para->freq_sampling;
    T inv_speed_of_sound = p_para->inverse_speed_of_sound;
    unsigned short rf_data_dim = p_para->p_rf_data_dim[line]; // getsDim();
    {
        T xdc_x = p_xdc_def_pos_4d[e * 4 + DIM_X];
        T xdc_z = p_xdc_def_pos_4d[e * 4 + DIM_Z];
        for (int n = 0; n < num_sample / NUM_SEG_t; n++) {
            T x = p_points_x[n] - xdc_x;
            T z = p_points_z[n] - xdc_z;
            T rec_delay = sqrt(x * x + z * z) * inv_speed_of_sound;
            T total_delay = rec_delay + p_delay[n];
            T t_sample = total_delay * freq_sampling + 1;
            m_p_inside[n] = (t_sample >= 1) && (t_sample < rf_data_dim);
            m_p_data[n] = t_sample * m_p_inside[n] + 1 - m_p_inside[n];
        }
    }
    p_para->selfUpdate();
}
///////////////
// call by MbyM
///////////////
template <class T>
void fun_runAll_Sample(int m_num_line,
                       int m_num_sample,
                       int m_num_element,
                       int m_freq_sampling,
                       T m_inv_speed_of_sound,
                       T* p_points[],
                       T* p_delay,
                       T* m_p_data) { // call by MbyM
    for (int line = 0; line < m_num_line; line++)
        fun_genLineSample<T>(m_num_sample, m_num_element, m_freq_sampling, m_inv_speed_of_sound, example_1_xdc_def_pos,
                             &(p_points[DIM_X][line * m_num_sample]), &(p_points[DIM_Z][line * m_num_sample]),
                             &(p_delay[m_num_sample * line]), &(m_p_data[m_num_element * m_num_sample * line]));
}

template <class T>
class us_op_sample : public us_model_base {
   public:
    T* m_p_data;

    us_op_sample() : us_model_base(5, "Sample") {
        m_type = MODEL_SAMPLE;
        genFullName();
    }

    us_op_sample(int id, const char* nm) : us_model_base(id, nm) {
        m_type = MODEL_SAMPLE;
        genFullName();
    }

    void init(us_device* p_dev) {
        us_model_base::copyPara(p_dev);
        m_p_data = (T*)malloc(m_num_line * m_num_element * m_num_sample * sizeof(T));
    }
    ///////////////
    // call by MbyM
    ///////////////
    void runAll_Sample(T* p_points[], T* p_delay) { // call by MbyM
        fun_runAll_Sample<float>(m_num_line, m_num_sample, m_num_element, m_freq_sampling, m_inv_speed_of_sound,
                                 p_points, p_delay, m_p_data);
    }

    void print_member(FILE* fp) {
        fprintf(fp, "%s m_num_line  = %d\n", m_name_full, m_num_line);
        fprintf(fp, "%s m_num_element   = %d\n", m_name_full, m_num_element);
        fprintf(fp, "%s m_num_sample   = %lx\n", m_name_full, m_num_sample);
    }

    void print_data(FILE* fp) { print_data(fp, 1); }

    void print_data(FILE* fp, int control) { printSample(fp, control); }

    void printSample(FILE* fp, int control) {
        assert(fp);
        fprintf(fp,
                "%s DATA FORMAT of All Sample: TxtPage:m_num_line(%d) by TxtLine:m_num_element(%d) by "
                "TxtCol:m_num_sample(%d) \n",
                m_name_full, m_num_line, m_num_element, m_num_sample);
        for (int line = 0; line < m_num_line; line++) {
            printSample_line(fp, line, control);
            fprintf(fp, "\n");
        }
    }

    void printSample_line(FILE* fp, int idx_line, int control) {
        assert(fp);
        if (control) fprintf(fp, "%s_line_%d:\n", m_name_full, idx_line);
        char print_format[128];
        for (int e = 0; e < m_num_element; e++) {
            for (int n = 0; n < m_num_sample; n++) {
                if (control == 1)
                    strcpy(print_format, "\t%.9f");
                else
                    strcpy(print_format, "%.9f\n");
                fprintf(fp, print_format, m_p_data[m_num_element * m_num_sample * idx_line + e * m_num_sample + n]);
            }
            if (control == 1) fprintf(fp, "\n");
        }
    }

    ~us_op_sample() {
        if (m_p_data) free(m_p_data);
    }

    char* save4aie(int format, const char* nm, int line, int element, int sample) {
        // char fname[DATA_SMP][300];
        const char* str_f[2] = {"col", "mtx"};
        sprintf(fname[DATA_SMP], "%s_L%d_E%d_S%d.smp.%s", nm, line, element, sample, str_f[format & PRINT_MTX]);
        printf("MODEL_TEST_SCANLINE: Saving smp data in file %s\n", fname[DATA_SMP]);
        FILE* fp = fopen(fname[DATA_SMP], "w");
        assert(line <= m_num_line);
        assert(sample <= m_num_sample);
        assert(fp);
        for (int l = 0; l < line; l++) {
            for (int e = 0; e < element; e++) {
                if ((format & PRINT_MTX) && (format & PRINT_IDX)) fprintf(fp, "line:%d:ele:%d\t", l, e);
                for (int s = 0; s < sample; s++) {
                    fprintf(fp, "%d", (int)this->m_p_data[l * m_num_sample * m_num_element + e * m_num_sample + s]);
                    if (!(format & PRINT_MTX))
                        fprintf(fp, "\n");
                    else
                        fprintf(fp, "\t");
                }
                if ((format & PRINT_MTX)) fprintf(fp, "\n");
            } // for e
        }     // for l
        fclose(fp);
        return fname[DATA_SMP];
    }
    char* save4aie(const char* nm) {
        save4aie(0, nm, m_num_line, m_num_element, m_num_sample);
        return fname[DATA_SMP];
    }
};
#endif