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
 * @file rowMultAcc.hpp
 * @brief common definitions used in host coce.
 *
 * This file is part of Vitis SPARSE Library.
 */

#ifndef XF_SPARSE_L2_DEFINITIONS_HPP
#define XF_SPARSE_L2_DEFINITIONS_HPP

#include "gen_cscmv.hpp"
#include "mtxFile.hpp"

namespace xf {
namespace sparse {

// common types
typedef NnzUnit<SPARSE_dataType, SPARSE_indexType> NnzUnitType;
typedef MtxFile<SPARSE_dataType, SPARSE_indexType> MtxFileType;
typedef Program<SPARSE_pageSize> ProgramType;
typedef CscMat<SPARSE_dataType, SPARSE_indexType, SPARSE_parEntries, SPARSE_hbmMemBits, SPARSE_ddrMemBits> CscMatType;

typedef ColVec<SPARSE_dataType, SPARSE_ddrMemBits> ColVecType;

typedef CscPartition<SPARSE_dataType,
                     SPARSE_indexType,
                     SPARSE_parEntries,
                     SPARSE_hbmMemBits,
                     SPARSE_ddrMemBits,
                     SPARSE_hbmChannels>
    CscPartitionType;

typedef GenCscMat<SPARSE_dataType,
                  SPARSE_indexType,
                  SPARSE_parEntries,
                  SPARSE_parGroups,
                  SPARSE_hbmMemBits,
                  SPARSE_ddrMemBits,
                  SPARSE_pageSize>
    GenCscMatType;

typedef GenVec<SPARSE_dataType, SPARSE_parEntries, SPARSE_ddrMemBits, SPARSE_pageSize> GenVecType;

typedef GenCscPartition<SPARSE_dataType,
                        SPARSE_indexType,
                        SPARSE_parEntries,
                        SPARSE_parGroups,
                        SPARSE_hbmMemBits,
                        SPARSE_ddrMemBits,
                        SPARSE_maxColMemBlocks,
                        SPARSE_maxRowBlocks,
                        SPARSE_hbmChannels,
                        SPARSE_hbmChannelMegaBytes,
                        SPARSE_pageSize>
    GenCscPartitionType;
} // end namespace sparse
} // end namespace xf
#endif
