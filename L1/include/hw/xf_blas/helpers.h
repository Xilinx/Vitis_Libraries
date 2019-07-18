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

/**
 * @file helpers.h
 * @brief common datatypes for L1 modules.
 * 
 * This file is part of XF BLAS Library.
 */

#ifndef XF_BLAS_HELPERS_H
#define XF_BLAS_HELPERS_H

/*        UTILITY FUNCTIONS             */
#include "helpers/utils/types.h"
#include "helpers/utils/utils.h"


/*        DATA MOVER            */
#include "helpers/dataMover/dataMover.h"
#include "helpers/dataMover/sbMatMover.hpp"


/*        HELPER FUNCTIONS             */
#include "helpers/funcs/maxmin.h"
#include "helpers/funcs/abs.h"
#include "helpers/funcs/sum.h"
#include "helpers/funcs/mul.h"

#endif
