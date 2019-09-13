#ifndef __HLS_SSR_FFT_TYPES_H__
#define __HLS_SSR_FFT_TYPES_H__
//#include "xf_fft/hls_ssr_fft_enums.hpp"

#include "xf_fft/hls_ssr_fft_enums.hpp"
#include "xf_fft/hls_ssr_fft_input_traits.hpp"
#include "xf_fft/hls_ssr_fft_output_traits.hpp"

#define HLS_SSR_FFT_DEFAULT_INSTANCE_ID 999999

namespace xf {
namespace dsp {
namespace fft {

template <typename ssr_fft_param_struct, typename T_in>
struct FFTIOTypes {
    typedef T_in T_inType;
    typedef typename FFTOutputTraits<ssr_fft_param_struct::N,
                                     ssr_fft_param_struct::R,
                                     ssr_fft_param_struct::scaling_mode,
                                     ssr_fft_param_struct::transform_direction,
                                     ssr_fft_param_struct::butterfly_rnd_mode,
                                     typename FFTInputTraits<T_in>::T_castedType>::T_FFTOutType T_outType;
};

} // end namespace fft
} // end namespace dsp
} // end namespace xf

#endif //__HLS_SSR_FFT_TYPES_H__
