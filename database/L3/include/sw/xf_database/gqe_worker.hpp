/*
 * Copyright 2020 Xilinx, Inc.
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

#ifndef GQE_WORKER_HPP
#define GQE_WORKER_HPP
#include <CL/cl_ext_xilinx.h>
#include <CL/cl.h>

#include <vector>
#include <string>
#include <cstring>
#include <iostream>
#include <cstdlib>
#include <cstdio>
#include <queue>
#include <thread>
#include <chrono>
#include <atomic>
#include <condition_variable>

#include "xf_database/gqe_utils.hpp"
#include "xf_database/gqe_memcpy.hpp"
#include <ap_int.h>

namespace xf {
namespace database {
namespace gqe {

using namespace std;

enum WorkerFunctions { DUMMY, JOIN, AGGR }; // used to identy xclbins

struct migrate_task {
    WorkerFunctions func;
    size_t k_id;
    size_t p_id;

    vector<size_t>* arg_buf_idx;       //[columns]
    vector<vector<size_t> >* arg_head; //[part][column]
    vector<vector<size_t> >* arg_size; //[part][column]
    vector<size_t>* res_row;           //[part]

    cl_event* evt_wait_list;
    size_t evt_wait_num;
    cl_event* evt;
};

class Worker {
   private:
    void setSubBufDim(size_t kernel_num, size_t ping_pong_num, vector<size_t> arg_nums);
    void resetAccBufSize();
    void createNoneOverLap(size_t k_idx, size_t pp_idx, size_t buf_arg_idx, size_t buf_idx, size_t size_needed);
    void dupSubBuf(size_t sk, size_t sp, size_t sbuf_arg, size_t dk, size_t dp, size_t dbuf_arg);
    void migrate_func();
    size_t join_partlen(ap_uint<512>* metaData, size_t part_idx);
    size_t join_reslen(ap_uint<512>* metaData);

   public:
    cl_int err;
    cl_context ctx; // do not need to be released by Worker
    cl_command_queue cq;
    cl_program prg;

    vector<char*> h_buf;         // raw pinned buffer
    vector<cl_mem> d_buf;        // raw device buffer
    vector<size_t> max_buf_size; // max buff size allowed
    vector<size_t> acc_buf_size; // buff size already allocated

    vector<vector<cl_kernel> > krn;                      // [kernels][pingpong]
    vector<vector<vector<size_t> > > sub_buf_head;       //[kernel][pingpong][arg]
    vector<vector<vector<size_t> > > sub_buf_size;       //[kernel][pingpong][arg]
    vector<vector<vector<cl_mem> > > sub_buf_parent;     //[kernel][pingpong][arg]
    vector<vector<vector<cl_mem> > > sub_buf;            //[kernel][pingpong][arg]
    vector<vector<vector<char*> > > sub_buf_host_parent; //[kernel][pingpong][arg]

    thread migrate_t;
    mutex m;
    condition_variable cv;
    bool migrate_run;
    SafeQueue<migrate_task> q;

   public:
    // funciton
    Worker& operator=(const Worker&) = delete;
    void start();
    Worker(cl_context context, cl_device_id device_id, string xclbin_path, WorkerFunctions func_needed);
    Worker(const Worker& worker);
    void release();
    void print();

    void runKernel(WorkerFunctions func,
                   size_t kernel_id,
                   size_t pingpong_id,
                   vector<size_t> scalar_arg,
                   cl_event* evt_wait_list,
                   size_t evt_wait_num,
                   cl_event* evt);

    void MigrateToDevice(WorkerFunctions func,
                         size_t kernel_id,
                         size_t pingpong_id,
                         vector<size_t>* arg_buf_idx,
                         vector<size_t>* arg_size, // assume arg_head are all 0
                         cl_event* evt_wait_list,
                         size_t evt_wait_num,
                         cl_event* evt);

    void MigrateMetaToHost(WorkerFunctions func,
                           size_t kernel_id,
                           size_t pingpong_id,
                           cl_event* evt_wait_list,
                           size_t evt_wait_num,
                           cl_event* evt);

    void MigrateResToHost(WorkerFunctions func,
                          size_t kernel_id,
                          size_t pingpong_id,
                          vector<size_t>* arg_buf_idx,       //[columns]
                          vector<vector<size_t> >* arg_head, //[part][column]
                          vector<vector<size_t> >* arg_size, //[part][column]
                          vector<size_t>* res_row,           //[part]
                          cl_event* evt_wait_list,
                          size_t evt_wait_num,
                          cl_event* evt); // evt is user event
};

} // gqe
} // database
} // xf
#endif // GQE_WORKER_HPP
