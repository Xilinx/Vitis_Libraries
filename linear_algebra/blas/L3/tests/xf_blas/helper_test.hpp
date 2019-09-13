/*
 * Copyright 2019 Xilinx, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef HELPER_TEST_HPP
#define HELPER_TEST_HPP

#include <string>
#include <iomanip>
#include <cmath>

using namespace std;

void readMatBin(
    char* mat, unsigned int index, unsigned int m, unsigned int n, string dataDir, string name, unsigned int eleSize) {
    ifstream inFile;
    inFile.open(dataDir + name + to_string(index) + "_" + to_string(m) + "_" + to_string(n) + ".bin", ifstream::binary);
    if (inFile.is_open()) {
        inFile.read((char*)mat, eleSize * m * n);
        inFile.close();
    } else {
        cerr << "Could not find "
             << (dataDir + name + to_string(index) + "_" + to_string(m) + "_" + to_string(n) + ".bin") << endl;
        exit(1);
    }
}

template <typename t_dataType>
bool compareMat(t_dataType* c, t_dataType* goldenC, int m, int n, float p_TolRel = 3e-3, float p_TolAbs = 1e-5) {
    bool l_check = true;
    cout << " Comparing matrix with size " << m << " * " << n << "\n";
    int mismatch = 0;
    for (int row = 0; row < m; row++) {
        for (int col = 0; col < n; col++) {
            t_dataType l_ref = goldenC[IDX2R(row, col, n)];
            t_dataType l_result = c[IDX2R(row, col, n)];
            float l_diffAbs = abs(l_ref - l_result);
            float l_diffRel = l_diffAbs;
            if (goldenC[IDX2R(row, col, n)] != 0) {
                l_diffRel /= abs(l_ref);
            }
            bool check = (l_diffRel <= p_TolRel) || (l_diffAbs <= p_TolAbs);
            if (!check) {
                cout << "row " << row << " col " << col << " golden result " << setprecision(10)
                     << goldenC[IDX2R(row, col, n)] << " is not equal to fpga result " << setprecision(10)
                     << c[IDX2R(row, col, n)] << " DifAbs " << l_diffAbs << " DifRel " << l_diffRel << "\n";
                mismatch++;
                l_check = false;
            }
        }
    }

    cout << " Compared " << m * n << " values mismatches " << mismatch << "\n";
    return l_check;
}

template <typename t_dataType>
bool compareVector(t_dataType* y, t_dataType* goldenY, int m, float p_TolRel = 1e-3, float p_TolAbs = 1e-5) {
    bool l_check = true;
    cout << " Comparing vector with size " << m << " * 1 \n";
    int mismatch = 0;
    for (int row = 0; row < m; row++) {
        t_dataType l_ref = goldenY[row];
        t_dataType l_result = y[row];
        float l_diffAbs = abs(l_ref - l_result);
        float l_diffRel = l_diffAbs;
        if (goldenY[row] != 0) {
            l_diffRel /= abs(l_ref);
        }
        bool check = (l_diffRel <= p_TolRel) || (l_diffAbs <= p_TolAbs);
        if (!check) {
            cout << "row" << row << " golden result " << setprecision(10) << goldenY[row]
                 << " is not equal to fpga result " << setprecision(10) << y[row] << " DifAbs " << l_diffAbs
                 << " DifRel " << l_diffRel << "\n";
            mismatch++;
            l_check = false;
        }
    }
    cout << " Compared " << m << " values mismatches " << mismatch << "\n";
    return l_check;
}

#endif
