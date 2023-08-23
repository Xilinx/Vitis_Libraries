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
#ifndef __US_MODEL_BASE_HPP__
#define __US_MODEL_BASE_HPP__

#include <stdio.h>
#include <stdlib.h>
#include <cstring>
#include <cassert>
#include <fstream>
#include <iostream>
#include "us_example_parameter.hpp"
#include "json.hpp"

enum IDX_DIM { DIM_X = 0, DIM_Y, DIM_Z, DIM_E };
enum ENUM_PRINT_MODE { PURE_COL = 0, PURE_MTX, TITLE_COL, TITLE_MATRIX };
#define PRINT_MTX (1 << 0)
#define PRINT_IDX (1 << 1)
#define PRINT_FULL (1 << 2)
#define PRINT_PART (1 << 3)

#include <string>
template <typename T>
void writeFile(T* input, int N, std::string filename) {
    std::ofstream myfile0;
    myfile0.open(filename, std::ofstream::out | std::ofstream::app);
    for (int i = 0; i < N; i += 1) {
        myfile0 << input[i] << std::endl;
    }
    myfile0.close();
}
enum TYPE_RF { RF_JSON = 0, RF_2ELEMENT };
enum TYPE_M {
    MODEL_BASE = 0,
    MODEL_DEVICE,
    MODEL_IMAGEPOINT,
    MODEL_FOCUS,
    MODEL_DELAY,
    MODEL_APODIZATION,
    MODEL_SAMPLE,
    MODEL_INTERPOLATION,
    MODEL_MULT,
    MODEL_TAIL
};
enum DATA_M {
    DATA_RFD = 0,
    DATA_IMG0,
    DATA_IMG1,
    DATA_IMG2,
    DATA_IMG3,
    DATA_FOC,
    DATA_DLY,
    DATA_SMP,
    DATA_INS,
    DATA_INT,
    DATA_APO,
    DATA_MUL,
    NUM_DATA_M
};

class us_model_base {
   public:
    //////////////////////////////
    // Assistanc variables for algorithm object managements
    //////////////////////////////

    static const int num_type = MODEL_TAIL;
    const char* name_type[num_type] = {"MODEL_BASE",   "MODEL_DEVICE",        "MODEL_IMAGEPOINT",
                                       "MODEL_FOCUS",  "MODEL_DELAY",         "MODEL_APODIZATION",
                                       "MODEL_SAMPLE", "MODEL_INTERPOLATION", "MODEL_MULT"};
    TYPE_M m_type;
    char m_name_obj[64];
    char m_name_full[128];
    char m_info_init[128];
    // enum TYPE_RF {RF_JSON = 0, RF_2ELEMENT};
    int m_type_rf;
    char m_name_rf[256];
    char fname[NUM_DATA_M][256]; // file name for saving data
    bool m_isInit;
    int m_id;
    ////////////////////////////////////////////////////////////
    // common physic variables
    ////////////////////////////////////////////////////////////
    float m_speed_sound = 1540.0; // unit meter/sec
    float m_inv_speed_of_sound = 0.000649350649;
    int m_dim = 4;
    ////////////////////////////////////////////////////////////
    // Common initialized variable for all algorithm modules
    ////////////////////////////////////////////////////////////
    int m_freq_sound;    // unit Hz
    int m_freq_sampling; // unit Hz
    int m_num_line;      // up to MAX_NUM_LINE    (350)
    int m_num_element;   // up to MAX_NUM_ELEMENT (256)
    int m_num_sample;    // up to MAX_NUM_SAMPLE (4000)
    int m_num_upSample = 4;
    ////////////////////////////////////////////////////////////
    // Specific configurable parameter (Run-Time Parameters) for different algorithm modules
    ////////////////////////////////////////////////////////////
    // for image-points
    float m_p_start[4][MAX_NUM_LINE];
    // for delay
    float m_tx_def_ref_point[4] = {0};
    float tx_def_delay_distance = 0.02;
    float tx_def_delay_distance_ = 0.02;
    float m_tx_def_focal_point[4] = {0, 0, 0.02, 0};

