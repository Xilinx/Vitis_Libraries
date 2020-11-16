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

// L2
#include "xf_database/meta_table.hpp"
// L3
#include "xf_database/gqe_join.hpp"

namespace xf {
namespace database {
namespace gqe {

Joiner::Joiner(std::string xclbin) {
    xclbin_path = xclbin;
    err = xf::database::gqe::init_hardware(&ctx, &dev_id, &cq,
                                           CL_QUEUE_PROFILING_ENABLE | CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "ERROR: fail to init hardware\n");
        exit(1);
    }

    err = xf::database::gqe::load_binary(&prg, ctx, dev_id, xclbin_path.c_str());
    if (err != CL_SUCCESS) {
        fprintf(stderr, "ERROR: fail to program PL\n");
        exit(1);
    }
}

Joiner::~Joiner() {
    err = clReleaseProgram(prg);
    if (err != CL_SUCCESS) {
        std::cout << "deconstructor" << std::endl;
        exit(1);
    }

    clReleaseCommandQueue(cq);
    clReleaseContext(ctx);
};
ErrCode Joiner::join(Table& a,
                     std::string filter_a,
                     Table& b,
                     std::string filter_b,
                     std::string join_str, // comma seprated
                     Table& c,
                     std::string output_str, // comma seprated
                     int join_type,
                     JoinStrategyBase* strategyimp) {
    // strategy
    bool new_s = false;
    if (strategyimp == nullptr) {
        strategyimp = new JoinStrategyBase();
        new_s = true;
    }
    auto params = strategyimp->getSolutionParams(a, b);
    // cfg
    JoinConfig jcmd(a, filter_a, b, filter_b, join_str, {}, {}, c, output_str, join_type, (params[0] == 2));
    // join
    ErrCode err = join_all(a, b, c, jcmd, params);
    if (new_s) delete strategyimp;
    return err;
}

ErrCode Joiner::join_all(Table& tab_a, Table& tab_b, Table& tab_c, JoinConfig& jcmd, std::vector<size_t> params) {
    ErrCode err;
    size_t _solution = params[0];
    if (_solution == 0) {
#ifdef USER_DEBUG
        std::cout << "(direct join)" << std::endl;
#endif
        err = join_sol0(tab_a, tab_b, tab_c, jcmd, params);
    } else if (_solution == 1) {
#ifdef USER_DEBUG
        std::cout << "(1xbuild + multi-probe join)" << std::endl;
#endif
        err = join_sol1(tab_a, tab_b, tab_c, jcmd, params);
    } else if (_solution == 2) {
#ifdef USER_DEBUG
        std::cout << "(partition + join)" << std::endl;
#endif
        err = join_sol2(tab_a, tab_b, tab_c, jcmd, params);
    } else {
        return PARAM_ERROR;
    }
    return err;
}

ErrCode Joiner::join_sol0(Table& tab_a, Table& tab_b, Table& tab_c, JoinConfig& jcmd, std::vector<size_t> params) {
    gqe::utils::MM mm;
    // size_t kernel_has_aggr = 0;
    ap_uint<512>* table_cfg5s = jcmd.getJoinConfigBits();
    std::vector<std::vector<int8_t> > q5s_join_scan = jcmd.getScanSwShuf1();
    size_t o_nrow = tab_a.getRowNum();
    size_t l_nrow = tab_b.getRowNum();
    size_t result_nrow = tab_c.getRowNum();
    size_t o_valid_col_num = q5s_join_scan[0].size();
    size_t l_valid_col_num = q5s_join_scan[1].size();
    size_t out_valid_col_num = tab_c.getColNum();
    // Number of vec in input buf. Add some extra and align.
    size_t table_l_depth = (l_nrow + VEC_SCAN * 9 - 1) / VEC_SCAN;
    size_t table_o_depth = (o_nrow + VEC_SCAN * 9 - 1) / VEC_SCAN;

    size_t tab_o_col_type_size[8];
    size_t tab_l_col_type_size[8];
    size_t table_o_size[8];
    size_t table_l_size[8];

    // data load from disk. will re-use in each call, but assumed to be different.
    char* table_o_user[8];
    for (size_t i = 0; i < o_valid_col_num; i++) {
        table_o_user[i] = tab_a.getColPointer(q5s_join_scan[0][i]);
        tab_o_col_type_size[i] = (tab_a.getColTypeSize(i));
        table_o_size[i] = table_o_depth * VEC_SCAN * tab_o_col_type_size[i];
    }

    char* table_l_user[8];
    for (size_t i = 0; i < l_valid_col_num; i++) {
        table_l_user[i] = tab_b.getColPointer(q5s_join_scan[1][i]);
        tab_l_col_type_size[i] = (tab_b.getColTypeSize(i));
        table_l_size[i] = table_l_depth * VEC_SCAN * tab_l_col_type_size[i];
    }

    // host buffer to be mapped with device buffer through OpenCL

    char* table_o[8];
    char* table_l[8];

    for (size_t j = 0; j < 8; j++) {
        if (j < l_valid_col_num) {
            table_l[j] = mm.aligned_alloc<char>(table_l_size[j]);
        } else {
            table_l[j] = mm.aligned_alloc<char>(VEC_SCAN);
        }
    }
    for (size_t i = 0; i < 8; i++) {
        if (i < o_valid_col_num) {
            table_o[i] = mm.aligned_alloc<char>(table_o_size[i]);
        } else {
            table_o[i] = mm.aligned_alloc<char>(VEC_SCAN);
        }
    }

    // Num of vecs.
    const int size_apu_512 = 64;
    size_t table_result_depth = result_nrow / VEC_LEN; // 8 columns in one buffer
    size_t table_out_col_type_size[8];
    size_t table_result_size[8];
    char* table_out_user[8];
    char* table_out[8];

    for (size_t i = 0; i < 8; i++) {
        if (i < out_valid_col_num) {
            table_out_user[i] = tab_c.getColPointer(i);
            table_out_col_type_size[i] = tab_c.getColTypeSize(i);
            table_result_size[i] = table_result_depth * table_out_col_type_size[i];
            table_out[i] = mm.aligned_alloc<char>(table_result_size[i]);
        } else {
            table_out_user[i] = mm.aligned_alloc<char>(table_result_depth);
            table_out[i] = mm.aligned_alloc<char>(table_result_depth);
        }
    }

    // XXX ensure the out buffer is allocated

    std::cout << "Total aligned alloc size: " << mm.size() << std::endl;

    //--------------- metabuffer setup -----------------
    // using col0 and col1 buffer during build
    // setup build used meta input
    MetaTable meta_build_in;
    meta_build_in.setColNum(o_valid_col_num);
    for (size_t i = 0; i < o_valid_col_num; i++) {
        meta_build_in.setCol(i, i, o_nrow);
    }

    // setup probe used meta input
    MetaTable meta_probe_in;
    meta_probe_in.setColNum(l_valid_col_num);
    for (size_t i = 0; i < l_valid_col_num; i++) {
        meta_probe_in.setCol(i, i, l_nrow);
    }

    // ouput col0,1,2,3 buffers data, with order: 0 1 2 3. (When aggr is off)
    // when aggr is on, actually only using col0 is enough.
    // below example only illustrates the output buffers can be shuffled.
    // setup probe used meta output
    MetaTable meta_probe_out;
    meta_probe_out.setColNum(out_valid_col_num);
    for (size_t i = 0; i < out_valid_col_num; i++) {
        meta_probe_out.setCol(i, i, result_nrow);
    }
    //--------------------------------------------
    //
    size_t build_probe_flag_0 = 0;
    size_t build_probe_flag_1 = 1;

    // build kernel
    cl_kernel bkernel;
    bkernel = clCreateKernel(prg, "gqeJoin", &err);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "ERROR: failed to create kernel.\n");
        return DEV_ERROR;
    }
    // probe kernel, pipeline used handle
    cl_kernel jkernel;
    jkernel = clCreateKernel(prg, "gqeJoin", &err);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "ERROR: failed to create kernel.\n");
        return DEV_ERROR;
    }
    std::cout << "Kernel has been created\n";

    cl_mem_ext_ptr_t mext_table_o[8], mext_table_l[8], mext_cfg5s;
    cl_mem_ext_ptr_t mext_table_out[8];
    cl_mem_ext_ptr_t memExt[2][PU_NM * 2];
    cl_mem_ext_ptr_t mext_meta_build_in, mext_meta_probe_in, mext_meta_probe_out;

    for (int i = 0; i < 8; i++) {
        mext_table_o[i] = {i, table_o[i], bkernel};
    }

    mext_meta_build_in = {9, meta_build_in.meta(), bkernel};
    mext_meta_probe_in = {9, meta_probe_in.meta(), jkernel};
    mext_meta_probe_out = {10, meta_probe_out.meta(), jkernel};

    for (int j = 0; j < 8; j++) {
        mext_table_l[j] = {j, table_l[j], jkernel};
        mext_table_out[j] = {11 + j, table_out[j], jkernel};
    }

    mext_cfg5s = {19, table_cfg5s, bkernel};

    for (int c = 0; c < 16; c++) {
        memExt[0][c] = {20 + c, nullptr, bkernel};
    }
    for (int c = 0; c < 16; c++) {
        memExt[1][c] = {20 + c, nullptr, jkernel};
    }

    // Map buffers
    cl_mem buf_table_o[8];
    cl_mem buf_table_l[8];
    cl_mem buf_table_out[8];
    cl_mem buf_cfg5s;
    for (size_t i = 0; i < 8; i++) {
        if (i < o_valid_col_num) {
            buf_table_o[i] = clCreateBuffer(ctx, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY,
                                            table_o_size[i], &mext_table_o[i], &err);
        } else {
            buf_table_o[i] = clCreateBuffer(ctx, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY,
                                            (VEC_SCAN), &mext_table_o[i], &err);
        }
        if (err != CL_SUCCESS) {
            return MEM_ERROR;
        }
    }
    for (size_t j = 0; j < 8; j++) {
        if (j < l_valid_col_num) {
            buf_table_l[j] = clCreateBuffer(ctx, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY,
                                            table_l_size[j], &mext_table_l[j], &err);
        } else {
            buf_table_l[j] = clCreateBuffer(ctx, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY,
                                            (VEC_SCAN), &mext_table_l[j], &err);
        }
        if (j < out_valid_col_num) {
            buf_table_out[j] = clCreateBuffer(ctx, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                                              table_result_size[j], &mext_table_out[j], &err);
        } else {
            buf_table_out[j] = clCreateBuffer(ctx, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                                              table_result_depth, &mext_table_out[j], &err);
        }
        if (err != CL_SUCCESS) {
            return MEM_ERROR;
        }
    }

    buf_cfg5s = clCreateBuffer(ctx, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY, (size_apu_512 * 9),
                               &mext_cfg5s, &err);
    if (err != CL_SUCCESS) {
        return MEM_ERROR;
    }
    // htb stb buffers
    cl_mem buf_tmp[PU_NM * 2];
    for (int j = 0; j < PU_NM; j++) {
        buf_tmp[j] = clCreateBuffer(ctx, CL_MEM_READ_WRITE | CL_MEM_HOST_NO_ACCESS | CL_MEM_EXT_PTR_XILINX,
                                    (size_t)(sizeof(int64_t) * HT_BUFF_DEPTH / 2), &memExt[0][j], &err);
        if (err != CL_SUCCESS) {
            return MEM_ERROR;
        }
    }
    for (int j = PU_NM; j < PU_NM * 2; j++) {
        buf_tmp[j] = clCreateBuffer(ctx, CL_MEM_READ_WRITE | CL_MEM_HOST_NO_ACCESS | CL_MEM_EXT_PTR_XILINX,
                                    (size_t)(KEY_SZ * 2 * S_BUFF_DEPTH / 2), &memExt[0][j], &err);
        if (err != CL_SUCCESS) {
            return MEM_ERROR;
        }
    }
    cl_mem buf_meta_build_in = clCreateBuffer(ctx, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY,
                                              (size_apu_512 * 8), &mext_meta_build_in, &err);
    if (err != CL_SUCCESS) {
        return MEM_ERROR;
    }

    cl_mem buf_meta_probe_in = clCreateBuffer(ctx, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY,
                                              (size_apu_512 * 8), &mext_meta_probe_in, &err);
    if (err != CL_SUCCESS) {
        return MEM_ERROR;
    }
    cl_mem buf_meta_probe_out = clCreateBuffer(ctx, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                                               (size_apu_512 * 8), &mext_meta_probe_out, &err);
    if (err != CL_SUCCESS) {
        return MEM_ERROR;
    }

    std::cout << "buffers have been mapped.\n";

    // helper buffer sets
    std::vector<cl_mem> non_loop_bufs;
    for (int i = 0; i < 8; i++) {
        non_loop_bufs.push_back(buf_table_o[i]);
    }
    non_loop_bufs.push_back(buf_cfg5s);
    non_loop_bufs.push_back(buf_meta_build_in);
    non_loop_bufs.push_back(buf_meta_probe_out);

    std::vector<cl_mem> loop_in_bufs;
    for (int i = 0; i < 8; i++) {
        loop_in_bufs.push_back(buf_table_l[i]);
    }
    loop_in_bufs.push_back(buf_meta_probe_in);

    std::vector<cl_mem> loop_out_bufs;
    for (int i = 0; i < 8; i++) {
        loop_out_bufs.push_back(buf_table_out[i]);
    }
    loop_out_bufs.push_back(buf_meta_probe_out);

    clEnqueueMigrateMemObjects(cq, loop_in_bufs.size(), loop_in_bufs.data(), CL_MIGRATE_MEM_OBJECT_CONTENT_UNDEFINED, 0,
                               nullptr, nullptr);
    clEnqueueMigrateMemObjects(cq, loop_out_bufs.size(), loop_out_bufs.data(), CL_MIGRATE_MEM_OBJECT_CONTENT_UNDEFINED,
                               0, nullptr, nullptr);

    // set args and enqueue kernel
    int j = 0;
    clSetKernelArg(bkernel, j++, sizeof(cl_mem), &buf_table_o[0]);
    clSetKernelArg(bkernel, j++, sizeof(cl_mem), &buf_table_o[1]);
    clSetKernelArg(bkernel, j++, sizeof(cl_mem), &buf_table_o[2]);
    clSetKernelArg(bkernel, j++, sizeof(cl_mem), &buf_table_o[3]);
    clSetKernelArg(bkernel, j++, sizeof(cl_mem), &buf_table_o[4]);
    clSetKernelArg(bkernel, j++, sizeof(cl_mem), &buf_table_o[5]);
    clSetKernelArg(bkernel, j++, sizeof(cl_mem), &buf_table_o[6]);
    clSetKernelArg(bkernel, j++, sizeof(cl_mem), &buf_table_o[7]);
    clSetKernelArg(bkernel, j++, sizeof(size_t), &build_probe_flag_0);
    clSetKernelArg(bkernel, j++, sizeof(cl_mem), &buf_meta_build_in);
    clSetKernelArg(bkernel, j++, sizeof(cl_mem), &buf_meta_probe_out);
    clSetKernelArg(bkernel, j++, sizeof(cl_mem), &buf_table_out[0]);
    clSetKernelArg(bkernel, j++, sizeof(cl_mem), &buf_table_out[1]);
    clSetKernelArg(bkernel, j++, sizeof(cl_mem), &buf_table_out[2]);
    clSetKernelArg(bkernel, j++, sizeof(cl_mem), &buf_table_out[3]);
    clSetKernelArg(bkernel, j++, sizeof(cl_mem), &buf_table_out[4]);
    clSetKernelArg(bkernel, j++, sizeof(cl_mem), &buf_table_out[5]);
    clSetKernelArg(bkernel, j++, sizeof(cl_mem), &buf_table_out[6]);
    clSetKernelArg(bkernel, j++, sizeof(cl_mem), &buf_table_out[7]);
    clSetKernelArg(bkernel, j++, sizeof(cl_mem), &buf_cfg5s);
    for (int k = 0; k < PU_NM * 2; k++) {
        clSetKernelArg(bkernel, j++, sizeof(cl_mem), &buf_tmp[k]);
    }

    // set args and enqueue kernel
    j = 0;
    clSetKernelArg(jkernel, j++, sizeof(cl_mem), &buf_table_l[0]);
    clSetKernelArg(jkernel, j++, sizeof(cl_mem), &buf_table_l[1]);
    clSetKernelArg(jkernel, j++, sizeof(cl_mem), &buf_table_l[2]);
    clSetKernelArg(jkernel, j++, sizeof(cl_mem), &buf_table_l[3]);
    clSetKernelArg(jkernel, j++, sizeof(cl_mem), &buf_table_l[4]);
    clSetKernelArg(jkernel, j++, sizeof(cl_mem), &buf_table_l[5]);
    clSetKernelArg(jkernel, j++, sizeof(cl_mem), &buf_table_l[6]);
    clSetKernelArg(jkernel, j++, sizeof(cl_mem), &buf_table_l[7]);
    clSetKernelArg(jkernel, j++, sizeof(size_t), &build_probe_flag_1);
    clSetKernelArg(jkernel, j++, sizeof(cl_mem), &buf_meta_probe_in);
    clSetKernelArg(jkernel, j++, sizeof(cl_mem), &buf_meta_probe_out);
    clSetKernelArg(jkernel, j++, sizeof(cl_mem), &buf_table_out[0]);
    clSetKernelArg(jkernel, j++, sizeof(cl_mem), &buf_table_out[1]);
    clSetKernelArg(jkernel, j++, sizeof(cl_mem), &buf_table_out[2]);
    clSetKernelArg(jkernel, j++, sizeof(cl_mem), &buf_table_out[3]);
    clSetKernelArg(jkernel, j++, sizeof(cl_mem), &buf_table_out[4]);
    clSetKernelArg(jkernel, j++, sizeof(cl_mem), &buf_table_out[5]);
    clSetKernelArg(jkernel, j++, sizeof(cl_mem), &buf_table_out[6]);
    clSetKernelArg(jkernel, j++, sizeof(cl_mem), &buf_table_out[7]);
    clSetKernelArg(jkernel, j++, sizeof(cl_mem), &buf_cfg5s);
    for (int k = 0; k < PU_NM * 2; k++) {
        clSetKernelArg(jkernel, j++, sizeof(cl_mem), &buf_tmp[k]);
    }

#ifdef JOIN_PERF_PROFILE
    gqe::utils::Timer timer;
#endif

    std::array<cl_event, 1> evt_tb_o;
    std::array<cl_event, 1> evt_bkrn;
    std::array<cl_event, 1> evt_tb_l;
    std::array<cl_event, 1> evt_pkrn;
    std::array<cl_event, 1> evt_tb_out;

// 1) copy Order table from host DDR to build kernel pinned host buffer
#ifdef JOIN_PERF_PROFILE
    timer.add(); // 0
#endif
    for (size_t i = 0; i < o_valid_col_num; i++) {
        memcpy(table_o[i], table_o_user[i], tab_o_col_type_size[i] * VEC_SCAN * table_o_depth);
    }
#ifdef JOIN_PERF_PROFILE
    timer.add(); // 1
#endif

    // 2) migrate order table data from host buffer to device buffer
    clEnqueueMigrateMemObjects(cq, non_loop_bufs.size(), non_loop_bufs.data(), 0, 0, nullptr, &evt_tb_o[0]);

    // 3) launch build kernel
    clEnqueueTask(cq, bkernel, 1, evt_tb_o.data(), &evt_bkrn[0]);
    clWaitForEvents(1, &evt_bkrn[0]);
// 4) copy L table from host DDR to build kernel pinned host buffer
#ifdef JOIN_PERF_PROFILE
    timer.add(); // 2
#endif
    for (size_t i = 0; i < l_valid_col_num; i++) {
        memcpy(table_l[i], table_l_user[i], tab_l_col_type_size[i] * VEC_SCAN * table_l_depth);
    }
#ifdef JOIN_PERF_PROFILE
    timer.add(); // 3
#endif

    // 5) migrate L table data from host buffer to device buffer
    clEnqueueMigrateMemObjects(cq, loop_in_bufs.size(), loop_in_bufs.data(), 0, 0, nullptr, &evt_tb_l[0]);

    // 6) launch probe kernel
    clEnqueueTask(cq, jkernel, 1, evt_tb_l.data(), &evt_pkrn[0]);

    // 7) migrate result data from device buffer to host buffer
    clEnqueueMigrateMemObjects(cq, loop_out_bufs.size(), loop_out_bufs.data(), CL_MIGRATE_MEM_OBJECT_HOST, 1,
                               evt_pkrn.data(), &evt_tb_out[0]);

    // 8) copy output data from pinned host buffer to user host buffer
    clWaitForEvents(1, &evt_tb_out[0]);
#ifdef JOIN_PERF_PROFILE
    timer.add(); // 4
#endif
    for (size_t i = 0; i < out_valid_col_num; i++) {
        memcpy(table_out_user[i], table_out[i], table_result_size[i]);
    }
#ifdef JOIN_PERF_PROFILE
    timer.add(); // 5
