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
#ifndef __US_OP_IMAGEPOINT_HPP__
#define __US_OP_IMAGEPOINT_HPP__
#include "us_model_base.hpp"
#include "us_device.hpp"

///////////////
// call by MbyM
///////////////
template <class T>
void fun_UpdatingImagePoints_line(
    T dx, T dz, T xi, T zi, int m_num_sample, T* m_p_data_x, T* m_p_data_z) { // call by MbyM
    // with limited size of memory operation, possible to be applied on AIE
    for (int n = 0; n < m_num_sample; n++) {
        m_p_data_x[n] = xi + n * dx;
        m_p_data_z[n] = zi + n * dz;
    }
}

template <class T>
void fun_UpdatingImagePoints_line_1d(T start, T step, int start_n, int num, T* p_des) { // call by LbyL //call by UbyU
    T acc = start + start_n * step;
    p_des[0] = acc;
    for (int n = 1; n < num; n++) {
        acc += step;
        p_des[n] = acc;
    }
}

template <class T>
struct para_ImagePoint {
    int m_iter;
    T step;
    int start_n;
    int num;
    T* p_start;
    para_ImagePoint(T* p_str, T stp, int str_n, int n) {
        m_iter = 0;
        p_start = p_str;
        step = stp;
        start_n = str_n;
        num = n;
    }
    T getsStart() { return p_start[m_iter]; }
    void selfUpdate() { m_iter++; }
};

template <class T>
void fun_UpdatingImagePoints_line_1d_wrapper( // call by LbyL //call by UbyU
    para_ImagePoint<T>* p_para,               // Self-iterated data structure
    T* p_start_1d,                            // self-used constant array: start points location of 1D
    T* p_out                                  // output
    ) {
    T start = p_para->getsStart();
    T step = p_para->step;
    int start_n = p_para->start_n;
    int num = p_para->num;
    fun_UpdatingImagePoints_line_1d(start, step, start_n, num, p_out);
    p_para->selfUpdate();
}

template <class T, int NUM_LINE_t, int NUM_ELEMENT_t, int NUM_SAMPLE_t>
void fun_UpdatingImagePoints_EyE( // call by EbyE
    int line,
    int e,
    para_ImagePoint<T>* p_para, // para_img_t<T>* p_para, //Self-iterated data structure
    T* p_start_1d,              // self-used constant array: start points location of 1D
    T* p_out                    // output
    ) {
    T start = p_start_1d[line];
    T step = p_para->step;
    int start_n = p_para->start_n;
    int num = p_para->num;
    fun_UpdatingImagePoints_line_1d(start, step, 0, num, p_out);
    p_para->selfUpdate();
}

template <class T, int NUM_LINE_t, int NUM_ELEMENT_t, int NUM_SAMPLE_t, int NUM_SEG_t, int LEN_OUT_img_t>
void fun_UpdatingImagePoints_UbyU( // call by UbyU
    int line,
    int e,
    int seg,
    para_ImagePoint<T>* p_para, // para_img_t<T>* p_para, //Self-iterated data structure
    T* p_start_1d,              // self-used constant array: start points location of 1D
    T* p_out                    // output
    ) {
    T start = p_start_1d[line];
    T step = p_para->step;
    int base_i = LEN_OUT_img_t * seg;
    T v_base = start + step * base_i;
    for (int i = 0; i < LEN_OUT_img_t; i++) {
        *p_out++ = v_base;
        v_base = v_base + step;
    }
    // fun_UpdatingImagePoints_line_1d( start,  step,  start_n, num_seg, p_out);
    p_para->selfUpdate();
}

template <class T, int NUM_LINE_t, int NUM_ELEMENT_t, int NUM_SAMPLE_t, int NUM_SEG_t>
void fun_UpdatingImagePoints_UbyU( // call by LbyL //call by UbyU
    int line,
    int e,
    int seg,
    para_ImagePoint<T>* p_para, // para_img_t<T>* p_para, //Self-iterated data structure
    T* p_start_1d,              // self-used constant array: start points location of 1D
    T* p_out                    // output
    ) {
    if (e != 0) return;
    T start = p_start_1d[line];
    T step = p_para->step;
    int num_seg = p_para->num / NUM_SEG_t;
    int start_seg = seg * p_para->num / NUM_SEG_t;
    fun_UpdatingImagePoints_line_1d(start, step, start_seg, num_seg, p_out);
    p_para->selfUpdate();
}

