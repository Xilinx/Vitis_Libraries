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

#include "xf_database/compaction_core/core.hpp"
#include "xf_database/compaction_core/config.hpp"
#include <iostream>
#include <stdlib.h>
#include <vector>
#include <string>
#include <iterator>
#include <algorithm>
#include <ap_int.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <future>

#include <CL/cl_ext_xilinx.h>
#include <CL/cl.h>

#include "xclhost.hpp"
#include <assert.h>

class MM {
   private:
    size_t _total;
    std::vector<void*> _pvec;

   public:
    MM() : _total(0) {}
    ~MM() {
        for (void* p : _pvec) {
            if (p) free(p);
        }
    }
    size_t size() const { return _total; }
    template <typename T>
    T* aligned_alloc(std::size_t num) {
        void* ptr = nullptr;
        size_t sz = num * sizeof(T);
        if (posix_memalign(&ptr, 4096, sz)) throw std::bad_alloc();
        _pvec.push_back(ptr);
        _total += sz;
        // printf("align_alloc %lu Bytes\n", sz);
        return reinterpret_cast<T*>(ptr);
    }
};

enum ErrCode { SUCCESS = 0, ERROR = 1 };
enum DATASIZE {
    sz_1k = 1024,
    sz_64k = 64 * 1024,
    sz_128k = 128 * 1024,
    sz_1m = 1024 * 1024,
    sz_8m = 8 * 1024 * 1024,
    sz_64m = 64 * 1024 * 1024,
    sz_128m = 128 * 1024 * 1024,
    sz_256m = 256 * 1024 * 1024,
    sz_512m = 512 * 1024 * 1024,
    sz_1g = 1024 * 1024 * 1024,
    sz_test = sz_256m,
    sz_test_ch = sz_test / sz_64m
};

class WrapperAPI {
   private:
    static const int increase_factor = IncreaseFactor_;
    static const int datapath_width = BufWidth_Set_;
    static const int datapath_bytes = datapath_width / 8;

    cl_int err;
    cl_context ctx;
    cl_device_id dev_id;
    cl_command_queue cq;
    cl_program prg;

    cl_mem bank0_keyBuf[2];
    cl_mem bank0_keyLen[2];
    cl_mem bank2_keyBuf[2];
    cl_mem bank2_keyLen[2];

    ap_uint<datapath_width>* bank0_keyBuf_[2];
    ap_uint<32>* bank0_keyLen_[2];
    ap_uint<datapath_width>* bank2_keyBuf_[2];
    ap_uint<32>* bank2_keyLen_[2];

    cl_kernel CUCoreLoopTop_0;
    cl_kernel CUCoreLoopTop_1;

    std::string xclbin_path;
    MM mm;

    std::string gen_random(const int len) {
        std::string tmp_s;
        static const char alphanum[] =
            "0123456789"
            "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
            "abcdefghijklmnopqrstuvwxyz";

        tmp_s.reserve(len);

        for (int i = 0; i < len; ++i) tmp_s += alphanum[rand() % (sizeof(alphanum) - 1)];

        return tmp_s;
    }

   public:
    ~WrapperAPI() {
        clReleaseKernel(CUCoreLoopTop_0);
        clReleaseKernel(CUCoreLoopTop_1);
        clReleaseProgram(prg);
        clReleaseCommandQueue(cq);
        for (int i = 0; i < 2; i++) {
            clReleaseMemObject(bank0_keyBuf[i]);
            clReleaseMemObject(bank0_keyLen[i]);
            clReleaseMemObject(bank2_keyBuf[i]);
            clReleaseMemObject(bank2_keyLen[i]);
        }
        clReleaseContext(ctx);

        for (int i = 0; i < 2; i++) {
            free(bank0_keyBuf_[i]);
            free(bank0_keyLen_[i]);
            free(bank2_keyBuf_[i]);
            free(bank2_keyLen_[i]);
        }
    }

