/**********
 * Copyright (c) 2017, Xilinx, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its contributors
 * may be used to endorse or promote products derived from this software
 * without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * **********/
/**
 *  @brief common datatypes for HLS kernel code.
 *  @2019. 02. 01
 */

#ifndef XF_BLAS_UTILITY_H
#define XF_BLAS_UTILITY_H

#include <stdint.h>
#include <ostream>
#include <iomanip>
#include <iostream>
#include "ap_int.h"
#include "ap_shift_reg.h"

namespace xf {
namespace linear_algebra {
namespace blas {
// Helper macros for renaming kernel
#define PASTER(x,y) x ## y
#define EVALUATOR(x,y)  PASTER(x,y)


#define FLOAT_WIDTH 7
#define CMP_WIDTH 11

constexpr size_t mylog2(size_t n) {
  return ( (n<2) ? 0 : 1+mylog2(n/2));
}

template <typename T, unsigned int t_Width>
class WideType {
  private:
    T  m_Val[t_Width];
    static const unsigned int t_4k = 4096; 
  public:
    typedef T DataType;
    static const unsigned int t_WidthS = t_Width; 
    static const unsigned int t_per4k = t_4k / sizeof(T) / t_Width; 
  public:
    T &getVal(unsigned int i) {return(m_Val[i]);}
    T &operator[](unsigned int p_Idx) {return(m_Val[p_Idx]);}
    T *getValAddr() {return(&m_Val[0]);}
    WideType() {}
    WideType(T p_initScalar) {
        #pragma HLS inline self
        for(int i = 0; i < t_Width; ++i) {
        #pragma HLS UNROLL
          getVal(i) = p_initScalar;
        }
      }
    T
    shift(T p_ValIn) {
        #pragma HLS inline self
        #pragma HLS data_pack variable=p_ValIn
        T l_valOut = m_Val[t_Width-1];
        WIDE_TYPE_SHIFT:for(int i = t_Width - 1; i > 0; --i) {
          T l_val = m_Val[i - 1];
          #pragma HLS data_pack variable=l_val
          m_Val[i] = l_val;
        }
        m_Val[0] = p_ValIn;
        return(l_valOut);
      }
    T
    shift() {
        #pragma HLS inline self
        T l_valOut = m_Val[t_Width-1];
        WIDE_TYPE_SHIFT:for(int i = t_Width - 1; i > 0; --i) {
          T l_val = m_Val[i - 1];
          #pragma HLS data_pack variable=l_val
          m_Val[i] = l_val;
        }
        return(l_valOut);
      }
    T
    unshift() {
        #pragma HLS inline self
        T l_valOut = m_Val[0];
        WIDE_TYPE_SHIFT:for(int i = 0; i < t_Width - 1; ++i) {
          T l_val = m_Val[i + 1];
          #pragma HLS data_pack variable=l_val
          m_Val[i] = l_val;
        }
        return(l_valOut);
      }
    static const WideType zero() {
        WideType l_zero;
        #pragma HLS data_pack variable=l_zero
        for(int i = 0; i < t_Width; ++i) {
          l_zero[i] = 0;
        }
        return(l_zero);
      }
    static
    unsigned int
    per4k() {
        return(t_per4k);
      }
    void
    print(std::ostream& os) {
        for(int i = 0; i < t_Width; ++i) {
          os << std::setw(FLOAT_WIDTH) << getVal(i) << " ";
        }
      }
    
};

template <typename T1, unsigned int T2>
std::ostream& operator<<(std::ostream& os, WideType<T1, T2>& p_Val) {
  p_Val.print(os);
  return(os);
}


template <
    typename t_FloatType,
    unsigned int t_MemWidth,     // In t_FloatType
    unsigned int t_MemWidthBits  // In bits; both must be defined and be consistent
  >
class MemUtil
{
  private:
  public:
    typedef WideType<t_FloatType, t_MemWidth> MemWideType;    
};
/////////////////////////    Control and helper types    /////////////////////////

template<class T, uint8_t t_NumCycles>
T
hlsReg(T p_In)
{
  #pragma HLS INLINE self off
  #pragma HLS INTERFACE ap_none port=return register
  if (t_NumCycles == 1) {
    return p_In;
  } else {
    return hlsReg<T, uint8_t(t_NumCycles - 1)> (p_In);
  }
}

template<class T>
T
hlsReg(T p_In)
{
  return hlsReg<T, 1> (p_In);
}

template <unsigned int W>
class BoolArr {
  private:
    bool m_Val[W];
  public:
    BoolArr(){}
    BoolArr(bool p_Init) {
        #pragma HLS inline self
        for(unsigned int i = 0; i < W; ++i) {
          #pragma HLS UNROLL
          m_Val[i] = p_Init;
        }
    }
    bool & operator[](unsigned int p_Idx) {
        #pragma HLS inline self
        return m_Val[p_Idx];
    }
    bool And() {
        #pragma HLS inline self
        bool l_ret = true;
        for(unsigned int i = 0; i < W; ++i) {
          #pragma HLS UNROLL
          #pragma HLS ARRAY_PARTITION variable=m_Val COMPLETE
          l_ret = l_ret && m_Val[i];
        }
        return(l_ret);
      }
    bool Or() {
        #pragma HLS inline self
        bool l_ret = false;
        for(unsigned int i = 0; i < W; ++i) {
          #pragma HLS UNROLL
          #pragma HLS ARRAY_PARTITION variable=m_Val COMPLETE
          l_ret = l_ret || m_Val[i];
        }
        return(l_ret);
      }
    void Reset() {
        #pragma HLS inline self
        for(unsigned int i = 0; i < W; ++i) {
          #pragma HLS UNROLL
          #pragma HLS ARRAY_PARTITION variable=m_Val COMPLETE
          m_Val[i] = false;
        }
      }
};

template <class S, int W>
bool
streamsAreEmpty(S p_Sin[W]) {
  #pragma HLS inline self
  bool l_allEmpty = true;
  LOOP_S_IDX:for (int w = 0; w < W; ++w) {
    #pragma HLS UNROLL
    l_allEmpty = l_allEmpty && p_Sin[w].empty();
  }
  return(l_allEmpty);
}

//  Bit converter
template <typename T>
class BitConv {
  public:
    static const unsigned int t_SizeOf = sizeof(T);
    static const unsigned int t_NumBits = 8 * sizeof(T);
    typedef ap_uint<t_NumBits> BitsType;
  public:
    BitsType
    toBits(T p_Val) {
      return p_Val;
    }
    T
    toType(BitsType p_Val) {
      return p_Val;
    }
 };
 
template<>
inline
BitConv<float>::BitsType
BitConv<float>::toBits(float p_Val) {
  union {
    float f;
    unsigned int i;
  } u;
  u.f = p_Val;
  return(u.i);
}

template<>
inline
float
BitConv<float>::toType(BitConv<float>::BitsType p_Val) {
  union {
    float f;
    unsigned int i;
  } u;
  u.i = p_Val;
  return(u.f);
}

template<>
inline
BitConv<double>::BitsType
BitConv<double>::toBits(double p_Val) {
  union {
    double f;
    int64_t i;
  } u;
  u.f = p_Val;
  return(u.i);
}

template<>
inline
double
BitConv<double>::toType(BitConv<double>::BitsType p_Val) {
  union {
    double f;
    int64_t i;
  } u;
  u.i = p_Val;
  return(u.f);
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
    inline
    TD
    convert(TS p_Src) {
      TD l_dst;
      ConvSType l_convS;
      assert(t_wd * t_bd == l_numBits);
      for(int ws = 0; ws < t_ws; ++ws) {
        l_bits.range(t_bs * (ws+1) - 1, t_bs * ws) = l_convS.toBits(p_Src[ws]);
      }
      ConvDType l_convD;
      for(int wd = 0; wd < t_wd; ++wd) {
        l_dst[wd] = l_convD.toType(l_bits.range(t_bd * (wd+1) - 1, t_bd * wd));
      }

      return(l_dst);
    }
};
}
}
}
#endif
