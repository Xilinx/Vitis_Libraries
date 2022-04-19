#include "xf_database/compaction_core/config.hpp"
#include <vector>
#include <iostream>
#include <algorithm>
#include <stdlib.h>
#include <fstream>
#include <string>
#include <unistd.h>

std::string gen_random(const int len) {
    std::string tmp_s;
    static const char alphanum[] =
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";

    // srand( (unsigned) time(NULL) * getpid());
    // srand(seed);

    tmp_s.reserve(len);

    for (int i = 0; i < len; ++i) tmp_s += alphanum[rand() % (sizeof(alphanum) - 1)];

    return tmp_s;
}

int main(int argc, char** argv) {
    ////////////////////////////////////////////////////////////////
    // Test Data Generation
    ////////////////////////////////////////////////////////////////
    int keyAll = 0;
    srand((unsigned int)time(NULL));
    int in_keyCnt = 0;
    if (argc == 1) {
        keyAll = 30000 * IncreaseFactor_;
        in_keyCnt = rand() % keyAll;
    }
    if (argc == 2) {
        keyAll = atoi(argv[1]) * IncreaseFactor_;
        in_keyCnt = rand() % keyAll;
    }
    if (argc == 3) {
        keyAll = atoi(argv[1]) * IncreaseFactor_;
        in_keyCnt = atoi(argv[2]);
    }

    int keyCnt3 = rand() % keyAll;
    std::cout << keyAll << " " << in_keyCnt << " " << keyCnt3 << std::endl;

    std::vector<std::string> lall;
    for (int i = 0; i < keyAll + keyCnt3; i++) {
        lall.push_back(gen_random(rand() % 128 + 32));
    }
    auto a_it = std::unique(lall.begin(), lall.end());
    lall.resize(std::distance(lall.begin(), a_it));
    std::cout << "All keys generated" << std::endl;

    std::vector<std::string> l1;
    for (int i = 0; i < in_keyCnt; i++) {
        l1.push_back(lall[i]);
    }
    auto it = std::unique(l1.begin(), l1.end());
    l1.resize(std::distance(l1.begin(), it));
    sort(l1.begin(), l1.end());
    std::cout << "L1 added & sorted" << std::endl;

    std::vector<std::string> l2;
    for (int i = 0; i < keyAll - in_keyCnt; i++) {
        l2.push_back(lall[i + in_keyCnt]);
    }
    it = std::unique(l2.begin(), l2.end());
    l2.resize(std::distance(l2.begin(), it));
    sort(l2.begin(), l2.end());
    std::cout << "L2 added & sorted" << std::endl;

    std::vector<std::string> l3;
    for (int i = 0; i < keyCnt3; i++) {
        l3.push_back(lall[keyAll + i]);
    }
    it = std::unique(l3.begin(), l3.end());
    l3.resize(std::distance(l3.begin(), it));
    sort(l3.begin(), l3.end());
    std::cout << "L3 added & sorted" << std::endl;

    std::ofstream of1;
    of1.open("input1.txt", std::ofstream::out);
    for (auto l1_ite : l1) {
        of1 << l1_ite << std::endl;
    }
    of1.close();

    std::ofstream of2;
    of2.open("input2.txt", std::ofstream::out);
    for (auto l2_ite : l2) {
        of2 << l2_ite << std::endl;
    }
    of2.close();

    std::ofstream of3;
    of3.open("input3.txt", std::ofstream::out);
    for (auto l3_ite : l3) {
        of3 << l3_ite << std::endl;
    }
    of3.close();
}
