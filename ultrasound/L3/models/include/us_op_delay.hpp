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
#ifndef __US_OP_DELAY_HPP__
#define __US_OP_DELAY_HPP__
#include "us_model_base.hpp"

template <class T>
void fun_UpdatingDelay_line(int m_num_sample, // call by LbyL  //call by UbyU
                            T* tx_ref,
                            T* tileVApo,
                            T* focal_point,
                            T tx_delay_distance,
                            T tx_delay_distance_,
                            T inv_speed_of_sound,
                            T t_start,
                            T* p_points_x,
                            T* p_points_z,
                            T* m_p_data) {
    for (int n = 0; n < m_num_sample; n++) { // for sample
        T acc_sample1 = 0;
        T acc_sample2 = 0;

        // DIM_X
        T diff1 = p_points_x[n] - tx_ref[DIM_X];
        acc_sample1 = acc_sample1 + diff1 * tileVApo[DIM_X];
        T diff2 = p_points_x[n] - focal_point[DIM_X];
        acc_sample2 = acc_sample2 + diff2 * diff2;
        // DIM_Z
        diff1 = p_points_z[n] - tx_ref[DIM_Z];
        acc_sample1 = acc_sample1 + diff1 * tileVApo[DIM_Z];
        diff2 = p_points_z[n] - focal_point[DIM_Z];
        acc_sample2 = acc_sample2 + diff2 * diff2;

        acc_sample1 = std::abs(acc_sample1) - tx_delay_distance;
        int sign = acc_sample1 >= 0 ? 1 : -1;

        acc_sample2 = sign * std::sqrt(acc_sample2) + tx_delay_distance_;
        m_p_data[n] = acc_sample2 * inv_speed_of_sound - t_start;
    }
}

template <class T>
struct para_Delay {
    int m_iter;
    T* p_start;
    T* tx_ref_point;
    T* tileVApo;
    T* focal_point;
    T tx_delay_distance;
    T tx_delay_distance_;
    T inverse_speed_of_sound;
    int num;
    para_Delay(int n,
               T* p_str,
               T ref_point[],
               T tileV[],
               T foc_point[],
               T delay_distance,
               T delay_distance_,
               T inv_speed_of_sound) {
        m_iter = 0;
        p_start = p_str;
        tx_ref_point = ref_point;
        tileVApo = tileV;
        focal_point = foc_point;
        tx_delay_distance = delay_distance;
        tx_delay_distance_ = delay_distance_;
        inverse_speed_of_sound = inv_speed_of_sound;
        num = n;
    }
    T getsStart() { return p_start[m_iter]; }
    void selfUpdate() { m_iter++; }
};

template <class T>
void fun_UpdatingDelay_line( // call by LbyL //call by UbyU
    T* m_p_data,             // output
    para_Delay<T>* p_para,   // self-iterated structure
    T* t_start,              // self-used constant array: start time of 1D
    T* p_points_x,
    T* p_points_z) {
    int num_sample = p_para->num;
    T* tx_ref = p_para->tx_ref_point;
    T* tileVApo = p_para->tileVApo;
    T* focal_point = p_para->focal_point;
    T tx_delay_distance = p_para->tx_delay_distance;
    T tx_delay_distance_ = p_para->tx_delay_distance_;
    T inv_speed_of_sound = p_para->inverse_speed_of_sound;
    T t_start_line = p_para->getsStart();
    fun_UpdatingDelay_line<T>(num_sample, tx_ref, tileVApo, focal_point, tx_delay_distance, tx_delay_distance_,
                              inv_speed_of_sound, t_start_line, p_points_x, p_points_z, m_p_data);
    p_para->selfUpdate();
}

template <class T, int NUM_LINE_t, int NUM_ELEMENT_t, int NUM_SAMPLE_t>
void fun_UpdatingDelay_EbyE( // call by EbyE
    int line,
    int e,
    T* m_p_data,           // output
    para_Delay<T>* p_para, // self-iterated structure
    T* t_start,            // self-used constant array: start time of 1D
    T* p_points_x,
    T* p_points_z) {
    // if(e!=0)
    //    return;
    int num_sample = p_para->num;
    T* tx_ref = p_para->tx_ref_point;
    T* tileVApo = p_para->tileVApo;
    T* focal_point = p_para->focal_point;
    T tx_delay_distance = p_para->tx_delay_distance;
    T tx_delay_distance_ = p_para->tx_delay_distance_;
    T inv_speed_of_sound = p_para->inverse_speed_of_sound;
    T t_start_line = p_para->p_start[line];
    fun_UpdatingDelay_line<T>(num_sample, tx_ref, tileVApo, focal_point, tx_delay_distance, tx_delay_distance_,
                              inv_speed_of_sound, t_start_line, p_points_x, p_points_z, m_p_data);
    p_para->selfUpdate();
}