#endif

// 9) calc and print the execution time of each phase
#ifdef JOIN_PERF_PROFILE
    cl_ulong start, end;
    long ev_ns;

    std::cout << std::endl << "============== execution time ==================" << std::endl;
    // 9.0) memcpy O
    printf("memcpy left table time: %0.3f msec\n", timer.getMilliSec(0, 1));
    // 9.1) migrate O
    clGetEventProfilingInfo(evt_tb_o[0], CL_PROFILING_COMMAND_START, sizeof(cl_ulong), &start, NULL);
    clGetEventProfilingInfo(evt_tb_o[0], CL_PROFILING_COMMAND_END, sizeof(cl_ulong), &end, NULL);
    ev_ns = end - start;
    printf("migrate left table: %ld.%03ld msec\n", ev_ns / 1000000, (ev_ns % 1000000) / 1000);
    // 9.2) build kernel
    clGetEventProfilingInfo(evt_bkrn[0], CL_PROFILING_COMMAND_START, sizeof(cl_ulong), &start, NULL);
    clGetEventProfilingInfo(evt_bkrn[0], CL_PROFILING_COMMAND_END, sizeof(cl_ulong), &end, NULL);
    ev_ns = end - start;
    printf("build kernel: %ld.%03ld msec\n", ev_ns / 1000000, (ev_ns % 1000000) / 1000);
    // 9.3) memcpy L
    printf("memcpy right table time: %0.3f msec\n", timer.getMilliSec(2, 3));
    // 9.4) migrate L
    clGetEventProfilingInfo(evt_tb_l[0], CL_PROFILING_COMMAND_START, sizeof(cl_ulong), &start, NULL);
    clGetEventProfilingInfo(evt_tb_l[0], CL_PROFILING_COMMAND_END, sizeof(cl_ulong), &end, NULL);
    ev_ns = end - start;
    printf("migrate right table: %ld.%03ld msec\n", ev_ns / 1000000, (ev_ns % 1000000) / 1000);
    // 9.5) probe kernel
    clGetEventProfilingInfo(evt_pkrn[0], CL_PROFILING_COMMAND_START, sizeof(cl_ulong), &start, NULL);
    clGetEventProfilingInfo(evt_pkrn[0], CL_PROFILING_COMMAND_END, sizeof(cl_ulong), &end, NULL);
    ev_ns = end - start;
    printf("probe kernel: %ld.%03ld msec\n", ev_ns / 1000000, (ev_ns % 1000000) / 1000);
    // 9.6) migreate output
    clGetEventProfilingInfo(evt_tb_out[0], CL_PROFILING_COMMAND_START, sizeof(cl_ulong), &start, NULL);
    clGetEventProfilingInfo(evt_tb_out[0], CL_PROFILING_COMMAND_END, sizeof(cl_ulong), &end, NULL);
    ev_ns = end - start;
    printf("migrate output: %ld.%03ld msec\n", ev_ns / 1000000, (ev_ns % 1000000) / 1000);
    // 9.7) memcpy output
    printf("memcpy output table time: %0.3f\n", timer.getMilliSec(4, 5));

    // =========== print result ===========
    printf("\n");
#endif
    // check the probe updated meta
    int out_nrow = meta_probe_out.getColLen();
    std::cout << "Output buffer has " << out_nrow << " rows." << std::endl;
    tab_c.setRowNum(out_nrow);

    printf("GQE result has %d rows\n", out_nrow);

    //--------------release---------------
    for (int i = 0; i < 8; i++) {
        clReleaseMemObject(buf_table_o[i]);
    }
    for (int i = 0; i < 8; i++) {
        clReleaseMemObject(buf_table_l[i]);
        clReleaseMemObject(buf_table_out[i]);
    }
    clReleaseMemObject(buf_cfg5s);
    for (int k = 0; k < PU_NM * 2; k++) {
        clReleaseMemObject(buf_tmp[k]);
    }
    clReleaseMemObject(buf_meta_build_in);
    clReleaseMemObject(buf_meta_probe_in);
    clReleaseMemObject(buf_meta_probe_out);

    clReleaseEvent(evt_tb_o[0]);
    clReleaseEvent(evt_bkrn[0]);
    clReleaseEvent(evt_tb_l[0]);
    clReleaseEvent(evt_pkrn[0]);
    clReleaseEvent(evt_tb_out[0]);

    clReleaseKernel(bkernel);
    clReleaseKernel(jkernel);

    return SUCCESS;
}

ErrCode Joiner::join_sol1(Table& tab_a, Table& tab_b, Table& tab_c, JoinConfig& jcmd, std::vector<size_t> params) {
    ap_uint<512>* table_cfg5s = jcmd.getJoinConfigBits();
    std::vector<std::vector<int8_t> > q5s_join_scan = jcmd.getScanSwShuf1();
    gqe::utils::MM mm;
    size_t slice_num = params[3];
    // default to have
    size_t o_nrow = tab_a.getRowNum();
    size_t l_nrow = tab_b.getRowNum();
    size_t o_valid_col_num = q5s_join_scan[0].size();
    size_t l_valid_col_num = q5s_join_scan[1].size();
    size_t out_valid_col_num = tab_c.getColNum();
    // std::cout << "table o_nrow: " << o_nrow << std::endl;
    // std::cout << "table l_nrow: " << l_nrow << std::endl;
    // Number of vec in input buf. Add some extra and align.
    int table_o_nrow = o_nrow;
    int table_o_depth = (table_o_nrow + VEC_SCAN - 1) / VEC_SCAN;
    size_t tab_o_col_type_size[8];
    size_t tab_l_col_type_size[8];
    size_t table_out_col_type_size[8];

    size_t table_o_size[8];
    char* table_o_user[8];
    for (int i = 0; i < (int)o_valid_col_num; i++) {
        tab_o_col_type_size[i] = (tab_a.getColTypeSize(q5s_join_scan[0][i]));
        table_o_size[i] = table_o_depth * VEC_SCAN * tab_o_col_type_size[i];
        table_o_user[i] = tab_a.getColPointer(q5s_join_scan[0][i]);
    }

    int table_l_nrow = l_nrow;
    int table_l_slice_nrow = (table_l_nrow + slice_num - 1) / slice_num;
    int table_l_slice_size[8];
    char* table_l_user[8][slice_num];
    for (int i = 0; i < (int)l_valid_col_num; i++) {
        tab_l_col_type_size[i] = (tab_b.getColTypeSize(i));
        table_l_slice_size[i] = table_l_slice_nrow * tab_l_col_type_size[i];
        for (size_t j = 0; j < slice_num; j++) {
            table_l_user[i][j] = tab_b.getColPointer(q5s_join_scan[1][i], slice_num, j);
        }
    }
    // std::cout << "slice_num: " << slice_num << std::endl;

    size_t table_l_out_nrow = tab_c.getRowNum();
    size_t table_l_out_depth = (table_l_out_nrow + 15) / 16;
    size_t table_l_out_size[8];
    char* table_out_user[8];
    for (size_t j = 0; j < out_valid_col_num; j++) {
        table_out_col_type_size[j] = tab_c.getColTypeSize(j);
        table_l_out_size[j] = table_l_out_depth * 16 * table_out_col_type_size[j];
        table_out_user[j] = tab_c.getColPointer(j);
        memset(table_out_user[j], 0, table_l_out_size[j]);
    }

    // host buffer to be mapped with device buffer through OpenCL
    char* table_o[8];
    for (size_t i = 0; i < 8; i++) {
        if (o_valid_col_num > i) {
            table_o[i] = mm.aligned_alloc<char>(table_o_size[i]);
        } else {
            table_o[i] = mm.aligned_alloc<char>(VEC_SCAN);
        }
    }
    std::cout << "anchor...." << std::endl;

    char* table_l[8][2];
    for (size_t i = 0; i < 8; i++) {
        if (l_valid_col_num > i) {
            table_l[i][0] = mm.aligned_alloc<char>(table_l_slice_size[i]);
            table_l[i][1] = mm.aligned_alloc<char>(table_l_slice_size[i]);
        } else {
            table_l[i][0] = mm.aligned_alloc<char>(VEC_SCAN);
            table_l[i][1] = mm.aligned_alloc<char>(VEC_SCAN);
        }
    }

    char* table_out[8][2];
    const int size_apu_512 = 64;

    for (size_t i = 0; i < 8; i++) {
        if (out_valid_col_num > i) {
            table_out[i][0] = mm.aligned_alloc<char>(table_l_out_depth * size_apu_512);
            table_out[i][1] = mm.aligned_alloc<char>(table_l_out_depth * size_apu_512);
        } else {
            table_out[i][0] = mm.aligned_alloc<char>(VEC_SCAN);
            table_out[i][1] = mm.aligned_alloc<char>(VEC_SCAN);
        }
    }
#ifdef USER_DEBUG
    std::cout << "Host map buffer has been allocated.\n";
    std::cout << "Total aligned alloc size: " << mm.size() << std::endl;
#endif

    //--------------- metabuffer setup -----------------
    // using col0 and col1 buffer during build
    // setup build used meta input, un-used columns are assigned to -1, as shown
    // below.
    // 2 input columns data are valid, col0 and col1, not actually used cols can
    // be marked as -1.
    MetaTable meta_build_in;
    meta_build_in.setColNum(o_valid_col_num);
    for (size_t i = 0; i < o_valid_col_num; i++) {
        meta_build_in.setCol(i, i, table_o_nrow);
    }

    // setup probe used meta input
    MetaTable meta_probe_in;
    meta_probe_in.setColNum(l_valid_col_num);
    for (size_t i = 0; i < l_valid_col_num; i++) {
        meta_probe_in.setCol(i, i, table_l_slice_nrow);
    }

    // ouput col0,1,2,3 buffers data, with order: 0 1 2 3. (When aggr is off)
    // when aggr is on, actually only using col0 is enough.
    // below example only illustrates the output buffers can be shuffled.
    // setup probe used meta output
    MetaTable meta_probe_out[2];
    meta_probe_out[0].setColNum(out_valid_col_num);
    meta_probe_out[1].setColNum(out_valid_col_num);
    for (size_t i = 0; i < out_valid_col_num; i++) {
        meta_probe_out[0].setCol(i, i, table_l_out_nrow);
        meta_probe_out[1].setCol(i, i, table_l_out_nrow);
    }

    //--------------------------------------------
    //
    size_t build_probe_flag_0 = 0;
    size_t build_probe_flag_1 = 1;

    // Get CL devices.

    // build kernel
    cl_kernel bkernel;
    bkernel = clCreateKernel(prg, "gqeJoin", &err);
    // probe kernel, pipeline used handle
    cl_kernel pkernel[2];
    for (int i = 0; i < 2; i++) {
        pkernel[i] = clCreateKernel(prg, "gqeJoin", &err);
        if (err != CL_SUCCESS) {
            fprintf(stderr, "ERROR: failed to create kernel.\n");
            // return err;
        }
    }
#ifdef USER_DEBUG
    std::cout << "Kernel has been created\n";
#endif

    cl_mem_ext_ptr_t mext_table_o[8], mext_table_l[2][8], mext_cfg5s;
    cl_mem_ext_ptr_t mext_table_out[2][8];
    cl_mem_ext_ptr_t memExt[2][PU_NM * 2];
    cl_mem_ext_ptr_t mext_meta_build_in, mext_meta_probe_in, mext_meta_probe_out[2];

    for (int i = 0; i < 8; i++) {
        mext_table_o[i] = {i, table_o[i], bkernel};
    }

    mext_meta_build_in = {9, meta_build_in.meta(), bkernel};
    mext_meta_probe_in = {9, meta_probe_in.meta(), pkernel[0]};
    mext_meta_probe_out[0] = {10, meta_probe_out[0].meta(), pkernel[0]};
    mext_meta_probe_out[1] = {10, meta_probe_out[1].meta(), pkernel[1]};

    for (int i = 0; i < 2; i++) {
        for (int j = 0; j < 8; j++) {
            mext_table_l[i][j] = {j, table_l[j][i], pkernel[i]};
            mext_table_out[i][j] = {11 + j, table_out[j][i], pkernel[i]};
        }
    }

    mext_cfg5s = {19, table_cfg5s, bkernel};

    for (int c = 0; c < 16; c++) {
        memExt[0][c] = {20 + c, nullptr, bkernel};
    }
    for (int c = 0; c < 16; c++) {
        memExt[1][c] = {20 + c, nullptr, pkernel[0]};
    }

    // Map buffers
    cl_mem buf_table_o[8];
    cl_mem buf_table_l[8][2];
    cl_mem buf_table_out[8][2];
    cl_mem buf_cfg5s;

    for (size_t i = 0; i < 8; i++) {
        if (o_valid_col_num > i) {
            buf_table_o[i] = clCreateBuffer(ctx, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY,
                                            table_o_size[i], &mext_table_o[i], &err);
        } else {
            buf_table_o[i] = clCreateBuffer(ctx, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY,
                                            (VEC_SCAN), &mext_table_o[i], &err);
        }
    }
    for (size_t j = 0; j < 8; j++) {
        for (int i = 0; i < 2; i++) {
            if (l_valid_col_num > j) {
                buf_table_l[j][i] = clCreateBuffer(ctx, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY,
                                                   table_l_slice_size[j], &mext_table_l[i][j], &err);
            } else {
                buf_table_l[j][i] = clCreateBuffer(ctx, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY,
                                                   (VEC_SCAN), &mext_table_l[i][j], &err);
            }
            if (out_valid_col_num > j) {
                buf_table_out[j][i] =
                    clCreateBuffer(ctx, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                                   table_l_out_size[j], &mext_table_out[i][j], &err);
            } else {
                buf_table_out[j][i] =
                    clCreateBuffer(ctx, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE, (VEC_SCAN),
                                   &mext_table_out[i][j], &err);
            }
        }
    }

    buf_cfg5s = clCreateBuffer(ctx, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY, (size_apu_512 * 9),
                               &mext_cfg5s, &err);
    // htb stb buffers
    cl_mem buf_tmp[PU_NM * 2];
    for (int j = 0; j < PU_NM; j++) {
        buf_tmp[j] = clCreateBuffer(ctx, CL_MEM_READ_WRITE | CL_MEM_HOST_NO_ACCESS | CL_MEM_EXT_PTR_XILINX,
                                    (size_t)(sizeof(int64_t) * HT_BUFF_DEPTH / 2), &memExt[0][j], &err);
    }
    for (int j = PU_NM; j < PU_NM * 2; j++) {
        buf_tmp[j] = clCreateBuffer(ctx, CL_MEM_READ_WRITE | CL_MEM_HOST_NO_ACCESS | CL_MEM_EXT_PTR_XILINX,
                                    (size_t)(KEY_SZ * 2 * S_BUFF_DEPTH / 2), &memExt[0][j], &err);
    }
    // meta buffers
    cl_mem buf_meta_build_in = clCreateBuffer(ctx, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY,
                                              (size_apu_512 * 8), &mext_meta_build_in, &err);

    cl_mem buf_meta_probe_in = clCreateBuffer(ctx, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY,
                                              (size_apu_512 * 8), &mext_meta_probe_in, &err);
    cl_mem buf_meta_probe_out[2];
    buf_meta_probe_out[0] = clCreateBuffer(ctx, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                                           (size_apu_512 * 8), &mext_meta_probe_out[0], &err);
    buf_meta_probe_out[1] = clCreateBuffer(ctx, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                                           (size_apu_512 * 8), &mext_meta_probe_out[1], &err);
#ifdef USER_DEBUG
    std::cout << "Temp buffers have been mapped.\n";
#endif

    // helper buffer sets
    std::vector<cl_mem> non_loop_bufs;
    for (int i = 0; i < 8; i++) {
        non_loop_bufs.push_back(buf_table_o[i]);
    }
    non_loop_bufs.push_back(buf_cfg5s);
    non_loop_bufs.push_back(buf_meta_build_in);
    non_loop_bufs.push_back(buf_meta_probe_out[0]);
    non_loop_bufs.push_back(buf_meta_probe_out[1]);

    std::vector<cl_mem> loop_in_bufs[2];
    for (int k = 0; k < 2; k++) {
        for (int i = 0; i < 8; i++) {
            loop_in_bufs[k].push_back(buf_table_l[i][k]);
        }
        loop_in_bufs[k].push_back(buf_meta_probe_in);
    }

    std::vector<cl_mem> loop_out_bufs[2];
    for (int k = 0; k < 2; k++) {
        for (int i = 0; i < 8; i++) {
            loop_out_bufs[k].push_back(buf_table_out[i][k]);
        }
        loop_out_bufs[k].push_back(buf_meta_probe_out[k]);
    }

    // make sure all buffers are resident on device
    clEnqueueMigrateMemObjects(cq, loop_out_bufs[0].size(), loop_out_bufs[0].data(),
                               CL_MIGRATE_MEM_OBJECT_CONTENT_UNDEFINED, 0, nullptr, nullptr);
    clEnqueueMigrateMemObjects(cq, loop_out_bufs[1].size(), loop_out_bufs[1].data(),
                               CL_MIGRATE_MEM_OBJECT_CONTENT_UNDEFINED, 0, nullptr, nullptr);
    clEnqueueMigrateMemObjects(cq, loop_in_bufs[0].size(), loop_in_bufs[0].data(),
                               CL_MIGRATE_MEM_OBJECT_CONTENT_UNDEFINED, 0, nullptr, nullptr);
    clEnqueueMigrateMemObjects(cq, loop_in_bufs[1].size(), loop_in_bufs[1].data(),
                               CL_MIGRATE_MEM_OBJECT_CONTENT_UNDEFINED, 0, nullptr, nullptr);

    // set build kernel args
    int j = 0;
    clSetKernelArg(bkernel, j++, sizeof(cl_mem), &buf_table_o[0]);
    clSetKernelArg(bkernel, j++, sizeof(cl_mem), &buf_table_o[1]);
    clSetKernelArg(bkernel, j++, sizeof(cl_mem), &buf_table_o[2]);
    clSetKernelArg(bkernel, j++, sizeof(cl_mem), &buf_table_o[3]);
    clSetKernelArg(bkernel, j++, sizeof(cl_mem), &buf_table_o[4]);
    clSetKernelArg(bkernel, j++, sizeof(cl_mem), &buf_table_o[5]);
    clSetKernelArg(bkernel, j++, sizeof(cl_mem), &buf_table_o[6]);
    clSetKernelArg(bkernel, j++, sizeof(cl_mem), &buf_table_o[7]);
    clSetKernelArg(bkernel, j++, sizeof(size_t), &build_probe_flag_0);
    clSetKernelArg(bkernel, j++, sizeof(cl_mem), &buf_meta_build_in);
    clSetKernelArg(bkernel, j++, sizeof(cl_mem), &buf_meta_probe_out[0]);
    clSetKernelArg(bkernel, j++, sizeof(cl_mem), &buf_table_out[0][0]);
    clSetKernelArg(bkernel, j++, sizeof(cl_mem), &buf_table_out[1][0]);
    clSetKernelArg(bkernel, j++, sizeof(cl_mem), &buf_table_out[2][0]);
    clSetKernelArg(bkernel, j++, sizeof(cl_mem), &buf_table_out[3][0]);
    clSetKernelArg(bkernel, j++, sizeof(cl_mem), &buf_table_out[4][0]);
    clSetKernelArg(bkernel, j++, sizeof(cl_mem), &buf_table_out[5][0]);
    clSetKernelArg(bkernel, j++, sizeof(cl_mem), &buf_table_out[6][0]);
    clSetKernelArg(bkernel, j++, sizeof(cl_mem), &buf_table_out[7][0]);
    clSetKernelArg(bkernel, j++, sizeof(cl_mem), &buf_cfg5s);
    for (int k = 0; k < PU_NM * 2; k++) {
        clSetKernelArg(bkernel, j++, sizeof(cl_mem), &buf_tmp[k]);
    }

    // set probe kernel args
    for (int i = 0; i < 2; i++) {
        int j = 0;
        clSetKernelArg(pkernel[i], j++, sizeof(cl_mem), &buf_table_l[0][i]);
        clSetKernelArg(pkernel[i], j++, sizeof(cl_mem), &buf_table_l[1][i]);
        clSetKernelArg(pkernel[i], j++, sizeof(cl_mem), &buf_table_l[2][i]);
        clSetKernelArg(pkernel[i], j++, sizeof(cl_mem), &buf_table_l[3][i]);
        clSetKernelArg(pkernel[i], j++, sizeof(cl_mem), &buf_table_l[4][i]);
        clSetKernelArg(pkernel[i], j++, sizeof(cl_mem), &buf_table_l[5][i]);
        clSetKernelArg(pkernel[i], j++, sizeof(cl_mem), &buf_table_l[6][i]);
        clSetKernelArg(pkernel[i], j++, sizeof(cl_mem), &buf_table_l[7][i]);
        clSetKernelArg(pkernel[i], j++, sizeof(size_t), &build_probe_flag_1);
        clSetKernelArg(pkernel[i], j++, sizeof(cl_mem), &buf_meta_probe_in);
        clSetKernelArg(pkernel[i], j++, sizeof(cl_mem), &buf_meta_probe_out[i]);
        clSetKernelArg(pkernel[i], j++, sizeof(cl_mem), &buf_table_out[0][i]);
        clSetKernelArg(pkernel[i], j++, sizeof(cl_mem), &buf_table_out[1][i]);
        clSetKernelArg(pkernel[i], j++, sizeof(cl_mem), &buf_table_out[2][i]);
        clSetKernelArg(pkernel[i], j++, sizeof(cl_mem), &buf_table_out[3][i]);
        clSetKernelArg(pkernel[i], j++, sizeof(cl_mem), &buf_table_out[4][i]);
        clSetKernelArg(pkernel[i], j++, sizeof(cl_mem), &buf_table_out[5][i]);
        clSetKernelArg(pkernel[i], j++, sizeof(cl_mem), &buf_table_out[6][i]);
        clSetKernelArg(pkernel[i], j++, sizeof(cl_mem), &buf_table_out[7][i]);
        clSetKernelArg(pkernel[i], j++, sizeof(cl_mem), &buf_cfg5s);
        for (int k = 0; k < PU_NM * 2; k++) {
            clSetKernelArg(pkernel[i], j++, sizeof(cl_mem), &buf_tmp[k]);
        }
    }

// =================== starting the pipelined task =========================
#ifdef JOIN_PERF_PROFILE
    gqe::utils::Timer tv_build_memcpyin;
    gqe::utils::Timer tv_probe_memcpyin[slice_num];
    gqe::utils::Timer tv_probe_memcpyout[slice_num];
#endif
    gqe::utils::Timer tv;

    std::vector<cl_event> evt_build_h2d;
    std::vector<cl_event> evt_build_krn;
    evt_build_h2d.resize(1);
    evt_build_krn.resize(1);

    tv.add(); // 0
//--------------- build --------------------
// copy and migrate shared O to device
#ifdef JOIN_PERF_PROFILE
    tv_build_memcpyin.add(); // 0
#endif
    for (size_t i = 0; i < o_valid_col_num; i++) {
        memcpy(table_o[i], table_o_user[i], tab_o_col_type_size[i] * VEC_SCAN * table_o_depth);
    }
#ifdef JOIN_PERF_PROFILE
    tv_build_memcpyin.add(); // 1
#endif

    // h2d
    clEnqueueMigrateMemObjects(cq, non_loop_bufs.size(), non_loop_bufs.data(), 0, 0, nullptr, &evt_build_h2d[0]);

    // launch build kernel
    clEnqueueTask(cq, bkernel, evt_build_h2d.size(), evt_build_h2d.data(), &evt_build_krn[0]);

    clWaitForEvents(evt_build_krn.size(), evt_build_krn.data());

    // ------------------- probe -------------------
    // dep events
    std::vector<std::vector<cl_event> > evt_probe_h2d(slice_num);
    std::vector<std::vector<cl_event> > evt_probe_krn(slice_num);
    std::vector<std::vector<cl_event> > evt_probe_d2h(slice_num);
    for (size_t i = 0; i < slice_num; ++i) {
        evt_probe_h2d[i].resize(1);
        evt_probe_krn[i].resize(1);
        evt_probe_d2h[i].resize(1);
    }
    // kernel dep events, to guarantee the kernel time is accurate
    std::vector<std::vector<cl_event> > evt_dep(slice_num);
    evt_dep[0].resize(1);
    for (size_t i = 1; i < slice_num; ++i) {
        evt_dep[i].resize(2);
    }

    // loop: copy result, copy and migrate L, sched migrate L, kernel, and migrate
    // back result.
    // probe loop starts
    int nrow_all_results[slice_num];
    int probe_out_nrow_accu = 0;
    for (size_t i = 0; i < slice_num + 2; ++i) {
        int k_id = i % 2;

        // 1) copy L slice data from host DDR to pinned host DDR
        if (i > 1) {
            // if run loop/slice is >1, need to wait the i-2 loop kernel finish, then
            // memcpy to host input buffer
            clWaitForEvents(evt_probe_krn[i - 2].size(), evt_probe_krn[i - 2].data());
        }
        if (i < slice_num) {
#ifdef JOIN_PERF_PROFILE
            tv_probe_memcpyin[i].add(); // 0
#endif
            for (size_t j = 0; j < l_valid_col_num; j++) {
                memcpy(table_l[j][k_id], table_l_user[j][i], table_l_slice_size[j]);
            }
#ifdef JOIN_PERF_PROFILE
            tv_probe_memcpyin[i].add(); // 1
#endif
        }
        // 2) migrate slice data from host DDR to dev DDR
        if (i < slice_num) {
            // migrate h2d
            if (i > 1) {
                clEnqueueMigrateMemObjects(cq, loop_in_bufs[k_id].size(), loop_in_bufs[k_id].data(), 0,
                                           evt_probe_krn[i - 2].size(), evt_probe_krn[i - 2].data(),
                                           &evt_probe_h2d[i][0]);
            } else {
                clEnqueueMigrateMemObjects(cq, loop_in_bufs[k_id].size(), loop_in_bufs[k_id].data(), 0, 0, nullptr,
                                           &evt_probe_h2d[i][0]);
            }
        }
        // 5) memcpy the output data back to user host buffer, for i-2 round
        if (i > 1) {
            clWaitForEvents(evt_probe_d2h[i - 2].size(), evt_probe_d2h[i - 2].data());
            // get the output nrow
            int probe_out_nrow = meta_probe_out[k_id].getColLen();
#ifdef USER_DEBUG
            std::cout << "Output buffer has " << probe_out_nrow << " rows." << std::endl;
#endif
            nrow_all_results[i - 2] = probe_out_nrow;
// memcpy only valid results back
#ifdef JOIN_PERF_PROFILE
            tv_probe_memcpyout[i - 2].add(); // 0
#endif
            for (size_t j = 0; j < out_valid_col_num; j++) {
                memcpy(table_out_user[j] + probe_out_nrow_accu * table_out_col_type_size[j], table_out[j][k_id],
                       probe_out_nrow * table_out_col_type_size[j]);
            }
            probe_out_nrow_accu += probe_out_nrow;
#ifdef JOIN_PERF_PROFILE
            tv_probe_memcpyout[i - 2].add(); // 1
#endif
        }
        if (i < slice_num) {
            // 3) launch kernel
            clWaitForEvents(evt_build_krn.size(), evt_build_krn.data());
            evt_dep[i][0] = evt_probe_h2d[i][0];
            if (i > 0) {
                evt_dep[i][1] = evt_probe_krn[i - 1][0];
            }
            clEnqueueTask(cq, pkernel[k_id], evt_dep[i].size(), evt_dep[i].data(), &evt_probe_krn[i][0]);

            // 4) migrate result data from device buffer to host buffer
            clEnqueueMigrateMemObjects(cq, loop_out_bufs[k_id].size(), loop_out_bufs[k_id].data(),
                                       CL_MIGRATE_MEM_OBJECT_HOST, evt_probe_krn[i].size(), evt_probe_krn[i].data(),
                                       &evt_probe_d2h[i][0]);
        }
    }
    tv.add(); // 1

    // =================== Print results =========================

    // compute time
    long kernel_ex_time = 0;
    cl_ulong start, end;
    long ev_ns;
    clGetEventProfilingInfo(evt_build_krn[0], CL_PROFILING_COMMAND_START, sizeof(cl_ulong), &start, NULL);
    clGetEventProfilingInfo(evt_build_krn[0], CL_PROFILING_COMMAND_END, sizeof(cl_ulong), &end, NULL);
    ev_ns = end - start;
    kernel_ex_time += ev_ns;
    for (size_t i = 0; i < slice_num; i++) {
        clGetEventProfilingInfo(evt_probe_krn[i][0], CL_PROFILING_COMMAND_START, sizeof(cl_ulong), &start, NULL);
        clGetEventProfilingInfo(evt_probe_krn[i][0], CL_PROFILING_COMMAND_END, sizeof(cl_ulong), &end, NULL);
        ev_ns = end - start;
        kernel_ex_time += ev_ns;
    }
    // compute result
    int out_nrow_sum = 0;
    for (size_t i = 0; i < slice_num; i++) {
        // check the probe updated meta
        int out_nrow = nrow_all_results[i];
        out_nrow_sum += out_nrow;
#ifdef USER_DEBUG
        printf("GQE result %d has %d rows\n", (int)i, out_nrow);
#endif
    }
    tab_c.setRowNum(out_nrow_sum);

    double out_bytes = (double)out_nrow_sum * sizeof(int) * out_valid_col_num / 1024 / 1024;
    double in1_bytes = (double)o_nrow * sizeof(int) * o_valid_col_num / 1024 / 1024;
    double in2_bytes = (double)l_nrow * sizeof(int) * l_valid_col_num / 1024 / 1024;
    std::cout << "-----------------------Input/Output Info-----------------------" << std::endl;
    std::cout << "Table" << std::setw(20) << "Column Number" << std::setw(30) << "Row Number" << std::endl;
    std::cout << "L" << std::setw(24) << o_valid_col_num << std::setw(30) << o_nrow << std::endl;
    std::cout << "R" << std::setw(24) << l_valid_col_num << std::setw(30) << l_nrow << std::endl;
    std::cout << "LxR" << std::setw(22) << out_valid_col_num << std::setw(30) << out_nrow_sum << std::endl;
    std::cout << "-----------------------Data Transfer Info-----------------------" << std::endl;
    std::cout << "H2D size (Left Table) = " << in1_bytes << " MB" << std::endl;
    std::cout << "H2D size (Right Table) = " << in2_bytes << " MB" << std::endl;
    std::cout << "D2H size = " << out_bytes << " MB" << std::endl;

    std::cout << "-----------------------Performance Info-----------------------" << std::endl;
    double all_bytes = (double)(in1_bytes + in2_bytes) / 1024;
    printf("Total kernel execution time : %ld.%03ld msec\n", kernel_ex_time / 1000000,
           (kernel_ex_time % 1000000) / 1000);
    double tvtime = tv.getMilliSec();
    printf("End-to-end time: %lf msec\n", tvtime);
    printf("End-to-end throughput: %lf GB/s\n", all_bytes / (tvtime / 1000000));

    //--------------release---------------
    for (int i = 0; i < 8; i++) {
        clReleaseMemObject(buf_table_o[i]);
        for (int k = 0; k < 2; k++) {
            clReleaseMemObject(buf_table_l[i][k]);
            clReleaseMemObject(buf_table_out[i][k]);
        }
    }

    clReleaseMemObject(buf_cfg5s);
    for (int k = 0; k < PU_NM * 2; k++) {
        clReleaseMemObject(buf_tmp[k]);
    }
    clReleaseMemObject(buf_meta_build_in);
    clReleaseMemObject(buf_meta_probe_in);
    clReleaseMemObject(buf_meta_probe_out[0]);
    clReleaseMemObject(buf_meta_probe_out[1]);

    clReleaseEvent(evt_build_h2d[0]);
    clReleaseEvent(evt_build_krn[0]);
    for (size_t i = 0; i < slice_num; i++) {
        clReleaseEvent(evt_probe_h2d[i][0]);
        clReleaseEvent(evt_probe_krn[i][0]);
        clReleaseEvent(evt_probe_d2h[i][0]);
    }

    clReleaseKernel(bkernel);
    clReleaseKernel(pkernel[0]);
    clReleaseKernel(pkernel[1]);

    return SUCCESS;
}
struct queue_struct_join {
    // the sec index
    int sec;
    // the partition index
    int p;
    // the nrow setup of MetaTable, only the last round nrow is different to
    // per_slice_nrow in probe
    int meta_nrow;
    // updating meta info (nrow) for each partition&slice, due to async, this
    // change is done in threads
    MetaTable* meta;
    // dependency event num
    int num_event_wait_list;
    // dependency events
    cl_event* event_wait_list;
    // user event to trace current memcpy operation
    cl_event* event;
    // memcpy src locations
    int valid_col_num;
    char* ptr_src[8];
    // ----- part o memcpy in used -----
    // data size of memcpy in
    int type_size[8];
    int size[8];
    // memcpy dst locations
    char* ptr_dst[8];
    // ----- part o memcpy out used -----
    int partition_num;
    // the allocated size (nrow) of each partititon out buffer
    int part_max_nrow_512;
    // memcpy dst locations, used in part memcpy out
    char*** part_ptr_dst;
    // ----- probe memcpy used -----
    int slice;
    // the nrow of first (slice_num - 1) rounds, only valid in probe memcpy in
    int per_slice_nrow;
};
//
class threading_pool {
   public:
    const int size_apu_512 = 64;
    std::thread part_o_in_t;
    std::thread part_o_out_t;
    std::thread part_l_in_ping_t;
    std::thread part_l_in_pong_t;
    std::thread part_l_out_ping_t;
    std::thread part_l_out_pong_t;
    std::thread build_in_t;
    std::thread probe_in_ping_t;
    std::thread probe_in_pong_t;
    std::thread probe_out_t;

