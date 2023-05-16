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
#ifndef TYPES_HPP
#define TYPES_HPP
#include <condition_variable>
#include <mutex>
#include <atomic>
// a shared/threads safe queue
template <typename T>
class squeue {
   public:
    squeue() : d_buf(nullptr), d_write(0), d_read(0), d_sz(0) {}
    void init_size(int sz, int apf) {
        d_read = 0;
        d_write = 0;
        d_sz = sz + 1;
        delete[] d_buf;
        d_buf = new T[d_sz];
        d_apf = apf;
    }
    void put(T t) {
        std::unique_lock<std::mutex> wlock(d_write_mutex);
        if (full()) {
            do {
                d_full_cond.wait(wlock);
            } while (full());
        }
        if (empty()) {
            std::unique_lock<std::mutex> rlock(d_read_mutex);
            d_buf[d_write] = t;
            d_write = next(d_write);
            d_empty_cond.notify_one();
        } else {
            d_buf[d_write] = t;
            d_write = next(d_write);
        }
    }
    bool full() { return next(d_write) == d_read; }
    T get() {
        std::unique_lock<std::mutex> rlock(d_read_mutex);
        if (empty()) {
            do {
                d_empty_cond.wait(rlock);
            } while (empty());
        }
        if (full()) {
            std::unique_lock<std::mutex> wlock(d_write_mutex);
            T t = d_buf[d_read];
            d_read = next(d_read);
            d_full_cond.notify_one();
            return t;
        }
        T t = d_buf[d_read];
        d_read = next(d_read);
        return t;
    }
    bool empty() { return d_read == d_write; }
    void clear() {
        d_write = 0;
        d_read = 0;
    }
    int count() {
        int cnt = d_write - d_read;
        if (cnt < 0) cnt += d_sz;
        return cnt;
    }
    void print() {
        printf("<=");
        for (int i = d_read; i != d_write; i = next(i)) {
            printf(" %d", d_buf[i]);
        }
        printf(" <= (%d empty slots)\n", d_sz - 1 - count());
    }

   private:
    int next(int p) { return (p + 1) % d_sz; }

    T* d_buf;
    int d_write; // only written by producer
    int d_read;  // only written by consumer
    int d_sz;
    std::mutex d_write_mutex;
    std::mutex d_read_mutex;
    std::condition_variable d_full_cond;
    std::condition_variable d_empty_cond;
    int d_apf;

}; // class squeue

#endif