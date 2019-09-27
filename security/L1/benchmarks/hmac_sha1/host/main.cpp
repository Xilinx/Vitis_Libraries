/*
 * Copyright 2019 Xilinx, Inc.
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
#include <ap_int.h>
#include <iostream>

#include <openssl/sha.h>

#include <sys/time.h>
#include <new>
#include <cstdlib>

#include <xcl2.hpp>

#include "kernel_config.hpp"

// text length for each task in 128-bit
#define N_ROW 64
// text length for each task in Byte, should be dividable by (4 * 16 / SUB_GRP_SZ)
#define N_MSG 1024
// number of tasks for a single PCIe block
#define N_TASK 2 // 8192
// number of PUs
#define CH_NM 16
// cipher key size in bytes
#define KEY_SIZE 32

void hmacSHA1(const unsigned char* key,
              unsigned int keyLen,
              const unsigned char* message,
              unsigned int msgLen,
              unsigned char* h) {
    const int MSG_SIZE = 4;             // size of each message word in byte
    const int HASH_SIZE = 5 * MSG_SIZE; // size of hash value in byte
    const int MAX_MSG = 4096;           // max size of message in byte
    const int BLOCK_SIZE = 64;          // size of SHA1 block

    unsigned char kone[BLOCK_SIZE + 8] = {0};
    unsigned char kipad[BLOCK_SIZE + 8] = {0};
    unsigned char kopad[BLOCK_SIZE + 8] = {0};
    unsigned char kmsg[BLOCK_SIZE + MAX_MSG + 8] = {0};
    unsigned char khsh[BLOCK_SIZE + HASH_SIZE + 8] = {0};
    unsigned char h1[HASH_SIZE + 8] = {0};
    unsigned char h2[HASH_SIZE + 8] = {0};

    if (keyLen > BLOCK_SIZE) {
        SHA1((const unsigned char*)key, keyLen, (unsigned char*)h1);
        memcpy(kone, h1, HASH_SIZE);
    } else
        memcpy(kone, key, keyLen);

    for (int i = 0; i < BLOCK_SIZE; ++i) {
        kipad[i] = (unsigned int)(kone[i]) ^ 0x36;
        kopad[i] = (unsigned int)(kone[i]) ^ 0x5c;
    }

    memcpy(kmsg, kipad, BLOCK_SIZE);
    memcpy(kmsg + BLOCK_SIZE, message, msgLen);
    SHA1((const unsigned char*)kmsg, BLOCK_SIZE + msgLen, (unsigned char*)h2);

    memcpy(khsh, kopad, BLOCK_SIZE);
    memcpy(khsh + BLOCK_SIZE, h2, HASH_SIZE);
    SHA1((const unsigned char*)khsh, BLOCK_SIZE + HASH_SIZE, (unsigned char*)h);
}

class ArgParser {
   public:
    ArgParser(int& argc, const char** argv) {
        for (int i = 1; i < argc; ++i) mTokens.push_back(std::string(argv[i]));
    }
    bool getCmdOption(const std::string option, std::string& value) const {
        std::vector<std::string>::const_iterator itr;
        itr = std::find(this->mTokens.begin(), this->mTokens.end(), option);
        if (itr != this->mTokens.end() && ++itr != this->mTokens.end()) {
            value = *itr;
            return true;
        }
        return false;
    }

   private:
    std::vector<std::string> mTokens;
};

inline int tvdiff(struct timeval* tv0, struct timeval* tv1) {
    return (tv1->tv_sec - tv0->tv_sec) * 1000000 + (tv1->tv_usec - tv0->tv_usec);
}

template <typename T>
T* aligned_alloc(std::size_t num) {
    void* ptr = nullptr;
    if (posix_memalign(&ptr, 4096, num * sizeof(T))) throw std::bad_alloc();
    return reinterpret_cast<T*>(ptr);
}

int main(int argc, char* argv[]) {
    // cmd parser
    ArgParser parser(argc, (const char**)argv);
    std::string xclbin_path;
    if (!parser.getCmdOption("-xclbin", xclbin_path)) {
        std::cout << "ERROR:xclbin path is not set!\n";
        return 1;
    }

    // set repeat time
    int num_rep = 2;
    std::string num_str;
    if (parser.getCmdOption("-rep", num_str)) {
        try {
            num_rep = std::stoi(num_str);
        } catch (...) {
            num_rep = 2;
        }
    }
    if (num_rep < 2) {
        num_rep = 2;
        std::cout << "WARNING: ping-pong buffer shoulde be updated at least 1 time.\n";
    }
    if (num_rep > 20) {
        num_rep = 20;
        std::cout << "WARNING: limited repeat to " << num_rep << " times.\n";
    }

    // input data
    const unsigned char datain[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
                                    0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f};

    unsigned char messagein[N_MSG];
    for (int i = 0; i < N_MSG; i += 16) {
        for (int j = 0; j < 16; j++) {
            messagein[i + j] = datain[j];
        }
    }
    // cipher key
    const unsigned char key[] = {0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a,
                                 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25,
                                 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f};

    // initialization vector
    const unsigned char ivec[] = {0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
                                  0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f};

    // generate golden
    unsigned char hmacResult[20];
    hmacSHA1(key, 32, messagein, N_MSG, hmacResult);

    // ouput length of the result

    ap_uint<512> golden = 0;
    for (unsigned int j = 0; j < 20; j++) {
        golden.range(j * 8 + 7, j * 8) = hmacResult[j];
    }

    ap_uint<8 * KEY_SIZE> keyReg;
    for (unsigned int i = 0; i < KEY_SIZE; i++) {
        keyReg.range(i * 8 + 7, i * 8) = key[i];
    }

    ap_uint<128> IVReg;
    for (unsigned int i = 0; i < 16; i++) {
        IVReg.range(i * 8 + 7, i * 8) = ivec[i];
    }

    ap_uint<128> dataReg;
    for (unsigned int i = 0; i < 16; i++) {
        dataReg.range(i * 8 + 7, i * 8) = datain[i];
    }

    std::cout << "Goldens have been created using OpenSSL.\n";

    // Host buffers
    ap_uint<512>* hb_in1 = aligned_alloc<ap_uint<512> >(N_ROW * N_TASK * CH_NM / 4 + CH_NM);
    ap_uint<512>* hb_in2 = aligned_alloc<ap_uint<512> >(N_ROW * N_TASK * CH_NM / 4 + CH_NM);
    ap_uint<512>* hb_in3 = aligned_alloc<ap_uint<512> >(N_ROW * N_TASK * CH_NM / 4 + CH_NM);
    ap_uint<512>* hb_in4 = aligned_alloc<ap_uint<512> >(N_ROW * N_TASK * CH_NM / 4 + CH_NM);
    ap_uint<512>* hb_out_a[4];
    for (int i = 0; i < 4; i++) {
        hb_out_a[i] = aligned_alloc<ap_uint<512> >(N_TASK * CH_NM);
    }
    ap_uint<512>* hb_out_b[4];
    for (int i = 0; i < 4; i++) {
        hb_out_b[i] = aligned_alloc<ap_uint<512> >(N_TASK * CH_NM);
    }

    // generate configurations
    for (unsigned int j = 0; j < CH_NM; j++) {
        // massage length in 128-bit for each task
        hb_in1[j].range(511, 448) = N_ROW;
        hb_in2[j].range(511, 448) = N_ROW;
        hb_in3[j].range(511, 448) = N_ROW;
        hb_in4[j].range(511, 448) = N_ROW;

        // number of tasks in a single PCIe block
        hb_in1[j].range(447, 384) = N_TASK;
        hb_in2[j].range(447, 384) = N_TASK;
        hb_in3[j].range(447, 384) = N_TASK;
        hb_in4[j].range(447, 384) = N_TASK;

        // cipherkey
        hb_in1[j].range(255, 0) = keyReg.range(255, 0);
        hb_in2[j].range(255, 0) = keyReg.range(255, 0);
        hb_in3[j].range(255, 0) = keyReg.range(255, 0);
        hb_in4[j].range(255, 0) = keyReg.range(255, 0);
    }
    // generate texts
    /*
    for (int i = 0; i < N_TASK; i++) {
        for(int j = 0; j < N_ROW; j++) {
            for(int k = 0; k < 128 / 32; k++) {
                for(int l = 0; l < CH_NM / 16; l++) {
                    int pos = l + k * (CH_NM / 16) + j * (CH_NM / 16 * 128 / 32) + i * (CH_NM / 16 * 128 / 32 * N_ROW);
                    for(int m = 0; m < 16; m++) {
                        hb_in1[pos].range(m * 32 + 31, m * 32) = dataReg.range(k * 32 + 31, k * 32);
                        hb_in2[pos].range(m * 32 + 31, m * 32) = dataReg.range(k * 32 + 31, k * 32);
                        hb_in3[pos].range(m * 32 + 31, m * 32) = dataReg.range(k * 32 + 31, k * 32);
                        hb_in4[pos].range(m * 32 + 31, m * 32) = dataReg.range(k * 32 + 31, k * 32);
                    }
                }
            }
        }
    }
    */
    int chWord = (512 / 32 / SUB_GRP_SZ);
    for (int i = 0; i < N_TASK; i++) {
        for (int j = 0; j < (N_MSG / 4); j += chWord) {
            for (int k = 0; k < GRP_NM; k++) {
                int pos = k + (j / chWord) * GRP_NM + i * (N_MSG * CH_NM / 64) + CH_NM;
                for (int l = 0; l < SUB_GRP_SZ; l++) {
                    if (SUB_GRP_SZ >= 4) {
                        hb_in1[pos].range((l + 1) * 32 * chWord - 1, l * 32 * chWord) =
                            dataReg.range(((j % 4) + 1) * chWord * 32 - 1, (j % 4) * chWord * 32);
                        hb_in2[pos].range((l + 1) * 32 * chWord - 1, l * 32 * chWord) =
                            dataReg.range(((j % 4) + 1) * chWord * 32 - 1, (j % 4) * chWord * 32);
                        hb_in3[pos].range((l + 1) * 32 * chWord - 1, l * 32 * chWord) =
                            dataReg.range(((j % 4) + 1) * chWord * 32 - 1, (j % 4) * chWord * 32);
                        hb_in4[pos].range((l + 1) * 32 * chWord - 1, l * 32 * chWord) =
                            dataReg.range(((j % 4) + 1) * chWord * 32 - 1, (j % 4) * chWord * 32);
                    } else {
                        std::cout << "SUB_GRP_SZ too small, not supported" << std::endl;
                    }
                }
            }
        }
    }

    std::cout << "Host map buffer has been allocated and set.\n";

    // Get CL devices.
    std::vector<cl::Device> devices = xcl::get_xil_devices();
    cl::Device device = devices[0];

    // Create context and command queue for selected device
    cl::Context context(device);
    cl::CommandQueue q(context, device, CL_QUEUE_PROFILING_ENABLE | CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE);
    std::string devName = device.getInfo<CL_DEVICE_NAME>();
    std::cout << "Selected Device " << devName << "\n";

    cl::Program::Binaries xclBins = xcl::import_binary_file(xclbin_path);
    devices.resize(1);
    cl::Program program(context, devices, xclBins);

    cl::Kernel kernel0(program, "hmacSha1Kernel_1");
    cl::Kernel kernel1(program, "hmacSha1Kernel_2");
    cl::Kernel kernel2(program, "hmacSha1Kernel_3");
    cl::Kernel kernel3(program, "hmacSha1Kernel_4");
    std::cout << "Kernel has been created.\n";

    cl_mem_ext_ptr_t mext_in[4];
    mext_in[0] = {XCL_MEM_DDR_BANK0, hb_in1, 0};
    mext_in[1] = {XCL_MEM_DDR_BANK1, hb_in2, 0};
    mext_in[2] = {XCL_MEM_DDR_BANK2, hb_in3, 0};
    mext_in[3] = {XCL_MEM_DDR_BANK3, hb_in4, 0};

    cl_mem_ext_ptr_t mext_out_a[4];
    mext_out_a[0] = {XCL_MEM_DDR_BANK0, hb_out_a[0], 0};
    mext_out_a[1] = {XCL_MEM_DDR_BANK1, hb_out_a[1], 0};
    mext_out_a[2] = {XCL_MEM_DDR_BANK2, hb_out_a[2], 0};
    mext_out_a[3] = {XCL_MEM_DDR_BANK3, hb_out_a[3], 0};

    cl_mem_ext_ptr_t mext_out_b[4];
    mext_out_b[0] = {XCL_MEM_DDR_BANK0, hb_out_b[0], 0};
    mext_out_b[1] = {XCL_MEM_DDR_BANK1, hb_out_b[1], 0};
    mext_out_b[2] = {XCL_MEM_DDR_BANK2, hb_out_b[2], 0};
    mext_out_b[3] = {XCL_MEM_DDR_BANK3, hb_out_b[3], 0};

    // ping buffer
    cl::Buffer in_buff_a[4];
    cl::Buffer out_buff_a[4];
    // pong buffer
    cl::Buffer in_buff_b[4];
    cl::Buffer out_buff_b[4];

    // Map buffers
    for (int i = 0; i < 4; i++) {
        in_buff_a[i] = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY,
                                  (size_t)(sizeof(ap_uint<512>) * (N_ROW * N_TASK * CH_NM / 4 + CH_NM)), &mext_in[i]);
        out_buff_a[i] = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_WRITE_ONLY,
                                   (size_t)(sizeof(ap_uint<512>) * (N_TASK * CH_NM)), &mext_out_a[i]);
        in_buff_b[i] = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY,
                                  (size_t)(sizeof(ap_uint<512>) * (N_ROW * N_TASK * CH_NM / 4 + CH_NM)), &mext_in[i]);
        out_buff_b[i] = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_WRITE_ONLY,
                                   (size_t)(sizeof(ap_uint<512>) * (N_TASK * CH_NM)), &mext_out_b[i]);
    }

    std::cout << "DDR buffers have been mapped/copy-and-mapped\n";

    q.finish();

    std::cout << "Here A" << std::endl;

    struct timeval start_time, end_time;
    gettimeofday(&start_time, 0);

    std::vector<std::vector<cl::Event> > write_events(num_rep);
    std::vector<std::vector<cl::Event> > kernel_events(num_rep);
    std::vector<std::vector<cl::Event> > read_events(num_rep);
    for (int i = 0; i < num_rep; i++) {
        write_events[i].resize(1);
        kernel_events[i].resize(4);
        read_events[i].resize(1);
    }
    std::cout << "num_rep " << num_rep << std::endl;
    std::cout << "Here B" << std::endl;
    /*
     * W0-. W1----.     W2-.     W3-.
     *    '-K0--. '-K1-/-. '-K2-/-. '-K3---.
     *          '---R0-  '---R1-  '---R2   '--R3
     */

    std::cout << "Here C" << std::endl;
    for (int i = 0; i < num_rep; i++) {
        int use_a = i & 1;

        // write data to DDR
        std::vector<cl::Memory> ib;
        if (use_a) {
            ib.push_back(in_buff_a[0]);
            ib.push_back(in_buff_a[1]);
            ib.push_back(in_buff_a[2]);
            ib.push_back(in_buff_a[3]);
        } else {
            ib.push_back(in_buff_b[0]);
            ib.push_back(in_buff_b[1]);
            ib.push_back(in_buff_b[2]);
            ib.push_back(in_buff_b[3]);
        }

        if (i > 1) {
            q.enqueueMigrateMemObjects(ib, 0, &read_events[i - 2], &write_events[i][0]);
        } else {
            q.enqueueMigrateMemObjects(ib, 0, nullptr, &write_events[i][0]);
        }

        q.finish();
        std::cout << "Here C - 1" << std::endl;
        // set args and enqueue kernel
        if (use_a) {
            int j = 0;
            kernel0.setArg(j++, in_buff_a[0]);
            kernel0.setArg(j++, out_buff_a[0]);
            j = 0;
            kernel1.setArg(j++, in_buff_a[1]);
            kernel1.setArg(j++, out_buff_a[1]);
            j = 0;
            kernel2.setArg(j++, in_buff_a[2]);
            kernel2.setArg(j++, out_buff_a[2]);
            j = 0;
            kernel3.setArg(j++, in_buff_a[3]);
            kernel3.setArg(j++, out_buff_a[3]);
        } else {
            int j = 0;
            kernel0.setArg(j++, in_buff_b[0]);
            kernel0.setArg(j++, out_buff_b[0]);
            j = 0;
            kernel1.setArg(j++, in_buff_b[1]);
            kernel1.setArg(j++, out_buff_b[1]);
            j = 0;
            kernel2.setArg(j++, in_buff_b[2]);
            kernel2.setArg(j++, out_buff_b[2]);
            j = 0;
            kernel3.setArg(j++, in_buff_b[3]);
            kernel3.setArg(j++, out_buff_b[3]);
        }

        q.enqueueTask(kernel0, &write_events[i], &kernel_events[i][0]);
        q.finish();
        std::cout << "Here C - 2" << std::endl;

        q.enqueueTask(kernel1, &write_events[i], &kernel_events[i][1]);
        q.finish();
        std::cout << "Here C - 3" << std::endl;

        q.enqueueTask(kernel2, &write_events[i], &kernel_events[i][2]);
        q.finish();
        std::cout << "Here C - 4" << std::endl;

        q.enqueueTask(kernel3, &write_events[i], &kernel_events[i][3]);
        q.finish();
        std::cout << "Here C - 5" << std::endl;

        // read data from DDR
        std::vector<cl::Memory> ob;
        if (use_a) {
            ob.push_back(out_buff_a[0]);
            ob.push_back(out_buff_a[1]);
            ob.push_back(out_buff_a[2]);
            ob.push_back(out_buff_a[3]);
        } else {
            ob.push_back(out_buff_b[0]);
            ob.push_back(out_buff_b[1]);
            ob.push_back(out_buff_b[2]);
            ob.push_back(out_buff_b[3]);
        }
        q.enqueueMigrateMemObjects(ob, CL_MIGRATE_MEM_OBJECT_HOST, &kernel_events[i], &read_events[i][0]);
        q.finish();
        std::cout << "Here C - 6" << std::endl;
    }

    std::cout << "Here D" << std::endl;
    // wait all to finish
    // q.flush();
    std::cout << "Here D - 1" << std::endl;
    q.finish();
    std::cout << "Here D - 2" << std::endl;
    gettimeofday(&end_time, 0);
    std::cout << "Here D - 3" << std::endl;

    // check result
    bool checked = true;
    // check ping buffer
    std::cout << "check ping buffer" << std::endl;
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < N_TASK; j++) {
            for (int k = 0; k < CH_NM; k++) {
                if (hb_out_a[i][j * CH_NM + k] != golden) {
                    checked = false;
                    std::cout << std::dec << i << "th kernel " << j << "th message " << k
                              << "th channel's result is wrong" << std::endl;
                    std::cout << std::hex << "golden: " << golden << std::endl;
                    std::cout << std::hex << "result: " << hb_out_a[i][j * CH_NM + k] << std::endl;
                }
            }
        }
    }
    // check pong buffer
    std::cout << "check pong buffer" << std::endl;
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < N_TASK; j++) {
            for (int k = 0; k < CH_NM; k++) {
                if (hb_out_b[i][j * CH_NM + k] != golden) {
                    checked = false;
                    std::cout << std::dec << i << "th kernel " << j << "th message " << k
                              << "th channel's result is wrong" << std::endl;
                    std::cout << std::hex << "golden: " << golden << std::endl;
                    std::cout << std::hex << "result: " << hb_out_b[i][j * CH_NM + k] << std::endl;
                }
            }
        }
    }
    // final output
    std::cout << std::dec << std::endl;
    if (checked) {
        std::cout << std::dec << CH_NM << " channels, " << N_TASK << " tasks, " << N_ROW
                  << " messages verified. No error found!" << std::endl;
    }

    std::cout << "Kernel has been run for " << std::dec << num_rep << " times." << std::endl;
    std::cout << "Total execution time " << tvdiff(&start_time, &end_time) << "us" << std::endl;

    return 0;
}
