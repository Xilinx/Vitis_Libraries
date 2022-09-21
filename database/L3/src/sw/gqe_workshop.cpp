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

#include "xf_database/gqe_workshop.hpp"

namespace xf {
namespace database {
namespace gqe {

using namespace std;

size_t ret(size_t x) {
    return x;
}

Workshop::Workshop(string device_shell_name, string xclbin_path, WorkerFunctions func)
    : PlatformInit(device_shell_name) {
    // Create Context
    cl_context_properties ctx_prop[3] = {CL_CONTEXT_PLATFORM, (cl_context_properties)xilinx_platform_id, 0};
    ctx = clCreateContext(ctx_prop, device_num, device_id.data(), NULL, NULL, &err);

    // Create Workers
    for (auto itr = device_id.begin(); itr != device_id.end(); ++itr) {
        worker.push_back(Worker(ctx, *itr, xclbin_path, func));
    }

    for (size_t i = 0; i < device_num; i++) {
        worker[i].start();
    }

    h2p.start();
    p2h.start();

    join_service_run = true;
    join_service_t = thread(&Workshop::checkJoinQueue, this);
    cout << "Workshop started" << endl;
}

Workshop::~Workshop() {}

void Workshop::release() {
    cout << "Start workshop release" << endl;
    // release manage_t
    while (q.size() != 0) {
        unique_lock<mutex> lk(m);
        join_service_run = false;
        cv.notify_all();
        this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    {
        unique_lock<mutex> lk(m);
        join_service_run = false;
        cv.notify_all();
    }
    if (join_service_t.joinable()) {
        join_service_t.join();
    } else {
        cout << "thread manage_t is not joinable" << endl;
    }
    // release MemCoppier
    h2p.release();
    p2h.release();
    // release worker
    for (size_t i = 0; i < worker.size(); i++) {
        worker[i].release();
    }
    // release context
    clReleaseContext(ctx);
    // rlease device
    PlatformInit::release();
    cout << "Workshop released" << endl;
}

void Workshop::print() {
    PlatformInit::print();
    cout << "There're " << worker.size() << " workers" << endl;
    for (size_t i = 0; i < worker.size(); i++) {
        cout << "Worker[" << i << "]" << endl;
        worker[i].print();
    }
}

void Workshop::Join(TableSection* tab_a,
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
                    JoinStrategyBase* strategyimp) {
    task_complex tsk{JOIN_TASK,
                     tab_a,
                     filter_a,
                     tab_a_sec_ready,
                     tab_part_a,
                     tab_b,
                     filter_b,
                     tab_b_sec_ready,
                     tab_part_b,
                     join_str,
                     output_str,
                     tab_c,
                     tab_c_sec_ready_promise,
                     join_type,
                     strategyimp};
    {
        lock_guard<mutex> lk(m);
        q.push(tsk);
        cv.notify_all();
    }
}

void Workshop::Bloomfilter(TableSection* tab_a,
                           string filter_a,
                           vector<future<size_t> >* tab_a_sec_ready,
                           TableSection* tab_b,
                           string filter_b,
                           vector<future<size_t> >* tab_b_sec_ready,
                           string bf_str,
                           string output_str,
                           TableSection* tab_c,
                           vector<promise<size_t> >* tab_c_sec_ready_promise) {
    task_complex tsk{
        BF_TASK, tab_a,      filter_a, tab_a_sec_ready,         nullptr, tab_b,  filter_b, tab_b_sec_ready, nullptr,
        bf_str,  output_str, tab_c,    tab_c_sec_ready_promise, 0,       nullptr};
    {
        lock_guard<mutex> lk(m);
        q.push(tsk);
        cv.notify_all();
    }
}

void Workshop::processJoin(task_complex tsk) {
    // params
    StrategySet params = tsk.strategyimp->getSolutionParams((*(tsk.tab_a)), (*(tsk.tab_b)));
    cout << "start to process one task_complex" << endl;

    size_t wkn = worker.size();
    if (params.sol == 1) {
        if (params.sec_o != 1) {
            cout << "In Join solution 1, tab_a should only has 1 section" << endl;
        }
        JoinConfig jcfg(*tsk.tab_a, tsk.filter_a, *tsk.tab_b, tsk.filter_b, tsk.join_str, *tsk.tab_c, tsk.output_str,
                        false, tsk.join_type);
        JoinConfig jcfg_probe(*tsk.tab_a, tsk.filter_a, *tsk.tab_b, tsk.filter_b, tsk.join_str, *tsk.tab_c,
                              tsk.output_str, true, tsk.join_type);
        ap_uint<512>* join_cfg = jcfg.getJoinConfigBits();
        ap_uint<512>* probe_cfg = jcfg_probe.getJoinConfigBits();
        vector<vector<int8_t> > join_scan = jcfg.getShuffleScan();
        vector<int8_t> join_wr = jcfg.getShuffleWrite();
        // 1. build
        // 1.0 tab_a's sec[0] ready;
        vector<size_t> tab_a_sec_row(1);
        tab_a_sec_row[0] = (*tsk.tab_a_sec_ready)[0].get();
        // 1.1 events
        vector<cl_event> build_h2p_evt(wkn);
        vector<cl_event> build_p2d_evt(wkn);
        vector<cl_event> build_krn_evt(wkn);
        vector<vector<cl_event> > build_h2p_evt_dep(wkn);
        vector<vector<cl_event> > build_p2d_evt_dep(wkn);
        vector<vector<cl_event> > build_krn_evt_dep(wkn);
        for (size_t i = 0; i < wkn; i++) {
            build_h2p_evt[i] = clCreateUserEvent(ctx, &err);
        }

        // 1.2 h2p, p2d, krn args
        vector<vector<vector<char*> > > build_h2p_src(wkn);
        vector<vector<vector<char*> > > build_h2p_dst(wkn);
        vector<vector<vector<size_t> > > build_h2p_bias(wkn);
        vector<vector<vector<size_t> > > build_h2p_length(wkn);
        vector<vector<vector<size_t> > > build_h2p_acc(wkn);
        vector<MetaTable> meta_build_in(wkn);
        vector<vector<size_t> > build_p2d_arg_buf_idx(wkn);
        vector<vector<size_t> > build_p2d_arg_size(wkn);
        vector<vector<size_t> > build_krn_scalar_arg(wkn);
        for (size_t i = 0; i < wkn; i++) {
            build_h2p_src[i].resize(1);
            build_h2p_dst[i].resize(1);
            build_h2p_bias[i].resize(1);
            build_h2p_length[i].resize(1);
            build_h2p_acc[i].resize(1);
            // 1.2.1 din_col0,1,2
            for (size_t j = 0; j < 1; j++) {
                for (size_t k = 0; k < 3; k++) {
                    if (join_scan[0][k] != -1) {
                        build_h2p_src[i][j].push_back(tsk.tab_a->getColPointer(j, join_scan[0][k]));
                        build_h2p_dst[i][j].push_back(worker[i].sub_buf_host_parent[1][0][k] +
                                                      worker[i].sub_buf_head[1][0][k]);
                        build_h2p_bias[i][j].push_back(0);
                        build_h2p_length[i][j].push_back(tsk.tab_a->getColTypeSize(join_scan[0][k]) * tab_a_sec_row[j]);
                        build_h2p_acc[i][j].push_back(0);
                        build_p2d_arg_buf_idx[i].push_back(k);
                        build_p2d_arg_size[i].push_back(tsk.tab_a->getColTypeSize(join_scan[0][k]) * tab_a_sec_row[j]);
                    }
                }
            }

            // 1.2.2 din_val
            if (tsk.tab_a->getValidEnableFlag()) {
                for (size_t j = 0; j < 1; j++) {
                    build_h2p_src[i][j].push_back(tsk.tab_a->getValColPointer(j));
                    build_h2p_dst[i][j].push_back(worker[i].sub_buf_host_parent[1][0][3] +
                                                  worker[i].sub_buf_head[1][0][3]);
                    build_h2p_bias[i][j].push_back(0);
                    build_h2p_length[i][j].push_back((tab_a_sec_row[j] + 7) / 8);
                    build_h2p_acc[i][j].push_back(0);
                    build_p2d_arg_buf_idx[i].push_back(3);
                    build_p2d_arg_size[i].push_back((tab_a_sec_row[j] + 7) / 8);
                }
            }
            // 1.2.3 din_krn_cfg
            for (size_t j = 0; j < 1; j++) {
                build_h2p_src[i][j].push_back((char*)join_cfg);
                build_h2p_dst[i][j].push_back(worker[i].sub_buf_host_parent[1][0][4] + worker[i].sub_buf_head[1][0][4]);
                build_h2p_bias[i][j].push_back(0);
                build_h2p_length[i][j].push_back(sizeof(ap_uint<512>) * 14);
                build_h2p_acc[i][j].push_back(0);
                build_p2d_arg_buf_idx[i].push_back(4);
                build_p2d_arg_size[i].push_back(sizeof(ap_uint<512>) * 14);
            }
            // 1.2.4 din_meta_in
            meta_build_in[i].setSecID(0);
            meta_build_in[i].setColNum(3);
            for (size_t j = 0; j < 3; j++) {
                meta_build_in[i].setCol(j, j, tab_a_sec_row[0]);
            }
            for (size_t j = 0; j < 1; j++) {
                build_h2p_src[i][j].push_back((char*)meta_build_in[i].meta());
                build_h2p_dst[i][j].push_back(worker[i].sub_buf_host_parent[1][0][5] + worker[i].sub_buf_head[1][0][5]);
                build_h2p_bias[i][j].push_back(0);
                build_h2p_length[i][j].push_back(sizeof(ap_uint<512>) * 24);
                build_h2p_acc[i][j].push_back(0);
                build_p2d_arg_buf_idx[i].push_back(5);
                build_p2d_arg_size[i].push_back(sizeof(ap_uint<512>) * 24);
            }
            // 1.2.5 scalar_arg
            build_krn_scalar_arg[i].resize(1);
            build_krn_scalar_arg[i][0] = 0;
        }
        // 1.3 addTask
        // 1.3.1 h2p
        for (size_t i = 0; i < wkn; i++) {
            h2p.addTask(build_h2p_evt_dep[i].data(), build_h2p_evt_dep[i].size(), &build_h2p_evt[i], &build_h2p_src[i],
                        &build_h2p_dst[i], &build_h2p_bias[i], &build_h2p_length[i], &build_h2p_acc[i]);
        }
        // 1.3.2 p2d
        for (size_t i = 0; i < wkn; i++) {
            build_p2d_evt_dep[i].push_back(build_h2p_evt[i]);
            worker[i].MigrateToDevice(WorkerFunctions::JOIN, 0, 0, &build_p2d_arg_buf_idx[i], &build_p2d_arg_size[i],
                                      build_p2d_evt_dep[i].data(), build_p2d_evt_dep[i].size(), &build_p2d_evt[i]);
        }
        // 1.3.3 krn
        for (size_t i = 0; i < wkn; i++) {
            build_krn_evt_dep[i].push_back(build_p2d_evt[i]);
            worker[i].runKernel(WorkerFunctions::JOIN, 0, 0, build_krn_scalar_arg[i], build_krn_evt_dep[i].data(),
                                build_krn_evt_dep[i].size(), &build_krn_evt[i]);
        }
        // 1.4 wait
        clWaitForEvents(build_krn_evt.size(), build_krn_evt.data());

        // 2. probe
        size_t tab_b_sec_num = params.sec_l;
        vector<size_t> tab_b_sec_row(tab_b_sec_num);

        // 2.1 event and dependence
        vector<cl_event> probe_h2p_evt(tab_b_sec_num);
        vector<cl_event> probe_p2d_evt(tab_b_sec_num);
        vector<cl_event> probe_krn_evt(tab_b_sec_num);
        vector<cl_event> probe_meta_evt(tab_b_sec_num);
        vector<cl_event> probe_d2p_evt(tab_b_sec_num);
        vector<cl_event> probe_p2h_evt(tab_b_sec_num);
        for (size_t i = 0; i < tab_b_sec_num; i++) {
            probe_h2p_evt[i] = clCreateUserEvent(ctx, &err);
            probe_d2p_evt[i] = clCreateUserEvent(ctx, &err);
            probe_p2h_evt[i] = clCreateUserEvent(ctx, &err);
        }
        vector<vector<cl_event> > probe_h2p_evt_dep(tab_b_sec_num);
        vector<vector<cl_event> > probe_p2d_evt_dep(tab_b_sec_num);
        vector<vector<cl_event> > probe_krn_evt_dep(tab_b_sec_num);
        vector<vector<cl_event> > probe_meta_evt_dep(tab_b_sec_num);
        vector<vector<cl_event> > probe_d2p_evt_dep(tab_b_sec_num);
        vector<vector<cl_event> > probe_p2h_evt_dep(tab_b_sec_num);

        // 2.2 h2p, p2d, krn, meta, d2p, p2h args
        // 2.2.1 h2p
        vector<vector<vector<char*> > > probe_h2p_src(tab_b_sec_num);
        vector<vector<vector<char*> > > probe_h2p_dst(tab_b_sec_num);
        vector<vector<vector<size_t> > > probe_h2p_bias(tab_b_sec_num);
        vector<vector<vector<size_t> > > probe_h2p_length(tab_b_sec_num);
        vector<vector<vector<size_t> > > probe_h2p_acc(tab_b_sec_num);
        vector<MetaTable> meta_probe_in(tab_b_sec_num);
        vector<MetaTable> meta_probe_out(tab_b_sec_num);
        // 2.2.2 p2d
        vector<vector<size_t> > probe_p2d_arg_buf_idx(tab_b_sec_num);
        vector<vector<size_t> > probe_p2d_arg_size(tab_b_sec_num);
        // 2.2.3 krn
        vector<vector<size_t> > probe_krn_scalar_arg(tab_b_sec_num);
        // 2.2.4 meta, no extra args
        // 2.2.5 d2p
        vector<vector<size_t> > probe_d2p_arg_buf_idx(tab_b_sec_num);
        vector<vector<vector<size_t> > > probe_d2p_arg_head(tab_b_sec_num);
        // vector<vector<vector<size_t> > > probe_d2p_arg_size(tab_b_sec_num);
        // d2p's will write to probe_p2h_length
        vector<vector<size_t> > probe_d2p_res_row(tab_b_sec_num);
        // 2.2.6 p2h
        vector<vector<vector<char*> > > probe_p2h_src(tab_b_sec_num);
        vector<vector<vector<char*> > > probe_p2h_dst(tab_b_sec_num);
        vector<vector<vector<size_t> > > probe_p2h_bias(tab_b_sec_num);
        vector<vector<vector<size_t> > > probe_p2h_length(tab_b_sec_num);
        vector<vector<vector<size_t> > > probe_p2h_acc(tab_b_sec_num);

        // 2.3 check tab_b's sec ready one by one
        for (size_t i = 0; i < tab_b_sec_num; i++) {
            // 2.3.0. get tab_b's sec ready
            tab_b_sec_row[i] = (*tsk.tab_b_sec_ready)[i].get();
            size_t w_id = i % wkn;
            size_t p_id = (i / wkn) % 2;
            // 2.3.1 h2p, p2d, krn
            // 2.3.1.1 din_col0,1,2
            probe_h2p_src[i].resize(1);
            probe_h2p_dst[i].resize(1);
            probe_h2p_bias[i].resize(1);
            probe_h2p_length[i].resize(1);
            probe_h2p_acc[i].resize(1);
            for (size_t k = 0; k < 3; k++) {
                if (join_scan[1][k] != -1) {
                    probe_h2p_src[i][0].push_back(tsk.tab_b->getColPointer(i, join_scan[1][k]));
                    probe_h2p_dst[i][0].push_back(worker[w_id].sub_buf_host_parent[1][p_id][k] +
                                                  worker[w_id].sub_buf_head[1][p_id][k]);
                    probe_h2p_bias[i][0].push_back(0);
                    probe_h2p_length[i][0].push_back(tsk.tab_b->getColTypeSize(join_scan[1][k]) * tab_b_sec_row[i]);
                    probe_h2p_acc[i][0].push_back(0);
                    probe_p2d_arg_buf_idx[i].push_back(k);
                    probe_p2d_arg_size[i].push_back(tsk.tab_b->getColTypeSize(join_scan[1][k]) * tab_b_sec_row[i]);
                }
            }
            // 2.3.1.2 din_val
            if (tsk.tab_b->getValidEnableFlag()) {
                probe_h2p_src[i][0].push_back(tsk.tab_b->getValColPointer(i));
                probe_h2p_dst[i][0].push_back(worker[w_id].sub_buf_host_parent[1][p_id][3] +
                                              worker[w_id].sub_buf_head[1][p_id][3]);
                probe_h2p_bias[i][0].push_back(0);
                probe_h2p_length[i][0].push_back((tab_b_sec_row[i] + 7) / 8);
                probe_h2p_acc[i][0].push_back(0);
                probe_p2d_arg_buf_idx[i].push_back(3);
                probe_p2d_arg_size[i].push_back((tab_b_sec_row[i] + 7) / 8);
            }
            // 2.3.1.3 din_krn_cfg
            probe_h2p_src[i][0].push_back((char*)probe_cfg);
            probe_h2p_dst[i][0].push_back(worker[w_id].sub_buf_host_parent[1][p_id][4] +
                                          worker[w_id].sub_buf_head[1][p_id][4]);
            probe_h2p_bias[i][0].push_back(0);
            probe_h2p_length[i][0].push_back(sizeof(ap_uint<512>) * 14);
            probe_h2p_acc[i][0].push_back(0);
            probe_p2d_arg_buf_idx[i].push_back(4);
            probe_p2d_arg_size[i].push_back(sizeof(ap_uint<512>) * 14);
            // 2.3.1.4 din_meta_in
            meta_probe_in[i].setSecID(i);
            meta_probe_in[i].setColNum(3);
            for (size_t k = 0; k < 3; k++) {
                meta_probe_in[i].setCol(k, k, tab_b_sec_row[i]);
            }
            probe_h2p_src[i][0].push_back((char*)meta_probe_in[i].meta());
            probe_h2p_dst[i][0].push_back(worker[w_id].sub_buf_host_parent[1][p_id][5] +
                                          worker[w_id].sub_buf_head[1][p_id][5]);
            probe_h2p_bias[i][0].push_back(0);
            probe_h2p_length[i][0].push_back(sizeof(ap_uint<512>) * 24);
            probe_h2p_acc[i][0].push_back(0);
            probe_p2d_arg_buf_idx[i].push_back(5);
            probe_p2d_arg_size[i].push_back(sizeof(ap_uint<512>) * 24);
            // 2.3.1.5 din_meta_out
            meta_probe_out[i].setSecID(0);
            meta_probe_out[i].setColNum(4);
            for (size_t k = 0; k < 4; k++) {
                meta_probe_out[i].setCol(k, k, tsk.tab_c->getSecRowNum(i));
            }
            probe_h2p_src[i][0].push_back((char*)meta_probe_out[i].meta());
            probe_h2p_dst[i][0].push_back(worker[w_id].sub_buf_host_parent[1][p_id][6] +
                                          worker[w_id].sub_buf_head[1][p_id][6]);
            probe_h2p_bias[i][0].push_back(0);
            probe_h2p_length[i][0].push_back(sizeof(ap_uint<512>) * 24);
            probe_h2p_acc[i][0].push_back(0);
            probe_p2d_arg_buf_idx[i].push_back(6);
            probe_p2d_arg_size[i].push_back(sizeof(ap_uint<512>) * 24);
            // 2.3.2 krn
            probe_krn_scalar_arg[i].push_back(1);
            // 2.3.3 meta
            // 2.3.4 d2p, p2h
            probe_d2p_res_row[i].resize(1, 0);
            probe_d2p_arg_head[i].resize(1);

            probe_p2h_src[i].resize(1);
            probe_p2h_dst[i].resize(1);
            probe_p2h_bias[i].resize(1);
            probe_p2h_length[i].resize(1);
            probe_p2h_acc[i].resize(1);
            for (size_t k = 0; k < 4; k++) {
                if (join_wr[k] != -1) {
                    probe_d2p_arg_buf_idx[i].push_back(k + 7);
                    probe_d2p_arg_head[i][0].push_back(0);

                    probe_p2h_src[i][0].push_back(worker[w_id].sub_buf_host_parent[1][p_id][k + 7] +
                                                  worker[w_id].sub_buf_head[1][p_id][k + 7]);
                    probe_p2h_dst[i][0].push_back(tsk.tab_c->getColPointer(i, join_wr[k]));
                    probe_p2h_bias[i][0].push_back(0);
                    probe_p2h_length[i][0].push_back(0); // will be write by d2p and read by p2h
                    probe_p2h_acc[i][0].push_back(0);
                }
            }
            // 2.4 add Task
            // 2.4.1 h2p => p2d(-2)
            if (i >= (wkn * 2)) {
                probe_h2p_evt_dep[i].push_back(probe_p2d_evt[i - (2 * wkn)]);
            }
            h2p.addTask(probe_h2p_evt_dep[i].data(), probe_h2p_evt_dep[i].size(), &probe_h2p_evt[i], &probe_h2p_src[i],
                        &probe_h2p_dst[i], &probe_h2p_bias[i], &probe_h2p_length[i], &probe_h2p_acc[i]);
            // 2.4.2 p2d
            probe_p2d_evt_dep[i].push_back(probe_h2p_evt[i]);
            if (i >= (wkn * 2)) {
                probe_p2d_evt_dep[i].push_back(probe_krn_evt[i - (2 * wkn)]);
            }
            worker[w_id].MigrateToDevice(WorkerFunctions::JOIN, 0, p_id, &probe_p2d_arg_buf_idx[i],
                                         &probe_p2d_arg_size[i], probe_p2d_evt_dep[i].data(),
                                         probe_p2d_evt_dep[i].size(), &probe_p2d_evt[i]);
            // 2.4.3 krn
            probe_krn_evt_dep[i].push_back(probe_p2d_evt[i]);
            if (i >= wkn) {
                probe_krn_evt_dep[i].push_back(probe_krn_evt[i - wkn]);
            }
            if (i >= (wkn * 2)) {
                probe_krn_evt_dep[i].push_back(probe_d2p_evt[i - (2 * wkn)]);
            }
            worker[w_id].runKernel(WorkerFunctions::JOIN, 0, p_id, probe_krn_scalar_arg[i], probe_krn_evt_dep[i].data(),
                                   probe_krn_evt_dep[i].size(), &probe_krn_evt[i]);
            // 2.4.4 meta
            probe_meta_evt_dep[i].push_back(probe_krn_evt[i]);
            if (i >= (wkn * 2)) {
                probe_meta_evt_dep[i].push_back(probe_d2p_evt[i - (2 * wkn)]);
            }
            worker[w_id].MigrateMetaToHost(WorkerFunctions::JOIN, 0, p_id, probe_meta_evt_dep[i].data(),
                                           probe_meta_evt_dep[i].size(), &probe_meta_evt[i]);
            // 2.4.5 d2p
            probe_d2p_evt_dep[i].push_back(probe_meta_evt[i]);
            if (i >= (wkn * 2)) {
                probe_d2p_evt_dep[i].push_back(probe_p2h_evt[i - (wkn * 2)]);
            }
            worker[w_id].MigrateResToHost(WorkerFunctions::JOIN, 0, p_id, &probe_d2p_arg_buf_idx[i],
                                          &probe_d2p_arg_head[i], &probe_p2h_length[i], &probe_d2p_res_row[i],
                                          probe_d2p_evt_dep[i].data(), probe_d2p_evt_dep[i].size(), &probe_d2p_evt[i]);
            // 2.4.6 p2h
            probe_p2h_evt_dep[i].push_back(probe_d2p_evt[i]);
            p2h.addTask(probe_p2h_evt_dep[i].data(), probe_p2h_evt_dep[i].size(), &probe_p2h_evt[i], &probe_p2h_src[i],
                        &probe_p2h_dst[i], &probe_p2h_bias[i], &probe_p2h_length[i], &probe_p2h_acc[i]);
        }
        // 2.5 wait for event
        for (size_t i = 0; i < probe_p2h_evt.size(); i++) {
            clWaitForEvents(1, &probe_p2h_evt[i]);
            (*tsk.tab_c_sec_ready_promise)[i].set_value(probe_d2p_res_row[i][0]);
        }

        // 3 release cl_event
        for (size_t i = 0; i < wkn; i++) {
            clReleaseEvent(build_h2p_evt[i]);
            clReleaseEvent(build_p2d_evt[i]);
            clReleaseEvent(build_krn_evt[i]);
        }
        for (size_t i = 0; i < tab_b_sec_num; i++) {
            clReleaseEvent(probe_h2p_evt[i]);
            clReleaseEvent(probe_p2d_evt[i]);
            clReleaseEvent(probe_krn_evt[i]);
            clReleaseEvent(probe_meta_evt[i]);
            clReleaseEvent(probe_d2p_evt[i]);
            clReleaseEvent(probe_p2h_evt[i]);
        }
    } else if (params.sol == 2) {
        size_t tab_a_sec_num = params.sec_o;
        size_t tab_b_sec_num = params.sec_l;
        size_t part_num = (1 << params.log_part);

        PartJoinConfig pjcfg(*tsk.tab_a, tsk.filter_a, *tsk.tab_b, tsk.filter_b, tsk.join_str, *tsk.tab_c,
                             tsk.output_str, tsk.join_type);
        ap_uint<512>* part_cfg = pjcfg.getPartConfigBits();
        ap_uint<512>* join_cfg = pjcfg.getJoinConfigBits();
        vector<vector<int8_t> > part_scan = pjcfg.getShuffleScanPart();
        vector<vector<int8_t> > join_scan = pjcfg.getShuffleScanHJ();
        vector<vector<int8_t> > part_wr = pjcfg.getShuffleWritePart();
        vector<int8_t> join_wr = pjcfg.getShuffleWriteHJ();

        // 1. partition tab_a => tab_part_a
        // 1.1 event and dependence
        vector<cl_event> part_a_h2p_evt(tab_a_sec_num);
        vector<cl_event> part_a_p2d_evt(tab_a_sec_num);
        vector<cl_event> part_a_krn_evt(tab_a_sec_num);
        vector<cl_event> part_a_meta_evt(tab_a_sec_num);
        vector<cl_event> part_a_d2p_evt(tab_a_sec_num);
        vector<cl_event> part_a_p2h_evt(tab_a_sec_num);
        for (size_t i = 0; i < tab_a_sec_num; i++) {
            part_a_h2p_evt[i] = clCreateUserEvent(ctx, &err);
            part_a_d2p_evt[i] = clCreateUserEvent(ctx, &err);
            part_a_p2h_evt[i] = clCreateUserEvent(ctx, &err);
        }
        vector<vector<cl_event> > part_a_h2p_evt_dep(tab_a_sec_num);
        vector<vector<cl_event> > part_a_p2d_evt_dep(tab_a_sec_num);
        vector<vector<cl_event> > part_a_krn_evt_dep(tab_a_sec_num);
        vector<vector<cl_event> > part_a_meta_evt_dep(tab_a_sec_num);
        vector<vector<cl_event> > part_a_d2p_evt_dep(tab_a_sec_num);
        vector<vector<cl_event> > part_a_p2h_evt_dep(tab_a_sec_num);
        // 1.2 h2p, p2d, krn, meta, d2p, p2h args
        // 1.2.1 h2p
        vector<vector<vector<char*> > > part_a_h2p_src(tab_a_sec_num);
        vector<vector<vector<char*> > > part_a_h2p_dst(tab_a_sec_num);
        vector<vector<vector<size_t> > > part_a_h2p_bias(tab_a_sec_num);
        vector<vector<vector<size_t> > > part_a_h2p_length(tab_a_sec_num);
        vector<vector<vector<size_t> > > part_a_h2p_acc(tab_a_sec_num);
        vector<MetaTable> meta_part_a_in(tab_a_sec_num);
        vector<MetaTable> meta_part_a_out(tab_a_sec_num);
        // 1.2.2 p2d
        vector<vector<size_t> > part_a_p2d_arg_buf_idx(tab_a_sec_num);
        vector<vector<size_t> > part_a_p2d_arg_size(tab_a_sec_num);
        // 1.2.3 krn
        vector<vector<size_t> > part_a_krn_scalar_arg(tab_a_sec_num);
        // 1.2.4 meta, no extra args
        // 1.2.5 d2p
        vector<vector<size_t> > part_a_d2p_arg_buf_idx(tab_a_sec_num);
        vector<vector<vector<size_t> > > part_a_d2p_arg_head(tab_a_sec_num);
        // vector<vector<vector<size_t> > > part_a_d2p_arg_size(tab_a_sec_num);
        // d2p's will write to part_a_p2h_length
        vector<vector<size_t> > part_a_d2p_res_row(tab_a_sec_num);
        // 1.2.6 p2h
        vector<vector<vector<char*> > > part_a_p2h_src(tab_a_sec_num);
        vector<vector<vector<char*> > > part_a_p2h_dst(tab_a_sec_num);
        vector<vector<size_t> > part_a_p2h_bias(part_num);
        vector<vector<vector<size_t> > > part_a_p2h_length(tab_a_sec_num);
        vector<vector<size_t> >& part_a_p2h_acc = part_a_p2h_bias;
        // 1.3 check tab_a's sec ready one by one
        vector<size_t> tab_a_sec_row(tab_a_sec_num);
        for (size_t i = 0; i < tab_a_sec_num; i++) {
            tab_a_sec_row[i] = (*tsk.tab_a_sec_ready)[i].get();
            size_t w_id = i % wkn;
            size_t p_id = (i / wkn) % 2;

            // 1.3.1 h2p, p2d, krn
            // 1.3.1.1 din_col0, 1, 2
            part_a_h2p_src[i].resize(1);
            part_a_h2p_dst[i].resize(1);
            part_a_h2p_bias[i].resize(1);
            part_a_h2p_length[i].resize(1);
            part_a_h2p_acc[i].resize(1);
            for (size_t k = 0; k < 3; k++) {
                if (part_scan[0][k] != -1) {
                    part_a_h2p_src[i][0].push_back(tsk.tab_a->getColPointer(i, part_scan[0][k]));
                    part_a_h2p_dst[i][0].push_back(worker[w_id].sub_buf_host_parent[0][p_id][k] +
                                                   worker[w_id].sub_buf_head[0][p_id][k]);
                    part_a_h2p_bias[i][0].push_back(0);
                    part_a_h2p_length[i][0].push_back(tsk.tab_a->getColTypeSize(part_scan[0][k]) * tab_a_sec_row[i]);
                    part_a_h2p_acc[i][0].push_back(0);
                    part_a_p2d_arg_buf_idx[i].push_back(k);
                    part_a_p2d_arg_size[i].push_back(tsk.tab_a->getColTypeSize(part_scan[0][k]) * tab_a_sec_row[i]);
                }
            }
            // 1.3.1.2 din_val
            if (tsk.tab_a->getValidEnableFlag()) {
                part_a_h2p_src[i][0].push_back(tsk.tab_a->getValColPointer(i));
                part_a_h2p_dst[i][0].push_back(worker[w_id].sub_buf_host_parent[0][p_id][3] +
                                               worker[w_id].sub_buf_head[0][p_id][3]);
                part_a_h2p_bias[i][0].push_back(0);
                part_a_h2p_length[i][0].push_back((tab_a_sec_row[i] + 7) / 8);
                part_a_h2p_acc[i][0].push_back(0);
                part_a_p2d_arg_buf_idx[i].push_back(3);
                part_a_p2d_arg_size[i].push_back((tab_a_sec_row[i] + 7) / 8);
            }
            // 1.3.1.3 din_krn_cfg
            part_a_h2p_src[i][0].push_back((char*)part_cfg);
            part_a_h2p_dst[i][0].push_back(worker[w_id].sub_buf_host_parent[0][p_id][4] +
                                           worker[w_id].sub_buf_head[0][p_id][4]);
            part_a_h2p_bias[i][0].push_back(0);
            part_a_h2p_length[i][0].push_back(sizeof(ap_uint<512>) * 14);
            part_a_h2p_acc[i][0].push_back(0);
            part_a_p2d_arg_buf_idx[i].push_back(4);
            part_a_p2d_arg_size[i].push_back(sizeof(ap_uint<512>) * 14);
            // 1.3.1.4 din_meta
            meta_part_a_in[i].setSecID(i);
            meta_part_a_in[i].setColNum(3);
            for (size_t k = 0; k < 3; k++) {
                meta_part_a_in[i].setCol(k, k, tab_a_sec_row[i]);
            }
            part_a_h2p_src[i][0].push_back((char*)meta_part_a_in[i].meta());
            part_a_h2p_dst[i][0].push_back(worker[w_id].sub_buf_host_parent[0][p_id][5] +
                                           worker[w_id].sub_buf_head[0][p_id][5]);
            part_a_h2p_bias[i][0].push_back(0);
            part_a_h2p_length[i][0].push_back(sizeof(ap_uint<512>) * 24);
            part_a_h2p_acc[i][0].push_back(0);
            part_a_p2d_arg_buf_idx[i].push_back(5);
            part_a_p2d_arg_size[i].push_back(sizeof(ap_uint<512>) * 24);
            // 1.3.1.5 dout_meta
            // meta_part_a_out[i].setSecID(0);
            meta_part_a_out[i].setColNum(3);
            // tab_part_a's all sections should have same row num.
            // tab_part_b's all sections should have same row num
            size_t sub_buf_depth = worker[w_id].sub_buf_size[0][p_id][7] / 64; // how many 512bit rows
            size_t part_a_col_depth_init = sub_buf_depth;
            size_t part_a_col_part_depth = part_a_col_depth_init / part_num;
            meta_part_a_out[i].setPartition(part_num, part_a_col_part_depth);
            for (size_t k = 0; k < 3; k++) {
                meta_part_a_out[i].setCol(k, k, part_a_col_depth_init);
            }

            part_a_h2p_src[i][0].push_back((char*)meta_part_a_out[i].meta());
            part_a_h2p_dst[i][0].push_back(worker[w_id].sub_buf_host_parent[0][p_id][6] +
                                           worker[w_id].sub_buf_head[0][p_id][6]);
            part_a_h2p_bias[i][0].push_back(0);
            part_a_h2p_length[i][0].push_back(sizeof(ap_uint<512>) * 24);
            part_a_h2p_acc[i][0].push_back(0);
            part_a_p2d_arg_buf_idx[i].push_back(6);
            part_a_p2d_arg_size[i].push_back(sizeof(ap_uint<512>) * 24);

            // 1.3.2 krn
            const int k_depth = 512;
            const int idx_o = 0;
            part_a_krn_scalar_arg[i].push_back(k_depth);
            part_a_krn_scalar_arg[i].push_back(idx_o);
            part_a_krn_scalar_arg[i].push_back(params.log_part);
            // 1.3.3 meta
            // 1.3.4 d2p, p2h
            part_a_d2p_arg_head[i].resize(part_num);
            part_a_d2p_res_row[i].resize(part_num, 0);
            // part_a_d2p_arg_buf_idx
            // d2p's will write to part_a_p2h_length

            part_a_p2h_src[i].resize(part_num);
            part_a_p2h_dst[i].resize(part_num);
            part_a_p2h_length[i].resize(part_num);

            for (size_t k = 0; k < 3; k++) {
                if (part_wr[0][k] != -1) {
                    part_a_d2p_arg_buf_idx[i].push_back(k + 7);
                    for (size_t j = 0; j < part_num; j++) {
                        part_a_d2p_arg_head[i][j].push_back(j * sizeof(ap_uint<512>) * part_a_col_part_depth);
                        part_a_p2h_src[i][j].push_back(worker[w_id].sub_buf_host_parent[0][p_id][k + 7] +
                                                       worker[w_id].sub_buf_head[0][p_id][k + 7] +
                                                       j * sizeof(ap_uint<512>) * part_a_col_part_depth);
                        part_a_p2h_dst[i][j].push_back(tsk.tab_part_a->getColPointer(j, part_wr[0][k]));
                        part_a_p2h_length[i][j].push_back(0);

                        // part_a_p2h_bias is unique records, only init to zero at first section
                        if (i == 0) {
                            part_a_p2h_bias[j].push_back(0);
                        }
                    }
                }
            }
            // 1.4 add task
            // 1.4.1 h2p
            if (i >= (wkn * 2)) {
                part_a_h2p_evt_dep[i].push_back(part_a_p2d_evt[i - (2 * wkn)]);
            }
            h2p.addTask(part_a_h2p_evt_dep[i].data(), part_a_h2p_evt_dep[i].size(), &part_a_h2p_evt[i],
                        &part_a_h2p_src[i], &part_a_h2p_dst[i], &part_a_h2p_bias[i], &part_a_h2p_length[i],
                        &part_a_h2p_acc[i]);
            // 1.4.2 p2d
            part_a_p2d_evt_dep[i].push_back(part_a_h2p_evt[i]);
            if (i >= (wkn * 2)) {
                part_a_p2d_evt_dep[i].push_back(part_a_krn_evt[i - (2 * wkn)]);
            }
            worker[w_id].MigrateToDevice(WorkerFunctions::JOIN, 0, p_id, &part_a_p2d_arg_buf_idx[i],
                                         &part_a_p2d_arg_size[i], part_a_p2d_evt_dep[i].data(),
                                         part_a_p2d_evt_dep[i].size(), &part_a_p2d_evt[i]);
            // 1.4.3 krn
            part_a_krn_evt_dep[i].push_back(part_a_p2d_evt[i]);
            if (i >= wkn) {
                part_a_krn_evt_dep[i].push_back(part_a_krn_evt[i - wkn]);
            }
            if (i >= (wkn * 2)) {
                part_a_krn_evt_dep[i].push_back(part_a_d2p_evt[i - (2 * wkn)]);
            }
            worker[w_id].runKernel(WorkerFunctions::JOIN, 1, p_id, part_a_krn_scalar_arg[i],
                                   part_a_krn_evt_dep[i].data(), part_a_krn_evt_dep[i].size(), &part_a_krn_evt[i]);
            // 1.4.4 meta
            part_a_meta_evt_dep[i].push_back(part_a_krn_evt[i]);
            if (i >= (wkn * 2)) {
                part_a_meta_evt_dep[i].push_back(part_a_d2p_evt[i - (2 * wkn)]);
            }
            worker[w_id].MigrateMetaToHost(WorkerFunctions::JOIN, 0, p_id, part_a_meta_evt_dep[i].data(),
                                           part_a_meta_evt_dep[i].size(), &part_a_meta_evt[i]);
            // 1.4.5 d2p
            part_a_d2p_evt_dep[i].push_back(part_a_meta_evt[i]);
            if (i >= (wkn * 2)) {
                part_a_d2p_evt_dep[i].push_back(part_a_p2h_evt[i - (wkn * 2)]);
            }
            worker[w_id].MigrateResToHost(WorkerFunctions::JOIN, 0, p_id, &part_a_d2p_arg_buf_idx[i],
                                          &part_a_d2p_arg_head[i], &part_a_p2h_length[i], &part_a_d2p_res_row[i],
                                          part_a_d2p_evt_dep[i].data(), part_a_d2p_evt_dep[i].size(),
                                          &part_a_d2p_evt[i]);
            // 1.4.6 p2h
            part_a_p2h_evt_dep[i].push_back(part_a_d2p_evt[i]);
            p2h.addTask(part_a_p2h_evt_dep[i].data(), part_a_p2h_evt_dep[i].size(), &part_a_p2h_evt[i],
                        &part_a_p2h_src[i], &part_a_p2h_dst[i], &part_a_p2h_bias, &part_a_p2h_length[i],
                        &part_a_p2h_acc);
        }
        // 1.5 wait for event
        clWaitForEvents(part_a_p2h_evt.size(), part_a_p2h_evt.data());

        // 2. partition tab_b => tab_part_b
        // 2.1 event and dependence
        vector<cl_event> part_b_h2p_evt(tab_b_sec_num);
        vector<cl_event> part_b_p2d_evt(tab_b_sec_num);
        vector<cl_event> part_b_krn_evt(tab_b_sec_num);
        vector<cl_event> part_b_meta_evt(tab_b_sec_num);
        vector<cl_event> part_b_d2p_evt(tab_b_sec_num);
        vector<cl_event> part_b_p2h_evt(tab_b_sec_num);
        for (size_t i = 0; i < tab_b_sec_num; i++) {
            part_b_h2p_evt[i] = clCreateUserEvent(ctx, &err);
            part_b_d2p_evt[i] = clCreateUserEvent(ctx, &err);
            part_b_p2h_evt[i] = clCreateUserEvent(ctx, &err);
        }
        vector<vector<cl_event> > part_b_h2p_evt_dep(tab_b_sec_num);
        vector<vector<cl_event> > part_b_p2d_evt_dep(tab_b_sec_num);
        vector<vector<cl_event> > part_b_krn_evt_dep(tab_b_sec_num);
        vector<vector<cl_event> > part_b_meta_evt_dep(tab_b_sec_num);
        vector<vector<cl_event> > part_b_d2p_evt_dep(tab_b_sec_num);
        vector<vector<cl_event> > part_b_p2h_evt_dep(tab_b_sec_num);
        // 2.2 h2p, p2d, krn, meta, d2p, p2h args
        // 2.2.1 h2p
        vector<vector<vector<char*> > > part_b_h2p_src(tab_b_sec_num);
        vector<vector<vector<char*> > > part_b_h2p_dst(tab_b_sec_num);
        vector<vector<vector<size_t> > > part_b_h2p_bias(tab_b_sec_num);
        vector<vector<vector<size_t> > > part_b_h2p_length(tab_b_sec_num);
        vector<vector<vector<size_t> > > part_b_h2p_acc(tab_b_sec_num);
        vector<MetaTable> meta_part_b_in(tab_b_sec_num);
        vector<MetaTable> meta_part_b_out(tab_b_sec_num);
        // 2.2.2 p2d
        vector<vector<size_t> > part_b_p2d_arg_buf_idx(tab_b_sec_num);
        vector<vector<size_t> > part_b_p2d_arg_size(tab_b_sec_num);
        // 2.2.3 krn
        vector<vector<size_t> > part_b_krn_scalar_arg(tab_b_sec_num);
        // 2.2.4 meta, no extra args
        // 2.2.5 d2p
        vector<vector<size_t> > part_b_d2p_arg_buf_idx(tab_b_sec_num);
        vector<vector<vector<size_t> > > part_b_d2p_arg_head(tab_b_sec_num);
        // vector<vector<vector<size_t> > > part_b_d2p_arg_size(tab_b_sec_num);
        // d2p's will write to part_b_p2h_length
        vector<vector<size_t> > part_b_d2p_res_row(tab_b_sec_num);
        // 2.2.6 p2h
        vector<vector<vector<char*> > > part_b_p2h_src(tab_b_sec_num);
        vector<vector<vector<char*> > > part_b_p2h_dst(tab_b_sec_num);
        vector<vector<size_t> > part_b_p2h_bias(part_num);
        vector<vector<vector<size_t> > > part_b_p2h_length(tab_b_sec_num);
        vector<vector<size_t> >& part_b_p2h_acc = part_b_p2h_bias;
        // 2.3 check tab_b's sec ready one by one
        vector<size_t> tab_b_sec_row(tab_b_sec_num);
        for (size_t i = 0; i < tab_b_sec_num; i++) {
            tab_b_sec_row[i] = (*tsk.tab_b_sec_ready)[i].get();
            size_t w_id = i % wkn;
            size_t p_id = (i / wkn) % 2;

            // 2.3.1 h2p, p2d, krn
            // 2.3.1.1 din_col0, 1, 2
            part_b_h2p_src[i].resize(1);
            part_b_h2p_dst[i].resize(1);
            part_b_h2p_bias[i].resize(1);
            part_b_h2p_length[i].resize(1);
            part_b_h2p_acc[i].resize(1);
            for (size_t k = 0; k < 3; k++) {
                if (part_scan[1][k] != -1) {
                    part_b_h2p_src[i][0].push_back(tsk.tab_b->getColPointer(i, part_scan[1][k]));
                    part_b_h2p_dst[i][0].push_back(worker[w_id].sub_buf_host_parent[0][p_id][k] +
                                                   worker[w_id].sub_buf_head[0][p_id][k]);
                    part_b_h2p_bias[i][0].push_back(0);
                    part_b_h2p_length[i][0].push_back(tsk.tab_b->getColTypeSize(part_scan[1][k]) * tab_b_sec_row[i]);
                    part_b_h2p_acc[i][0].push_back(0);
                    part_b_p2d_arg_buf_idx[i].push_back(k);
                    part_b_p2d_arg_size[i].push_back(tsk.tab_b->getColTypeSize(part_scan[1][k]) * tab_b_sec_row[i]);
                }
            }
            // 2.3.1.2 din_val
            if (tsk.tab_b->getValidEnableFlag()) {
                part_b_h2p_src[i][0].push_back(tsk.tab_b->getValColPointer(i));
                part_b_h2p_dst[i][0].push_back(worker[w_id].sub_buf_host_parent[0][p_id][3] +
                                               worker[w_id].sub_buf_head[0][p_id][3]);
                part_b_h2p_bias[i][0].push_back(0);
                part_b_h2p_length[i][0].push_back((tab_b_sec_row[i] + 7) / 8);
                part_b_h2p_acc[i][0].push_back(0);
                part_b_p2d_arg_buf_idx[i].push_back(3);
                part_b_p2d_arg_size[i].push_back((tab_b_sec_row[i] + 7) / 8);
            }
            // 2.3.1.3 din_krn_cfg
            part_b_h2p_src[i][0].push_back((char*)part_cfg);
            part_b_h2p_dst[i][0].push_back(worker[w_id].sub_buf_host_parent[0][p_id][4] +
                                           worker[w_id].sub_buf_head[0][p_id][4]);
            part_b_h2p_bias[i][0].push_back(0);
            part_b_h2p_length[i][0].push_back(sizeof(ap_uint<512>) * 14);
            part_b_h2p_acc[i][0].push_back(0);
            part_b_p2d_arg_buf_idx[i].push_back(4);
            part_b_p2d_arg_size[i].push_back(sizeof(ap_uint<512>) * 14);
            // 2.3.1.4 din_meta
            meta_part_b_in[i].setSecID(i);
            meta_part_b_in[i].setColNum(3);
            for (size_t k = 0; k < 3; k++) {
                meta_part_b_in[i].setCol(k, k, tab_b_sec_row[i]);
            }
            part_b_h2p_src[i][0].push_back((char*)meta_part_b_in[i].meta());
            part_b_h2p_dst[i][0].push_back(worker[w_id].sub_buf_host_parent[0][p_id][5] +
                                           worker[w_id].sub_buf_head[0][p_id][5]);
            part_b_h2p_bias[i][0].push_back(0);
            part_b_h2p_length[i][0].push_back(sizeof(ap_uint<512>) * 24);
            part_b_h2p_acc[i][0].push_back(0);
            part_b_p2d_arg_buf_idx[i].push_back(5);
            part_b_p2d_arg_size[i].push_back(sizeof(ap_uint<512>) * 24);
            // 2.3.1.5 dout_meta
            // meta_part_b_out[i].setSecID(0);
            meta_part_b_out[i].setColNum(3);
            // tab_part_b's all sections should have same row num.
            // tab_part_b's all sections should have same row num
            size_t sub_buf_depth = worker[w_id].sub_buf_size[0][p_id][7] / 64; // how many 512bit rows
            size_t part_b_col_depth_init = sub_buf_depth;
            size_t part_b_col_part_depth = part_b_col_depth_init / part_num;
            meta_part_b_out[i].setPartition(part_num, part_b_col_part_depth);
            for (size_t k = 0; k < 3; k++) {
                meta_part_b_out[i].setCol(k, k, part_b_col_depth_init);
            }

            part_b_h2p_src[i][0].push_back((char*)meta_part_b_out[i].meta());
            part_b_h2p_dst[i][0].push_back(worker[w_id].sub_buf_host_parent[0][p_id][6] +
                                           worker[w_id].sub_buf_head[0][p_id][6]);
            part_b_h2p_bias[i][0].push_back(0);
            part_b_h2p_length[i][0].push_back(sizeof(ap_uint<512>) * 24);
            part_b_h2p_acc[i][0].push_back(0);
            part_b_p2d_arg_buf_idx[i].push_back(6);
            part_b_p2d_arg_size[i].push_back(sizeof(ap_uint<512>) * 24);

            // 2.3.2 krn
            const int k_depth = 512;
            const int idx_o = 1;
            part_b_krn_scalar_arg[i].push_back(k_depth);
            part_b_krn_scalar_arg[i].push_back(idx_o);
            part_b_krn_scalar_arg[i].push_back(params.log_part);
            // 2.3.3 meta
            // 2.3.4 d2p, p2h
            part_b_d2p_arg_head[i].resize(part_num);
            part_b_d2p_res_row[i].resize(part_num, 0);
            // part_b_d2p_arg_buf_idx
            // d2p's will write to part_b_p2h_length

            part_b_p2h_src[i].resize(part_num);
            part_b_p2h_dst[i].resize(part_num);
            part_b_p2h_length[i].resize(part_num);

            for (size_t k = 0; k < 3; k++) {
                if (part_wr[1][k] != -1) {
                    part_b_d2p_arg_buf_idx[i].push_back(k + 7);
                    for (size_t j = 0; j < part_num; j++) {
                        part_b_d2p_arg_head[i][j].push_back(j * sizeof(ap_uint<512>) * part_b_col_part_depth);
                        part_b_p2h_src[i][j].push_back(worker[w_id].sub_buf_host_parent[0][p_id][k + 7] +
                                                       worker[w_id].sub_buf_head[0][p_id][k + 7] +
                                                       j * sizeof(ap_uint<512>) * part_b_col_part_depth);
                        part_b_p2h_dst[i][j].push_back(tsk.tab_part_b->getColPointer(j, part_wr[1][k]));
                        part_b_p2h_length[i][j].push_back(0);

                        // part_b_p2h_bias is unique records, only init to zero at first section
                        if (i == 0) {
                            part_b_p2h_bias[j].push_back(0);
                        }
                    }
                }
            }
            // 2.4 add task
            // 2.4.1 h2p
            if (i >= (wkn * 2)) {
                part_b_h2p_evt_dep[i].push_back(part_b_p2d_evt[i - (2 * wkn)]);
            }
            h2p.addTask(part_b_h2p_evt_dep[i].data(), part_b_h2p_evt_dep[i].size(), &part_b_h2p_evt[i],
                        &part_b_h2p_src[i], &part_b_h2p_dst[i], &part_b_h2p_bias[i], &part_b_h2p_length[i],
                        &part_b_h2p_acc[i]);
            // 2.4.2 p2d
            part_b_p2d_evt_dep[i].push_back(part_b_h2p_evt[i]);
            if (i >= (wkn * 2)) {
                part_b_p2d_evt_dep[i].push_back(part_b_krn_evt[i - (2 * wkn)]);
            }
            worker[w_id].MigrateToDevice(WorkerFunctions::JOIN, 0, p_id, &part_b_p2d_arg_buf_idx[i],
                                         &part_b_p2d_arg_size[i], part_b_p2d_evt_dep[i].data(),
                                         part_b_p2d_evt_dep[i].size(), &part_b_p2d_evt[i]);
            // 2.4.3 krn
            part_b_krn_evt_dep[i].push_back(part_b_p2d_evt[i]);
            if (i >= wkn) {
                part_b_krn_evt_dep[i].push_back(part_b_krn_evt[i - wkn]);
            }
            if (i >= (wkn * 2)) {
                part_b_krn_evt_dep[i].push_back(part_b_d2p_evt[i - (2 * wkn)]);
            }
            worker[w_id].runKernel(WorkerFunctions::JOIN, 1, p_id, part_b_krn_scalar_arg[i],
                                   part_b_krn_evt_dep[i].data(), part_b_krn_evt_dep[i].size(), &part_b_krn_evt[i]);
            // 2.4.4 meta
            part_b_meta_evt_dep[i].push_back(part_b_krn_evt[i]);
            if (i >= (wkn * 2)) {
                part_b_meta_evt_dep[i].push_back(part_b_d2p_evt[i - (2 * wkn)]);
            }
            worker[w_id].MigrateMetaToHost(WorkerFunctions::JOIN, 0, p_id, part_b_meta_evt_dep[i].data(),
                                           part_b_meta_evt_dep[i].size(), &part_b_meta_evt[i]);
            // 2.4.5 d2p
            part_b_d2p_evt_dep[i].push_back(part_b_meta_evt[i]);
            if (i >= (wkn * 2)) {
                part_b_d2p_evt_dep[i].push_back(part_b_p2h_evt[i - (wkn * 2)]);
            }
            worker[w_id].MigrateResToHost(WorkerFunctions::JOIN, 0, p_id, &part_b_d2p_arg_buf_idx[i],
                                          &part_b_d2p_arg_head[i], &part_b_p2h_length[i], &part_b_d2p_res_row[i],
                                          part_b_d2p_evt_dep[i].data(), part_b_d2p_evt_dep[i].size(),
                                          &part_b_d2p_evt[i]);
            // 2.4.6 p2h
            part_b_p2h_evt_dep[i].push_back(part_b_d2p_evt[i]);
            p2h.addTask(part_b_p2h_evt_dep[i].data(), part_b_p2h_evt_dep[i].size(), &part_b_p2h_evt[i],
                        &part_b_p2h_src[i], &part_b_p2h_dst[i], &part_b_p2h_bias, &part_b_p2h_length[i],
                        &part_b_p2h_acc);
        }
        // 2.5 wait for event
        clWaitForEvents(part_b_p2h_evt.size(), part_b_p2h_evt.data());
        // XXX

        // 3. build tab_part_a, probe tab_part_b
        // 3.1 event and dependence
        vector<cl_event> build_h2p_evt(part_num);
        vector<cl_event> build_p2d_evt(part_num);
        vector<cl_event> build_krn_evt(part_num);
        for (size_t i = 0; i < part_num; i++) {
            build_h2p_evt[i] = clCreateUserEvent(ctx, &err);
        }
        vector<vector<cl_event> > build_h2p_evt_dep(part_num);
        vector<vector<cl_event> > build_p2d_evt_dep(part_num);
        vector<vector<cl_event> > build_krn_evt_dep(part_num);

        vector<cl_event> probe_h2p_evt(part_num);
        vector<cl_event> probe_p2d_evt(part_num);
        vector<cl_event> probe_krn_evt(part_num);
        vector<cl_event> probe_meta_evt(part_num);
        vector<cl_event> probe_d2p_evt(part_num);
        vector<cl_event> probe_p2h_evt(part_num);
        for (size_t i = 0; i < part_num; i++) {
            probe_h2p_evt[i] = clCreateUserEvent(ctx, &err);
            probe_d2p_evt[i] = clCreateUserEvent(ctx, &err);
            probe_p2h_evt[i] = clCreateUserEvent(ctx, &err);
        }
        vector<vector<cl_event> > probe_h2p_evt_dep(part_num);
        vector<vector<cl_event> > probe_p2d_evt_dep(part_num);
        vector<vector<cl_event> > probe_krn_evt_dep(part_num);
        vector<vector<cl_event> > probe_meta_evt_dep(part_num);
        vector<vector<cl_event> > probe_d2p_evt_dep(part_num);
        vector<vector<cl_event> > probe_p2h_evt_dep(part_num);
        // 3.2 build h2p, p2d, krn args
        vector<vector<vector<char*> > > build_h2p_src(part_num);
        vector<vector<vector<char*> > > build_h2p_dst(part_num);
        vector<vector<vector<size_t> > > build_h2p_bias(part_num);
        vector<vector<vector<size_t> > > build_h2p_length(part_num);
        vector<vector<vector<size_t> > > build_h2p_acc(part_num);
        vector<MetaTable> meta_build_in(part_num);
        vector<vector<size_t> > build_p2d_arg_buf_idx(part_num);
        vector<vector<size_t> > build_p2d_arg_size(part_num);
        vector<vector<size_t> > build_krn_scalar_arg(part_num);
        // 3.3 probe h2p, p2d, krn, meta, d2p, p2h args
        // 3.3.1 h2p
        vector<vector<vector<char*> > > probe_h2p_src(part_num);
        vector<vector<vector<char*> > > probe_h2p_dst(part_num);
        vector<vector<vector<size_t> > > probe_h2p_bias(part_num);
        vector<vector<vector<size_t> > > probe_h2p_length(part_num);
        vector<vector<vector<size_t> > > probe_h2p_acc(part_num);
        vector<MetaTable> meta_probe_in(part_num);
        vector<MetaTable> meta_probe_out(part_num);
        // 3.3.2 p2d
        vector<vector<size_t> > probe_p2d_arg_buf_idx(part_num);
        vector<vector<size_t> > probe_p2d_arg_size(part_num);
        // 3.3.3 krn
        vector<vector<size_t> > probe_krn_scalar_arg(part_num);
        // 3.3.4 meta, no extra args
        // 3.3.5 d2p
        vector<vector<size_t> > probe_d2p_arg_buf_idx(part_num);
        vector<vector<vector<size_t> > > probe_d2p_arg_head(part_num);
        vector<vector<size_t> > probe_d2p_res_row(part_num);
        // 3.3.6 p2h
        vector<vector<vector<char*> > > probe_p2h_src(part_num);
        vector<vector<vector<char*> > > probe_p2h_dst(part_num);
        vector<vector<vector<size_t> > > probe_p2h_bias(part_num);
        vector<vector<vector<size_t> > > probe_p2h_length(part_num);
        vector<vector<vector<size_t> > > probe_p2h_acc(part_num);
        // 3.3.7 part_a, part_b rows
        vector<size_t> part_a_rows(part_num);
        vector<size_t> part_b_rows(part_num);
        // 3.4 add task
        for (size_t i = 0; i < part_num; i++) {
            size_t w_id = i % wkn;
            size_t p_id = (i / wkn) % 2;
            //
            part_a_rows[i] = part_a_p2h_bias[i][0] / 8; // bias in byte, rows in 8 bytes
            part_b_rows[i] = part_b_p2h_bias[i][0] / 8; // bias in byte, rows in 8 bytes

            build_h2p_src[i].resize(1);
            build_h2p_dst[i].resize(1);
            build_h2p_bias[i].resize(1);
            build_h2p_length[i].resize(1);
            build_h2p_acc[i].resize(1);
            // build din_col0,1,2
            for (size_t k = 0; k < 3; k++) {
                if (join_scan[0][k] != -1) {
                    build_h2p_src[i][0].push_back(tsk.tab_part_a->getColPointer(i, join_scan[0][k]));
                    build_h2p_dst[i][0].push_back(worker[w_id].sub_buf_host_parent[1][p_id][k] +
                                                  worker[w_id].sub_buf_head[1][p_id][k]);
                    build_h2p_bias[i][0].push_back(0);
                    build_h2p_length[i][0].push_back(tsk.tab_part_a->getColTypeSize(join_scan[0][k]) * part_a_rows[i]);
                    build_h2p_acc[i][0].push_back(0);
                    build_p2d_arg_buf_idx[i].push_back(k);
                    build_p2d_arg_size[i].push_back(tsk.tab_part_a->getColTypeSize(join_scan[0][k]) * part_a_rows[i]);
                }
            }
            // build din_val, not use in solution 2
            // build din_krn_cfg
            build_h2p_src[i][0].push_back((char*)join_cfg);
            build_h2p_dst[i][0].push_back(worker[w_id].sub_buf_host_parent[1][p_id][4] +
                                          worker[w_id].sub_buf_head[1][p_id][4]);
            build_h2p_bias[i][0].push_back(0);
            build_h2p_length[i][0].push_back(sizeof(ap_uint<512>) * 14);
            build_h2p_acc[i][0].push_back(0);
            build_p2d_arg_buf_idx[i].push_back(4);
            build_p2d_arg_size[i].push_back(sizeof(ap_uint<512>) * 14);
            // build din_meta_in
            meta_build_in[i].setSecID(i);
            meta_build_in[i].setColNum(3);
            for (size_t k = 0; k < 3; k++) {
                meta_build_in[i].setCol(k, k, part_a_rows[i]);
            }
            build_h2p_src[i][0].push_back((char*)meta_build_in[i].meta());
            build_h2p_dst[i][0].push_back(worker[w_id].sub_buf_host_parent[1][p_id][5] +
                                          worker[w_id].sub_buf_head[1][p_id][5]);
            build_h2p_bias[i][0].push_back(0);
            build_h2p_length[i][0].push_back(sizeof(ap_uint<512>) * 24);
            build_h2p_acc[i][0].push_back(0);
            build_p2d_arg_buf_idx[i].push_back(5);
            build_p2d_arg_size[i].push_back(sizeof(ap_uint<512>) * 24);
            // build scalar arg
            build_krn_scalar_arg[i].resize(1);
            build_krn_scalar_arg[i][0] = 0;
            //
            probe_h2p_src[i].resize(1);
            probe_h2p_dst[i].resize(1);
            probe_h2p_bias[i].resize(1);
            probe_h2p_length[i].resize(1);
            probe_h2p_acc[i].resize(1);
            // probe din_col0,1,2
            for (size_t k = 0; k < 3; k++) {
                if (join_scan[1][k] != -1) {
                    probe_h2p_src[i][0].push_back(tsk.tab_part_b->getColPointer(i, join_scan[1][k]));
                    probe_h2p_dst[i][0].push_back(worker[w_id].sub_buf_host_parent[1][p_id][k] +
                                                  worker[w_id].sub_buf_head[1][p_id][k]);
                    probe_h2p_bias[i][0].push_back(0);
                    probe_h2p_length[i][0].push_back(tsk.tab_part_b->getColTypeSize(join_scan[1][k]) * part_b_rows[i]);
                    probe_h2p_acc[i][0].push_back(0);
                    probe_p2d_arg_buf_idx[i].push_back(k);
                    probe_p2d_arg_size[i].push_back(tsk.tab_part_b->getColTypeSize(join_scan[1][k]) * part_b_rows[i]);
                }
            }
            // probe din_val, not use
            // probe din_krn_cfg
            probe_h2p_src[i][0].push_back((char*)join_cfg);
            probe_h2p_dst[i][0].push_back(worker[w_id].sub_buf_host_parent[1][p_id][4] +
                                          worker[w_id].sub_buf_head[1][p_id][4]);
            probe_h2p_bias[i][0].push_back(0);
            probe_h2p_length[i][0].push_back(sizeof(ap_uint<512>) * 14);
            probe_h2p_acc[i][0].push_back(0);
            probe_p2d_arg_buf_idx[i].push_back(4);
            probe_p2d_arg_size[i].push_back(sizeof(ap_uint<512>) * 14);
            // probe din_meta_in
            meta_probe_in[i].setSecID(i);
            meta_probe_in[i].setColNum(3);
            for (size_t k = 0; k < 3; k++) {
                meta_probe_in[i].setCol(k, k, part_b_rows[i]);
            }
            probe_h2p_src[i][0].push_back((char*)meta_probe_in[i].meta());
            probe_h2p_dst[i][0].push_back(worker[w_id].sub_buf_host_parent[1][p_id][5] +
                                          worker[w_id].sub_buf_head[1][p_id][5]);
            probe_h2p_bias[i][0].push_back(0);
            probe_h2p_length[i][0].push_back(sizeof(ap_uint<512>) * 24);
            probe_h2p_acc[i][0].push_back(0);
            probe_p2d_arg_buf_idx[i].push_back(5);
            probe_p2d_arg_size[i].push_back(sizeof(ap_uint<512>) * 24);
            // probe din_meta_out
            meta_probe_out[i].setSecID(0);
            meta_probe_out[i].setColNum(4);
            for (size_t k = 0; k < 4; k++) {
                meta_probe_out[i].setCol(k, k, tsk.tab_c->getSecRowNum(i));
            }
            probe_h2p_src[i][0].push_back((char*)meta_probe_out[i].meta());
            probe_h2p_dst[i][0].push_back(worker[w_id].sub_buf_host_parent[1][p_id][6] +
                                          worker[w_id].sub_buf_head[1][p_id][6]);
            probe_h2p_bias[i][0].push_back(0);
            probe_h2p_length[i][0].push_back(sizeof(ap_uint<512>) * 24);
            probe_h2p_acc[i][0].push_back(0);
            probe_p2d_arg_buf_idx[i].push_back(6);
            probe_p2d_arg_size[i].push_back(sizeof(ap_uint<512>) * 24);
            // probe krn arg
            probe_krn_scalar_arg[i].push_back(1);
            // probe meta
            // probe d2p, p2h
            probe_d2p_res_row[i].resize(1, 0);
            probe_d2p_arg_head[i].resize(1);

            probe_p2h_src[i].resize(1);
            probe_p2h_dst[i].resize(1);
            probe_p2h_bias[i].resize(1);
            probe_p2h_length[i].resize(1);
            probe_p2h_acc[i].resize(1);
            for (size_t k = 0; k < 4; k++) {
                if (join_wr[k] != -1) {
                    probe_d2p_arg_buf_idx[i].push_back(k + 7);
                    probe_d2p_arg_head[i][0].push_back(0);
                    probe_p2h_src[i][0].push_back(worker[w_id].sub_buf_host_parent[1][p_id][k + 7] +
                                                  worker[w_id].sub_buf_head[1][p_id][k + 7]);
                    probe_p2h_dst[i][0].push_back(tsk.tab_c->getColPointer(i, join_wr[k]));
                    probe_p2h_bias[i][0].push_back(0);
                    probe_p2h_length[i][0].push_back(0); // will be write by d2p and read by p2h
                    probe_p2h_acc[i][0].push_back(0);
                }
            }
            // start to add task
            // build h2p
            if (i >= (2 * wkn)) {
                build_h2p_evt_dep[i].push_back(probe_p2d_evt[i - (2 * wkn)]);
            }
            h2p.addTask(build_h2p_evt_dep[i].data(), build_h2p_evt_dep[i].size(), &build_h2p_evt[i], &build_h2p_src[i],
                        &build_h2p_dst[i], &build_h2p_bias[i], &build_h2p_length[i], &build_h2p_acc[i]);
            // build p2d
            build_p2d_evt_dep[i].push_back(build_h2p_evt[i]);
            if (i >= (2 * wkn)) {
                build_p2d_evt_dep[i].push_back(probe_krn_evt[i - (2 * wkn)]);
            }
            worker[w_id].MigrateToDevice(WorkerFunctions::JOIN, 1, p_id, &build_p2d_arg_buf_idx[i],
                                         &build_p2d_arg_size[i], build_p2d_evt_dep[i].data(),
                                         build_p2d_evt_dep[i].size(), &build_p2d_evt[i]);
            // build krn
            build_krn_evt_dep[i].push_back(build_p2d_evt[i]);
            if (i >= wkn) {
                build_krn_evt_dep[i].push_back(probe_krn_evt[i - wkn]);
            }
            worker[w_id].runKernel(WorkerFunctions::JOIN, 0, p_id, build_krn_scalar_arg[i], build_krn_evt_dep[i].data(),
                                   build_krn_evt_dep[i].size(), &build_krn_evt[i]);
            // probe h2p
            probe_h2p_evt_dep[i].push_back(build_p2d_evt[i]);
            h2p.addTask(probe_h2p_evt_dep[i].data(), probe_h2p_evt_dep[i].size(), &probe_h2p_evt[i], &probe_h2p_src[i],
                        &probe_h2p_dst[i], &probe_h2p_bias[i], &probe_h2p_length[i], &probe_h2p_acc[i]);
            // probe p2d
            probe_p2d_evt_dep[i].push_back(probe_h2p_evt[i]);
            probe_p2d_evt_dep[i].push_back(build_krn_evt[i]);
            worker[w_id].MigrateToDevice(WorkerFunctions::JOIN, 1, p_id, &probe_p2d_arg_buf_idx[i],
                                         &probe_p2d_arg_size[i], probe_p2d_evt_dep[i].data(),
                                         probe_p2d_evt_dep[i].size(), &probe_p2d_evt[i]);
            // probe krn
            probe_krn_evt_dep[i].push_back(probe_p2d_evt[i]);
            probe_krn_evt_dep[i].push_back(build_krn_evt[i]);
            if (i >= (2 * wkn)) {
                probe_krn_evt_dep[i].push_back(probe_d2p_evt[i - (2 * wkn)]);
            }
            worker[w_id].runKernel(WorkerFunctions::JOIN, 0, p_id, probe_krn_scalar_arg[i], probe_krn_evt_dep[i].data(),
                                   probe_krn_evt_dep[i].size(), &probe_krn_evt[i]);
            // probe meta
            probe_meta_evt_dep[i].push_back(probe_krn_evt[i]);
            if (i >= (2 * wkn)) {
                probe_meta_evt_dep[i].push_back(probe_d2p_evt[i - (2 * wkn)]);
            }
            worker[w_id].MigrateMetaToHost(WorkerFunctions::JOIN, 1, p_id, probe_meta_evt_dep[i].data(),
                                           probe_meta_evt_dep[i].size(), &probe_meta_evt[i]);
            // probe d2p
            probe_d2p_evt_dep[i].push_back(probe_meta_evt[i]);
            if (i >= (wkn * 2)) {
                probe_d2p_evt_dep[i].push_back(probe_p2h_evt[i - (2 * wkn)]);
            }
            worker[w_id].MigrateResToHost(WorkerFunctions::JOIN, 1, p_id, &probe_d2p_arg_buf_idx[i],
                                          &probe_d2p_arg_head[i], &probe_p2h_length[i], &probe_d2p_res_row[i],
                                          probe_d2p_evt_dep[i].data(), probe_d2p_evt_dep[i].size(), &probe_d2p_evt[i]);
            // probe p2h
            probe_p2h_evt_dep[i].push_back(probe_d2p_evt[i]);
            p2h.addTask(probe_p2h_evt_dep[i].data(), probe_p2h_evt_dep[i].size(), &probe_p2h_evt[i], &probe_p2h_src[i],
                        &probe_p2h_dst[i], &probe_p2h_bias[i], &probe_p2h_length[i], &probe_p2h_acc[i]);
        }
        // 3.5 wait for event
        for (size_t i = 0; i < probe_p2h_evt.size(); i++) {
            clWaitForEvents(1, &probe_p2h_evt[i]);
            (*tsk.tab_c_sec_ready_promise)[i].set_value(probe_d2p_res_row[i][0]);
        }
        // XXX
        // 4. release cl_event
        for (size_t i = 0; i < tab_a_sec_num; i++) {
            clReleaseEvent(part_a_h2p_evt[i]);
            clReleaseEvent(part_a_p2d_evt[i]);
            clReleaseEvent(part_a_krn_evt[i]);
            clReleaseEvent(part_a_meta_evt[i]);
            clReleaseEvent(part_a_d2p_evt[i]);
            clReleaseEvent(part_a_p2h_evt[i]);
        }
        for (size_t i = 0; i < tab_b_sec_num; i++) {
            clReleaseEvent(part_b_h2p_evt[i]);
            clReleaseEvent(part_b_p2d_evt[i]);
            clReleaseEvent(part_b_krn_evt[i]);
            clReleaseEvent(part_b_meta_evt[i]);
            clReleaseEvent(part_b_d2p_evt[i]);
            clReleaseEvent(part_b_p2h_evt[i]);
        }
        for (size_t i = 0; i < part_num; i++) {
            clReleaseEvent(build_h2p_evt[i]);
            clReleaseEvent(build_p2d_evt[i]);
            clReleaseEvent(build_krn_evt[i]);

            clReleaseEvent(probe_h2p_evt[i]);
            clReleaseEvent(probe_p2d_evt[i]);
            clReleaseEvent(probe_krn_evt[i]);
            clReleaseEvent(probe_meta_evt[i]);
            clReleaseEvent(probe_d2p_evt[i]);
            clReleaseEvent(probe_p2h_evt[i]);
        }
    } else {
        cout << "params.sol = " << params.sol << ", not supported." << endl;
        cout << "only 1 and 2 are supported" << endl;
    }
}

void Workshop::processBF(task_complex tsk) {
    size_t wkn = worker.size();

    size_t tab_a_sec_num = tsk.tab_a->getSecNum();
    size_t tab_b_sec_num = tsk.tab_b->getSecNum();
    std::cout << "sec a = " << tab_a_sec_num << " sec b = " << tab_b_sec_num << std::endl;
    if (tab_a_sec_num != 1) {
        std::cout << "Sections number for tab A should be 1" << std::endl;
        exit(0);
    }

    BloomFilterConfig bf_build_cfg(*tsk.tab_a, tsk.filter_a, *tsk.tab_b, tsk.filter_b, tsk.join_str, *tsk.tab_c,
                                   tsk.output_str, false);
    BloomFilterConfig bf_probe_cfg(*tsk.tab_a, tsk.filter_a, *tsk.tab_b, tsk.filter_b, tsk.join_str, *tsk.tab_c,
                                   tsk.output_str, true);

    ap_uint<512>* build_cfg = bf_build_cfg.getBloomFilterConfigBits();
    ap_uint<512>* probe_cfg = bf_probe_cfg.getBloomFilterConfigBits();

    vector<vector<int8_t> > bf_scan = bf_build_cfg.getShuffleScan();
    vector<int8_t> bf_wr = bf_build_cfg.getShuffleWrite();

    // 1. build
    // 1.0 tab_a's sec[0] ready;
    vector<size_t> tab_a_sec_row(1);
    tab_a_sec_row[0] = (*tsk.tab_a_sec_ready)[0].get();
    // 1.1 events
    vector<cl_event> build_h2p_evt(wkn);
    vector<cl_event> build_p2d_evt(wkn);
    vector<cl_event> build_krn_evt(wkn);
    vector<vector<cl_event> > build_h2p_evt_dep(wkn);
    vector<vector<cl_event> > build_p2d_evt_dep(wkn);
    vector<vector<cl_event> > build_krn_evt_dep(wkn);
    for (size_t i = 0; i < wkn; i++) {
        build_h2p_evt[i] = clCreateUserEvent(ctx, &err);
    }

    // 1.2 h2p, p2d, krn args
    vector<vector<vector<char*> > > build_h2p_src(wkn);
    vector<vector<vector<char*> > > build_h2p_dst(wkn);
    vector<vector<vector<size_t> > > build_h2p_bias(wkn);
    vector<vector<vector<size_t> > > build_h2p_length(wkn);
    vector<vector<vector<size_t> > > build_h2p_acc(wkn);
    vector<MetaTable> meta_build_in(wkn);
    vector<vector<size_t> > build_p2d_arg_buf_idx(wkn);
    vector<vector<size_t> > build_p2d_arg_size(wkn);
    vector<vector<size_t> > build_krn_scalar_arg(wkn);
    for (size_t i = 0; i < wkn; i++) {
        build_h2p_src[i].resize(1);
        build_h2p_dst[i].resize(1);
        build_h2p_bias[i].resize(1);
        build_h2p_length[i].resize(1);
        build_h2p_acc[i].resize(1);
        // 1.2.1 din_col0,1,2
        for (size_t j = 0; j < 1; j++) {
            for (size_t k = 0; k < 3; k++) {
                if (bf_scan[0][k] != -1) {
                    build_h2p_src[i][j].push_back(tsk.tab_a->getColPointer(j, bf_scan[0][k]));
                    build_h2p_dst[i][j].push_back(worker[i].sub_buf_host_parent[1][0][k] +
                                                  worker[i].sub_buf_head[1][0][k]);
                    build_h2p_bias[i][j].push_back(0);
                    build_h2p_length[i][j].push_back(tsk.tab_a->getColTypeSize(bf_scan[0][k]) * tab_a_sec_row[j]);
                    build_h2p_acc[i][j].push_back(0);
                    build_p2d_arg_buf_idx[i].push_back(k);
                    build_p2d_arg_size[i].push_back(tsk.tab_a->getColTypeSize(bf_scan[0][k]) * tab_a_sec_row[j]);
                }
            }
        }

        // 1.2.2 din_val
        if (tsk.tab_a->getValidEnableFlag()) {
            for (size_t j = 0; j < 1; j++) {
                build_h2p_src[i][j].push_back(tsk.tab_a->getValColPointer(j));
                build_h2p_dst[i][j].push_back(worker[i].sub_buf_host_parent[1][0][3] + worker[i].sub_buf_head[1][0][3]);
                build_h2p_bias[i][j].push_back(0);
                build_h2p_length[i][j].push_back((tab_a_sec_row[j] + 7) / 8);
                build_h2p_acc[i][j].push_back(0);
                build_p2d_arg_buf_idx[i].push_back(3);
                build_p2d_arg_size[i].push_back((tab_a_sec_row[j] + 7) / 8);
            }
        }
        // 1.2.3 din_krn_cfg
        for (size_t j = 0; j < 1; j++) {
            build_h2p_src[i][j].push_back((char*)build_cfg);
            build_h2p_dst[i][j].push_back(worker[i].sub_buf_host_parent[1][0][4] + worker[i].sub_buf_head[1][0][4]);
            build_h2p_bias[i][j].push_back(0);
            build_h2p_length[i][j].push_back(sizeof(ap_uint<512>) * 14);
            build_h2p_acc[i][j].push_back(0);
            build_p2d_arg_buf_idx[i].push_back(4);
            build_p2d_arg_size[i].push_back(sizeof(ap_uint<512>) * 14);
        }
        // 1.2.4 din_meta_in
        meta_build_in[i].setSecID(0);
        meta_build_in[i].setColNum(3);
        for (size_t j = 0; j < 3; j++) {
            meta_build_in[i].setCol(j, j, tab_a_sec_row[0]);
        }
        for (size_t j = 0; j < 1; j++) {
            build_h2p_src[i][j].push_back((char*)meta_build_in[i].meta());
            build_h2p_dst[i][j].push_back(worker[i].sub_buf_host_parent[1][0][5] + worker[i].sub_buf_head[1][0][5]);
            build_h2p_bias[i][j].push_back(0);
            build_h2p_length[i][j].push_back(sizeof(ap_uint<512>) * 24);
            build_h2p_acc[i][j].push_back(0);
            build_p2d_arg_buf_idx[i].push_back(5);
            build_p2d_arg_size[i].push_back(sizeof(ap_uint<512>) * 24);
        }
        // 1.2.5 scalar_arg
        build_krn_scalar_arg[i].resize(1);
        build_krn_scalar_arg[i][0] = 0;
    }
    // 1.3 addTask
    // 1.3.1 h2p
    for (size_t i = 0; i < wkn; i++) {
        h2p.addTask(build_h2p_evt_dep[i].data(), build_h2p_evt_dep[i].size(), &build_h2p_evt[i], &build_h2p_src[i],
                    &build_h2p_dst[i], &build_h2p_bias[i], &build_h2p_length[i], &build_h2p_acc[i]);
    }
    // 1.3.2 p2d
    for (size_t i = 0; i < wkn; i++) {
        build_p2d_evt_dep[i].push_back(build_h2p_evt[i]);
        worker[i].MigrateToDevice(WorkerFunctions::JOIN, 0, 0, &build_p2d_arg_buf_idx[i], &build_p2d_arg_size[i],
                                  build_p2d_evt_dep[i].data(), build_p2d_evt_dep[i].size(), &build_p2d_evt[i]);
    }
    // 1.3.3 krn
    for (size_t i = 0; i < wkn; i++) {
        build_krn_evt_dep[i].push_back(build_p2d_evt[i]);
        worker[i].runKernel(WorkerFunctions::JOIN, 0, 0, build_krn_scalar_arg[i], build_krn_evt_dep[i].data(),
                            build_krn_evt_dep[i].size(), &build_krn_evt[i]);
    }
    // 1.4 wait
    clWaitForEvents(build_krn_evt.size(), build_krn_evt.data());

    // 2. probe
    vector<size_t> tab_b_sec_row(tab_b_sec_num);

    // 2.1 event and dependence
    vector<cl_event> probe_h2p_evt(tab_b_sec_num);
    vector<cl_event> probe_p2d_evt(tab_b_sec_num);
    vector<cl_event> probe_krn_evt(tab_b_sec_num);
    vector<cl_event> probe_meta_evt(tab_b_sec_num);
    vector<cl_event> probe_d2p_evt(tab_b_sec_num);
    vector<cl_event> probe_p2h_evt(tab_b_sec_num);
    for (size_t i = 0; i < tab_b_sec_num; i++) {
        probe_h2p_evt[i] = clCreateUserEvent(ctx, &err);
        probe_d2p_evt[i] = clCreateUserEvent(ctx, &err);
        probe_p2h_evt[i] = clCreateUserEvent(ctx, &err);
    }
    vector<vector<cl_event> > probe_h2p_evt_dep(tab_b_sec_num);
    vector<vector<cl_event> > probe_p2d_evt_dep(tab_b_sec_num);
    vector<vector<cl_event> > probe_krn_evt_dep(tab_b_sec_num);
    vector<vector<cl_event> > probe_meta_evt_dep(tab_b_sec_num);
    vector<vector<cl_event> > probe_d2p_evt_dep(tab_b_sec_num);
    vector<vector<cl_event> > probe_p2h_evt_dep(tab_b_sec_num);

    // 2.2 h2p, p2d, krn, meta, d2p, p2h args
    // 2.2.1 h2p
    vector<vector<vector<char*> > > probe_h2p_src(tab_b_sec_num);
    vector<vector<vector<char*> > > probe_h2p_dst(tab_b_sec_num);
    vector<vector<vector<size_t> > > probe_h2p_bias(tab_b_sec_num);
    vector<vector<vector<size_t> > > probe_h2p_length(tab_b_sec_num);
    vector<vector<vector<size_t> > > probe_h2p_acc(tab_b_sec_num);
    vector<MetaTable> meta_probe_in(tab_b_sec_num);
    vector<MetaTable> meta_probe_out(tab_b_sec_num);
    // 2.2.2 p2d
    vector<vector<size_t> > probe_p2d_arg_buf_idx(tab_b_sec_num);
    vector<vector<size_t> > probe_p2d_arg_size(tab_b_sec_num);
    // 2.2.3 krn
    vector<vector<size_t> > probe_krn_scalar_arg(tab_b_sec_num);
    // 2.2.4 meta, no extra args
    // 2.2.5 d2p
    vector<vector<size_t> > probe_d2p_arg_buf_idx(tab_b_sec_num);
    vector<vector<vector<size_t> > > probe_d2p_arg_head(tab_b_sec_num);
    // vector<vector<vector<size_t> > > probe_d2p_arg_size(tab_b_sec_num);
    // d2p's will write to probe_p2h_length
    vector<vector<size_t> > probe_d2p_res_row(tab_b_sec_num);
    // 2.2.6 p2h
    vector<vector<vector<char*> > > probe_p2h_src(tab_b_sec_num);
    vector<vector<vector<char*> > > probe_p2h_dst(tab_b_sec_num);
    vector<vector<vector<size_t> > > probe_p2h_bias(tab_b_sec_num);
    vector<vector<vector<size_t> > > probe_p2h_length(tab_b_sec_num);
    vector<vector<vector<size_t> > > probe_p2h_acc(tab_b_sec_num);

    // 2.3 check tab_b's sec ready one by one
    for (size_t i = 0; i < tab_b_sec_num; i++) {
        // 2.3.0. get tab_b's sec ready
        tab_b_sec_row[i] = (*tsk.tab_b_sec_ready)[i].get();
        size_t w_id = i % wkn;
        size_t p_id = (i / wkn) % 2;
        // 2.3.1 h2p, p2d, krn
        // 2.3.1.1 din_col0,1,2
        probe_h2p_src[i].resize(1);
        probe_h2p_dst[i].resize(1);
        probe_h2p_bias[i].resize(1);
        probe_h2p_length[i].resize(1);
        probe_h2p_acc[i].resize(1);
        for (size_t k = 0; k < 3; k++) {
            if (bf_scan[1][k] != -1) {
                probe_h2p_src[i][0].push_back(tsk.tab_b->getColPointer(i, bf_scan[1][k]));
                probe_h2p_dst[i][0].push_back(worker[w_id].sub_buf_host_parent[1][p_id][k] +
                                              worker[w_id].sub_buf_head[1][p_id][k]);
                probe_h2p_bias[i][0].push_back(0);
                probe_h2p_length[i][0].push_back(tsk.tab_b->getColTypeSize(bf_scan[1][k]) * tab_b_sec_row[i]);
                probe_h2p_acc[i][0].push_back(0);
                probe_p2d_arg_buf_idx[i].push_back(k);
                probe_p2d_arg_size[i].push_back(tsk.tab_b->getColTypeSize(bf_scan[1][k]) * tab_b_sec_row[i]);
            }
        }
        // 2.3.1.2 din_val
        if (tsk.tab_b->getValidEnableFlag()) {
            probe_h2p_src[i][0].push_back(tsk.tab_b->getValColPointer(i));
            probe_h2p_dst[i][0].push_back(worker[w_id].sub_buf_host_parent[1][p_id][3] +
                                          worker[w_id].sub_buf_head[1][p_id][3]);
            probe_h2p_bias[i][0].push_back(0);
            probe_h2p_length[i][0].push_back((tab_b_sec_row[i] + 7) / 8);
            probe_h2p_acc[i][0].push_back(0);
            probe_p2d_arg_buf_idx[i].push_back(3);
            probe_p2d_arg_size[i].push_back((tab_b_sec_row[i] + 7) / 8);
        }
        // 2.3.1.3 din_krn_cfg
        probe_h2p_src[i][0].push_back((char*)probe_cfg);
        probe_h2p_dst[i][0].push_back(worker[w_id].sub_buf_host_parent[1][p_id][4] +
                                      worker[w_id].sub_buf_head[1][p_id][4]);
        probe_h2p_bias[i][0].push_back(0);
        probe_h2p_length[i][0].push_back(sizeof(ap_uint<512>) * 14);
        probe_h2p_acc[i][0].push_back(0);
        probe_p2d_arg_buf_idx[i].push_back(4);
        probe_p2d_arg_size[i].push_back(sizeof(ap_uint<512>) * 14);
        // 2.3.1.4 din_meta_in
        meta_probe_in[i].setSecID(i);
        meta_probe_in[i].setColNum(3);
        for (size_t k = 0; k < 3; k++) {
            meta_probe_in[i].setCol(k, k, tab_b_sec_row[i]);
        }
        probe_h2p_src[i][0].push_back((char*)meta_probe_in[i].meta());
        probe_h2p_dst[i][0].push_back(worker[w_id].sub_buf_host_parent[1][p_id][5] +
                                      worker[w_id].sub_buf_head[1][p_id][5]);
        probe_h2p_bias[i][0].push_back(0);
        probe_h2p_length[i][0].push_back(sizeof(ap_uint<512>) * 24);
        probe_h2p_acc[i][0].push_back(0);
        probe_p2d_arg_buf_idx[i].push_back(5);
        probe_p2d_arg_size[i].push_back(sizeof(ap_uint<512>) * 24);
        // 2.3.1.5 din_meta_out
        meta_probe_out[i].setSecID(0);
        meta_probe_out[i].setColNum(4);
        for (size_t k = 0; k < 4; k++) {
            meta_probe_out[i].setCol(k, k, tsk.tab_c->getSecRowNum(i));
        }
        probe_h2p_src[i][0].push_back((char*)meta_probe_out[i].meta());
        probe_h2p_dst[i][0].push_back(worker[w_id].sub_buf_host_parent[1][p_id][6] +
                                      worker[w_id].sub_buf_head[1][p_id][6]);
        probe_h2p_bias[i][0].push_back(0);
        probe_h2p_length[i][0].push_back(sizeof(ap_uint<512>) * 24);
        probe_h2p_acc[i][0].push_back(0);
        probe_p2d_arg_buf_idx[i].push_back(6);
        probe_p2d_arg_size[i].push_back(sizeof(ap_uint<512>) * 24);
        // 2.3.2 krn
        probe_krn_scalar_arg[i].push_back(1);
        // 2.3.3 meta
        // 2.3.4 d2p, p2h
        probe_d2p_res_row[i].resize(1, 0);
        probe_d2p_arg_head[i].resize(1);

        probe_p2h_src[i].resize(1);
        probe_p2h_dst[i].resize(1);
        probe_p2h_bias[i].resize(1);
        probe_p2h_length[i].resize(1);
        probe_p2h_acc[i].resize(1);
        for (size_t k = 0; k < 4; k++) {
            if (bf_wr[k] != -1) {
                probe_d2p_arg_buf_idx[i].push_back(k + 7);
                probe_d2p_arg_head[i][0].push_back(0);

                probe_p2h_src[i][0].push_back(worker[w_id].sub_buf_host_parent[1][p_id][k + 7] +
                                              worker[w_id].sub_buf_head[1][p_id][k + 7]);
                probe_p2h_dst[i][0].push_back(tsk.tab_c->getColPointer(i, bf_wr[k]));
                probe_p2h_bias[i][0].push_back(0);
                probe_p2h_length[i][0].push_back(0); // will be write by d2p and read by p2h
                probe_p2h_acc[i][0].push_back(0);
            }
        }
        // 2.4 add Task
        // 2.4.1 h2p => p2d(-2)
        if (i >= (wkn * 2)) {
            probe_h2p_evt_dep[i].push_back(probe_p2d_evt[i - (2 * wkn)]);
        }
        h2p.addTask(probe_h2p_evt_dep[i].data(), probe_h2p_evt_dep[i].size(), &probe_h2p_evt[i], &probe_h2p_src[i],
                    &probe_h2p_dst[i], &probe_h2p_bias[i], &probe_h2p_length[i], &probe_h2p_acc[i]);
        // 2.4.2 p2d
        probe_p2d_evt_dep[i].push_back(probe_h2p_evt[i]);
        if (i >= (wkn * 2)) {
            probe_p2d_evt_dep[i].push_back(probe_krn_evt[i - (2 * wkn)]);
        }
        worker[w_id].MigrateToDevice(WorkerFunctions::JOIN, 0, p_id, &probe_p2d_arg_buf_idx[i], &probe_p2d_arg_size[i],
                                     probe_p2d_evt_dep[i].data(), probe_p2d_evt_dep[i].size(), &probe_p2d_evt[i]);
        // 2.4.3 krn
        probe_krn_evt_dep[i].push_back(probe_p2d_evt[i]);
        if (i >= wkn) {
            probe_krn_evt_dep[i].push_back(probe_krn_evt[i - wkn]);
        }
        if (i >= (wkn * 2)) {
            probe_krn_evt_dep[i].push_back(probe_d2p_evt[i - (2 * wkn)]);
        }
        worker[w_id].runKernel(WorkerFunctions::JOIN, 0, p_id, probe_krn_scalar_arg[i], probe_krn_evt_dep[i].data(),
                               probe_krn_evt_dep[i].size(), &probe_krn_evt[i]);
        // 2.4.4 meta
        probe_meta_evt_dep[i].push_back(probe_krn_evt[i]);
        if (i >= (wkn * 2)) {
            probe_meta_evt_dep[i].push_back(probe_d2p_evt[i - (2 * wkn)]);
        }
        worker[w_id].MigrateMetaToHost(WorkerFunctions::JOIN, 0, p_id, probe_meta_evt_dep[i].data(),
                                       probe_meta_evt_dep[i].size(), &probe_meta_evt[i]);
        // 2.4.5 d2p
        probe_d2p_evt_dep[i].push_back(probe_meta_evt[i]);
        if (i >= (wkn * 2)) {
            probe_d2p_evt_dep[i].push_back(probe_p2h_evt[i - (wkn * 2)]);
        }
        worker[w_id].MigrateResToHost(WorkerFunctions::JOIN, 0, p_id, &probe_d2p_arg_buf_idx[i], &probe_d2p_arg_head[i],
                                      &probe_p2h_length[i], &probe_d2p_res_row[i], probe_d2p_evt_dep[i].data(),
                                      probe_d2p_evt_dep[i].size(), &probe_d2p_evt[i]);
        // 2.4.6 p2h
        probe_p2h_evt_dep[i].push_back(probe_d2p_evt[i]);
        p2h.addTask(probe_p2h_evt_dep[i].data(), probe_p2h_evt_dep[i].size(), &probe_p2h_evt[i], &probe_p2h_src[i],
                    &probe_p2h_dst[i], &probe_p2h_bias[i], &probe_p2h_length[i], &probe_p2h_acc[i]);
    }
    // 2.5 wait for event
    for (size_t i = 0; i < probe_p2h_evt.size(); i++) {
        clWaitForEvents(1, &probe_p2h_evt[i]);
        (*tsk.tab_c_sec_ready_promise)[i].set_value(probe_d2p_res_row[i][0]);
    }

    // 3 release cl_event
    for (size_t i = 0; i < wkn; i++) {
        clReleaseEvent(build_h2p_evt[i]);
        clReleaseEvent(build_p2d_evt[i]);
        clReleaseEvent(build_krn_evt[i]);
    }
    for (size_t i = 0; i < tab_b_sec_num; i++) {
        clReleaseEvent(probe_h2p_evt[i]);
        clReleaseEvent(probe_p2d_evt[i]);
        clReleaseEvent(probe_krn_evt[i]);
        clReleaseEvent(probe_meta_evt[i]);
        clReleaseEvent(probe_d2p_evt[i]);
        clReleaseEvent(probe_p2h_evt[i]);
    }
}

void Workshop::checkJoinQueue() {
    while (join_service_run) {
        {
            unique_lock<mutex> lk(m);
            cv.wait(lk, [&] { return (!q.empty() || !join_service_run); });
        }

        while (!q.empty()) {
            task_complex tsk = q.front();
            if (tsk.tsk_type == JOIN_TASK) {
                processJoin(tsk);
            } else {
                processBF(tsk);
            }

            {
                unique_lock<mutex> lk(m);
                q.pop();
            }
        }
    }
    // clean up q's left task
    while (!q.empty()) {
        task_complex tsk = q.front();
        if (tsk.tsk_type == JOIN_TASK) {
            processJoin(tsk);
        } else {
            processBF(tsk);
        }

        {
            unique_lock<mutex> lk(m);
            q.pop();
        }
    }
}

} // gqe
} // database
} // xf
