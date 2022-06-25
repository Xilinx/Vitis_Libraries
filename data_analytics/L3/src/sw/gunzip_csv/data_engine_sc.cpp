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

#include <unordered_map>
#include "data_engine_sc.hpp"
#include <fcntl.h>
#include <cstdio>
#include <cstdint>
#include <unistd.h>
#include <sstream>
#include <iostream>
#include <sys/time.h>
#include <fstream>

#define CHUNK_SZ 48 * 1024 * 1024

namespace sssd_engine {
namespace data_engine_sc {
// init device
// load xclbin
DataEngine::DataEngine(int t_id,
                       VPP_BP& _csvInBufPool,
                       VPP_BP& _cfgInBufPool,
                       VPP_BP& _outBufPool,
                       VPP_BP& _metaBufPool,
                       VPP_CC* _cuCluster)
    : cuCluster(_cuCluster),
      csvInBufPool(_csvInBufPool),
      cfgInBufPool(_cfgInBufPool),
      outBufPool(_outBufPool),
      metaBufPool(_metaBufPool) {
    const char* log_path = getenv("SSSD_ENGINE_LOG_FILE");
    log_ptr = NULL;
    if (log_path) {
        std::string path(log_path);
        path = path + "/log_" + std::to_string(t_id);
        log_ptr = fopen(path.c_str(), "a");
        if (log_ptr == NULL) {
            fprintf(stderr, "ERROR: SSSD_ENGINE_LOG_FILE %s cannot be opened\n", path);
        }
    } else {
        fprintf(stderr, "Warning: please set SSSD_ENGINE_LOG_FILE as log path\n");
    }
    card_id = t_id;
    printf("add_card %d\n", t_id);

    sleep(1);
    tokens.init_size(2 * 3 * N, 0);
    for (int i = 0; i < 2 * 3 * N; ++i) {
        char* buf = new char[CHUNK_SZ];
        memset(buf, 0, CHUNK_SZ);
        tokens.put(buf);
    }
    run_t = std::thread(&DataEngine::run, this);
    dummy_cfg = new uint64_t[19];
    memset(dummy_cfg, 0, 19 * sizeof(uint64_t));
}

// release program, command queue, context and device
DataEngine::~DataEngine() {
    QueueRequest q;

    q.valid = false;
    {
        std::unique_lock<std::mutex> lock(p_mutex);
        run_q.push_back(std::move(q));
        lock.unlock();
        p_cond.notify_one();
    }
    run_t.join();
    for (int i = 0; i < 2 * 3 * N; ++i) {
        char* buf = tokens.get();
        delete[] buf;
    }
    if (log_ptr != NULL) fflush(log_ptr);
}
// Run
void DataEngine::run() {
    auto send_fn = [=]() -> bool {
        bool last = false;
        std::vector<std::string> file_list;
        std::vector<uint32_t> file_sz;
        uint64_t* cfg;
        std::unique_lock<std::mutex> lock(p_mutex);
        while (run_q.empty()) {
            p_cond.wait(lock);
        }
        printf("send get lock p_mutex\n");
        int file_nm = 0;
        int cnt = run_q.size();
        if (cnt > N)
            file_nm = N;
        else
            file_nm = cnt;

        std::unique_lock<std::mutex> lock_e(e_mutex);
        printf("send get lock e_mutex\n");
        for (int i = 0; i < file_nm; ++i) {
            QueueRequest q = std::move(run_q.front());
            q.file_nm = file_nm - i - 1;
            file_list.push_back(q.file_path);
            file_sz.push_back(q.size);
            last = q.valid;
            cfg = q.cfg_buf;

            run_q.pop_front();
            ex_q.push_back(std::move(q));
        }
        lock.unlock();
        lock_e.unlock();
        e_cond.notify_one();
        if (!last) return 0;
        printf("Get request\n");
        if (log_ptr != NULL) {
            time_t tm = time(NULL);
            fprintf(log_ptr, "%s: card[%d] send_while got %d files scan requet\n", std::ctime(&tm), card_id, file_nm);
        }

        ap_uint<64>* hbuf_meta =
            data_engine_acc::alloc_buf<ap_uint<64> >(metaBufPool, DDR_SIZE_META_LWORD * sizeof(ap_uint<64>));
        hbuf_meta[0] = N;
        hbuf_meta[3 * N + 1] = file_nm;
        uint32_t in_offt = 0;
        uint32_t out_offt = 0;
        uint32_t buf_offt = 0;
        ap_uint<128>* hbuf_in;
        FileDescriptors* s_r_handle = new FileDescriptors;
        int* fd_collect = new int[file_nm];
        int* buf_sz_collect = new int[file_nm];
        for (int i = 0; i < file_nm; ++i) {
#ifdef USE_P2P
            // p2p data transfer size aligned to 4K
            int fd = open(file_list[i].c_str(), O_RDONLY | O_DIRECT);
#else
            int fd = open(file_list[i].c_str(), O_RDONLY);
#endif

            if (fd == -1) {
                fprintf(stderr, "ERROR: Cannot open the input file!!\n");
            }
            fd_collect[i] = fd;
            // limitation: input file size less than 1GB
            uint64_t sz = file_sz[i];
            hbuf_meta[1 + i] = sz << 32;
            hbuf_meta[1 + i] += in_offt; // file size
            uint64_t out_sz = CHUNK_SZ;  //(double)sz * pass_ratio; // 20000 * 4 * 8;
            hbuf_meta[N + 1 + i] = out_sz << 32;
            hbuf_meta[N + 1 + i] += out_offt; // file size
            // align to 4K
            int csv_buf_sz = (sz + 4095) / 4096 * 4096;
            buf_sz_collect[i] = csv_buf_sz;
            in_offt += csv_buf_sz / 16;
            out_offt += (out_sz + 31) / 32;

#ifdef USE_P2P
            hbuf_in = (ap_uint<128>*)data_engine_acc::file_buf(csvInBufPool, fd, csv_buf_sz, 0, buf_offt);
#endif
            buf_offt += csv_buf_sz;
            last_file = file_list[i];
        }
#ifndef USE_P2P
        hbuf_in = (ap_uint<128>*)data_engine_acc::alloc_buf(csvInBufPool, buf_offt);
        uint8_t* hbuf_in_i8 = reinterpret_cast<uint8_t*>(hbuf_in);
        buf_offt = 0;
        for (int i = 0; i < file_nm; i++) {
            if (pread(fd_collect[i], hbuf_in_i8 + buf_offt, buf_sz_collect[i], 0) == -1) {
                fprintf(stderr, "ERROR: File reading failed.\n");
            }
            buf_offt += buf_sz_collect[i];
        }
#endif
        s_r_handle->fd = fd_collect;
        data_engine_acc::set_handle(int64_t(s_r_handle));
        for (int i = file_nm; i < N; ++i) {
            hbuf_meta[1 + i] = 0;     // file size
            hbuf_meta[N + 1 + i] = 0; // file size
        }
        ap_uint<256>* hbuf_out = data_engine_acc::alloc_buf<ap_uint<256> >(outBufPool, out_offt);
        ap_uint<64>* hbuf_cfg = data_engine_acc::alloc_buf<ap_uint<64> >(cfgInBufPool, DDR_SIZE_CFG_LWORD);

        memcpy(hbuf_cfg, cfg, sizeof(ap_uint<64>) * DDR_SIZE_CFG_LWORD);
        data_engine_acc::custom_sync_outputs([=]() {
            auto fut = data_engine_acc::sync_output<ap_uint<64> >(hbuf_meta, DDR_SIZE_META_LWORD, 0);
            fut.get();
            for (int chunk = 0; chunk < N; ++chunk) {
                uint32_t size_in_byte = hbuf_meta[2 * N + 1 + chunk];
                if (size_in_byte > 0) {
                    int out_sz = (size_in_byte + 31) / 32;
                    int offt = chunk * CHUNK_SZ / 32;
                    data_engine_acc::sync_output<ap_uint<256> >(hbuf_out, out_sz, offt);
                }
            }
        });
        data_engine_acc::compute(hbuf_in, hbuf_out, hbuf_meta, hbuf_cfg);
        return 1;
    };
    auto recv_fn = [=]() {
        int64_t recv_handle_addr = data_engine_acc::get_handle();
        FileDescriptors* s_r_handle = (FileDescriptors*)recv_handle_addr;
        // buffer for cfg
        // memcpy cfg
        ap_uint<256>* hbuf_out = (ap_uint<256>*)data_engine_acc::get_buf(outBufPool);
        ap_uint<64>* hbuf_meta = (ap_uint<64>*)data_engine_acc::get_buf(metaBufPool);
        printf("Finish one task\n");
        if (log_ptr != NULL) {
            time_t tm = time(NULL);
            fprintf(log_ptr, "%s: card[%d] receive_all_in_order finised scan tasks\n", std::ctime(&tm), card_id);
        }
        uint32_t in_offt = 0;
        uint32_t out_offt = 0;
        int ovf = 0;
        int file_nm = 1;
        int i = 0;
        while (file_nm > 0) {
            if (close(s_r_handle->fd[i]) != 0) {
                std::perror("Close file: ");
            }
            std::unique_lock<std::mutex> lock(e_mutex);
            while (ex_q.empty()) {
                e_cond.wait(lock);
            }
            QueueRequest q = std::move(ex_q.front());
            ex_q.pop_front();
            lock.unlock();
            printf("receive get lock e_mutex\n");
            file_nm = q.file_nm;

            if (q.valid) {
                // check overflow
                uint64_t sz = q.size;
                if (hbuf_meta[1 + i].range(63, 32) != sz) ovf++;
                if (hbuf_meta[1 + i].range(31, 0) != in_offt) ovf++; // file size
                uint64_t out_sz = CHUNK_SZ; //(double)sz * pass_ratio;           // 20000 * 4 * 8;
                if (hbuf_meta[N + 1 + i].range(63, 32) != out_sz) ovf++;
                if (hbuf_meta[N + 1 + i].range(31, 0) != out_offt) ovf++; // file size
                // align to 4K
                int csv_buf_sz = (sz + 4095) / 4096 * 4096;
                in_offt += csv_buf_sz / 16;
                out_offt += (out_sz + 31) / 32;
                if (ovf == 0) {
                    uint32_t size_in_byte = hbuf_meta[2 * N + 1 + i];
                    uint32_t offt = hbuf_meta[N + 1 + i].range(31, 0);

                    char* out_p = tokens.get();

                    memcpy(out_p, &size_in_byte, 4);
                    memcpy(out_p + sizeof(int32_t), hbuf_out + offt, size_in_byte);
                    RetObj ret;
                    ret.data = out_p;
                    ret.status = SUCCESS;
                    ret.size = size_in_byte;
                    q.p.set_value(std::move(ret));
                } else {
                    RetObj ret;
                    ret.status = CFG_ERR;
                    ret.size = 0;
                    q.p.set_value(std::move(ret));
                }
            }
            i++;
        }
        if (ovf > 0) {
            fprintf(stderr, "please setting SSSD_BUFFER_STRATEGY (default 2) with low level\n");
        }
    };
    data_engine_acc::send_while([=]() -> bool { return send_fn(); }, *cuCluster);
    data_engine_acc::receive_all_in_order([=]() { recv_fn(); }, *cuCluster);
    data_engine_acc::join(*cuCluster);
}
// push request to queue
void DataEngine::pushRequest(std::promise<RetObj> prom, std::string file_path, size_t size, uint64_t* cfg) {
    QueueRequest q;
    q.file_path = file_path;
    q.size = size;
    q.p = std::move(prom);
    q.cfg_buf = cfg;
    q.valid = true;
    std::unique_lock<std::mutex> lock(p_mutex);
    run_q.push_back(std::move(q));
    lock.unlock();
    p_cond.notify_one();

    if (log_ptr != NULL) {
        time_t tm = time(NULL);
        fprintf(log_ptr, "%s: card[%d] push one file scan requet\n", std::ctime(&tm), card_id);
    }
}
} // end of namespace data_engine_sc
} // namespace sssd_engine
