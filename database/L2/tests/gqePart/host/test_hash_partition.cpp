/*
 * Copyright 2019 Xilinx, Inc.
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

// OpenCL C API utils
#include "xclhost.hpp"
#include "x_utils.hpp"
// L1
#include "xf_database/hash_lookup3.hpp"
// GQE L2
#include "xf_database/meta_table.hpp"
#include "xf_database/join_command.hpp"
// HLS
#include "ap_int.h"

#include "table_dt.hpp"

#include <sys/time.h>
#include <algorithm>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <cstdio>
#include <thread>
#include <unistd.h>
const int HASHWH = 2;
const int HASHWL = 8;

const int PU_NM = 8;
const int VEC_SCAN = 8; // 256-bit column.

inline int tvdiff(const timeval& tv0, const timeval& tv1) {
    return (tv1.tv_sec - tv0.tv_sec) * 1000000 + (tv1.tv_usec - tv0.tv_usec);
}

template <typename T>
int generate_data(T* data, int range, size_t n) {
    if (!data) {
        return -1;
    }

    for (size_t i = 0; i < n; i++) {
        data[i] = (T)(rand() % range + 1);
    }

    return 0;
}

class MM {
   private:
    size_t _total;
    std::vector<void*> _pvec;

   public:
    MM() : _total(0) {}
    ~MM() {
        for (void* p : _pvec) {
            if (p) free(p);
        }
    }
    size_t size() const { return _total; }
    template <typename T>
    T* aligned_alloc(std::size_t num) {
        void* ptr = nullptr;
        size_t sz = num * sizeof(T);
        if (posix_memalign(&ptr, 4096, sz)) throw std::bad_alloc();
        _pvec.push_back(ptr);
        _total += sz;
        //  printf("align_alloc %lu Bytes\n", sz);
        return reinterpret_cast<T*>(ptr);
    }
    void total_mem() {
        std::cout << "+++++++++++++++++++++++++++++++++++++++" << std::endl;
        std::cout << "++ total host mem used: " << (double)_total / 1024 / 1024 / 1024 << " GB" << std::endl;
        std::cout << "+++++++++++++++++++++++++++++++++++++++" << std::endl;
    }
};

template <typename T>
int load_dat(void* data, const std::string& name, const std::string& dir, const int sf, const size_t n) {
    if (!data) {
        return -1;
    }
    std::string fn = dir + "/dat" + std::to_string(sf) + "/" + name + ".dat";
    FILE* f = fopen(fn.c_str(), "rb");
    if (!f) {
        std::cerr << "ERROR: " << fn << " cannot be opened for binary read." << std::endl;
    }
    size_t cnt = fread(data, sizeof(T), n, f);
    fclose(f);
    if (cnt != n) {
        std::cerr << "ERROR: " << cnt << " entries read from " << fn << ", " << n << " entries required." << std::endl;
        return -1;
    }
    return 0;
}

int main(int argc, const char* argv[]) {
    // cmd arg parser.
    x_utils::ArgParser parser(argc, argv);

    std::string xclbin_path;
    if (!parser.getCmdOption("-xclbin", xclbin_path)) {
        std::cout << "ERROR: xclbin path is not set!\n";
        return 1;
    }
    std::string scale;
    int sim_scale = 1;
    if (parser.getCmdOption("-scale", scale)) {
        try {
            sim_scale = std::stoi(scale);
        } catch (...) {
            sim_scale = 10000;
        }
    }

    // int l_nrow = 1000;
    int l_nrow = L_MAX_ROW / sim_scale;
    std::cout << "Lineitem " << l_nrow << " rows\n";

    // setup OpenCL related
    cl_context ctx;
    cl_device_id dev_id;
    cl_command_queue cmq;
    cl_program prg;
    int err = xclhost::init_hardware(&ctx, &dev_id, &cmq,
                                     CL_QUEUE_PROFILING_ENABLE | CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE, MSTR(XDEVICE));
    if (err != CL_SUCCESS) {
        fprintf(stderr, "ERROR: fail to init OpenCL with " MSTR(XDEVICE) "\n");
        return err;
    }
    err = xclhost::load_binary(&prg, ctx, dev_id, xclbin_path.c_str());
    if (err != CL_SUCCESS) {
        fprintf(stderr, "ERROR: fail to program PL\n");
        return err;
    }
    // partition kernel
    cl_kernel partkernel;
    partkernel = clCreateKernel(prg, "gqePart", &err);

    // ------------------------------------------
    // --------- partitioning Table L ----------
    // partition setups
    const int k_depth = 512;
    const int log_partition_num = 3;
    const int partition_num = 1 << log_partition_num;

    // the col nrow of each section
    int tab_part_sec_nrow_each = l_nrow;
    int tab_part_sec_size = tab_part_sec_nrow_each * sizeof(TPCH_INT);

    MM mm;

    // data load from disk. due to table size, data read into several sections
    // L host side pinned buffers for partition kernel
    TPCH_INT* tab_part_in_col[8];
    for (int i = 0; i < 8; i++) {
        tab_part_in_col[i] = mm.aligned_alloc<TPCH_INT>(tab_part_sec_nrow_each);
        err += generate_data<TPCH_INT>((int*)(tab_part_in_col[i]), 1000, l_nrow);
    }

    if (err) {
        fprintf(stderr, "ERROR: failed to load dat/gen file.\n");
        return 1;
    }
    std::cout << "finished dat loading/generating" << std::endl;

    // partition output data
    int tab_part_out_col_nrow_512_init = tab_part_sec_nrow_each * 1.3 / VEC_LEN;
    assert(tab_part_out_col_nrow_512_init > 0 && "Error: table output col size must > 0");
    // the depth of each partition in each col
    int tab_part_out_col_eachpart_nrow_512 = (tab_part_out_col_nrow_512_init + partition_num - 1) / partition_num;
    // pool.l_partition_out_col_part_nrow_max = tab_part_out_col_eachpart_nrow_512 * 16;
    int tab_part_out_col_nrow_512 = partition_num * tab_part_out_col_eachpart_nrow_512;
    int tab_part_out_col_size = tab_part_out_col_nrow_512 * sizeof(TPCH_INT) * VEC_LEN;

    // partition_output data
    ap_uint<512>* tab_part_out_col[8];
    for (int i = 0; i < 8; i++) {
        tab_part_out_col[i] = mm.aligned_alloc<ap_uint<512> >(tab_part_out_col_nrow_512);
    }

    using jcmdclass = xf::database::gqe::JoinCommand;
    jcmdclass jcmd = jcmdclass();

    jcmd.setJoinType(xf::database::INNER_JOIN);

    jcmd.Scan(0, {0, 1, 2, 3, 4, 5, 6});
    // jcmd.setDualKeyOn();
    ap_uint<512>* cfg_part = jcmd.getConfigBits();

    //--------------- metabuffer setup L -----------------
    xf::database::gqe::MetaTable meta_part_in;
    xf::database::gqe::MetaTable meta_part_out;
    meta_part_in.setColNum(7);
    meta_part_in.setCol(0, 0, tab_part_sec_nrow_each);
    meta_part_in.setCol(1, 1, tab_part_sec_nrow_each);
    meta_part_in.setCol(2, 2, tab_part_sec_nrow_each);
    meta_part_in.setCol(3, 3, tab_part_sec_nrow_each);
    meta_part_in.setCol(4, 4, tab_part_sec_nrow_each);
    meta_part_in.setCol(5, 5, tab_part_sec_nrow_each);
    meta_part_in.setCol(6, 6, tab_part_sec_nrow_each);
    meta_part_in.meta();

    // setup partition kernel used meta output
    meta_part_out.setColNum(7);
    meta_part_out.setPartition(partition_num, tab_part_out_col_eachpart_nrow_512);
    meta_part_out.setCol(0, 0, tab_part_out_col_nrow_512);
    meta_part_out.setCol(1, 1, tab_part_out_col_nrow_512);
    meta_part_out.setCol(2, 2, tab_part_out_col_nrow_512);
    meta_part_out.setCol(3, 3, tab_part_out_col_nrow_512);
    meta_part_out.setCol(4, 4, tab_part_out_col_nrow_512);
    meta_part_out.setCol(5, 5, tab_part_out_col_nrow_512);
    meta_part_out.setCol(6, 6, tab_part_out_col_nrow_512);
    meta_part_out.meta();

    cl_mem_ext_ptr_t mext_tab_part_in_col[8];
    for (int i = 0; i < 8; ++i) {
        mext_tab_part_in_col[i] = {(3 + i), tab_part_in_col[i], partkernel};
    }

    cl_mem_ext_ptr_t mext_tab_part_out_col[8];
    for (int i = 0; i < 8; ++i) {
        mext_tab_part_out_col[i] = {(13 + i), tab_part_out_col[i], partkernel};
    }
    cl_mem_ext_ptr_t mext_cfg_part = {21, cfg_part, partkernel};

    // dev buffers, part in
    cl_mem buf_tab_part_in_col[8];
    for (int c = 0; c < 8; c++) {
        if (c < 7) {
            buf_tab_part_in_col[c] = clCreateBuffer(ctx, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY,
                                                    tab_part_sec_size, &mext_tab_part_in_col[c], &err);
        } else {
            buf_tab_part_in_col[c] = clCreateBuffer(ctx, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY,
                                                    16 * sizeof(TPCH_INT), &mext_tab_part_in_col[c], &err);
        }
    }

    // dev buffers, part out
    std::cout << "Input device buffer has been created" << std::endl;
    cl_mem buf_tab_part_out_col[8];
    for (int c = 0; c < 8; c++) {
        if (c < 7) {
            buf_tab_part_out_col[c] =
                clCreateBuffer(ctx, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                               tab_part_out_col_size, &mext_tab_part_out_col[c], &err);
        } else {
            buf_tab_part_out_col[c] =
                clCreateBuffer(ctx, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                               16 * sizeof(TPCH_INT), &mext_tab_part_out_col[c], &err);
        }
    }
    std::cout << "Output device buffer has been created" << std::endl;

    cl_mem buf_cfg_part = clCreateBuffer(ctx, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY,
                                         (sizeof(ap_uint<512>) * 9), &mext_cfg_part, &err);

    cl_mem_ext_ptr_t mext_meta_part_in, mext_meta_part_out;

    mext_meta_part_in = {11, meta_part_in.meta(), partkernel};
    mext_meta_part_out = {12, meta_part_out.meta(), partkernel};

    cl_mem buf_meta_part_in;
    buf_meta_part_in = clCreateBuffer(ctx, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY,
                                      (sizeof(ap_uint<512>) * 8), &mext_meta_part_in, &err);
    cl_mem buf_meta_part_out;
    buf_meta_part_out = clCreateBuffer(ctx, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                                       (sizeof(ap_uint<512>) * 24), &mext_meta_part_out, &err);

    //----------------------partition L run-----------------------------
    std::cout << "------------------- Partitioning L table -----------------" << std::endl;
    const int idx = 0;
    int j = 0;
    clSetKernelArg(partkernel, j++, sizeof(int), &k_depth);
    clSetKernelArg(partkernel, j++, sizeof(int), &idx);
    clSetKernelArg(partkernel, j++, sizeof(int), &log_partition_num);
    clSetKernelArg(partkernel, j++, sizeof(cl_mem), &buf_tab_part_in_col[0]);
    clSetKernelArg(partkernel, j++, sizeof(cl_mem), &buf_tab_part_in_col[1]);
    clSetKernelArg(partkernel, j++, sizeof(cl_mem), &buf_tab_part_in_col[2]);
    clSetKernelArg(partkernel, j++, sizeof(cl_mem), &buf_tab_part_in_col[3]);
    clSetKernelArg(partkernel, j++, sizeof(cl_mem), &buf_tab_part_in_col[4]);
    clSetKernelArg(partkernel, j++, sizeof(cl_mem), &buf_tab_part_in_col[5]);
    clSetKernelArg(partkernel, j++, sizeof(cl_mem), &buf_tab_part_in_col[6]);
    clSetKernelArg(partkernel, j++, sizeof(cl_mem), &buf_tab_part_in_col[7]);
    clSetKernelArg(partkernel, j++, sizeof(cl_mem), &buf_meta_part_in);
    clSetKernelArg(partkernel, j++, sizeof(cl_mem), &buf_meta_part_out);
    clSetKernelArg(partkernel, j++, sizeof(cl_mem), &buf_tab_part_out_col[0]);
    clSetKernelArg(partkernel, j++, sizeof(cl_mem), &buf_tab_part_out_col[1]);
    clSetKernelArg(partkernel, j++, sizeof(cl_mem), &buf_tab_part_out_col[2]);
    clSetKernelArg(partkernel, j++, sizeof(cl_mem), &buf_tab_part_out_col[3]);
    clSetKernelArg(partkernel, j++, sizeof(cl_mem), &buf_tab_part_out_col[4]);
    clSetKernelArg(partkernel, j++, sizeof(cl_mem), &buf_tab_part_out_col[5]);
    clSetKernelArg(partkernel, j++, sizeof(cl_mem), &buf_tab_part_out_col[6]);
    clSetKernelArg(partkernel, j++, sizeof(cl_mem), &buf_tab_part_out_col[7]);
    clSetKernelArg(partkernel, j++, sizeof(cl_mem), &buf_cfg_part);

    // partition h2d
    std::vector<cl_mem> part_in_vec;
    part_in_vec.push_back(buf_tab_part_in_col[0]);
    part_in_vec.push_back(buf_tab_part_in_col[1]);
    part_in_vec.push_back(buf_tab_part_in_col[2]);
    part_in_vec.push_back(buf_tab_part_in_col[3]);
    part_in_vec.push_back(buf_tab_part_in_col[4]);
    part_in_vec.push_back(buf_tab_part_in_col[5]);
    part_in_vec.push_back(buf_tab_part_in_col[6]);
    part_in_vec.push_back(buf_meta_part_in);
    part_in_vec.push_back(buf_cfg_part);

    // partition d2h
    std::vector<cl_mem> part_out_vec;
    part_out_vec.push_back(buf_tab_part_out_col[0]);
    part_out_vec.push_back(buf_tab_part_out_col[1]);
    part_out_vec.push_back(buf_tab_part_out_col[2]);
    part_out_vec.push_back(buf_tab_part_out_col[3]);
    part_out_vec.push_back(buf_tab_part_out_col[4]);
    part_out_vec.push_back(buf_tab_part_out_col[5]);
    part_out_vec.push_back(buf_tab_part_out_col[6]);
    part_out_vec.push_back(buf_meta_part_out);
    clEnqueueMigrateMemObjects(cmq, 1, &buf_meta_part_out, 0, 0, nullptr, nullptr);

    clEnqueueMigrateMemObjects(cmq, part_in_vec.size(), part_in_vec.data(), CL_MIGRATE_MEM_OBJECT_CONTENT_UNDEFINED, 0,
                               nullptr, nullptr);
    clEnqueueMigrateMemObjects(cmq, part_out_vec.size(), part_out_vec.data(), CL_MIGRATE_MEM_OBJECT_CONTENT_UNDEFINED,
                               0, nullptr, nullptr);

    cl_event evt_part_h2d, evt_part_krn, evt_part_d2h;

    std::vector<cl_event> evt_part_h2d_dep;
    std::vector<cl_event> evt_part_krn_dep;
    std::vector<cl_event> evt_part_d2h_dep;

    timeval tv_part_start, tv_part_end;
    gettimeofday(&tv_part_start, 0);
    clEnqueueMigrateMemObjects(cmq, part_in_vec.size(), part_in_vec.data(), 0, 0, nullptr, &evt_part_h2d);
    clEnqueueTask(cmq, partkernel, 1, &evt_part_h2d, &evt_part_krn);
    clEnqueueMigrateMemObjects(cmq, part_out_vec.size(), part_out_vec.data(), 1, 1, &evt_part_krn, &evt_part_d2h);

    clFlush(cmq);
    clFinish(cmq);
    gettimeofday(&tv_part_end, 0);
    std::cout << "Checking result...." << std::endl;
    hls::stream<ap_uint<64> > key_in;
    hls::stream<ap_uint<64> > key_out;
    std::map<int, int> key_part_map;
    int* nrow_per_part = meta_part_out.getPartLen();
    for (int i = 0; i < partition_num; i++) {
        int offset = tab_part_out_col_eachpart_nrow_512 * i;
        int prow = nrow_per_part[i];
        std::cout << "Part " << i << " nrow: " << prow << std::endl;
        const int nread = (prow + 15) / 16;
        int abc = 0;
        ap_uint<HASHWH + HASHWL> golden = 0;
        for (int n = 0; n < nread; n++) {
            const int len = (abc + VEC_LEN) > prow ? (prow - abc) : VEC_LEN;
            for (int m = 0; m < len; m++) {
                ap_uint<32> key = tab_part_out_col[0][offset + n](m * 32 + 31, m * 32);
                // pre-check whether same key are in the same partition
                if (key_part_map.find(key) != key_part_map.end()) {
                    if (i != key_part_map[key]) {
                        std::cout << "Find Error, Error key is " << key << std::endl;
                    }
                } else {
                    key_part_map.insert(std::make_pair(key, i));
                }
                ap_uint<64> k1 = (ap_uint<32>(0), key);
                key_in.write(k1);
                // check if the valid bits of key hash are in the same partition
                xf::database::hashLookup3<64>(13, key_in, key_out);
                ap_uint<64> k_g = key_out.read();
                ap_uint<log_partition_num> key_out_hashpart = 0;
                int low_part_nm = log_partition_num - 2;
                // the actually used hash bits for different partition in each PU: (log_part-2, 0)
                key_out_hashpart(low_part_nm - 1, 0) = k_g(low_part_nm - 1, 0);
                // 4 PUs used in partition kernel. The hash (HASHWH+HASHWL-1, HASHWL) bits
                // are used for dispatching to different PUs.
                key_out_hashpart(low_part_nm + 1, low_part_nm) = k_g(HASHWH + HASHWL - 1, HASHWL);
                if ((n == 0) && (m == 0)) {
                    golden = key_out_hashpart;
                }
                if (golden != key_out_hashpart) {
                    std::cout << "Error in " << i << ", " << n << ", " << m << std::endl;
                    return 1;
                }
            }
            abc += len;
        }
    }
    std::cout << "All partitions are checked.\nPASS!" << std::endl;

    return 0;
}
