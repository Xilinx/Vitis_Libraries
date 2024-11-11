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

/**
 * @file qrd_graph.hpp
 * @brief This file captures the definition of the `L2` graph level class for the QR Decomposition with cfloat data
 *type.
 **/

#ifndef _XF_SOLVER_QRD_GRAPH_HPP_
#define _XF_SOLVER_QRD_GRAPH_HPP_

#include "qrd_kernel.hpp"

namespace xf {
namespace solver {
namespace internal {
class alongCascade {
   public:
    alongCascade(unsigned int start_column_id, unsigned int start_row_id) {
        s_row = start_row_id;
        s_col = start_column_id;
        c_row = s_row;
        c_col = s_col;
        if (s_row % 2 == 0) {
            dir_inc_c = true;
        } else {
            dir_inc_c = false;
        }
    }

    void next(unsigned int& row_id, unsigned int& col_id, bool if_print = false) {
        if (dir_inc_c) {
            if (c_col == (num_columns - 1)) {
                c_row++;
                dir_inc_c = false;
            } else {
                c_col++;
            }
        } else {
            if (c_col == 0) {
                c_row++;
                dir_inc_c = true;
            } else {
                c_col--;
            }
        }

        curr(row_id, col_id);

        if (if_print) {
            print();
        }
    }

    void curr(unsigned int& row_id, unsigned int& col_id, bool if_print = false) {
        row_id = c_row;
        col_id = c_col;

        if (if_print) {
            print();
        }
    }

   private:
    unsigned int s_row;
    unsigned int s_col;
    unsigned int c_row;
    unsigned int c_col;
    bool dir_inc_c;

    static const unsigned int num_rows = 8;
    static const unsigned int num_columns = 50;

    void print() { std::cout << "current tile stands on [" << c_row << "][" << c_col << "]." << std::endl; }
};
}

using namespace adf;

/**
 * @class QRDComplexFloat
 * @brief QR decomposition is a decomposition of a matrix A into a product A = QR of an orthonormal matrix Q and an
 *upper triangular matrix R.
 *
 * These are the templates to configure the function.
 * @tparam column_num describes the number of columns.
 * @tparam row_num describes the number of rows.
 * @tparam k_rep descripbes the number of input matrix.
 **/
template <int column_num, int row_num, int k_rep = 1>
class QRDComplexFloat : public adf::graph {
   public:
    /**
     * kernel instance.
     * The chain of kernels that will be created and mapped on AIE tiles.
     **/
    kernel m_k[column_num];
    /**
     * The input data to the function.
     **/
    input_port in_0;
    input_port in_1;
    /**
     * The output data to the function.
     **/
    output_port out_0;
    output_port out_1;

    /**
     * @brief This is the constructor function for the QRDComplexFloat.
     **/
    QRDComplexFloat() {
        for (int i = 0; i < column_num; i++) {
            m_k[i] = kernel::create_object<GramSchmidtKernelComplexFloat<row_num, column_num, k_rep> >(i);
            headers(m_k[i]) = {"qrd_kernel.hpp"};
            // source file
            source(m_k[i]) = "qrd_kernel.cpp";
            runtime<ratio>(m_k[i]) = 1.0;
            stack_size(m_k[i]) = 22000;

            if (i == 0) {
                connect<stream> net0(in_0, m_k[i].in[0]);
                connect<stream> net1(in_1, m_k[i].in[1]);
                fifo_depth(net0) = 16;
                fifo_depth(net1) = 16;
            } else {
                connect<stream> net0(m_k[i - 1].out[0], m_k[i].in[0]);
                connect<stream> net1(m_k[i - 1].out[1], m_k[i].in[1]);
                fifo_depth(net0) = 16;
                fifo_depth(net1) = 16;
            }
            if (i == column_num - 1) {
                connect<stream> net2(m_k[i].out[0], out_0);
                connect<stream> net3(m_k[i].out[1], out_1);
                fifo_depth(net2) = 16;
                fifo_depth(net3) = 16;
            }
        }
    }
};

/**
 * @class QRDComplexFloat_CASC
 *
 * These are the templates to configure the function.
 * @tparam column_num describes the number of columns.
 * @tparam row_num describes the number of rows.
 * @tparam k_rep descripbes the number of input matrix.
 **/
template <int column_num, int row_num, int k_rep = 1>
class QRDComplexFloat_CASC : public adf::graph {
   public:
    /**
     * kernel instance.
     * The chain of kernels that will be created and mapped on AIE tiles.
     **/
    kernel m_k[column_num];
    /**
     * The input data to the function.
     **/
    input_port in_0;
    input_port in_1;
    /**
     * The output data to the function.
     **/
    output_port out_0;
    output_port out_1;

    /**
     * @brief This is the constructor function for the QRDComplexFloat_CASC.
     **/
    QRDComplexFloat_CASC() {
        internal::alongCascade walker(44, 0);
        unsigned int r_id, c_id;

        for (int i = 0; i < column_num; i++) {
            if (i == 0) {
                m_k[i] = kernel::create_object<GramSchmidtKernelComplexFloat_Start<k_rep> >(column_num, row_num, i);
                walker.curr(r_id, c_id);
            } else if (i == column_num - 1) {
                m_k[i] = kernel::create_object<GramSchmidtKernelComplexFloat_End<k_rep> >(column_num, row_num, i);
                walker.next(r_id, c_id);
            } else {
                m_k[i] = kernel::create_object<GramSchmidtKernelComplexFloat_Mid<k_rep> >(column_num, row_num, i);
                walker.next(r_id, c_id);
            }
            headers(m_k[i]) = {"qrd_kernel.hpp"};
            source(m_k[i]) = "qrd_kernel.cpp";
            runtime<ratio>(m_k[i]) = 1.0;
            stack_size(m_k[i]) = 22000;
            location<kernel>(m_k[i]) = tile(c_id, r_id);

            if (i == 0) {
                connect<stream> net0(in_0, m_k[i].in[0]);
                connect<stream> net1(in_1, m_k[i].in[1]);
            } else {
                connect<cascade> net0(m_k[i - 1].out[0], m_k[i].in[0]);
            }
            if (i == column_num - 1) {
                connect<stream> net2(m_k[i].out[0], out_0);
                connect<stream> net3(m_k[i].out[1], out_1);
            }
        }
    }
};

} // namespace solver
} // namespace xf

#endif
