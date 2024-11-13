/*
 * Copyright (C) 2019-2022, Xilinx, Inc.
 * Copyright (C) 2022-2023, Advanced Micro Devices, Inc.
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

#include <iostream>
#include <fstream>
using namespace std;

void printOutputMatrix32(std::string file0, std::string file1, int numRow, int numCol) {
    ifstream fd0, fd1;
    char c = 'T';
    string str0, str1;
    fd0.open(file0, ios::in);
    fd1.open(file1, ios::in);
    std::complex<float> X[numRow][numCol];
    for (unsigned int j = 0; j < numCol; j++) {
        for (unsigned int i = 0; i < numRow; i++) {
            while (getline(fd0, str0, '\n')) {
                if (str0.front() != c) {
                    break;
                }
            }
            std::stringstream stream0(str0);
            float data0R;
            stream0 >> data0R;

            X[i][j].real(data0R);

            while (getline(fd1, str1, '\n')) {
                if (str1.front() != c) {
                    break;
                }
            }

            std::stringstream stream1(str1);
            float data0I;
            stream1 >> data0I;

            X[i][j].imag(data0I);
        }
    }
    std::cout << "Matrix_Output[" << numRow << " x " << numCol << "]" << std::endl;
    for (unsigned int i = 0; i < numRow; i++) {
        std::cout << "Row[" << i << "]: ";
        for (unsigned int j = 0; j < numCol; j++) {
            std::cout << "(" << X[i][j].real() << ", " << X[i][j].imag() << "), ";
        }
        std::cout << std::endl;
    }
    std::cout << std::endl;
}
void printOutputMatrix128(std::string file0, std::string file1, int numRow, int numCol) {
    ifstream fd0, fd1;
    char c = 'T';
    string str0, str1;
    fd0.open(file0, ios::in);
    fd1.open(file1, ios::in);
    std::complex<float> X[numRow][numCol];
    for (unsigned int j = 0; j < numCol; j++) {
        for (unsigned int i = 0; i < numRow; i+=4) {
            while (getline(fd0, str0, '\n')) {
                if (str0.front() != c) {
                    break;
                }
            }
            std::stringstream stream0(str0);
            float data0R;
            float data1R;
            float data2R;
            float data3R;
            stream0 >> data0R;
            stream0 >> data1R;
            stream0 >> data2R;
            stream0 >> data3R;

            X[i+0][j].real(data0R);
            X[i+1][j].real(data1R);
            X[i+2][j].real(data2R);
            X[i+3][j].real(data3R);

            while (getline(fd1, str1, '\n')) {
                if (str1.front() != c) {
                    break;
                }
            }

            std::stringstream stream1(str1);
            float data0I;
            float data1I;
            float data2I;
            float data3I;
            stream1 >> data0I;
            stream1 >> data1I;
            stream1 >> data2I;
            stream1 >> data3I;

            X[i+0][j].imag(data0I);
            X[i+1][j].imag(data1I);
            X[i+2][j].imag(data2I);
            X[i+3][j].imag(data3I);
        }
    }
    std::cout << "Matrix_Output[" << numRow << " x " << numCol << "]" << std::endl;
    for (unsigned int i = 0; i < numRow; i++) {
        std::cout << "Row[" << i << "]: ";
        for (unsigned int j = 0; j < numCol; j++) {
            std::cout << "(" << X[i][j].real() << ", " << X[i][j].imag() << "), ";
        }
        std::cout << std::endl;
    }
    std::cout << std::endl;
}
void printTriangularMatrix(std::string file0, std::string file1, int numRow, int numCol) {
    ifstream fd0, fd1;
    char c = 'T';
    string str0, str1;
    fd0.open(file0, ios::in);
    fd1.open(file1, ios::in);
    std::complex<float> X[numRow][numCol];
    for (unsigned int j = 0; j < numCol; j++) {
        for (unsigned int i = 0; i < j + 1; i++) {
            while (getline(fd0, str0, '\n')) {
                if (str0.front() != c) {
                    break;
                }
            }
            std::stringstream stream0(str0);
            float data0R;
            stream0 >> data0R;

            X[i][j].real(data0R);

            while (getline(fd1, str1, '\n')) {
                if (str1.front() != c) {
                    break;
                }
            }

            std::stringstream stream1(str1);
            float data0I;
            stream1 >> data0I;

            X[i][j].imag(data0I);
        }
    }
    std::cout << "Matrix[" << numRow << " x " << numCol << "]" << std::endl;
    for (unsigned int i = 0; i < numRow; i++) {
        std::cout << "Row[" << i << "]: ";
        for (unsigned int j = 0; j < i; j++) {
            std::cout << "(" << 0.0 << ", " << 0.0 << "), ";
        }
        for (unsigned int j = i; j < numCol; j++) {
            std::cout << "(" << X[i][j].real() << ", " << X[i][j].imag() << "), ";
        }
        std::cout << std::endl;
    }
    std::cout << std::endl;
}
void printComplexMatrix128(std::string file0, std::string file1, int numRow, int numCol, int numRow1, int numCol1) {
    ifstream fd0, fd1;
    char c = 'T';
    string str0, str1;
    fd0.open(file0, ios::in);
    fd1.open(file1, ios::in);
    std::complex<float> R[numRow][numCol];
    std::complex<float> B[numRow1][numCol1];
    for (unsigned int j = 0; j < numCol; j++) {
        for (unsigned int i = 0; i < numRow; i += 4) {
            while (getline(fd0, str0, '\n')) {
                if (str0.front() != c) {
                    break;
                }
            }
            std::stringstream stream0(str0);
            float data0R, data0I;
            float data1R, data1I;
            stream0 >> data0R;
            stream0 >> data0I;
            stream0 >> data1R;
            stream0 >> data1I;

            R[i + 0][j].real(data0R);
            R[i + 0][j].imag(data0I);
            R[i + 1][j].real(data1R);
            R[i + 1][j].imag(data1I);

            while (getline(fd1, str1, '\n')) {
                if (str1.front() != c) {
                    break;
                }
            }

            std::stringstream stream1(str1);
            float data2R, data2I;
            float data3R, data3I;
            stream1 >> data2R;
            stream1 >> data2I;
            stream1 >> data3R;
            stream1 >> data3I;

            R[i + 2][j].real(data2R);
            R[i + 2][j].imag(data2I);
            R[i + 3][j].real(data3R);
            R[i + 3][j].imag(data3I);
        }
    }
    for (unsigned int j = 0; j < numCol1; j++) {
        for (unsigned int i = 0; i < numRow1; i += 4) {
            while (getline(fd0, str0, '\n')) {
                if (str0.front() != c) {
                    break;
                }
            }
            std::stringstream stream0(str0);
            float data0R, data0I;
            float data1R, data1I;
            stream0 >> data0R;
            stream0 >> data0I;
            stream0 >> data1R;
            stream0 >> data1I;

            B[i + 0][j].real(data0R);
            B[i + 0][j].imag(data0I);
            B[i + 1][j].real(data1R);
            B[i + 1][j].imag(data1I);

            while (getline(fd1, str1, '\n')) {
                if (str1.front() != c) {
                    break;
                }
            }

            std::stringstream stream1(str1);
            float data2R, data2I;
            float data3R, data3I;
            stream1 >> data2R;
            stream1 >> data2I;
            stream1 >> data3R;
            stream1 >> data3I;

            B[i + 2][j].real(data2R);
            B[i + 2][j].imag(data2I);
            B[i + 3][j].real(data3R);
            B[i + 3][j].imag(data3I);
        }
    }

    std::cout << "Matrix_0[" << numRow << " x " << numCol << "]" << std::endl;
    for (unsigned int i = 0; i < numRow; i++) {
        std::cout << "Row[" << i << "]: ";
        for (unsigned int j = 0; j < numCol; j++) {
            std::cout << "(" << R[i][j].real() << ", " << R[i][j].imag() << "), ";
        }
        std::cout << std::endl;
    }
    std::cout << std::endl;
    std::cout << "Matrix_1[" << numRow1 << " x " << numCol1 << "]" << std::endl;
    for (unsigned int i = 0; i < numRow1; i++) {
        std::cout << "Row[" << i << "]: ";
        for (unsigned int j = 0; j < numCol1; j++) {
            std::cout << "(" << B[i][j].real() << ", " << B[i][j].imag() << "), ";
        }
        std::cout << std::endl;
    }
    std::cout << std::endl;
}

int compare(float out, float gld) {
    if (1 == isnan(out)) {
        std::cout << "out is abnorman, out=" << out << std::endl;
        return 1;
    } else {
        if (abs(gld) > 1.0) {
            if ((abs(out - gld) / abs(gld)) > 0.01) {
                return 1;
            }
        } else {
            if (abs(out - gld) > 0.01) {
                return 1;
            }
        }
    }
    return 0;
}

int verify_output(std::string output_file0,
                  std::string output_file1,
                  std::string golden_file0,
                  std::string golden_file1,
                  int numRow,
                  int numCol) {
    int numErr = 0;
    ifstream f_gld0, f_gld1;
    ifstream f_out0, f_out1;
    f_gld0.open(golden_file0, ios::in);
    f_gld1.open(golden_file1, ios::in);
    f_out0.open(output_file0, ios::in);
    f_out1.open(output_file1, ios::in);

    char c = 'T';
    string str_out0, str_out1;
    string str_gld0, str_gld1;
    int num = numCol * numRow;
    int num4 = numCol / 4;

    for (int j = 0; j < numCol; j++) {
        for (int i = 0; i < numRow; i += 4) {
            while (getline(f_out0, str_out0, '\n')) {
                if (str_out0.front() != c) {
                    break;
                }
            }
            std::stringstream strOut0(str_out0);
            float dat0[4];
            for (int k = 0; k < 4; k++) {
                strOut0 >> dat0[k];
            }
            float gld0[4];
            for (int k = 0; k < 4; k++) {
                while (getline(f_gld0, str_gld0, '\n')) {
                    if (str_out0.front() != c) {
                        break;
                    }
                }
                std::stringstream strGld0(str_gld0);
                strGld0 >> gld0[k];
            }
            while (getline(f_out1, str_out1, '\n')) {
                if (str_out1.front() != c) {
                    break;
                }
            }
            std::stringstream strOut1(str_out1);
            float dat1[4];
            for (int k = 0; k < 4; k++) {
                strOut1 >> dat1[k];
            }
            float gld1[4];
            for (int k = 0; k < 4; k++) {
                while (getline(f_gld1, str_gld1, '\n')) {
                    if (str_out1.front() != c) {
                        break;
                    }
                }
                std::stringstream strGld1(str_gld1);
                strGld1 >> gld1[k];
            }
            if (i + 4 < numRow) {
                for (int l = 0; l < 4; l++) {
                    float outR = dat0[l];
                    float outI = dat1[l];
                    float gldR = gld0[l];
                    float gldI = gld1[l];
                    int idCol = j;
                    int idRow = i + l;
                    if ((0 != compare(outR, gldR)) || (0 != compare(outI, gldI))) {
                        numErr++;
                        std::cout << "numErr=" << numErr << ", Mat[" << idRow << "][" << idCol << "] mis-matched"
                                  << ", out=(" << outR << ", " << outI << "), gld=(" << gldR << ", " << gldI << ")"
                                  << std::endl;
                    }
                }
            } else {
                for (int l = 0; l < numRow - i; l++) {
                    float outR = dat0[l];
                    float outI = dat1[l];
                    float gldR = gld0[l];
                    float gldI = gld1[l];
                    int idCol = j;
                    int idRow = i + l;
                    if ((0 != compare(outR, gldR)) || (0 != compare(outI, gldI))) {
                        numErr++;
                        std::cout << "numErr=" << numErr << ", Mat[" << idRow << "][" << idCol << "] mis-matched"
                                  << ", out=(" << outR << ", " << outI << "), gld=(" << gldR << ", " << gldI << ")"
                                  << std::endl;
                    }
                }
            }
        } // end_for_i
    }     // end_for_j
    return numErr;
}
int golden_check(std::string output_file0,
                 std::string output_file1,
                 std::string golden_file0,
                 std::string golden_file1,
                 int numRow,
                 int numCol,
                 int numRow1,
                 int numCol1) {
    int numErr = 0;
    ifstream f_gld0, f_gld1;
    ifstream f_out0, f_out1;
    f_gld0.open(golden_file0, ios::in);
    f_gld1.open(golden_file1, ios::in);
    f_out0.open(output_file0, ios::in);
    f_out1.open(output_file1, ios::in);

    char c = 'T';
    string str_out0, str_out1;
    string str_gld0, str_gld1;
    int numR = numCol * numRow;
    int numB = numCol1 * numRow1;
    int num = (numR + numB);
    int num4 = num / 4;
    int numR4 = numR / 4;
    int numB4 = numB / 4;

    for (int j = 0; j < num4; j++) {
        while (getline(f_out0, str_out0, '\n')) {
            if (str_out0.front() != c) {
                break;
            }
        }
        std::stringstream strOut0(str_out0);
        float dat0[4];
        for (int k = 0; k < 4; k++) {
            strOut0 >> dat0[k];
        }
        while (getline(f_gld0, str_gld0, '\n')) {
            if (str_out0.front() != c) {
                break;
            }
        }
        std::stringstream strGld0(str_gld0);
        float gld0[4];
        for (int k = 0; k < 4; k++) {
            strGld0 >> gld0[k];
        }
        for (int l = 0; l < 2; l++) {
            float outR = dat0[l * 2];
            float outI = dat0[l * 2 + 1];
            float gldR = gld0[l * 2];
            float gldI = gld0[l * 2 + 1];
            if ((0 != compare(outR, gldR)) || (0 != compare(outI, gldI))) {
                numErr++;
                if (j < numR4) {
                    int idCol = (j * 4) / numRow;
                    int idRow = (j * 4) % numRow + l;
                    std::cout << "numErr=" << numErr << ", MatR[" << idRow << "][" << idCol << "] mis-matched"
                              << ", out=(" << outR << ", " << outI << "), gld=(" << gldR << ", " << gldI << ")"
                              << std::endl;
                } else {
                    int idCol = ((j - numR4) * 4) / numRow;
                    int idRow = ((j - numR4) * 4) % numRow + l;
                    std::cout << "numErr=" << numErr << ", MatB[" << idRow << "][" << idCol << "] mis-matched"
                              << ", out=(" << outR << ", " << outI << "), gld=(" << gldR << ", " << gldI << ")"
                              << std::endl;
                }
            }
        }
        while (getline(f_out1, str_out1, '\n')) {
            if (str_out1.front() != c) {
                break;
            }
        }
        std::stringstream strOut1(str_out1);
        float dat1[4];
        for (int k = 0; k < 4; k++) {
            strOut1 >> dat1[k];
        }
        while (getline(f_gld1, str_gld1, '\n')) {
            if (str_out1.front() != c) {
                break;
            }
        }
        std::stringstream strGld1(str_gld1);
        float gld1[4];
        for (int k = 0; k < 4; k++) {
            strGld1 >> gld1[k];
        }
        for (int l = 0; l < 2; l++) {
            float outR = dat1[l * 2];
            float outI = dat1[l * 2 + 1];
            float gldR = gld1[l * 2];
            float gldI = gld1[l * 2 + 1];
            if ((0 != compare(outR, gldR)) || (0 != compare(outI, gldI))) {
                numErr++;
                if (j < numR4) {
                    int idCol = (j * 4) / numRow;
                    int idRow = (j * 4) % numRow + l + 2;
                    std::cout << "numErr=" << numErr << ", MatR[" << idRow << "][" << idCol << "] mis-matched"
                              << ", out=(" << outR << ", " << outI << "), gld=(" << gldR << ", " << gldI << ")"
                              << std::endl;
                } else {
                    int idCol = ((j - numR4) * 4) / numRow;
                    int idRow = ((j - numR4) * 4) % numRow + l + 2;
                    std::cout << "numErr=" << numErr << ", MatB[" << idRow << "][" << idCol << "] mis-matched"
                              << ", out=(" << outR << ", " << outI << "), gld=(" << gldR << ", " << gldI << ")"
                              << std::endl;
                }
            }
        }
    }
    return numErr;
}
