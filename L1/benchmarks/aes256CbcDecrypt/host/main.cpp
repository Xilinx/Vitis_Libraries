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

#include <openssl/aes.h>
#include <openssl/evp.h>

#include <sys/time.h>
#include <new>
#include <cstdlib>

#include <xcl2.hpp>

// text length for each task in 128-bit
#define N_ROW 64
// number of tasks for a single PCIe block
#define N_TASK 2 // 8192
// number of PUs
#define CH_NM 4
// cipher key size in bytes
#define KEY_SIZE 32

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
        std::cout << "WARNING: ping-pong buffer should be updated at least 1 time.\n";
    }
    if (num_rep > 20) {
        num_rep = 20;
        std::cout << "WARNING: limited repeat to " << num_rep << " times.\n";
    }

    // input data
    const char datain[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
                           0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f};

    // cipher key
    const unsigned char key[] = {0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a,
                                 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25,
                                 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f};

    // initialization vector
    const unsigned char ivec[] = {0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
                                  0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f};

    // generate golden
    // ouput length of the result
    int outlen = 0;
    int plaintext_len = 0;
    // input data length 16 bytes
    unsigned int inlen = 16;
    // output result buffer
    unsigned char dout[N_ROW * inlen];

    // call OpenSSL API to get the golden
    EVP_CIPHER_CTX* ctx;
    ctx = EVP_CIPHER_CTX_new();
    EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, ivec);
    for (unsigned int i = 0; i < N_ROW; i++) {
        EVP_DecryptUpdate(ctx, dout + plaintext_len, &outlen, (const unsigned char*)datain, inlen);
        plaintext_len += outlen;
    }
    EVP_DecryptFinal_ex(ctx, dout + plaintext_len, &outlen);
    EVP_CIPHER_CTX_free(ctx);

    ap_uint<128> golden[N_ROW];
    for (unsigned int i = 0; i < N_ROW; i++) {
        for (unsigned int j = 0; j < 16; j++) {
            golden[i].range(j * 8 + 7, j * 8) = dout[i * 16 + j];
        }
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
    ap_uint<512>* hb_in = aligned_alloc<ap_uint<512> >(N_ROW * N_TASK * CH_NM / 4 + CH_NM);
    ap_uint<512>* hb_out_a = aligned_alloc<ap_uint<512> >(N_ROW * N_TASK * CH_NM / 4);
    ap_uint<512>* hb_out_b = aligned_alloc<ap_uint<512> >(N_ROW * N_TASK * CH_NM / 4);

    // generate configurations
    for (unsigned int j = 0; j < CH_NM; j++) {
        // massage length in 128-bit for each task
        hb_in[j].range(511, 448) = N_ROW;

        // number of tasks in a single PCIe block
        hb_in[j].range(447, 384) = N_TASK;

        // initialization vector
        hb_in[j].range(383, 256) = IVReg.range(127, 0);

        // cipherkey
        hb_in[j].range(255, 0) = keyReg.range(255, 0);
    }
    // generate texts
    for (unsigned int j = CH_NM; j < N_ROW * N_TASK * CH_NM / 4 + CH_NM; j++) {
        hb_in[j].range(511, 384) = dataReg.range(127, 0);
        hb_in[j].range(383, 256) = dataReg.range(127, 0);
        hb_in[j].range(255, 128) = dataReg.range(127, 0);
        hb_in[j].range(127, 0) = dataReg.range(127, 0);
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
    cl::Kernel kernel0(program, "aes256CbcDecryptKernel");
    std::cout << "Kernel has been created.\n";

#ifdef USE_DDR
    std::cout << "allocate to DDR" << std::endl;
    cl_mem_ext_ptr_t mext_in = {XCL_MEM_DDR_BANK0, hb_in, 0};
    cl_mem_ext_ptr_t mext_out_a = {XCL_MEM_DDR_BANK1, hb_out_a, 0};
    cl_mem_ext_ptr_t mext_out_b = {XCL_MEM_DDR_BANK1, hb_out_b, 0};
#else
    std::cout << "allocate to HBM" << std::endl;
    cl_mem_ext_ptr_t mext_in{0, hb_in, kernel0()};
    cl_mem_ext_ptr_t mext_out_a{1, hb_out_a, kernel0()};
    cl_mem_ext_ptr_t mext_out_b{1, hb_out_b, kernel0()};
#endif

    // Map buffers
    // ping buffer
    cl::Buffer buf_in_a(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY,
                        (size_t)(sizeof(ap_uint<512>) * (N_ROW * N_TASK * CH_NM / 4 + CH_NM)), &mext_in);

    cl::Buffer buf_out_a(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_WRITE_ONLY,
                         (size_t)(sizeof(ap_uint<512>) * (N_ROW * N_TASK * CH_NM / 4)), &mext_out_a);

    // pong buffer
    cl::Buffer buf_in_b(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY,
                        (size_t)(sizeof(ap_uint<512>) * (N_ROW * N_TASK * CH_NM / 4 + CH_NM)), &mext_in);

    cl::Buffer buf_out_b(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_WRITE_ONLY,
                         (size_t)(sizeof(ap_uint<512>) * (N_ROW * N_TASK * CH_NM / 4)), &mext_out_b);

    std::cout << "DDR buffers have been mapped/copy-and-mapped\n";

    q.finish();

    struct timeval start_time, end_time;
    gettimeofday(&start_time, 0);

    std::vector<std::vector<cl::Event> > write_events(num_rep);
    std::vector<std::vector<cl::Event> > kernel_events(num_rep);
    std::vector<std::vector<cl::Event> > read_events(num_rep);
    for (int i = 0; i < num_rep; i++) {
        write_events[i].resize(1);
        kernel_events[i].resize(1);
        read_events[i].resize(1);
    }

    /*
     * W0-. W1----.     W2-.     W3-.
     *    '-K0--. '-K1-/-. '-K2-/-. '-K3---.
     *          '---R0-  '---R1-  '---R2   '--R3
     */
    for (int i = 0; i < num_rep; i++) {
        int use_a = i & 1;

        // write data to DDR
        std::vector<cl::Memory> ib;
        if (use_a) {
            ib.push_back(buf_in_a);
        } else {
            ib.push_back(buf_in_b);
        }

        if (i > 1) {
            q.enqueueMigrateMemObjects(ib, 0, &read_events[i - 2], &write_events[i][0]);
        } else {
            q.enqueueMigrateMemObjects(ib, 0, nullptr, &write_events[i][0]);
        }

        // set args and enqueue kernel
        if (use_a) {
            int j = 0;
            kernel0.setArg(j++, buf_in_a);
            kernel0.setArg(j++, buf_out_a);
        } else {
            int j = 0;
            kernel0.setArg(j++, buf_in_b);
            kernel0.setArg(j++, buf_out_b);
        }
        q.enqueueTask(kernel0, &write_events[i], &kernel_events[i][0]);

        // read data from DDR
        std::vector<cl::Memory> ob;
        if (use_a) {
            ob.push_back(buf_out_a);
        } else {
            ob.push_back(buf_out_b);
        }
        q.enqueueMigrateMemObjects(ob, CL_MIGRATE_MEM_OBJECT_HOST, &kernel_events[i], &read_events[i][0]);
    }

    // wait all to finish
    q.flush();
    q.finish();
    gettimeofday(&end_time, 0);

    // check result
    bool checked = true;
    // check ping buffer
    for (unsigned int j = 0; j < N_TASK; j++) {
        for (unsigned int i = 0; i < N_ROW; i++) {
            for (unsigned int k = 0; k < CH_NM / 4; k++) {
                if (hb_out_a[j * N_ROW * CH_NM / 4 + i * CH_NM / 4 + k].range(511, 384) != golden[i]) {
                    checked = false;
                    std::cout << "Error found in " << std::dec << k + 3 << " channel, " << j << " task, " << i
                              << " message" << std::endl;
                    std::cout << "golden = " << std::hex << golden[i] << std::endl;
                    std::cout << "fpga   = " << std::hex
                              << hb_out_a[j * N_ROW * CH_NM / 4 + i * CH_NM / 4 + k].range(511, 384) << std::endl;
                }
                if (hb_out_a[j * N_ROW * CH_NM / 4 + i * CH_NM / 4 + k].range(383, 256) != golden[i]) {
                    checked = false;
                    std::cout << "Error found in " << std::dec << k + 2 << " channel, " << j << " task, " << i
                              << " message" << std::endl;
                    std::cout << "golden = " << std::hex << golden[i] << std::endl;
                    std::cout << "fpga   = " << std::hex
                              << hb_out_a[j * N_ROW * CH_NM / 4 + i * CH_NM / 4 + k].range(383, 256) << std::endl;
                }
                if (hb_out_a[j * N_ROW * CH_NM / 4 + i * CH_NM / 4 + k].range(255, 128) != golden[i]) {
                    checked = false;
                    std::cout << "Error found in " << std::dec << k + 1 << " channel, " << j << " task, " << i
                              << " message" << std::endl;
                    std::cout << "golden = " << std::hex << golden[i] << std::endl;
                    std::cout << "fpga   = " << std::hex
                              << hb_out_a[j * N_ROW * CH_NM / 4 + i * CH_NM / 4 + k].range(255, 128) << std::endl;
                }
                if (hb_out_a[j * N_ROW * CH_NM / 4 + i * CH_NM / 4 + k].range(127, 0) != golden[i]) {
                    checked = false;
                    std::cout << "Error found in " << std::dec << k << " channel, " << j << " task, " << i << " message"
                              << std::endl;
                    std::cout << "golden = " << std::hex << golden[i] << std::endl;
                    std::cout << "fpga   = " << std::hex
                              << hb_out_a[j * N_ROW * CH_NM / 4 + i * CH_NM / 4 + k].range(127, 0) << std::endl;
                }
            }
        }
    }
    // check pong buffer
    for (unsigned int j = 0; j < N_TASK; j++) {
        for (unsigned int i = 0; i < N_ROW; i++) {
            for (unsigned int k = 0; k < CH_NM / 4; k++) {
                if (hb_out_b[j * N_ROW * CH_NM / 4 + i * CH_NM / 4 + k].range(511, 384) != golden[i]) {
                    checked = false;
                    std::cout << "Error found in " << std::dec << k + 3 << " channel, " << j << " task, " << i
                              << " message" << std::endl;
                    std::cout << "golden = " << std::hex << golden[i] << std::endl;
                    std::cout << "fpga   = " << std::hex
                              << hb_out_b[j * N_ROW * CH_NM / 4 + i * CH_NM / 4 + k].range(511, 384) << std::endl;
                }
                if (hb_out_b[j * N_ROW * CH_NM / 4 + i * CH_NM / 4 + k].range(383, 256) != golden[i]) {
                    checked = false;
                    std::cout << "Error found in " << std::dec << k + 2 << " channel, " << j << " task, " << i
                              << " message" << std::endl;
                    std::cout << "golden = " << std::hex << golden[i] << std::endl;
                    std::cout << "fpga   = " << std::hex
                              << hb_out_b[j * N_ROW * CH_NM / 4 + i * CH_NM / 4 + k].range(383, 256) << std::endl;
                }
                if (hb_out_b[j * N_ROW * CH_NM / 4 + i * CH_NM / 4 + k].range(255, 128) != golden[i]) {
                    checked = false;
                    std::cout << "Error found in " << std::dec << k + 1 << " channel, " << j << " task, " << i
                              << " message" << std::endl;
                    std::cout << "golden = " << std::hex << golden[i] << std::endl;
                    std::cout << "fpga   = " << std::hex
                              << hb_out_b[j * N_ROW * CH_NM / 4 + i * CH_NM / 4 + k].range(255, 128) << std::endl;
                }
                if (hb_out_b[j * N_ROW * CH_NM / 4 + i * CH_NM / 4 + k].range(127, 0) != golden[i]) {
                    checked = false;
                    std::cout << "Error found in " << std::dec << k << " channel, " << j << " task, " << i << " message"
                              << std::endl;
                    std::cout << "golden = " << std::hex << golden[i] << std::endl;
                    std::cout << "fpga   = " << std::hex
                              << hb_out_b[j * N_ROW * CH_NM / 4 + i * CH_NM / 4 + k].range(127, 0) << std::endl;
                }
            }
        }
    }

    if (checked) {
        std::cout << std::dec << CH_NM << " channels, " << N_TASK << " tasks, " << N_ROW
                  << " messages verified. No error found!" << std::endl;
    }

    std::cout << "Kernel has been run for " << std::dec << num_rep << " times." << std::endl;
    std::cout << "Total execution time " << tvdiff(&start_time, &end_time) << "us" << std::endl;

    if (checked) {
        return 0;
    } else {
        return 1;
    }
}
