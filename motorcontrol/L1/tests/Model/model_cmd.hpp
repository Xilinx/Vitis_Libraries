/*
 * Copyright 2022 Xilinx, Inc.
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
#ifndef _MODEL_CMD_HPP_
#define _MODEL_CMD_HPP_
#include "model_motor.hpp"
#include <thread>
#include <unistd.h>
#include "string.h"
#include "stdio.h"
#include "stdlib.h"
#include <list>
// Giving a line and parsing it into command and parameter stream
// pl <obj>
// run <num_step>
// gdb : r, s, c
// set <ojb>.<para> <value>
// help
// quit

static int cmd_findPara(int argc, char** argv, char* para) {
    assert(para);
    for (int i = 1; i < argc; i++) {
        if (0 == strcmp(argv[i], para)) return i;
    }
    return -1;
}
template <class T>
bool cmd_findParaValue(int argc, char** argv, char* para, T& v) {
    int idx = cmd_findPara(argc, argv, para);
    if (idx < 1) return false;
    if ((idx + 1) >= argc) return false;
    if (std::is_same<int, T>::value)
        v = atoi(argv[idx + 1]);
    else if (std::is_same<double, T>::value)
        v = atof(argv[idx + 1]);
    else if (std::is_same<float, T>::value)
        v = atof(argv[idx + 1]);
    return true;
}
// clang-format off
enum SIMCMD {
    CMD_MAN=0,
    CMD_RUN,
    CMD_SET,
    CMD_STOP,
    CMD_PL,
    //ALWAYS ADD NEW BEFORE CMD_QUIT
    CMD_QUIT,
    CMD_TOTAL_NUM
};

#define NUM_CMD (CMD_TOTAL_NUM)
static const char* cmdlist[]{
    "man",
    "run",
    "set",
    "clear",
    "pl",
    "quit"
};

static const char* cmdlist_intro[]{
    "man  : Manual of commands.",
    "run  : Running simulator with set/default number of step. Eg. <cmd> [<num_step>] [-log <filename>] [-restart]",
    "set  : Setting a parameter's value of a model object. Eg. <cmd> <name_obj>[.<name_obj>]*.<parameter>.",
    "clear: Stopping the motor and pids and clearing states variable",
    "pl   : Listing the model object's information. Eg. <cmd> <name_obj>[.<name_obj>]*" ,
    "quit : To quit simulator."
};
// clang-format on
class cmd2Argv {
   public:
    int argc;
    char argv[64][256];
    cmd2Argv() { cmd_resetArg(); };
    void parseRun(int& num_step, char* filename, bool& isRestar) {}
    void cmd_resetArg() {
        if (argc == 0) return;
        for (int i = 0; i < argc; i++) argv[i][0] = '\0';
        argc = 0;
    }
    bool isChNormal(char ch) { return (ch != '\0') && (ch != '\n') && (ch != ' ') && (ch != '\t' && (ch != '#')); }
    char* cmd_SkipSpace(char* str) {
        char* pch = str;
        while (*pch != 0) {
            if (*pch == ' ' || *pch == '\t') {
                pch++;
            } else
                break;
        }
        return pch;
    }
    int cmd_CpyWord(char* des, char* src) {
        int cnt = 0;
        assert(des);
        assert(src);
        while (isChNormal(*src)) {
            *des = *src;
            src++;
            des++;
            cnt++;
        }
        *des = '\0';
        return cnt;
    }
    int cmd_getArgs(char* str) {
        assert(str);
        cmd_resetArg();
        char* pch = str;
        do {
            pch = cmd_SkipSpace(pch);
            if (isChNormal(*pch)) {
                pch += cmd_CpyWord(argv[argc++], pch);
            }
        } while (*pch != '\n' && *pch != '\0' && *pch != '#');
        return argc;
    }

    int cmd_getCmdID() {
        int cmd_last;
        for (cmd_last = 0; cmd_last < NUM_CMD; cmd_last++)
            if (0 == strcmp(cmdlist[cmd_last], argv[0])) break;
        return cmd_last;
    }
};

;
#endif