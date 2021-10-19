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

#ifndef GQE_CLIENT_HPP
#define GQE_CLIENT_HPP
#include <CL/cl_ext_xilinx.h>
#include <CL/cl.h>

#include <vector>
#include <queue>
#include <thread>
#include <chrono>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include <string>
#include <cstring>
#include <iostream>

#include "xf_database/gqe_worker.hpp"
#include "xf_database/gqe_workshop.hpp"

namespace xf {
namespace database {
namespace gqe {

using namespace std;

class Client {
   public:
    cl_int err;
    cl_context ctx;
    vector<Worker> worker;

    Client(const Workshop& workshop);
    Client(const Client&) = delete;
    void print();
};

Client::Client(const Workshop& workshop) : err(workshop.err), ctx(workshop.ctx), worker(workshop.worker) {}

void Client::print() {
    cout << worker.size() << " workers in total" << endl;
    for (size_t i = 0; i < worker.size(); i++) {
        cout << "worker[" << i << "]:" << endl;
        worker[i].print();
    }
}

} // gqe
} // database
} // xf
#endif // GQE_WORKSHOP_HPP
