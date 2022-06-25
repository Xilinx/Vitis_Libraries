/*
 * (c) Copyright 2022 Xilinx, Inc. All rights reserved.
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
 *
 */
#include "cmdlineparser.h"
#include "logger.h"
#include "smithwaterman.h"
#include <assert.h>
#include <iostream>
#include <memory>
#include <string>
#include <unistd.h>

using namespace std;
using namespace sda;
using namespace sda::utils;

#include "sw.h"

// pass cmd line options to select opencl device
int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cout << "Usage: " << argv[0] << " <XCLBIN File>" << std::endl;
        return EXIT_FAILURE;
    }

    std::string binaryFile = argv[1];

    LogInfo("\nXilinx Smith Waterman benchmark started");
    string strKernelFullPath = sda::GetApplicationPath() + "/";

    // parse commandline
    CmdLineParser parser;
    parser.addSwitch("--sample-file", "-f", "input sample file path", "ref.txt");
    parser.addSwitch("--platform-name", "-p", "OpenCl platform vendor name", "xilinx");
    parser.addSwitch("--device-name", "-d", "OpenCl device name", "fpga0");
    parser.addSwitch("--kernel-file", "-k", "OpenCl kernel file to use");
    parser.addSwitch("--select-device", "-s", "Select from multiple matched devices [0-based index]", "0");
    parser.addSwitch("--number-of-runs", "-n", "Number of times the kernel runs on the device to compute the average.",
                     "1");
    parser.addSwitch("--block-size", "-bz", "Number of samples in a block for SmithWaterman", "-1");
    parser.addSwitch("--number-of-blocks", "-nb", "Number of blocks of samples for SmithWaterman", "1");
    parser.addSwitch("--number-of-threads", "-nt", "Number of threads for striped SmitWaterman", "1");
    parser.addSwitch("--double-buffered", "-db", "Double buffred host to fpga communication(now working)", "0");
    parser.addSwitch("--verify-mode", "-vm", "Verify output of FPGA using precomputed ref.txt", "0");
    parser.addSwitch("--output", "-o", "results output file", "result.json");
    parser.setDefaultKey("--kernel-file");
    parser.parse(argc, argv);

    // read settings
    string strSampleFP = parser.value("sample-file");
    string strPlatformName = parser.value("platform-name");
    string strDeviceName = parser.value("device-name");
    string strKernelRelFP = parser.value("kernel-file");

    int nRuns = parser.value_to_int("number-of-runs");
    int nBlocks = parser.value_to_int("number-of-blocks");
    int blkSz = parser.value_to_int("block-size");

    auto NUMITER = xcl::is_emulation() ? 4 : 1024;

    if (blkSz < 1) {
        blkSz = NUMITER;
    }
    int doubleBuffered = parser.value_to_int("double-buffered");
    int idxSelectedDevice = parser.value_to_int("select-device");
    int verifyMode = parser.value_to_int("verify-mode");

    LogInfo("Platform: %s, Device: %s", strPlatformName.c_str(), strDeviceName.c_str());
    LogInfo("Kernel FP: %s", strKernelRelFP.c_str());

    // MatchInfo: fetch results and forwards them
    int totalSz = nBlocks * NUMPACKED * blkSz;

    const std::unique_ptr<MatchArray> pMatchInfo(new MatchArray(totalSz, MAXROW, MAXCOL));

    if (parser.isValid("kernel-file")) {
        strKernelFullPath += parser.value("kernel-file");
    }
    if (verifyMode) {
        if (!is_file(parser.value("sample-file"))) {
            LogError("Input sample file: %s does not exist!", parser.value("sample-file").c_str());
            return -1;
        }
    }

    SmithWatermanApp smithwaterman(strPlatformName, strDeviceName, idxSelectedDevice, strKernelFullPath, strSampleFP,
                                   binaryFile, nBlocks, blkSz, doubleBuffered == 0 ? false : true,
                                   verifyMode == 0 ? false : true, pMatchInfo.get());

    // SWAN-HLS Xilinx SDAccel flow
    bool res = smithwaterman.run(0, nRuns);
    if (!res) {
        LogError("An error occurred when running benchmark on device 0");
        return -1;
    }

    LogInfo("finished");

    return 0;
}
