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
 * @file types.hpp
 * @brief common datatypes for L1 modules.
 *
 * This file is part of Vitis BLAS Library.
 */

#ifndef XF_BLAS_TYPES_HPP
#define XF_BLAS_TYPES_HPP

#include <stdint.h>
#include <ostream>
#include <iomanip>
#include <iostream>
#include "hls_math.h"
#include "hls_stream.h"
#include "ap_int.h"
#include "ap_shift_reg.h"

namespace xf {

namespace blas {

template <typename T, unsigned int t_Width, unsigned int t_DataWidth = sizeof(T) * 8>
class WideType {
   private:
    T m_Val[t_Width];
    static const unsigned int t_4k = 4096;
    static const unsigned int FLOAT_WIDTH = 7;

   public:
    static const unsigned int t_TypeWidth = t_Width * t_DataWidth;
    typedef ap_uint<t_TypeWidth> t_TypeInt;
    typedef T DataType;
    static const unsigned int t_WidthS = t_Width;
    static const unsigned int t_per4k = t_4k / t_DataWidth / t_Width * 8;

   public:
    T& getVal(unsigned int i) {
#ifndef __SYNTHESIS__
        assert(i < t_Width);
#endif
        return (m_Val[i]);
    }
    T& operator[](unsigned int p_Idx) {
#ifndef __SYNTHESIS__
        assert(p_Idx < t_Width);
#endif
        return (m_Val[p_Idx]);
    }
    const T& operator[](unsigned int p_Idx) const {
#ifndef __SYNTHESIS__
        assert(p_Idx < t_Width);
#endif
        return (m_Val[p_Idx]);
    }
    T* getValAddr() { return (&m_Val[0]); }

    WideType() {
#pragma HLS ARRAY_PARTITION variable = m_Val complete dim = 1
    }

    WideType(const WideType& wt) {
#pragma HLS ARRAY_PARTITION variable = m_Val complete dim = 1
        for (int i = 0; i < t_Width; i++)
#pragma HLS UNROLL
            m_Val[i] = wt[i];
    }

    WideType(const t_TypeInt& p_val) {
#pragma HLS ARRAY_PARTITION variable = m_Val complete dim = 1
        for (int i = 0; i < t_Width; ++i) {
#pragma HLS UNROLL
            ap_uint<t_DataWidth> l_val = p_val.range(t_DataWidth * (1 + i) - 1, t_DataWidth * i);
            m_Val[i] = *reinterpret_cast<T*>(&l_val);
        }
    }

    WideType(const T p_initScalar) {
#pragma HLS ARRAY_PARTITION variable = m_Val complete dim = 1
        for (int i = 0; i < t_Width; ++i) {
#pragma HLS UNROLL
            m_Val[i] = p_initScalar;
        }
    }

    operator const t_TypeInt() {
#pragma HLS ARRAY_PARTITION variable = m_Val complete dim = 1
        t_TypeInt l_fVal;
        for (int i = 0; i < t_Width; ++i) {
#pragma HLS UNROLL
            T l_v = m_Val[i];
            ap_uint<t_DataWidth> l_val = *reinterpret_cast<ap_uint<t_DataWidth>*>(&l_v);
            l_fVal.range(t_DataWidth * (1 + i) - 1, t_DataWidth * i) = l_val;
        }
        return l_fVal;
    }

    T shift(T p_ValIn) {
        T l_valOut = m_Val[t_Width - 1];
    WIDE_TYPE_SHIFT:
        for (int i = t_Width - 1; i > 0; --i) {
#pragma HLS UNROLL
            T l_val = m_Val[i - 1];
            m_Val[i] = l_val;
        }
        m_Val[0] = p_ValIn;
        return (l_valOut);
    }

    T shift() {
        T l_valOut = m_Val[t_Width - 1];
        for (int i = t_Width - 1; i > 0; --i) {
#pragma HLS UNROLL
            T l_val = m_Val[i - 1];
            m_Val[i] = l_val;
        }
        return (l_valOut);
    }

    T unshift() {
        T l_valOut = m_Val[0];
        for (int i = 0; i < t_Width - 1; ++i) {
#pragma HLS UNROLL
            T l_val = m_Val[i + 1];
            m_Val[i] = l_val;
        }
        return (l_valOut);
    }

    T unshift(const T p_val) {
        T l_valOut = m_Val[0];
        for (int i = 0; i < t_Width - 1; ++i) {
#pragma HLS UNROLL
            T l_val = m_Val[i + 1];
            m_Val[i] = l_val;
        }
        m_Val[t_Width - 1] = p_val;
        return (l_valOut);
    }