    std::queue<queue_struct_join> q0;      // part o memcpy in used queue
    std::queue<queue_struct_join> q1;      // part o memcpy out used queue
    std::queue<queue_struct_join> q2_ping; // part l memcpy in used queue
    std::queue<queue_struct_join> q2_pong; // part l memcpy in used queue
    std::queue<queue_struct_join> q3_ping; // part l memcpy out used queue
    std::queue<queue_struct_join> q3_pong; // part l memcpy out used queue
    std::queue<queue_struct_join> q4;      // build memcpy in used queue
    std::queue<queue_struct_join> q5_ping; // probe memcpy in used queue
    std::queue<queue_struct_join> q5_pong; // probe memcpy in used queue
    std::queue<queue_struct_join> q6;      // probe memcpy out used queue

    // the flag indicate each thread is running
    std::atomic<bool> q0_run;
    std::atomic<bool> q1_run;
    std::atomic<bool> q2_ping_run;
    std::atomic<bool> q2_pong_run;
    std::atomic<bool> q3_ping_run;
    std::atomic<bool> q3_pong_run;
    std::atomic<bool> q4_run;
    std::atomic<bool> q5_run_ping;
    std::atomic<bool> q5_run_pong;
    std::atomic<bool> q6_run;

    // the nrow of each partition
    int o_new_part_offset[256];
    std::atomic<int> l_new_part_offset[256];
    int probe_out_nrow_accu = 0;
    int toutrow[256][32];

    // the buffer size of each output partition of Tab L.
    int l_partition_out_col_part_nrow_max;
    // the buffer size of each output partition of Tab O.
    int o_partition_out_col_part_nrow_max;

    // constructor
    threading_pool(){};

    // table O memcpy in thread
    // -----------------------------------------------------------------------
    // memcpy(table_o_partition_in_col[0][kid], table_o_user_col0_sec[sec],
    // sizeof(TPCH_INT) *
    // table_o_sec_depth[sec]);
    // memcpy(table_o_partition_in_col[1][kid], table_o_user_col1_sec[sec],
    // sizeof(TPCH_INT) *
    // table_o_sec_depth[sec]);
    // meta_o_partition_in[kid].setColNum(2);
    // meta_o_partition_in[kid].setCol(0, 0, table_o_sec_depth[sec]);
    // meta_o_partition_in[kid].setCol(1, 1, table_o_sec_depth[sec]);
    // meta_o_partition_in[kid].meta();
    // -----------------------------------------------------------------------
    void part_o_memcpy_in_t() {
        while (q0_run) {
#if valgrind_debug
            sleep(1);
#endif
            while (!q0.empty()) {
                queue_struct_join q = q0.front();
                clWaitForEvents(q.num_event_wait_list, q.event_wait_list);

#if JOIN_PERF_PROFILE_2
                gqe::utils::Timer tv;
                tv.add(); // 0
#endif
                for (int i = 0; i < q.valid_col_num; i++) {
                    memcpy(q.ptr_dst[i], q.ptr_src[i], q.size[i]);
                }

                q.meta->setColNum(q.valid_col_num);
                for (int i = 0; i < q.valid_col_num; i++) {
                    q.meta->setCol(i, i, q.meta_nrow);
                }
                q.meta->meta();

                clSetUserEventStatus(q.event[0], CL_COMPLETE);
                // remove the first element after processing it.
                q0.pop();

#if JOIN_PERF_PROFILE_2
                tv.add(); // 1
                double tvtime = tv.getMilliSec();
                double q_total_size = 0;
                for (int i = 0; i < q.valid_col_num; i++) {
                    q_total_size += q.size[i];
                }
                double input_memcpy_size = q_total_size / 1024 / 1024;

                std::cout << "Tab O sec: " << q.sec << " memcpy in, size: " << input_memcpy_size
                          << " MB, time: " << tvtime / 1000
                          << " ms, throughput: " << input_memcpy_size / 1024 / ((double)tvtime / 1000000) << "GB/s"
                          << std::endl;
#endif
            }
        }
    };

    // table O memcpy out thread
    void part_o_memcpy_out_t() {
        while (q1_run) {
#if Valgrind_debug
            sleep(1);
#endif
            while (!q1.empty()) {
                queue_struct_join q = q1.front();
                clWaitForEvents(q.num_event_wait_list, q.event_wait_list);

#if JOIN_PERF_PROFILE_2
                gqe::utils::Timer tv;
                tv.add(); // 0
#endif

                int o_partition_out_col_part_depth = q.part_max_nrow_512;

                int* nrow_per_part_o = q.meta->getPartLen();

                for (int p = 0; p < q.partition_num; p++) {
                    int sec_partitioned_res_part_nrow = nrow_per_part_o[p];
                    // error out when the partition out buffer isze is smaller than output
                    // nrow
                    if (sec_partitioned_res_part_nrow > o_partition_out_col_part_nrow_max) {
                        std::cerr << "partition out nrow: " << sec_partitioned_res_part_nrow
                                  << ", buffer size: " << o_partition_out_col_part_nrow_max << std::endl;
                        std::cerr << "ERROR: Table O output partition size is smaller than "
                                     "required!"
                                  << std::endl;
                        exit(1);
                    }
                    int offset = o_new_part_offset[p];
                    o_new_part_offset[p] += sec_partitioned_res_part_nrow;
                    for (int i = 0; i < q.valid_col_num; i++) {
                        // q.type_size[i] = 4;
                        memcpy(q.part_ptr_dst[p][i] + offset * q.type_size[i],
                               q.ptr_src[i] + p * o_partition_out_col_part_depth * (size_apu_512), // size_apu_512
                               q.type_size[i] * sec_partitioned_res_part_nrow);
                    }
                }

                clSetUserEventStatus(q.event[0], CL_COMPLETE);
                q1.pop();

#if JOIN_PERF_PROFILE_2
                tv.add(); // 1
                double tvtime = tv.getMilliSec();
                int output_memcpy_nrow = 0;
                for (int p = 0; p < q.partition_num; p++) {
                    output_memcpy_nrow += nrow_per_part_o[p];
                }

                double output_memcpy_size = (double)output_memcpy_nrow * q.valid_col_num * 4 / 1024 / 1024;
                std::cout << "Tab O sec: " << q.sec << " memcpy out, size: " << output_memcpy_size << " MB, with "
                          << q.partition_num * q.valid_col_num << " times, total time: " << tvtime / 1000
                          << " ms, throughput: " << output_memcpy_size / 1024 / ((double)tvtime / 1000000) << "GB/s"
                          << std::endl;
#endif
            }
        }
    };
    // table L memcpy in thread
    void part_l_memcpy_in_ping_t() {
        while (q2_ping_run) {
#if Valgrind_debug
            sleep(1);
#endif
            while (!q2_ping.empty()) {
                queue_struct_join q = q2_ping.front();
                clWaitForEvents(q.num_event_wait_list, q.event_wait_list);

#if JOIN_PERF_PROFILE_2
                gqe::utils::Timer tv;
                tv.add(); // 0
#endif
                for (int i = 0; i < q.valid_col_num; i++) {
                    memcpy(q.ptr_dst[i], q.ptr_src[i], q.size[i]);
                }

                q.meta->setColNum(q.valid_col_num);
                for (int i = 0; i < q.valid_col_num; i++) {
                    q.meta->setCol(i, i, q.meta_nrow);
                }
                q.meta->meta();

                clSetUserEventStatus(q.event[0], CL_COMPLETE);
                // remove the first element after processing it.
                q2_ping.pop();

#if JOIN_PERF_PROFILE_2
                tv.add(); // 1
                double tvtime = tv.getMilliSec();
                double input_memcpy_size = (double)q.size * q.valid_col_num / 1024 / 1024;

                std::cout << "Tab L sec: " << q.sec << " memcpy in, size: " << input_memcpy_size
                          << " MB, time: " << tvtime / 1000
                          << " ms, throughput: " << input_memcpy_size / 1024 / ((double)tvtime / 1000000) << "GB/s"
                          << std::endl;
#endif
            }
        }
    };

