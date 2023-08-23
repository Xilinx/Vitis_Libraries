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
#ifndef __US_OP_APODIZATION_HPP__
#define __US_OP_APODIZATION_HPP__
#include "us_model_base.hpp"

#include <kernel_focusing.hpp>
#include <aie_api/aie.hpp>
using namespace aie;
/*#include "aie_api/utils.hpp"
#include "aie_api/aie_adf.hpp"
#include <aie_api/operators.hpp>
*/
///////////////
// call by MbyM
///////////////
template <class T>
void fun_genLineApodization(int m_num_sample,
                            int m_element,
                            T m_f_number,
                            T m_tileVApo[],
                            T m_ref_pos[],
                            T* p_points_x,
                            T* p_points_z,
                            T* p_focal,
                            T* m_p_data) { // call by MbyM
    for (int e = 0; e < m_element; e++) {
        for (int n = 0; n < m_num_sample; n++) {
            T diff_x = p_points_x[n] - m_ref_pos[DIM_X];
            T diff_z = p_points_z[n] - m_ref_pos[DIM_Z];
            T D = std::abs(diff_x * m_tileVApo[DIM_X] + diff_z * m_tileVApo[DIM_Z]);
            bool equalZero = D == 0;
            D = D + equalZero * 1e-16;
            T x_ele = m_f_number * p_focal[e] / D;
            bool in_range = std::abs(x_ele) <= 1;
            T cos_res = std::cos(M_PI * (1 + x_ele));
            m_p_data[e * m_num_sample + n] = 0.5 * (1.0 - cos_res) * in_range;
        }
    }
}

template <class T>
void fun_genLineApodization_vec_part1( // call by LbyL //call by UbyU
    T* p_invD,
    int m_num_sample,
    T* m_tileVApo,
    T* m_ref_pos,
    T* p_points_x,
    T* p_points_z) {
    for (int n = 0; n < m_num_sample; n++) {
        // m_p_points_x[n] = xi + n * dx;
        // m_p_points_z[n] = zi + n * dz;
        T diff_x_msc = (p_points_x[n] - m_ref_pos[DIM_X]) * m_tileVApo[DIM_X];
        T diff_z_msc = (p_points_z[n] - m_ref_pos[DIM_Z]) * m_tileVApo[DIM_Z];
        T D = std::abs(diff_x_msc * 1.0f + diff_z_msc * 1.0f);
        bool equalZero = D == 0;
        D = D + equalZero * 1e-16;
        // p_invD[n] = 1/D; //aie::inv(D) // 210578f184c9966e78ed30cd4cb2ffa8
        p_invD[n] = D; // 0c72c33e744d21bc6e12f3927643daeb
    }
#ifdef WRITE_APODI
    writeFile<T>(p_points_x, m_num_sample, "apodi_preprocess/p_points_x.txt");
    writeFile<T>(p_points_z, m_num_sample, "apodi_preprocess/p_points_z.txt");
    writeFile<T>(m_tileVApo, 4, "apodi_preprocess/m_tileVApo.txt");
    writeFile<T>(m_ref_pos, 4, "apodi_preprocess/m_ref_pos.txt");
    writeFile<T>(p_invD, m_num_sample, "apodi_preprocess/p_invD.txt");
#endif
}

template <class T>
void fun_genLineApodization_vec_part2( // call by LbyL //call by UbyU
    T* m_p_data,
    int m_element,
    int m_num_sample,
    T m_f_number,
    T* p_invD,
    T* p_focal) {
    for (int e = 0; e < m_element; e++) {
        for (int n = 0; n < m_num_sample; n++) {
            T x_ele = m_f_number * p_focal[e] / p_invD[n]; //*p_invD[n]
            bool in_range = std::abs(x_ele) <= 1;
            T cos_res = std::cos(M_PI * (1 + x_ele)); // aie::cos
            m_p_data[e * m_num_sample + n] = 0.5 * (1.0 - cos_res) * in_range;
        }
#ifdef WRITE_APODI_MAIN
        for (int seg = 0; seg < 4; seg++) writeFile<T>(p_focal, m_element, "apodi_main/p_focal.txt");
        writeFile<T>(p_invD, m_num_sample, "apodi_main/p_invD.txt");
#endif
    }
#ifdef WRITE_APODI_MAIN
    writeFile<T>(m_p_data, m_element * m_num_sample, "apodi_main/m_p_data.txt");
#endif
}

template <class T>
struct para_Apodization {
    int iter;
    int num_element;
    int num_sample;
    T f_number;

    para_Apodization(int n_ele, int n_dep, T f_num) { // const unsigned short *rf_dat_dim
        iter = 0;
        num_element = n_ele;
        num_sample = n_dep;
        f_number = f_num;
    }
    void selfUpdate() { iter++; }
};

