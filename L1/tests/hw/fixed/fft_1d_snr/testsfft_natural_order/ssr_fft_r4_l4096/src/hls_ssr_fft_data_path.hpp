// File Name : hls_ssr_fft_data_path.hpp
#ifndef HLS_SSR_FFT_DATA_PATH_
#define HLS_SSR_FFT_DATA_PATH_
#include <ap_fixed.h>
#include "xf_fft.hpp"
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 *  Declare FFT Radix or SSR factor and length for the test
 *+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 */

//#define SSR_FFT_L  (4096)
//#define SSR_FFT_R 4
#include "fft_size.hpp"

using namespace xf::dsp::fft;

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 *  Set appropriate bitwidth for the storage of sin/cos or exponential tables and also define input bit withs
 *+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 */

#define SSR_FFT_IN_WL 27 // 90db:27  // Word Length Total bits to represent input word length
#define SSR_FFT_IN_IL 10 // 90db:8 // The bits to be use for integer part
#define SSR_FFT_TW_WL 18 // The bits to be used for representing sine and cosine table
#define SSR_FFT_TW_IL \
    2 // The integer part should used 2 its and not less then 2 bits for sine and cosine reprensentation

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 * Define double type that is used for creaing a reference floating point model which is used for verification and
 *calculating fixed point model SNR in dbs.
 *+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 */
typedef double type_data;
typedef double tip_fftInType;
typedef double tip_fftOutType;
typedef double tip_fftTwiddleType;
typedef double tip_complexExpTableType;
typedef double tip_complexMulOutType;

typedef ap_fixed<SSR_FFT_IN_WL, SSR_FFT_IN_IL> T_INNER_SSR_FFT_IN;
typedef ap_fixed<SSR_FFT_TW_WL, SSR_FFT_TW_IL> T_INNER_SSR_TWIDDLE_TABLE;
typedef T_INNER_SSR_TWIDDLE_TABLE T_INNER_SSR_EXP_TABLE;

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 * Define ap_fixed complex type for input samples and the complex sin/cos table storage
 *+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 */

typedef std::complex<ap_fixed<SSR_FFT_IN_WL, SSR_FFT_IN_IL> > T_SSR_FFT_IN;
typedef std::complex<ap_fixed<SSR_FFT_TW_WL, SSR_FFT_TW_IL> > T_SSR_TWIDDLE_TABLE;

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 * Define a ssr fft parameter structure that extends a predefine structure with defaul values, redefine only the members
 *whose default values are to be changes , the structure which is extended here is called ssr_fft_default_params which
 *defines def- -ault values
 *+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 */

struct ssr_fft_fix_params : ssr_fft_default_params {
    static const int N = SSR_FFT_L;
    static const int R = SSR_FFT_R;

    static const scaling_mode_enum scaling_mode =
        SSR_FFT_GROW_TO_MAX_WIDTH; // SSR_FFT_NO_SCALING;//SSR_FFT_GROW_TO_MAX_WIDTH; //SSR_FFT_SCALE
    // Twiddle and Complex Exponential Tables : Effectively sin/cos storage resolution
    static const int twiddle_table_word_length = 18;       // 90db:18
    static const int twiddle_table_intger_part_length = 2; // 2 bits are selected to represent +1/-1 correctly
};

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 * Define a type for output storage which is pre-defined in a structure ssr_fft_output_type : this structure returns
 *proper ty- -pe provided the constant parameter structure as defined above and the ssr fft input type also defined
 *above
 *+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 */
typedef xf::dsp::fft::ssr_fft_output_type<ssr_fft_fix_params, T_SSR_FFT_IN>::t_ssr_fft_out T_SSR_FFT_OUT;
typedef T_SSR_TWIDDLE_TABLE T_SSR_EXP_TABLE;

#endif // HLS_SSR_FFT_DATA_PATH_