    // table L memcpy in thread
    void part_l_memcpy_in_pong_t() {
        while (q2_pong_run) {
#if Valgrind_debug
            sleep(1);
#endif
            while (!q2_pong.empty()) {
                queue_struct_join q = q2_pong.front();
                clWaitForEvents(q.num_event_wait_list, q.event_wait_list);

#if JOIN_PERF_PROFILE_2
                gqe::utils::Timer tv;
                tv.add(); // 0
#endif
                for (int i = 0; i < q.valid_col_num; i++) {
                    memcpy(q.ptr_dst[i], q.ptr_src[i], q.size[i]);
                }

                q.meta->setColNum(q.valid_col_num);
                for (int i = 0; i < q.valid_col_num; i++) {
                    q.meta->setCol(i, i, q.meta_nrow);
                }

                q.meta->meta();

                clSetUserEventStatus(q.event[0], CL_COMPLETE);
                // remove the first element after processing it.
                q2_pong.pop();

#if JOIN_PERF_PROFILE_2
                tv.add(); // 1
                double tvtime = tv.getMilliSec();
                double input_memcpy_size = (double)q.size * q.valid_col_num / 1024 / 1024;

                std::cout << "Tab L sec: " << q.sec << " memcpy in, size: " << input_memcpy_size
                          << " MB, time: " << tvtime / 1000
                          << " ms, throughput: " << input_memcpy_size / 1024 / ((double)tvtime / 1000000) << "GB/s"
                          << std::endl;
#endif
            }
        }
    };

    // table L memcpy out thread
    void part_l_memcpy_out_ping_t() {
        while (q3_ping_run) {
#if Valgrind_debug
            sleep(1);
#endif
            while (!q3_ping.empty()) {
                queue_struct_join q = q3_ping.front();
                clWaitForEvents(q.num_event_wait_list, q.event_wait_list);

#if JOIN_PERF_PROFILE_2
                gqe::utils::Timer tv;
                tv.add(); // 0
#endif
                int l_partition_out_col_part_depth = q.part_max_nrow_512;

                int* nrow_per_part_l = q.meta->getPartLen();

                for (int p = 0; p < q.partition_num; ++p) {
                    int sec_partitioned_res_part_nrow = nrow_per_part_l[p];
                    if (sec_partitioned_res_part_nrow > l_partition_out_col_part_nrow_max) {
                        std::cerr << "partition out nrow: " << sec_partitioned_res_part_nrow
                                  << ", buffer size: " << l_partition_out_col_part_nrow_max << std::endl;
                        std::cerr << "ERROR: Table L output partition size is smaller than "
                                     "required!"
                                  << std::endl;
                        exit(1);
                    }

                    int offset = l_new_part_offset[p];
                    l_new_part_offset[p] += sec_partitioned_res_part_nrow;
                    for (int i = 0; i < q.valid_col_num; i++) {
                        // q.type_size[i] = 4;
                        memcpy(q.part_ptr_dst[p][i] + offset * q.type_size[i],
                               q.ptr_src[i] + p * l_partition_out_col_part_depth * size_apu_512,
                               q.type_size[i] * sec_partitioned_res_part_nrow);
                    }
                }

                clSetUserEventStatus(q.event[0], CL_COMPLETE);
                q3_ping.pop();

#if JOIN_PERF_PROFILE_2
                tv.add(); // 1
                double tvtime = tv.getMilliSec();
                int output_memcpy_nrow = 0;
                for (int p = 0; p < q.partition_num; p++) {
                    output_memcpy_nrow += nrow_per_part_l[p];
                }

                double output_memcpy_size = (double)output_memcpy_nrow * q.valid_col_num * 4 / 1024 / 1024;
                std::cout << "Tab L sec: " << q.sec << " memcpy out, size: " << output_memcpy_size << " MB, with "
                          << q.partition_num * q.valid_col_num << " times, total time: " << tvtime / 1000
                          << " ms, throughput: " << output_memcpy_size / 1024 / ((double)tvtime / 1000000) << "GB/s"
                          << std::endl;
#endif
            }
        }
    };

    // table L memcpy out thread
    void part_l_memcpy_out_pong_t() {
        while (q3_pong_run) {
#if Valgrind_debug
            sleep(1);
#endif
            while (!q3_pong.empty()) {
                queue_struct_join q = q3_pong.front();
                clWaitForEvents(q.num_event_wait_list, q.event_wait_list);

#if JOIN_PERF_PROFILE_2
                gqe::utils::Timer tv;
                tv.add(); // 0
#endif
                int l_partition_out_col_part_depth = q.part_max_nrow_512;

                int* nrow_per_part_l = q.meta->getPartLen();

                for (int p = 0; p < q.partition_num; ++p) {
                    int sec_partitioned_res_part_nrow = nrow_per_part_l[p];
                    if (sec_partitioned_res_part_nrow > l_partition_out_col_part_nrow_max) {
                        std::cerr << "partition out nrow: " << sec_partitioned_res_part_nrow
                                  << ", buffer size: " << l_partition_out_col_part_nrow_max << std::endl;
                        std::cerr << "ERROR: Table L output partition size is smaller than "
                                     "required!"
                                  << std::endl;
                        exit(1);
                    }

                    int offset = l_new_part_offset[p];
                    l_new_part_offset[p] += sec_partitioned_res_part_nrow;
                    for (int i = 0; i < q.valid_col_num; i++) {
                        memcpy(q.part_ptr_dst[p][i] + offset * q.type_size[i],
                               (q.ptr_src[i] + p * l_partition_out_col_part_depth * size_apu_512),
                               q.type_size[i] * sec_partitioned_res_part_nrow);
                    }
                }

                clSetUserEventStatus(q.event[0], CL_COMPLETE);
                q3_pong.pop();

#if JOIN_PERF_PROFILE_2
                tv.add(); // 1
                double tvtime = tv.getMilliSec();
                int output_memcpy_nrow = 0;
                for (int p = 0; p < q.partition_num; p++) {
                    output_memcpy_nrow += nrow_per_part_l[p];
                }

                double output_memcpy_size = (double)output_memcpy_nrow * q.valid_col_num * 4 / 1024 / 1024;
                std::cout << "Tab L sec: " << q.sec << " memcpy out, size: " << output_memcpy_size << " MB, with "
                          << q.partition_num * q.valid_col_num << " times, total time: " << tvtime / 1000
                          << " ms, throughput: " << output_memcpy_size / 1024 / ((double)tvtime / 1000000) << "GB/s"
                          << std::endl;
#endif
            }
        }
    };
    // build memcpy in thread
    // memcpy(table_o_build_in_col[0], table_o_new_part_col[p][0],
    // table_o_build_in_size);
    // memcpy(table_o_build_in_col[1], table_o_new_part_col[p][1],
    // table_o_build_in_size);
    void build_memcpy_in_t() {
        while (q4_run) {
#if Valgrind_debug
            sleep(1);
#endif
            while (!q4.empty()) {
                queue_struct_join q = q4.front();
                clWaitForEvents(q.num_event_wait_list, q.event_wait_list);

#if JOIN_PERF_PROFILE_2
                gqe::utils::Timer tv;
                tv.add(); // 0
#endif
                for (int i = 0; i < q.valid_col_num; i++) {
                    memcpy(q.ptr_dst[i], q.ptr_src[i], q.size[i]);
                }

                q.meta->setColNum(q.valid_col_num);
                for (int i = 0; i < q.valid_col_num; i++) {
                    q.meta->setCol(i, i, q.meta_nrow);
                }
                q.meta->meta();

                clSetUserEventStatus(q.event[0], CL_COMPLETE);
                // remove the first element after processing it.
                q4.pop();

#if JOIN_PERF_PROFILE_2
                tv.add(); // 1
                double tvtime = tv.getMilliSec();

                double data_size = (double)q.size * q.valid_col_num / 1024 / 1024;
                std::cout << "Tab O p: " << q.p << " build memcpy in, size: " << data_size
                          << " MB, time: " << tvtime / 1000
                          << " ms, throughput: " << data_size / 1024 / ((double)tvtime / 1000000) << " GB/s"
                          << std::endl;
#endif
            }
        }
    };
    // probe memcpy in thread
    // memcpy(reinterpret_cast<int*>(table_l_probe_in_col[0][sid]),
    //        reinterpret_cast<int*>(table_l_new_part_col[p][0]) + per_slice_nrow
    //        * slice,
    //        table_l_probe_in_slice_nrow_sid_size);
    // memcpy(reinterpret_cast<int*>(table_l_probe_in_col[1][sid]),
    //        reinterpret_cast<int*>(table_l_new_part_col[p][1]) + per_slice_nrow
    //        * slice,
    //        table_l_probe_in_slice_nrow_sid_size);
    // memcpy(reinterpret_cast<int*>(table_l_probe_in_col[2][sid]),
    //        reinterpret_cast<int*>(table_l_new_part_col[p][2]) + per_slice_nrow
    //        * slice,
    //        table_l_probe_in_slice_nrow_sid_size);
    void probe_memcpy_in_ping_t() {
        while (q5_run_ping) {
#if Valgrind_debug
            sleep(1);
#endif
            while (!q5_ping.empty()) {
                queue_struct_join q = q5_ping.front();
                clWaitForEvents(q.num_event_wait_list, q.event_wait_list);

                for (int i = 0; i < q.valid_col_num; i++) {
                    memcpy(q.ptr_dst[i], q.ptr_src[i] + q.per_slice_nrow * q.slice * q.type_size[i], q.size[i]);
                }

                q.meta->setColNum(q.valid_col_num);
                for (int i = 0; i < q.valid_col_num; i++) {
                    q.meta->setCol(i, i, q.meta_nrow);
                }
                q.meta->meta();

                clSetUserEventStatus(q.event[0], CL_COMPLETE);
                // remove the first element after processing it.
                q5_ping.pop();
            }
        }
    }
    void probe_memcpy_in_pong_t() {
        while (q5_run_pong) {
#if Valgrind_debug
            sleep(1);
#endif
            while (!q5_pong.empty()) {
                queue_struct_join q = q5_pong.front();
                clWaitForEvents(q.num_event_wait_list, q.event_wait_list);

                for (int i = 0; i < q.valid_col_num; i++) {
                    memcpy(q.ptr_dst[i], q.ptr_src[i] + q.per_slice_nrow * q.slice * q.type_size[i], q.size[i]);
                }

                q.meta->setColNum(q.valid_col_num);
                for (int i = 0; i < q.valid_col_num; i++) {
                    q.meta->setCol(i, i, q.meta_nrow);
                }
                q.meta->meta();

                clSetUserEventStatus(q.event[0], CL_COMPLETE);
                // remove the first element after processing it.
                q5_pong.pop();
            }
        }
    }

    // probe memcpy out thread
    // only copy necessary output rows back to the user final output space.
    void probe_memcpy_out_t() {
        while (q6_run) {
#if Valgrind_debug
            sleep(1);
#endif
            while (!q6.empty()) {
                queue_struct_join q = q6.front();
                clWaitForEvents(q.num_event_wait_list, q.event_wait_list);

#if JOIN_PERF_PROFILE_2
                gqe::utils::Timer tv;
                tv.add(); // 0
#endif
                // save output data nrow
                int probe_out_nrow = q.meta->getColLen();
                toutrow[q.p][q.slice] = probe_out_nrow;
                int h_dst_size = q.part_max_nrow_512 * size_apu_512;
                for (int i = 0; i < q.valid_col_num; i++) {
                    int u_dst_size = q.size[i]; // user buffer size
                    int pout_size = probe_out_nrow * q.type_size[i];
                    if (pout_size > h_dst_size) { // host buffer is not enough
                        std::cerr << "Error in checking probe memcpy out size: host buffer size(" << h_dst_size << ")"
                                  << " < output size(" << pout_size << ")" << std::endl;
                        std::cerr << "Please using the JoinStrategyManualSet strategy, and set enough buffer size for "
                                     "output table"
                                  << std::endl;
                        exit(1);
                    }
                    if (probe_out_nrow_accu * q.type_size[i] + pout_size > u_dst_size) {
                        std::cerr << "Error in checking probe memcpy out size: user buffer size(" << u_dst_size << ")"
                                  << " < output size(" << (probe_out_nrow_accu * q.type_size[i] + pout_size) << ")"
                                  << std::endl;
                        std::cerr << "Please using the JoinStrategyManualSet strategy, and set enough buffer size for "
                                     "output table"
                                  << std::endl;
                        exit(1);
                    }
                    memcpy(q.ptr_dst[i] + probe_out_nrow_accu * q.type_size[i], q.ptr_src[i], pout_size);
                }

                // save the accumulate output nrow, to record the data offset
                probe_out_nrow_accu += probe_out_nrow;
                clSetUserEventStatus(q.event[0], CL_COMPLETE);
                // remove the first element after processing it.
                q6.pop();

#if JOIN_PERF_PROFILE_2
                tv.add(); // 1
                double tvtime = tv.getMilliSec();
                double data_size = (double)pout_size * q.valid_col_num / 1024 / 1024;
                std::cout << "Tab L p: " << q.p << ", s: " << q.slice << ", probe memcpy out, size: " << data_size
                          << " MB, time: " << tvtime / 1000
                          << " ms, throughput: " << data_size / 1024 / ((double)tvtime / 1000000) << " GB/s"
                          << std::endl;

#endif
            }
        }
    }

    // initialize the table O partition threads
    void parto_init() {
        memset(o_new_part_offset, 0, sizeof(int) * 256);
        memset(l_new_part_offset, 0, sizeof(int) * 256);
        for (int i = 0; i < 256; i++) {
            memset(toutrow[i], 0, sizeof(int) * 32);
        }

        // start the part o memcpy in thread and non-stop running
        q0_run = 1;
        part_o_in_t = std::thread(&threading_pool::part_o_memcpy_in_t, this);

        // start the part o memcpy out thread and non-stop running
        q1_run = 1;
        part_o_out_t = std::thread(&threading_pool::part_o_memcpy_out_t, this);
    }

    // initialize the table L partition threads
    void partl_init() {
        // start the part o memcpy in thread and non-stop running
        q2_ping_run = 1;
        part_l_in_ping_t = std::thread(&threading_pool::part_l_memcpy_in_ping_t, this);

        // start the part o memcpy in thread and non-stop running
        q2_pong_run = 1;
        part_l_in_pong_t = std::thread(&threading_pool::part_l_memcpy_in_pong_t, this);

        // start the part o memcpy in thread and non-stop running
        q3_ping_run = 1;
        part_l_out_ping_t = std::thread(&threading_pool::part_l_memcpy_out_ping_t, this);

        // start the part o memcpy in thread and non-stop running
        q3_pong_run = 1;
        part_l_out_pong_t = std::thread(&threading_pool::part_l_memcpy_out_pong_t, this);
    }

    // initialize the hash join threads
    void hj_init() {
        // start the build memcpy in thread and non-stop running
        q4_run = 1;
        build_in_t = std::thread(&threading_pool::build_memcpy_in_t, this);

        // start the probe memcpy in thread and non-stop running
        q5_run_ping = 1;
        probe_in_ping_t = std::thread(&threading_pool::probe_memcpy_in_ping_t, this);

        // start the probe memcpy in thread and non-stop running
        q5_run_pong = 1;
        probe_in_pong_t = std::thread(&threading_pool::probe_memcpy_in_pong_t, this);

        // start the probe memcpy out thread and non-stop running
        q6_run = 1;
        probe_out_t = std::thread(&threading_pool::probe_memcpy_out_t, this);
    };
};

ErrCode Joiner::join_sol2(Table& tab_a, Table& tab_b, Table& tab_c, JoinConfig& jcmd, std::vector<size_t> params) {
    ap_uint<512>* q5s_cfg_part = jcmd.getPartConfigBits();
    ap_uint<512>* q5s_cfg_join = jcmd.getJoinConfigBits();
    std::vector<std::vector<int8_t> > q5s_part_scan = jcmd.getScanSwShuf1();
    std::vector<std::vector<int8_t> > q5s_join_scan = jcmd.getScanSwShuf2();
    gqe::utils::MM mm;
    // read params from user
    int sec_o = params[1];
    int sec_l = params[2];
    int slice_num = params[3];
    int log_part = params[4];
    bool probe_buf_size_auto = params[5];
    // get total row number and valid col number
    size_t o_nrow = tab_a.getRowNum();
    size_t l_nrow = tab_b.getRowNum();
    // int o_valid_col_num = tab_a.getColNum();
    // int l_valid_col_num = tab_b.getColNum();
    int o_valid_col_num = q5s_part_scan[0].size();
    int l_valid_col_num = q5s_part_scan[1].size();
    int out_valid_col_num = tab_c.getColNum();

    // start threading pool threads
    threading_pool pool;
    pool.parto_init();
    // -------------------------setup partition O ----------------------------
    // Assuming table O can not be put to FPGA DDR, divided into two sections
    int table_o_sec_num = sec_o;
    int* table_o_sec_depth;
    // the col depth/nrow of original table o
    if (table_o_sec_num == 0) {
        table_o_sec_num = tab_a.getSecNum();
        table_o_sec_depth = new int[table_o_sec_num];
        for (int sec = 0; sec < table_o_sec_num; sec++) {
            table_o_sec_depth[sec] = tab_a.getSecRowNum(sec);
        }
    } else {
        table_o_sec_depth = new int[table_o_sec_num];
        int table_o_col_depth = o_nrow;
        int table_o_sec_depth_each = (table_o_col_depth + table_o_sec_num - 1) / table_o_sec_num;
        for (int sec = 0; sec < table_o_sec_num - 1; sec++) {
            table_o_sec_depth[sec] = table_o_sec_depth_each;
        }
        table_o_sec_depth[table_o_sec_num - 1] = table_o_col_depth - table_o_sec_depth_each * (table_o_sec_num - 1);
    }
// check each section number
// //////////////////////////////////////////////////
#ifdef USER_DEBUG
    std::cout << "table_o_sec_num: " << table_o_sec_num << std::endl;
    for (int i = 0; i < table_o_sec_num; i++) {
        std::cout << "table_o_sec_depth: " << table_o_sec_depth[i] << std::endl;
    }
#endif
    // //////////////////////////////////////////////////
    for (int sec = 0; sec < table_o_sec_num; sec++) {
        if (table_o_sec_depth[sec] < table_o_sec_num) {
            std::cerr << "Error: Table O section size is smaller than section number!!!";
            std::cerr << "sec size of Table O: " << table_o_sec_depth[sec] << ", ";
            std::cerr << "sec number of Table O: " << table_o_sec_num << std::endl;
            exit(1);
        }
    }
    // Get each col type
    // get each section byte size column * sec_num
    //
    int table_o_sec_size[8][table_o_sec_num];
    int table_o_col_types[8];
    for (int j = 0; j < o_valid_col_num; j++) {
        table_o_col_types[j] = tab_a.getColTypeSize(q5s_part_scan[0][j]);

        for (int i = 0; i < table_o_sec_num; i++) {
            table_o_sec_size[j][i] = table_o_sec_depth[i] * table_o_col_types[j];
        }
    }
    // get max seciction buffer depth and byte size
    int table_o_sec_depth_max = 0;
    int table_o_sec_size_max[8];
    for (int i = 0; i < table_o_sec_num; i++) {
        if (table_o_sec_depth[i] > table_o_sec_depth_max) {
            table_o_sec_depth_max = table_o_sec_depth[i];
        }
    }
    for (int j = 0; j < o_valid_col_num; j++) {
        table_o_sec_size_max[j] = table_o_sec_depth_max * table_o_col_types[j];
    }

    // data load from disk. due to table size, data read into two sections
    char* table_o_user_col_sec[8][table_o_sec_num];
    for (int j = 0; j < o_valid_col_num; j++) {
        for (int i = 0; i < table_o_sec_num; ++i) {
            table_o_user_col_sec[j][i] = tab_a.getColPointer(q5s_part_scan[0][j], sec_o, i);
        }
    }
    for (int j = o_valid_col_num; j < 8; j++) {
        for (int i = 0; i < table_o_sec_num; ++i) {
            table_o_user_col_sec[j][i] = mm.aligned_alloc<char>(32);
        }
    }

    // host side pinned buffers for partition kernel
    char* table_o_partition_in_col[8][2]; // 8cols, ping-pong
    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 2; j++) {
            if (o_valid_col_num > i) {
                table_o_partition_in_col[i][j] = mm.aligned_alloc<char>(table_o_sec_size_max[i]);
            } else {
                table_o_partition_in_col[i][j] = mm.aligned_alloc<char>(8); // table_o_sec_depth);
            }
        }
    }
    const int size_apu_512 = 64;
    // partition setups
    const int k_depth = 512;
    int log_partition_num = log_part;
    const int partition_num = 1 << log_partition_num;
    // partition output col size,  setup the proper size by multiple 1.5
    // int o_partition_out_col_depth_init = table_o_sec_depth_each * 1.3 / VEC_LEN;
    int o_partition_out_col_depth_init = table_o_sec_depth_max * 1.3 / VEC_LEN;
    assert(o_partition_out_col_depth_init > 0 && "Table O output col size must > 0");
    int o_partition_out_col_part_depth = (o_partition_out_col_depth_init + partition_num - 1) / partition_num;

    // the depth of each partition in each col.
    pool.o_partition_out_col_part_nrow_max = o_partition_out_col_part_depth * 16;
    // update depth to make sure the buffer size is aligned by partititon_num *
    // o_partition_out_col_part_depth;
    int o_partition_out_col_depth = partition_num * o_partition_out_col_part_depth;
    int o_partition_out_col_size = o_partition_out_col_depth * size_apu_512;

    // partition output data
    char* table_o_partition_out_col[8][2];
    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 2; j++) {
            if (o_valid_col_num > i) {
                table_o_partition_out_col[i][j] = mm.aligned_alloc<char>(o_partition_out_col_depth * size_apu_512);
            } else {
                table_o_partition_out_col[i][j] = mm.aligned_alloc<char>(8); // o_partition_out_col_depth);
            }
        }
    }

    //--------------- metabuffer setup O -----------------
    // using col0 and col1 buffer during build,setup partition kernel used meta
    // input
    MetaTable meta_o_partition_in[2];
    for (int k = 0; k < 2; k++) {
        meta_o_partition_in[k].setColNum(o_valid_col_num);
        for (int i = 0; i < o_valid_col_num; i++) {
            meta_o_partition_in[k].setCol(i, i, table_o_sec_depth[i]);
        }
        meta_o_partition_in[k].meta();
    }

    // ouput col0,1,2,3 buffers data, with order: 0 1 2 3.
    // setup partition kernel used meta output
    MetaTable meta_o_partition_out[2];
    for (int k = 0; k < 2; k++) {
        meta_o_partition_out[k].setColNum(o_valid_col_num);
        meta_o_partition_out[k].setPartition(partition_num, o_partition_out_col_part_depth);
        for (int i = 0; i < o_valid_col_num; i++) {
            meta_o_partition_out[k].setCol(i, i, o_partition_out_col_depth);
        }
        meta_o_partition_out[k].meta();
    }

    // partition kernel settings
    cl_kernel partkernel_O[2], partkernel_L[2];
    partkernel_O[0] = clCreateKernel(prg, "gqePart", &err);

    partkernel_O[1] = clCreateKernel(prg, "gqePart", &err);
    partkernel_L[0] = clCreateKernel(prg, "gqePart", &err);
    partkernel_L[1] = clCreateKernel(prg, "gqePart", &err);
