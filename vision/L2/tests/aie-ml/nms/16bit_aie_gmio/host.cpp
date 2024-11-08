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

#define PROFILE

#include <chrono>
#include <common/xf_aie_sw_utils.hpp>
#include <common/xfcvDataMovers.h>
#include <fstream>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <xrt/experimental/xrt_kernel.h>
#include <xaiengine.h>

#include "nms_aa_ref.hpp"
#include "config.h"
/*
 ******************************************************************************
 * Top level executable
 ******************************************************************************
 */

int main(int argc, char** argv) {
    try {
        if (argc != 2) {
            std::stringstream errorMessage;
            errorMessage << argv[0] << " <xclbin>";
            std::cerr << errorMessage.str();
            throw std::invalid_argument(errorMessage.str());
        }

        const char* xclBinName = argv[1];

        // Initialize device
        xF::deviceInit(xclBinName);

        int BLOCK_SIZE_in_Bytes = TILE_WINDOW_SIZE;
        int BLOCK_SIZE_out_Bytes = TILE_WINDOW_SIZE_OUT;

        auto yminBuffer = xrt::aie::bo(xF::gpDhdl, BLOCK_SIZE_in_Bytes, xrt::bo::flags::normal, /*memory group*/ 0);
        auto xminBuffer = xrt::aie::bo(xF::gpDhdl, BLOCK_SIZE_in_Bytes, xrt::bo::flags::normal, /*memory group*/ 0);
        auto ymaxBuffer = xrt::aie::bo(xF::gpDhdl, BLOCK_SIZE_in_Bytes, xrt::bo::flags::normal, /*memory group*/ 0);
        auto xmaxBuffer = xrt::aie::bo(xF::gpDhdl, BLOCK_SIZE_in_Bytes, xrt::bo::flags::normal, /*memory group*/ 0);
        auto outputBuffer = xrt::aie::bo(xF::gpDhdl, BLOCK_SIZE_out_Bytes, xrt::bo::flags::normal, /*memory group*/ 0);
        auto outputRefBuffer =
            xrt::aie::bo(xF::gpDhdl, BLOCK_SIZE_out_Bytes, xrt::bo::flags::normal, /*memory group*/ 0);

        int16_t* yminData = yminBuffer.map<int16_t*>();
        int16_t* xminData = xminBuffer.map<int16_t*>();
        int16_t* xmaxData = xmaxBuffer.map<int16_t*>();
        int16_t* ymaxData = ymaxBuffer.map<int16_t*>();
        int16_t* outputData = outputBuffer.map<int16_t*>();
        int16_t* outputRefData = outputRefBuffer.map<int16_t*>();
        /*        int16_t* yminData = yminBuffer.map<int16_t*>();
                int16_t* xminData = (int16_t*)GMIO::malloc(BLOCK_SIZE_in_Bytes);
                int16_t* ymaxData = (int16_t*)GMIO::malloc(BLOCK_SIZE_in_Bytes);
                int16_t* xmaxData = (int16_t*)GMIO::malloc(BLOCK_SIZE_in_Bytes);
                int16_t* outputData = (int16_t*)GMIO::malloc(BLOCK_SIZE_out_Bytes);
                int16_t* outputRefData = (int16_t*)malloc(BLOCK_SIZE_out_Bytes);

                memset(yminData, 0, BLOCK_SIZE_in_Bytes);
                memset(xminData, 0, BLOCK_SIZE_in_Bytes);
                memset(ymaxData, 0, BLOCK_SIZE_in_Bytes);
                memset(xmaxData, 0, BLOCK_SIZE_in_Bytes);
        */
        // Generate random test data for test, the data must be positive
        for (int i = 0; i < TILE_ELEMENTS; i++) {
            yminData[i] = (int16_t)(rand() % 1000);
            yminData[i] = yminData[i] < 0 ? -yminData[i] : yminData[i];
            xminData[i] = (int16_t)(rand() % 1000);
            xminData[i] = xminData[i] < 0 ? -xminData[i] : xminData[i];
            ymaxData[i] = (int16_t)(yminData[i] + (rand() % 200) + 1);
            xmaxData[i] = (int16_t)(xminData[i] + (rand() % 200) + 1);
        }

        // set this parameter based on the number of valid boxes. all the valid boxes must be segregated to the top of
        // the list
        constexpr int total_valid_boxes = 100;
#if !__X86_DEVICE__
        auto gHndl = xrt::graph(xF::gpDhdl, xF::xclbin_uuid, "mygraph");
        std::cout << "XRT graph opened" << std::endl;
        gHndl.reset();
#endif

        yminBuffer.async("mygraph.inprt1", XCL_BO_SYNC_BO_GMIO_TO_AIE, BLOCK_SIZE_in_Bytes, 0);
        xminBuffer.async("mygraph.inprt2", XCL_BO_SYNC_BO_GMIO_TO_AIE, BLOCK_SIZE_in_Bytes, 0);
        ymaxBuffer.async("mygraph.inprt3", XCL_BO_SYNC_BO_GMIO_TO_AIE, BLOCK_SIZE_in_Bytes, 0);
        xmaxBuffer.async("mygraph.inprt4", XCL_BO_SYNC_BO_GMIO_TO_AIE, BLOCK_SIZE_in_Bytes, 0);

#if !__X86_DEVICE__
        // update RTPs
        gHndl.update("mygraph.k1.in[4]", (int16_t)IOU_THRESH);
        gHndl.update("mygraph.k1.in[5]", (int16_t)MAX_DET);
        gHndl.update("mygraph.k1.in[6]", (int16_t)total_valid_boxes);
        //@{
        START_TIMER
        // graph run
        gHndl.run(1);
#endif
        /*
                mygraph.inprt1.gm2aie_nb(yminData, BLOCK_SIZE_in_Bytes);
                mygraph.inprt2.gm2aie_nb(xminData, BLOCK_SIZE_in_Bytes);
                mygraph.inprt3.gm2aie_nb(ymaxData, BLOCK_SIZE_in_Bytes);
                mygraph.inprt4.gm2aie_nb(xmaxData, BLOCK_SIZE_in_Bytes);
                mygraph.outprt.aie2gm_nb(outputData, BLOCK_SIZE_out_Bytes);
        */
        // wait for the kernel to complete
        auto outputBuffer_run =
            outputBuffer.async("mygraph.outprt", XCL_BO_SYNC_BO_AIE_TO_GMIO, BLOCK_SIZE_out_Bytes, 0);

#if !__X86_DEVICE__
        gHndl.wait();
#endif
        // wait for the output data transfer to DDR to complete
        outputBuffer_run.wait();
        STOP_TIMER("Total time to process frame")
//@}

#if !__X86_DEVICE__
        // graph end
        gHndl.end();
#endif
        // Validate result
        nms_ref<TILE_ELEMENTS>(yminData, xminData, ymaxData, xmaxData, outputRefData, IOU_THRESH_FL, (int)MAX_DET,
                               total_valid_boxes);
        printf("\nTOTAL DETECTED BOXES:%d", (int)outputData[0]);
        printf("\nTOTAL REFERENCE BOXES:%d", (int)outputRefData[0]);
#if DUMP_BOXES
        for (int i = 0; i < outputData[0]; i++) {
            printf("\nbox %d - ymin: %d\n", i + 1, (int)outputData[i * 4 + 1]);
            printf("box %d - xmin: %d\n", i + 1, (int)outputData[i * 4 + 2]);
            printf("box %d - ymax: %d\n", i + 1, (int)outputData[i * 4 + 3]);
            printf("box %d - xmax: %d\n", i + 1, (int)outputData[i * 4 + 4]);
        }

        for (int i = 0; i < outputRefData[0]; i++) {
            printf("\nbox %d - ymin: %d\n", i + 1, (int)outputRefData[i * 4 + 1]);
            printf("box %d - xmin: %d\n", i + 1, (int)outputRefData[i * 4 + 2]);
            printf("box %d - ymax: %d\n", i + 1, (int)outputRefData[i * 4 + 3]);
            printf("box %d - xmax: %d\n", i + 1, (int)outputRefData[i * 4 + 4]);
        }
#endif

        if (outputData[0] != outputRefData[0]) {
            std::cout << "Total detected boxes doesnot match!\nTest failed!" << std::endl;
            exit(-1);
        }

        printf("\nComparing with the detected boxes with the reference...\n");
        for (int i = 0; i < outputData[0]; i++) {
            bool valid = false;
            _Box box_t;
            box_t.ymin = outputData[i * 4 + 1];
            box_t.xmin = outputData[i * 4 + 2];
            box_t.ymax = outputData[i * 4 + 3];
            box_t.xmax = outputData[i * 4 + 4];
            for (int j = 0; j < outputRefData[0]; j++) {
                _Box box_r;
                box_r.ymin = outputRefData[j * 4 + 1];
                box_r.xmin = outputRefData[j * 4 + 2];
                box_r.ymax = outputRefData[j * 4 + 3];
                box_r.xmax = outputRefData[j * 4 + 4];

                float iou_val = compIOU(box_t, box_r);
                // set the thresh value based on accuracy required
                if (iou_val > 0.85) valid = true;
            }
            if (valid == true) {
                std::cout << "Box " << (i + 1) << " passed!" << std::endl;
            } else {
                std::cout << "Test failed for box id: " << (i + 1) << std::endl;
                exit(-1);
            }
        }
        std::cout << "Test passed!" << std::endl;

        std::cout << "Run Complete!" << std::endl;

        // free memory buffers
        /*        GMIO::free(yminData);
                GMIO::free(xminData);
                GMIO::free(ymaxData);
                GMIO::free(xmaxData);
                GMIO::free(outputData);
                free(outputRefData);
        */
        return 0;
    } catch (std::exception& e) {
        const char* errorMessage = e.what();
        std::cerr << "Exception caught: " << errorMessage << std::endl;
        exit(-1);
    }
}