    static const WideType zero() {
        WideType l_zero;
        for (int i = 0; i < t_Width; ++i) {
#pragma HLS UNROLL
            l_zero[i] = 0;
        }
        return (l_zero);
    }

    static unsigned int per4k() { return (t_per4k); }
    void print(std::ostream& os) {
        for (int i = 0; i < t_Width; ++i) {
            os << std::setw(FLOAT_WIDTH) << m_Val[i] << " ";
        }
    }

    friend std::ostream& operator<<(std::ostream& os, WideType& p_Val) {
        p_Val.print(os);
        return (os);
    }
};

template <typename T, unsigned int t_DataWidth>
class WideType<T, 1, t_DataWidth> {
   private:
    T m_Val;
    static const unsigned int t_4k = 4096;
    static const unsigned int FLOAT_WIDTH = 7;

   public:
    static const unsigned int t_TypeWidth = t_DataWidth;
    typedef T t_TypeInt;
    typedef T DataType;
    static const unsigned int t_WidthS = 1;
    static const unsigned int t_per4k = t_4k / t_DataWidth * 8;

   public:
    T& operator[](unsigned int p_Idx) {
#ifndef __SYNTHESIS__
        assert(p_Idx == 0);
#endif
        return m_Val;
    }

    const T& operator[](unsigned int p_Idx) const {
#ifndef __SYNTHESIS__
        assert(p_Idx == 0);
#endif
        return m_Val;
    }

    T* getValAddr() { return (&m_Val); }

    WideType() {}

    WideType(const WideType& wt) { m_Val = wt[0]; }

    WideType(const T p_initScalar) { m_Val = p_initScalar; }

    operator const t_TypeInt() { return m_Val; }

    T shift(T p_ValIn) {
        T l_valOut = m_Val;
        m_Val = p_ValIn;
        return l_valOut;
    }
    T shift() { return m_Val; }

    T unshift() { return m_Val; }

    T unshift(T p_ValIn) {
        T l_valOut = m_Val;
        m_Val = p_ValIn;
        return l_valOut;
    }

    static const WideType zero() { return WideType(0); }

    static unsigned int per4k() { return (t_per4k); }
    void print(std::ostream& os) { os << std::setw(FLOAT_WIDTH) << m_Val << " "; }

    friend std::ostream& operator<<(std::ostream& os, WideType& p_Val) {
        p_Val.print(os);
        return (os);
    }
};

template <typename t_FloatType,
          unsigned int t_MemWidth,    // In t_FloatType
          unsigned int t_MemWidthBits // In bits; both must be defined and be consistent
          >
class MemUtil {
   private:
   public:
    typedef WideType<t_FloatType, t_MemWidth> MemWideType;
};
/////////////////////////    Control and helper types    /////////////////////////

template <class T, uint8_t t_NumCycles>
T hlsReg(T p_In) {
#pragma HLS INLINE self off
#pragma HLS INTERFACE ap_none port = return register
    if (t_NumCycles == 1) {
        return p_In;
    } else {
        return hlsReg<T, uint8_t(t_NumCycles - 1)>(p_In);
    }
}

template <class T>
T hlsReg(T p_In) {
    return hlsReg<T, 1>(p_In);
}

template <unsigned int W>
class BoolArr {
   private:
    bool m_Val[W];

   public:
    BoolArr() {}
    BoolArr(bool p_Init) {
#pragma HLS inline self
        for (unsigned int i = 0; i < W; ++i) {
#pragma HLS UNROLL
            m_Val[i] = p_Init;
        }
    }
    bool& operator[](unsigned int p_Idx) {
#pragma HLS inline self
        return m_Val[p_Idx];
    }
    bool And() {
#pragma HLS inline self
        bool l_ret = true;
        for (unsigned int i = 0; i < W; ++i) {
#pragma HLS UNROLL
#pragma HLS ARRAY_PARTITION variable = m_Val COMPLETE
            l_ret = l_ret && m_Val[i];
        }
        return (l_ret);
    }
    bool Or() {
#pragma HLS inline self
        bool l_ret = false;
        for (unsigned int i = 0; i < W; ++i) {
#pragma HLS UNROLL
#pragma HLS ARRAY_PARTITION variable = m_Val COMPLETE
            l_ret = l_ret || m_Val[i];
        }
        return (l_ret);
    }
    void Reset() {
#pragma HLS inline self
        for (unsigned int i = 0; i < W; ++i) {
#pragma HLS UNROLL
#pragma HLS ARRAY_PARTITION variable = m_Val COMPLETE
            m_Val[i] = false;
        }
    }
};

template <class S, int W>
bool streamsAreEmpty(S p_Sin[W]) {
#pragma HLS inline self
    bool l_allEmpty = true;
LOOP_S_IDX:
    for (int w = 0; w < W; ++w) {
#pragma HLS UNROLL
        l_allEmpty = l_allEmpty && p_Sin[w].empty();
    }
    return (l_allEmpty);
}

//  Bit converter
template <typename T>
class BitConv {
   public:
    static const unsigned int t_SizeOf = sizeof(T);
    static const unsigned int t_NumBits = 8 * sizeof(T);
    typedef ap_uint<t_NumBits> BitsType;

