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
#ifndef MATRIX_MULT_HPP
#define MATRIX_MULT_HPP
/*
Matrix Multiply Definition

The file holds the definition of the Matrix Multiply kernel class.

*/

/* Coding conventions
  TT_      template type suffix
  TP_      template parameter suffix
*/

#include <adf.h>

#include <assert.h>
#include <array>
#include <cstdint>
#include <type_traits>

#include "device_defs.h"
#include "fir_utils.hpp"
#include "matrix_mult_traits.hpp"
// added for stubs in hw flow
#include "matrix_mult_tiler.hpp"
#include "matrix_mult_untiler.hpp"
#include "matrix_mult_tiling_scheme.hpp"

// CEIL rounds x up to the next multiple of y, which may be x itself.
#define CEIL(x, y) (((x + y - 1) / y) * y)
// Whichever type has the largest size (will be more complicated in future)
//#ifndef GET_TT_OUT
//#define GET_TT_OUT(A,B) std::conditional_t<(sizeof(B) > sizeof(A)),B, A>
//#endif //GET_TT_OUT
#ifndef ROW_MAJOR
#define ROW_MAJOR 0
#endif // ROW_MAJOR
#ifndef COL_MAJOR
#define COL_MAJOR 1
#endif // COL_MAJOR

namespace xf {
namespace dsp {
namespace aie {
namespace blas {
namespace matrix_mult {

// TODO: Move this into a common dsp::aie namespace.
// Functions to support defensive checks
enum { enumUnknownType = 0, enumInt16, enumCint16, enumInt32, enumCint32, enumFloat, enumCfloat };
// function to return an enumeration of the data or coefficient type
template <typename TT_INPUT>
INLINE_DECL constexpr unsigned int fnEnumType() {
    return enumUnknownType;
}; // returns 0 as default. This can be trapped as an error;
template <>
INLINE_DECL constexpr unsigned int fnEnumType<int16>() {
    return enumInt16;
};
template <>
INLINE_DECL constexpr unsigned int fnEnumType<cint16>() {
    return enumCint16;
};
template <>
INLINE_DECL constexpr unsigned int fnEnumType<int32>() {
    return enumInt32;
};
template <>
INLINE_DECL constexpr unsigned int fnEnumType<cint32>() {
    return enumCint32;
};
template <>
INLINE_DECL constexpr unsigned int fnEnumType<float>() {
    return enumFloat;
};
template <>
INLINE_DECL constexpr unsigned int fnEnumType<cfloat>() {
    return enumCfloat;
};

//-----------------------------------------------------------------------------------------------------
template <typename TT_DATA_A,
          typename TT_DATA_B,
          typename TT_OUT_DATA,
          unsigned int TP_DIM_A,
          unsigned int TP_DIM_AB,
          unsigned int TP_DIM_B,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_SAT = 1,
          unsigned int TP_DIM_A_LEADING = ROW_MAJOR,
          unsigned int TP_DIM_B_LEADING = COL_MAJOR,
          unsigned int TP_DIM_OUT_LEADING = ROW_MAJOR,
          unsigned int TP_INPUT_WINDOW_VSIZE_A = TP_DIM_A* TP_DIM_AB,
          unsigned int TP_INPUT_WINDOW_VSIZE_B = TP_DIM_B* TP_DIM_AB,
          bool TP_CASC_IN = CASC_IN_FALSE,
          bool TP_CASC_OUT = CASC_OUT_FALSE,
          unsigned int TP_DIM_A_RANGE = TP_DIM_A,
          unsigned int TP_DIM_AB_RANGE = TP_DIM_AB,
          unsigned int TP_DIM_B_RANGE = TP_DIM_B,
          unsigned int TP_KERNEL_POSITION = 0,
          unsigned int TP_CASC_LEN = 1>
class kernelMatMultClass {
   protected:
    // Members defined here can be changed in derived classes to support customer inheritance.
    // using TT_OUT = outType_t<TT_DATA_A, TT_DATA_B>;
    // These will actually be the result of a constexpr function, depending on
    // how well the B data would fit into the A data. Or how awkward it is to
    // load the data due to LEADING_DIM.
    using m_tZbuff = TT_DATA_B;
    using m_tXbuff = TT_DATA_A;

