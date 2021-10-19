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

#ifndef GQE_MEMCPY_HPP
#define GQE_MEMCPY_HPP
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

namespace xf {
namespace database {
namespace gqe {

using namespace std;

template <typename T>
class SafeQueue {
   private:
    queue<T> _q;
    mutex _m;

   public:
    SafeQueue() {}
    ~SafeQueue() {}

    bool empty() {
        unique_lock<mutex> lk(_m);
        return _q.empty();
    }

    T& front() {
        unique_lock<mutex> lk(_m);
        return _q.front();
    }

    void pop() {
        unique_lock<mutex> lk(_m);
        _q.pop();
    }

    void push(const T& val) {
        unique_lock<mutex> lk(_m);
        _q.push(val);
    }

    size_t size() {
        unique_lock<mutex> lk(_m);
        return _q.size();
    }
};

struct memcpy_task {
    cl_event* evt_wait_list;
    size_t evt_wait_num;
    cl_event* evt_copy;
    vector<vector<char*> >* src;
    vector<vector<char*> >* dst;
    vector<vector<size_t> >* bias;
    vector<vector<size_t> >* length;
    vector<vector<size_t> >* acc;
};

class MemCoppier {
   private:
    const static size_t T_N = 1;
    void z_func();
    void n_func(const size_t i);

   public:
    thread cpy_t[T_N];
    mutex m[T_N];
    condition_variable cv[T_N];
    bool z_run;
    bool n_run;
    SafeQueue<memcpy_task> q[T_N];

   public:
    MemCoppier& operator=(const MemCoppier& st) = delete;
    MemCoppier(){};
    MemCoppier(const MemCoppier& obj){};

    void start();

    void release();
    void addTask(cl_event* evt_wait_list,
                 size_t evt_wait_num,
                 cl_event* evt_copy, // user event
                 vector<vector<char*> >* src,
                 vector<vector<char*> >* dst,
                 vector<vector<size_t> >* bias,
                 vector<vector<size_t> >* length,
                 vector<vector<size_t> >* acc);
};

} // gqe
} // database
} // xf

#endif // GQE_WORKSHOP_HPP
