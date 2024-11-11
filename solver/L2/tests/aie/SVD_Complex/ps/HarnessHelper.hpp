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

#include <complex>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string>
#include <cstring>
#include "DatamoverCfgGen.hpp"
#include "FastXM.hpp"

/*
    Introduction:
        This function transfers the data between PS. PL and AIE,
        it has two input (from_aie_strm0 and from_aie_strm1) and
        two output (to_aie_strm0 and to_aie_strm1) on PL side.
        To use the api, you need to modify the file cfg/system.cfg
        to config the connection between PL and AIE.

        The input data on PS side is transfered in such way:
        Every WDATA bits on PS input side are divided into two WDATA/2 bits parts,
        then the low WDATA/2 bits are transfered to to_aie_strm0,
        the high WDATA/2 bits are transfered to to_aie_strm1,
        the data from AIE will be transfered to from_aie_strm0 and from_aie_strm1.
        Supported WDATA size: 32/64/128/256
    Template Parameters
        WDATA: the bandwidth of datamover
        T: the type of data
*/

template <int WDATA, typename T>
class HarnessHelper : public fastXM {
   private:
    T* data;
    T* result;
    int pattern_size;
    int PL_loop;
    bool graph_started;

   public:
    HarnessHelper(unsigned int device_index, std::string xclbin_file_path, std::vector<std::string> graph_name)
        : fastXM(device_index, xclbin_file_path, {"biDatamover", "splitMerge"}, graph_name) {
        graph_started = false;
    }

    void runPL(T* data, int data_length, int result_length, int PL_loop) {
        const int data_sz = data_length * sizeof(T);
        this->data = (T*)malloc(data_sz);
        memcpy(this->data, data, data_sz);
        const int result_sz = result_length * sizeof(T);
        result = (T*)malloc(result_sz);

        this->PL_loop = PL_loop;

        pattern_size = data_sz / (WDATA / 8);
        DatamoverCfgGen<WDATA> datamoverCfgGen(pattern_size, PL_loop);

        if (!graph_started) {
            std::cout << "Warning: you're trying to call 'runTestHarness' before calling 'runAIEGraph'." << std::endl;
            std::cout << "This might lead to result of 'printPerf' to be fluctuated." << std::endl;
            std::cout << "It is strongly recommended to call 'runAIEGraph' before 'runTestHarness'." << std::endl;
        }
        this->fastXM::runPL(
            0, {{0, true, (unsigned int)datamoverCfgGen.cfg_size_in_byte, (char*)datamoverCfgGen.cfg_buf, 0},
                {1, true, (unsigned int)data_sz, (char*)this->data, 0},
                {2, true, (unsigned int)result_sz, (char*)this->result, 0}});
        this->fastXM::runPL(1, {{0, false, 0, nullptr, pattern_size}, {1, false, 0, nullptr, PL_loop}});
    }

    void runAIEGraph(int g_index, int AIERunTimes) {
        this->runGraph(g_index, AIERunTimes);
        graph_started = true;
    }

    T* waitForRes(int graph_timeout_millisec) {
        this->waitDone(graph_timeout_millisec);
        graph_started = false;
        this->fetchRes();
        return result;
    }
};