   private:
    static_assert((((TP_CASC_LEN == 1) && (TP_KERNEL_POSITION == 0)) &&
                   ((TP_DIM_A_RANGE == TP_DIM_A) && (TP_DIM_B_RANGE == TP_DIM_B) && (TP_DIM_AB_RANGE == TP_DIM_AB))),
                  "ERROR: Cascading/Tiling is not currently available. ");

    static_assert(((TP_INPUT_WINDOW_VSIZE_A == (TP_DIM_A * TP_DIM_AB)) &&
                   (TP_INPUT_WINDOW_VSIZE_B == (TP_DIM_B * TP_DIM_AB))),
                  "ERROR: Batch window processing is not currently available. ");

    // static_assert((
    //    (TP_DIM_A_LEADING == ROW_MAJOR) &&
    //    (TP_DIM_B_LEADING == COL_MAJOR) &&
    //    (TP_DIM_OUT_LEADING == ROW_MAJOR)),
    //    "ERROR: Other memory storage options are currently unavailable. Please transpose input/output matrices to
    //    achieve desired alignment.");

    // Not sure exactly - other __restrictions will hit first
    static const int TP_DIM_MIN = 4;

    // Parameter value defensive and legality checks
    static_assert(TP_DIM_A_RANGE* TP_DIM_AB_RANGE* TP_DIM_B_RANGE >= TP_DIM_MIN,
                  "ERROR: Illegal combination of design matrices and cascade length, resulting in kernel matrice sizes "
                  "below minimum required value. Should have at least TP_DIM_MIN macs. ");
    static_assert(TP_SHIFT >= SHIFT_MIN && TP_SHIFT <= SHIFT_MAX, "ERROR: SHIFT is out of the supported range.");
    static_assert(TP_RND >= ROUND_MIN && TP_RND <= ROUND_MAX, "ERROR: RND is out of the supported range.");
    static_assert(TP_SAT >= SAT_MODE_MIN && TP_SAT <= SAT_MODE_MAX, "ERROR: TP_SAT is out of supported range");
    static_assert(TP_SAT != 2, "ERROR: TP_SAT is invalid. Valid values of TP_SAT are 0, 1, and 3");
    static_assert((TP_INPUT_WINDOW_VSIZE_A % (TP_DIM_A * TP_DIM_AB)) == 0,
                  "ERROR: TP_INPUT_WINDOW_VSIZE_A must be an integer multiple of TP_DIM_A*TP_DIM_AB.");
    static_assert((TP_INPUT_WINDOW_VSIZE_B % (TP_DIM_B * TP_DIM_AB)) == 0,
                  "ERROR: TP_INPUT_WINDOW_VSIZE_B must be an integer multiple of TP_DIM_B*TP_DIM_AB.");
    static_assert(
        TP_INPUT_WINDOW_VSIZE_A <= getMaxLen<TT_DATA_A>(),
        "ERROR: TP_INPUT_WINDOW_VSIZE_A must fit within a data memory bank of 32kB for AIE1, 64kB for AIE-ML.");
    static_assert(
        TP_INPUT_WINDOW_VSIZE_B <= getMaxLen<TT_DATA_B>(),
        "ERROR: TP_INPUT_WINDOW_VSIZE_B must fit within a data memory bank of 32kB for AIE1, 64kB for AIE-ML.");
    static_assert(((TP_INPUT_WINDOW_VSIZE_A / TP_DIM_AB) * (TP_INPUT_WINDOW_VSIZE_B / TP_DIM_AB)) <=
                      getMaxLen<TT_OUT_DATA>(),
                  "ERROR: Output matrix must fit within a data memory bank of 32kB for AIE1, 64kB for AIE-ML.");
    static_assert(!(std::is_same<TT_DATA_A, cfloat>::value || std::is_same<TT_DATA_A, float>::value) || (TP_SHIFT == 0),
                  "ERROR: TP_SHIFT cannot be performed for TT_DATA=cfloat, so must be set to 0"); // only necessary to
                                                                                                  // check TT_DATA_A as
                                                                                                  // TT_DATA_B will also
                                                                                                  // be float or integer
                                                                                                  // to match TT_DATA_A.

    static constexpr unsigned int m_kArch = 0; // no other arch right now

    // Only one implementation (with an old name rigt now)
    void matMult_impl1(
        T_inputIF<TP_CASC_IN, TT_DATA_A, TT_DATA_B> inInterface,
        T_outputIF<TP_CASC_OUT, TT_DATA_A, TT_DATA_B, TT_OUT_DATA> outInterface); // Each phase is calculated in turn
                                                                                  // which avoids need for multiple
                                                                                  // accumulators, but requires data
                                                                                  // reloading.

