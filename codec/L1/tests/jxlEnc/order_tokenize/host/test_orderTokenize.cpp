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

#include "utils.hpp"
#include <string.h>
#include <iostream>
#include "topOrderTokenize.hpp"

int main(int argc, const char* argv[]) {
    std::cout << "\n-----------------Order Tokenize----------------\n";

    // cmd parser
    ArgParser parser(argc, argv);

    std::string infile;
    if (!parser.getCmdOption("-i", infile)) {
        std::cout << "ERROR: input file path is not set!\n";
        return 1;
    }

    std::string goldenfile;
    if (!parser.getCmdOption("-g", goldenfile)) {
        std::cout << "ERROR: golden input file path is not set!\n";
        return 1;
    }

    hls::stream<ap_uint<32> > orderStrm("orderStrm");
    hls::stream<ap_uint<64> > tokenStrm("tokenStrm");
    hls::stream<bool> e_tokenStrm("e_tokenStrm");

    // read data
    FILE* pFile;
    pFile = fopen(infile.c_str(), "r");
    int num_orders;
    int used_orders;
    fscanf(pFile, "%d %d", &used_orders, &num_orders);
    printf("INFO: num_orders is %d \n", num_orders);
    for (int j = 0; j < num_orders; j++) {
        int tmp;
        fscanf(pFile, "%d", &tmp);
        orderStrm.write(tmp);
    }
    fclose(pFile);

    top_order_tokenize(used_orders, orderStrm, tokenStrm, e_tokenStrm);

    uint32_t num_tokens;
    uint32_t context, value;

    FILE* outFile;
    std::string outfile(infile);
    std::size_t found = outfile.find_last_of(".");
    outfile.insert(found, "_tokens");
    outFile = fopen(outfile.c_str(), "w");
    int err = 0;

    bool e_kens(false);
    ap_uint<64> tokenTmp;
    uint32_t contextTmp, valueTmp;

    pFile = fopen(goldenfile.c_str(), "r");
    fscanf(pFile, "%d", &num_tokens);
    while (!e_kens) {
        e_kens = e_tokenStrm.read();
        if (!e_kens) {
            tokenStrm.read(tokenTmp);
            contextTmp = tokenTmp.range(31, 0).to_uint();
            valueTmp = tokenTmp.range(63, 32).to_uint();
            fscanf(pFile, "%d %d", &context, &value);
            fprintf(outFile, "%d %d\n", contextTmp, valueTmp);
            if (contextTmp != context || valueTmp != value) {
                err++;
            }
        }
    }

    fclose(pFile);
    fclose(outFile);

    if (err) {
        std::cerr << "INFO: Tokenize \033[31merrors " << err << "\033[0m" << std::endl;
    } else {
        std::cout << "INFO: Tokenize CSim Pass!" << std::endl;
    }

    return err;
}
