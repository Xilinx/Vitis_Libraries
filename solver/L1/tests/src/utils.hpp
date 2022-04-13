/*
 * Copyright 2021 Xilinx, Inc.
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

#ifndef _XF_SOLVER_UTILS_HPP_
#define _XF_SOLVER_UTILS_HPP_

#include <fstream>
#include <iostream>
#include <complex>
#include <sstream>
#include <algorithm>
#include "hls_x_complex.h"

template <typename T>
void readTxt(std::string file, T* buffer, int size) {
    int index = 0;
    char line[1024] = {0};

    std::fstream fhdl(file.c_str(), std::ios::in);
    if (!fhdl) {
        std::cout << "[ERROR] File: " << file << " could not open !" << std::endl;
        exit(1);
    }
    while (fhdl.getline(line, sizeof(line))) {
        std::string str(line);
        std::replace(str.begin(), str.end(), ',', ' ');
        std::stringstream data(str.c_str());
        data >> buffer[index++];
        if (index >= size) {
            break;
        }
    }
    fhdl.close();
}

template <typename T>
void readTxt(std::string file, hls::x_complex<T>* buffer, int size) {
    int index = 0;
    T real = 0;
    T imag = 0;
    hls::x_complex<T> tmp;
    char line[1024] = {0};

    std::fstream fhdl(file.c_str(), std::ios::in);
    if (!fhdl) {
        std::cout << "[ERROR] File: " << file << " could not open !" << std::endl;
        exit(1);
    }
    while (fhdl.getline(line, sizeof(line))) {
        std::string str(line);
        std::replace(str.begin(), str.end(), ',', ' ');
        std::stringstream data(str.c_str());
        data >> real;
        data >> imag;
        tmp.real(real);
        tmp.imag(imag);
        buffer[index] = tmp;
        index++;
        if (index >= size) {
            break;
        }
    }
    fhdl.close();
}

template <typename T>
void readTxt(std::string file, std::complex<T>* buffer, int size) {
    int index = 0;
    T real = 0;
    T imag = 0;
    std::complex<T> tmp;
    char line[1024] = {0};

    std::fstream fhdl(file.c_str(), std::ios::in);
    if (!fhdl) {
        std::cout << "[ERROR] File: " << file << " could not open !" << std::endl;
        exit(1);
    }
    while (fhdl.getline(line, sizeof(line))) {
        std::string str(line);
        std::replace(str.begin(), str.end(), ',', ' ');
        std::stringstream data(str.c_str());
        data >> real;
        data >> imag;
        tmp.real(real);
        tmp.imag(imag);
        buffer[index] = tmp;
        index++;
        if (index >= size) {
            break;
        }
    }
    fhdl.close();
}

template <typename T>
void writeTxt(std::string file, T* buffer, int size) {
    std::fstream fhdl(file.c_str(), std::ios::out);
    if (!fhdl) {
        std::cout << "[ERROR] File: " << file << " file could not open !" << std::endl;
        exit(1);
    }
    for (int i = 0; i < size; i++) {
        fhdl << buffer[i] << "\n";
    }
    fhdl.close();
}

template <typename T>
void writeTxt(std::string file, hls::x_complex<T>* buffer, int size) {
    std::fstream fhdl(file.c_str(), std::ios::out);
    if (!fhdl) {
        std::cout << "[ERROR] File: " << file << " could not open !" << std::endl;
        exit(1);
    }
    for (int i = 0; i < size; i++) {
        fhdl << buffer[i].real() << "  ";
        fhdl << buffer[i].imag() << "\n";
    }
    fhdl.close();
}

template <typename T>
void writeTxt(std::string file, std::complex<T>* buffer, int size) {
    std::fstream fhdl(file.c_str(), std::ios::out);
    if (!fhdl) {
        std::cout << "[ERROR] File: " << file << " could not open !" << std::endl;
        exit(1);
    }
    for (int i = 0; i < size; i++) {
        fhdl << buffer[i].real() << "  ";
        fhdl << buffer[i].imag() << "\n";
    }
    fhdl.close();
}

#endif