#ifdef USER_DEBUG
    std::cout << "Kernel has been created\n";
#endif

    cl_mem_ext_ptr_t mext_table_o_partition_in_col[8][2];
    cl_mem_ext_ptr_t mext_meta_o_partition_in[2], mext_meta_o_partition_out[2];
    cl_mem_ext_ptr_t mext_table_o_partition_out_col[8][2];

    for (int i = 0; i < 8; ++i) {
        mext_table_o_partition_in_col[i][0] = {3 + i, table_o_partition_in_col[i][0], partkernel_O[0]};
        mext_table_o_partition_in_col[i][1] = {3 + i, table_o_partition_in_col[i][1], partkernel_O[1]};
    }

    mext_meta_o_partition_in[0] = {11, meta_o_partition_in[0].meta(), partkernel_O[0]};
    mext_meta_o_partition_in[1] = {11, meta_o_partition_in[1].meta(), partkernel_O[1]};

    mext_meta_o_partition_out[0] = {12, meta_o_partition_out[0].meta(), partkernel_O[0]};
    mext_meta_o_partition_out[1] = {12, meta_o_partition_out[1].meta(), partkernel_O[1]};

    for (int i = 0; i < 8; ++i) {
        mext_table_o_partition_out_col[i][0] = {13 + i, table_o_partition_out_col[i][0], partkernel_O[0]};
        mext_table_o_partition_out_col[i][1] = {13 + i, table_o_partition_out_col[i][1], partkernel_O[1]};
    }

    cl_mem_ext_ptr_t mext_cfg5s_part = {21, q5s_cfg_part, partkernel_O[0]};
    // dev buffers
    cl_mem buf_table_o_partition_in_col[8][2];
    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 2; j++) {
            if (o_valid_col_num > i) {
                buf_table_o_partition_in_col[i][j] =
                    clCreateBuffer(ctx, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY,
                                   table_o_sec_size_max[i], &mext_table_o_partition_in_col[i][j], &err);
            } else {
                buf_table_o_partition_in_col[i][j] =
                    clCreateBuffer(ctx, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY, 8,
                                   &mext_table_o_partition_in_col[i][j], &err);
            }
        }
    }
    cl_mem buf_table_o_partition_out_col[8][2];
    for (int i = 0; i < 8; i++) {
        for (int k = 0; k < 2; k++) {
            if (o_valid_col_num > i) {
                buf_table_o_partition_out_col[i][k] =
                    clCreateBuffer(ctx, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                                   o_partition_out_col_size, &mext_table_o_partition_out_col[i][k], &err);
            } else {
                buf_table_o_partition_out_col[i][k] =
                    clCreateBuffer(ctx, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE, 8,
                                   &mext_table_o_partition_out_col[i][k], &err);
            }
        }
    }

    cl_mem buf_cfg5s_part = clCreateBuffer(ctx, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY,
                                           (size_apu_512 * 9), &mext_cfg5s_part, &err);
    cl_mem buf_meta_o_partition_in[2];
    buf_meta_o_partition_in[0] = clCreateBuffer(ctx, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY,
                                                (size_apu_512 * 8), &mext_meta_o_partition_in[0], &err);
    buf_meta_o_partition_in[1] = clCreateBuffer(ctx, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY,
                                                (size_apu_512 * 8), &mext_meta_o_partition_in[1], &err);

    cl_mem buf_meta_o_partition_out[2];
    buf_meta_o_partition_out[0] = clCreateBuffer(ctx, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                                                 (size_apu_512 * 24), &mext_meta_o_partition_out[0], &err);
    buf_meta_o_partition_out[1] = clCreateBuffer(ctx, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                                                 (size_apu_512 * 24), &mext_meta_o_partition_out[1], &err);
//------------------------------end Order table
// partition-----------------------------
//------------------------------------------------------------------------------------
//
//----------------------------partition O run-------------------------------
#ifdef USER_DEBUG
    std::cout << "------------------- Partitioning O table -----------------" << std::endl;
#endif
    // set args and enqueue kernel
    const int idx_o = 0;
    int j = 0;
    for (int k = 0; k < 2; k++) {
        j = 0;
        clSetKernelArg(partkernel_O[k], j++, sizeof(int), &k_depth);
        clSetKernelArg(partkernel_O[k], j++, sizeof(int), &idx_o);
        clSetKernelArg(partkernel_O[k], j++, sizeof(int), &log_partition_num);
        clSetKernelArg(partkernel_O[k], j++, sizeof(cl_mem), &buf_table_o_partition_in_col[0][k]);
        clSetKernelArg(partkernel_O[k], j++, sizeof(cl_mem), &buf_table_o_partition_in_col[1][k]);
        clSetKernelArg(partkernel_O[k], j++, sizeof(cl_mem), &buf_table_o_partition_in_col[2][k]);
        clSetKernelArg(partkernel_O[k], j++, sizeof(cl_mem), &buf_table_o_partition_in_col[3][k]);
        clSetKernelArg(partkernel_O[k], j++, sizeof(cl_mem), &buf_table_o_partition_in_col[4][k]);
        clSetKernelArg(partkernel_O[k], j++, sizeof(cl_mem), &buf_table_o_partition_in_col[5][k]);
        clSetKernelArg(partkernel_O[k], j++, sizeof(cl_mem), &buf_table_o_partition_in_col[6][k]);
        clSetKernelArg(partkernel_O[k], j++, sizeof(cl_mem), &buf_table_o_partition_in_col[7][k]);
        clSetKernelArg(partkernel_O[k], j++, sizeof(cl_mem), &buf_meta_o_partition_in[k]);
        clSetKernelArg(partkernel_O[k], j++, sizeof(cl_mem), &buf_meta_o_partition_out[k]);
        clSetKernelArg(partkernel_O[k], j++, sizeof(cl_mem), &buf_table_o_partition_out_col[0][k]);
        clSetKernelArg(partkernel_O[k], j++, sizeof(cl_mem), &buf_table_o_partition_out_col[1][k]);
        clSetKernelArg(partkernel_O[k], j++, sizeof(cl_mem), &buf_table_o_partition_out_col[2][k]);
        clSetKernelArg(partkernel_O[k], j++, sizeof(cl_mem), &buf_table_o_partition_out_col[3][k]);
        clSetKernelArg(partkernel_O[k], j++, sizeof(cl_mem), &buf_table_o_partition_out_col[4][k]);
        clSetKernelArg(partkernel_O[k], j++, sizeof(cl_mem), &buf_table_o_partition_out_col[5][k]);
        clSetKernelArg(partkernel_O[k], j++, sizeof(cl_mem), &buf_table_o_partition_out_col[6][k]);
        clSetKernelArg(partkernel_O[k], j++, sizeof(cl_mem), &buf_table_o_partition_out_col[7][k]);
        clSetKernelArg(partkernel_O[k], j++, sizeof(cl_mem), &buf_cfg5s_part);
    }

    // partition h2d
    std::vector<cl_mem> partition_o_in_vec[2];
    for (int k = 0; k < 2; k++) {
        partition_o_in_vec[k].push_back(buf_table_o_partition_in_col[0][k]);
        partition_o_in_vec[k].push_back(buf_table_o_partition_in_col[1][k]);
        partition_o_in_vec[k].push_back(buf_table_o_partition_in_col[2][k]);
        partition_o_in_vec[k].push_back(buf_table_o_partition_in_col[3][k]);
        partition_o_in_vec[k].push_back(buf_table_o_partition_in_col[4][k]);
        partition_o_in_vec[k].push_back(buf_table_o_partition_in_col[5][k]);
        partition_o_in_vec[k].push_back(buf_table_o_partition_in_col[6][k]);
        partition_o_in_vec[k].push_back(buf_table_o_partition_in_col[7][k]);
        partition_o_in_vec[k].push_back(buf_meta_o_partition_in[k]);
        partition_o_in_vec[k].push_back(buf_cfg5s_part);
    }

    // partition d2h
    std::vector<cl_mem> partition_o_out_vec[2];
    for (int k = 0; k < 2; k++) {
        partition_o_out_vec[k].push_back(buf_table_o_partition_out_col[0][k]);
        partition_o_out_vec[k].push_back(buf_table_o_partition_out_col[1][k]);
        partition_o_out_vec[k].push_back(buf_table_o_partition_out_col[2][k]);
        partition_o_out_vec[k].push_back(buf_table_o_partition_out_col[3][k]);
        partition_o_out_vec[k].push_back(buf_table_o_partition_out_col[4][k]);
        partition_o_out_vec[k].push_back(buf_table_o_partition_out_col[5][k]);
        partition_o_out_vec[k].push_back(buf_table_o_partition_out_col[6][k]);
        partition_o_out_vec[k].push_back(buf_table_o_partition_out_col[7][k]);
        partition_o_out_vec[k].push_back(buf_meta_o_partition_out[k]);
    }
    clEnqueueMigrateMemObjects(cq, 1, &buf_meta_o_partition_out[0], 0, 0, nullptr, nullptr);
    clEnqueueMigrateMemObjects(cq, 1, &buf_meta_o_partition_out[1], 0, 0, nullptr, nullptr);

    clEnqueueMigrateMemObjects(cq, partition_o_in_vec[0].size(), partition_o_in_vec[0].data(),
                               CL_MIGRATE_MEM_OBJECT_CONTENT_UNDEFINED, 0, nullptr, nullptr);
    clEnqueueMigrateMemObjects(cq, partition_o_in_vec[1].size(), partition_o_in_vec[1].data(),
                               CL_MIGRATE_MEM_OBJECT_CONTENT_UNDEFINED, 0, nullptr, nullptr);
    clEnqueueMigrateMemObjects(cq, partition_o_out_vec[0].size(), partition_o_out_vec[0].data(),
                               CL_MIGRATE_MEM_OBJECT_CONTENT_UNDEFINED, 0, nullptr, nullptr);
    clEnqueueMigrateMemObjects(cq, partition_o_out_vec[1].size(), partition_o_out_vec[1].data(),
                               CL_MIGRATE_MEM_OBJECT_CONTENT_UNDEFINED, 0, nullptr, nullptr);

    // create user partition res cols
    // all sections partition 0 output to same 8-col buffers
    char*** table_o_new_part_col = new char**[partition_num];

    // combine sec0_partition0, sec1_parttion0, ...secN_partition0 in 1 buffer.
    // the depth is
    int o_new_part_depth = o_partition_out_col_part_depth * table_o_sec_num;

    for (int p = 0; p < partition_num; ++p) {
        table_o_new_part_col[p] = new char*[8];
        for (int i = 0; i < 8; ++i) {
            if (i < o_valid_col_num) {
                table_o_new_part_col[p][i] = mm.aligned_alloc<char>(o_new_part_depth * size_apu_512);
                memset(table_o_new_part_col[p][i], 0, o_new_part_depth * size_apu_512);
            } else {
                table_o_new_part_col[p][i] = mm.aligned_alloc<char>(size_apu_512);
            }
        }
    }

    // create events
    std::vector<std::vector<cl_event> > evt_part_o_h2d(table_o_sec_num);
    std::vector<std::vector<cl_event> > evt_part_o_krn(table_o_sec_num);
    std::vector<std::vector<cl_event> > evt_part_o_d2h(table_o_sec_num);
    for (int sec = 0; sec < table_o_sec_num; sec++) {
        evt_part_o_h2d[sec].resize(1);
        evt_part_o_krn[sec].resize(1);
        evt_part_o_d2h[sec].resize(1);
    }

    gqe::utils::Timer tv_opart;
    double tvtime_opart;

    std::vector<std::vector<cl_event> > evt_part_o_h2d_dep(table_o_sec_num);
    evt_part_o_h2d_dep[0].resize(1);
    for (int i = 1; i < table_o_sec_num; ++i) {
        if (i == 1) {
            evt_part_o_h2d_dep[i].resize(1);
        } else {
            evt_part_o_h2d_dep[i].resize(2);
        }
    }
    std::vector<std::vector<cl_event> > evt_part_o_krn_dep(table_o_sec_num);
    evt_part_o_krn_dep[0].resize(1);
    for (int i = 1; i < table_o_sec_num; ++i) {
        if (i == 1)
            evt_part_o_krn_dep[i].resize(2);
        else
            evt_part_o_krn_dep[i].resize(3);
    }
    std::vector<std::vector<cl_event> > evt_part_o_d2h_dep(table_o_sec_num);
    evt_part_o_d2h_dep[0].resize(1);
    for (int i = 1; i < table_o_sec_num; ++i) {
        if (i == 1)
            evt_part_o_d2h_dep[i].resize(1);
        else
            evt_part_o_d2h_dep[i].resize(2);
    }

    // define parto memcpy in user events
    std::vector<std::vector<cl_event> > evt_part_o_memcpy_in(table_o_sec_num);
    for (int i = 0; i < table_o_sec_num; i++) {
        evt_part_o_memcpy_in[i].resize(1);
        evt_part_o_memcpy_in[i][0] = clCreateUserEvent(ctx, &err);
    }
    std::vector<std::vector<cl_event> > evt_part_o_memcpy_out(table_o_sec_num);
    for (int i = 0; i < table_o_sec_num; i++) {
        evt_part_o_memcpy_out[i].resize(1);
        evt_part_o_memcpy_out[i][0] = clCreateUserEvent(ctx, &err);
    }

    queue_struct_join parto_min[table_o_sec_num];
    queue_struct_join parto_mout[table_o_sec_num];

    tv_opart.add(); // 0
    for (int sec = 0; sec < table_o_sec_num; sec++) {
        int kid = sec % 2;

        // 1) memcpy in
        parto_min[sec].sec = sec;
        parto_min[sec].event = &evt_part_o_memcpy_in[sec][0];
        parto_min[sec].meta_nrow = table_o_sec_depth[sec];
        parto_min[sec].meta = &meta_o_partition_in[kid];
        parto_min[sec].valid_col_num = o_valid_col_num;
        for (int i = 0; i < o_valid_col_num; i++) {
            parto_min[sec].size[i] = table_o_sec_size[i][sec];
            parto_min[sec].type_size[i] = table_o_col_types[i];

            parto_min[sec].ptr_src[i] = table_o_user_col_sec[i][sec];
            parto_min[sec].ptr_dst[i] = table_o_partition_in_col[i][kid];
        }
        if (sec > 1) {
            parto_min[sec].num_event_wait_list = evt_part_o_h2d[sec - 2].size();
            parto_min[sec].event_wait_list = evt_part_o_h2d[sec - 2].data();
        } else {
            parto_min[sec].num_event_wait_list = 0;
            parto_min[sec].event_wait_list = nullptr;
        }
        pool.q0.push(parto_min[sec]);

        // 2) h2d
        evt_part_o_h2d_dep[sec][0] = evt_part_o_memcpy_in[sec][0];

        if (sec > 1) {
            evt_part_o_h2d_dep[sec][1] = evt_part_o_krn[sec - 2][0];
        }
        clEnqueueMigrateMemObjects(cq, partition_o_in_vec[kid].size(), partition_o_in_vec[kid].data(), 0,
                                   evt_part_o_h2d_dep[sec].size(), evt_part_o_h2d_dep[sec].data(),
                                   &evt_part_o_h2d[sec][0]);
        // 3) kernel
        evt_part_o_krn_dep[sec][0] = evt_part_o_h2d[sec][0];
        if (sec > 0) {
            evt_part_o_krn_dep[sec][1] = evt_part_o_krn[sec - 1][0];
        }
        if (sec > 1) {
            evt_part_o_krn_dep[sec][2] = evt_part_o_d2h[sec - 2][0];
        }
        clEnqueueTask(cq, partkernel_O[kid], evt_part_o_krn_dep[sec].size(), evt_part_o_krn_dep[sec].data(),
                      &evt_part_o_krn[sec][0]);

        // 4) d2h, transfer partiton results back
        evt_part_o_d2h_dep[sec][0] = evt_part_o_krn[sec][0];
        if (sec > 1) {
            evt_part_o_d2h_dep[sec][1] = evt_part_o_memcpy_out[sec - 2][0];
        }
        clEnqueueMigrateMemObjects(cq, partition_o_out_vec[kid].size(), partition_o_out_vec[kid].data(), 1,
                                   evt_part_o_d2h_dep[sec].size(), evt_part_o_d2h_dep[sec].data(),
                                   &evt_part_o_d2h[sec][0]);

        // 5) memcpy out
        parto_mout[sec].sec = sec;
        parto_mout[sec].partition_num = partition_num;
        parto_mout[sec].part_max_nrow_512 = o_partition_out_col_part_depth;
        parto_mout[sec].event = &evt_part_o_memcpy_out[sec][0];
        parto_mout[sec].meta = &meta_o_partition_out[kid];
        parto_mout[sec].valid_col_num = o_valid_col_num;
        for (int i = 0; i < o_valid_col_num; i++) {
            parto_mout[sec].ptr_src[i] = table_o_partition_out_col[i][kid];
            parto_mout[sec].type_size[i] = table_o_col_types[i];
        }
        parto_mout[sec].part_ptr_dst = table_o_new_part_col;
        parto_mout[sec].num_event_wait_list = evt_part_o_d2h[sec].size();
        parto_mout[sec].event_wait_list = evt_part_o_d2h[sec].data();
        pool.q1.push(parto_mout[sec]);
    }
    clWaitForEvents(evt_part_o_memcpy_out[table_o_sec_num - 1].size(),
                    evt_part_o_memcpy_out[table_o_sec_num - 1].data());
    if (table_o_sec_num > 1) {
        clWaitForEvents(evt_part_o_memcpy_out[table_o_sec_num - 2].size(),
                        evt_part_o_memcpy_out[table_o_sec_num - 2].data());
    }
    tv_opart.add(); // 1
    pool.q0_run = 0;
    pool.q1_run = 0;
    pool.part_o_in_t.join();
    pool.part_o_out_t.join();

    // print the execution times for O table partition

    tvtime_opart = tv_opart.getMilliSec();
    double o_input_memcpy_size = 0; //(double)table_o_sec_size[p] * table_o_sec_num * o_valid_col_num / 1024 / 1024;
    for (int sec = 0; sec < table_o_sec_num; sec++) {
        for (int i = 0; i < o_valid_col_num; i++) {
            o_input_memcpy_size += (double)table_o_sec_size[i][sec];
        }
    }
    o_input_memcpy_size = o_input_memcpy_size / 1024 / 1024;

#ifdef USER_DEBUG
    std::cout << "----------- finished O table partition---------------" << std::endl << std::endl;
