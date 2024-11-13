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

#include "aie_graph.hpp"

const int rowA_num = M;
const int colA_num = N;
const int rowB_num = M;
const int colB_num = K;

std::string fin_0 = "./data/in_0_" + std::to_string(rowA_num) + "_" + std::to_string(colA_num) + ".txt";
std::string fin_1 = "./data/in_1_" + std::to_string(rowA_num) + "_" + std::to_string(colA_num) + ".txt";

std::string fRC_0 = "./data/RC_0_" + std::to_string(rowA_num) + "_" + std::to_string(colA_num) + ".txt";
std::string fRC_1 = "./data/RC_1_" + std::to_string(rowA_num) + "_" + std::to_string(colA_num) + ".txt";
std::string fRC_QRD_0 = "./data/RC_QRD_0_" + std::to_string(rowA_num) + "_" + std::to_string(colA_num) + ".txt";
std::string fRC_QRD_1 = "./data/RC_QRD_1_" + std::to_string(rowA_num) + "_" + std::to_string(colA_num) + ".txt";

std::string fgldRC_0 = "./data/gldRC_0_" + std::to_string(rowA_num) + "_" + std::to_string(colA_num) + ".txt";
std::string fgldRC_1 = "./data/gldRC_1_" + std::to_string(rowA_num) + "_" + std::to_string(colA_num) + ".txt";
std::string fRC_Trans_0 = "./data/RC_Trans_0_" + std::to_string(rowA_num) + "_" + std::to_string(colA_num) + ".txt";
std::string fRC_Trans_1 = "./data/RC_Trans_1_" + std::to_string(rowA_num) + "_" + std::to_string(colA_num) + ".txt";

std::string fRC_Trans_backwords_0 = "./data/RC_Trans_backwords_0_" + std::to_string(rowA_num) + "_" + std::to_string(colA_num) + ".txt";
std::string fRC_Trans_backwords_1 = "./data/RC_Trans_backwords_1_" + std::to_string(rowA_num) + "_" + std::to_string(colA_num) + ".txt";

std::string fout_backwords_0 = "./data/out_backwords_0_" + std::to_string(colA_num) + ".txt";
std::string fout_backwords_1 = "./data/out_backwords_1_" + std::to_string(colA_num) + ".txt";
std::string fgld_backwords_0 = "./data/gld_backwords_0_" + std::to_string(colA_num) + ".txt";
std::string fgld_backwords_1 = "./data/gld_backwords_1_" + std::to_string(colA_num) + ".txt";

TestGraph mygraph(fRC_Trans_backwords_0, fRC_Trans_backwords_1, fout_backwords_0, fout_backwords_1);

#if defined(__AIESIM__) || defined(__X86SIM__)
#include <iostream>
#include <fstream>
#include "utils.hpp"
using namespace std;


int main(int argc, char** argv) {
    adf::return_code ret;
    mygraph.init();
    for(int i=0; i<colA_num; i++) {
        //mygraph.update(mygraph.column_id0[i], i);
        mygraph.update(mygraph.column_id1[i], i);
    }

    ret = mygraph.run(1);
    if (ret != adf::ok) {
        printf("Run Failed\n");
        return ret;
    }

    ret = mygraph.end();
    if (ret != adf::ok) {
        printf("End Failed\n");
        return ret;
    }

    int errNum = 9999;
    int sampleNum = 9999;

    std::cout << "Verify LST With QRD and Back-Substitution!" << std::endl;

    //sampleNum = colA_num * colB_num;
    sampleNum = N* K;

    //std::cout << "Verify QR Decompsition" << std::endl;
    //std::cout << "Output Matrix_R && Matrix_B \n";
    //printComplexMatrix128(fRC_0, fRC_1, rowA_num, colA_num, rowA_num, colB_num);
    //std::cout << "Golden Matrix_R && Matrix_B \n";
    //printComplexMatrix128(fRC_QRD_0, fRC_QRD_1, rowA_num, colA_num, rowA_num, colB_num);
    //errNum = golden_check(fRC_0, fRC_1, fRC_QRD_0, fRC_QRD_1,  rowA_num, colA_num, rowA_num, colB_num);

    //std::cout << "Verify Transform" << std::endl;
    //std::cout << "Input  Matrix\n";
    //printComplexMatrix128(fRC_QRD_0, fRC_QRD_1, rowA_num, colA_num, rowB_num, colB_num);
    //std::cout << "Output Matrix\n";
    //printTriangularMatrix(fgldRC_0, fgldRC_1, rowA_num, colA_num);
    //sampleNum = ((1+colA_num)*colA_num )/2 + colA_num * colB_num;
    //errNum = verify_output(fgldRC_0, fgldRC_1, fRC_Trans_0, fRC_Trans_1, sampleNum);

    std::cout << "Verify Back-Substition" << std::endl;
    if(N < 65) {
        std::cout << "Out Matrix\n";
        printOutputMatrix128(fout_backwords_0, fout_backwords_1, colA_num, colB_num);
        std::cout << "Gld Matrix\n";
        printOutputMatrix32(fgld_backwords_0, fgld_backwords_1, colA_num, colB_num);
    }
    errNum = verify_output(fout_backwords_0, fout_backwords_1, fgld_backwords_0, fgld_backwords_1,  colA_num, colB_num);


    return errNum;
}
#endif
