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
#ifndef __US_DEVICE_HPP__
#define __US_DEVICE_HPP__
#include "us_model_base.hpp"

class us_device : public us_model_base {
   public:
    float* rf_data;     //[line][sample][element], used by MbyM and LbyL
    float* rf_data_les; //[line][element][sample]

    us_device() : us_model_base(0, "dev1") {}

    void Init(float speed, int freq_snd, int freq_smp, int num_smp, int num_line, int num_elmt, const char* info) {
        m_speed_sound = speed; // 1540.0;
        m_inv_speed_of_sound = 1.0 / speed;
        m_freq_sound = freq_snd;    // 5*1000*1000;
        m_freq_sampling = freq_smp; // 100*1000*1000;
        m_num_sample = num_smp;     // 2048;
        m_num_line = num_line;      // 41;
        m_num_element = num_elmt;   // 128;
        m_type_rf = RF_JSON;
        assert(m_num_sample <= MAX_NUM_SAMPLE);
        assert(m_num_line <= MAX_NUM_LINE);
        assert(m_num_element <= MAX_NUM_ELEMENT);
        strcpy(this->m_info_init, info);
        // init_startPoint(start_positions);
        rf_data = NULL;
        rf_data_les = NULL;
        m_isInit = true;
    }
    void Init(float speed,
              int freq_snd,
              int freq_smp,
              int num_smp,
              int num_line,
              int num_elmt,
              const char* info,
              int type_rf,
              const char* nm_rf) {
        Init(speed, freq_snd, freq_smp, num_smp, num_line, num_elmt, info);
        this->init_rf_data(type_rf, nm_rf);
    }

    void init_startPoint(const float start_positions_fxied[4]) {
        if (!m_isInit) return;
        for (int w = 0; w < m_num_line; w++) {
            for (int dim = 0; dim < 4; dim++)
                m_p_start[dim][w] = start_positions_fxied[dim]; // to be changed by using us_device
        }
    }
    void init_rf_data() { init_rf_data("./out_abt_beamform.json"); }

    void init_rf_data(const char* nm_rf) {
        if (!m_isInit) return;
        nlohmann::json json_data;
        rf_data = (float*)malloc(m_num_line * m_num_sample * m_num_element * sizeof(float));
        rf_data_les = (float*)malloc(m_num_line * m_num_sample * m_num_element * sizeof(float));

        std::string input_json_path = nm_rf;
        printf("MODEL_TEST_SCANLINE: ******* Getting rf_data from default json file: %s\n", input_json_path.c_str());

        std::fstream inputValues(input_json_path);
        inputValues >> json_data;

        auto rf_data_vec = json_data["rf_data"].get<std::vector<std::vector<std::vector<float> > > >();

        int w_dev = m_num_line;
        int d_dev = m_num_sample;
        int e_dev = m_num_element;

        int w_data = rf_data_vec.size();
        int d_data = rf_data_vec[0].size();
        int e_data = rf_data_vec[0][0].size();
        // clang-format off
        printf("MODEL_TEST_SCANLINE: width of imaging points in rf_data: %d \tin the device: %d\n", w_data, w_dev);
        printf("MODEL_TEST_SCANLINE: depth of imaging points in rf_data: %d \tin the device: %d\n", d_data, d_dev); 
        printf("MODEL_TEST_SCANLINE: number of elements      in rf_data: %d \tin the device: %d\n", e_data, e_dev);
        // clang-format on
        for (int w = 0; w < w_data; ++w) {
            for (int d = 0; d < d_data; ++d) {
                for (int e = 0; e < e_data; ++e) {
                    rf_data[e_dev * d_dev * w + e_dev * d + e] = rf_data_vec.at(w).at(d).at(e);
                }
            }
        }
        for (int w = 0; w < w_data; ++w) {
            for (int e = 0; e < e_data; ++e) {
                for (int d = 0; d < d_data; ++d) {
                    rf_data_les[e_dev * d_dev * w + d_dev * e + d] = rf_data_vec.at(w).at(d).at(e);
                }
            }
        }
    }

    void save_les_txt(const char* nm) {
        char fname[128];
        sprintf(fname, "%s_line%d_element%d_sample%d.txt", nm, m_num_line, m_num_element, m_num_sample);
        FILE* fp = fopen(fname, "w");
        assert(fp);
        assert(rf_data_les);
        long size = m_num_line * m_num_element * m_num_sample;
        for (int i = 0; i < size; i++) fprintf(fp, "%.9f\n", rf_data_les[i]);
        fclose(fp);
        printf("%s: RF data saved in file %s in col-txt format\n", m_name_full, fname);
    }