    void init(std::string xclbin_path, int device_id, bool user_setting = false) {
        size_t buf_size = sizeof(ap_uint<datapath_width>) * sz_64k * increase_factor * 2;
        size_t meta_size = sizeof(ap_uint<32>) * sz_64k * increase_factor * 2;

        // device init
        std::string dsa_name;
        err = init_hardware(&ctx, &dev_id, &cq, CL_QUEUE_PROFILING_ENABLE | CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE,
                            dsa_name, device_id, user_setting);
        if (err != CL_SUCCESS) {
            fprintf(stderr, "ERROR: fail to init OpenCL \n");
            exit(1);
        }

        std::cout << "platform initiated" << std::endl;
        err = load_binary(&prg, ctx, dev_id, xclbin_path.c_str());
        if (err != CL_SUCCESS) {
            fprintf(stderr, "ERROR: fail to program PL\n");
            exit(1);
        }
        std::cout << "binary loaded" << std::endl;

        CUCoreLoopTop_0 = clCreateKernel(prg, "CUCoreLoopTop:{CUCoreLoopTop_0}", &err);
        if (err != CL_SUCCESS) {
            fprintf(stderr, "ERROR: fail to create kernel 0\n");
            exit(1);
        }
        CUCoreLoopTop_1 = clCreateKernel(prg, "CUCoreLoopTop:{CUCoreLoopTop_1}", &err);
        if (err != CL_SUCCESS) {
            fprintf(stderr, "ERROR: fail to create kernel 1\n");
            exit(1);
        }

        cl_mem_ext_ptr_t mext_bank0_keyBuf[2], mext_bank0_keyLen[2], mext_bank2_keyBuf[2], mext_bank2_keyLen[2];
        for (int i = 0; i < 2; i++) {
            assert(!posix_memalign((void**)&bank0_keyBuf_[i], 4096, buf_size));
            assert(!posix_memalign((void**)&bank0_keyLen_[i], 4096, meta_size));
            assert(!posix_memalign((void**)&bank2_keyBuf_[i], 4096, buf_size));
            assert(!posix_memalign((void**)&bank2_keyLen_[i], 4096, meta_size));

            mext_bank0_keyBuf[i] = {0};
            mext_bank0_keyBuf[i].banks = 0 | XCL_MEM_TOPOLOGY;
            mext_bank0_keyBuf[i].host_ptr = bank0_keyBuf_[i];
            mext_bank0_keyLen[i] = {0};
            mext_bank0_keyLen[i].banks = 0 | XCL_MEM_TOPOLOGY;
            mext_bank0_keyLen[i].host_ptr = bank0_keyLen_[i];

            mext_bank2_keyBuf[i] = {0};
            mext_bank2_keyBuf[i].banks = 2 | XCL_MEM_TOPOLOGY;
            mext_bank2_keyBuf[i].host_ptr = bank2_keyBuf_[i];
            mext_bank2_keyLen[i] = {0};
            mext_bank2_keyLen[i].banks = 2 | XCL_MEM_TOPOLOGY;
            mext_bank2_keyLen[i].host_ptr = bank2_keyLen_[i];

            bank0_keyBuf[i] = clCreateBuffer(ctx, CL_MEM_EXT_PTR_XILINX | CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR,
                                             buf_size, &mext_bank0_keyBuf[i], &err);
            if (err != CL_SUCCESS) {
                std::cout << "Create Device bank0_keyBuf[" << i << "] failed!" << std::endl;
                exit(1);
            }
            bank0_keyLen[i] = clCreateBuffer(ctx, CL_MEM_EXT_PTR_XILINX | CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR,
                                             meta_size, &mext_bank0_keyLen[i], &err);
            if (err != CL_SUCCESS) {
                std::cout << "Create Device bank0_keyLen[" << i << "] failed!" << std::endl;
                exit(1);
            }
            bank2_keyBuf[i] = clCreateBuffer(ctx, CL_MEM_EXT_PTR_XILINX | CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR,
                                             buf_size, &mext_bank2_keyBuf[i], &err);
            if (err != CL_SUCCESS) {
                std::cout << "Create Device bank2_keyBuf[" << i << "] failed!" << std::endl;
                exit(1);
            }
            bank2_keyLen[i] = clCreateBuffer(ctx, CL_MEM_EXT_PTR_XILINX | CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR,
                                             meta_size, &mext_bank2_keyLen[i], &err);
            if (err != CL_SUCCESS) {
                std::cout << "Create Device bank2_keyLen[" << i << "] failed!" << std::endl;
                exit(1);
            }
        }

        std::cout << "Start initialization done" << std::endl;
    }