template <class T, int NUM_LINE_t, int NUM_ELEMENT_t, int NUM_SAMPLE_t, int NUM_SEG_t>
void fun_UpdatingDelay_UbyU( // call by UbyU
    int line,
    int e,
    int seg,
    T* m_p_data,           // output
    para_Delay<T>* p_para, // self-iterated structure
    T* t_start,            // self-used constant array: start time of 1D
    T* p_points_x,
    T* p_points_z) {
    int num_sample = p_para->num / NUM_SEG_t;
    T* tx_ref = p_para->tx_ref_point;
    T* tileVApo = p_para->tileVApo;
    T* focal_point = p_para->focal_point;
    T tx_delay_distance = p_para->tx_delay_distance;
    T tx_delay_distance_ = p_para->tx_delay_distance_;
    T inv_speed_of_sound = p_para->inverse_speed_of_sound;
    T t_start_line = p_para->p_start[line];
    fun_UpdatingDelay_line<T>(num_sample, tx_ref, tileVApo, focal_point, tx_delay_distance, tx_delay_distance_,
                              inv_speed_of_sound, t_start_line, p_points_x, p_points_z, m_p_data);
    p_para->selfUpdate();
}

template <class T>
class us_op_delay : public us_model_base {
   public:
    T* m_p_data; // m_num_line * m_num_sample

    us_op_delay() : us_model_base(3, "Delay") {
        m_type = MODEL_DELAY;
        genFullName();
    }

    us_op_delay(int id, const char* nm) : us_model_base(id, nm) {
        m_type = MODEL_DELAY;
        genFullName();
    }

    void freeMem() {
        if (m_p_data) free(m_p_data);
    }

    ~us_op_delay() { freeMem(); }

    void init_focalPoints() {
        for (int w = 0; w < m_num_line; w++) {
            for (int dim = 0; dim < m_dim; dim++) {
                m_p_focal[dim][w] = m_tx_def_focal_point[dim];
            }
            m_tx_def_delay_distance[w] = tx_def_delay_distance;
            m_tx_def_delay_distance_[w] = tx_def_delay_distance_;
        }
    }

    void init(us_device* p_dev) {
        us_model_base::copyPara(p_dev);
        m_p_data = (T*)malloc(m_num_line * m_num_sample * sizeof(T));
        init_focalPoints();
        m_isInit = true;
    }

    void print_member(FILE* fp) {
        fprintf(fp, "%s m_num_line    = %d\n", m_name_full, m_num_line);
        fprintf(fp, "%s m_num_sample    = %d\n", m_name_full, m_num_sample);
        fprintf(fp, "%s m_dim      = %d\n", m_name_full, m_dim);
        fprintf(fp, "%s m_tx_def_delay_distance  = %lx\n", m_name_full, m_tx_def_delay_distance);
        fprintf(fp, "%s m_tx_def_delay_distance_ = %lx\n", m_name_full, m_tx_def_delay_distance_);
        fprintf(fp, "%s m_p_focal  = %lx\n", m_name_full, m_p_focal);
        fprintf(fp, "%s m_p_data = %lx\n", m_name_full, m_p_data);
    }

    void print_data(FILE* fp, int control) {
        printFocalPoints(fp, control);
        printDelay(fp, control);
    }

    void print_data(FILE* fp) { print_data(fp, 1); }

    void printFocalPoints(FILE* fp, int control) {
        assert(fp);
        fprintf(fp, "%s DATA FORMAT OF FOCALPOINT: TxtLine:DIM_X DIM_Z by TxtCol:m_num_line(%d) \n", m_name_full,
                m_num_line);
        for (int line = 0; line < m_num_line; line++) {
            if (control) fprintf(fp, "%s start_line(%d):", m_name_full, line);
            fprintf(fp, "\t%.9f", DIM_X, m_p_focal[DIM_X][line]);
            fprintf(fp, "\t%.9f", DIM_Z, m_p_focal[DIM_Z][line]);
            fprintf(fp, "\n");
        }
    }

    void print_line(FILE* fp, int line, int control) {
        assert(fp);
        if (control) fprintf(fp, "%s line_%d:\n", m_name_full, line);
        char print_format[128];
        if (control == 1)
            strcpy(print_format, "\t%.9f");
        else
            strcpy(print_format, "%.9f\n");
        for (int i = 0; i < m_num_sample; i++) { // for sample
            int k = line * m_num_sample + i;
            fprintf(fp, print_format, m_p_data[k]);
        }
        if (control == 1) fprintf(fp, "\n");
    }

    void printDelay(FILE* fp, int control) {
        assert(fp);
        fprintf(fp,
                "%s DATA FORMAT OF ImagePoints: TxtLine:m_num_sample(%d) by TxtCol:m_num_line* <DIM_X, DIM_Z>(%d ) \n",
                m_name_full, m_num_sample, m_num_line);
        for (int line = 0; line < m_num_line; line++) { // for scanline
            print_line(fp, line, control);
        }
    }

