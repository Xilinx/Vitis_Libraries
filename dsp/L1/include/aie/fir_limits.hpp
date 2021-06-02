#ifndef _DSPLIB_FIR_LIMITS_HPP_
#define _DSPLIB_FIR_LIMITS_HPP_

/* This file exists to define parameter scope limits of the FIR designs
   and hold static_asserts which assist checking these limits.
*/

// The following maximums are the maximums tested. The function may work for larger values.
#define FIR_LEN_MAX 240
#define FIR_LEN_MIN 4
#define SHIFT_MAX 62
#define SHIFT_MIN 0
#define ROUND_MAX 7
#define ROUND_MIN 0

#endif // _DSPLIB_FIR_LIMITS_HPP_
