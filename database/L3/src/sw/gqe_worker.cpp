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

#include "xf_database/gqe_worker.hpp"

namespace xf {
namespace database {
namespace gqe {
namespace internal {

static unsigned long read_binary_file(const char* fname, void** buffer) {
    unsigned long size = 0;
    FILE* fp = fopen(fname, "rb");
    if (!fp) {
        fprintf(stderr, "File %s cannot be opened for read.\n", fname);
    }
    fseek(fp, 0, SEEK_END);
    size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    *buffer = (void*)malloc(size);
    fread(*buffer, 1, size, fp);
    fclose(fp);
    return size;
}

template <typename T>
T* aligned_alloc(size_t num) {
    void* ptr = nullptr;
    size_t sz = num * sizeof(T);
    if (posix_memalign(&ptr, 4096, sz)) throw std::bad_alloc();
    return reinterpret_cast<T*>(ptr);
}

} // internal

using namespace std;

size_t Worker::join_partlen(ap_uint<512>* metaData, size_t part_idx) {
    size_t r_id = part_idx / 16;
    size_t r_id_ex = part_idx % 16;
    size_t res = metaData[8 + r_id].range(32 * r_id_ex + 31, 32 * r_id_ex);
    return res;
}

size_t Worker::join_reslen(ap_uint<512>* metaData) {
    size_t res = metaData[0].range(71, 8);
    return res;
}

void Worker::setSubBufDim(size_t kernel_num, size_t ping_pong_num, vector<size_t> arg_nums) {
    if (kernel_num != arg_nums.size()) {
        std::cout << "arg_num vector size does not match kernel num" << std::endl;
    }

    sub_buf_head.clear();
    sub_buf_size.clear();
    sub_buf_parent.clear();
    sub_buf.clear();
    sub_buf_host_parent.clear();

    sub_buf_head.resize(kernel_num);
    sub_buf_size.resize(kernel_num);
    sub_buf_parent.resize(kernel_num);
    sub_buf.resize(kernel_num);
    sub_buf_host_parent.resize(kernel_num);

    for (size_t i = 0; i < kernel_num; i++) {
        sub_buf_head[i].resize(ping_pong_num);
        sub_buf_size[i].resize(ping_pong_num);
        sub_buf_parent[i].resize(ping_pong_num);
        sub_buf[i].resize(ping_pong_num);
        sub_buf_host_parent[i].resize(ping_pong_num);
    }

    for (size_t i = 0; i < kernel_num; i++) {
        for (size_t j = 0; j < ping_pong_num; j++) {
            sub_buf_head[i][j].resize(arg_nums[i]);
            sub_buf_size[i][j].resize(arg_nums[i]);
            sub_buf_parent[i][j].resize(arg_nums[i]);
            sub_buf[i][j].resize(arg_nums[i]);
            sub_buf_host_parent[i][j].resize(arg_nums[i]);
        }
    }
}

void Worker::resetAccBufSize() {
    for (size_t i = 0; i < acc_buf_size.size(); i++) {
        acc_buf_size[i] = 0;
    }
}

void Worker::createNoneOverLap(size_t k_idx, size_t pp_idx, size_t buf_arg_idx, size_t buf_idx, size_t size_needed) {
    size_t align_4k_size = (size_needed + 4095) / 4096 * 4096;

    if (acc_buf_size[buf_idx] + align_4k_size > max_buf_size[buf_idx]) {
        std::cout << "Buff size not enough for buffer idx = " << buf_idx << std::endl;
    } else {
        sub_buf_head[k_idx][pp_idx][buf_arg_idx] = acc_buf_size[buf_idx];
        sub_buf_size[k_idx][pp_idx][buf_arg_idx] = align_4k_size;
        sub_buf_parent[k_idx][pp_idx][buf_arg_idx] = d_buf[buf_idx];
        sub_buf_host_parent[k_idx][pp_idx][buf_arg_idx] = h_buf[buf_idx];
        cl_buffer_region tmp_region = {sub_buf_head[k_idx][pp_idx][buf_arg_idx],
                                       sub_buf_size[k_idx][pp_idx][buf_arg_idx]};
        sub_buf[k_idx][pp_idx][buf_arg_idx] = clCreateSubBuffer(
            d_buf[buf_idx], CL_MEM_EXT_PTR_XILINX | CL_MEM_READ_WRITE, CL_BUFFER_CREATE_TYPE_REGION, &tmp_region, &err);
        acc_buf_size[buf_idx] += align_4k_size;
    }
}

void Worker::dupSubBuf(size_t sk, size_t sp, size_t sbuf_arg, size_t dk, size_t dp, size_t dbuf_arg) {
    sub_buf_head[dk][dp][dbuf_arg] = sub_buf_head[sk][sp][sbuf_arg];
    sub_buf_size[dk][dp][dbuf_arg] = sub_buf_size[sk][sp][sbuf_arg];
    sub_buf_parent[dk][dp][dbuf_arg] = sub_buf_parent[sk][sp][sbuf_arg];
    sub_buf_host_parent[dk][dp][dbuf_arg] = sub_buf_host_parent[sk][sp][sbuf_arg];
    sub_buf[dk][dp][dbuf_arg] = sub_buf[sk][sp][sbuf_arg];
}

void Worker::migrate_func() {
    while (migrate_run) {
        {
            unique_lock<mutex> lk(m);
            cv.wait(lk, [&] { return (!q.empty() || !migrate_run); });
        }

        while (!q.empty()) {
            migrate_task tsk = q.front();
            clWaitForEvents(tsk.evt_wait_num, tsk.evt_wait_list);

            // fill in res_row and arg_size based on meta
            if (tsk.func == WorkerFunctions::JOIN) {
                if (tsk.k_id == 1) {
                    ap_uint<512>* meta = (ap_uint<512>*)(sub_buf_host_parent[tsk.k_id][tsk.p_id][6] +
                                                         sub_buf_head[tsk.k_id][tsk.p_id][6]);
                    // part dim loop
                    for (size_t i = 0; i < tsk.res_row->size(); i++) {
                        size_t tmp_row = join_partlen(meta, i);
                        (*tsk.res_row)[i] = tmp_row;

                        // column dim loop
                        for (size_t j = 0; j < (*tsk.arg_size)[i].size(); j++) {
                            // XXX:Fix to 64bit elements
                            (*tsk.arg_size)[i][j] = tmp_row * 8;
                        }
                    }
                } else if (tsk.k_id == 0) {
                    ap_uint<512>* meta = (ap_uint<512>*)(sub_buf_host_parent[tsk.k_id][tsk.p_id][6] +
                                                         sub_buf_head[tsk.k_id][tsk.p_id][6]);
                    // part = 1
                    size_t tmp_row = join_reslen(meta);
                    (*tsk.res_row)[0] = tmp_row;
                    // column dim loop
                    for (size_t j = 0; j < (*tsk.arg_size)[0].size(); j++) {
                        // XXX:Fix to 64bit elements
                        (*tsk.arg_size)[0][j] = tmp_row * 8;
                    }
                }
            }

            // create sub and enqueue migrate
            vector<cl_mem> d2p_mem;
            // part dim
            for (size_t i = 0; i < tsk.res_row->size(); i++) {
                // column dim
                for (size_t j = 0; j < tsk.arg_buf_idx->size(); j++) {
                    size_t arg_id = (*tsk.arg_buf_idx)[j];
                    cl_buffer_region tmp_region = {sub_buf_head[tsk.k_id][tsk.p_id][arg_id] + (*tsk.arg_head)[i][j],
                                                   (*tsk.arg_size)[i][j]};
                    d2p_mem.push_back(clCreateSubBuffer(sub_buf_parent[tsk.k_id][tsk.p_id][arg_id],
                                                        CL_MEM_EXT_PTR_XILINX | CL_MEM_READ_WRITE,
                                                        CL_BUFFER_CREATE_TYPE_REGION, &tmp_region, &err));
                }
            }

            cl_event tmp_mig_evt;
            clEnqueueMigrateMemObjects(cq, d2p_mem.size(), d2p_mem.data(), CL_MIGRATE_MEM_OBJECT_HOST, 0, nullptr,
                                       &tmp_mig_evt);
            clFlush(cq);
            // wait finish and mark user event complete
            clWaitForEvents(1, &tmp_mig_evt);
            clReleaseEvent(tmp_mig_evt);
            clSetUserEventStatus(*(tsk.evt), CL_COMPLETE);
            q.pop();
        }
    }

    // clean up q
    while (!q.empty()) {
        migrate_task tsk = q.front();
        clWaitForEvents(tsk.evt_wait_num, tsk.evt_wait_list);

        // fill in res_row and arg_size based on meta
        if (tsk.func == WorkerFunctions::JOIN) {
            if (tsk.k_id == 1) {
                ap_uint<512>* meta =
                    (ap_uint<512>*)(sub_buf_host_parent[tsk.k_id][tsk.p_id][6] + sub_buf_head[tsk.k_id][tsk.p_id][6]);
                // part dim loop
                for (size_t i = 0; i < tsk.res_row->size(); i++) {
                    size_t tmp_row = join_partlen(meta, i);
                    (*tsk.res_row)[i] = tmp_row;

                    // column dim loop
                    for (size_t j = 0; j < (*tsk.arg_size)[i].size(); j++) {
                        // XXX:Fix to 64bit elements
                        (*tsk.arg_size)[i][j] = tmp_row * 8;
                    }
                }
            } else if (tsk.k_id == 0) {
                ap_uint<512>* meta =
                    (ap_uint<512>*)(sub_buf_host_parent[tsk.k_id][tsk.p_id][6] + sub_buf_head[tsk.k_id][tsk.p_id][6]);
                // part = 1
                size_t tmp_row = join_reslen(meta);
                (*tsk.res_row)[0] = tmp_row;
                // column dim loop
                for (size_t j = 0; j < (*tsk.arg_size)[0].size(); j++) {
                    // XXX:Fix to 64bit elements
                    (*tsk.arg_size)[0][j] = tmp_row * 8;
                }
            }
        }

        // create sub and enqueue migrate
        vector<cl_mem> d2p_mem;
        // part dim
        for (size_t i = 0; i < tsk.res_row->size(); i++) {
            // column dim
            for (size_t j = 0; j < tsk.arg_buf_idx->size(); j++) {
                size_t arg_id = (*tsk.arg_buf_idx)[j];
                cl_buffer_region tmp_region = {sub_buf_head[tsk.k_id][tsk.p_id][arg_id] + (*tsk.arg_head)[i][j],
                                               (*tsk.arg_size)[i][j]};
                d2p_mem.push_back(clCreateSubBuffer(sub_buf_parent[tsk.k_id][tsk.p_id][arg_id],
                                                    CL_MEM_EXT_PTR_XILINX | CL_MEM_READ_WRITE,
                                                    CL_BUFFER_CREATE_TYPE_REGION, &tmp_region, &err));
            }
        }

        cl_event tmp_mig_evt;
        clEnqueueMigrateMemObjects(cq, d2p_mem.size(), d2p_mem.data(), CL_MIGRATE_MEM_OBJECT_HOST, 0, nullptr,
                                   &tmp_mig_evt);
        clFlush(cq);
        // wait finish and mark user event complete
        clWaitForEvents(1, &tmp_mig_evt);
        clReleaseEvent(tmp_mig_evt);
        clSetUserEventStatus(*(tsk.evt), CL_COMPLETE);
        q.pop();
    }
}

/*
struct migrate_task {
    WorkerFunctions func;
    size_t k_id;
    size_t p_id;

    vector<vector<size_t> >* arg_head;
    vector<vector<size_t> >* arg_size;
    vector<size_t>* res_row;
};
*/

void Worker::start() {
    migrate_run = true;
    migrate_t = thread(&Worker::migrate_func, this);
}

Worker::Worker(cl_context context, cl_device_id device_id, string xclbin_path, WorkerFunctions func_needed) {
    // context / command queue / load xclbin
    ctx = context;
    cq = clCreateCommandQueue(ctx, device_id, CL_QUEUE_PROFILING_ENABLE | CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE, &err);
    void* kernel_image = NULL;
    unsigned long size = internal::read_binary_file(xclbin_path.c_str(), &kernel_image);
    prg = clCreateProgramWithBinary(ctx, 1, &device_id, &size, (const unsigned char**)&kernel_image, NULL, &err);
    err = clBuildProgram(prg, 1, &device_id, NULL, NULL, NULL);
    free(kernel_image);

    // hardware specific kernel / buffer initialization
    switch (func_needed) {
        case JOIN:
            // create kernels
            krn.resize(2); //[part,join][ping,pong]
            krn[0].push_back(clCreateKernel(prg, "gqeKernel", &err));
            krn[0].push_back(clCreateKernel(prg, "gqeKernel", &err));
            krn[1].push_back(clCreateKernel(prg, "gqeKernel", &err));
            krn[1].push_back(clCreateKernel(prg, "gqeKernel", &err));

            // max buf size
            max_buf_size.clear();
            max_buf_size.resize(24, (1 << 28)); // 0 - 15 mapped to HBM
            // max_buf_size[16] = 4000000000;      // 16 and 17 mapped to DDR
            // max_buf_size[17] = 4000000000;      // 16 and 17 mapped to DDR
            acc_buf_size.resize(24, 0);
            // create raw pinned buffer
            h_buf.resize(24);
            for (size_t i = 0; i < h_buf.size(); i++) {
                h_buf[i] = internal::aligned_alloc<char>(max_buf_size[i]);
                memset(h_buf[i], 0, max_buf_size[i] * sizeof(char));
            }
            // create raw device buffer
            d_buf.resize(24);
            for (size_t i = 0; i < d_buf.size(); i++) {
                cl_mem_ext_ptr_t mext_d_buf;
                switch (i) {
                    case 0:
                        mext_d_buf = {XCL_MEM_TOPOLOGY | unsigned(0), h_buf[i], 0};
                        break;
                    case 1:
                        mext_d_buf = {XCL_MEM_TOPOLOGY | unsigned(1), h_buf[i], 0};
                        break;
                    case 2:
                        mext_d_buf = {XCL_MEM_TOPOLOGY | unsigned(2), h_buf[i], 0};
                        break;
                    case 3:
                        mext_d_buf = {XCL_MEM_TOPOLOGY | unsigned(3), h_buf[i], 0};
                        break;
                    case 4:
                        mext_d_buf = {XCL_MEM_TOPOLOGY | unsigned(14), h_buf[i], 0};
                        break;
                    case 5:
                        mext_d_buf = {XCL_MEM_TOPOLOGY | unsigned(15), h_buf[i], 0};
                        break;
                    case 6:
                        mext_d_buf = {XCL_MEM_TOPOLOGY | unsigned(16), h_buf[i], 0};
                        break;
                    case 7:
                        mext_d_buf = {XCL_MEM_TOPOLOGY | unsigned(17), h_buf[i], 0};
                        break;
                    case 8:
                        mext_d_buf = {XCL_MEM_TOPOLOGY | unsigned(4), h_buf[i], 0};
                        break;
                    case 9:
                        mext_d_buf = {XCL_MEM_TOPOLOGY | unsigned(6), h_buf[i], 0};
                        break;
                    case 10:
                        mext_d_buf = {XCL_MEM_TOPOLOGY | unsigned(8), h_buf[i], 0};
                        break;
                    case 11:
                        mext_d_buf = {XCL_MEM_TOPOLOGY | unsigned(10), h_buf[i], 0};
                        break;
                    case 12:
                        mext_d_buf = {XCL_MEM_TOPOLOGY | unsigned(20), h_buf[i], 0};
                        break;
                    case 13:
                        mext_d_buf = {XCL_MEM_TOPOLOGY | unsigned(22), h_buf[i], 0};
                        break;
                    case 14:
                        mext_d_buf = {XCL_MEM_TOPOLOGY | unsigned(24), h_buf[i], 0};
                        break;
                    case 15:
                        mext_d_buf = {XCL_MEM_TOPOLOGY | unsigned(26), h_buf[i], 0};
                        break;
                    case 16:
                        mext_d_buf = {XCL_MEM_TOPOLOGY | unsigned(5), h_buf[i], 0};
                        break;
                    case 17:
                        mext_d_buf = {XCL_MEM_TOPOLOGY | unsigned(7), h_buf[i], 0};
                        break;
                    case 18:
                        mext_d_buf = {XCL_MEM_TOPOLOGY | unsigned(9), h_buf[i], 0};
                        break;
                    case 19:
                        mext_d_buf = {XCL_MEM_TOPOLOGY | unsigned(11), h_buf[i], 0};
                        break;
                    case 20:
                        mext_d_buf = {XCL_MEM_TOPOLOGY | unsigned(21), h_buf[i], 0};
                        break;
                    case 21:
                        mext_d_buf = {XCL_MEM_TOPOLOGY | unsigned(23), h_buf[i], 0};
                        break;
                    case 22:
                        mext_d_buf = {XCL_MEM_TOPOLOGY | unsigned(25), h_buf[i], 0};
                        break;
                    case 23:
                        mext_d_buf = {XCL_MEM_TOPOLOGY | unsigned(27), h_buf[i], 0};
                        break;
                    default:
                        break;
                }
                d_buf[i] = clCreateBuffer(ctx, CL_MEM_READ_WRITE | CL_MEM_USE_HOST_PTR | CL_MEM_EXT_PTR_XILINX,
                                          (size_t)(max_buf_size[i]), &mext_d_buf, &err);
            }
            clEnqueueMigrateMemObjects(cq, d_buf.size(), d_buf.data(), 0, 0, nullptr, nullptr);
            clFinish(cq);

            // set cl sub buffer dimensions
            setSubBufDim(2, 2, {27, 27});
            resetAccBufSize();
            // set cl buffer for kernel 1
            for (size_t p_id = 0; p_id < 2; p_id++) {
                createNoneOverLap(0, p_id, 0, 0, (1 << 26));
                createNoneOverLap(0, p_id, 1, 1, (1 << 26));
                createNoneOverLap(0, p_id, 2, 2, (1 << 26));
                createNoneOverLap(0, p_id, 3, 3, (1 << 26));
                createNoneOverLap(0, p_id, 4, 3, (512 / 8 * 14));
                createNoneOverLap(0, p_id, 5, 3, (512 / 8 * 24));
                createNoneOverLap(0, p_id, 6, 7, (512 / 8 * 24));
                createNoneOverLap(0, p_id, 7, 4, (1 << 26));
                createNoneOverLap(0, p_id, 8, 5, (1 << 26));
                createNoneOverLap(0, p_id, 9, 6, (1 << 26));
                createNoneOverLap(0, p_id, 10, 7, (1 << 26));
                if (p_id == 0) {
                    for (size_t i = 0; i < 16; i++) {
                        createNoneOverLap(0, p_id, i + 11, i + 8, (1 << 28));
                    }
                } else { // 16 HBM channel's buffer are the same between ping and pong
                    for (size_t i = 0; i < 16; i++) {
                        dupSubBuf(0, 0, i + 11, 0, p_id, i + 11);
                    }
                }
            }

            // set cl buffer for kernel 0
            for (size_t p_id = 0; p_id < 2; p_id++) {
                for (size_t arg_id = 0; arg_id < 27; arg_id++) {
                    dupSubBuf(0, p_id, arg_id, 1, p_id, arg_id);
                }
            }

            break;
        case AGGR:
            break;
        case DUMMY:
        default:
            break;
    }
}

Worker::Worker(const Worker& worker)
    : err(worker.err),
      ctx(worker.ctx),
      cq(worker.cq),
      prg(worker.prg),
      h_buf(worker.h_buf),
      d_buf(worker.d_buf),
      max_buf_size(worker.max_buf_size),
      acc_buf_size(worker.acc_buf_size),
      krn(worker.krn),
      sub_buf_head(worker.sub_buf_head),
      sub_buf_size(worker.sub_buf_size),
      sub_buf_parent(worker.sub_buf_parent),
      sub_buf(worker.sub_buf),
      sub_buf_host_parent(worker.sub_buf_host_parent){};

void Worker::release() {
    // release program, command queue
    clReleaseProgram(prg);
    clReleaseCommandQueue(cq);

    for (size_t i = 0; i < d_buf.size(); i++) {
        clReleaseMemObject(d_buf[i]);
    }

    for (size_t i = 0; i < h_buf.size(); i++) {
        free(h_buf[i]);
    }

    for (size_t i = 0; i < krn.size(); i++) {
        for (size_t j = 0; j < krn[i].size(); j++) {
            clReleaseKernel(krn[i][j]);
        }
    }

    // join migrate_t;
    {
        while (q.size() != 0) {
            this_thread::sleep_for(std::chrono::milliseconds(100));
            unique_lock<mutex> lk(m);
            cv.notify_all();
        }

        {
            unique_lock<mutex> lk(m);
            migrate_run = false;
            cv.notify_all();
        }

        if (migrate_t.joinable()) {
            migrate_t.join();
        }
    }
}

void Worker::print() {
    cout << "\nHost pinned buffer :" << endl;
    for (size_t i = 0; i < h_buf.size(); i++) {
        cout << "h_buf[" << i << "]=" << h_buf[i] << endl;
    }
    cout << "\nDevice buffer :" << endl;
    for (size_t i = 0; i < d_buf.size(); i++) {
        cout << "d_buf[" << i << "]=" << d_buf[i] << endl;
    }
    cout << "\nMax buffer size :" << endl;
    for (size_t i = 0; i < max_buf_size.size(); i++) {
        cout << "max_buf_size[" << i << "]=" << max_buf_size[i] << endl;
    }
    cout << "\nKernel :" << endl;
    for (size_t i = 0; i < krn.size(); i++) {
        for (size_t j = 0; j < krn[i].size(); j++) {
            cout << "krn[" << i << "][" << j << "] = " << krn[i][j] << endl;
        }
    }
    cout << "\nSub Buffer Head/Size/Parent/Sub Buffer/Pinned Buffer Address :" << endl;
    cout << "Dim 0 = " << sub_buf_head.size() << ", " << sub_buf_size.size() << ", " << sub_buf_parent.size() << ", "
         << sub_buf.size() << ", " << sub_buf_host_parent.size() << endl;
    for (size_t i = 0; i < sub_buf.size(); i++) {
        cout << "Dim 1 = " << sub_buf_head[i].size() << ", " << sub_buf_size[i].size() << ", "
             << sub_buf_parent[i].size() << ", " << sub_buf[i].size() << ", " << sub_buf_host_parent[i].size() << endl;

        for (size_t j = 0; j < sub_buf[i].size(); j++) {
            cout << "Dim 2 = " << sub_buf_head[i][j].size() << ", " << sub_buf_size[i][j].size() << ", "
                 << sub_buf_parent[i][j].size() << ", " << sub_buf[i][j].size() << ", "
                 << sub_buf_host_parent[i][j].size() << endl;

            for (size_t k = 0; k < sub_buf[i][j].size(); k++) {
                cout << sub_buf_head[i][j][k] << ", " << sub_buf_size[i][j][k] << ", " << sub_buf_parent[i][j][k]
                     << ", " << sub_buf[i][j][k] << ", " << sub_buf_host_parent[i][j][k] << endl;
            }
        }
    }
}

void Worker::runKernel(WorkerFunctions func,
                       size_t k_id,
                       size_t p_id,
                       vector<size_t> scalar_arg,
                       cl_event* evt_wait_list,
                       size_t evt_wait_num,
                       cl_event* evt) {
    if (func == WorkerFunctions::JOIN) {
        if (k_id == 1) { // part
            // set arg
            for (size_t i = 0; i < 13; i++) {
                if (i < 3) {
                    // int int_arg = scalar_arg[i];
                    // clSetKernelArg(krn[k_id][p_id], i, sizeof(int), &int_arg);
                } else {
                    clSetKernelArg(krn[k_id][p_id], i - 3, sizeof(cl_mem), &sub_buf[k_id][p_id][i - 3]);
                }
            }
            // enqueue task
            clEnqueueTask(cq, krn[k_id][p_id], evt_wait_num, evt_wait_list, evt);
        } else if (k_id == 0) { // join
            // set arg
            for (size_t i = 0; i < 28; i++) {
                if (i < 1) {
                    // clSetKernelArg(krn[k_id][p_id], i, sizeof(size_t), &(scalar_arg[i]));
                } else {
                    clSetKernelArg(krn[k_id][p_id], i - 1, sizeof(cl_mem), &sub_buf[k_id][p_id][i - 1]);
                }
            }
            // enqueue task
            clEnqueueTask(cq, krn[k_id][p_id], evt_wait_num, evt_wait_list, evt);
        }
    }
    clFlush(cq);
}

void Worker::MigrateToDevice(WorkerFunctions func,
                             size_t k_id,
                             size_t p_id,
                             vector<size_t>* arg_buf_idx,
                             vector<size_t>* arg_size,
                             cl_event* evt_wait_list,
                             size_t evt_wait_num,
                             cl_event* evt) {
    vector<cl_mem> p2d_mem;
    for (size_t i = 0; i < arg_buf_idx->size(); i++) {
        size_t buf_idx = (*arg_buf_idx)[i];
        cl_buffer_region tmp_region = {sub_buf_head[k_id][p_id][buf_idx], (*arg_size)[i]};
        p2d_mem.push_back(clCreateSubBuffer(sub_buf_parent[k_id][p_id][buf_idx],
                                            CL_MEM_EXT_PTR_XILINX | CL_MEM_READ_WRITE, CL_BUFFER_CREATE_TYPE_REGION,
                                            &tmp_region, &err));
    }
    cl_int hehe = clEnqueueMigrateMemObjects(cq, p2d_mem.size(), p2d_mem.data(), 0, evt_wait_num, evt_wait_list, evt);
    clFlush(cq);
}

void Worker::MigrateMetaToHost(
    WorkerFunctions func, size_t k_id, size_t p_id, cl_event* evt_wait_list, size_t evt_wait_num, cl_event* evt) {
    if (func == WorkerFunctions::JOIN) {
        if (k_id == 0) {
            clEnqueueMigrateMemObjects(cq, 1, &sub_buf[k_id][p_id][6], 1, evt_wait_num, evt_wait_list, evt);
        } else if (k_id == 1) {
            clEnqueueMigrateMemObjects(cq, 1, &sub_buf[k_id][p_id][6], 1, evt_wait_num, evt_wait_list, evt);
        }
    }
    clFlush(cq);
}

void Worker::MigrateResToHost(WorkerFunctions func,
                              size_t k_id,
                              size_t p_id,
                              vector<size_t>* arg_buf_idx,
                              vector<vector<size_t> >* arg_head,
                              vector<vector<size_t> >* arg_size,
                              vector<size_t>* res_row,
                              cl_event* evt_wait_list,
                              size_t evt_wait_num,
                              cl_event* evt) {
    migrate_task tsk;

    tsk.func = func;
    tsk.k_id = k_id;
    tsk.p_id = p_id;
    tsk.arg_buf_idx = arg_buf_idx;
    tsk.arg_head = arg_head;
    tsk.arg_size = arg_size;
    tsk.res_row = res_row;
    tsk.evt_wait_list = evt_wait_list;
    tsk.evt_wait_num = evt_wait_num;
    tsk.evt = evt;

    {
        lock_guard<mutex> lk(m);
        q.push(tsk);
        cv.notify_all();
    }
}

} // gqe
} // database
} // xf