    ///////////////
    // call by MbyM
    ///////////////
    void UpdatingDelay_line(T* p_points[], int idx_line) { // call by MbyM
        T diff1;
        T diff2;
        T acc_sample1;
        T acc_sample2;
        int sign;
        for (int n = 0; n < m_num_sample; n++) { // for sample
            acc_sample1 = 0;
            acc_sample2 = 0;
            for (int d = 0; d < m_dim; d++) {
                diff1 = p_points[d][idx_line * m_num_sample + n] - m_tx_def_ref_point[d];
                acc_sample1 += diff1 * m_tileVApo[d];
                diff2 = p_points[d][idx_line * m_num_sample + n] - m_p_focal[d][idx_line];
                acc_sample2 += diff2 * diff2;
            }
            acc_sample1 = std::abs(acc_sample1) - m_tx_def_delay_distance[idx_line];
            sign = acc_sample1 >= 0 ? 1 : -1;

            acc_sample2 = sign * std::sqrt(acc_sample2) + m_tx_def_delay_distance_[idx_line];
            m_p_data[idx_line * m_num_sample + n] = acc_sample2 * m_inv_speed_of_sound - m_t_start[idx_line];
        }
    }

    void UpdatingDelay_line(int idx_line, T* p_points_x, T* p_points_z) {
        for (int n = 0; n < m_num_sample; n++) { // for sample
            T acc_sample1 = 0;
            T acc_sample2 = 0;

            // DIM_X
            T diff1 = p_points_x[n] - m_tx_def_ref_point[DIM_X];
            acc_sample1 = acc_sample1 + diff1 * m_tileVApo[DIM_X];
            T diff2 = p_points_x[n] - m_p_focal[DIM_X][idx_line];
            acc_sample2 = acc_sample2 + diff2 * diff2;
            // DIM_Z
            diff1 = p_points_z[n] - m_tx_def_ref_point[DIM_Z];
            acc_sample1 = acc_sample1 + diff1 * m_tileVApo[DIM_Z];
            diff2 = p_points_z[n] - m_p_focal[DIM_Z][idx_line];
            acc_sample2 = acc_sample2 + diff2 * diff2;

            acc_sample1 = std::abs(acc_sample1) - m_tx_def_delay_distance[idx_line];
            int sign = acc_sample1 >= 0 ? 1 : -1;

            acc_sample2 = sign * std::sqrt(acc_sample2) + m_tx_def_delay_distance_[idx_line];
            m_p_data[n] = acc_sample2 * m_tx_def_delay_distance_[idx_line] - m_t_start[idx_line];
        }
    }
    ///////////////
    // call by MbyM
    ///////////////
    void UpdatingDelay(T* p_points[]) {                 // call by MbyM
        for (int line = 0; line < m_num_line; line++) { // for scanline
            UpdatingDelay_line(p_points, line);
            // UpdatingDelay_line(line, p_points[DIM_X], p_points[DIM_Z]);
        }
    }
    void UpdatingDelay(T* p_points[], T* p_foc) {       // call by MbyM
        for (int line = 0; line < m_num_line; line++) { // for scanline
            UpdatingDelay_line(line, p_points[DIM_X], p_points[DIM_Z]);
        }
    }
    ///////////////
    // call by MbyM
    ///////////////
    void runAll_Delay(T* p_points[]) { // call by MbyM
        UpdatingDelay(p_points);
    }

    char* save4aie(int format, const char* nm, int line, int sample) {
        // char fname[DATA_DLY][300];
        // sprintf(fname[DATA_DLY], "%s_format%d_line%dof%d_smp_%dof%d.dly", nm, format, line, m_num_line, sample,
        // m_num_sample);
        // sprintf(fname[DATA_DLY], "%s_format%d_L%d_S%d.dly", nm, format, line, sample);
        const char* str_f[2] = {"col", "mtx"};
        sprintf(fname[DATA_DLY], "%s_L%d_S%d.dly.%s", nm, line, sample, str_f[format & PRINT_MTX]);
        printf("MODEL_TEST_SCANLINE: Saving dly data in file %s\n", fname[DATA_DLY]);
        FILE* fp = fopen(fname[DATA_DLY], "w");
        assert(line <= m_num_line);
        assert(sample <= m_num_sample);
        assert(fp);
        for (int l = 0; l < line; l++) {
            if ((format & PRINT_MTX) && (format & PRINT_IDX)) fprintf(fp, "line_%d:\t", l);
            for (int s = 0; s < sample; s++) {
                fprintf(fp, "%e", this->m_p_data[l * m_num_sample + s]);
                if (!(format & PRINT_MTX))
                    fprintf(fp, "\n");
                else
                    fprintf(fp, "\t");
            }
            if ((format & PRINT_MTX)) fprintf(fp, "\n");
        }
        fclose(fp);
        return fname[DATA_DLY];
    }

    char* save4aie(const char* nm) {
        save4aie(0, nm, m_num_line, m_num_sample);
        return fname[DATA_DLY];
    }
};
#endif
