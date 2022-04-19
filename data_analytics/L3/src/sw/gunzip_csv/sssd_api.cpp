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
#include "sssd_api.h"
#include "smart_ssd_cache.hpp"

using namespace sssd_engine;
#ifdef __cplusplus
extern "C" {
#endif

sssd_t sssd_init(const char* xclbinpath, int ndisk, sssd_info_t* disks) {
    const char* log_path = getenv("SSSD_ENGINE_LOG_FILE");
    FILE* log = NULL;
    if (log_path) {
        std::string path(log_path);
        path = path + "/log";
        log = fopen(path.c_str(), "a");
        if (log == NULL) {
            fprintf(stderr, "ERROR: SSSD_ENGINE_LOG_FILE %s cannot be opened\n", path);
        }
    } else {
        fprintf(stderr, "Warning: please set SSSD_ENGINE_LOG_FILE as log path\n");
    }
    return reinterpret_cast<void*>(new SmartSSDCache(xclbinpath, ndisk, disks, log));
}

void sssd_final(sssd_t handle) {
    delete reinterpret_cast<SmartSSDCache*>(handle);
}

int sssd_scan(sssd_t sssd, const char* fname, sssd_scandesc_t* sd, sssd_scanfn_t fn, void* context) {
    // step-1: call the function of sssd to execute scan
    ErrorCode err = SUCCESS;
    SmartSSDCache* sssd_impl = reinterpret_cast<SmartSSDCache*>(sssd);
    sssd_impl->print_input(sd);
    if (sd->schema.u.csv.header != 0 || sd->schema.u.csv.delim != 0 || sd->schema.u.csv.quote != 0) {
        fprintf(stderr,
                "ERROR: only no header, comma delimiter and double quote supported by now, escape and nullstr not "
                "supported\n");
        return -1;
    }
    char* out = sssd_impl->scanFile(fname, sd, err /*, sz*/);
    int64_t* out_ctxt = new int64_t[sd->natt];
    //???How to use
    bool* isnull = new bool[sd->natt];
    int32_t hash = 0;
    if (err != SUCCESS) {
        fprintf(stderr, "ERROR[%d]: scanFile failed\n", err);
        return -1;
    }

    assert(out != nullptr);

    uint32_t tot_size;
    memcpy(&tot_size, out, sizeof(int));
    uint32_t offset = sizeof(int);
    int r = 0;
    while (offset < tot_size) {
        for (int j = 0; j < sd->natt; ++j) {
            int nm = sd->att[j];
            if (sd->schema.dtype[nm] == SSSD_DTYPE_BOOL) {
                out_ctxt[j] = *(out + offset);
                offset += 1;
            } else if (sd->schema.dtype[nm] == SSSD_DTYPE_STRING) {
                out_ctxt[j] = reinterpret_cast<int64_t>(out + offset);
                offset += (sizeof(int32_t) + *reinterpret_cast<int32_t*>(out + offset));
            } else {
                out_ctxt[j] = *reinterpret_cast<int64_t*>(out + offset);
                offset += 8;
            }
            isnull[j] = false;
        }
        if (sd->nhashatt > 0) {
            hash = *reinterpret_cast<int32_t*>(out + offset);
            offset += sizeof(int32_t);
        }
        r = fn(out_ctxt, isnull, hash, context);
        if (r == -1) {
            fprintf(stderr, "ERROR: callback sssd_scanfn failed\n");
            break;
        }
    }
    // release memory
    delete[] out_ctxt;
    delete[] isnull;
    sssd_impl->release(fname, out);
    return r;
}

int sssd_list(sssd_t sssd, const char* path_pattern, sssd_listfn_t fn, void* context) {
    // step-1: call

    SmartSSDCache* sssd_impl = reinterpret_cast<SmartSSDCache*>(sssd);
    std::vector<std::string> file_list;
    ErrorCode err = sssd_impl->listFiles(path_pattern, file_list);
    if (err != SUCCESS) {
        fprintf(stderr, "ERROR[%d]: listFiles failed\n", err);
        return -1;
    }
    printf("Get %d files\n", file_list.size());
    int r = 0;
    for (int i = 0; i < file_list.size(); ++i) {
        r = fn(file_list[i].c_str(), context);
        if (r == -1) {
            fprintf(stderr, "ERROR: callback sssd_listfn failed\n");
            break;
        }
    }
    return r;
}
int sssd_errnum(sssd_t handle) {
    printf("Warning: sssd_errnum is not implemented by now\n");
    return 0;
}

const char* sssd_errmsg(sssd_t handle) {
    return "Warning: sssd_errmsg is not implemented by now\n";
}
#ifdef __cplusplus
}
#endif