   public:
    // Access function for AIE Synthesizer
    unsigned int get_m_kArch() { return m_kArch; };

    // struct tilingStruct {
    //     unsigned int Atile;
    //     unsigned int ABtile;
    //     unsigned int Btile;
    // };
    static INLINE_DECL constexpr tilingStruct getTilingScheme() { return fnTilingScheme<TT_DATA_A, TT_DATA_B>(); };

    // Putting this into a function so that the static assert error message includes the value of the tiling scheme.
    template <unsigned Atile, unsigned ABtile, unsigned Btile>
    static bool constexpr tilingSchemeMultiples() {
        static_assert(TP_DIM_A % Atile == 0, "Error: TP_DIM_A is not a multiple of the tiling scheme.");
        static_assert(TP_DIM_B % Btile == 0, "Error: TP_DIM_B is not a multiple of the tiling scheme.");
        static_assert(TP_DIM_AB % ABtile == 0, "Error: TP_DIM_AB is not a multiple of the tiling scheme.");
        return (TP_DIM_A % Atile == 0) && (TP_DIM_B % Btile == 0) && (TP_DIM_AB % ABtile == 0);
    }
    static constexpr tilingStruct tilingScheme = getTilingScheme();
    static_assert((tilingScheme.Atile > 1 || tilingScheme.ABtile > 1 || tilingScheme.Btile > 1),
                  "ERROR: There are no supported Matrix Multiplication Modes for this data type combination");
    static_assert(tilingSchemeMultiples<tilingScheme.Atile, tilingScheme.ABtile, tilingScheme.Btile>(),
                  "Error: Dimensions are not multiples of tiling scheme.");

    // Constructor
    kernelMatMultClass(){};