#endif

    pool.partl_init();
    // Assuming table L can not be put to FPGA DDR, divided into many sections
    int table_l_sec_num = sec_l;
    int* table_l_sec_depth;
    if (table_l_sec_num == 0) {
        table_l_sec_num = tab_b.getSecNum();
        table_l_sec_depth = new int[table_l_sec_num];
        for (int sec = 0; sec < table_l_sec_num; sec++) {
            table_l_sec_depth[sec] = tab_b.getSecRowNum(sec);
        }
    } else {
        table_l_sec_depth = new int[table_l_sec_num];
        int table_l_col_depth = l_nrow;
        int table_l_sec_depth_each = (table_l_col_depth + table_l_sec_num - 1) / table_l_sec_num;
        for (int sec = 0; sec < table_l_sec_num - 1; sec++) {
            table_l_sec_depth[sec] = table_l_sec_depth_each;
        }
        table_l_sec_depth[table_l_sec_num - 1] = table_l_col_depth - table_l_sec_depth_each * (table_l_sec_num - 1);
    }
// check each section number
// check each section number
// //////////////////////////////////////////////////
#ifdef USER_DEBUG
    std::cout << "table_l_sec_num: " << table_l_sec_num << std::endl;
    for (int i = 0; i < table_l_sec_num; i++) {
        std::cout << "table_l_sec_depth: " << table_l_sec_depth[i] << std::endl;
    }
#endif
    // //////////////////////////////////////////////////
    for (int sec = 0; sec < table_l_sec_num; sec++) {
        if (table_l_sec_depth[sec] < table_l_sec_num) {
            std::cerr << "Error: Table L section size is smaller than section number!!!";
            std::cerr << "sec size of Table L: " << table_l_sec_depth[sec] << ", ";
            std::cerr << "sec number of Table L: " << table_l_sec_num << std::endl;
            exit(1);
        }
    }

    int table_l_sec_size[8][table_l_sec_num];
    int table_l_col_types[8];
    for (int j = 0; j < l_valid_col_num; j++) {
        table_l_col_types[j] = tab_b.getColTypeSize(q5s_part_scan[1][j]);

        for (int i = 0; i < table_l_sec_num; i++) {
            table_l_sec_size[j][i] = table_l_sec_depth[i] * table_l_col_types[j];
        }
    }
    // get max seciction buffer depth and byte size
    int table_l_sec_depth_max = 0;
    int table_l_sec_size_max[8];
    for (int i = 0; i < table_l_sec_num; i++) {
        if (table_l_sec_depth[i] > table_l_sec_depth_max) {
            table_l_sec_depth_max = table_l_sec_depth[i];
        }
    }
    for (int j = 0; j < l_valid_col_num; j++) {
        table_l_sec_size_max[j] = table_l_sec_depth_max * table_l_col_types[j];
    }

    // data load from disk. due to table size, data read into several sections
    char* table_l_user_col_sec[8][table_l_sec_num];
    for (int j = 0; j < l_valid_col_num; j++) {
        for (int i = 0; i < table_l_sec_num; ++i) {
            table_l_user_col_sec[j][i] = tab_b.getColPointer(q5s_part_scan[1][j], sec_l, i);
        }
    }
    for (int j = l_valid_col_num; j < 8; j++) {
        for (int i = 0; i < table_l_sec_num; ++i) {
            table_l_user_col_sec[j][i] = mm.aligned_alloc<char>(8);
        }
    }

    // L host side pinned buffers for partition kernel
    char* table_l_partition_in_col[8][2];
    for (int i = 0; i < 8; i++) {
        if (l_valid_col_num > i) {
            table_l_partition_in_col[i][0] = mm.aligned_alloc<char>(table_l_sec_size_max[i]);
            table_l_partition_in_col[i][1] = mm.aligned_alloc<char>(table_l_sec_size_max[i]);
        } else {
            table_l_partition_in_col[i][0] = mm.aligned_alloc<char>(8); // table_l_sec_depth);
            table_l_partition_in_col[i][1] = mm.aligned_alloc<char>(8); // table_l_sec_depth);
        }
    }
    for (int c = 0; c < l_valid_col_num; c++) {
        memset(table_l_partition_in_col[c][0], 0, table_l_sec_size_max[c]);
        memset(table_l_partition_in_col[c][1], 0, table_l_sec_size_max[c]);
    }

    // partition output col size,  setup the proper size by multiple 1.5
    int l_partition_out_col_depth_init = table_l_sec_depth_max * 1.3 / VEC_LEN;
    assert(l_partition_out_col_depth_init > 0 && "Table L output col size must > 0");
    // the depth of each partition in each col.
    int l_partition_out_col_part_depth = (l_partition_out_col_depth_init + partition_num - 1) / partition_num;
    pool.l_partition_out_col_part_nrow_max = l_partition_out_col_part_depth * 16;

    // update depth to make sure the buffer size is aligned by partititon_num *
    // l_partition_out_col_part_depth;
    int l_partition_out_col_depth = partition_num * l_partition_out_col_part_depth;
    int l_partition_out_col_size = l_partition_out_col_depth * size_apu_512;

    // partition output data
    char* table_l_partition_out_col[8][2];
    for (int i = 0; i < 8; i++) {
        for (int k = 0; k < 2; k++) {
            if (l_valid_col_num > i) {
                table_l_partition_out_col[i][k] = mm.aligned_alloc<char>(l_partition_out_col_depth * size_apu_512);
            } else {
                table_l_partition_out_col[i][k] = mm.aligned_alloc<char>(8); // l_partition_out_col_depth);
            }
        }
    }
    //--------------- metabuffer setup L -----------------
    MetaTable meta_l_partition_in[2];
    for (int k = 0; k < 2; k++) {
        meta_l_partition_in[k].setColNum(l_valid_col_num);
        for (int i = 0; i < l_valid_col_num; i++) {
            meta_l_partition_in[k].setCol(i, i, table_l_sec_depth[i]);
        }
        meta_l_partition_in[k].meta();
    }

    // ouput col0,1,2,3 buffers data, with order: 0 1 2 3.
    // setup partition kernel used meta output
    MetaTable meta_l_partition_out[2];
    for (int k = 0; k < 2; k++) {
        meta_l_partition_out[k].setColNum(l_valid_col_num);
        meta_l_partition_out[k].setPartition(partition_num, l_partition_out_col_part_depth);
        for (int i = 0; i < l_valid_col_num; i++) {
            meta_l_partition_out[k].setCol(i, i, l_partition_out_col_depth);
        }
        meta_l_partition_out[k].meta();
    }

    cl_mem_ext_ptr_t mext_table_l_partition_in_col[8][2];
    cl_mem_ext_ptr_t mext_meta_l_partition_in[2], mext_meta_l_partition_out[2];
    cl_mem_ext_ptr_t mext_table_l_partition_out_col[8][2];

    for (int i = 0; i < 8; ++i) {
        mext_table_l_partition_in_col[i][0] = {3 + i, table_l_partition_in_col[i][0], partkernel_L[0]};
        mext_table_l_partition_in_col[i][1] = {3 + i, table_l_partition_in_col[i][1], partkernel_L[1]};
    }

    mext_meta_l_partition_in[0] = {11, meta_l_partition_in[0].meta(), partkernel_L[0]};
    mext_meta_l_partition_in[1] = {11, meta_l_partition_in[1].meta(), partkernel_L[1]};

    mext_meta_l_partition_out[0] = {12, meta_l_partition_out[0].meta(), partkernel_L[0]};
    mext_meta_l_partition_out[1] = {12, meta_l_partition_out[1].meta(), partkernel_L[1]};

    for (int i = 0; i < 8; ++i) {
        mext_table_l_partition_out_col[i][0] = {13 + i, table_l_partition_out_col[i][0], partkernel_L[0]};
        mext_table_l_partition_out_col[i][1] = {13 + i, table_l_partition_out_col[i][1], partkernel_L[1]};
    }

    // dev buffers
    cl_mem buf_table_l_partition_in_col[8][2];

    for (int i = 0; i < 8; i++) {
        for (int k = 0; k < 2; k++) {
            if (l_valid_col_num > i) {
                buf_table_l_partition_in_col[i][k] =
                    clCreateBuffer(ctx, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY,
                                   table_l_sec_size_max[i], &mext_table_l_partition_in_col[i][k], &err);
            } else {
                buf_table_l_partition_in_col[i][k] =
                    clCreateBuffer(ctx, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY, 8,
                                   &mext_table_l_partition_in_col[i][k], &err);
            }
        }
    }

    cl_mem buf_table_l_partition_out_col[8][2];

    for (int i = 0; i < 8; i++) {
        for (int k = 0; k < 2; k++) {
            if (l_valid_col_num > i) {
                buf_table_l_partition_out_col[i][k] =
                    clCreateBuffer(ctx, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                                   l_partition_out_col_size, &mext_table_l_partition_out_col[i][k], &err);
            } else {
                buf_table_l_partition_out_col[i][k] =
                    clCreateBuffer(ctx, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE, 8,
                                   &mext_table_l_partition_out_col[i][k], &err);
            }
        }
    }
    cl_mem buf_meta_l_partition_in[2];
    buf_meta_l_partition_in[0] = clCreateBuffer(ctx, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY,
                                                (size_apu_512 * 8), &mext_meta_l_partition_in[0], &err);
    buf_meta_l_partition_in[1] = clCreateBuffer(ctx, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY,
                                                (size_apu_512 * 8), &mext_meta_l_partition_in[1], &err);

    cl_mem buf_meta_l_partition_out[2];
    buf_meta_l_partition_out[0] = clCreateBuffer(ctx, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                                                 (size_apu_512 * 24), &mext_meta_l_partition_out[0], &err);
    buf_meta_l_partition_out[1] = clCreateBuffer(ctx, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                                                 (size_apu_512 * 24), &mext_meta_l_partition_out[1], &err);
//-------------------------------end of partition L
// setup----------------------------------
//

//----------------------partition L run-----------------------------
#ifdef USER_DEBUG
    std::cout << "------------------- Partitioning L table -----------------" << std::endl;
#endif
    const int idx_l = 1;
    j = 0;
    for (int k = 0; k < 2; k++) {
        j = 0;
        clSetKernelArg(partkernel_L[k], j++, sizeof(int), &k_depth);
        clSetKernelArg(partkernel_L[k], j++, sizeof(int), &idx_l);
        clSetKernelArg(partkernel_L[k], j++, sizeof(int), &log_partition_num);
        clSetKernelArg(partkernel_L[k], j++, sizeof(cl_mem), &buf_table_l_partition_in_col[0][k]);
        clSetKernelArg(partkernel_L[k], j++, sizeof(cl_mem), &buf_table_l_partition_in_col[1][k]);
        clSetKernelArg(partkernel_L[k], j++, sizeof(cl_mem), &buf_table_l_partition_in_col[2][k]);
        clSetKernelArg(partkernel_L[k], j++, sizeof(cl_mem), &buf_table_l_partition_in_col[3][k]);
        clSetKernelArg(partkernel_L[k], j++, sizeof(cl_mem), &buf_table_l_partition_in_col[4][k]);
        clSetKernelArg(partkernel_L[k], j++, sizeof(cl_mem), &buf_table_l_partition_in_col[5][k]);
        clSetKernelArg(partkernel_L[k], j++, sizeof(cl_mem), &buf_table_l_partition_in_col[6][k]);
        clSetKernelArg(partkernel_L[k], j++, sizeof(cl_mem), &buf_table_l_partition_in_col[7][k]);
        clSetKernelArg(partkernel_L[k], j++, sizeof(cl_mem), &buf_meta_l_partition_in[k]);
        clSetKernelArg(partkernel_L[k], j++, sizeof(cl_mem), &buf_meta_l_partition_out[k]);
        clSetKernelArg(partkernel_L[k], j++, sizeof(cl_mem), &buf_table_l_partition_out_col[0][k]);
        clSetKernelArg(partkernel_L[k], j++, sizeof(cl_mem), &buf_table_l_partition_out_col[1][k]);
        clSetKernelArg(partkernel_L[k], j++, sizeof(cl_mem), &buf_table_l_partition_out_col[2][k]);
        clSetKernelArg(partkernel_L[k], j++, sizeof(cl_mem), &buf_table_l_partition_out_col[3][k]);
        clSetKernelArg(partkernel_L[k], j++, sizeof(cl_mem), &buf_table_l_partition_out_col[4][k]);
        clSetKernelArg(partkernel_L[k], j++, sizeof(cl_mem), &buf_table_l_partition_out_col[5][k]);
        clSetKernelArg(partkernel_L[k], j++, sizeof(cl_mem), &buf_table_l_partition_out_col[6][k]);
        clSetKernelArg(partkernel_L[k], j++, sizeof(cl_mem), &buf_table_l_partition_out_col[7][k]);
        clSetKernelArg(partkernel_L[k], j++, sizeof(cl_mem), &buf_cfg5s_part);
    }

    // partition h2d
    std::vector<cl_mem> partition_l_in_vec[2];
    for (int k = 0; k < 2; k++) {
        partition_l_in_vec[k].push_back(buf_table_l_partition_in_col[0][k]);
        partition_l_in_vec[k].push_back(buf_table_l_partition_in_col[1][k]);
        partition_l_in_vec[k].push_back(buf_table_l_partition_in_col[2][k]);
        partition_l_in_vec[k].push_back(buf_table_l_partition_in_col[3][k]);
        partition_l_in_vec[k].push_back(buf_table_l_partition_in_col[4][k]);
        partition_l_in_vec[k].push_back(buf_table_l_partition_in_col[5][k]);
        partition_l_in_vec[k].push_back(buf_table_l_partition_in_col[6][k]);
        partition_l_in_vec[k].push_back(buf_table_l_partition_in_col[7][k]);
        partition_l_in_vec[k].push_back(buf_meta_l_partition_in[k]);
        partition_l_in_vec[k].push_back(buf_cfg5s_part);
    }

    // partition d2h
    std::vector<cl_mem> partition_l_out_vec[2];
    for (int k = 0; k < 2; k++) {
        partition_l_out_vec[k].push_back(buf_table_l_partition_out_col[0][k]);
        partition_l_out_vec[k].push_back(buf_table_l_partition_out_col[1][k]);
        partition_l_out_vec[k].push_back(buf_table_l_partition_out_col[2][k]);
        partition_l_out_vec[k].push_back(buf_table_l_partition_out_col[3][k]);
        partition_l_out_vec[k].push_back(buf_table_l_partition_out_col[4][k]);
        partition_l_out_vec[k].push_back(buf_table_l_partition_out_col[5][k]);
        partition_l_out_vec[k].push_back(buf_table_l_partition_out_col[6][k]);
        partition_l_out_vec[k].push_back(buf_table_l_partition_out_col[7][k]);
        partition_l_out_vec[k].push_back(buf_meta_l_partition_out[k]);
    }
    clEnqueueMigrateMemObjects(cq, 1, &buf_meta_l_partition_out[0], 0, 0, nullptr, nullptr);
    clEnqueueMigrateMemObjects(cq, 1, &buf_meta_l_partition_out[1], 0, 0, nullptr, nullptr);

    clEnqueueMigrateMemObjects(cq, partition_l_in_vec[0].size(), partition_l_in_vec[0].data(),
                               CL_MIGRATE_MEM_OBJECT_CONTENT_UNDEFINED, 0, nullptr, nullptr);
    clEnqueueMigrateMemObjects(cq, partition_l_in_vec[1].size(), partition_l_in_vec[1].data(),
                               CL_MIGRATE_MEM_OBJECT_CONTENT_UNDEFINED, 0, nullptr, nullptr);
    clEnqueueMigrateMemObjects(cq, partition_l_out_vec[0].size(), partition_l_out_vec[0].data(),
                               CL_MIGRATE_MEM_OBJECT_CONTENT_UNDEFINED, 0, nullptr, nullptr);
    clEnqueueMigrateMemObjects(cq, partition_l_out_vec[1].size(), partition_l_out_vec[1].data(),
                               CL_MIGRATE_MEM_OBJECT_CONTENT_UNDEFINED, 0, nullptr, nullptr);

    // create user partition res cols
    // all sections partition 0 output to same 8-col buffers
    char*** table_l_new_part_col = new char**[partition_num];

    // combine sec0_partition0, sec1_parttion0, ...secN_partition0 in 1 buffer.
    // the depth is
    int l_new_part_depth = l_partition_out_col_part_depth * table_l_sec_num;

    for (int p = 0; p < partition_num; ++p) {
        table_l_new_part_col[p] = new char*[8];
        for (int i = 0; i < 8; ++i) {
            if (i < l_valid_col_num) {
                table_l_new_part_col[p][i] = mm.aligned_alloc<char>(l_new_part_depth * size_apu_512);
                memset(table_l_new_part_col[p][i], 0, l_new_part_depth * size_apu_512);
            } else {
                table_l_new_part_col[p][i] = mm.aligned_alloc<char>(size_apu_512);
            }
        }
    }
    // record the new_part table offset to write partition i of all sections

    std::vector<std::vector<cl_event> > evt_part_l_h2d(table_l_sec_num);
    std::vector<std::vector<cl_event> > evt_part_l_krn(table_l_sec_num);
    std::vector<std::vector<cl_event> > evt_part_l_d2h(table_l_sec_num);

    for (int sec = 0; sec < table_l_sec_num; sec++) {
        evt_part_l_h2d[sec].resize(1);
        evt_part_l_krn[sec].resize(1);
        evt_part_l_d2h[sec].resize(1);
    }

    std::vector<std::vector<cl_event> > evt_part_l_h2d_dep(table_l_sec_num);
    evt_part_l_h2d_dep[0].resize(1);
    for (int i = 1; i < table_l_sec_num; ++i) {
        if (i == 1)
            evt_part_l_h2d_dep[i].resize(1);
        else
            evt_part_l_h2d_dep[i].resize(2);
    }
    std::vector<std::vector<cl_event> > evt_part_l_krn_dep(table_l_sec_num);
    evt_part_l_krn_dep[0].resize(1);
    for (int i = 1; i < table_l_sec_num; ++i) {
        if (i == 1)
            evt_part_l_krn_dep[i].resize(2);
        else
            evt_part_l_krn_dep[i].resize(3);
    }
    std::vector<std::vector<cl_event> > evt_part_l_d2h_dep(table_l_sec_num);
    evt_part_l_d2h_dep[0].resize(1);
    for (int i = 1; i < table_l_sec_num; ++i) {
        if (i == 1)
            evt_part_l_d2h_dep[i].resize(1);
        else
            evt_part_l_d2h_dep[i].resize(2);
    }

    // define partl memcpy in user events
    std::vector<std::vector<cl_event> > evt_part_l_memcpy_in(table_l_sec_num);
    for (int i = 0; i < table_l_sec_num; i++) {
        evt_part_l_memcpy_in[i].resize(1);
        evt_part_l_memcpy_in[i][0] = clCreateUserEvent(ctx, &err);
    }
    std::vector<std::vector<cl_event> > evt_part_l_memcpy_out(table_l_sec_num);
    for (int i = 0; i < table_l_sec_num; i++) {
        evt_part_l_memcpy_out[i].resize(1);
        evt_part_l_memcpy_out[i][0] = clCreateUserEvent(ctx, &err);
    }

    queue_struct_join partl_min[table_l_sec_num];
    queue_struct_join partl_mout[table_l_sec_num];

    gqe::utils::Timer tv_lpart;
    tv_lpart.add(); // 0
    for (int sec = 0; sec < table_l_sec_num; sec++) {
        int kid = sec % 2;
        // 1) memcpy in
        partl_min[sec].sec = sec;
        partl_min[sec].event = &evt_part_l_memcpy_in[sec][0];
        partl_min[sec].meta_nrow = table_l_sec_depth[sec];
        partl_min[sec].meta = &meta_l_partition_in[kid];
        partl_min[sec].valid_col_num = l_valid_col_num;
        for (int i = 0; i < l_valid_col_num; i++) {
            partl_min[sec].size[i] = table_l_sec_size[i][sec];
            partl_min[sec].type_size[i] = table_l_col_types[i];
            partl_min[sec].ptr_src[i] = table_l_user_col_sec[i][sec];
            partl_min[sec].ptr_dst[i] = table_l_partition_in_col[i][kid];
        }
        if (sec > 1) {
            partl_min[sec].num_event_wait_list = evt_part_l_h2d[sec - 2].size();
            partl_min[sec].event_wait_list = evt_part_l_h2d[sec - 2].data();
        } else {
            partl_min[sec].num_event_wait_list = 0;
            partl_min[sec].event_wait_list = nullptr;
        }
        if (kid == 0) pool.q2_ping.push(partl_min[sec]);
        if (kid == 1) pool.q2_pong.push(partl_min[sec]);

        // 2) h2d
        evt_part_l_h2d_dep[sec][0] = evt_part_l_memcpy_in[sec][0];
        if (sec > 1) {
            evt_part_l_h2d_dep[sec][1] = evt_part_l_krn[sec - 2][0];
        }
        clEnqueueMigrateMemObjects(cq, partition_l_in_vec[kid].size(), partition_l_in_vec[kid].data(), 0,
                                   evt_part_l_h2d_dep[sec].size(), evt_part_l_h2d_dep[sec].data(),
                                   &evt_part_l_h2d[sec][0]);

        // 3) kernel
        evt_part_l_krn_dep[sec][0] = evt_part_l_h2d[sec][0];
        if (sec > 0) {
            evt_part_l_krn_dep[sec][1] = evt_part_l_krn[sec - 1][0];
        }
        if (sec > 1) {
            evt_part_l_krn_dep[sec][2] = evt_part_l_d2h[sec - 2][0];
        }
        clEnqueueTask(cq, partkernel_L[kid], evt_part_l_krn_dep[sec].size(), evt_part_l_krn_dep[sec].data(),
                      &evt_part_l_krn[sec][0]);

        // 4) d2h, transfer partiton results back
        evt_part_l_d2h_dep[sec][0] = evt_part_l_krn[sec][0];
        if (sec > 1) {
            evt_part_l_d2h_dep[sec][1] = evt_part_l_memcpy_out[sec - 2][0];
        }
        clEnqueueMigrateMemObjects(cq, partition_l_out_vec[kid].size(), partition_l_out_vec[kid].data(), 1,
                                   evt_part_l_d2h_dep[sec].size(), evt_part_l_d2h_dep[sec].data(),
                                   &evt_part_l_d2h[sec][0]);
        // 5) memcpy out
        partl_mout[sec].sec = sec;
        partl_mout[sec].partition_num = partition_num;
        partl_mout[sec].part_max_nrow_512 = l_partition_out_col_part_depth;
        partl_mout[sec].event = &evt_part_l_memcpy_out[sec][0];
        partl_mout[sec].meta = &meta_l_partition_out[kid];
        partl_mout[sec].valid_col_num = l_valid_col_num;
        for (int i = 0; i < l_valid_col_num; i++) {
            partl_mout[sec].ptr_src[i] = table_l_partition_out_col[i][kid];
            partl_mout[sec].type_size[i] = table_l_col_types[i];
        }
        partl_mout[sec].part_ptr_dst = table_l_new_part_col;
        partl_mout[sec].num_event_wait_list = evt_part_l_d2h[sec].size();
        partl_mout[sec].event_wait_list = evt_part_l_d2h[sec].data();
        if (kid == 0) pool.q3_ping.push(partl_mout[sec]);
        if (kid == 1) pool.q3_pong.push(partl_mout[sec]);
    }
    clWaitForEvents(evt_part_l_memcpy_out[table_l_sec_num - 1].size(),
                    evt_part_l_memcpy_out[table_l_sec_num - 1].data());
    if (table_l_sec_num > 1) {
        clWaitForEvents(evt_part_l_memcpy_out[table_l_sec_num - 2].size(),
                        evt_part_l_memcpy_out[table_l_sec_num - 2].data());
    }
    tv_lpart.add(); // 1
    pool.q2_ping_run = 0;
    pool.q2_pong_run = 0;

    pool.q3_ping_run = 0;
    pool.q3_pong_run = 0;
    pool.part_l_in_ping_t.join();
    pool.part_l_in_pong_t.join();
    pool.part_l_out_ping_t.join();
    pool.part_l_out_pong_t.join();

    double tvtime_lpart = tv_lpart.getMilliSec();
    double l_input_memcpy_size = 0; //(double)table_l_sec_size * table_l_sec_num * l_valid_col_num / 1024 / 1024;
    for (int i = 0; i < l_valid_col_num; i++) {
        for (int j = 0; j < table_l_sec_num; j++) {
            l_input_memcpy_size += table_l_sec_size[i][j];
        }
    }
    l_input_memcpy_size = l_input_memcpy_size / 1024 / 1024;

