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
 * @file cscKernel.hpp
 * @brief SPARSE Level 2 template functions for building kernels.
 *
 * This file is part of Vitis SPARSE Library.
 */

#ifndef XF_SPARSE_CSCKERNEL_HPP
#define XF_SPARSE_CSCKERNEL_HPP

#ifndef __cplusplus
#error "SPARSE Library only works with C++."
#endif

#include <cstdint>
#include "ap_axi_sdata.h"
#include "ap_int.h"
#include "hls_stream.h"
#ifndef __SYNTHESIS__
#include <iostream>
#endif
#include "xf_sparse.hpp"
#include "cscMatMoverL2.hpp"

namespace xf {
namespace sparse {
using namespace xf::blas;

template <unsigned int t_ParEntries, unsigned int t_MemBits, unsigned int t_DataBits, typename t_PktType>
void memStr2DatPktStr(hls::stream<ap_uint<t_MemBits> >& p_memStr,
                      const unsigned int p_memBlocks,
                      hls::stream<t_PktType>& p_datPktStr) {
#ifndef __SYNTHESIS__
    assert(t_MemBits >= (t_DataBits * t_ParEntries));
    assert(t_MemBits % (t_DataBits * t_ParEntries) == 0);
#endif

    const unsigned int t_DataWords = t_MemBits / (t_DataBits * t_ParEntries);
    const unsigned int t_DataWordBits = t_DataBits * t_ParEntries;

    for (unsigned int i = 0; i < p_memBlocks; ++i) {
#pragma HLS PIPELINE II = t_DataWords
        ap_uint<t_MemBits> l_memVal = p_memStr.read();
        for (unsigned int j = 0; j < t_DataWords; ++j) {
            ap_uint<t_DataWordBits> l_datVal = l_memVal.range((j + 1) * t_DataWordBits - 1, j * t_DataWordBits);
            t_PktType l_pkt;
            l_pkt.data = l_datVal;
            p_datPktStr.write(l_pkt);
        }
    }
}

template <unsigned int t_LogParEntries,
          typename t_PktType,
          typename t_DataType,
          typename t_IndexType = unsigned int,
          unsigned int t_DataBits = 32,
          unsigned int t_IndexBits = 32>
void formRowEntriesPkt(const unsigned int p_nnzBlocks,
                       hls::stream<ap_uint<t_DataBits*(1 << t_LogParEntries)> >& p_nnzValStr,
                       hls::stream<t_PktType>& p_nnzColValPktStr,
                       hls::stream<ap_uint<t_IndexBits*(1 << t_LogParEntries)> >& p_rowIndexStr,
                       hls::stream<ap_uint<t_DataBits + t_IndexBits> > p_rowEntryStr[1 << t_LogParEntries],
                       hls::stream<ap_uint<1> > p_isEndStr[1 << t_LogParEntries]) {
    const unsigned int t_ParEntries = 1 << t_LogParEntries;
    for (unsigned int i = 0; i < p_nnzBlocks; ++i) {
#pragma HLS PIPELINE
        ap_uint<t_DataBits * t_ParEntries> l_nnzBits;
        ap_uint<t_DataBits * t_ParEntries> l_nnzColBits;
        ap_uint<t_DataBits * t_ParEntries> l_rowIndexBits;

        RowEntry<t_DataType, t_IndexType> l_rowEntry[t_ParEntries];
        l_nnzBits = p_nnzValStr.read();
        t_PktType l_nnzColBitsPkt = p_nnzColValPktStr.read();
        l_nnzColBits = l_nnzColBitsPkt.data;
        l_rowIndexBits = p_rowIndexStr.read();
        WideType<t_DataType, t_ParEntries> l_nnzVal(l_nnzBits);
        WideType<t_DataType, t_ParEntries> l_nnzColVal(l_nnzColBits);
        WideType<t_IndexType, t_ParEntries> l_rowIndex(l_rowIndexBits);
#pragma HLS ARRAY_PARTITION variable = l_nnzVal complete
#pragma HLS ARRAY_PARTITION variable = l_nnzColVal complete
#pragma HLS ARRAY_PARTITION variable = l_rowIndex complete
#pragma HLS ARRAY_PARTITION variable = l_rowEntry complete

        for (unsigned int j = 0; j < t_ParEntries; ++j) {
            l_rowEntry[j].getVal() = l_nnzVal[j] * l_nnzColVal[j];
            l_rowEntry[j].getRow() = l_rowIndex[j];
            p_rowEntryStr[j].write(l_rowEntry[j].toBits());
            p_isEndStr[j].write(0);
        }
    }
    for (unsigned int j = 0; j < t_ParEntries; ++j) {
        p_isEndStr[j].write(1);
    }
}

template <unsigned int t_LogParEntries,
          typename t_PktType,
          typename t_DataType,
          typename t_IndexType = unsigned int,
          unsigned int t_DataBits = 32,
          unsigned int t_IndexBits = 32>
void xBarRowPkt(const unsigned int p_nnzBlocks,
                hls::stream<ap_uint<t_DataBits*(1 << t_LogParEntries)> >& p_nnzValStr,
                hls::stream<t_PktType>& p_nnzColValPktStr,
                hls::stream<ap_uint<t_IndexBits*(1 << t_LogParEntries)> >& p_rowIndexStr,
                hls::stream<ap_uint<t_DataBits + t_IndexBits> > p_rowEntryStr[1 << t_LogParEntries],
                hls::stream<ap_uint<1> > p_isEndStr[1 << t_LogParEntries]) {
    const unsigned int t_ParEntries = 1 << t_LogParEntries;
    hls::stream<ap_uint<t_DataBits + t_IndexBits> > l_rowEntryStr[t_ParEntries];
#pragma HLS STREAM variable = l_rowEntryStr depth = 4
    hls::stream<ap_uint<1> > l_isEndStr[t_ParEntries];
    hls::stream<ap_uint<t_DataBits + t_IndexBits> > l_splittedRowEntryStr[t_ParEntries][t_ParEntries];
    hls::stream<ap_uint<1> > l_isEndSplitStr[t_ParEntries];
#pragma HLS DATAFLOW
    formRowEntriesPkt<t_LogParEntries, t_PktType, t_DataType, t_IndexType, t_DataBits, t_IndexBits>(
        p_nnzBlocks, p_nnzValStr, p_nnzColValPktStr, p_rowIndexStr, l_rowEntryStr, l_isEndStr);
    for (unsigned int i = 0; i < t_ParEntries; ++i) {
#pragma HLS UNROLL
        xBarRowSplit<t_LogParEntries, t_DataType, t_IndexType, t_DataBits, t_IndexBits>(
            l_rowEntryStr[i], l_isEndStr[i], l_splittedRowEntryStr[i], l_isEndSplitStr[i]);
    }
    xBarRowMerge<t_LogParEntries, t_DataType, t_IndexType, t_DataBits, t_IndexBits>(
        l_splittedRowEntryStr, l_isEndSplitStr, p_rowEntryStr, p_isEndStr);
}

template <unsigned int t_ParEntries,
          unsigned int t_ParGroups,
          typename t_PktType,
          typename t_DataType,
          typename t_IndexType,
          unsigned int t_DataBits = 32>
void rowAggPkt(const unsigned int p_rowBlocks,
               hls::stream<ap_uint<t_DataBits> > p_rowValStr[t_ParEntries][t_ParGroups],
               hls::stream<t_PktType>& p_rowAggPktStr) {
    for (unsigned int i = 0; i < p_rowBlocks; ++i) {
        for (unsigned int g = 0; g < t_ParGroups; ++g) {
#pragma HLS PIPELINE
            ap_uint<t_DataBits * t_ParEntries> l_valOut;
            for (unsigned int b = 0; b < t_ParEntries; ++b) {
                ap_uint<t_DataBits> l_val = p_rowValStr[b][g].read();
                l_valOut.range((b + 1) * t_DataBits - 1, b * t_DataBits) = l_val;
            }
            t_PktType l_pktOut;
            l_pktOut.data = l_valOut;
            p_rowAggPktStr.write(l_pktOut);
        }
    }
}
template <unsigned int t_MaxRowBlocks,
          unsigned int t_LogParEntries,
          unsigned int t_LogParGroups,
          typename t_PktType,
          typename t_DataType,
          typename t_IndexType = unsigned int,
          unsigned int t_DataBits = 32,
          unsigned int t_IndexBits = 32>
void cscRowPkt(const unsigned int p_nnzBlocks,
               const unsigned int p_rowBlocks,
               hls::stream<ap_uint<t_DataBits*(1 << t_LogParEntries)> >& p_nnzValStr,
               hls::stream<t_PktType>& p_nnzColPktStr,
               hls::stream<ap_uint<t_IndexBits*(1 << t_LogParEntries)> >& p_rowIndexStr,
               hls::stream<t_PktType>& p_rowAggPktStr) {
    const unsigned int t_ParEntries = 1 << t_LogParEntries;
    const unsigned int t_ParGroups = 1 << t_LogParGroups;
    const unsigned int t_RowOffsetBits = t_IndexBits - t_LogParEntries - t_LogParGroups;

    hls::stream<ap_uint<t_DataBits + t_IndexBits> > l_xBarRowDatStr[t_ParEntries];
    hls::stream<ap_uint<1> > l_xBarRowContStr[t_ParEntries];
    hls::stream<ap_uint<t_DataBits + t_RowOffsetBits> > l_rowIntDatStr[t_ParEntries][t_ParGroups];
    hls::stream<ap_uint<1> > l_rowIntContStr[t_ParEntries][t_ParGroups];
    hls::stream<ap_uint<t_DataBits> > l_rowValStr[t_ParEntries][t_ParGroups];

#pragma HLS DATAFLOW
    xBarRowPkt<t_LogParEntries, t_PktType, t_DataType, t_IndexType, t_DataBits, t_IndexBits>(
        p_nnzBlocks, p_nnzValStr, p_nnzColPktStr, p_rowIndexStr, l_xBarRowDatStr, l_xBarRowContStr);

    for (unsigned int i = 0; i < t_ParEntries; ++i) {
#pragma HLS UNROLL
        rowInterleave<t_LogParEntries, t_LogParGroups, t_DataType, t_IndexType, t_DataBits, t_IndexBits>(
            l_xBarRowDatStr[i], l_xBarRowContStr[i], l_rowIntDatStr[i], l_rowIntContStr[i]);

        for (unsigned int j = 0; j < t_ParGroups; ++j) {
#pragma HLS UNROLL
            rowAcc<t_MaxRowBlocks, 1, t_LogParGroups, t_DataType, t_IndexType, t_DataBits, t_RowOffsetBits>(
                p_rowBlocks, l_rowIntDatStr[i][j], l_rowIntContStr[i][j], l_rowValStr[i][j]);
        }
    }

    rowAggPkt<t_ParEntries, t_ParGroups, t_PktType, t_DataType, t_IndexType, t_DataBits>(p_rowBlocks, l_rowValStr,
                                                                                         p_rowAggPktStr);
}

template <unsigned int t_ParEntries, unsigned int t_MemBits, unsigned int t_DataBits, typename t_PktType>
void datPktStr2MemStr(hls::stream<t_PktType>& p_datPktStr,
                      const unsigned int p_memBlocks,
                      hls::stream<ap_uint<t_MemBits> >& p_memStr) {
#ifndef __SYNTHESIS__
    assert(t_MemBits >= (t_DataBits * t_ParEntries));
    assert(t_MemBits % (t_DataBits * t_ParEntries) == 0);
#endif

    const unsigned int t_DataWords = t_MemBits / (t_DataBits * t_ParEntries);
    const unsigned int t_DataWordBits = t_DataBits * t_ParEntries;

    for (unsigned int i = 0; i < p_memBlocks; ++i) {
#pragma HLS PIPELINE II = t_DataWords
        ap_uint<t_MemBits> l_memVal;
        for (unsigned int j = 0; j < t_DataWords; ++j) {
            t_PktType l_datPkt = p_datPktStr.read();
            ap_uint<t_DataWordBits> l_datVal = l_datPkt.data;
            l_memVal.range((j + 1) * t_DataWordBits - 1, j * t_DataWordBits) = l_datVal;
        }
        p_memStr.write(l_memVal);
    }
}

template <unsigned int t_MemBits, unsigned int t_ParEntries, unsigned int t_DataBits, typename t_PktType>
void loadDat2PktStr(const ap_uint<t_MemBits>* p_memPtr,
                    const unsigned int p_memBlocks,
                    hls::stream<t_PktType>& p_datPktStr) {
    hls::stream<ap_uint<t_MemBits> > l_memStr;
#pragma HLS DATAFLOW
    loadMemBlocks<t_MemBits>(p_memPtr, p_memBlocks, l_memStr);
    memStr2DatPktStr<t_ParEntries, t_MemBits, t_DataBits, t_PktType>(l_memStr, p_memBlocks, p_datPktStr);
}

template <unsigned int t_MaxRowBlocks,
          unsigned int t_LogParEntries,
          unsigned int t_LogParGroups,
          typename t_DataType,
          typename t_IndexType,
          unsigned int t_DataBits,
          unsigned int t_IndexBits,
          unsigned int t_MemBits,
          typename t_PktType>
void cscRowPkt(const ap_uint<t_MemBits>* p_aNnzIdx,
               const unsigned int p_memBlocks,
               const unsigned int p_nnzBlocks,
               const unsigned int p_rowBlocks,
               hls::stream<t_PktType>& p_nnzColValPktStr,
               hls::stream<t_PktType>& p_rowAggPktStr) {
    const unsigned int t_ParEntries = 1 << t_LogParEntries;
    hls::stream<ap_uint<t_DataBits * t_ParEntries> > l_nnzStr;
    hls::stream<ap_uint<t_IndexBits * t_ParEntries> > l_idxStr;
#pragma HLS DATAFLOW

    loadNnzIdx<1 << t_LogParEntries, t_MemBits, t_DataBits, t_IndexBits>(p_aNnzIdx, p_memBlocks, l_nnzStr, l_idxStr);

    cscRowPkt<t_MaxRowBlocks, t_LogParEntries, t_LogParGroups, t_PktType, t_DataType, t_IndexType, t_DataBits,
              t_IndexBits>(p_nnzBlocks, p_rowBlocks, l_nnzStr, p_nnzColValPktStr, l_idxStr, p_rowAggPktStr);
}

template <unsigned int t_ParEntries, unsigned int t_MemBits, unsigned int t_DataBits, typename t_PktType>
void storeDatPkt(hls::stream<t_PktType>& p_datPktStr, const unsigned int p_memBlocks, ap_uint<t_MemBits>* p_memPtr) {
    hls::stream<ap_uint<t_MemBits> > l_memStr;

#pragma HLS DATAFLOW

    datPktStr2MemStr<t_ParEntries, t_MemBits, t_DataBits, t_PktType>(p_datPktStr, p_memBlocks, l_memStr);

    storeMemBlocks<t_MemBits>(l_memStr, p_memBlocks, p_memPtr);
}

template <unsigned int t_LogParEntries,
          typename t_DataType,
          typename t_IndexType,
          unsigned int t_DataBits,
          unsigned int t_IndexBits,
          typename t_DataPktType,
          typename t_IndexPktType>
void xBarColPkt(const unsigned int p_colPtrBlocks,
                const unsigned int p_nnzBlocks,
                hls::stream<t_DataPktType>& p_colValStr,
                hls::stream<t_IndexPktType>& p_colPtrStr,
                hls::stream<t_DataPktType>& p_nnzColValStr) {
    const unsigned int t_ParEntries = 1 << t_LogParEntries;
    const unsigned int t_IndexBusBits = t_IndexBits * t_ParEntries;
    const unsigned int t_DataBusBits = t_DataBits * t_ParEntries;

    hls::stream<ap_uint<t_IndexBusBits> > l_colPtrStr;
    hls::stream<ap_uint<t_DataBusBits> > l_colValStr;
    hls::stream<ap_uint<t_DataBusBits> > l_nnzColValStr;

#pragma HLS DATAFLOW
    datPktStr2MemStr<t_ParEntries, t_IndexBusBits, t_IndexBits, t_IndexPktType>(p_colPtrStr, p_colPtrBlocks,
                                                                                l_colPtrStr);

    datPktStr2MemStr<t_ParEntries, t_DataBusBits, t_DataBits, t_DataPktType>(p_colValStr, p_colPtrBlocks, l_colValStr);

    xBarCol<t_LogParEntries, t_DataType, t_IndexType, t_DataBits, t_IndexBits>(p_colPtrBlocks, p_nnzBlocks, l_colPtrStr,
                                                                               l_colValStr, l_nnzColValStr);

    memStr2DatPktStr<t_ParEntries, t_DataBusBits, t_DataBits, t_DataPktType>(l_nnzColValStr, p_nnzBlocks,
                                                                             p_nnzColValStr);
}

template <unsigned int t_MemBits,
          unsigned int t_ParEntries,
          unsigned int t_DataBits,
          unsigned int t_IndexBits,
          typename t_DataPktType,
          typename t_IndexPktType>
void loadColPtrVal2PktStr(const ap_uint<t_MemBits>* p_memPtr,
                          const unsigned int p_memBlocks,
                          hls::stream<t_DataPktType>& p_datPktStr,
                          hls::stream<t_IndexPktType>& p_idxPktStr) {
    hls::stream<ap_uint<t_MemBits> > l_memStr;
#pragma HLS DATAFLOW
    loadMemBlocks<t_MemBits>(p_memPtr, p_memBlocks, l_memStr);
    memStr2ColPtrValStr<t_MemBits, t_ParEntries, t_DataBits, t_IndexBits, t_DataPktType, t_IndexPktType>(
        l_memStr, p_memBlocks, p_datPktStr, p_idxPktStr);
}

template <unsigned int t_MaxColMemBlocks,
          unsigned int t_MemBits,
          unsigned int t_ParEntries,
          unsigned int t_DataBits,
          unsigned int t_IndexBits,
          typename t_DataPktType,
          typename t_IndexPktType>
void loadCol2PktStrStep(const ap_uint<t_MemBits>* p_memColVal,
                        const ap_uint<t_MemBits>* p_memColPtr,
                        const unsigned int p_memBlocks,
                        const unsigned int p_numTrans,
                        hls::stream<t_DataPktType>& p_datPktStr,
                        hls::stream<t_IndexPktType>& p_idxPktStr) {
    hls::stream<ap_uint<t_MemBits> > l_memStr;
    hls::stream<ap_uint<t_MemBits> > l_valPtrStr;
#pragma HLS DATAFLOW
    loadColValPtrBlocks<t_MemBits>(p_memColVal, p_memColPtr, p_memBlocks, l_memStr);
    bufferTransCols<t_MaxColMemBlocks, t_MemBits>(p_memBlocks, p_numTrans, l_memStr, l_valPtrStr);
    memStr2ColPtrValStr<t_MemBits, t_ParEntries, t_DataBits, t_IndexBits, t_DataPktType, t_IndexPktType>(
        l_valPtrStr, p_memBlocks * 2, p_datPktStr, p_idxPktStr);
}

template <typename t_ParamPktType, typename t_DataPktType, unsigned int t_HbmChannels, unsigned int t_MemBits>
void loadCol(ap_uint<32>* p_paramPtr,
             ap_uint<t_MemBits>* p_colValPtr,
             ap_uint<t_MemBits>* p_nnzColPtr,
             hls::stream<t_ParamPktType>& p_colVecParamPktStr,
             hls::stream<t_DataPktType>& p_colVecPktStr,
             hls::stream<t_ParamPktType>& p_nnzColParamPktStr,
             hls::stream<t_DataPktType>& p_nnzColPktStr) {
    ap_uint<32> l_vecBlocks;
    ap_uint<32> l_chBlocks[t_HbmChannels];

    ap_uint<32>* l_paramPtr = p_paramPtr;
    l_vecBlocks = l_paramPtr[0];
    t_ParamPktType l_paramPkt;
    l_paramPkt.data = l_vecBlocks;
    p_colVecParamPktStr.write(l_paramPkt);
    l_paramPtr++;
    for (unsigned int i = 0; i < t_HbmChannels; ++i) {
        for (unsigned int j = 0; j < 3; ++j) {
            // write l_vecBlocks and chBlocks, minColIdx, maxColIdx for each channel
            l_paramPkt.data = l_paramPtr[j];
            p_colVecParamPktStr.write(l_paramPkt);
            l_chBlocks[j] = (j == 0) ? l_paramPkt.data : 0;
        }
        l_paramPtr += 3;
    }

    for (unsigned int i = 0; i < t_HbmChannels; ++i) {
        l_paramPkt.data = l_chBlocks[i];
        p_nnzColParamPktStr.write(l_paramPkt);
    }

    for (unsigned int i = 0; i < l_vecBlocks; ++i) {
#pragma HLS PIPELINE
        t_DataPktType l_datPkt;
        l_datPkt.data = p_colValPtr[i];
        p_colVecPktStr.write(l_datPkt);
    }

    unsigned int l_nnzColBlocks = 0;
    for (unsigned int i = 0; i < t_HbmChannels; ++i) {
        l_nnzColBlocks += l_chBlocks[i];
    }
    for (unsigned int i = 0; i < l_nnzColBlocks; ++i) {
#pragma HLS PIPELINE
        t_DataPktType l_datPkt;
        l_datPkt.data = p_nnzColPtr[i];
        p_nnzColPktStr.write(l_datPkt);
    }
}

template <typename t_ParamPktType,
          typename t_MemPktType,
          unsigned int t_MaxColBlocks,
          unsigned int t_HbmChannels,
          unsigned int t_ParEntries,
          unsigned int t_MemBits,
          unsigned int t_DataBits>
void bufTransColVec(hls::stream<t_ParamPktType>& p_colVecParamPktStr,
                    hls::stream<t_MemPktType>& p_colVecPktStr,
                    hls::stream<ap_uint<32> > p_paramOutStr[t_HbmChannels],
                    hls::stream<ap_uint<t_DataBits * t_ParEntries> > p_datOutStr[t_HbmChannels]) {
    const unsigned int t_ParWords = t_MemBits / (t_DataBits * t_ParEntries);
    const unsigned int t_MaxColParBlocks = t_MaxColBlocks * t_ParWords;

    hls::stream<ap_uint<32> > l_paramStr;
    hls::stream<ap_uint<t_DataBits * t_ParEntries> > l_datStr;
#pragma HLS DATAFLOW
    readColVecPkt<t_ParamPktType, t_MemPktType, t_HbmChannels, t_ParEntries, t_MemBits, t_DataBits>(
        p_colVecParamPktStr, p_colVecPktStr, l_paramStr, l_datStr);

    dispCol<t_MaxColParBlocks, t_HbmChannels, t_ParEntries, t_DataBits>(l_paramStr, l_datStr, p_paramOutStr,
                                                                        p_datOutStr);
}

template <typename t_ParamPktType,
          typename t_MemPktType,
          unsigned int t_MaxColBlocks,
          unsigned int t_HbmChannels,
          unsigned int t_ParEntries,
          unsigned int t_MemBits,
          unsigned int t_DataBits>
void bufTransNnzCol(hls::stream<t_ParamPktType>& p_nnzColParamPktStr,
                    hls::stream<t_MemPktType>& p_nnzColPktStr,
                    hls::stream<ap_uint<32> > p_paramOutStr[t_HbmChannels],
                    hls::stream<ap_uint<t_DataBits * t_ParEntries> > p_datOutStr[t_HbmChannels]) {
    const unsigned int t_ParWords = t_MemBits / (t_DataBits * t_ParEntries);
    const unsigned int t_MaxColParBlocks = t_MaxColBlocks * t_ParWords;

    hls::stream<ap_uint<32> > l_paramStr;
    hls::stream<ap_uint<t_DataBits * t_ParEntries> > l_datStr;
#pragma HLS DATAFLOW
    readNnzColPkt<t_ParamPktType, t_MemPktType, t_HbmChannels, t_ParEntries, t_MemBits, t_DataBits>(
        p_nnzColParamPktStr, p_nnzColPktStr, l_paramStr, l_datStr);

    dispNnzCol<t_MaxColParBlocks, t_HbmChannels, t_ParEntries, t_DataBits>(l_paramStr, l_datStr, p_paramOutStr,
                                                                           p_datOutStr);
}
} // end namespace sparse
} // end namespace xf
#endif