///////////////
// call by MbyM
///////////////
template <class T>
void fun_runAll_ImagePoints(
    T dx, T dz, int m_num_line, int m_num_sample, T m_p_start[4][MAX_NUM_LINE], T* m_p_data[4]) { // call by MbyM
    for (int line = 0; line < m_num_line; line++) {
        T xi = m_p_start[DIM_X][line];
        T zi = m_p_start[DIM_Z][line];
        fun_UpdatingImagePoints_line(dx, dz, xi, zi, m_num_sample, &(m_p_data[DIM_X][line * m_num_sample]),
                                     &(m_p_data[DIM_Z][line * m_num_sample]));
    }
}

template <class T>
class us_op_imagepoint : public us_model_base {
   public:
    T* m_p_data[4]; // [m_num_line][m_num_sample]

    us_op_imagepoint() : us_model_base(1, "Imagepoints") {
        m_type = MODEL_IMAGEPOINT;
        genFullName();
    }

    us_op_imagepoint(int id, const char* nm) : us_model_base(id, nm) {
        m_type = MODEL_IMAGEPOINT;
        genFullName();
    }

    void freeMem() {
        for (int i = 0; i < 4; i++) {
            if (m_p_data[i]) free(m_p_data[i]);
        }
    }

    ~us_op_imagepoint() { freeMem(); }

    void init(us_device* p_dev) {
        copyPara((us_model_base*)p_dev);
        for (int i = 0; i < 4; i++) {
            // m_p_start[i] = (T*)malloc( m_num_line * sizeof(T) );
            m_p_data[i] = (T*)malloc(m_num_line * m_num_sample * sizeof(T));
            memset(m_p_data[i], 0, m_num_line * m_num_sample * sizeof(T));
        }
        m_isInit = true;
    }
    void print_member(FILE* fp) {
        fprintf(fp, "%s m_num_line    = %d\n", m_name_full, m_num_line);
        fprintf(fp, "%s m_num_sample    = %d\n", m_name_full, m_num_sample);
        fprintf(fp, "%s m_dim      = %d\n", m_name_full, m_dim);
        fprintf(fp, "%s m_p_start  = %lx\n", m_name_full, m_p_start);
        fprintf(fp, "%s m_p_data = %lx\n", m_name_full, m_p_data);
    }

    void print_data(FILE* fp, int control) {
        printStartPoints(fp, control);
        printImagePoints(fp, control);
    }

    void print_data(FILE* fp) { print_data(fp, 1); }

    void printStartPoints(FILE* fp, int control) {
        assert(fp);
        fprintf(fp, "%s DATA FORMAT OF STARTPOINT: TxtLine:DIM_X DIM_Z by TxtCol:m_num_line(%d) \n", m_name_full,
                m_num_line);
        for (int line = 0; line < m_num_line; line++) {
            if (control == ENUM_PRINT_MODE::TITLE_MATRIX) fprintf(fp, "%s start_line(%d):", m_name_full, line);
            fprintf(fp, "\t%.9f", DIM_X, m_p_start[DIM_X][line]);
            fprintf(fp, "\t%.9f", DIM_Z, m_p_start[DIM_Z][line]);
            fprintf(fp, "\n");
        }
    }

    void printImagePoints_line_dim(FILE* fp, int line, int dim, int control) {
        assert(fp);
        if (control == ENUM_PRINT_MODE::TITLE_MATRIX) fprintf(fp, "%s line_%d_dim_%d:", m_name_full, line, dim);
        for (int i = 0; i < m_num_sample; i++) {
            int k = line * m_num_sample + i;
            fprintf(fp, "\t%.9f", m_p_data[dim][k]);
        }
        fprintf(fp, "\n");
    }

    void printImagePoints_line(FILE* fp, int line, int control) {
        printImagePoints_line_dim(fp, line, DIM_X, control);
        printImagePoints_line_dim(fp, line, DIM_Z, control);
    }

