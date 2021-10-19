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

#ifndef GQE_WORKSHOP_HPP
#define GQE_WORKSHOP_HPP
#include <CL/cl_ext_xilinx.h>
#include <CL/cl.h>

#include <vector>
#include <string>
#include <cstring>
#include <iostream>
#include <thread>
#include <atomic>
#include <iomanip>
#include <algorithm>
#include <future>
#include <mutex>
#include <condition_variable>
#include <unistd.h>
#include <ap_int.h>
#include "xf_database/meta_table.hpp"
#include "xf_database/gqe_input.hpp"
#include "xf_database/gqe_join_strategy.hpp"
#include "xf_database/gqe_join_config.hpp"
#include "xf_database/gqe_partjoin_config.hpp"
#include "xf_database/gqe_platform.hpp"
#include "xf_database/gqe_worker.hpp"
#include "xf_database/gqe_memcpy.hpp"

namespace xf {
namespace database {
namespace gqe {

using namespace std;

struct join_task {
    TableSection* tab_a;
    string filter_a;
    vector<future<size_t> >* tab_a_sec_ready;
    TableSection* tab_part_a;
    TableSection* tab_b;
    string filter_b;
    vector<future<size_t> >* tab_b_sec_ready;
    TableSection* tab_part_b;
    string join_str;
    string output_str;
    TableSection* tab_c;
    vector<promise<size_t> >* tab_c_sec_ready_promise;
    int join_type;
    JoinStrategyBase* strategyimp;
};

class Workshop : public PlatformInit {
   private:
    void processJoin(join_task tsk);
    void checkJoinQueue();

   public:
    // data
    cl_int err;
    cl_context ctx;
    vector<Worker> worker;
    MemCoppier h2p;
    MemCoppier p2h;

    thread join_service_t;
    bool join_service_run;
    SafeQueue<join_task> q;

    mutex m;
    condition_variable cv;

   public:
    Workshop(const Workshop& obj) = delete;
    Workshop& operator=(const Workshop& st) = delete;

    Workshop(string device_shell_name, string xclbin_path, WorkerFunctions func);
    ~Workshop();
    void release();
    void print();
    void Join(TableSection* tab_a,
              string filter_a,
              vector<future<size_t> >* tab_a_sec_ready,
              TableSection* tab_part_a,
              TableSection* tab_b,
              string filter_b,
              vector<future<size_t> >* tab_b_sec_ready,
              TableSection* tab_part_b,
              string join_str,
              string output_str,
              TableSection* tab_c,
              vector<promise<size_t> >* tab_c_sec_ready_promise,
              int join_type,
              JoinStrategyBase* strategyimp);
};
} // gqe
} // database
} // xf
#endif // GQE_WORKSHOP_HPP
