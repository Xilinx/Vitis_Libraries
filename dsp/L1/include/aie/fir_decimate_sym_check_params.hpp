#ifndef _DSPLIB_FIR_DECIMATE_SYM_CHECK_PARAMS_HPP_
#define _DSPLIB_FIR_DECIMATE_SYM_CHECK_PARAMS_HPP_

// This file holds the static_assert statements which alert the user to any illegal or unsupported
// values and combinations of values of template parameters for the library element in question.
#include "fir_utils.hpp"
#include "fir_decimate_sym_traits.hpp"

// Parameter value defensive and legality checks
static_assert(TP_DECIMATE_FACTOR <= fnMaxDecimateFactor<TT_DATA, TT_COEFF>(),
              "ERROR: Max Decimate factor exxceeded. High Decimate factors do not take advantage from symmetrical "
              "implementation. Use fir_decimate_asym instead.");
static_assert(TP_FIR_LEN <= FIR_LEN_MAX, "ERROR: Max supported FIR length exceeded. ");
static_assert(TP_FIR_RANGE_LEN >= FIR_LEN_MIN,
              "ERROR: Illegal combination of design FIR length and cascade length, resulting in kernel FIR length "
              "below minimum required value. ");
static_assert(TP_FIR_LEN % TP_DECIMATE_FACTOR == 0, "ERROR: TP_FIR_LEN must be a multiple of TP_DECIMATE_FACTOR");
static_assert(TP_SHIFT >= SHIFT_MIN && TP_SHIFT <= SHIFT_MAX, "ERROR: TP_SHIFT is out of the supported range.");
static_assert(TP_RND >= ROUND_MIN && TP_RND <= ROUND_MAX, "ERROR: TP_RND is out of the supported range.");
static_assert(fnEnumType<TT_DATA>() != enumUnknownType, "ERROR: TT_DATA is not a supported type.");
static_assert(fnEnumType<TT_COEFF>() != enumUnknownType, "ERROR: TT_COEFF is not a supported type.");
static_assert(fnFirDecSymTypeSupport<TT_DATA, TT_COEFF>() != 0,
              "ERROR: The combination of TT_DATA and TT_COEFF is not supported for this class.");
static_assert(fnFirDecSymmertySupported<TT_DATA, TT_COEFF>() != 0,
              "ERROR: The combination of TT_DATA and TT_COEFF is not supported for this class, as implementation would "
              "not use the benefits of symmetry. Use fir_decimate_asym instead.");
static_assert(fnTypeCheckDataCoeffSize<TT_DATA, TT_COEFF>() != 0,
              "ERROR: TT_DATA type less precise than TT_COEFF is not supported.");
static_assert(fnTypeCheckDataCoeffCmplx<TT_DATA, TT_COEFF>() != 0,
              "ERROR: real TT_DATA with complex TT_COEFF is not supported.");
static_assert(fnTypeCheckDataCoeffFltInt<TT_DATA, TT_COEFF>() != 0,
              "ERROR: a mix of float and integer types of TT_DATA and TT_COEFF is not supported.");
static_assert(fnFirDecSymMultiColumn<TT_DATA, TT_COEFF>() != 0,
              "ERROR: The combination of TT_DATA and TT_COEFF is currently unsupported.");
#endif // _DSPLIB_FIR_DECIMATE_SYM_CHECK_PARAMS_HPP_

/*  (c) Copyright 2020 Xilinx, Inc. All rights reserved.

    This file contains confidential and proprietary information
    of Xilinx, Inc. and is protected under U.S. and
    international copyright and other intellectual property
    laws.

    DISCLAIMER
    This disclaimer is not a license and does not grant any
    rights to the materials distributed herewith. Except as
    otherwise provided in a valid license issued to you by
    Xilinx, and to the maximum extent permitted by applicable
    law: (1) THESE MATERIALS ARE MADE AVAILABLE "AS IS" AND
    WITH ALL FAULTS, AND XILINX HEREBY DISCLAIMS ALL WARRANTIES
    AND CONDITIONS, EXPRESS, IMPLIED, OR STATUTORY, INCLUDING
    BUT NOT LIMITED TO WARRANTIES OF MERCHANTABILITY, NON-
    INFRINGEMENT, OR FITNESS FOR ANY PARTICULAR PURPOSE; and
    (2) Xilinx shall not be liable (whether in contract or tort,
    including negligence, or under any other theory of
    liability) for any loss or damage of any kind or nature
    related to, arising under or in connection with these
    materials, including for any direct, or any indirect,
    special, incidental, or consequential loss or damage
    (including loss of data, profits, goodwill, or any type of
    loss or damage suffered as a result of any action brought
    by a third party) even if such damage or loss was
    reasonably foreseeable or Xilinx had been advised of the
    possibility of the same.

    CRITICAL APPLICATIONS
    Xilinx products are not designed or intended to be fail-
    safe, or for use in any application requiring fail-safe
    performance, such as life-support or safety devices or
    systems, Class III medical devices, nuclear facilities,
    applications related to the deployment of airbags, or any
    other applications that could lead to death, personal
    injury, or severe property or environmental damage
    (individually and collectively, "Critical
    Applications"). Customer assumes the sole risk and
    liability of any use of Xilinx products in Critical
    Applications, subject only to applicable laws and
    regulations governing limitations on product liability.

    THIS COPYRIGHT NOTICE AND DISCLAIMER MUST BE RETAINED AS
    PART OF THIS FILE AT ALL TIMES.                       */
