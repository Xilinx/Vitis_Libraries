#ifndef _SOLVERLIB_SUBSTITUTION_TRAITS_HPP_
#define _SOLVERLIB_SUBSTITUTION_TRAITS_HPP_

#include <adf.h>
#include "aie_api/aie_adf.hpp"
#include "aie_api/aie.hpp"
#include "fir_utils.hpp"
#include "device_defs.h"

using namespace adf;
using namespace ::xf::dsp::aie;

namespace xf {
namespace solver {
namespace aie {
namespace substitution {

enum subst_type { FWD, BWD };

struct leading {
    static constexpr bool Trans = true;
};

static constexpr unsigned int kMaxReadInBytes = 256 / 8;

  //---------------------------------------------
  //base type - this is used the real part of a potentially complex type since all diagonal elements are effectively real-only.

  template <typename T_D>
  struct tFbsubBaseType_t {
    //empty base class is effectively an error trap if a new type is added;
  };
  template <>
  struct tFbsubBaseType_t<cint32> {
    using type = int32;
  };

#ifdef _SUPPORTS_BFLOAT16_
  template <>
  struct tFbsubBaseType_t<bfloat16> {
    using type = bfloat16;
  };
#endif
#ifdef _SUPPORTS_CBFLOAT16_
  template <>
  struct tFbsubBaseType_t<cbfloat16> {
    using type = bfloat16;
  };
#endif
  template <>
  struct tFbsubBaseType_t<float> {
    using type = float;
  };
#ifdef __SUPPORTS_CFLOAT__
  template <>
  struct tFbsubBaseType_t<cfloat> {
    using type = float;
  };
#endif

//A little shorthand to make the use of tFbsubBaseType a bit less verbose
template<typename T_D>
using tFbsubBaseType = typename tFbsubBaseType_t<T_D>::type;


//acctype
template <typename TT_DATA>
struct accTypeFbsub_t {
    using type = acc48;
};
template <>
struct accTypeFbsub_t<float> {
    using type = accfloat;
};
template <>
struct accTypeFbsub_t<cfloat> {
    using type = caccfloat;
};

//A little shorthand to make the use of accTypeFbsub_t a bit less verbose
template<typename T_D>
using accTypeFbsub = typename accTypeFbsub_t<T_D>::type;

template <typename TT_DATA>
struct fbsub_input_params {
    TT_DATA* __restrict in_L;
    TT_DATA* __restrict in_y;
    input_stream<TT_DATA>* __restrict in_x;
    input_cascade<accTypeFbsub<TT_DATA>>* __restrict in_pp;
};

template <typename TT_DATA>
struct fbsub_output_params {
    TT_DATA* __restrict out_x;
    output_stream<TT_DATA>* __restrict out_x_strm;
    output_cascade<accTypeFbsub<TT_DATA>>* __restrict out_pp;
};

} // namespace substitution
} // namespace aie
} // namespace solver
} // namespace xf

#endif // _SOLVERLIB_SUBSTITUTION_TRAITS_HPP_