template <class T>
void fun_genLineApodization_warpper( // call by LbyL //call by UbyU
    T* p_apodi_line,                 // output  : up to 2k(num_sample), total 2k*128(num_sample * num_ele)
    para_Apodization<T>* p_para,     // para self-iterated structure
    T* p_const_tileVApo,             // const : up to 4(num_dim) * sizeof(float)
    T* p_const_ref_pos,              // const : up to 4(num_dim) * sizeof(float)
    T* p_points_x,                   // input
    T* p_points_z,
    T* p_focal) {
    int line = p_para->iter;
    int n_ele = p_para->num_element;
    int n_dep = p_para->num_sample;
    T f_num = p_para->f_number;
    T p_invD[n_dep];
    fun_genLineApodization_vec_part1<T>(p_invD, n_dep, p_const_tileVApo, p_const_ref_pos, p_points_x, p_points_z);
    fun_genLineApodization_vec_part2<T>(p_apodi_line, n_ele, n_dep, f_num, p_invD, p_focal);
    p_para->selfUpdate();
}

template <class T, int NUM_LINE_t, int NUM_ELEMENT_t, int NUM_SAMPLE_t>
void fun_genLineApodization_EbyE( // call by LbyL //call by UbyU
    int line,
    int e,
    T* m_p_data,                 // output  : up to 2k(num_sample), total 2k*128(num_sample * num_ele)
    para_Apodization<T>* p_para, // para self-iterated structure
    T* m_tileVApo,               // const : up to 4(num_dim) * sizeof(float)
    T* m_ref_pos,                // const : up to 4(num_dim) * sizeof(float)
    T* p_points_x,               // input
    T* p_points_z,
    T* p_focal) {
    int m_num_sample = p_para->num_sample;
    T m_f_number = p_para->f_number;
    T p_invD[m_num_sample];

    for (int n = 0; n < m_num_sample; n++) {
        T diff_x_msc = (p_points_x[n] - m_ref_pos[DIM_X]) * m_tileVApo[DIM_X];
        T diff_z_msc = (p_points_z[n] - m_ref_pos[DIM_Z]) * m_tileVApo[DIM_Z];
        T D = std::abs(diff_x_msc * 1.0f + diff_z_msc * 1.0f);
        bool equalZero = D == 0;
        D = D + equalZero * 1e-16;
        p_invD[n] = D; // 0c72c33e744d21bc6e12f3927643daeb
    }

    {
        for (int n = 0; n < m_num_sample; n++) {
            T x_ele = m_f_number * p_focal[e] / p_invD[n]; //*p_invD[n]
            bool in_range = std::abs(x_ele) <= 1;
            T cos_res = std::cos(M_PI * (1 + x_ele)); // aie::cos
            m_p_data[n] = 0.5 * (1.0 - cos_res) * in_range;
        }
    }
    p_para->selfUpdate();
}
template <class T, int NUM_LINE_t, int NUM_ELEMENT_t, int NUM_SAMPLE_t, int NUM_SEG_t>
void fun_genLineApodization_UbyU( // call by UbyU
    int line,
    int e,
    int seg,
    T* m_p_data,                 // output  : up to 2k(num_sample), total 2k*128(num_sample * num_ele)
    para_Apodization<T>* p_para, // para self-iterated structure
    T* m_tileVApo,               // const : up to 4(num_dim) * sizeof(float)
    T* m_ref_pos,                // const : up to 4(num_dim) * sizeof(float)
    T* p_points_x,               // input
    T* p_points_z,
    T* p_focal) {
    int m_num_sample = p_para->num_sample;
    T m_f_number = p_para->f_number;
    T p_invD[m_num_sample / NUM_SEG_t];

    for (int n = 0; n < m_num_sample / NUM_SEG_t; n++) {
        T diff_x_msc = (p_points_x[n] - m_ref_pos[DIM_X]) * m_tileVApo[DIM_X];
        T diff_z_msc = (p_points_z[n] - m_ref_pos[DIM_Z]) * m_tileVApo[DIM_Z];
        T D = std::abs(diff_x_msc * 1.0f + diff_z_msc * 1.0f);
        bool equalZero = D == 0;
        D = D + equalZero * 1e-16;
        p_invD[n] = D;
    }

    {
        for (int n = 0; n < m_num_sample / NUM_SEG_t; n++) {
            T x_ele = m_f_number * p_focal[e] / p_invD[n]; //*p_invD[n]
            bool in_range = std::abs(x_ele) <= 1;
            T cos_res = std::cos(M_PI * (1 + x_ele)); // aie::cos
            // float cos_in = M_PI * (x_ele - 1);
            // float cos_res = aie::cos(cos_in); //Note, aie::cos input must in [-M_PI, M_PI]
            m_p_data[n] = 0.5 * (1.0 - cos_res) * in_range;
        }
    }
    p_para->selfUpdate();
}
//---------------------------done----------------------------//
///////////////
// call by MbyM
///////////////
template <class T>
void fun_runAll_Apodization(int m_num_line,
                            int m_num_sample,
                            int m_num_element,
                            T m_f_number,
                            T m_tileVApo[],
                            T m_ref_pos[],
                            T* p_points[],
                            T* p_focal,
                            T* m_p_data) { // call by MbyM
    for (int line = 0; line < m_num_line; line++)
        fun_genLineApodization(m_num_sample, m_num_element, m_f_number, m_tileVApo, m_ref_pos,
                               &(p_points[DIM_X][line * m_num_sample]), &(p_points[DIM_Z][line * m_num_sample]),
                               &(p_focal[line * m_num_element]), &(m_p_data[m_num_element * m_num_sample * line]));
}