#ifdef USER_DEBUG
    std::cout << "----------- finished L table partition---------------" << std::endl << std::endl;
#endif
    //
    //--------------------------------setup hash
    // join-------------------------------------
    //--------------------------build o_part_0-and probe with
    // l_part_0--------------------
    //
    //
    pool.hj_init();
    // build-probe need to be run for each partition pair. TO use the same
    // host/device buffer, the max data size
    // amone partitions must be obtained. Then buffer allocations are using the
    // max data size
    int table_o_build_in_nrow_max = 0;
    for (int p = 0; p < partition_num; p++) {
        table_o_build_in_nrow_max = std::max(table_o_build_in_nrow_max, pool.o_new_part_offset[p]);
    }
    int table_o_build_in_depth_max = (table_o_build_in_nrow_max + 15) / 16;
    int table_o_build_in_size_max = table_o_build_in_depth_max * size_apu_512;

    int table_l_probe_in_nrow_max = 0;
    for (int p = 0; p < partition_num; p++) {
        // table_l_probe_in_nrow_max = std::max(table_l_probe_in_nrow_max,
        // pool.l_new_part_offset[p]);
        int tmp = pool.l_new_part_offset[p];
        table_l_probe_in_nrow_max = std::max(table_l_probe_in_nrow_max, tmp);
    }
    int table_l_probe_in_depth_max = (table_l_probe_in_nrow_max + 15) / 16;
    // slice probe in size
    int table_l_probe_in_slice_depth = (table_l_probe_in_depth_max + slice_num - 1) / slice_num;
    int table_l_probe_in_slice_size = table_l_probe_in_slice_depth * size_apu_512;
    int l_result_nrow = tab_c.getRowNum() / partition_num;
    if (probe_buf_size_auto) {
        bool if_filter_l = jcmd.getIfFilterL();
        if (if_filter_l) {
            l_result_nrow = table_l_probe_in_nrow_max / 4;
        } else {
            l_result_nrow = table_l_probe_in_nrow_max;
        }
    }

    int table_l_probe_out_nrow = l_result_nrow;
    int table_l_probe_out_depth = (table_l_probe_out_nrow + 15) / 16;
    // int table_l_probe_out_size = table_l_probe_out_depth *
    // size_apu_512;
    // slice probe out size
    int table_l_probe_out_slice_nrow = table_l_probe_out_nrow / slice_num;
    int table_l_probe_out_slice_depth = (table_l_probe_out_depth + slice_num - 1) / slice_num;
    int table_l_probe_out_slice_size = table_l_probe_out_slice_depth * size_apu_512;

    // define build kernel pinned host buffers, input
    o_valid_col_num = q5s_join_scan[0].size();
    l_valid_col_num = q5s_join_scan[1].size();

#ifdef USER_DEBUG
    std::cout << "In join o_valid_col_num:" << o_valid_col_num << std::endl;
    std::cout << "In join l_valid_col_num:" << l_valid_col_num << std::endl;
#endif

    char* table_o_build_in_col[8];
    for (int i = 0; i < 8; i++) {
        if (i < o_valid_col_num) {
            table_o_build_in_col[i] = mm.aligned_alloc<char>(table_o_build_in_size_max);
        } else {
            table_o_build_in_col[i] = mm.aligned_alloc<char>(8);
        }
    }

    // define probe kernel pinned host buffers, input
    char* table_l_probe_in_col[8][2];
    for (int k = 0; k < 2; k++) {
        for (int i = 0; i < l_valid_col_num; i++) {
            table_l_probe_in_col[i][k] = mm.aligned_alloc<char>(table_l_probe_in_slice_size);
        }
        for (int i = l_valid_col_num; i < 8; i++) {
            table_l_probe_in_col[i][k] = mm.aligned_alloc<char>(8);
        }
    }

    // define probe kernel pinned host buffers, output
    char* table_l_probe_out_col[8][2];
    for (int k = 0; k < 2; k++) {
        for (int i = 0; i < out_valid_col_num; i++) {
            table_l_probe_out_col[i][k] = mm.aligned_alloc<char>(table_l_probe_out_slice_size);
        }
        for (int i = out_valid_col_num; i < 8; i++) {
            table_l_probe_out_col[i][k] = mm.aligned_alloc<char>(8);
        }
    }
    for (int k = 0; k < 2; k++) {
        for (int i = 0; i < out_valid_col_num; i++) {
            memset(table_l_probe_out_col[i][k], 0, table_l_probe_out_slice_size);
        }
    }

    // define the final output and memset 0
    char* table_out_col[8];
    size_t table_out_col_type[8];
    // for (int p = 0; p < partition_num; p++) {
    for (int i = 0; i < 8; i++) {
        if (i < out_valid_col_num) {
            table_out_col[i] = tab_c.getColPointer(i);
            table_out_col_type[i] = tab_c.getColTypeSize(i);
        } else {
            table_out_col[i] = mm.aligned_alloc<char>(size_apu_512);
        }
    }
    //}

    //--------------- metabuffer setup -----------------
    // using col0 and col1 buffer during build
    // setup build used meta input
    // set to max here, can be updated in the iteration hash build-probe
    MetaTable meta_build_in;
    meta_build_in.setColNum(o_valid_col_num);
    for (int i = 0; i < o_valid_col_num; i++) {
        meta_build_in.setCol(i, i, table_o_build_in_nrow_max);
    }

    // setup probe used meta input
    MetaTable meta_probe_in[2];
    for (int k = 0; k < 2; k++) {
        meta_probe_in[k].setColNum(l_valid_col_num);
        for (int i = 0; i < l_valid_col_num; i++) {
            meta_probe_in[k].setCol(i, i, table_l_probe_in_nrow_max);
        }
    }
    //
    // ouput col0,1,2,3 buffers data, with order: 0 1 2 3. (When aggr is off)
    // when aggr is on, actually only using col0 is enough.
    // below example only illustrates the output buffers can be shuffled.
    // setup probe used meta output
    MetaTable meta_probe_out[2];
    for (int k = 0; k < 2; k++) {
        meta_probe_out[k].setColNum(out_valid_col_num);
        for (int i = 0; i < out_valid_col_num; i++) {
            meta_probe_out[k].setCol(i, i, table_l_probe_out_slice_nrow);
        }
    }

    //--------------------------------------------

    // build kernel
    cl_kernel bkernel;
    bkernel = clCreateKernel(prg, "gqeJoin", &err);
    // probe kernel
    cl_kernel pkernel[2];
    pkernel[0] = clCreateKernel(prg, "gqeJoin", &err);
    pkernel[1] = clCreateKernel(prg, "gqeJoin", &err);

    size_t build_probe_flag_0 = 0;
    size_t build_probe_flag_1 = 1;

    cl_mem_ext_ptr_t mext_table_o_build_in_col[8], mext_cfg5s_hj;
    cl_mem_ext_ptr_t mext_table_l_probe_in_col[8][2], mext_table_l_probe_out_col[8][2];
    cl_mem_ext_ptr_t mext_meta_build_in, mext_meta_probe_in[2], mext_meta_probe_out[2];
    cl_mem_ext_ptr_t mext_hj_tmp[PU_NM * 2];

    for (int i = 0; i < 8; i++) {
        mext_table_o_build_in_col[i] = {i, table_o_build_in_col[i], bkernel};
    }

    for (int k = 0; k < 2; k++) {
        for (int i = 0; i < 8; i++) {
            mext_table_l_probe_in_col[i][k] = {i, table_l_probe_in_col[i][k], pkernel[k]};
        }
    }
    mext_meta_build_in = {9, meta_build_in.meta(), bkernel};
    mext_meta_probe_in[0] = {9, meta_probe_in[0].meta(), pkernel[0]};
    mext_meta_probe_in[1] = {9, meta_probe_in[1].meta(), pkernel[1]};
    mext_meta_probe_out[0] = {10, meta_probe_out[0].meta(), pkernel[0]};
    mext_meta_probe_out[1] = {10, meta_probe_out[1].meta(), pkernel[1]};

    for (int k = 0; k < 2; k++) {
        for (int i = 0; i < 8; i++) {
            mext_table_l_probe_out_col[i][k] = {11 + i, table_l_probe_out_col[i][k], pkernel[k]};
        }
    }
    mext_cfg5s_hj = {19, q5s_cfg_join, bkernel};

    for (int i = 0; i < PU_NM * 2; ++i) {
        mext_hj_tmp[i] = {20 + i, nullptr, bkernel};
    }

    cl_mem buf_table_o_build_in_col[8];
    for (int i = 0; i < 8; i++) {
        if (i < o_valid_col_num) {
            buf_table_o_build_in_col[i] =
                clCreateBuffer(ctx, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY,
                               table_o_build_in_size_max, &mext_table_o_build_in_col[i], &err);
        } else {
            buf_table_o_build_in_col[i] =
                clCreateBuffer(ctx, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY, 8,
                               &mext_table_o_build_in_col[i], &err);
        }
    }
    cl_mem buf_table_l_probe_in_col[8][2];
    for (int k = 0; k < 2; k++) {
        for (int i = 0; i < 8; i++) {
            if (i < l_valid_col_num) {
                buf_table_l_probe_in_col[i][k] =
                    clCreateBuffer(ctx, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY,
                                   table_l_probe_in_slice_size, &mext_table_l_probe_in_col[i][k], &err);
            } else {
                buf_table_l_probe_in_col[i][k] =
                    clCreateBuffer(ctx, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY, 8,
                                   &mext_table_l_probe_in_col[i][k], &err);
            }
        }
    }
    cl_mem buf_table_l_probe_out_col[8][2];
    for (int k = 0; k < 2; k++) {
        for (int i = 0; i < 8; i++) {
            if (i < out_valid_col_num) {
                buf_table_l_probe_out_col[i][k] =
                    clCreateBuffer(ctx, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                                   table_l_probe_out_slice_size, &mext_table_l_probe_out_col[i][k], &err);
            } else {
                buf_table_l_probe_out_col[i][k] =
                    clCreateBuffer(ctx, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE, 8,
                                   &mext_table_l_probe_out_col[i][k], &err);
            }
        }
    }

    cl_mem buf_cfg5s_hj = clCreateBuffer(ctx, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY,
                                         (size_apu_512 * 9), &mext_cfg5s_hj, &err);

    // inter htb stb buffers
    cl_mem buf_hj_tmp[PU_NM * 2];
    for (int i = 0; i < PU_NM; i++) {
        buf_hj_tmp[i] = clCreateBuffer(ctx, CL_MEM_READ_WRITE | CL_MEM_HOST_NO_ACCESS | CL_MEM_EXT_PTR_XILINX,
                                       (size_t)(sizeof(int64_t) * HT_BUFF_DEPTH / 2), &mext_hj_tmp[i], &err);
    }
    for (int i = PU_NM; i < PU_NM * 2; i++) {
        buf_hj_tmp[i] = clCreateBuffer(ctx, CL_MEM_READ_WRITE | CL_MEM_HOST_NO_ACCESS | CL_MEM_EXT_PTR_XILINX,
                                       (size_t)(KEY_SZ * 2 * S_BUFF_DEPTH / 2), &mext_hj_tmp[i], &err);
    }

    cl_mem buf_meta_build_in = clCreateBuffer(ctx, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY,
                                              (size_apu_512 * 8), &mext_meta_build_in, &err);

    cl_mem buf_meta_probe_in[2];
    buf_meta_probe_in[0] = clCreateBuffer(ctx, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY,
                                          (size_apu_512 * 8), &mext_meta_probe_in[0], &err);
    buf_meta_probe_in[1] = clCreateBuffer(ctx, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY,
                                          (size_apu_512 * 8), &mext_meta_probe_in[1], &err);
    cl_mem buf_meta_probe_out[2];
    buf_meta_probe_out[0] = clCreateBuffer(ctx, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                                           (size_apu_512 * 8), &mext_meta_probe_out[0], &err);
    buf_meta_probe_out[1] = clCreateBuffer(ctx, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                                           (size_apu_512 * 8), &mext_meta_probe_out[1], &err);
//-----------------------end of hash join
// setup---------------------------------

#ifdef USER_DEBUG
    std::cout << "------------------- HASH JOIN for each partition-----------------" << std::endl;
