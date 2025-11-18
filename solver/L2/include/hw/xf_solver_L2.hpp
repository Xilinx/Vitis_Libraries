/*
 * Copyright (C) 2019-2022, Xilinx, Inc.
 * Copyright (C) 2022-2025, Advanced Micro Devices, Inc.
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
 * @file xf_solver_L2.h
 * @brief Top-levle header for XF Solver Libaray level-2.
 */

#ifndef _XF_SOLVER_L2_HPP_
#define _XF_SOLVER_L2_HPP_

// Matrix decomposition
#include "MatrixDecomposition/potrf.hpp"
#include "MatrixDecomposition/getrf_nopivot.hpp"
#include "MatrixDecomposition/getrf.hpp"

#include "MatrixDecomposition/geqrf.hpp"
#include "MatrixDecomposition/gesvdj.hpp"
#include "MatrixDecomposition/gesvj.hpp"

// Linear solver
#include "LinearSolver/pomatrixinverse.hpp"
#include "LinearSolver/gematrixinverse.hpp"
#include "LinearSolver/trtrs.hpp"
#include "LinearSolver/polinearsolver.hpp"
#include "LinearSolver/gelinearsolver.hpp"
#include "LinearSolver/gtsv_pcr.hpp"

// Eigen value solver
#include "EigenSolver/syevj.hpp"

#endif
