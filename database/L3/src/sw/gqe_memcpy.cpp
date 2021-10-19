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

#include "xf_database/gqe_memcpy.hpp"

namespace xf {
namespace database {
namespace gqe {

using namespace std;

void MemCoppier::z_func() {
    const size_t id = 0;
    // init other thread
    n_run = true;
    for (size_t i = 1; i < T_N; i++) {
        cpy_t[i] = thread(&MemCoppier::n_func, this, i);
    }
    // main work loop
    while (z_run) {
        {
            unique_lock<mutex> lk(m[0]);
            cv[0].wait(lk, [&] { return (!q[0].empty() || !z_run); });
        }

        while (!q[0].empty()) {
            memcpy_task tsk = q[0].front();
            clWaitForEvents(tsk.evt_wait_num, tsk.evt_wait_list);

            for (size_t i = 1; i < T_N; i++) {
                lock_guard<mutex> lk(m[i]);
                q[i].push(tsk);
                cv[i].notify_all();
            }

            size_t p_num = tsk.src->size();
            for (size_t i = 0; i < p_num; i++) {
                size_t c_num = (*tsk.src)[i].size();
                for (size_t j = 0; j < c_num; j++) {
                    size_t d_len = ((*tsk.length)[i][j] + (T_N - 1)) / T_N;
                    size_t p_bias = d_len * id;

                    void* destination = (*tsk.dst)[i][j] + (*tsk.bias)[i][j] + p_bias;
                    void* source = (*tsk.src)[i][j] + p_bias;
                    size_t num;
                    if (id == T_N - 1) {
                        num = (*tsk.length)[i][j] - p_bias;
                    } else {
                        num = d_len;
                    }

                    memcpy(destination, source, num);
                    (*tsk.acc)[i][j] = (*tsk.bias)[i][j] + (*tsk.length)[i][j];
                }
            }

            for (size_t i = 1; i < T_N; i++) {
                unique_lock<mutex> lk(m[i]);
                cv[i].wait(lk, [&] { return q[i].empty(); });
            }
            clSetUserEventStatus(*(tsk.evt_copy), CL_COMPLETE);
            q[0].pop();
        }
    }
    // clean up q[0]'s left task
    while (!q[0].empty()) {
        memcpy_task tsk = q[0].front();
        clWaitForEvents(tsk.evt_wait_num, tsk.evt_wait_list);

        for (size_t i = 1; i < T_N; i++) {
            lock_guard<mutex> lk(m[i]);
            q[i].push(tsk);
            cv[i].notify_all();
        }

        size_t p_num = tsk.src->size();
        for (size_t i = 0; i < p_num; i++) {
            size_t c_num = (*tsk.src)[i].size();
            for (size_t j = 0; j < c_num; j++) {
                size_t d_len = ((*tsk.length)[i][j] + (T_N - 1)) / T_N;
                size_t p_bias = d_len * id;

                void* destination = (*tsk.dst)[i][j] + (*tsk.bias)[i][j] + p_bias;
                void* source = (*tsk.src)[i][j] + p_bias;
                size_t num;
                if (id == T_N - 1) {
                    num = (*tsk.length)[i][j] - p_bias;
                } else {
                    num = d_len;
                }

                memcpy(destination, source, num);
                (*tsk.acc)[i][j] = (*tsk.bias)[i][j] + (*tsk.length)[i][j];
            }
        }

        for (size_t i = 1; i < T_N; i++) {
            unique_lock<mutex> lk(m[i]);
            cv[i].wait(lk, [&] { return q[i].empty(); });
        }
        clSetUserEventStatus(*(tsk.evt_copy), CL_COMPLETE);
        q[0].pop();
    }

    // join other thread
    n_run = false;
    for (size_t i = 1; i < T_N; i++) {
        {
            lock_guard<mutex> lk(m[i]);
            cv[i].notify_all();
        }

        if (cpy_t[i].joinable()) {
            cpy_t[i].join();
        } else {
            cout << "thread [" << i << "] of MemCoppier is not joinable!" << endl;
        }
    }
}

void MemCoppier::n_func(const size_t id) {
    // main work loop
    while (n_run) {
        // wait notification to check q[id]
        {
            unique_lock<mutex> lk(m[id]);
            cv[id].wait(lk, [&] { return !q[id].empty() || !n_run; });
        }
        // check if there's task in q[id] to process
        while (!q[id].empty()) {
            memcpy_task tsk = q[id].front();

            size_t p_num = tsk.src->size();
            for (size_t i = 0; i < p_num; i++) {
                size_t c_num = (*tsk.src)[i].size();
                for (size_t j = 0; j < c_num; j++) {
                    size_t d_len = ((*tsk.length)[i][j] + (T_N - 1)) / T_N;
                    size_t p_bias = d_len * id;

                    void* destination = (*tsk.dst)[i][j] + (*tsk.bias)[i][j] + p_bias;
                    void* source = (*tsk.src)[i][j] + p_bias;
                    size_t num;
                    if (id == T_N - 1) {
                        num = (*tsk.length)[i][j] - p_bias;
                    } else {
                        num = d_len;
                    }

                    memcpy(destination, source, num);
                }
            }

            {
                lock_guard<mutex> lk(m[id]);
                q[id].pop();
                cv[id].notify_all();
            }
        }
    }
}

void MemCoppier::start() {
    z_run = true;
    cpy_t[0] = thread(&MemCoppier::z_func, this);
}

void MemCoppier::release() {
    // wait until q[0] is empty
    while (!q[0].empty()) {
        this_thread::sleep_for(std::chrono::milliseconds(1));
        lock_guard<mutex> lk(m[0]);
        cv[0].notify_all();
    }

    // notify thread[0] to end
    {
        lock_guard<mutex> lk(m[0]);
        z_run = false;
        cv[0].notify_all();
    }

    // join thread[0]
    if (cpy_t[0].joinable()) {
        cpy_t[0].join();
    } else {
        cout << "thread [0] of MemCoppier is not joinable!" << endl;
    }
}

void MemCoppier::addTask(cl_event* evt_wait_list,
                         size_t evt_wait_num,
                         cl_event* evt_copy,
                         vector<vector<char*> >* src,
                         vector<vector<char*> >* dst,
                         vector<vector<size_t> >* bias,
                         vector<vector<size_t> >* length,
                         vector<vector<size_t> >* acc) {
    memcpy_task tsk;
    tsk.evt_wait_list = evt_wait_list;
    tsk.evt_wait_num = evt_wait_num;
    tsk.evt_copy = evt_copy;
    tsk.src = src;
    tsk.dst = dst;
    tsk.bias = bias;
    tsk.length = length;
    tsk.acc = acc;

    {
        lock_guard<mutex> lk(m[0]);
        q[0].push(tsk);
        cv[0].notify_all();
    }
}

} // gqe
} // database
} // xf