#endif
    // build kernel h2d
    std::vector<cl_mem> build_in_vec;
    for (int i = 0; i < o_valid_col_num; i++) {
        build_in_vec.push_back(buf_table_o_build_in_col[i]);
    }
    build_in_vec.push_back(buf_cfg5s_hj);
    build_in_vec.push_back(buf_meta_build_in);

    // probe kernel h2d
    std::vector<cl_mem> probe_in_vec[2];
    for (int k = 0; k < 2; k++) {
        for (int i = 0; i < l_valid_col_num; i++) {
            probe_in_vec[k].push_back(buf_table_l_probe_in_col[i][k]);
        }
        probe_in_vec[k].push_back(buf_meta_probe_in[k]);
    }

    // probe kernel d2h
    std::vector<cl_mem> probe_out_vec[2];
    for (int k = 0; k < 2; k++) {
        for (int i = 0; i < out_valid_col_num; i++) {
            probe_out_vec[k].push_back(buf_table_l_probe_out_col[i][k]);
        }
        probe_out_vec[k].push_back(buf_meta_probe_out[k]);
    }

    // make sure buffers resident on dev
    clEnqueueMigrateMemObjects(cq, probe_in_vec[0].size(), probe_in_vec[0].data(),
                               CL_MIGRATE_MEM_OBJECT_CONTENT_UNDEFINED, 0, nullptr, nullptr);
    clEnqueueMigrateMemObjects(cq, probe_in_vec[1].size(), probe_in_vec[1].data(),
                               CL_MIGRATE_MEM_OBJECT_CONTENT_UNDEFINED, 0, nullptr, nullptr);

    clEnqueueMigrateMemObjects(cq, probe_out_vec[0].size(), probe_out_vec[0].data(),
                               CL_MIGRATE_MEM_OBJECT_CONTENT_UNDEFINED, 0, nullptr, nullptr);
    clEnqueueMigrateMemObjects(cq, probe_out_vec[1].size(), probe_out_vec[1].data(),
                               CL_MIGRATE_MEM_OBJECT_CONTENT_UNDEFINED, 0, nullptr, nullptr);

    // set kernel args
    // bkernel
    j = 0;
    clSetKernelArg(bkernel, j++, sizeof(cl_mem), &buf_table_o_build_in_col[0]);
    clSetKernelArg(bkernel, j++, sizeof(cl_mem), &buf_table_o_build_in_col[1]);
    clSetKernelArg(bkernel, j++, sizeof(cl_mem), &buf_table_o_build_in_col[2]);
    clSetKernelArg(bkernel, j++, sizeof(cl_mem), &buf_table_o_build_in_col[3]);
    clSetKernelArg(bkernel, j++, sizeof(cl_mem), &buf_table_o_build_in_col[4]);
    clSetKernelArg(bkernel, j++, sizeof(cl_mem), &buf_table_o_build_in_col[5]);
    clSetKernelArg(bkernel, j++, sizeof(cl_mem), &buf_table_o_build_in_col[6]);
    clSetKernelArg(bkernel, j++, sizeof(cl_mem), &buf_table_o_build_in_col[7]);
    clSetKernelArg(bkernel, j++, sizeof(size_t), &build_probe_flag_0);
    clSetKernelArg(bkernel, j++, sizeof(cl_mem), &buf_meta_build_in);
    clSetKernelArg(bkernel, j++, sizeof(cl_mem), &buf_meta_probe_out[0]);
    clSetKernelArg(bkernel, j++, sizeof(cl_mem), &buf_table_l_probe_out_col[0][0]); // no output for build
    clSetKernelArg(bkernel, j++, sizeof(cl_mem), &buf_table_l_probe_out_col[0][0]);
    clSetKernelArg(bkernel, j++, sizeof(cl_mem), &buf_table_l_probe_out_col[0][0]);
    clSetKernelArg(bkernel, j++, sizeof(cl_mem), &buf_table_l_probe_out_col[0][0]);
    clSetKernelArg(bkernel, j++, sizeof(cl_mem), &buf_table_l_probe_out_col[0][0]);
    clSetKernelArg(bkernel, j++, sizeof(cl_mem), &buf_table_l_probe_out_col[0][0]);
    clSetKernelArg(bkernel, j++, sizeof(cl_mem), &buf_table_l_probe_out_col[0][0]);
    clSetKernelArg(bkernel, j++, sizeof(cl_mem), &buf_table_l_probe_out_col[0][0]);
    clSetKernelArg(bkernel, j++, sizeof(cl_mem), &buf_cfg5s_hj);
    for (int t = 0; t < PU_NM * 2; t++) {
        clSetKernelArg(bkernel, j++, sizeof(cl_mem), &buf_hj_tmp[t]);
    }

    // pkernel
    for (int k = 0; k < 2; k++) {
        j = 0;
        clSetKernelArg(pkernel[k], j++, sizeof(cl_mem), &buf_table_l_probe_in_col[0][k]);
        clSetKernelArg(pkernel[k], j++, sizeof(cl_mem), &buf_table_l_probe_in_col[1][k]);
        clSetKernelArg(pkernel[k], j++, sizeof(cl_mem), &buf_table_l_probe_in_col[2][k]);
        clSetKernelArg(pkernel[k], j++, sizeof(cl_mem), &buf_table_l_probe_in_col[3][k]);
        clSetKernelArg(pkernel[k], j++, sizeof(cl_mem), &buf_table_l_probe_in_col[4][k]);
        clSetKernelArg(pkernel[k], j++, sizeof(cl_mem), &buf_table_l_probe_in_col[5][k]);
        clSetKernelArg(pkernel[k], j++, sizeof(cl_mem), &buf_table_l_probe_in_col[6][k]);
        clSetKernelArg(pkernel[k], j++, sizeof(cl_mem), &buf_table_l_probe_in_col[7][k]);
        clSetKernelArg(pkernel[k], j++, sizeof(size_t), &build_probe_flag_1);
        clSetKernelArg(pkernel[k], j++, sizeof(cl_mem), &buf_meta_probe_in[k]);
        clSetKernelArg(pkernel[k], j++, sizeof(cl_mem), &buf_meta_probe_out[k]);
        clSetKernelArg(pkernel[k], j++, sizeof(cl_mem), &buf_table_l_probe_out_col[0][k]);
        clSetKernelArg(pkernel[k], j++, sizeof(cl_mem), &buf_table_l_probe_out_col[1][k]);
        clSetKernelArg(pkernel[k], j++, sizeof(cl_mem), &buf_table_l_probe_out_col[2][k]);
        clSetKernelArg(pkernel[k], j++, sizeof(cl_mem), &buf_table_l_probe_out_col[3][k]);
        clSetKernelArg(pkernel[k], j++, sizeof(cl_mem), &buf_table_l_probe_out_col[4][k]);
        clSetKernelArg(pkernel[k], j++, sizeof(cl_mem), &buf_table_l_probe_out_col[5][k]);
        clSetKernelArg(pkernel[k], j++, sizeof(cl_mem), &buf_table_l_probe_out_col[6][k]);
        clSetKernelArg(pkernel[k], j++, sizeof(cl_mem), &buf_table_l_probe_out_col[7][k]);
        clSetKernelArg(pkernel[k], j++, sizeof(cl_mem), &buf_cfg5s_hj);
        for (int t = 0; t < PU_NM * 2; t++) {
            clSetKernelArg(pkernel[k], j++, sizeof(cl_mem), &buf_hj_tmp[t]);
        }
    }
    // define cl_event used for build and probe
    std::vector<std::vector<cl_event> > evt_build_h2d(partition_num);
    std::vector<std::vector<cl_event> > evt_build_krn(partition_num);
    std::vector<std::vector<cl_event> > evt_probe_h2d(partition_num * slice_num);
    std::vector<std::vector<cl_event> > evt_probe_krn(partition_num * slice_num);
    std::vector<std::vector<cl_event> > evt_probe_d2h(partition_num * slice_num);

    for (int e_i = 0; e_i < partition_num * slice_num; e_i++) {
        evt_probe_h2d[e_i].resize(1);
        evt_probe_krn[e_i].resize(1);
        evt_probe_d2h[e_i].resize(1);
    }
    for (int p = 0; p < partition_num; p++) {
        evt_build_h2d[p].resize(1);
        evt_build_krn[p].resize(1);
    }

    std::vector<std::vector<cl_event> > evt_probe_krn_dep(partition_num * slice_num);
    evt_probe_krn_dep[0].resize(2);
    evt_probe_krn_dep[1].resize(3);
    for (int e_i = 2; e_i < partition_num * slice_num; e_i++) {
        evt_probe_krn_dep[e_i].resize(4);
    }

    // define user events used for memcpy functions
    std::vector<std::vector<cl_event> > evt_build_memcpy_in(partition_num);
    for (int p = 0; p < partition_num; p++) {
        evt_build_memcpy_in[p].resize(1);
        evt_build_memcpy_in[p][0] = clCreateUserEvent(ctx, &err);
    }

    // define dependence events for build and probe
    std::vector<std::vector<cl_event> > evt_build_h2d_dep(partition_num);
    evt_build_h2d_dep[0].resize(1);
    for (int p = 1; p < partition_num; p++) {
        evt_build_h2d_dep[p].resize(2);
    }

    std::vector<std::vector<cl_event> > evt_build_krn_dep(partition_num);
    evt_build_krn_dep[0].resize(1);
    evt_build_krn_dep[1].resize(3);
    for (int p = 2; p < partition_num; p++) {
        evt_build_krn_dep[p].resize(4);
    }

    std::vector<std::vector<cl_event> > evt_probe_memcpy_in(partition_num * slice_num);
    for (int e_i = 0; e_i < partition_num * slice_num; e_i++) {
        evt_probe_memcpy_in[e_i].resize(1);
        evt_probe_memcpy_in[e_i][0] = clCreateUserEvent(ctx, &err);
    }

    std::vector<std::vector<cl_event> > evt_probe_h2d_dep(partition_num * slice_num);
    evt_probe_h2d_dep[0].resize(1);
    evt_probe_h2d_dep[1].resize(1);
    for (int e_i = 2; e_i < partition_num * slice_num; e_i++) {
        evt_probe_h2d_dep[e_i].resize(2);
    }

    std::vector<std::vector<cl_event> > evt_probe_d2h_dep(partition_num * slice_num);
    evt_probe_d2h_dep[0].resize(1);
    evt_probe_d2h_dep[1].resize(1);
    for (int e_i = 2; e_i < partition_num * slice_num; e_i++) {
        evt_probe_d2h_dep[e_i].resize(2);
    }

    std::vector<std::vector<cl_event> > evt_probe_memcpy_out(partition_num * slice_num);
    for (int e_i = 0; e_i < partition_num * slice_num; e_i++) {
        evt_probe_memcpy_out[e_i].resize(1);
        evt_probe_memcpy_out[e_i][0] = clCreateUserEvent(ctx, &err);
    }

    gqe::utils::Timer tv_hj;

    // define callback function memcpy in/out used struct objects
    queue_struct_join build_min[partition_num];
    queue_struct_join probe_min[partition_num][slice_num];
    queue_struct_join probe_mout[partition_num][slice_num];

    tv_hj.add(); // 0
    // to fully pipeline the build-probe processes among different partitions,
    // counter e_i is used, which equals
    // partition_i * slice_i
    clEnqueueMigrateMemObjects(cq, 1, &buf_meta_probe_out[0], 0, 0, nullptr, nullptr);
    clEnqueueMigrateMemObjects(cq, 1, &buf_meta_probe_out[1], 0, 0, nullptr, nullptr);

    int e_i = 0;
    for (int p = 0; p < partition_num; p++) {
        int table_o_build_in_nrow = pool.o_new_part_offset[p]; // 0 is partition 0
        int table_o_build_in_depth = (table_o_build_in_nrow + 15) / 16;
        int table_o_build_in_size = table_o_build_in_depth * size_apu_512;

        int table_l_probe_in_nrow = pool.l_new_part_offset[p]; // 0 is partition 0
        int per_slice_nrow = (table_l_probe_in_nrow + slice_num - 1) / slice_num;

        assert(table_l_probe_in_nrow > slice_num);
        assert(per_slice_nrow > slice_num);

        int table_l_probe_in_slice_nrow[slice_num];

        for (int slice = 0; slice < slice_num; slice++) {
            table_l_probe_in_slice_nrow[slice] = per_slice_nrow;
            if (slice == slice_num - 1) {
                table_l_probe_in_slice_nrow[slice] = table_l_probe_in_nrow - slice * per_slice_nrow;
            }
        }

        //---------------build kernel run-------------------
        // 1) copy Order table from host DDR to build kernel pinned host buffer
        build_min[p].p = p;
        build_min[p].event = &evt_build_memcpy_in[p][0];
        build_min[p].meta_nrow = table_o_build_in_nrow;
        build_min[p].meta = &meta_build_in;
        build_min[p].valid_col_num = o_valid_col_num;
        for (int i = 0; i < o_valid_col_num; i++) {
            build_min[p].ptr_src[i] = table_o_new_part_col[p][q5s_join_scan[0][i]];
            build_min[p].ptr_dst[i] = table_o_build_in_col[i];
            build_min[p].size[i] = table_o_build_in_size;
        }
        if (p > 0) {
            build_min[p].num_event_wait_list = evt_build_h2d[p - 1].size();
            build_min[p].event_wait_list = evt_build_h2d[p - 1].data();
        } else {
            build_min[p].num_event_wait_list = 0;
            build_min[p].event_wait_list = nullptr;
        }
        pool.q4.push(build_min[p]);

        // 2) migrate order table data from host buffer to device buffer
        evt_build_h2d_dep[p][0] = evt_build_memcpy_in[p][0];
        if (p > 0) {
            evt_build_h2d_dep[p][1] = evt_build_krn[p - 1][0];
        }
        clEnqueueMigrateMemObjects(cq, build_in_vec.size(), build_in_vec.data(), 0, evt_build_h2d_dep[p].size(),
                                   evt_build_h2d_dep[p].data(), &evt_build_h2d[p][0]);

        // 3) launch build kernel
        evt_build_krn_dep[p][0] = evt_build_h2d[p][0];
        if (p > 0) {
            evt_build_krn_dep[p][1] = evt_build_krn[p - 1][0];
            evt_build_krn_dep[p][2] = evt_probe_krn[e_i - 1][0];
        }
        if (p > 1) {
            evt_build_krn_dep[p][3] = evt_probe_krn[e_i - 2][0];
        }
        clEnqueueTask(cq, bkernel, evt_build_krn_dep[p].size(), evt_build_krn_dep[p].data(), &evt_build_krn[p][0]);

        //------------------probe kernel run in pipeline------------------
        int table_l_probe_in_slice_nrow_sid;
        // int table_l_probe_in_slice_nrow_sid_size;

        // the idx of event
        for (int slice = 0; slice < slice_num; slice++) {
            int sid = e_i % 2;
            // the real nrow for each slice, only the last round is different to
            // per_slice_nrow
            table_l_probe_in_slice_nrow_sid = table_l_probe_in_slice_nrow[slice];
            // table_l_probe_in_slice_nrow_sid_size = table_l_probe_in_slice_nrow_sid * sizeof(int);

            // setup probe used meta input
            // 4) copy L table from host DDR to build kernel pinned host buffer
            probe_min[p][slice].per_slice_nrow = per_slice_nrow; // number in each slice
            probe_min[p][slice].p = p;
            probe_min[p][slice].slice = slice;
            probe_min[p][slice].event = &evt_probe_memcpy_in[e_i][0];
            probe_min[p][slice].meta_nrow = table_l_probe_in_slice_nrow_sid;
            probe_min[p][slice].meta = &meta_probe_in[sid];
            probe_min[p][slice].valid_col_num = l_valid_col_num;
            for (int i = 0; i < l_valid_col_num; i++) {
                probe_min[p][slice].ptr_src[i] = table_l_new_part_col[p][q5s_join_scan[1][i]];
                probe_min[p][slice].ptr_dst[i] = table_l_probe_in_col[i][sid];
                probe_min[p][slice].type_size[i] = table_l_col_types[q5s_join_scan[1][i]];
                probe_min[p][slice].size[i] = probe_min[p][slice].type_size[i] * table_l_probe_in_slice_nrow_sid;
            }
            if (e_i > 1) {
                probe_min[p][slice].num_event_wait_list = evt_probe_h2d[e_i - 2].size();
                probe_min[p][slice].event_wait_list = evt_probe_h2d[e_i - 2].data();
            } else {
                probe_min[p][slice].num_event_wait_list = 0;
                probe_min[p][slice].event_wait_list = nullptr;
            }
            if (sid == 0) pool.q5_ping.push(probe_min[p][slice]);
            if (sid == 1) pool.q5_pong.push(probe_min[p][slice]);

            // 5) migrate L table data from host buffer to device buffer
            evt_probe_h2d_dep[e_i][0] = evt_probe_memcpy_in[e_i][0];
            if (e_i > 1) {
                evt_probe_h2d_dep[e_i][1] = evt_probe_krn[e_i - 2][0];
            }
            clEnqueueMigrateMemObjects(cq, probe_in_vec[sid].size(), probe_in_vec[sid].data(), 0,
                                       evt_probe_h2d_dep[e_i].size(), evt_probe_h2d_dep[e_i].data(),
                                       &evt_probe_h2d[e_i][0]);
            // 6) launch probe kernel
            evt_probe_krn_dep[e_i][0] = evt_probe_h2d[e_i][0];
            evt_probe_krn_dep[e_i][1] = evt_build_krn[p][0];
            if (e_i > 0) {
                evt_probe_krn_dep[e_i][2] = evt_probe_krn[e_i - 1][0];
            }
            if (e_i > 1) {
                evt_probe_krn_dep[e_i][3] = evt_probe_d2h[e_i - 2][0];
            }
            clEnqueueTask(cq, pkernel[sid], evt_probe_krn_dep[e_i].size(), evt_probe_krn_dep[e_i].data(),
                          &evt_probe_krn[e_i][0]);

            // 7) migrate result data from device buffer to host buffer
            evt_probe_d2h_dep[e_i][0] = evt_probe_krn[e_i][0];
            if (e_i > 1) {
                evt_probe_d2h_dep[e_i][1] = evt_probe_memcpy_out[e_i - 2][0];
            }
            clEnqueueMigrateMemObjects(cq, probe_out_vec[sid].size(), probe_out_vec[sid].data(),
                                       CL_MIGRATE_MEM_OBJECT_HOST, evt_probe_d2h_dep[e_i].size(),
                                       evt_probe_d2h_dep[e_i].data(), &evt_probe_d2h[e_i][0]);

            // 8) memcpy the output data back to user host buffer
            probe_mout[p][slice].p = p;
            probe_mout[p][slice].slice = slice;
            probe_mout[p][slice].event = &evt_probe_memcpy_out[e_i][0];
            probe_mout[p][slice].meta = &meta_probe_out[sid];
            probe_mout[p][slice].valid_col_num = out_valid_col_num;
            probe_mout[p][slice].part_max_nrow_512 = table_l_probe_out_slice_depth;
            for (int i = 0; i < out_valid_col_num; i++) {
                probe_mout[p][slice].ptr_dst[i] = table_out_col[i];
                probe_mout[p][slice].ptr_src[i] = table_l_probe_out_col[i][sid];
                probe_mout[p][slice].type_size[i] = table_out_col_type[i];
                probe_mout[p][slice].size[i] = table_out_col_type[i] * tab_c.getRowNum();
            }
            probe_mout[p][slice].num_event_wait_list = evt_probe_d2h[e_i].size();
            probe_mout[p][slice].event_wait_list = evt_probe_d2h[e_i].data();
            pool.q6.push(probe_mout[p][slice]);

            e_i++;
        }
    }
    // std::cout << "debug: " << e_i << std::endl;
    clWaitForEvents(1, evt_probe_memcpy_out[e_i - 1].data());
    clWaitForEvents(1, evt_probe_memcpy_out[e_i - 2].data());
    tv_hj.add(); // 1

    pool.q4_run = 0;
    pool.q5_run_ping = 0;
    pool.q5_run_pong = 0;
    pool.q6_run = 0;

    pool.build_in_t.join();
    pool.probe_in_ping_t.join();
    pool.probe_in_pong_t.join();
    pool.probe_out_t.join();
    // calculate the aggr results:
    // sum(l_extendedprice * (1-discount))
    // col0:extendedprice*100 col1: discount*100 col2: orderdate col3: keyid
    int out_nrow_sum = 0;
    for (int p = 0; p < partition_num; p++) {
        for (int slice = 0; slice < slice_num; slice++) {
#ifdef USER_DEBUG
            printf("GQE result p: %d s %d has %d rows\n", p, slice, pool.toutrow[p][slice]);
#endif
            out_nrow_sum += pool.toutrow[p][slice];
        }
    }
    tab_c.setRowNum(out_nrow_sum);
    // printf("GQE result all has %d rows\n", out_nrow_sum);

    //------------------------------------------------------------------------
    //-----------------print the execution time of each part------------------

    double tvtime = 0;

    double hj_total_size = o_input_memcpy_size + l_input_memcpy_size;
    tvtime = tv_hj.getMilliSec();
    double total_time = tvtime + tvtime_lpart + tvtime_opart;
    double out_bytes = (double)out_nrow_sum * sizeof(int) * out_valid_col_num / 1024 / 1024;

    std::cout << "-----------------------Input/Output Info-----------------------" << std::endl;
    std::cout << "Table" << std::setw(20) << "Column Number" << std::setw(30) << "Row Number" << std::endl;
    std::cout << "L" << std::setw(24) << o_valid_col_num << std::setw(30) << o_nrow << std::endl;
    std::cout << "R" << std::setw(24) << l_valid_col_num << std::setw(30) << l_nrow << std::endl;
    std::cout << "LxR" << std::setw(22) << out_valid_col_num << std::setw(30) << out_nrow_sum << std::endl;
    std::cout << "-----------------------Data Transfer Info-----------------------" << std::endl;
    std::cout << "H2D size (Left Table) = " << o_input_memcpy_size << " MB" << std::endl;
    std::cout << "H2D size (Right Table) = " << l_input_memcpy_size << " MB" << std::endl;
    std::cout << "D2H size = " << out_bytes << " MB" << std::endl;

    std::cout << "-----------------------Performance Info-----------------------" << std::endl;
    std::cout << "End-to-end JOIN time: ";
    std::cout << (double)total_time / 1000
              << " ms, throughput: " << hj_total_size / 1024 / ((double)total_time / 1000000) << " GB/s" << std::endl;

    //--------------release---------------
    //---------part o-------
    for (int c = 0; c < 8; c++) {
        for (int k = 0; k < 2; k++) {
            clReleaseMemObject(buf_table_o_partition_in_col[c][k]);
            clReleaseMemObject(buf_table_o_partition_out_col[c][k]);
        }
    }
    clReleaseMemObject(buf_cfg5s_part);
    for (int k = 0; k < 2; k++) {
        clReleaseMemObject(buf_meta_o_partition_in[k]);
        clReleaseMemObject(buf_meta_o_partition_out[k]);
    }
    for (int i = 0; i < table_o_sec_num; i++) {
        clReleaseEvent(evt_part_o_memcpy_in[i][0]);
        clReleaseEvent(evt_part_o_h2d[i][0]);
        clReleaseEvent(evt_part_o_krn[i][0]);
        clReleaseEvent(evt_part_o_d2h[i][0]);
        clReleaseEvent(evt_part_o_memcpy_out[i][0]);
    }

    //--------part l-----
    for (int c = 0; c < 8; c++) {
        for (int k = 0; k < 2; k++) {
            clReleaseMemObject(buf_table_l_partition_in_col[c][k]);
            clReleaseMemObject(buf_table_l_partition_out_col[c][k]);
        }
    }
    for (int k = 0; k < 2; k++) {
        clReleaseMemObject(buf_meta_l_partition_in[k]);
        clReleaseMemObject(buf_meta_l_partition_out[k]);
    }
    for (int i = 0; i < table_l_sec_num; i++) {
        clReleaseEvent(evt_part_l_memcpy_in[i][0]);
        clReleaseEvent(evt_part_l_h2d[i][0]);
        clReleaseEvent(evt_part_l_krn[i][0]);
        clReleaseEvent(evt_part_l_d2h[i][0]);
        clReleaseEvent(evt_part_l_memcpy_out[i][0]);
    }

    //--------hj------
    for (int c = 0; c < 8; c++) {
        clReleaseMemObject(buf_table_o_build_in_col[c]);
        for (int k = 0; k < 2; k++) {
            clReleaseMemObject(buf_table_l_probe_in_col[c][k]);
            clReleaseMemObject(buf_table_l_probe_out_col[c][k]);
        }
    }
    clReleaseMemObject(buf_cfg5s_hj);
    for (int k = 0; k < PU_NM * 2; k++) {
        clReleaseMemObject(buf_hj_tmp[k]);
    }
    clReleaseMemObject(buf_meta_build_in);
    clReleaseMemObject(buf_meta_probe_in[0]);
    clReleaseMemObject(buf_meta_probe_in[1]);
    clReleaseMemObject(buf_meta_probe_out[0]);
    clReleaseMemObject(buf_meta_probe_out[1]);

    for (int p = 0; p < partition_num; p++) {
        clReleaseEvent(evt_build_memcpy_in[p][0]);
        clReleaseEvent(evt_build_h2d[p][0]);
        clReleaseEvent(evt_build_krn[p][0]);
    }
    for (int e_i = 0; e_i < partition_num * slice_num; e_i++) {
        clReleaseEvent(evt_probe_memcpy_in[e_i][0]);
        clReleaseEvent(evt_probe_h2d[e_i][0]);
        clReleaseEvent(evt_probe_krn[e_i][0]);
        clReleaseEvent(evt_probe_d2h[e_i][0]);
        clReleaseEvent(evt_probe_memcpy_out[e_i][0]);
    }

    //------
    for (int k = 0; k < 2; k++) {
        clReleaseKernel(partkernel_O[k]);
        clReleaseKernel(partkernel_L[k]);
        clReleaseKernel(pkernel[k]);
    }
    clReleaseKernel(bkernel);

    return SUCCESS;
}

} // database
} // gqe
} // xf
