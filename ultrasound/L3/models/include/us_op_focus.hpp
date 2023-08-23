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
#ifndef __US_OP_FOCUS_HPP__
#define __US_OP_FOCUS_HPP__
#include "us_model_base.hpp"

template <class T>
struct para_Focus {
    int m_iter;
    int m_num_element;
    para_Focus(int num_e) {
        m_iter = 0;
        m_num_element = num_e;
    }
    void selfUpdate() { m_iter++; }
};

template <class T>
void fun_genLineFocus(     // call by LbyL //call by UbyU
    T* p_foc_line,         // output: : up to 256(m_num_element)
    para_Focus<T>* p_para, // self-iterated structure
    T* p_xdc_def_pos_4d    // const array: up to 256(m_num_element) * 4 * sizeof(float)
    ) {
    for (int e = 0; e < p_para->m_num_element; e++) {
        T x = p_xdc_def_pos_4d[e * 4 + DIM_X];
        T y = p_xdc_def_pos_4d[e * 4 + DIM_Y];
        p_foc_line[e] = sqrt(x * x + y * y);
    }
    p_para->selfUpdate();
}

template <class T, int NUM_LINE_t, int NUM_ELEMENT_t, int NUM_SAMPLE_t>
void fun_genFocus_EbyE( // call by EbyE
    int e,
    T* p_foc_line,      // output: : up to 256(m_num_element)
    T* p_xdc_def_pos_4d // const array: up to 256(m_num_element) * 4 * sizeof(float)
    ) {
    T x = p_xdc_def_pos_4d[e * 4 + DIM_X];
    T y = p_xdc_def_pos_4d[e * 4 + DIM_Y];
    p_foc_line[e] = sqrt(x * x + y * y);
}

template <class T, int NUM_LINE_t, int NUM_ELEMENT_t, int NUM_SAMPLE_t>
void fun_genFocus_UbyU( // call by EbyE //call by UbyU
    int e,
    T* p_foc_line,      // output: : up to 256(m_num_element)
    T* p_xdc_def_pos_4d // const array: up to 256(m_num_element) * 4 * sizeof(float)
    ) {
    fun_genFocus_EbyE<T, NUM_LINE_t, NUM_ELEMENT_t, NUM_SAMPLE_t>(e, p_foc_line, p_xdc_def_pos_4d);
}

template <class T>
class us_op_focus : public us_model_base {
   public:
    T* m_p_data; // from float focusing[NUM_LINE][NUM_ELEMENT] in buffer_focusing_out.cpp

    us_op_focus() : us_model_base(2, "Focusing") {
        m_type = MODEL_FOCUS;
        genFullName();
    }
    us_op_focus(int id, const char* nm) : us_model_base(id, nm) {
        m_type = MODEL_FOCUS;
        genFullName();
    }

    void init(us_device* p_dev) {
        us_model_base::copyPara(p_dev);
        m_p_data = (T*)malloc(m_num_element * m_num_line * sizeof(T));
        memset(m_p_data, 0, m_num_line * m_num_element * sizeof(T));
    }
    ///////////////
    // call by MbyM
    ///////////////
    void genLineFocus(T* p_foc_line) { // call by MbyM
        for (int e = 0; e < m_num_element; e++) {
            T x = example_1_xdc_def_pos[e][DIM_X];
            T y = example_1_xdc_def_pos[e][DIM_Y];
            p_foc_line[e] = sqrt(x * x + y * y);
        }
    }
    ///////////////
    // call by MbyM
    ///////////////
    void runAll_Focus() { // call by MbyM
        for (int line = 0; line < m_num_line; line++) genLineFocus(m_p_data + line * m_num_element);
    }

    void print_member(FILE* fp) {
        fprintf(fp, "%s m_num_line  = %d\n", m_name_full, m_num_line);
        fprintf(fp, "%s m_num_element   = %d\n", m_name_full, m_num_element);
        fprintf(fp, "%s m_p_data   = %lx\n", m_name_full, m_p_data);
    }

    void print_data(FILE* fp, int control) {
        fprintf(fp, "%s DATA FORMAT of All Focus: TxtLine:m_num_element(%d) by TxtCol:m_num_line(%d) \n", m_name_full,
                m_num_element, m_num_line);
        for (int line = 0; line < m_num_line; line++) {
            if (control) fprintf(fp, "%s_line_%d", m_name_full, line);
            for (int e = 0; e < m_num_element; e++) {
                fprintf(fp, "\t%.9f", m_p_data[line * m_num_element + e]);
            }
            fprintf(fp, "\n");
        }
    }

    void print_data(FILE* fp) { print_data(fp, 1); }

    ~us_op_focus() {
        if (m_p_data) free(m_p_data);
    }

    void save4aie(int format, const char* nm, int line, int element) {
        // char fname[300];
        // sprintf(fname, "%s_format%d_line%dof%d_element_%dof%d.foc", nm, format, line, m_num_line, element,
        // m_num_element);
        // sprintf(fname, "%s_format%d_L%d_E%d.foc", nm, format, line, element);
        const char* str_f[2] = {"col", "mtx"};
        sprintf(fname[DATA_FOC], "%s_L%d_E%d.foc.%s", nm, line, element, str_f[format & PRINT_MTX]);
        printf("MODEL_TEST_SCANLINE: Saving focusing data in file %s\n", fname[DATA_FOC]);
        FILE* fp = fopen(fname[DATA_FOC], "w");
        assert(line <= m_num_line);
        assert(element <= m_num_element);
        assert(fp);
        for (int l = 0; l < line; l++) {
            if ((format & PRINT_MTX) && (format & PRINT_IDX)) fprintf(fp, "line_%d:\t", l);
            for (int e = 0; e < element; e++) {
                fprintf(fp, "%e", this->m_p_data[l * m_num_element + e]);
                if (!(format & PRINT_MTX))
                    fprintf(fp, "\n");
                else
                    fprintf(fp, "\t");
            }
            if ((format & PRINT_MTX)) fprintf(fp, "\n");
        }
        fclose(fp);
    }
    void save4aie(const char* nm) { save4aie(0, nm, m_num_line, m_num_element); }
};
#endif