    void save_les_bin(const char* nm) {
        char fname[128];
        sprintf(fname, "data_%s_line%d_element%d_sample%d.bin", nm, m_num_line, m_num_element, m_num_sample);
        FILE* fp = fopen(fname, "w");
        assert(fp);
        assert(rf_data_les);
        long size = m_num_line * m_num_element * m_num_sample;
        fwrite((void*)rf_data_les, sizeof(float), size, fp);
        fclose(fp);
        printf("%s: RF data saved in file %s in bin format\n", m_name_full, fname);
    }

    us_device(int id, const char* nm) : us_model_base(id, nm) {
        m_type = MODEL_DEVICE;
        genFullName();
        // print(stdout);
    }
    ~us_device() {
        if (rf_data) free(rf_data);
        if (rf_data_les) free(rf_data_les);
        // save();
    }

    int getImageP_f_size() { return m_num_line * m_num_sample; }

    float getWaveLength() { return m_speed_sound / (float)m_freq_sound; }

    float getSampleLength() { return m_speed_sound / (float)m_freq_sampling / 2.0; }

    float getSampleDepth() { return getSampleLength() * m_num_sample / 2.0; }
    float getSampleCycle() { return (float)m_num_sample / (float)m_freq_sampling; }
    float getImagingThroughput_in() { return float(m_num_element * m_num_sample) / getSampleCycle() / 1000000.0; }
    float getImagingThroughput_out() { return float(m_num_element * m_num_line) / getSampleCycle() / 1000000.0; }
    float getImagingfps() { return 1.0 / getSampleCycle() / (float)m_num_line; }

    void print_member(FILE* fp) {
        if (!m_isInit) {
            fprintf(fp, "%s: not be initialized\n", m_name_full);
            return;
        }
        // clang-format off
        fprintf(fp, "%s: %s\n", m_name_full, m_info_init);
        fprintf(fp, "%s: speed_sound     = %.1f \tm/s\n", m_name_full, m_speed_sound);
        fprintf(fp, "%s: freq_sound      = %d \tHz\n", m_name_full, m_freq_sound);
        fprintf(fp, "%s: Wave Length     = %.9f \tm\n", m_name_full, getWaveLength() * 1);
        fprintf(fp, "%s: freq_sampling   = %d \tHz\n", m_name_full, m_freq_sampling);
        fprintf(fp, "%s: num_sample      = %d \t\tsample / line\n", m_name_full, m_num_sample);
        fprintf(fp, "%s: num_line        = %d \t\tline / image\n", m_name_full, m_num_line);
        fprintf(fp, "%s: num_element     = %d \t\telemments on transducer\n", m_name_full, m_num_element);
        fprintf(fp, "%s: Sampling Length = %.9f \tm\n", m_name_full, getSampleLength() * 1);
        fprintf(fp, "%s: Sampling Depth  = %.9f \tm\n", m_name_full, getSampleDepth());
        fprintf(fp, "%s: Sampling Cycle  = %.9f \ts\n", m_name_full, getSampleCycle());
        fprintf(fp, "%s: Sampling Input  = %.3f \tMSps\n", m_name_full, getImagingThroughput_in() );
        fprintf(fp, "%s: Imaging output  = %.3f \tMPps\n", m_name_full, getImagingThroughput_out() );
        fprintf(fp, "%s: Imaging spf     = %.d \tPixel per frame\n", m_name_full, getImageP_f_size());
        fprintf(fp, "%s: Imaging fps     = %.3f \tfps\n", m_name_full, getImagingfps());
        // clang-format on
    }
    void print(FILE* fp) { print_member(fp); }

    void save4aie(int format, const char* nm, int line, int element, int sample) {
        char fname[300];
        // sprintf(fname, "%s_format%d_line%dof%d_ele%dof%d_smp_%dof%d.rf", nm, format, line, m_num_line, element,
        // m_num_element, sample, m_num_sample);
        // sprintf(fname, "%s_format%d_L%d_E%d_S%d.rfd", nm, format, line, element, sample);
        const char* str_f[2] = {"col", "mtx"};
        sprintf(fname, "%s_L%d_E%d_S%d.rfd.%s", nm, line, element, sample, str_f[format & PRINT_MTX]);
        printf("MODEL_TEST_SCANLINE: Saving RF data in file %s\n", fname);
        FILE* fp = fopen(fname, "w");
        assert(line <= m_num_line);
        assert(sample <= m_num_sample);
        assert(fp);
        for (int l = 0; l < line; l++) {
            for (int e = 0; e < element; e++) {
                if ((format & PRINT_MTX) && (format & PRINT_IDX)) fprintf(fp, "line:%d:ele:%d\t", l, e);
                for (int s = 0; s < sample; s++) {
                    fprintf(fp, "%.9f", this->rf_data_les[l * m_num_sample * m_num_element + e * m_num_sample + s]);
                    if (!(format & PRINT_MTX))
                        fprintf(fp, "\n");
                    else
                        fprintf(fp, "\t");
                }
                if ((format & PRINT_MTX)) fprintf(fp, "\n");
            } // for e
        }     // for l
        fclose(fp);
    }

