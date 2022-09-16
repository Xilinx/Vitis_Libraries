/*
 * Copyright 2022 Xilinx, Inc.
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
#include "smart_ssd_cache.hpp"
#include <iostream>
#include <mutex>
#include <utility>
#include <thread>
#include <chrono>
#include <dirent.h>
#include <sys/stat.h>
#include <fnmatch.h>

namespace sssd_engine {
std::mutex m;
SmartSSDCache::SmartSSDCache(const char* xclbin_path, int card_num, sssd_info_t* disks, FILE* log)
    : card_nm(card_num), log_ptr(log) {
    cuCluster = new VPP_CC[card_num];
    for (int i = 0; i < card_num; ++i) {
        printf("add card %d\n", i);
        data_engine_acc::add_card(cuCluster[i], i);
    }
#ifdef USE_P2P
    csvInBufPool = data_engine_acc::create_bufpool(vpp::input, vpp::p2p); // for U.2. device only
#else
    csvInBufPool = data_engine_acc::create_bufpool(vpp::input);
#endif
    cfgInBufPool = data_engine_acc::create_bufpool(vpp::input);
    outBufPool = data_engine_acc::create_bufpool(vpp::output);
    metaBufPool = data_engine_acc::create_bufpool(vpp::bidirectional);

    for (int i = 0; i < card_num; ++i) {
        engine[i] =
            new data_engine_sc::DataEngine(i, csvInBufPool, cfgInBufPool, outBufPool, metaBufPool, &cuCluster[i]);
    }
    for (int i = 0; i < card_num; ++i) {
        MountInfo info;
        info.device_id = disks[i].device_id;
        info.mount_path = disks[i].mount_path;
        // ensure disks[i].mount_path ends with '/' here, so that it is okay to have both /mnt/disk0 and /mnt/disk01 as
        // mount point.
        if (info.mount_path.back() != '/') {
            info.mount_path += '/';
        }
        std::cout << info.mount_path << std::endl;
        mount_info.push_back(info);
    }
}
SmartSSDCache::~SmartSSDCache() {
    for (int i = 0; i < card_nm; ++i) {
        delete engine[i];
    }
}

ErrorCode SmartSSDCache::listFiles(const char* path_pattern, std::vector<std::string>& file_list) {
    std::string pattern = path_pattern;
    std::vector<std::string> lists;

    char sep = '/';
    char star = '*';

    std::string dir_name;
    size_t p = pattern.rfind(sep, pattern.length());
    if (p != std::string::npos) {
        dir_name = pattern.substr(0, p);
    } else {
        fprintf(stderr, "ERROR: could not get dir name due to file path error %s\n", pattern.c_str());
        return CFG_ERR;
    }
    std::string base_pattern = pattern.substr(p + 1, pattern.length() - 1 - p);
    std::string r_name;
    std::string pre_pattern;
    std::string mid_name;
    bool double_star = false;
    size_t p_s = dir_name.rfind(star, dir_name.length());
    if (p_s != std::string::npos) {
        size_t p_t_1 = dir_name.rfind(sep, p_s);
        size_t p_t_2 = dir_name.find(sep, p_s);
        if (p_t_1 != std::string::npos) {
            r_name = dir_name.substr(0, p_t_1);
            if (p_t_2 != std::string::npos) {
                pre_pattern = dir_name.substr(p_t_1 + 1, p_t_2 - p_t_1 - 1);
                mid_name = dir_name.substr(p_t_2 + 1, dir_name.length() - p_t_2 - 1);
            } else {
                pre_pattern = dir_name.substr(p_t_1 + 1, dir_name.length() - p_t_1);
                mid_name = "";
            }
        } else {
            fprintf(stderr, "ERROR: could not get dir name due to file path error %s\n", pattern.c_str());
            return CFG_ERR;
        }
        double_star = true;
    } else {
        r_name = dir_name;
        pre_pattern = "";
        mid_name = "";
    }

    std::vector<std::string> matched_dir;
    if (double_star) {
        // find the matched directory first
        if (auto d = opendir(r_name.c_str())) {
            while (auto f = readdir(d)) {
                if (!f->d_name || f->d_name[0] == '.') continue;
                // disable recursively,
                if (f->d_type == DT_DIR) {
                    auto r0 = fnmatch(pre_pattern.c_str(), f->d_name, FNM_PATHNAME | FNM_PERIOD);
                    if (!r0) {
                        std::string tmp_path = r_name + '/' + f->d_name;
                        if (mid_name.size() > 0) tmp_path += '/' + mid_name;
                        matched_dir.push_back(tmp_path);
                    }
                }
                if (f->d_type == DT_REG) continue;
            }
            closedir(d);
        }
    } else {
        matched_dir.push_back(r_name);
    }
    for (int i = 0; i < matched_dir.size(); ++i) {
        if (auto d = opendir(matched_dir[i].c_str())) {
            while (auto f = readdir(d)) {
                if (!f->d_name || f->d_name[0] == '.') continue;
                // disable recursively,
                if (f->d_type == DT_DIR) continue;
                if (f->d_type == DT_REG) {
                    std::string file_name = f->d_name;
                    auto ret = fnmatch(base_pattern.c_str(), f->d_name, FNM_PATHNAME | FNM_PERIOD);
                    if (!ret) {
                        std::string file_path = matched_dir[i] + '/' + f->d_name;
                        lists.push_back(file_path);
                    }
                }
            }
            closedir(d);
        }
    }
    std::vector<std::string> ordered_lists[card_nm];
    for (int i = 0; i < lists.size(); ++i) {
        int id = getDiskID(lists[i]);
        if (id != -1) {
            ordered_lists[id].push_back(lists[i]);
        } else {
            fprintf(stderr, "ERROR: %s is not on the mounted disks\n", lists[i].c_str());
        }
    }
    int i = 0;
    for (int j = 0; j < card_nm; ++j) {
        for (; i < ordered_lists[j].size(); ++i) {
            for (int c = j; c < card_nm; ++c) {
                if (i < ordered_lists[c].size()) {
                    file_list.push_back(ordered_lists[c][i]);
                }
            }
        }
    }
    return SUCCESS;
}

char* SmartSSDCache::scanFile(const char* fname, sssd_scandesc_t* sd, ErrorCode& err) {
    // check the fname
    std::string file_path(fname);
    char sep = '/';
    int disk_id = -1;
    printf("file_path = %s\n", fname);
    if (file_path.length() != 0 && file_path.at(0) == sep) {
        disk_id = getDiskID(file_path);
        if (disk_id == -1) {
            fprintf(stderr, "ERROR: %s is not on the mounted disks\n", file_path.c_str());
            err = CFG_ERR;
            return "";
        }
        // check if it is gzip compressed
        bool gzip = false;
        if (file_path.compare(file_path.size() - 3, 3, ".gz") == 0)
            gzip = 1;
        else
            gzip = 0;
        // generate kernel config
        uint64_t* cfg_buf = new uint64_t[DDR_SIZE_CFG_LWORD];
        err = dg_cfg.genConfigBits(gzip, sd, cfg_buf);
        if (err != SUCCESS) {
            fprintf(stderr, "ERROR[%d]: genConfigBits failed\n", err);
            return nullptr;
        }
        std::promise<RetObj> prom;
        std::future<RetObj> fut = prom.get_future();
        // need to change
        // consider to device id when storing file
        uint32_t file_size = getFileSize(file_path);
        std::cout << "Selected device: " << disk_id << std::endl;
        engine[disk_id]->pushRequest(std::move(prom), file_path, file_size, cfg_buf);
        // blocking
        RetObj rt = fut.get();

        // release the cfg_buf
        delete[] cfg_buf;

        err = rt.status;
        if (err != SUCCESS) {
            fprintf(stderr, "ERROR[%d]: scan impl failed\n", err);
            return nullptr;
        } else {
            char* out = rt.data;
            uint32_t size = rt.size;
            return out;
        }
    } else {
        fprintf(stderr, "ERROR: fname must the full path\n");
        err = CFG_ERR;
        return "";
    }
}

int32_t SmartSSDCache::getDiskID(const std::string& file_path) {
    int32_t disk_id = -1;
    for (int i = 0; i < card_nm; ++i) {
        if (file_path.rfind(mount_info[i].mount_path, 0) == 0) {
            disk_id = i;
            break;
        }
    }
    if (disk_id == -1) {
        fprintf(stderr, "ERROR: could not get disk id due to file path error %s\n", file_path.c_str());
    }
    return disk_id;
}
void SmartSSDCache::print_input(const sssd_scandesc_t* sd) {
    std::string type_name[11] = {"sssd_invalid", "sssd_bool",   "sssd_int",  "sssd_float",    "", "", "",
                                 "sssd_numeric", "sssd_string", "sssd_date", "sssd_timestamp"};
    std::string cmp_name[8] = {"SSSD_GT", "SSSD_GE",  "SSSD_LT",  "SSSD_LE",
                               "SSSD_EQ", "SSSD_NEQ", "SSSD_NOT", "SSSD_ISNULL"};
    if (log_ptr != NULL) {
        fprintf(log_ptr, "sssd_schema_t.natt: %d, ", sd->schema.natt);
        for (int i = 0; i < sd->schema.natt; ++i) {
            fprintf(log_ptr, "dtype[%d]: %s, ", i, type_name[sd->schema.dtype[i]].c_str());
        }
        fprintf(log_ptr, "ftype: %s\n", sd->schema.ftype);
        fprintf(log_ptr, "projection: %d cols, ", sd->natt);
        for (int i = 0; i < sd->natt; ++i) {
            fprintf(log_ptr, "att[%d]: %d, ", i, sd->att[i]);
        }
        fprintf(log_ptr, "\n");
        fprintf(log_ptr, "nfilter: %d\n", sd->nfilter);
        for (int i = 0; i < sd->nfilter; ++i) {
            fprintf(log_ptr, "filter[%d].att: %d, dtype: %s, cmp: %s, ", i, sd->filter[i]->att,
                    type_name[sd->filter[i]->dtype].c_str(), cmp_name[sd->filter[i]->cmp].c_str());
            switch (sd->filter[i]->dtype) {
                case (SSSD_DTYPE_INT): {
                    fprintf(log_ptr, "%d\n", sd->filter[i]->arg_value.cmp_b);
                    break;
                }
                case (SSSD_DTYPE_BOOL): {
                    fprintf(log_ptr, "%d\n", sd->filter[i]->arg_value.cmp_i64);
                    break;
                }
                case (SSSD_DTYPE_DATE): {
                    fprintf(log_ptr, "%d-%d-%d\n", sd->filter[i]->arg_value.cmp_date.year,
                            sd->filter[i]->arg_value.cmp_date.month, sd->filter[i]->arg_value.cmp_date.day);
                    break;
                }
                case (SSSD_DTYPE_NUMERIC): {
                    fprintf(log_ptr, "%de%d\n", sd->filter[i]->arg_value.cmp_n.significand,
                            sd->filter[i]->arg_value.cmp_n.exponent);
                    break;
                }
                case (SSSD_DTYPE_STRING): {
                    fprintf(log_ptr, "%d chars %.*s\n", sd->filter[i]->arg_value.cmp_s.len,
                            sd->filter[i]->arg_value.cmp_s.len, sd->filter[i]->arg_value.cmp_s.byte);
                    break;
                }
                default:
                    fprintf(log_ptr, "ERROR: not supported output data type\n");
            }
        }
        fprintf(log_ptr, "nhash: %d, ", sd->nhashatt);
        for (int i = 0; i < sd->nhashatt; ++i) {
            fprintf(log_ptr, "hashatt[%d]: %d, ", i, sd->hashatt[i]);
        }
        fprintf(log_ptr, "\n");
        fflush(log_ptr);
    }
}
void SmartSSDCache::print_output(const int64_t value[], const bool isnull[], int32_t hash, const sssd_scandesc_t* sd) {
    if (log_ptr != NULL) {
        for (int i = 0; i < sd->natt; ++i) {
            int nm = sd->att[i];
            if (!isnull[i]) {
                switch (sd->schema.dtype[nm]) {
                    case (SSSD_DTYPE_INT):
                    case (SSSD_DTYPE_BOOL): {
                        fprintf(log_ptr, "%d, ", value[i]);
                        break;
                    }
                    case (SSSD_DTYPE_DATE): {
                        sssd_date_t dt;
                        memcpy(&dt, &value[i], sizeof(int64_t));
                        fprintf(log_ptr, "%d-%d-%d, ", dt.year, dt.month, dt.day);
                        break;
                    }
                    case (SSSD_DTYPE_NUMERIC): {
                        sssd_numeric_t dt;
                        memcpy(&dt, &value[i], sizeof(int64_t));
                        fprintf(log_ptr, "%de%d, ", dt.significand, dt.exponent);
                        break;
                    }
                    case (SSSD_DTYPE_STRING): {
                        sssd_string_t* dt = reinterpret_cast<sssd_string_t*>(value[i]);
                        fprintf(log_ptr, "%d chars %.*s, ", dt->len, dt->len, dt->byte);
                        break;
                    }
                    default:
                        fprintf(log_ptr, "ERROR: not supported output data type, ");
                }
            } else {
                fprintf(log_ptr, "IS NULL, ");
            }
        }
        fprintf(log_ptr, "hash: %d\n", hash);
        fflush(log_ptr);
    }
}

void SmartSSDCache::release(const char* file_path, char* buff_ptr) {
    int disk_id = getDiskID(file_path);
    engine[disk_id]->release(buff_ptr);
}

} // namespace sssd_engine
