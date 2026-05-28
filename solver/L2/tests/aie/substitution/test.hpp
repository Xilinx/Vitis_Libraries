#ifndef _TEST_HPP_
#define _TEST_HPP_

#include <adf.h>
#include <string>
#include <array>
#include <cstdio>
#include "uut_config.h"
#include "substitution_graph.hpp"

#define Q(x) #x
#define QUOTE(x) Q(x)

#ifndef UUT_GRAPH
#define UUT_GRAPH substitution_graph
#endif

#include QUOTE(UUT_GRAPH.hpp)

using namespace adf;

namespace xf::solver::aie::test {

class test_graph : public adf::graph {
public:
#if USING_UUT==1
    static constexpr unsigned int kNumKernels = GRID_DIM*(GRID_DIM+1)/2;
    // Define input/output PLIO ports
    std::array<input_plio, kNumKernels> plio_in_L;
    std::array<input_plio, kNumKernels> plio_in_y;
    std::array<output_plio, GRID_DIM> plio_out_x;

    test_graph() {
        printf("========================\n");
        printf("== Substitution test.hpp parameters: ");
        printf(QUOTE(UUT_GRAPH));
        printf("\n");
        printf("========================\n");
        printf("Data type      = ");
        printf(QUOTE(DATA_TYPE));
        printf("\n");
        printf("DIM          = %d \n", DIM_SIZE);
        printf("GRID_DIM     = %d \n", GRID_DIM);
        printf("DIRECTION    = %d \n", SUBST_TYPE);
        printf("L_LEADING    = %d \n", L_LEADING);
        printf("NUM_FRAMES   = %d \n", NUM_FRAMES);
        printf("DIAG_INV     = %d \n", DIAG_INV);
        printf("NITER        = %d \n", NITER);
        printf("kNumKernels  = %d \n", kNumKernels);

        // Instantiate the substitution graph
        xf::solver::aie::substitution::UUT_GRAPH<DATA_TYPE, DIM_SIZE, SUBST_TYPE, L_LEADING, GRID_DIM, NUM_FRAMES, DIAG_INV> dut_graph;

        int portIdx = 0;
        for (int y = 0; y < GRID_DIM; y++) {
            for (int x = 0; x < y+1; x++) {
                // Make connections
                std::string filenameInL = QUOTE(INPUT_FILE_L);
                if (L_LEADING==0) {
                  filenameInL.insert(filenameInL.length() - 4, ("_" + std::to_string(x) + "_" + std::to_string(y)));
                } else {
                  filenameInL.insert(filenameInL.length() - 4, ("_" + std::to_string(y) + "_" + std::to_string(x)));
                }
                //if (SUBST_TYPE == 0) {
                //  if (L_LEADING==0) {
                //    filenameInL.insert(filenameInL.length() - 4, ("_" + std::to_string(x) + "_" + std::to_string(y)));
                //  } else {
                //    filenameInL.insert(filenameInL.length() - 4, ("_" + std::to_string(y) + "_" + std::to_string(x)));
                //  }
                //} else {
                //  if (L_LEADING==0) {
                //    filenameInL.insert(filenameInL.length() - 4, ("_" + std::to_string(y) + "_" + std::to_string(x)));
                //  } else {
                //    filenameInL.insert(filenameInL.length() - 4, ("_" + std::to_string(x) + "_" + std::to_string(y)));
                //  }
                // }
                plio_in_L[portIdx] = input_plio::create("PLIO_in_L_" + std::to_string(portIdx), adf::plio_64_bits, filenameInL);
                connect<>(plio_in_L[portIdx].out[0], dut_graph.L_in[portIdx]);
                single_buffer(dut_graph.L_in[portIdx]);

                std::string filenameInY = QUOTE(INPUT_FILE_Y);
                filenameInY.insert(filenameInY.length() - 4, ("_" + std::to_string(x) + "_0" ));
                plio_in_y[portIdx] = input_plio::create("PLIO_in_Y_" + std::to_string(portIdx), adf::plio_64_bits, filenameInY);
                connect<>(plio_in_y[portIdx].out[0], dut_graph.y_in[portIdx]);

                if (x == y) {
                  std::string filenameOutX = QUOTE(OUTPUT_FILE);
                  filenameOutX.insert(filenameOutX.length() - 4, ("_" + std::to_string(y) + "_0" ));
                  plio_out_x[y] = output_plio::create("PLIO_out_X_" + std::to_string(y), adf::plio_64_bits, filenameOutX);
                  connect<>(dut_graph.x_out[y], plio_out_x[y].in[0]);
                }
                portIdx++;
            }
        }
#else
    // Define input/output PLIO ports
    std::array<input_plio, 1> plio_in_L;
    std::array<input_plio, 1> plio_in_y;
    std::array<output_plio, 1> plio_out_x;

    test_graph() {
        printf("========================\n");
        printf("== Substitution test.hpp parameters: ");
        printf(QUOTE(UUT_GRAPH));
        printf("\n");
        printf("========================\n");
        printf("Data type      = ");
        printf(QUOTE(DATA_TYPE));
        printf("\nINPUT_FILE_L   = ");
        printf(QUOTE(INPUT_FILE_L));
        printf("\nINPUT_FILE_Y   = ");
        printf(QUOTE(INPUT_FILE_Y));
        printf("\nOUTPUT_FILE    = ");
        printf(QUOTE(OUTPUT_FILE));
        printf("\n");
        printf("DIM          = %d \n", DIM_SIZE);
        printf("DIRECTION    = %d \n", SUBST_TYPE);
        printf("L_LEADING    = %d \n", L_LEADING);
        printf("NUM_FRAMES   = %d \n", NUM_FRAMES);
        printf("DIAG_INV     = %d \n", DIAG_INV);
        printf("NITER        = %d \n", NITER);

        // Instantiate the substitution graph
        xf::solver::aie::substitution::UUT_GRAPH<DATA_TYPE, DIM_SIZE, SUBST_TYPE, L_LEADING, GRID_DIM, NUM_FRAMES, DIAG_INV> dut_graph;

        plio_in_L[0] = input_plio::create("PLIO_inL_" + std::to_string(0), adf::plio_64_bits, QUOTE(INPUT_FILE_L));
        connect<>(plio_in_L[0].out[0], dut_graph.L_in[0]);
        plio_in_y[0] = input_plio::create("PLIO_inY_" + std::to_string(0), adf::plio_64_bits, QUOTE(INPUT_FILE_Y));
        connect<>(plio_in_y[0].out[0], dut_graph.y_in[0]);

        plio_out_x[0] = output_plio::create("PLIO_outX_" + std::to_string(0), adf::plio_64_bits, QUOTE(OUTPUT_FILE));
        connect<>(dut_graph.x_out[0], plio_out_x[0].in[0]);

#endif
    }
};

} // namespace xf::solver::aie::test

#endif // _TEST_HPP_
