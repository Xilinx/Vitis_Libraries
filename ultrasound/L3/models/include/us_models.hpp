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
#ifndef __US_MODELS_HPP__
#define __US_MODELS_HPP__
#include "us_system.hpp"
//#include "json.hpp"

template <class T, int NUM_LINE_t, int NUM_ELEMENT_t, int NUM_SAMPLE_t, int NUM_SEG_t>
class us_models {
   public:
    us_device dev1;
    us_op_imagepoint<T> obj_img;
    us_op_focus<T> obj_foc;
    us_op_delay<T> obj_dly;
    us_op_apodization<T> obj_apo;
    us_op_sample<T> obj_smp;
    us_op_interpolation<T> obj_int;
    us_op_mult<T> obj_mul;

    void Init(T speed,
              int freq_snd,
              int freq_smp,
              int num_smp,
              int num_line,
              int num_elmt,
              const T* start_positions,
              const char* info,
              int type_rf,
              const char* nm_rf) {
        dev1.Init(example_1_speed_sound, example_1_freq_sound, example_1_freq_sampling, example_1_num_sample,
                  example_1_num_line, example_1_num_element, example_1_info);
        dev1.init_startPoint(start_positions);
        // dev1.init_rf_data();
        dev1.init_rf_data(type_rf, nm_rf);
        dev1.print(stdout);

        obj_img.init(&dev1);
        obj_foc.init(&dev1);
        obj_dly.init(&dev1);
        obj_apo.init(&dev1);
        obj_smp.init(&dev1);
        obj_int.init(&dev1);
        obj_mul.init(&dev1);
    }
    void rstMem() {
        obj_img.rstMem();
        obj_foc.rstMem();
        obj_dly.rstMem();
        obj_apo.rstMem();
        obj_smp.rstMem();
        obj_int.rstMem();
        obj_mul.rstMem();
    }

    void saveTxt(int code_print, // code_print&PRINT_MTX is 1 for matrix format otherwise
                 int num_print_line,
                 int num_print_element,
                 int num_print_sample,
                 const char* nm_dir,
                 const char* pre_fn,
                 DATA_M id) {
        if (code_print || id < NUM_DATA_M) {
            char pre_dir_fn[256];
            sprintf(pre_dir_fn, "%s/%s", nm_dir, pre_fn);
            char cmd_testdir[256];
            sprintf(cmd_testdir, "test -d %s", nm_dir);
            if (system(cmd_testdir) != 0) {
                char cmd_mkdir[256];
                sprintf(cmd_mkdir, "mkdir %s", nm_dir);
                system(cmd_mkdir);
            }
            int isMtx = code_print & PRINT_MTX;
            if (id < NUM_DATA_M && id == DATA_RFD)
                this->dev1.save4aie(isMtx, pre_dir_fn, num_print_line, num_print_element, num_print_sample);
            if (code_print || (id >= DATA_IMG0 && id <= DATA_IMG3))
                this->obj_img.save4aie(isMtx, pre_dir_fn, num_print_line, num_print_sample);
            if (code_print || id == DATA_FOC)
                this->obj_foc.save4aie(isMtx, pre_dir_fn, num_print_line, num_print_element);
            if (code_print || id == DATA_DLY)
                this->obj_dly.save4aie(isMtx, pre_dir_fn, num_print_line, num_print_sample);
            if (code_print || id == DATA_SMP)
                this->obj_smp.save4aie(isMtx, pre_dir_fn, num_print_line, num_print_element, num_print_sample);
            if (code_print || id == DATA_INT || id == DATA_INS)
                this->obj_int.save4aie(isMtx, pre_dir_fn, num_print_line, num_print_element, num_print_sample);
            if (code_print || id == DATA_APO)
                this->obj_apo.save4aie(isMtx, pre_dir_fn, num_print_line, num_print_element, num_print_sample);
            if (code_print || id == DATA_MUL)
                this->obj_mul.save4aie(isMtx, pre_dir_fn, num_print_line, num_print_sample);
            collectFileName();
        }
    }
    void collectFileName() {
        strcpy(dev1.fname[DATA_IMG0], obj_img.fname[DATA_IMG0]);
        printf("%s\n", dev1.fname[DATA_IMG0]);
        strcpy(dev1.fname[DATA_IMG2], obj_img.fname[DATA_IMG2]);
        printf("%s\n", dev1.fname[DATA_IMG2]);
        strcpy(dev1.fname[DATA_FOC], obj_foc.fname[DATA_FOC]);
        printf("%s\n", dev1.fname[DATA_FOC]);
        strcpy(dev1.fname[DATA_DLY], obj_dly.fname[DATA_DLY]);
        printf("%s\n", dev1.fname[DATA_DLY]);
        strcpy(dev1.fname[DATA_SMP], obj_smp.fname[DATA_SMP]);
        printf("%s\n", dev1.fname[DATA_SMP]);
        strcpy(dev1.fname[DATA_INT], obj_int.fname[DATA_INT]);
        printf("%s\n", dev1.fname[DATA_INT]);
        strcpy(dev1.fname[DATA_INS], obj_int.fname[DATA_INS]);
        printf("%s\n", dev1.fname[DATA_INS]);
        strcpy(dev1.fname[DATA_APO], obj_apo.fname[DATA_APO]);
        printf("%s\n", dev1.fname[DATA_APO]);
        strcpy(dev1.fname[DATA_MUL], obj_mul.fname[DATA_MUL]);
        printf("%s\n", dev1.fname[DATA_MUL]);
    }

