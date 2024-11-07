/*
 * Copyright (C) 2019-2022, Xilinx, Inc.
 * Copyright (C) 2022-2024, Advanced Micro Devices, Inc.
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
#include <cstdlib>
#include <cmath>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <iostream>
#include <sstream>

#include <unistd.h>
#include <xrt/xrt_device.h>
#include <xrt/xrt_kernel.h>

#include <experimental/xrt_aie.h>
#include <experimental/xrt_graph.h>
#include <experimental/xrt_ip.h>

#define Q(x) #x
#define QUOTE(x) Q(x)

template <unsigned int TP_POINT_SIZE>
static constexpr unsigned int fnPtSizeD1() {
    unsigned int sqrtVal =
        TP_POINT_SIZE == 65536
            ? 256
            : TP_POINT_SIZE == 32768
                  ? 128
                  : TP_POINT_SIZE == 16384
                        ? 128
                        : TP_POINT_SIZE == 8192
                              ? 64
                              : TP_POINT_SIZE == 4096
                                    ? 64
                                    : TP_POINT_SIZE == 2048
                                          ? 32
                                          : TP_POINT_SIZE == 1024
                                                ? 32
                                                : TP_POINT_SIZE == 512
                                                      ? 16
                                                      : TP_POINT_SIZE == 256
                                                            ? 16
                                                            : TP_POINT_SIZE == 128
                                                                  ? 8
                                                                  : TP_POINT_SIZE == 64
                                                                        ? 8
                                                                        : TP_POINT_SIZE == 32
                                                                              ? 4
                                                                              : TP_POINT_SIZE == 16 ? 4 : 0;
    return sqrtVal;
}

template <unsigned int num, unsigned int rnd>
constexpr unsigned int fnCeil() {
    return (((num + rnd - 1) / rnd) * rnd);
}

static const char* STR_ERROR = "ERROR:   ";
static const char* STR_PASSED = "PASSED:  ";
static const char* STR_USAGE = "USAGE:   ";
static const char* STR_INFO = "INFO:    ";

// ------------------------------------------------------------
// DDR Parameters
// ------------------------------------------------------------

typedef int32_t TT_DATA; // Assume int32 data for I/O data (will be cint32)

static constexpr int32_t NUM_ITER = -1; // Let the graph run and have s2mm terminate things
static constexpr int32_t LOOP_CNT = NITER;
static constexpr int32_t LOOP_SEL = 0; // ID of loop to capture by DDR SNK PL HLS block
static constexpr unsigned NSTREAM = SSR;
static constexpr unsigned samplesPerRead = 2;

static constexpr unsigned ptSizeD1 = fnPtSizeD1<POINT_SIZE>();
static constexpr unsigned ptSizeD1Ceil = fnCeil<ptSizeD1, NSTREAM>();
static constexpr unsigned ptSizeD2 = POINT_SIZE / ptSizeD1;
static constexpr unsigned ptSizeD2Ceil = fnCeil<ptSizeD2, NSTREAM>();
static constexpr unsigned int DEPTH = ptSizeD2Ceil * ptSizeD1Ceil;

static constexpr unsigned DDR_WORD_DEPTH_I = ptSizeD2Ceil * ptSizeD1;
static constexpr unsigned DDR_WORD_DEPTH_O = ptSizeD2Ceil * ptSizeD1;
static constexpr unsigned NUM_SAMPLES_I = 2 * DDR_WORD_DEPTH_I; // 2 x 32-bit (int32) words for cint32 I/O
static constexpr unsigned NUM_SAMPLES_O = 2 * DDR_WORD_DEPTH_O; // 2 x 32-bit (int32) words for cint32 I/O

static constexpr unsigned DDR_BUFFSIZE_I_BYTES = NITER * NUM_SAMPLES_I * 4; // Each real/imag sample is 4 bytes 2*4096*4
static constexpr unsigned DDR_BUFFSIZE_O_BYTES = NITER * NUM_SAMPLES_O * 4; // Each real/imag sample is 4 bytes

// ------------------------------------------------------------
// Main
// ------------------------------------------------------------

int main(int argc, char* argv[]) {
    typedef typename std::conditional<(QUOTE(TT_DATA) == "cint32"), int32_t, float>::type real_dtype;
    // ------------------------------------------------------------
    // Load XCLBIN
    // ------------------------------------------------------------
    char xclbinFilename[] = "kernel.xclbin";
    unsigned dev_index = 0;
    auto my_device = xrt::device(dev_index);
    std::cout << STR_PASSED << "auto my_device = xrt::device(" << dev_index << ")" << std::endl;

    auto xclbin_uuid = my_device.load_xclbin(xclbinFilename);
    std::cout << STR_PASSED << "auto xclbin_uuid = my_device.load_xclbin(" << xclbinFilename << ")" << std::endl;

    auto my_graph = xrt::graph(my_device, xclbin_uuid, "fft_tb");
    std::cout << STR_PASSED << "auto my_graph  = xrt::graph(my_device, xclbin_uuid, \"fft_tb\")" << std::endl;

    my_graph.reset();
    std::cout << STR_PASSED << "my_graph.reset()" << std::endl;

    my_graph.run(NUM_ITER);
    std::cout << STR_PASSED << "my_graph.run( NUM_ITER=" << NUM_ITER << " )" << std::endl;

    // ------------------------------------------------------------
    // Load and Start DDR Source/Sink PL Kernels
    // ------------------------------------------------------------

    auto mm2s = xrt::kernel(my_device, xclbin_uuid, "mm2s_wrapper:{mm2s}");
    std::cout << STR_PASSED << "auto mm2s = xrt::kernel(my_device, xclbin_uuid, \"mm2s_wrapper:{mm2s}\")" << std::endl;

    auto s2mm = xrt::kernel(my_device, xclbin_uuid, "s2mm_wrapper:{s2mm}");
    std::cout << STR_PASSED << "auto s2mm = xrt::kernel(my_device, xclbin_uuid, \"s2mm_wrapper:{s2mm}\")" << std::endl;

    xrt::run s2mm_run = xrt::run(s2mm);
    std::cout << STR_PASSED << "xrt::run s2mm_run = xrt::run(s2mm_run)" << std::endl;

    xrt::run mm2s_run = xrt::run(mm2s);
    std::cout << STR_PASSED << "xrt::run mm2s = xrt::run(mm2s)" << std::endl;

    // ------------------------------------------------------------
    // Configure DDR Buffer Objects
    // ------------------------------------------------------------

    auto mm2s_bo = xrt::bo(my_device, DDR_BUFFSIZE_I_BYTES, mm2s.group_id(0));
    std::cout << STR_PASSED << "mm2s = xrt::bo(my_device, DDR_BUFFSIZE_I_BYTES, mm2s.group_id(0) (=" << mm2s.group_id(0)
              << "))" << std::endl;

    auto mm2s_bo_mapped = mm2s_bo.map<real_dtype*>();
    std::cout << STR_PASSED << "auto mm2s_bo_mapped = mm2s_bo.map<real_dtype*>()" << std::endl;

    auto s2mm_bo = xrt::bo(my_device, DDR_BUFFSIZE_O_BYTES, s2mm.group_id(0));
    std::cout << STR_PASSED
              << "s2mm_bo = xrt::bo(my_device, DDR_BUFFSIZE_O_BYTES, s2mm.group_id(0) (=" << s2mm.group_id(0) << "))"
              << std::endl;

    auto s2mm_bo_mapped = s2mm_bo.map<real_dtype*>();
    std::cout << STR_PASSED << "auto s2mm_bo_mapped = s2mm_bo.map<real_dtype*>()" << std::endl;

    // Open stimulus input file:
    std::ifstream ss_i;
    ss_i.open("input_front.txt", std::ifstream::in);
    if (ss_i.is_open() == 0) {
        std::cout << STR_ERROR << "failed to open input_front.txt" << std::endl;
        return (33);
    }

    // Read data from input file:

    for (unsigned ss = 0; ss < NUM_SAMPLES_I * NITER; ss += 2) {
        if ((ss % NUM_SAMPLES_I) < (POINT_SIZE * 2)) {
            real_dtype val_re, val_im;
            ss_i >> val_re >> val_im;
            mm2s_bo_mapped[ss] = val_re;
            mm2s_bo_mapped[ss + 1] = val_im;
        } else {
            mm2s_bo_mapped[ss] = 0;
            mm2s_bo_mapped[ss + 1] = 0;
        }
    }
    ss_i.close();
    std::cout << STR_PASSED << "Successfully read input file input_front.txt" << std::endl;

    // ------------------------------------------------------------
    // Load and start PL kernels
    // ------------------------------------------------------------

    mm2s_run.set_arg(0, mm2s_bo);
    std::cout << STR_PASSED << "mm2s_run.set_arg( 0, mm2s_bo )" << std::endl;

    s2mm_run.set_arg(0, s2mm_bo);
    std::cout << STR_PASSED << "s2mm.run.set_arg( 0, s2mm_bo )" << std::endl;

    mm2s_run.start();
    std::cout << STR_PASSED << "mm2s_run.start()" << std::endl;

    s2mm_run.start();
    std::cout << STR_PASSED << "s2mm_run.start()" << std::endl;

    s2mm_run.wait();
    std::cout << STR_PASSED << "s2mm_run.wait()" << std::endl;

    // ------------------------------------------------------------
    // Retrieve Results
    // ------------------------------------------------------------

    s2mm_bo.sync(XCL_BO_SYNC_BO_FROM_DEVICE);
    std::cout << STR_PASSED << "s2mm_bo.sync(XCL_BO_SYNC_BO_FROM_DEVICE)" << std::endl;

    mm2s_bo.sync(XCL_BO_SYNC_BO_FROM_DEVICE);
    std::cout << STR_PASSED << "mm2s_bo.sync(XCL_BO_SYNC_BO_FROM_DEVICE)" << std::endl;
    // Open GOLDEN results file:
    std::ifstream ss_o;

    ss_o.open("ref_output.txt", std::ifstream::in);
    if (ss_o.is_open() == 0) {
        std::cout << STR_ERROR << "failed to open ref_output.txt" << std::endl;
        return (34);
    }
    std::ofstream ss_a;
    ss_a.open("sig_a.txt", std::ofstream::out);
    if (ss_a.is_open() == 0) {
        std::cout << STR_ERROR << "failed to open sig_a.txt" << std::endl;
        return (35);
    }

    // Validate results:
    bool flag = 0;
    int level = (1 << 8);
    for (unsigned ss = 0; ss < NUM_SAMPLES_O * NITER; ss += 2) {
        if ((ss % NUM_SAMPLES_O) < (POINT_SIZE * 2)) {
            real_dtype val_g_re, val_g_im;
            ss_o >> val_g_re >> val_g_im;
            real_dtype val_a_re = s2mm_bo_mapped[ss];
            real_dtype val_a_im = s2mm_bo_mapped[ss + 1];
            ss_a << val_a_re << " " << val_a_im << std::endl;
            real_dtype err_re = abs(val_g_re - val_a_re);
            real_dtype err_im = abs(val_g_im - val_a_im);
            bool this_flag = (err_re > level) || (err_im > level); // Reference model is not bit accurate
            std::cout << "ss: " << (ss >> 1) << "  Gld: " << val_g_re << " " << val_g_im << "  Act: " << val_a_re << " "
                      << val_a_im << "  Err: " << err_re << " " << err_im;
            if (this_flag == 1) std::cout << " ***";
            std::cout << std::endl;
            flag |= this_flag;
        }
    }
    ss_a.close();

    std::cout << "Level: " << level << std::endl;

    // Done:
    if (flag == 0)
        std::cout << std::endl << "--- PASSED ---" << std::endl;
    else
        std::cout << std::endl << "*** FAILED ***" << std::endl;
    return (flag);
}
