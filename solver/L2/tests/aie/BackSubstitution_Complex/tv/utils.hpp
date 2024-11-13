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
#include <string>
#include <cstring>
#include "matrix.hpp"

using namespace std; 
void printComplexMatrix128(std::string file0, std::string file1, int row0, int col0, int row1, int col1) {
    ifstream fd0, fd1;
    char c = 'T';
    string str0, str1;
    fd0.open(file0, ios::in);
    fd1.open(file1, ios::in);
    std::complex<float> R[row0][col0];
    std::complex<float> Q[row1][col1];
    for (unsigned int j = 0; j < col0; j++) {
        for (unsigned int i = 0; i < row0; i += 4) {
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
    for (unsigned int j = 0; j < col1; j++) {
        for (unsigned int i = 0; i < row1; i += 4) {
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

            Q[i + 0][j].real(data0R);
            Q[i + 0][j].imag(data0I);
            Q[i + 1][j].real(data1R);
            Q[i + 1][j].imag(data1I);

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

            Q[i + 2][j].real(data2R);
            Q[i + 2][j].imag(data2I);
            Q[i + 3][j].real(data3R);
            Q[i + 3][j].imag(data3I);
        }
    }

    std::cout << "Matrix_0[" << row0 << " x " << col0 << "]" << std::endl;
    for (unsigned int i = 0; i < row0; i++) {
        std::cout << "Row[" << i << "]: ";
        for (unsigned int j = 0; j < col0; j++) {
            std::cout << "(" << R[i][j].real() << ", " << R[i][j].imag() << "), ";
        }
        std::cout << std::endl;
    }
    std::cout << std::endl;
    std::cout << "Matrix_1[" << row1 << " x " << col1 << "]" << std::endl;
    for (unsigned int i = 0; i < row1; i++) {
        std::cout << "Row[" << i << "]: ";
        for (unsigned int j = 0; j < col1; j++) {
            std::cout << "(" << Q[i][j].real() << ", " << Q[i][j].imag() << "), ";
        }
        std::cout << std::endl;
    }
    std::cout << std::endl;
}

void printMatrix(std::string file0, std::string file1, int row0, int col0, int row1, int col1) {
    ifstream fd0, fd1;
    char c = 'T';
    string str0, str1;
    fd0.open(file0, ios::in);
    fd1.open(file1, ios::in);
    std::complex<float> R[row0][col0];
    std::complex<float> Q[row1][col1];
    // getline (fd0, str0, '\n');
    // getline (fd1, str1, '\n');
    for (unsigned int j = 0; j < col0; j++) {
        for (unsigned int i = 0; i < row0; i ++) {
            while (getline(fd0, str0, '\n')) {
                if (str0.front() != c) {
                    break;
                }
            }
            std::stringstream stream0(str0);
            float dataR;
            stream0 >> dataR;

            R[i][j].real(dataR);

            while (getline(fd1, str1, '\n')) {
                if (str1.front() != c) {
                    break;
                }
            }
            std::stringstream stream1(str1);
            float dataI;
            stream1 >> dataI;
            R[i][j].imag(dataI);

        }
    }
    for (unsigned int j = 0; j < col1; j++) {
        for (unsigned int i = 0; i < row1; i ++) {
            while (getline(fd0, str0, '\n')) {
                if (str0.front() != c) {
                    break;
                }
            }
            std::stringstream stream0(str0);
            float dataR;
            stream0 >> dataR;

            Q[i][j].real(dataR);

            while (getline(fd1, str1, '\n')) {
                if (str1.front() != c) {
                    break;
                }
            }
            std::stringstream stream1(str1);
            float dataI;
            stream1 >> dataI;
            Q[i][j].imag(dataI);

        }
    }

    std::cout << "Matrix_0[" << row0 << " x " << col0 << "]" << std::endl;
    for (unsigned int i = 0; i < row0; i++) {
        std::cout << "Row[" << i << "]: ";
        for (unsigned int j = 0; j < col0; j++) {
            std::cout << "(" << R[i][j].real() << ", " << R[i][j].imag() << "), ";
        }
        std::cout << std::endl;
    }
    std::cout << std::endl;
    std::cout << "Matrix_1[" << row1 << " x " << col1 << "]" << std::endl;
    for (unsigned int i = 0; i < row1; i++) {
        std::cout << "Row[" << i << "]: ";
        for (unsigned int j = 0; j < col1; j++) {
            std::cout << "(" << Q[i][j].real() << ", " << Q[i][j].imag() << "), ";
        }
        std::cout << std::endl;
    }
    std::cout << std::endl;
}

void printTriangularMatrix(std::string file0, std::string file1, int row0, int col0, int row1, int col1) {
    ifstream fd0, fd1;
    char c = 'T';
    string str0, str1;
    fd0.open(file0, ios::in);
    fd1.open(file1, ios::in);
    std::complex<float> R[row0][col0];
    std::complex<float> Q[row1][col1];
    for (unsigned int j = 0; j < col0; j++) {
        for (unsigned int i = 0; i <= j; i ++) {
            while (getline(fd0, str0, '\n')) {
                if (str0.front() != c) {
                    break;
                }
            }
            std::stringstream stream0(str0);
            float dataR;
            stream0 >> dataR;

            R[i][j].real(dataR);

            while (getline(fd1, str1, '\n')) {
                if (str1.front() != c) {
                    break;
                }
            }
            std::stringstream stream1(str1);
            float dataI;
            stream1 >> dataI;
            R[i][j].imag(dataI);

        }
        for (unsigned int i = j+1; i < row0; i ++) {
            R[i][j].real(0.0);
            R[i][j].imag(0.0);
        }
    }
    for (unsigned int j = 0; j < col1; j++) {
        for (unsigned int i = 0; i < row1; i ++) {
            while (getline(fd0, str0, '\n')) {
                if (str0.front() != c) {
                    break;
                }
            }
            std::stringstream stream0(str0);
            float dataR;
            stream0 >> dataR;

            Q[i][j].real(dataR);

            while (getline(fd1, str1, '\n')) {
                if (str1.front() != c) {
                    break;
                }
            }
            std::stringstream stream1(str1);
            float dataI;
            stream1 >> dataI;
            Q[i][j].imag(dataI);

        }
    }

    std::cout << "Matrix_0[" << row0 << " x " << col0 << "]" << std::endl;
    for (unsigned int i = 0; i < row0; i++) {
        std::cout << "Row[" << i << "]: ";
        for (unsigned int j = 0; j < col0; j++) {
            std::cout << "(" << R[i][j].real() << ", " << R[i][j].imag() << "), ";
        }
        std::cout << std::endl;
    }
    std::cout << std::endl;
    std::cout << "Matrix_1[" << row1 << " x " << col1 << "]" << std::endl;
    for (unsigned int i = 0; i < row1; i++) {
        std::cout << "Row[" << i << "]: ";
        for (unsigned int j = 0; j < col1; j++) {
            std::cout << "(" << Q[i][j].real() << ", " << Q[i][j].imag() << "), ";
        }
        std::cout << std::endl;
    }
    std::cout << std::endl;
}

void printTriangularMatrix_backwords(std::string file0, std::string file1, int row0, int col0, int row1, int col1) {
    ifstream fd0, fd1;
    char c = 'T';
    string str0, str1;
    fd0.open(file0, ios::in);
    fd1.open(file1, ios::in);
    std::complex<float> R[row0][col0];
    std::complex<float> Q[row1][col1];
    for (int j = col0-1; j >= 0; j--) {
        for (int i = row0-1; i >j ; i --) {
            R[i][j].real(0.0);
            R[i][j].imag(0.0);
        }
        for (int i = j; i >= 0; i --) {
            while (getline(fd0, str0, '\n')) {
                if (str0.front() != c) {
                    break;
                }
            }
            std::stringstream stream0(str0);
            float dataR;
            stream0 >> dataR;
            R[i][j].real(dataR);

            while (getline(fd1, str1, '\n')) {
                if (str1.front() != c) {
                    break;
                }
            }
            std::stringstream stream1(str1);
            float dataI;
            stream1 >> dataI;
            R[i][j].imag(dataI);
            //std::cout << "DEBUG: i=" << i << ", j=" << j << ", R.real=" << dataR << ", R.imag=" << dataI<< std::endl;
        }
    }
    for (int j = col1-1; j >= 0; j--) {
        for (int i = row1-1; i >=0; i --) {
            while (getline(fd0, str0, '\n')) {
                if (str0.front() != c) {
                    break;
                }
            }
            std::stringstream stream0(str0);
            float dataR;
            stream0 >> dataR;

            Q[i][j].real(dataR);

            while (getline(fd1, str1, '\n')) {
                if (str1.front() != c) {
                    break;
                }
            }
            std::stringstream stream1(str1);
            float dataI;
            stream1 >> dataI;
            Q[i][j].imag(dataI);

        }
    }

    std::cout << "Matrix_0[" << row0 << " x " << col0 << "]" << std::endl;
    for (unsigned int i = 0; i < row0; i++) {
        std::cout << "Row[" << i << "]: ";
        for (unsigned int j = 0; j < col0; j++) {
            std::cout << "(" << R[i][j].real() << ", " << R[i][j].imag() << "), ";
        }
        std::cout << std::endl;
    }
    std::cout << std::endl;
    std::cout << "Matrix_1[" << row1 << " x " << col1 << "]" << std::endl;
    for (unsigned int i = 0; i < row1; i++) {
        std::cout << "Row[" << i << "]: ";
        for (unsigned int j = 0; j < col1; j++) {
            std::cout << "(" << Q[i][j].real() << ", " << Q[i][j].imag() << "), ";
        }
        std::cout << std::endl;
    }
    std::cout << std::endl;
}

template <typename T>
void writeComplexToFiles128(ComplexMatrix<T>& X, std::string filename0, std::string filename1) {
    ofstream myfile0;
    ofstream myfile1;
    myfile0.open(filename0.c_str(), std::ios::app);
    myfile1.open(filename1.c_str(), std::ios::app);
    for (int j = 0; j < X.N; j++) {
        for (int i = 0; i < X.M; i += 4) {
            myfile0 << X.elem(i + 0, j).real() << " ";
            myfile0 << X.elem(i + 0, j).imag() << " ";
            myfile0 << X.elem(i + 1, j).real() << " ";
            myfile0 << X.elem(i + 1, j).imag() << " " << std::endl;

            myfile1 << X.elem(i + 2, j).real() << " ";
            myfile1 << X.elem(i + 2, j).imag() << " ";
            myfile1 << X.elem(i + 3, j).real() << " ";
            myfile1 << X.elem(i + 3, j).imag() << " " << std::endl;
        }
    }
    myfile0.close();
    myfile1.close();
}

template <typename T>
void writeComplexToFiles128_backwords(ComplexMatrix<T>& A, std::string filename0, std::string filename1) {
    ofstream myfile0;
    ofstream myfile1;
    myfile0.open(filename0.c_str(), std::ios::app);
    myfile1.open(filename1.c_str(), std::ios::app);
    for (int j = A.N-1; j >= 0; j--) {
        for (int i = A.M-1; i >= 0; i -= 4) {
            myfile0 << A.elem(i - 0, j).real() << " ";
            myfile1 << A.elem(i - 0, j).imag() << " ";
            myfile0 << A.elem(i - 1, j).real() << " ";
            myfile1 << A.elem(i - 1, j).imag() << " ";

            myfile0 << A.elem(i - 2, j).real() << " ";
            myfile1 << A.elem(i - 2, j).imag() << " ";
            myfile0 << A.elem(i - 3, j).real() << " " << std::endl;
            myfile1 << A.elem(i - 3, j).imag() << " " << std::endl;
        }
    }
    myfile0.close();
    myfile1.close();
}

template <typename T>
void writeComplexTriangularToFiles(ComplexMatrix<T>& X, std::string filename0, std::string filename1) {
    ofstream myfile0;
    ofstream myfile1;
    myfile0.open(filename0.c_str(), std::ios::app);
    myfile1.open(filename1.c_str(), std::ios::app);
    for (int j = 0; j < X.N; j++) {
        for (int i = 0; i <= j; i ++) {
            myfile0 << X.elem(i, j).real() << " " << std::endl;
            myfile1 << X.elem(i, j).imag() << " " << std::endl;
        }
    }
    myfile0.close();
    myfile1.close();
}

template <typename T>
void writeComplexTriangularToFiles_backwords(ComplexMatrix<T>& X, std::string filename0, std::string filename1) {
    ofstream myfile0;
    ofstream myfile1;
    myfile0.open(filename0.c_str(), std::ios::app);
    myfile1.open(filename1.c_str(), std::ios::app);
    for (int j = X.N-1; j>=0; j--) {
        for (int i = j; i >= 0; i --) {
            myfile0 << X.elem(i, j).real() << " " << std::endl;
            myfile1 << X.elem(i, j).imag() << " " << std::endl;
        }
    }
    myfile0.close();
    myfile1.close();
}

template <typename T>
void writeComplexToFiles(ComplexMatrix<T>& X, std::string filename0, std::string filename1, int dim) {
    ofstream myfile0;
    ofstream myfile1;
    myfile0.open(filename0.c_str(), std::ios::app);
    myfile1.open(filename1.c_str(), std::ios::app);
    for (int j = 0; j < X.N; j++) {
        for (int i = 0; i < dim; i ++) {
            myfile0 << X.elem(i, j).real() << " " << std::endl;
            myfile1 << X.elem(i, j).imag() << " " << std::endl;
        }
    }
    myfile0.close();
    myfile1.close();
}

template <typename T>
void writeComplexToFiles_backwords(ComplexMatrix<T>& X, std::string filename0, std::string filename1, int dim) {
    ofstream myfile0;
    ofstream myfile1;
    myfile0.open(filename0.c_str(), std::ios::app);
    myfile1.open(filename1.c_str(), std::ios::app);
    for (int j = X.N-1; j >= 0; j--) {
        for (int i = dim-1; i >= 0; i --) {
            myfile0 << X.elem(i, j).real() << " " << std::endl;
            myfile1 << X.elem(i, j).imag() << " " << std::endl;
        }
    }
    myfile0.close();
    myfile1.close();
}
