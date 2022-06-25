/*
 * Copyright 2021 Xilinx, Inc.
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
#include "loop_stt.h"
#include "../vp8/util/memory.hh"
#include "../vp8/util/debug.hh"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sstream>
#include <math.h>
#include <fcntl.h>
#include <assert.h>
#include <ctime>
#include <memory>
#include <atomic>
#include <signal.h>
#ifndef _WIN32
#include <sys/time.h>
#include <sys/types.h>
#include <dirent.h>

#include <unistd.h>
#else
#include <io.h>
#endif
#ifdef __linux
//#include <linux/seccomp.h>
#include <sys/prctl.h>
#include <sys/syscall.h>

#endif
#include <emmintrin.h>
#include "jpgcoder_hls.hh"
#include "jpgcoder.hh"
//#include "recoder.hh"
//#include "bitops.hh"
//#include "component_info.hh"
//#include "uncompressed_components.hh"
//#include "vp8_decoder.hh"
//#include "vp8_encoder.hh"
//#include "simple_decoder.hh"
//#include "simple_encoder.hh"
//#include "fork_serve.hh"
//#include "socket_serve.hh"
//#include "validation.hh"
//#include "../io/ZlibCompression.hh"
//#include "../io/MemReadWriter.hh"
//#include "../io/BufferedIO.hh"
//#include "../io/Zlib0.hh"
//#include "../io/Seccomp.hh"
#ifndef HLS_TEST
#include "xcl2.hpp"
//#include "xhpp_context.hpp"
//#include "xhpp_taskkernel.hpp"
//#include "xhpp_tasktransfer.hpp"
//#include "xhpp_bufferhost.hpp"
//#include "xhpp_graph.hpp"
//#include "xhpp_scheduler.hpp"
#endif

int tvdiff(struct timeval* tv0, struct timeval* tv1) {
    return (tv1->tv_sec - tv0->tv_sec) * 1000000 + (tv1->tv_usec - tv0->tv_usec);
}

bool hls_decode_jpeg_kernel(std::string xclbin_path,
                            int filecnt,
                            std::vector<uint8_t*> datatoDDR,
                            std::vector<int> jpgSize,
                            std::vector<struct_arith>& arith,
                            std::vector<uint8_t*> res,
                            std::vector<uint32_t>& left,
                            std::vector<uint32_t>& rst) {
#ifdef HLS_TEST
    for (int i = 0; i < filecnt; i++) {
        int arith_info[9];
        jpegDecLeptonEncKernel_0((ap_uint<AXI_WIDTH>*)datatoDDR[i], // uint16_t* datatoDDR,
                                 (int)jpgSize[i],                   // int size,
                                 arith_info,
                                 res[i] // uint8_t* res
                                 );
        arith[i].count = arith_info[0];
        arith[i].value = (uint32_t)arith_info[1];
        arith[i].pre_byte = (uint8_t)arith_info[2];
        arith[i].run = (uint16_t)arith_info[3];
        arith[i].pos = (uint32_t)arith_info[4];
        arith[i].range = (uint8_t)arith_info[5];
        arith[i].isFirst = (bool)arith_info[6];
        left[i] = (uint32_t)arith_info[7];
        rst[i] = (uint32_t)arith_info[8];
    }
#else
    // platform related operations
    std::vector<cl::Device> devices = xcl::get_xil_devices();
    cl::Device device = devices[0];

    // Creating Context and Command Queue for selected Device
    cl::Context context(device);
    cl::CommandQueue q(context, device, CL_QUEUE_PROFILING_ENABLE | CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE);
    std::string devName = device.getInfo<CL_DEVICE_NAME>();
    printf("Found Device=%s\n", devName.c_str());

    cl::Program::Binaries xclBins = xcl::import_binary_file(xclbin_path);
    devices.resize(1);
    cl::Program program(context, devices, xclBins);
    // knum used to define kernel number
    int knum = 1;
    std::vector<cl::Kernel> lepEncKernel(knum);
    for (int i = 0; i < knum; i++) {
        // lepEncKernel[i] = cl::Kernel(program, ("lepEnc:{lepEnc_" + std::to_string(i) + "}").c_str());
        lepEncKernel[i] = cl::Kernel(program, ("lepEnc:{lepEnc_" + std::to_string(i) + "}").c_str());
    }

    std::cout << "kernel has been created" << std::endl;

    std::vector<uint8_t*> datatoDDR_d(knum);
    std::vector<int> jpgSize_d(knum);
    std::vector<int*> arith_info_d(knum);
    std::vector<uint8_t*> res_d(knum);

    std::vector<cl_mem_ext_ptr_t> mext_datatoDDR(knum);
    std::vector<cl_mem_ext_ptr_t> mext_arith_info(knum);
    std::vector<cl_mem_ext_ptr_t> mext_res(knum);

    std::vector<cl::Buffer> datatoDDR_buf(knum);
    std::vector<cl::Buffer> arith_info_buf(knum);
    std::vector<cl::Buffer> res_buf(knum);

    uint64_t maxJpgSize = 0;
    for (int i = 0; i < filecnt; i++) {
        if (jpgSize[i] > maxJpgSize) maxJpgSize = jpgSize[i];
    }

    for (int i = 0; i < knum; i++) {
        datatoDDR_d[i] = aligned_alloc<uint8_t>(maxJpgSize);
        res_d[i] = aligned_alloc<uint8_t>(maxJpgSize);
        arith_info_d[i] = aligned_alloc<int>(9);

        if (i < 3) {
            mext_datatoDDR[i] = {(unsigned int)(0) | XCL_MEM_TOPOLOGY, datatoDDR_d[i]};
            mext_arith_info[i] = {(unsigned int)(0) | XCL_MEM_TOPOLOGY, arith_info_d[i]};
            mext_res[i] = {(unsigned int)(0) | XCL_MEM_TOPOLOGY, res_d[i]};
        } else if (i == 3) {
            mext_datatoDDR[i] = {(unsigned int)(1) | XCL_MEM_TOPOLOGY, datatoDDR_d[i]};
            mext_arith_info[i] = {(unsigned int)(1) | XCL_MEM_TOPOLOGY, arith_info_d[i]};
            mext_res[i] = {(unsigned int)(1) | XCL_MEM_TOPOLOGY, res_d[i]};

        } else {
            mext_datatoDDR[i] = {(unsigned int)(2) | XCL_MEM_TOPOLOGY, datatoDDR_d[i]};
            mext_arith_info[i] = {(unsigned int)(2) | XCL_MEM_TOPOLOGY, arith_info_d[i]};
            mext_res[i] = {(unsigned int)(2) | XCL_MEM_TOPOLOGY, res_d[i]};
        }

        datatoDDR_buf[i] = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY,
                                      (size_t)(maxJpgSize), &mext_datatoDDR[i]);

        res_buf[i] = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY,
                                (size_t)(maxJpgSize), &mext_res[i]);

        arith_info_buf[i] = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY,
                                       (size_t)(9 * sizeof(int)), &mext_arith_info[i]);
    }

    int cur_k = 0;
    for (int i = 0; i < filecnt; i++) {
        jpgSize_d[cur_k] = jpgSize[i];
        memcpy(datatoDDR_d[cur_k], datatoDDR[i], jpgSize_d[cur_k]);

        std::vector<cl::Memory> ib;
        ib.push_back(datatoDDR_buf[cur_k]);

        int j = 0;
        lepEncKernel[cur_k].setArg(j++, datatoDDR_buf[cur_k]);
        lepEncKernel[cur_k].setArg(j++, jpgSize_d[cur_k]);
        lepEncKernel[cur_k].setArg(j++, arith_info_buf[cur_k]);
        lepEncKernel[cur_k].setArg(j++, res_buf[cur_k]);

        std::vector<cl::Memory> ob;
        ob.push_back(arith_info_buf[cur_k]);
        ob.push_back(res_buf[cur_k]);

        std::vector<cl::Event> write_event;
        std::vector<cl::Event> kernel_event;
        std::vector<cl::Event> read_event;
        write_event.resize(1);
        kernel_event.resize(1);
        read_event.resize(1);

        q.enqueueMigrateMemObjects(ib, 0, nullptr, &write_event[0]);
        q.enqueueTask(lepEncKernel[cur_k], &write_event, &kernel_event[0]);
        q.enqueueMigrateMemObjects(ob, CL_MIGRATE_MEM_OBJECT_HOST, &kernel_event, &read_event[0]);
        std::cout << "host kernel call" << std::endl;
        q.finish();
        std::cout << "host kernel end" << std::endl;
        arith[i].count = arith_info_d[cur_k][0];
        arith[i].value = (uint32_t)arith_info_d[cur_k][1];
        arith[i].pre_byte = (uint8_t)arith_info_d[cur_k][2];
        arith[i].run = (uint16_t)arith_info_d[cur_k][3];
        arith[i].pos = (uint32_t)arith_info_d[cur_k][4];
        arith[i].range = (uint8_t)arith_info_d[cur_k][5];
        arith[i].isFirst = (bool)arith_info_d[cur_k][6];
        left[i] = (uint32_t)arith_info_d[cur_k][7];
        rst[i] = (uint32_t)arith_info_d[cur_k][8];
        memcpy(res[i], res_d[cur_k], arith[i].pos);
        std::cout << "kernel " << cur_k << " finish" << std::endl;
        //        if (cur_k != 6) {
        //            cur_k++;
        //        } else {
        //            cur_k = 0;
        //        }
    }

    /*    int knum = 7;
        xhpp::context cst("xilinx_u200_xdma_201830_2", xclbin_path, xhpp::pipeline);
        cst.create();
        // init vbuffers, used in graph
        std::vector<xhpp::vbuffer::host<uint16_t> > hostb_in(knum, &cst);
        std::vector<xhpp::vbuffer::host<uint8_t> > hostb_out(knum, &cst);
        std::vector<xhpp::vbuffer::host<int> > hostb_s1(knum, &cst);

        std::vector<xhpp::vbuffer::device<uint16_t> > devb_in(knum, &cst);
        std::vector<xhpp::vbuffer::device<uint8_t> > devb_out(knum, &cst);
        std::vector<xhpp::vbuffer::device<int> > devb_s1(knum, &cst);

        uint64_t maxJpgSize = 0;
        for (int i = 0; i < filecnt; i++) {
            if (jpgSize[i] > maxJpgSize) maxJpgSize = jpgSize[i];
        }
        // int n_size = 1000000;//1MB for android and small.jpg

        for (int i = 0; i < knum; i++) {
            hostb_in[i].setsize(maxJpgSize);
            hostb_out[i].setsize(maxJpgSize);
            hostb_s1[i].setsize(9);

            devb_in[i].setsize(maxJpgSize);
            devb_out[i].setsize(maxJpgSize);
            devb_s1[i].setsize(9);
        }

        // init tasks
        std::vector<xhpp::task::data_transfer> tsk1;
        for (int i = 0; i < knum; i++) {
            xhpp::task::data_transfer tmp(&cst, xhpp::host2dev);
            tsk1.push_back(tmp);
        }

        // xhpp::task::data_transfer tsk2(&cst, xhpp::host2dev);
        std::vector<xhpp::task::dev_func> tsk2(knum, &cst);
        std::vector<xhpp::task::data_transfer> tsk3;
        for (int i = 0; i < knum; i++) {
            xhpp::task::data_transfer tmp(&cst, xhpp::dev2host);
            tsk3.push_back(tmp);
        }
        std::vector<xhpp::task::data_transfer> tsk4;
        for (int i = 0; i < knum; i++) {
            xhpp::task::data_transfer tmp(&cst, xhpp::dev2host);
            tsk4.push_back(tmp);
        }

        for (int i = 0; i < knum; i++) {
            tsk1[i].setparam(&(hostb_in[i]), &devb_in[i]); // host2dev
            // tsk2.setparam(&hb_b, &db_b);   //host2dev

            int vadd_banks[3] = {0, 0, 0};
            if (i < 3) {
                vadd_banks[0] = 0;
                vadd_banks[1] = 0;
                vadd_banks[2] = 0;
            } else if (i == 3) {
                vadd_banks[0] = 1;
                vadd_banks[1] = 1;
                vadd_banks[2] = 1;
            } else {
                vadd_banks[0] = 2;
                vadd_banks[1] = 2;
                vadd_banks[2] = 2;
            }
            // int vadd_banks2[3]={2,2,3};
            // tsk2.addcu("JPEGD_LeptonE_kernel", 3, vadd_banks);
            std::string name = "lepEnc:{lepEnc_" + std::to_string(i) + "}";
            tsk2[i].addcu(name, 3, vadd_banks);
            // tsk3.addcu("JPEGD_LeptonE_kernel2", 3, vadd_banks2);
            int sizeint = 0;
            tsk2[i].setparam(devb_in[i], sizeint,
                             devb_s1[i], // arith.count,
                             devb_out[i]);
            tsk3[i].setparam(&hostb_out[i], &devb_out[i]);
            tsk4[i].setparam(&hostb_s1[i], &devb_s1[i]);
        }

        std::vector<xhpp::graph> gr(knum);
        for (int i = 0; i < knum; i++) {
            std::string name1 = std::to_string(i * 10 + 1);
            std::string name2 = std::to_string(i * 10 + 2);
            std::string name3 = std::to_string(i * 10 + 3);
            std::string name4 = std::to_string(i * 10 + 4);

            gr[i].addnode(&(tsk1[i]), name1, xhpp::start);
            gr[i].addnode(&(tsk2[i]), name2);
            gr[i].addnode(&(tsk3[i]), name3, xhpp::end);
            gr[i].addnode(&(tsk4[i]), name4, xhpp::end);

            gr[i].addedge(name1, name2);
            gr[i].addedge(name2, name3);
            gr[i].addedge(name3, name4);

            gr[i].setup();
        }

        std::vector<xhpp::engine> sch;
        for (int i = 0; i < knum; i++) {
            xhpp::engine tmp(&cst, &(gr[i]));
            sch.push_back(tmp);
            sch.back().setup();
        }

        //----dump setup----

        //---------------------
        // assign data of topapi, use buffer in the schedule
        // The value of the vbuffer is overwritten by the buffer input
        // todo imporve the speed!
        // int loop_size = 1;
        xhpp::buffer::host<uint16_t> hdata_in[filecnt](&cst);
        xhpp::buffer::host<uint8_t> hdata_out[filecnt](&cst);
        xhpp::buffer::host<int> hdata_s1[filecnt](&cst);

        //	  a.allocate(jpgSize);
        //	  for(int i = 0; i<size; i++)
        //		  a[i] = datatoDDR[i];
        for (int i = 0; i < filecnt; i++) {
            hdata_in[i].allocate(maxJpgSize);
            hdata_out[i].allocate(maxJpgSize);
            hdata_s1[i].allocate(9);
            int buf_size;
            if (jpgSize[i] & 1 == 1) {
                buf_size = (jpgSize[i] + 1) / 2;
            } else {
                buf_size = jpgSize[i] / 2;
            }
            for (int j = 0; j < buf_size; j++) {
                ap_uint<AXI_WIDTH>* tmp = (ap_uint<AXI_WIDTH>*)datatoDDR[i];
                hdata_in[i][j] = tmp[j];
                hdata_out[i][j] = 0;
            }
        }

        // lauch top api
        //	  for (int i=0; i < loop_size; i++){
        //	    sch.run("input", &tsk1, &(hdata_in[i]), 0, //0 refers to the 1st param of task1.
        //	            "output",&tsk3, &(hdata_out[i]), 0,
        //				"output",&tsk4, &(hdata_s1[i]), 0);
        //	  }
        cst.wait();
        struct timeval tv_r_s, tv_r_e;
        gettimeofday(&tv_r_s, 0);
        for (int j = 0; j < filecnt; j++) {
            int i = j % knum;
            sch[i].run("input", &(tsk1[i]), &(hdata_in[j]), 0, // 0 refers to the 1st param of task1.
                       "input", &(tsk2[i]), jpgSize[j], 1, "output", &(tsk3[i]), &(hdata_out[j]),
                       0, // 0 refers to the 1st param of task2
                       "output", &(tsk4[i]), &(hdata_s1[j]), 0);
        }
        cst.wait();
        gettimeofday(&tv_r_e, 0);
        std::cout << "End to End time: " << std::dec << tvdiff(&tv_r_s, &tv_r_e) / 1000 << " ms" << std::endl;

        //--------output data---------
        for (int s = 0; s < filecnt; s++) {
            arith[s].count = hdata_s1[s][0];
            arith[s].value = (uint32_t)hdata_s1[s][1];
            arith[s].pre_byte = (uint8_t)hdata_s1[s][2];
            arith[s].run = (uint16_t)hdata_s1[s][3];
            arith[s].pos = (uint32_t)hdata_s1[s][4];
            arith[s].range = (uint8_t)hdata_s1[s][5];
            arith[s].isFirst = (bool)hdata_s1[s][6];
            left[s] = (uint32_t)hdata_s1[s][7];
            rst[s] = (uint32_t)hdata_s1[s][8];

            for (int j = 0; j < arith[s].pos; j++) {
                *(res[s] + j) = hdata_out[s][j];
            }
        }*/

    fprintf(stderr, "=========== arith print ==========\n");
    fprintf(stderr, "count = %d\n", arith[0].count);
    fprintf(stderr, "value = %d\n", arith[0].value);
    fprintf(stderr, "pre_byte = %d\n", arith[0].pre_byte);
    fprintf(stderr, "run = %d\n", arith[0].run);
    fprintf(stderr, "pos = %d\n", arith[0].pos);
    fprintf(stderr, "range = %d\n", arith[0].range);
    fprintf(stderr, "isFirst = %d\n", arith[0].isFirst);
    //  for(int pos=0; pos<arith.pos; pos++){
    //  //      fprintf( stderr, " %.4x\n" , *(res+pos));
    //  //  }
    fprintf(stderr, "============ end print ==========\n");
#endif
    return true;
}
