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
#ifndef _DSPLIB_FIR_DECIMATE_ASYM_TRAITS_HPP_
#define _DSPLIB_FIR_DECIMATE_ASYM_TRAITS_HPP_

#include "device_defs.h"
#include <stdio.h>
#include <adf.h>

namespace xf {
namespace dsp {
namespace aie {
namespace fir {
namespace decimate_asym {
/*
Decimator Asymmetric FIR traits.
This file contains sets of overloaded, templatized and specialized templatized functions which
encapsulate properties of the intrinsics used by the main kernal class. Specifically,
this file does not contain any vector types or intrinsics since it is required for construction
and therefore must be suitable for the aie compiler graph-level compilation.
*/

enum eArchType { kArchBasic, kArchIncrStrobe, kArchStream, kArchPhaseParallel, kArchStreamPhaseParallel };

enum eDFType { kLowDF, kHighDF };

static constexpr unsigned int kDFThresh =
    6; // Decimation factors from this upwards require data to be compacted prior to the multiplication operation
static constexpr unsigned int kXoffsetRange = 16; // Xoffset is limited to 4-bits
static constexpr unsigned int kMaxColumns = 4;
static constexpr unsigned int kUpdWSize = 32;         // Upd_w size in Bytes (256bit) - const for all data/coeff types
static constexpr unsigned int kBuffSize128Byte = 128; // 1024-bit buffer size in Bytes
static constexpr unsigned int kBuffSize64Byte = 64;   // 512-bit buffer size in Bytes

// FIR element support type combination with AIE API
template <typename TT_DATA, typename TT_COEFF>
INLINE_DECL constexpr unsigned int fnFirDecAsymTypeSupport() {
    return fnUnsupportedTypeCombo<TT_DATA, TT_COEFF>();
}; // all combinations supported, apart from int16 data & int16 coeffs - due to HW __restriction (xsquare)

// function to return the number of lanes for a type combo
template <typename TT_DATA, typename TT_COEFF>
INLINE_DECL constexpr unsigned int fnNumLanesDecAsym() {
    return fnNumLanes384<TT_DATA, TT_COEFF>(); //
};

// function to return the number of columns for a type combo for the intrinsics used for this single rate asym FIR
template <typename TT_DATA, typename TT_COEFF>
INLINE_DECL constexpr unsigned int fnNumColumnsDecAsym() {
    return fnNumCols384<TT_DATA, TT_COEFF>();
};
template <>
INLINE_DECL constexpr unsigned int fnNumColumnsDecAsym<float, float>() {
    return 2;
};

// Function to determine how many bits to load each time data is fetched from the input window.
template <typename TT_DATA, typename TT_COEFF>
INLINE_DECL constexpr unsigned int fnLoadSizeDecAsym() {
    return 256;
};
// template <> INLINE_DECL constexpr unsigned int fnLoadSizeDecAsym<  int16,  int16>() {return 256;};
// template <> INLINE_DECL constexpr unsigned int fnLoadSizeDecAsym< cint16,  int16>() {return 256;};
// template <> INLINE_DECL constexpr unsigned int fnLoadSizeDecAsym< cint16, cint16>() {return 256;};
template <>
INLINE_DECL constexpr unsigned int fnLoadSizeDecAsym<int32, int16>() {
    return 128;
};
template <>
INLINE_DECL constexpr unsigned int fnLoadSizeDecAsym<int32, int32>() {
    return 128;
};
template <>
INLINE_DECL constexpr unsigned int fnLoadSizeDecAsym<cint32, int16>() {
    return 128;
};
template <>
INLINE_DECL constexpr unsigned int fnLoadSizeDecAsym<cint32, cint16>() {
    return 128;
};
template <>
INLINE_DECL constexpr unsigned int fnLoadSizeDecAsym<cint32, int32>() {
    return 128;
};
template <>
INLINE_DECL constexpr unsigned int fnLoadSizeDecAsym<cint32, cint32>() {
    return 128;
};
// template <> INLINE_DECL constexpr unsigned int fnLoadSizeDecAsym< float,  float>() {return 128;};
// template <> INLINE_DECL constexpr unsigned int fnLoadSizeDecAsym< cfloat,  float>() {return 128;};
// template <> INLINE_DECL constexpr unsigned int fnLoadSizeDecAsym< cfloat, cfloat>() {return 128;};

// Function returns window access size, the memory data path is either 128-bits or 256-bits wide for vector operations.
template <typename TT_DATA, typename TT_COEFF>
INLINE_DECL constexpr unsigned int fnWinAccessByteSize() {
    return 16;
};

template <typename TT_DATA>
INLINE_DECL constexpr unsigned int fnDataLoadsInRegDecAsym() {
    // To fill a full 1024-bit input vector register using a 256-bit upd_w command it takes 4 load operations.
    // Always return 4, the exception (handled by explicit template instantiation)
    // would be needed it a different command was used (e.g. upd_v, upd_x).
    return 4;
}

// Parameter to constant resolution functions
template <typename TT_DATA>
INLINE_DECL constexpr unsigned int fnDataLoadVsizeDecAsym() {
    return (kUpdWSize /
            sizeof(TT_DATA)); // again, if some case requires 128 bit loads, specialize this function for the exception
}

// function to return the number of samples in an output vector for a type combo
template <typename TT_DATA, typename TT_COEFF>
INLINE_DECL constexpr unsigned int fnVOutSizeDecAsym() {
    return fnNumLanesDecAsym<TT_DATA, TT_COEFF>();
};

template <unsigned int TP_DECIMATE_FACTOR,
          unsigned int m_kLanes,
          unsigned int streamInitNullAccs,
          int TP_MODIFY_MARGIN_OFFSET,
          unsigned int dataOffsetNthKernel,
          unsigned int m_kStreamLoadVsize,
          unsigned int TP_FIR_RANGE_LEN,
          unsigned int m_kFirRangeOffset,
          unsigned int m_kFirRangeOffsetLastKernel>
INLINE_DECL constexpr unsigned int getInitDataNeeded() {
    constexpr int dataNeededLastKernel =
        1 + TP_DECIMATE_FACTOR * (m_kLanes - 1) + (streamInitNullAccs * TP_DECIMATE_FACTOR * m_kLanes);
    constexpr unsigned int kMinDataNeeded = (TP_MODIFY_MARGIN_OFFSET + dataNeededLastKernel - dataOffsetNthKernel);
    constexpr unsigned int kMinDataLoaded = CEIL(kMinDataNeeded, m_kStreamLoadVsize);
    constexpr unsigned int kMinDataLoadCycles = kMinDataLoaded / m_kStreamLoadVsize;

    /* In a non-cascaded design or in the last kernel in a chain of cascaded kernels this gives the maximum data that
       needs to fit in the buffer.
       For ex, a cint32 data type which needs a single data read of 4 data samples for the first lane of outputs would
       look like this after the first
       read and before any computation is performed :
       sbuff =  1 2 3 4  0 0 0 0  0 0 0 0  0 0 0 0

       Since the data pointer for the mac operations increment by one after every MAC, and we want to point to the first
       new element after (TP_FIR_RANGE_LEN - 1) MACs
       (the previous MACs would use margin samples), initDataNeeded would need (enough samples for the margin samples) +
       (enough data to perform first computation).

    */
    constexpr unsigned int m_kInitDataNeededNoCasc = TP_FIR_RANGE_LEN - 1 + kMinDataLoadCycles * m_kStreamLoadVsize;
    /* when kernels are cascaded, the number of margin samples can be more than (TP_FIR_RANGE_LEN - 1).
       The first output sample produced by this kernel needs  (streamInitNullAccs * TP_DECIMATE_FACTOR * m_kVOutSize)
       input data,
       but the first input data sample that this kernel processes is the data corresponding to the
       (dataOffsetNthKernel)th coefficient.
       The data register needs to accommodate this difference in the margin samples.
    */
    constexpr unsigned int m_kInitDataNeeded =
        m_kInitDataNeededNoCasc + (dataOffsetNthKernel - streamInitNullAccs * TP_DECIMATE_FACTOR * m_kLanes);
    return m_kInitDataNeeded;
}

template <unsigned int TP_FIR_LEN, unsigned int TP_FIR_RANGE_LEN, unsigned int m_kFirRangeOffset>
INLINE_DECL constexpr unsigned int getDataOffset() {
    constexpr int dataOffsetNthKernel = (TP_FIR_LEN - TP_FIR_RANGE_LEN - m_kFirRangeOffset);
    return dataOffsetNthKernel;
}

template <unsigned int dataOffsetNthKernel, unsigned int TP_DECIMATE_FACTOR, unsigned int m_kVOutSize>
INLINE_DECL constexpr unsigned int getInitNullAccs() {
    constexpr int emptyInitLanes = CEIL(dataOffsetNthKernel, TP_DECIMATE_FACTOR) / TP_DECIMATE_FACTOR;
    constexpr int streamInitNullAccs =
        emptyInitLanes / m_kVOutSize; // Number of Null Mac Vectors sent as partial prouducts over cascade.
    return streamInitNullAccs;
}
// Function to return Lsize - the number of interations required to cover the input window.
template <unsigned int TP_INPUT_WINDOW_VSIZE, unsigned int TP_DECIMATE_FACTOR, unsigned int m_kVOutSize>
INLINE_DECL constexpr unsigned int fnLsize() {
    return (TP_INPUT_WINDOW_VSIZE / TP_DECIMATE_FACTOR) / m_kVOutSize;
}

#define NOT_SUPPORTED 0
#define SUPPORTED 1

// IncrStrobe architecture disabled for cint32 data & coeff type combination. Reverting to basic arch.
template <typename TT_DATA, typename TT_COEFF>
INLINE_DECL constexpr unsigned int fnFirDecIncStrSupported() {
    return SUPPORTED;
};
template <>
INLINE_DECL constexpr unsigned int fnFirDecIncStrSupported<cint32, cint32>() {
    return NOT_SUPPORTED;
};

// Max Xoffset range for cfloat data type is limited to 3-bits. Otherwise 4-bits.
template <typename TT_DATA>
INLINE_DECL constexpr unsigned int fnMaxXoffsetRange() {
    return 16;
};
template <>
INLINE_DECL constexpr unsigned int fnMaxXoffsetRange<cfloat>() {
    return 8;
};

template <typename TT_DATA, typename TT_COEFF, unsigned int TP_DECIMATE_FACTOR>
static constexpr unsigned int getStreamReadWidth() {
    // constexpr unsigned int  StreamReadWidth    = fnStreamReadWidth<TT_DATA, TT_COEFF>();
    constexpr unsigned int StreamReadWidth =
        (TP_DECIMATE_FACTOR % 2 == 0) ? 256 : fnStreamReadWidth<TT_DATA, TT_COEFF>();
    return StreamReadWidth;
}

template <typename TT_DATA, typename TT_COEFF, unsigned int TP_DECIMATE_FACTOR>
static constexpr unsigned int getKernelStreamLoadVsize() {
    constexpr unsigned int StreamReadWidth = getStreamReadWidth<TT_DATA, TT_COEFF, TP_DECIMATE_FACTOR>();
    return (StreamReadWidth / 8 / sizeof(TT_DATA));
}
}
}
}
}
} // namespaces
#endif // _DSPLIB_FIR_DECIMATE_ASYM_TRAITS_HPP_