    void save4aie(
        int format, const char* nm, int line_s, int element_s, int sample_s, int line_e, int element_e, int sample_e) {
        char fname[300];
        // sprintf(fname, "%s_format%d_line%dof%d_ele%dof%d_smp_%dof%d.rf", nm, format, line, m_num_line, element,
        // m_num_element, sample, m_num_sample);
        // sprintf(fname, "%s_format%d_L%d_E%d_S%d.rfd", nm, format, line, element, sample);
        const char* str_f[2] = {"col", "mtx"};
        sprintf(fname, "%s_L%d_%d_E%d_%d_S%d_%d.rfd.%s", nm, line_s, line_e, element_s, element_e, sample_s, sample_e,
                str_f[format & PRINT_MTX]);
        printf("MODEL_TEST_SCANLINE: Saving RF data in file %s\n", fname);
        FILE* fp = fopen(fname, "w");
        assert(line_e <= m_num_line);
        assert(element_e <= m_num_element);
        assert(sample_e <= m_num_sample);
        assert(fp);
        for (int l = line_s; l < line_e; l++) {
            for (int e = element_s; e < element_e; e++) {
                if ((format & PRINT_MTX) && (format & PRINT_IDX)) fprintf(fp, "line:%d:ele:%d\t", l, e);
                for (int s = sample_s; s < sample_e; s++) {
                    fprintf(fp, "%.9f", this->rf_data_les[l * m_num_sample * m_num_element + e * m_num_sample + s]);
                    if (!(format & PRINT_MTX))
                        fprintf(fp, "\n");
                    else
                        fprintf(fp, "\t");
                }
                if ((format & PRINT_MTX)) fprintf(fp, "\n");
            } // for e
        }     // for l
        fclose(fp);
    }
    void save4aie(const char* nm) { save4aie(0, nm, m_num_line, m_num_element, m_num_sample); }
    void init_rf_data(int type_rf, const char* nm_rf) {
        this->m_type_rf = type_rf;
        strcpy(this->m_name_rf, nm_rf);
        if (this->m_type_rf == RF_JSON)
            this->init_rf_data(m_name_rf);
        else if (this->m_type_rf == RF_2ELEMENT)
            createRFbyfewerElements(m_name_rf, this->m_num_element / 2 - 1, this->m_num_element / 2 + 1);
    }
    int createRFbyfewerElements(const char* fn, int e_start, int e_end) {
        assert(e_start >= 0);
        assert(e_start < m_num_element - 1);
        assert(e_end > 0);
        assert(e_end < m_num_element);
        assert(e_start < e_end);

        if (!m_isInit) {
            printf("Not initialized\n");
            exit(1);
        }
        std::fstream fin(fn, std::ios::in);
        if (!fin) {
            std::cout << "Error : createRFbyfewerElements() " << fn << "file doesn't exist !" << std::endl;
            exit(1);
        }
        rf_data = (float*)malloc(m_num_line * m_num_sample * m_num_element * sizeof(float));
        rf_data_les = (float*)malloc(m_num_line * m_num_sample * m_num_element * sizeof(float));
        for (int l = 0; l < m_num_line; l++) {
            for (int e = 0; e < m_num_element; e++) {
                for (int s = 0; s < m_num_sample; s++) {
                    float data;
                    if (e < e_start || e >= e_end)
                        data = 0.0;
                    else {
                        std::string line;
                        if (std::getline(fin, line)) {
                            std::stringstream istr(line);
                            istr >> data;
                        } else {
                            printf("File %s size not match at %d expecting %d * %d * %d = %d\n", fn,
                                   l * m_num_sample * m_num_element + e * m_num_sample + s, m_num_line, m_num_element,
                                   m_num_sample, m_num_sample * m_num_element * m_num_line);
                        }
                    }
                    rf_data_les[l * m_num_sample * m_num_element + e * m_num_sample + s] = data;
                    rf_data[l * m_num_sample * m_num_element + e + m_num_element * s] = data;
                }
            }
        }
        fin.close();
        return 0;
    }
};

;
#endif