    int compaction_test() {
        ////////////////////////////////////////////////////////////////
        // Test Data Generation
        ////////////////////////////////////////////////////////////////
        int keyAll = 10000 * increase_factor;
        unsigned int seed = time(NULL);
        std::cout << "WARNING " << seed << std::endl;
        srand(seed);
        int in_keyCnt = rand() % (keyAll - 1) + 1;
        int in_keyCnt3 = rand() % keyAll + 1;
        assert(keyAll > 0);
        assert(in_keyCnt > 0);
        assert(keyAll > in_keyCnt);
        assert(in_keyCnt3 > 0);
        std::cout << "keyAll " << keyAll << " keyCnt1 " << in_keyCnt << " keyCnt2 " << keyAll - in_keyCnt << " keyCnt3 "
                  << in_keyCnt3 << std::endl;

        std::vector<std::string> lall;
        for (int i = 0; i < keyAll + in_keyCnt3; i++) {
            lall.push_back(gen_random(rand() % 128 + 32));
        }
        auto a_it = std::unique(lall.begin(), lall.end());
        lall.resize(std::distance(lall.begin(), a_it));
        std::cout << "All keys generated" << std::endl;

        std::vector<std::string> l1;
        for (int i = 0; i < in_keyCnt; i++) {
            l1.push_back(lall[i]);
        }
        auto it = std::unique(l1.begin(), l1.end());
        l1.resize(std::distance(l1.begin(), it));
        sort(l1.begin(), l1.end());
        std::cout << "L1 added & sorted" << std::endl;
        unsigned int byte_cnt1 = 0;
        unsigned int token_cnt1 = 0;
        for (auto it : l1) {
            byte_cnt1 += it.length();
            token_cnt1 += (it.length() + datapath_bytes - 1) / datapath_bytes;
        }

        std::vector<std::string> l2;
        for (int i = 0; i < keyAll - in_keyCnt; i++) {
            l2.push_back(lall[i + in_keyCnt]);
        }
        it = std::unique(l2.begin(), l2.end());
        l2.resize(std::distance(l2.begin(), it));
        sort(l2.begin(), l2.end());
        std::cout << "L2 added & sorted" << std::endl;
        unsigned int byte_cnt2 = 0;
        unsigned int token_cnt2 = 0;
        for (auto it : l2) {
            byte_cnt2 += it.length();
            token_cnt2 += (it.length() + datapath_bytes - 1) / datapath_bytes;
        }

        std::vector<std::string> l3;
        for (int i = 0; i < in_keyCnt3; i++) {
            l3.push_back(lall[i + keyAll]);
        }
        it = std::unique(l3.begin(), l3.end());
        l3.resize(std::distance(l3.begin(), it));
        sort(l3.begin(), l3.end());
        std::cout << "L3 added & sorted" << std::endl;
        unsigned int byte_cnt3 = 0;
        unsigned int token_cnt3 = 0;
        for (auto it : l3) {
            byte_cnt3 += it.length();
            token_cnt3 += (it.length() + datapath_bytes - 1) / datapath_bytes;
            ;
        }

        unsigned int byte_cnt = (byte_cnt1 > byte_cnt2) ? byte_cnt1 : byte_cnt2;
        if ((byte_cnt + datapath_bytes - 1) / datapath_bytes > 1024 * 64 * increase_factor) {
            printf("Buffer overflow! need %d\n", (byte_cnt + datapath_bytes - 1) / datapath_bytes);
            exit(1);
        }
        std::cout << "Buffer word count: " << (byte_cnt + datapath_bytes - 1) / datapath_bytes << std::endl;

        // Fill two bank 0 buffer sets
        bank0_keyLen_[0][0] = l1.size();
        unsigned int word_ptr = 0;
        unsigned int byte_ptr = 0;
        unsigned int meta_ptr = 1;
        for (auto curr_string : l1) {
            for (auto curr_char : curr_string) {
                bank0_keyBuf_[0][word_ptr].range(byte_ptr * 8 + 8 - 1, byte_ptr * 8) = curr_char;
                byte_ptr++;
                if (byte_ptr == datapath_bytes) {
                    word_ptr++;
                    byte_ptr = 0;
                }
            }
            bank0_keyLen_[0][meta_ptr++] = curr_string.length();
        }
        std::cout << "Bank0 Buf 0 word ptr " << word_ptr << std::endl;

        bank0_keyLen_[1][0] = keyAll - in_keyCnt;
        word_ptr = 0;
        byte_ptr = 0;
        unsigned int base_meta_ptr = 1;

        for (auto curr_string : l2) {
            // std::cout << curr_string << " " << curr_string.length() << std::endl;
            for (auto curr_char : curr_string) {
                bank0_keyBuf_[1][word_ptr].range(byte_ptr * 8 + 8 - 1, byte_ptr * 8) = curr_char;
                byte_ptr++;
                if (byte_ptr == datapath_bytes) {
                    word_ptr++;
                    byte_ptr = 0;
                }
            }
            bank0_keyLen_[1][base_meta_ptr++] = curr_string.length();
        }
        std::cout << "Bank0 Buf 1 word ptr " << word_ptr << std::endl;

        for (int i = 0; i < 2; i++) {
            for (int j = 0; j < datapath_bytes; j++) {
                unsigned char curr = bank0_keyBuf_[0][i].range(j * 8 + 8 - 1, j * 8);
                if (curr != 0) {
                    printf("-- %c ", curr);
                } else {
                    printf("-- %x", curr);
                }
            }
            printf("\n");
        }

        std::cout << "---------------------------------------------------" << std::endl;

        for (int i = 0; i < 2; i++) {
            for (int j = 0; j < datapath_bytes; j++) {
                unsigned char curr = bank0_keyBuf_[1][i].range(j * 8 + 8 - 1, j * 8);
                if (curr != 0) {
                    printf("-- %c ", curr);
                } else {
                    printf("-- %x", curr);
                }
            }
            printf("\n");
        }

        ////////////////////////////////////////////////////////////
        // Kernel Input Setup
        ////////////////////////////////////////////////////////////
        int j = 0;
        unsigned char new_id = 0;
        unsigned char base_id = 1;
        unsigned int in_dataBurst_len = (byte_cnt1 + datapath_bytes - 1) / datapath_bytes + 4;
        unsigned int in_metaBurst_len = meta_ptr + 4;
        unsigned int base_dataBurst_len = (byte_cnt2 + datapath_bytes - 1) / datapath_bytes + 4;
        unsigned int base_metaBurst_len = base_meta_ptr + 4;
        std::cout << "in data burst len: " << (byte_cnt1 + datapath_bytes - 1) / datapath_bytes + 4
                  << " meta burst len: " << meta_ptr + 4 << std::endl;
        std::cout << "base data burst len: " << (byte_cnt2 + datapath_bytes - 1) / datapath_bytes + 4
                  << " meta burst len: " << base_meta_ptr + 4 << std::endl;

        clSetKernelArg(CUCoreLoopTop_1, j++, sizeof(unsigned char), &new_id);
        clSetKernelArg(CUCoreLoopTop_1, j++, sizeof(cl_mem), &bank0_keyBuf[0]);
        clSetKernelArg(CUCoreLoopTop_1, j++, sizeof(cl_mem), &bank0_keyLen[0]);

        clSetKernelArg(CUCoreLoopTop_1, j++, sizeof(unsigned char), &base_id);
        clSetKernelArg(CUCoreLoopTop_1, j++, sizeof(cl_mem), &bank0_keyBuf[1]);
        clSetKernelArg(CUCoreLoopTop_1, j++, sizeof(cl_mem), &bank0_keyLen[1]);

        clSetKernelArg(CUCoreLoopTop_1, j++, sizeof(cl_mem), &bank2_keyBuf[1]);
        clSetKernelArg(CUCoreLoopTop_1, j++, sizeof(cl_mem), &bank2_keyLen[1]);

        clSetKernelArg(CUCoreLoopTop_1, j++, sizeof(unsigned int), &in_dataBurst_len);
        clSetKernelArg(CUCoreLoopTop_1, j++, sizeof(unsigned int), &in_metaBurst_len);
        clSetKernelArg(CUCoreLoopTop_1, j++, sizeof(unsigned int), &base_dataBurst_len);
        clSetKernelArg(CUCoreLoopTop_1, j++, sizeof(unsigned int), &base_metaBurst_len);

        std::cout << "Set kernel 1 arguments done" << std::endl;

        ////////////////////////////////////////////////////////////
        // Transfer Test Data
        ////////////////////////////////////////////////////////////
        cl_event ev_;
        cl_mem in_mem[4];
        in_mem[0] = bank0_keyBuf[0];
        in_mem[1] = bank0_keyBuf[1];
        in_mem[2] = bank0_keyLen[0];
        in_mem[3] = bank0_keyLen[1];
        clEnqueueMigrateMemObjects(cq, 4, in_mem, 0, 0, NULL, &ev_);
        // clWaitForEvents(1, &ev_);
        clFinish(cq);
        clReleaseEvent(ev_);
        std::cout << "Migrating input data done" << std::endl;
        ////////////////////////////////////////////////////////////
        // Execute Kernel
        ////////////////////////////////////////////////////////////

        cl_event ev_kernel;
        err = clEnqueueTask(cq, CUCoreLoopTop_1, 0, NULL, &ev_kernel);
        if (err != CL_SUCCESS) {
            std::cout << "EnqueueTask insert CUCoreLoopTop_1 failed" << std::endl;
        }
        // clWaitForEvents(1, &ev_kernel);
        clFinish(cq);
        std::cout << "Enqueue kernel task done" << std::endl;

        ////////////////////////////////////////////////////////////
        // Retrieve Result Data
        ////////////////////////////////////////////////////////////
        /*
        cl_event ev_read;
        clEnqueueMigrateMemObjects(cq, 1, &out_meta, CL_MIGRATE_MEM_OBJECT_HOST, 0, NULL, &ev_read);
        //clWaitForEvents(1, &ev_read);
        clFinish(cq);
        clReleaseEvent(ev_read);
        std::cout << "Migrating output data done" << std::endl;
        */
        ////////////////////////////////////////////////////////////
        // Throughput measurement
        ////////////////////////////////////////////////////////////
        cl_ulong time_start, time_end, time_used;
        clGetEventProfilingInfo(ev_kernel, CL_PROFILING_COMMAND_START, sizeof(cl_ulong), &time_start, NULL);
        clGetEventProfilingInfo(ev_kernel, CL_PROFILING_COMMAND_END, sizeof(cl_ulong), &time_end, NULL);
        time_used = time_end - time_start;
        printf("Key processed: %d | Time used: %.3f ms ", keyAll, (double)time_used / 1000000);
        double throughput = (double)keyAll / ((double)time_used / 1000000000);
        if (throughput > 1000000000)
            printf("| Throughput: %.3f G samples/s\n", throughput / 1000000000);
        else if (throughput > 1000000)
            printf("| Throughput: %.3f M samples/s\n", throughput / 1000000);
        else if (throughput > 1000)
            printf("| Throughput: %.3f k samples/s\n", throughput / 1000);
        else
            printf("| Throughput: %.3f samples/s\n", throughput / 1);
        clReleaseEvent(ev_kernel);

        ////////////////////////////////////////////////////////////
        // Compare results
        ////////////////////////////////////////////////////////////
        std::vector<std::string> lout;
        std::vector<std::string> lcheck;
        unsigned int sample_cnt = 0;
        ap_uint<32>* out_keyCnt = (ap_uint<32>*)bank2_keyLen_[1];
        std::cout << "Get out keyCnt " << out_keyCnt[0] << std::endl;
        ap_uint<32>* out_streamID = (ap_uint<32>*)&out_keyCnt[1];
        std::cout << "Get out streamID" << std::endl;
        unsigned int l1_ptr = 0, l2_ptr = 0;
        /*
        for(int i = 0; i < out_keyCnt[0]; i++){
            if((unsigned int)(out_streamID[i].range(7,0)) == 0)
                lout.push_back(l1[l1_ptr++]);
            if((unsigned int)(out_streamID[i].range(7,0)) == 1)
                lout.push_back(l2[l2_ptr++]);
        }

        std::cout << "Hardware results generated" << std::endl;

        lcheck = l1;
        copy(l2.begin(), l2.end(), back_inserter(lcheck));
        sort(lcheck.begin(), lcheck.end());

        printf("Check: %s\n", (lout == lcheck) ? "PASS" : "FAIL");
        std::cout << "keyAll " << keyAll << " keyCnt " << in_keyCnt << std::endl;

        printf("\nHardware: ");
        for(auto it: lout){
            printf("%s ", it.c_str());
            sample_cnt ++;
            if(sample_cnt > 10)
                break;
        }
        printf("\nSoftware: ");
        sample_cnt = 0;
        for(auto it: lcheck){
            printf("%s ", it.c_str());
            sample_cnt ++;
            if(sample_cnt > 10)
                break;
        }
        printf("\n");
        */

        ///////////////////////////////////////////////////
        // add in a new stream
        ///////////////////////////////////////////////////
        bank2_keyLen_[0][0] = in_keyCnt3;
        word_ptr = 0;
        byte_ptr = 0;
        unsigned int new_meta_ptr = 1;

        for (auto curr_string : l3) {
            // std::cout << curr_string << " " << curr_string.length() << std::endl;
            for (auto curr_char : curr_string) {
                bank2_keyBuf_[0][word_ptr].range(byte_ptr * 8 + 8 - 1, byte_ptr * 8) = curr_char;
                byte_ptr++;
                if (byte_ptr == datapath_bytes) {
                    word_ptr++;
                    byte_ptr = 0;
                }
            }
            bank2_keyLen_[0][new_meta_ptr++] = curr_string.length();
        }
        std::cout << "Buf 3 word ptr " << word_ptr << " byte ptr " << byte_ptr << std::endl;
        ////////////////////////////////////////////////////////////
        // Kernel Input Setup
        ////////////////////////////////////////////////////////////
        j = 0;
        new_id = 2;
        base_id = 0xFF;
        in_dataBurst_len = (byte_cnt3 + datapath_bytes - 1) / datapath_bytes + 4;
        in_metaBurst_len = new_meta_ptr + 4;
        base_dataBurst_len = token_cnt1 + token_cnt2 + 4;
        base_metaBurst_len = meta_ptr + base_meta_ptr + 4;
        std::cout << "in data burst len: " << (byte_cnt3 + datapath_bytes - 1) / datapath_bytes + 4
                  << " meta burst len: " << new_meta_ptr + 4 << std::endl;

        clSetKernelArg(CUCoreLoopTop_0, j++, sizeof(unsigned char), &new_id);
        clSetKernelArg(CUCoreLoopTop_0, j++, sizeof(cl_mem), &bank2_keyBuf[0]);
        clSetKernelArg(CUCoreLoopTop_0, j++, sizeof(cl_mem), &bank2_keyLen[0]);

        clSetKernelArg(CUCoreLoopTop_0, j++, sizeof(unsigned char), &base_id);
        clSetKernelArg(CUCoreLoopTop_0, j++, sizeof(cl_mem), &bank2_keyBuf[1]);
        clSetKernelArg(CUCoreLoopTop_0, j++, sizeof(cl_mem), &bank2_keyLen[1]);

        clSetKernelArg(CUCoreLoopTop_0, j++, sizeof(cl_mem), &bank0_keyBuf[1]);
        clSetKernelArg(CUCoreLoopTop_0, j++, sizeof(cl_mem), &bank0_keyLen[1]);

        clSetKernelArg(CUCoreLoopTop_0, j++, sizeof(unsigned int), &in_dataBurst_len);
        clSetKernelArg(CUCoreLoopTop_0, j++, sizeof(unsigned int), &in_metaBurst_len);
        clSetKernelArg(CUCoreLoopTop_0, j++, sizeof(unsigned int), &base_dataBurst_len);
        clSetKernelArg(CUCoreLoopTop_0, j++, sizeof(unsigned int), &base_metaBurst_len);

        ////////////////////////////////////////////////////////////
        // Transfer Test Data
        ////////////////////////////////////////////////////////////
        cl_event ev_new_;
        cl_mem in_mem_new[2];
        in_mem_new[0] = bank2_keyBuf[0];
        in_mem_new[1] = bank2_keyLen[0];
        clEnqueueMigrateMemObjects(cq, 2, in_mem_new, 0, 0, NULL, &ev_new_);
        clFinish(cq);
        clReleaseEvent(ev_new_);
        std::cout << "Migrating input data done" << std::endl;

        ////////////////////////////////////////////////////////////
        // Execute Kernel
        ////////////////////////////////////////////////////////////

        cl_event ev_kernel_new_;
        err = clEnqueueTask(cq, CUCoreLoopTop_0, 0, NULL, &ev_kernel_new_);
        if (err != CL_SUCCESS) {
            std::cout << "EnqueueTask insert CUCoreLoopTop_0 failed" << std::endl;
        }
        clFinish(cq);
        clReleaseEvent(ev_kernel_new_);
        std::cout << "Enqueue kernel task done" << std::endl;

        ////////////////////////////////////////////////////////////
        // Retrieve Result Data
        ////////////////////////////////////////////////////////////
        cl_event ev_read_new;
        clEnqueueMigrateMemObjects(cq, 1, &bank0_keyLen[1], CL_MIGRATE_MEM_OBJECT_HOST, 0, NULL, &ev_read_new);
        clFinish(cq);
        clReleaseEvent(ev_read_new);
        std::cout << "Migrating output data done" << std::endl;

        ////////////////////////////////////////////////////////////
        // Compare results
        ////////////////////////////////////////////////////////////
        lout.clear();
        out_keyCnt = (ap_uint<32>*)bank0_keyLen_[1];
        std::cout << "Get out keyCnt " << out_keyCnt[0] << std::endl;
        out_streamID = (ap_uint<32>*)&out_keyCnt[1];
        std::cout << "Get out streamID" << std::endl;
        l1_ptr = 0, l2_ptr = 0;
        unsigned int l3_ptr = 0;
        for (int i = 0; i < out_keyCnt[0]; i++) {
            if ((unsigned int)(out_streamID[i].range(7, 0)) == 0) lout.push_back(l1[l1_ptr++]);
            if ((unsigned int)(out_streamID[i].range(7, 0)) == 1) lout.push_back(l2[l2_ptr++]);
            if ((unsigned int)(out_streamID[i].range(7, 0)) == 2) lout.push_back(l3[l3_ptr++]);
        }
        std::cout << "Hardware results generated" << std::endl;

        lcheck.clear();
        lcheck = l1;
        copy(l2.begin(), l2.end(), back_inserter(lcheck));
        copy(l3.begin(), l3.end(), back_inserter(lcheck));
        sort(lcheck.begin(), lcheck.end());

        printf("\nHardware: ");
        sample_cnt = 0;
        for (auto it : lout) {
            printf("%s ", it.c_str());
            sample_cnt++;
            if (sample_cnt > 10) break;
        }
        printf("\nSoftware: ");
        sample_cnt = 0;
        for (auto it : lcheck) {
            printf("%s ", it.c_str());
            sample_cnt++;
            if (sample_cnt > 10) break;
        }
        printf("\n\n");

        bool ok = lout == lcheck;
        printf("Check: %s\n", ok ? "PASS" : "FAIL");
        std::cout << "keyAll " << keyAll + in_keyCnt3 << " keyCnt " << in_keyCnt << std::endl;

        return (ok ? 0 : 1);
    }
};

int main(int argc, char** argv) {
    if (argc <= 1) {
        printf("Require xclbin path input!\n");
        exit(1);
    }
    std::string xclbin_path(argv[1]);
    WrapperAPI wrapper_api;
    wrapper_api.init(xclbin_path, 0, false);
    int err = wrapper_api.compaction_test();
    return err;
}
