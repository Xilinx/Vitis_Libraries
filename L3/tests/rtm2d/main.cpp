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

#include <chrono>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <vector>
#include <omp.h>

#include "binFiles.hpp"
#include "types.hpp"
#include "utils.hpp"
#include "fpga_xrt.hpp"

using namespace std;

typedef WideData<RTM_dataType, RTM_nPE> RTM_wideType;
typedef WideData<RTM_wideType, 2> RTM_pairType;

double execute(xclDeviceHandle m_handle) {
    auto start = chrono::high_resolution_clock::now();
#pragma omp parallel
    {
        omp_set_dynamic(0);
        omp_set_num_threads(2);
#pragma omp for
        for (int i = 0; i < 2; i++) {
            while (xclExecWait(m_handle, 1) == 0)
                ;
        }
    }

    auto finish = chrono::high_resolution_clock::now();
    chrono::duration<double> elapsed = finish - start;
    return elapsed.count();
}

int main(int argc, char** argv) {
    const int requiredArg = 8;
    vector<string> args(argv + 1, argv + argc);
    if (args.size() < requiredArg) return EXIT_FAILURE;

    unsigned int argId = 0;

    string l_xclbinFile(args[argId++]);
    int l_height = atoi(args[argId++].c_str());
    int l_width = atoi(args[argId++].c_str());
    int l_time = atoi(args[argId++].c_str());
    int l_shot = atoi(args[argId++].c_str());
    int l_fsx = atoi(args[argId++].c_str());
    int l_sp = atoi(args[argId++].c_str());
    string filePath = args[argId++];

    bool l_verify = false;
    if (args.size() > argId) l_verify = atoi(args[argId++].c_str()) == 0 ? false : true;

    unsigned int l_deviceId = 0;
    if (args.size() > argId) {
        l_deviceId = atoi(args[argId++].c_str());
    }

    int l_imgX = l_width - 2 * RTM_NXB;
    int l_imgZ = l_height - 2 * RTM_NZB;
    int l_area = l_width * l_height / RTM_nPE;

    int err0 = 0, err1 = 0, err2 = 0, err3 = 0, err4 = 0, err5 = 0, err6 = 0, err7 = 0;
    bool pass1, pass2, pass3, pass4, pass5, pass6, pass;

    assert(l_height <= RTM_maxDim);
    assert(l_time % RTM_numFSMs == 0);
    assert(l_time % RTM_numBSMs == 0);
    assert(l_width * l_height % RTM_parEntries == 0);

    AlignMem<RTM_pairType>* p_snaps = new AlignMem<RTM_pairType>[ l_shot ];
    AlignMem<RTM_pairType>* p_ps = new AlignMem<RTM_pairType>[ l_shot ];
    AlignMem<RTM_pairType>* p_rs = new AlignMem<RTM_pairType>[ l_shot ];
    AlignMem<RTM_dataType>* p_upbs = new AlignMem<RTM_dataType>[ l_shot ];
    AlignMem<RTM_dataType> p_i;
    vector<RTM_dataType> p, pp, bp, bpp, pr, rr, snap0, snap1, pref, ppref, rref, ref, imlocref;

    p.resize(l_width * l_height);
    pp.resize(l_width * l_height);
    bp.resize(l_width * l_height);
    bpp.resize(l_width * l_height);
    rr.resize(l_width * l_height);
    pr.resize(l_width * l_height);

    const char* l_xclbinName = l_xclbinFile.c_str();
    int* err = nullptr;

    FPGA fpga(l_xclbinName, err, l_deviceId);
    ForwardKernel<RTM_dataType, RTM_order, RTM_nPE> fwd(&fpga, l_height, l_width, RTM_NZB, RTM_NXB, l_time, l_shot);
    BackwardKernel<RTM_dataType, RTM_order, RTM_nPE> bwd(&fpga, l_height, l_width, RTM_NZB, RTM_NXB, l_time, l_shot);

    fwd.loadData(filePath);
    bwd.loadData(filePath);
    double elapsedF = 0, elapsedB = 0, elapsedTotal = 0;

    AlignMem<RTM_pairType> p_snap0;
    AlignMem<RTM_dataType> p_upb0;

    auto start = chrono::high_resolution_clock::now();

    elapsedF = fwd.run(0, l_fsx, p_snap0, p_upb0);

    p_snaps[0] = p_snap0;
    p_upbs[0] = p_upb0;

    for (int s = 0; s < l_shot - 1; s++) {
        AlignMem<RTM_pairType> l_p0_forward(l_width * l_height / RTM_nPE);
        AlignMem<RTM_dataType> l_upb(l_width * RTM_order * l_time / 2);

        AlignMem<RTM_pairType> l_p0_backward(l_width * l_height / RTM_nPE);
        AlignMem<RTM_pairType> l_r0(l_width * l_height / RTM_nPE);
        AlignMem<RTM_dataType> l_i0 = p_i;
        if (l_i0.size() == 0) l_i0.alloc(l_imgX * l_imgZ);

        AlignMem<RTM_pairType> p_snap, p_p, p_r;
        AlignMem<RTM_dataType> p_upb;

        bwd.before_run(s, p_snaps[s], p_upbs[s], p_p, p_r, p_i, l_p0_backward, l_r0, l_i0);
        fwd.before_run(s + 1, l_fsx + l_sp * (s + 1), l_p0_forward, l_upb);

        elapsedTotal += execute(fpga.m_handle);

        bwd.after_run(p_p, p_r, p_i, l_p0_backward, l_r0, l_i0);

        p_ps[s] = p_p;
        p_rs[s] = p_r;

        fwd.after_run(p_snap, p_upb, l_p0_forward, l_upb);

        p_snaps[s + 1] = p_snap;
        p_upbs[s + 1] = p_upb;
    }

    AlignMem<RTM_pairType> p_p_last, p_r_last;

    elapsedB = bwd.run(l_shot - 1, p_snaps[l_shot - 1], p_upbs[l_shot - 1], p_p_last, p_r_last, p_i);

    p_ps[l_shot - 1] = p_p_last;
    p_rs[l_shot - 1] = p_r_last;

    xclClose(fpga.m_handle);

    auto finish = chrono::high_resolution_clock::now();
    chrono::duration<double> elapsed = finish - start;

    for (int s = 0; s < l_shot; s++) {
        if (l_verify) {
            for (int i = 0; i < l_area; i++) {
                for (int pe = 0; pe < RTM_nPE; pe++) {
                    p[i * RTM_nPE + pe] = p_snaps[s][i][1][pe];
                    pp[i * RTM_nPE + pe] = p_snaps[s][i][0][pe];

                    bpp[i * RTM_nPE + pe] = p_ps[s][i][0][pe];
                    bp[i * RTM_nPE + pe] = p_ps[s][i][1][pe];
                    rr[i * RTM_nPE + pe] = p_rs[s][i][0][pe];
                    pr[i * RTM_nPE + pe] = p_rs[s][i][1][pe];
                }
            }

            readBin(filePath + "p0_s" + to_string(s) + ".bin", sizeof(float) * l_width * l_height, ppref);
            readBin(filePath + "p1_s" + to_string(s) + ".bin", sizeof(float) * l_width * l_height, pref);

            readBin(filePath + "r0_s" + to_string(s) + ".bin", sizeof(float) * l_width * l_height, rref);
            readBin(filePath + "r1_s" + to_string(s) + ".bin", sizeof(float) * l_width * l_height, ref);

            readBin(filePath + "imloc_s" + to_string(s) + ".bin", sizeof(float) * l_imgX * l_imgZ, imlocref);

            pass1 = compare<RTM_dataType>(l_width * l_height, bp.data(), pref.data(), err1);
            cout << "There are in total " << err1 << " errors in bp v.s. pref" << endl;

            pass2 = compare<RTM_dataType>(l_width * l_height, bpp.data(), ppref.data(), err2);
            cout << "There are in total " << err2 << " errors in bpp v.s. ppref" << endl;

            pass3 = compare<RTM_dataType>(l_width * l_height, pr.data(), ref.data(), err3);
            cout << "There are in total " << err3 << " errors in pr v.s. prref" << endl;

            pass4 = compare<RTM_dataType>(l_width * l_height, rr.data(), rref.data(), err4);
            cout << "There are in total " << err4 << " errors in rr v.s. pprref" << endl;

            readBin(filePath + "snap0_s" + to_string(s) + ".bin", sizeof(float) * l_width * l_height, snap0);
            readBin(filePath + "snap1_s" + to_string(s) + ".bin", sizeof(float) * l_width * l_height, snap1);

            pass5 = compare<RTM_dataType>(l_width * l_height, pp.data(), snap0.data(), err5);
            cout << "There are in total " << err5 << " errors in pp v.s. snap0" << endl;

            pass6 = compare<RTM_dataType>(l_width * l_height, p.data(), snap1.data(), err6);
            cout << "There are in total " << err6 << " errors in p v.s. snap1" << endl;

            if (pass1 && pass2 && pass3 && pass4 && pass5 && pass6) {
                cout << "Test passed!" << endl;
            } else {
                cout << "Test failed, there are in total " << err0 + err1 + err2 + err3 + err4 + err5 + err6
                     << " errors!" << endl;
                return EXIT_FAILURE;
            }
        }
    }
    if (l_verify) {
        readBin(filePath + "imloc.bin", sizeof(float) * l_imgX * l_imgZ, imlocref);
        pass = compare<RTM_dataType>(l_imgX * l_imgZ, p_i.ptr(), imlocref.data(), err7);
        cout << "There are in total " << err7 << " errors in img v.s. img_ref" << endl;
    } else {
        pass = true;
        cout << "WARNING: run without verification." << endl;
    }
    cout << "Execution completed" << endl;
    cout << "one forward execution time " << elapsedF << "s." << endl;
    cout << "one backward execution time " << elapsedB << "s." << endl;
    cout << "Average async execution time " << elapsedTotal / (l_shot - 1) << "s." << endl;
    cout << "Total execution time " << elapsed.count() << "s." << endl;

    writeBin(filePath + "imloc.bin", sizeof(float) * l_imgX * l_imgZ, p_i.ptr());

    delete[] p_snaps;
    delete[] p_ps;
    delete[] p_rs;
    delete[] p_upbs;

    return EXIT_SUCCESS;
}