template <class T>
class us_op_apodization : public us_model_base {
   public:
    T* m_p_data; // apodization output m_num_line * m_num_element * m_num_sample

    us_op_apodization() : us_model_base(4, "Apodization") {
        m_type = MODEL_APODIZATION;
        genFullName();
    }

    us_op_apodization(int id, const char* nm) : us_model_base(id, nm) {
        m_type = MODEL_APODIZATION;
        genFullName();
    }

    void init(us_device* p_dev) {
        us_model_base::copyPara(p_dev);
        m_p_data = (T*)malloc(m_num_line * m_num_element * m_num_sample * sizeof(T));
    }
    ///////////////
    // call by MbyM
    ///////////////
    void runAll_Apodization(T* p_points[], T* p_focal) { // call by MbyM
        fun_runAll_Apodization<float>(m_num_line, m_num_sample, m_num_element, m_f_number, m_tileVApo, m_ref_pos,
                                      p_points, p_focal, m_p_data);
    }

    void print_member(FILE* fp) {
        fprintf(fp, "%s m_num_line  = %d\n", m_name_full, m_num_line);
        fprintf(fp, "%s m_num_element   = %d\n", m_name_full, m_num_element);
        fprintf(fp, "%s m_num_sample   = %lx\n", m_name_full, m_num_sample);
    }

    void print_data(FILE* fp) { print_data(fp, 1); }

    void print_data(FILE* fp, int control) {
        fprintf(fp,
                "%s DATA FORMAT of All Apodization: TxtPage:m_num_line(%d) by TxtLine:m_num_element(%d) by "
                "TxtCol:m_num_sample(%d) \n",
                m_name_full, m_num_line, m_num_element, m_num_sample);
        for (int line = 0; line < m_num_line; line++) {
            printApodization_line(fp, line, control);
            fprintf(fp, "\n");
        }
    }

    void printApodization_line(FILE* fp, int idx_line, int control) {
        assert(fp);
        if (control) fprintf(fp, "%s_line_%d\n", m_name_full, idx_line);
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

    ~us_op_apodization() {
        if (m_p_data) free(m_p_data);
    }

    char* save4aie(int format, const char* nm, int line, int element, int sample) {
        // sprintf(fname[DATA_APO], "%s_format%d_line%dof%d_ele%dof%d_smp_%dof%d.apo", nm, format, line, m_num_line,
        // element, m_num_element, sample, m_num_sample);
        // sprintf(fname[DATA_APO], "%s_format%d_L%d_E%d_S%d.apo", nm, format, line,  element, sample);
        const char* str_f[2] = {"col", "mtx"};
        sprintf(fname[DATA_APO], "%s_L%d_E%d_S%d.apo.%s", nm, line, element, sample, str_f[format & PRINT_MTX]);
        printf("MODEL_TEST_SCANLINE: Saving apo data in file %s\n", fname[DATA_APO]);
        FILE* fp = fopen(fname[DATA_APO], "w");
        assert(line <= m_num_line);
        assert(sample <= m_num_sample);
        assert(fp);
        for (int l = 0; l < line; l++) {
            for (int e = 0; e < element; e++) {
                if ((format & PRINT_MTX) && (format & PRINT_IDX)) fprintf(fp, "line:%d:ele:%d\t", l, e);
                for (int s = 0; s < sample; s++) {
                    fprintf(fp, "%.9f", this->m_p_data[l * m_num_sample * m_num_element + e * m_num_sample + s]);
                    if (!(format & PRINT_MTX))
                        fprintf(fp, "\n");
                    else
                        fprintf(fp, "\t");
                }
                if ((format & PRINT_MTX)) fprintf(fp, "\n");
            } // for e
        }     // for l
        fclose(fp);
        return fname[DATA_APO];
    }
    char* save4aie(const char* nm) {
        save4aie(0, nm, m_num_line, m_num_element, m_num_sample);
        return fname[DATA_APO];
    }
};
#endif