    // FIR
    void matMultKernel(T_inputIF<TP_CASC_IN, TT_DATA_A, TT_DATA_B> inInterface,
                       T_outputIF<TP_CASC_OUT, TT_DATA_A, TT_DATA_B, TT_OUT_DATA> outInterface);
};

//-----------------------------------------------------------------------------------------------------
// Cascade layer class and specializations

//-----------------------------------------------------------------------------------------------------
// This is the main declaration of the matrix_mult class, and is also used for the Standalone kernel specialization with
// no cascade ports, a single input and no reload
template <typename TT_DATA_A,
          typename TT_DATA_B,
          typename TT_OUT_DATA,
          unsigned int TP_DIM_A,
          unsigned int TP_DIM_AB,
          unsigned int TP_DIM_B,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_SAT = 1,
          unsigned int TP_DIM_A_LEADING = ROW_MAJOR,
          unsigned int TP_DIM_B_LEADING = COL_MAJOR,
          unsigned int TP_DIM_OUT_LEADING = ROW_MAJOR,
          unsigned int TP_INPUT_WINDOW_VSIZE_A = TP_DIM_A* TP_DIM_AB,
          unsigned int TP_INPUT_WINDOW_VSIZE_B = TP_DIM_B* TP_DIM_AB,
          bool TP_CASC_IN = CASC_IN_FALSE,
          bool TP_CASC_OUT = CASC_OUT_FALSE,
          unsigned int TP_DIM_A_RANGE = TP_DIM_A,
          unsigned int TP_DIM_AB_RANGE = TP_DIM_AB,
          unsigned int TP_DIM_B_RANGE = TP_DIM_B,
          unsigned int TP_KERNEL_POSITION = 0,
          unsigned int TP_CASC_LEN = 1>
class matrix_mult : public kernelMatMultClass<TT_DATA_A,
                                              TT_DATA_B,
                                              TT_OUT_DATA,
                                              TP_DIM_A,
                                              TP_DIM_AB,
                                              TP_DIM_B,
                                              TP_SHIFT,
                                              TP_RND,
                                              TP_SAT,
                                              TP_DIM_A_LEADING,
                                              TP_DIM_B_LEADING,
                                              TP_DIM_OUT_LEADING,
                                              TP_INPUT_WINDOW_VSIZE_A,
                                              TP_INPUT_WINDOW_VSIZE_B,
                                              TP_CASC_IN,
                                              TP_CASC_OUT,
                                              TP_DIM_A,
                                              TP_DIM_AB,
                                              TP_DIM_B,
                                              TP_KERNEL_POSITION,
                                              TP_CASC_LEN> {
   private:
   public:
    // Constructor calls base class constructor (neither of which have anything to do)
    matrix_mult()
        : kernelMatMultClass<TT_DATA_A,
                             TT_DATA_B,
                             TT_OUT_DATA,
                             TP_DIM_A,
                             TP_DIM_AB,
                             TP_DIM_B,
                             TP_SHIFT,
                             TP_RND,
                             TP_SAT,
                             TP_DIM_A_LEADING,
                             TP_DIM_B_LEADING,
                             TP_DIM_OUT_LEADING,
                             TP_INPUT_WINDOW_VSIZE_A,
                             TP_INPUT_WINDOW_VSIZE_B,
                             TP_CASC_IN,
                             TP_CASC_OUT,
                             TP_DIM_A,
                             TP_DIM_AB,
                             TP_DIM_B,
                             TP_KERNEL_POSITION,
                             TP_CASC_LEN>() {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(matrix_mult::matMult); }

    // FIR
    void matMult(input_buffer<TT_DATA_A>& __restrict inWindowA,
                 input_buffer<TT_DATA_B>& __restrict inWindowB,
                 output_buffer<TT_OUT_DATA>& __restrict outWindow);
};

//-----------------------------------------------------------------------------------------------------
// Partially specialized classes for cascaded interface (final kernel in cascade), single input, no reload
template <typename TT_DATA_A,
          typename TT_DATA_B,
          typename TT_OUT_DATA,
          unsigned int TP_DIM_A,
          unsigned int TP_DIM_AB,
          unsigned int TP_DIM_B,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_DIM_A_LEADING,
          unsigned int TP_DIM_B_LEADING,
          unsigned int TP_DIM_OUT_LEADING,
          unsigned int TP_INPUT_WINDOW_VSIZE_A,
          unsigned int TP_INPUT_WINDOW_VSIZE_B,
          unsigned int TP_DIM_A_RANGE,
          unsigned int TP_DIM_AB_RANGE,
          unsigned int TP_DIM_B_RANGE,
          unsigned int TP_KERNEL_POSITION,
          unsigned int TP_CASC_LEN>
class matrix_mult<TT_DATA_A,
                  TT_DATA_B,
                  TT_OUT_DATA,
                  TP_DIM_A,
                  TP_DIM_AB,
                  TP_DIM_B,
                  TP_SHIFT,
                  TP_RND,
                  TP_SAT,
                  TP_DIM_A_LEADING,
                  TP_DIM_B_LEADING,
                  TP_DIM_OUT_LEADING,
                  TP_INPUT_WINDOW_VSIZE_A,
                  TP_INPUT_WINDOW_VSIZE_B,
                  CASC_IN_TRUE,
                  CASC_OUT_FALSE,
                  TP_DIM_A_RANGE,
                  TP_DIM_AB_RANGE,
                  TP_DIM_B_RANGE,
                  TP_KERNEL_POSITION,
                  TP_CASC_LEN> : public kernelMatMultClass<TT_DATA_A,
                                                           TT_DATA_B,
                                                           TT_OUT_DATA,
                                                           TP_DIM_A,
                                                           TP_DIM_AB,
                                                           TP_DIM_B,
                                                           TP_SHIFT,
                                                           TP_RND,
                                                           TP_SAT,
                                                           TP_DIM_A_LEADING,
                                                           TP_DIM_B_LEADING,
                                                           TP_DIM_OUT_LEADING,
                                                           TP_INPUT_WINDOW_VSIZE_A,
                                                           TP_INPUT_WINDOW_VSIZE_B,
                                                           CASC_IN_TRUE,
                                                           CASC_OUT_FALSE,
                                                           TP_DIM_A_RANGE,
                                                           TP_DIM_AB_RANGE,
                                                           TP_DIM_B_RANGE,
                                                           TP_KERNEL_POSITION,
                                                           TP_CASC_LEN> {
   private:
   public:
    // Constructor (does nothing and neither does the base class, but this is left as a placeholder)
    matrix_mult()
        : kernelMatMultClass<TT_DATA_A,
                             TT_DATA_B,
                             TT_OUT_DATA,
                             TP_DIM_A,
                             TP_DIM_AB,
                             TP_DIM_B,
                             TP_SHIFT,
                             TP_RND,
                             TP_SAT,
                             TP_DIM_A_LEADING,
                             TP_DIM_B_LEADING,
                             TP_DIM_OUT_LEADING,
                             TP_INPUT_WINDOW_VSIZE_A,
                             TP_INPUT_WINDOW_VSIZE_B,
                             CASC_IN_TRUE,
                             CASC_OUT_FALSE,
                             TP_DIM_A_RANGE,
                             TP_DIM_AB_RANGE,
                             TP_DIM_B_RANGE,
                             TP_KERNEL_POSITION,
                             TP_CASC_LEN>() {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(matrix_mult::matMult); }

    // FIR
    void matMult(input_buffer<TT_DATA_A>& __restrict inWindowA,
                 input_buffer<TT_DATA_B>& __restrict inWindowB,
                 input_cascade<accType_t<TT_DATA_A, TT_DATA_B> >* inCascade,
                 output_buffer<TT_OUT_DATA>& __restrict outWindow);
};

//-----------------------------------------------------------------------------------------------------
// Partially specialized classes for cascaded interface (First kernel in cascade), single input, no reload
template <typename TT_DATA_A,
          typename TT_DATA_B,
          typename TT_OUT_DATA,
          unsigned int TP_DIM_A,
          unsigned int TP_DIM_AB,
          unsigned int TP_DIM_B,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_DIM_A_LEADING,
          unsigned int TP_DIM_B_LEADING,
          unsigned int TP_DIM_OUT_LEADING,
          unsigned int TP_INPUT_WINDOW_VSIZE_A,
          unsigned int TP_INPUT_WINDOW_VSIZE_B,
          unsigned int TP_DIM_A_RANGE,
          unsigned int TP_DIM_AB_RANGE,
          unsigned int TP_DIM_B_RANGE,
          unsigned int TP_KERNEL_POSITION,
          unsigned int TP_CASC_LEN>
class matrix_mult<TT_DATA_A,
                  TT_DATA_B,
                  TT_OUT_DATA,
                  TP_DIM_A,
                  TP_DIM_AB,
                  TP_DIM_B,
                  TP_SHIFT,
                  TP_RND,
                  TP_SAT,
                  TP_DIM_A_LEADING,
                  TP_DIM_B_LEADING,
                  TP_DIM_OUT_LEADING,
                  TP_INPUT_WINDOW_VSIZE_A,
                  TP_INPUT_WINDOW_VSIZE_B,
                  CASC_IN_FALSE,
                  CASC_OUT_TRUE,
                  TP_DIM_A_RANGE,
                  TP_DIM_AB_RANGE,
                  TP_DIM_B_RANGE,
                  TP_KERNEL_POSITION,
                  TP_CASC_LEN> : public kernelMatMultClass<TT_DATA_A,
                                                           TT_DATA_B,
                                                           TT_OUT_DATA,
                                                           TP_DIM_A,
                                                           TP_DIM_AB,
                                                           TP_DIM_B,
                                                           TP_SHIFT,
                                                           TP_RND,
                                                           TP_SAT,
                                                           TP_DIM_A_LEADING,
                                                           TP_DIM_B_LEADING,
                                                           TP_DIM_OUT_LEADING,
                                                           TP_INPUT_WINDOW_VSIZE_A,
                                                           TP_INPUT_WINDOW_VSIZE_B,
                                                           CASC_IN_FALSE,
                                                           CASC_OUT_TRUE,
                                                           TP_DIM_A_RANGE,
                                                           TP_DIM_AB_RANGE,
                                                           TP_DIM_B_RANGE,
                                                           TP_KERNEL_POSITION,
                                                           TP_CASC_LEN> {
   private:
   public:
    // Constructor
    matrix_mult()
        : kernelMatMultClass<TT_DATA_A,
                             TT_DATA_B,
                             TT_OUT_DATA,
                             TP_DIM_A,
                             TP_DIM_AB,
                             TP_DIM_B,
                             TP_SHIFT,
                             TP_RND,
                             TP_SAT,
                             TP_DIM_A_LEADING,
                             TP_DIM_B_LEADING,
                             TP_DIM_OUT_LEADING,
                             TP_INPUT_WINDOW_VSIZE_A,
                             TP_INPUT_WINDOW_VSIZE_B,
                             CASC_IN_FALSE,
                             CASC_OUT_TRUE,
                             TP_DIM_A_RANGE,
                             TP_DIM_AB_RANGE,
                             TP_DIM_B_RANGE,
                             TP_KERNEL_POSITION,
                             TP_CASC_LEN>() {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(matrix_mult::matMult); }

    // FIR
    void matMult(input_buffer<TT_DATA_A>& __restrict inWindowA,
                 input_buffer<TT_DATA_B>& __restrict inWindowB,
                 output_cascade<accType_t<TT_DATA_A, TT_DATA_B> >* outCascade);
};

//-----------------------------------------------------------------------------------------------------
// Partially specialized classes for cascaded interface (middle kernels in cascade), single input, no reload
template <typename TT_DATA_A,
          typename TT_DATA_B,
          typename TT_OUT_DATA,
          unsigned int TP_DIM_A,
          unsigned int TP_DIM_AB,
          unsigned int TP_DIM_B,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_DIM_A_LEADING,
          unsigned int TP_DIM_B_LEADING,
          unsigned int TP_DIM_OUT_LEADING,
          unsigned int TP_INPUT_WINDOW_VSIZE_A,
          unsigned int TP_INPUT_WINDOW_VSIZE_B,
          unsigned int TP_DIM_A_RANGE,
          unsigned int TP_DIM_AB_RANGE,
          unsigned int TP_DIM_B_RANGE,
          unsigned int TP_KERNEL_POSITION,
          unsigned int TP_CASC_LEN>
class matrix_mult<TT_DATA_A,
                  TT_DATA_B,
                  TT_OUT_DATA,
                  TP_DIM_A,
                  TP_DIM_AB,
                  TP_DIM_B,
                  TP_SHIFT,
                  TP_RND,
                  TP_SAT,
                  TP_DIM_A_LEADING,
                  TP_DIM_B_LEADING,
                  TP_DIM_OUT_LEADING,
                  TP_INPUT_WINDOW_VSIZE_A,
                  TP_INPUT_WINDOW_VSIZE_B,
                  CASC_IN_TRUE,
                  CASC_OUT_TRUE,
                  TP_DIM_A_RANGE,
                  TP_DIM_AB_RANGE,
                  TP_DIM_B_RANGE,
                  TP_KERNEL_POSITION,
                  TP_CASC_LEN> : public kernelMatMultClass<TT_DATA_A,
                                                           TT_DATA_B,
                                                           TT_OUT_DATA,
                                                           TP_DIM_A,
                                                           TP_DIM_AB,
                                                           TP_DIM_B,
                                                           TP_SHIFT,
                                                           TP_RND,
                                                           TP_SAT,
                                                           TP_DIM_A_LEADING,
                                                           TP_DIM_B_LEADING,
                                                           TP_DIM_OUT_LEADING,
                                                           TP_INPUT_WINDOW_VSIZE_A,
                                                           TP_INPUT_WINDOW_VSIZE_B,
                                                           CASC_IN_TRUE,
                                                           CASC_OUT_TRUE,
                                                           TP_DIM_A_RANGE,
                                                           TP_DIM_AB_RANGE,
                                                           TP_DIM_B_RANGE,
                                                           TP_KERNEL_POSITION,
                                                           TP_CASC_LEN> {
   private:
   public:
    // Constructor
    matrix_mult()
        : kernelMatMultClass<TT_DATA_A,
                             TT_DATA_B,
                             TT_OUT_DATA,
                             TP_DIM_A,
                             TP_DIM_AB,
                             TP_DIM_B,
                             TP_SHIFT,
                             TP_RND,
                             TP_SAT,
                             TP_DIM_A_LEADING,
                             TP_DIM_B_LEADING,
                             TP_DIM_OUT_LEADING,
                             TP_INPUT_WINDOW_VSIZE_A,
                             TP_INPUT_WINDOW_VSIZE_B,
                             CASC_IN_TRUE,
                             CASC_OUT_TRUE,
                             TP_DIM_A_RANGE,
                             TP_DIM_AB_RANGE,
                             TP_DIM_B_RANGE,
                             TP_KERNEL_POSITION,
                             TP_CASC_LEN>() {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(matrix_mult::matMult); }

    // FIR
    void matMult(input_buffer<TT_DATA_A>& __restrict inWindowA,
                 input_buffer<TT_DATA_B>& __restrict inWindowB,
                 input_cascade<accType_t<TT_DATA_A, TT_DATA_B> >* inCascade,
                 output_cascade<accType_t<TT_DATA_A, TT_DATA_B> >* outCascade);
};
}
}
}
}
}

#endif // matrix_mult_HPP
