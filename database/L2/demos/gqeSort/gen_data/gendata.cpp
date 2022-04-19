#include <cstdlib>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <random>
#include "x_utils.hpp"
#include "gendata.hpp"

int main(int argc, const char* argv[]) {
    srand(time(NULL));
    x_utils::ArgParser parser(argc, argv);
    std::string size_s = "1"; //
    int size = 1024 * 1024;
    if (!parser.getCmdOption("-ss", size_s)) {
        size = 1024 * 1024;
    } else {
        size *= std::stoi(size_s);
    }
    std::string out_dir = "./";
    if (!parser.getCmdOption("-out", out_dir)) {
        out_dir = "./";
    }

    int loop_num;
    std::string loop_num_str;
    if (!parser.getCmdOption("-ln", loop_num_str)) {
        loop_num_str = "1";
    }
    loop_num = std::stoi(loop_num_str);
    std::cout << "num_iters:" << loop_num << ", size:" << size / 1024 / 1024 << "M" << std::endl;

    std::cout << "RAND_MAX = " << RAND_MAX << std::endl;
    for (int i = 0; i < loop_num; i++) {
        std::cout << "generating " << i << std::endl;
        gen_dat(out_dir + "/input_" + size_s + "M_" + std::to_string(i) + ".dat", size);
    }
    std::cout << "Generated " << loop_num << " " << size_s << "M int pair data" << std::endl;
    return 0;
}