    void save4aie_fullSize(int code, const char* pre_fn, DATA_M id) {
        saveTxt(code, dev1.m_num_line, dev1.m_num_element, dev1.m_num_sample, "data_model", pre_fn, id);
    }
    void save4aie_fullSize(const char* pre_fn) { save4aie_fullSize(PRINT_PART, pre_fn, NUM_DATA_M); }
    void save4aie_oneSampleline(const char* pre_fn) {
        int code = PRINT_PART;
        saveTxt(code, 1, 1, dev1.m_num_sample, "data_model", pre_fn, NUM_DATA_M);
    }
    void save4aie_oneScanline(const char* pre_fn) {
        int code = PRINT_PART;
        saveTxt(code, 1, dev1.m_num_element, dev1.m_num_sample, "data_model", pre_fn, NUM_DATA_M);
    }
    void saveJson(const char* fn) {
        std::string output_json_path = fn;
        // Json_out<float, NUM_LINE_t, NUM_SAMPLE_t>(obj_mul, output_json_path);
        std::fstream outJson;
        nlohmann::json json_out;
        std::string res;

        printf("MODEL_TEST_SCANLINE: Saving image in Json file %s\n", output_json_path.c_str());
        outJson.open(output_json_path, std::ios::out);
        if (!outJson.is_open()) {
            std::cout << "Can't open file" << std::endl;
        }

        float rf_data_[NUM_LINE_t * NUM_SAMPLE_t * 4] = {0};
        assert(NUM_LINE_t * NUM_SAMPLE_t * 4 >= obj_mul.m_num_line * obj_mul.m_num_sample * 4);
        for (int i = 0; i < obj_mul.m_num_line * obj_mul.m_num_sample * 4; i++) rf_data_[i] = obj_mul.m_p_data[i];
        json_out["rf_data"] = rf_data_;
        res = json_out.dump();
        outJson << res;
        outJson.close();
    }
    /*
    int scanline_MbyM(){
        return scanline_MbyM<T, NUM_LINE_t, NUM_ELEMENT_t, NUM_SAMPLE_t>((*this));
    }*/

    int diffCheck(const char* fnm_aie, DATA_M id_data, float th_err_abs, float th_err_ratio) {
        return fdatacmp<float>(fnm_aie, dev1.fname[id_data], th_err_abs, th_err_ratio);
    }
};
#endif