    void printImagePoints(FILE* fp, int control) {
        fprintf(fp,
                "%s DATA FORMAT OF ImagePoints: TxtLine:m_num_sample(%d) by TxtCol:m_num_line* <DIM_X, DIM_Z>(%d ) \n",
                m_name_full, m_num_sample, m_num_line);
        for (int line = 0; line < m_num_line; line++) printImagePoints_line(fp, line, control);
    }

    void UpdatingImagePoints_line(int idx_line, T dx, T dz) {
        T xi = m_p_start[DIM_X][idx_line];
        T zi = m_p_start[DIM_Z][idx_line];
        for (int n = 0; n < m_num_sample; n++) {
            m_p_data[DIM_X][idx_line * m_num_sample + n] = xi + n * dx;
            m_p_data[DIM_Z][idx_line * m_num_sample + n] = zi + n * dz;
            // core_GettingImagePoints(m_p_data[DIM_X] + idx_line* m_num_sample, xi, dx, m_num_sample);
            // core_GettingImagePoints(m_p_data[DIM_Z] + idx_line* m_num_sample, zi, dz, m_num_sample);
        }
    }
    ///////////////
    // call by MbyM
    ///////////////
    void runAll_ImagePoints(T dx, T dz) { // call by MbyM
        fun_runAll_ImagePoints<T>(dx, dz, m_num_line, m_num_sample, m_p_start, m_p_data);
    }

    static void core_GettingImagePoints(T* p_data, T start, T delta, int num) {
        assert(p_data);
        assert(num > 0 && num <= MAX_NUM_SAMPLE);
        unsigned short p_idx[MAX_NUM_SAMPLE];
        // initializing index as an existing vector
        for (int i = 0; i < MAX_NUM_SAMPLE; i++) p_idx[i] = i;
        for (int n = 0; n < num; n++)
            // p_data[n] = start + n * delta;        //not sure this can be vectorized easlly
            p_data[n] = start + p_idx[n] * delta; // be sure this can be vectorized easlly
    }

    char* save4aie(int format, const char* nm, int line, int sample, int dim) {
        // char fname[DATA_IMG][300];
        // sprintf(fname[DATA_IMG], "%s_format%d_line%dof%d_smp_%dof%d_dim%d.img", nm, format, line, m_num_line, sample,
        // m_num_sample, dim);
        // sprintf(fname[DATA_IMG], "%s_format%d_L%d_S%d_Dim%d.img", nm, format, line, sample, dim);
        const char* str_f[2] = {"col", "mtx"};
        sprintf(fname[DATA_IMG0 + dim], "%s_L%d_S%d_Dim%d.img.%s", nm, line, sample, dim, str_f[format & PRINT_MTX]);
        printf("MODEL_TEST_SCANLINE: Saving img data in file %s\n", fname[DATA_IMG0 + dim]);
        FILE* fp = fopen(fname[DATA_IMG0 + dim], "w");
        assert(line <= m_num_line);
        assert(sample <= m_num_sample);
        assert(dim < 4);
        assert(fp);
        for (int i = 0; i < line; i++) {
            if ((format & PRINT_MTX) && (format & PRINT_IDX)) fprintf(fp, "line_%d:\t", i);
            for (int s = 0; s < sample; s++) {
                fprintf(fp, "%e", this->m_p_data[dim][i * m_num_sample + s]);
                if (!(format & PRINT_MTX))
                    fprintf(fp, "\n");
                else
                    fprintf(fp, "\t");
            }
            if ((format & PRINT_MTX)) fprintf(fp, "\n");
        }
        fclose(fp);
        return fname[DATA_IMG0 + dim];
    }

    char* save4aie(int format, const char* nm, int line, int sample) {
        save4aie(0, nm, line, sample, DIM_X);
        save4aie(0, nm, line, sample, DIM_Z);
        return fname[DATA_IMG0 + DIM_Z];
    }

    char* save4aie(const char* nm) {
        save4aie(0, nm, m_num_line, m_num_sample);
        return fname[DATA_IMG0 + DIM_Z];
    }
};

;
#endif