   public:
    BitsType toBits(T p_Val) { return p_Val; }
    T toType(BitsType p_Val) { return p_Val; }
};

template <>
inline BitConv<float>::BitsType BitConv<float>::toBits(float p_Val) {
    union {
        float f;
        unsigned int i;
    } u;
    u.f = p_Val;
    return (u.i);
}

template <>
inline float BitConv<float>::toType(BitConv<float>::BitsType p_Val) {
    union {
        float f;
        unsigned int i;
    } u;
    u.i = p_Val;
    return (u.f);
}

template <>
inline BitConv<double>::BitsType BitConv<double>::toBits(double p_Val) {
    union {
        double f;
        int64_t i;
    } u;
    u.f = p_Val;
    return (u.i);
}

template <>
inline double BitConv<double>::toType(BitConv<double>::BitsType p_Val) {
    union {
        double f;
        int64_t i;
    } u;
    u.i = p_Val;
    return (u.f);
}

template <unsigned int t_Bits, unsigned int t_Width, typename t_DataType>
ap_uint<t_Bits> convWideVal2Bits(WideType<t_DataType, t_Width> p_val) {
#pragma HLS inline
#ifndef __SYNTHESIS__
    assert((t_Bits > t_Width) && (t_Bits % t_Width == 0));
#endif
    const unsigned int t_DataBits = sizeof(t_DataType) * 8;
    const unsigned int t_ResEntryBits = t_Bits / t_Width;
    ap_uint<t_Bits> l_res;
    for (unsigned int i = 0; i < t_Width; ++i) {
#pragma HLS UNROLL
        BitConv<t_DataType> l_bitConv;
        ap_uint<t_DataBits> l_datBits = l_bitConv.toBits(p_val[i]);
        ap_uint<t_ResEntryBits> l_resEntry = l_datBits;
        l_res.range((i + 1) * t_ResEntryBits - 1, i * t_ResEntryBits) = l_resEntry;
    }
    return l_res;
}

template <unsigned int t_Bits, unsigned int t_Width, typename t_DataType>
WideType<t_DataType, t_Width> convBits2WideType(ap_uint<t_Bits> p_bits) {
#pragma HLS inline
#ifndef __SYNTHESIS__
    assert((t_Bits > t_Width) && (t_Bits % t_Width == 0));
#endif
    const unsigned int t_DataBits = sizeof(t_DataType) * 8;
    const unsigned int t_InEntryBits = t_Bits / t_Width;
    WideType<t_DataType, t_Width> l_res;
    for (unsigned int i = 0; i < t_Width; ++i) {
#pragma HLS UNROLL
        BitConv<t_DataType> l_bitConv;
        ap_uint<t_InEntryBits> l_inDatBits = p_bits.range((i + 1) * t_InEntryBits - 1, i * t_InEntryBits);
        ap_uint<t_DataBits> l_datBits = l_inDatBits;
        t_DataType l_val = l_bitConv.toType(l_datBits);
        l_res[i] = l_val;
    }
    return l_res;
}
// Type converter - for vectors of different lengths and types
template <typename TS, typename TD>
class WideConv {
   private:
    static const unsigned int t_ws = TS::t_WidthS;
    static const unsigned int t_wd = TD::t_WidthS;
    typedef BitConv<typename TS::DataType> ConvSType;
    typedef BitConv<typename TD::DataType> ConvDType;
    static const unsigned int t_bs = ConvSType::t_NumBits;
    static const unsigned int t_bd = ConvDType::t_NumBits;
    static const unsigned int l_numBits = t_ws * t_bs;

   private:
    ap_uint<l_numBits> l_bits;

   public:
    inline TD convert(TS p_Src) {
        TD l_dst;
        ConvSType l_convS;
        assert(t_wd * t_bd == l_numBits);
        for (int ws = 0; ws < t_ws; ++ws) {
            l_bits.range(t_bs * (ws + 1) - 1, t_bs * ws) = l_convS.toBits(p_Src[ws]);
        }
        ConvDType l_convD;
        for (int wd = 0; wd < t_wd; ++wd) {
            l_dst[wd] = l_convD.toType(l_bits.range(t_bd * (wd + 1) - 1, t_bd * wd));
        }

        return (l_dst);
    }
};

} // namespace blas

} // namespace xf
#endif