    float m_tx_def_delay_distance[MAX_NUM_LINE];
    float m_tx_def_delay_distance_[MAX_NUM_LINE];
    float m_p_focal[4][MAX_NUM_LINE];

    float m_t_start[41] = {1.197e-05, 1.203e-05, 1.211e-05, 1.22e-05,  1.228e-05, 1.237e-05, 1.244e-05,
                           1.252e-05, 1.258e-05, 1.264e-05, 1.27e-05,  1.275e-05, 1.28e-05,  1.284e-05,
                           1.288e-05, 1.291e-05, 1.293e-05, 1.295e-05, 1.297e-05, 1.297e-05, 1.298e-05,
                           1.297e-05, 1.297e-05, 1.295e-05, 1.293e-05, 1.291e-05, 1.288e-05, 1.284e-05,
                           1.28e-05,  1.275e-05, 1.27e-05,  1.264e-05, 1.258e-05, 1.252e-05, 1.244e-05,
                           1.237e-05, 1.228e-05, 1.22e-05,  1.211e-05, 1.203e-05, 1.197e-05};
    // for apodization
    float m_f_number = 4.0;
    float m_tileVApo[4] = {0, 0, 1, 0};
    float m_ref_pos[4] = {0, 0, 0, 0};

    us_model_base(int id, const char* nm) {
        m_id = id;
        int len = strlen(nm);
        if (len > 0 && len < 64)
            strcpy(m_name_obj, nm);
        else
            strcpy(m_name_obj, "no_name");
        m_type = MODEL_BASE;
        m_isInit = false;
        strcpy(m_info_init, "This is a base class of US object");
    }

    void genFullName() { sprintf(m_name_full, "%s_%s_id_%d", name_type[m_type], m_name_obj, m_id); }

    void copyPara(us_model_base* p_obj) {
        this->m_speed_sound = p_obj->m_speed_sound;
        this->m_inv_speed_of_sound = p_obj->m_inv_speed_of_sound;
        this->m_freq_sound = p_obj->m_freq_sound;       // unit Hz
        this->m_freq_sampling = p_obj->m_freq_sampling; // unit Hz
        this->m_num_line = p_obj->m_num_line;           // up to MAX_NUM_LINE    (350)
        this->m_num_element = p_obj->m_num_element;     // up to MAX_NUM_ELEMENT (256)
        this->m_num_sample = p_obj->m_num_sample;       // up to MAX_NUM_SAMPLE (4000)
        memcpy((void*)this->m_p_start, (void*)p_obj->m_p_start, 4 * MAX_NUM_LINE * sizeof(float));
    }

    void save() {
        FILE* fp = fopen(m_name_full, "wb");
        assert(fp);
        print(fp);
        fclose(fp);
    }

    void save(int control) {
        FILE* fp = fopen(m_name_full, "wb");
        assert(fp);
        print(fp, control);
        fclose(fp);
    }

    void rstMem() {}

    ~us_model_base() {
        // printf("De-constructuring object(%s)\n", m_name_full);
    }

    virtual void print_member(FILE* fp) {
        fprintf(fp, "print_member(): Used for printing members of the object(%s)\n", m_name_full);
    };

    virtual void print_data(FILE* fp) {
        fprintf(fp, "print_data(): Used for printing data of the object(%s)\n", m_name_full);
    };

    virtual void print(FILE* fp) {
        print_member(fp);
        print_data(fp);
    };

    virtual void print_member(FILE* fp, int control) {
        fprintf(fp, "print_member(): Used for printing members of the object(%s)\n", m_name_full);
    };

    virtual void print_data(FILE* fp, int control) {
        fprintf(fp, "print_data(): Used for printing data of the object(%s)\n", m_name_full);
    };

    virtual void print(FILE* fp, int control) {
        print_member(fp);
        print_data(fp, control);
    };

    virtual void freeMem(){};
}

;
#endif
