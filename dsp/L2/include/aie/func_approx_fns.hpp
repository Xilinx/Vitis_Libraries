/*
 * Copyright (C) 2019-2022, Xilinx, Inc.
 * Copyright (C) 2022-2024, Advanced Micro Devices, Inc.
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
#ifndef _DSPLIB_FUNC_APPROX_WINDOW_FNS_HPP_
#define _DSPLIB_FUNC_APPROX_WINDOW_FNS_HPP_

namespace xf {
namespace dsp {
namespace aie {
namespace func_approx {

/**
 * @addtogroup  func_approx Function Approximation
 *
 * @{
 *
 */

/**
 * @defgroup func_approx_utils Function Approximation utility functions
 *
 * @ingroup func_approx
 *
 * The Function Approximation utilities contain helper functions to create lookup tables of slope and offset
 * values for a number of common functions.
 *
 */
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// createLut function - creates lookup table with slope offset values using provided arrays of x and y values.
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// create slope offset lookup table for integers using point-slope form
template <typename T_D, typename T_L>
void createLut(T_L* lut_values,
               float* x_values,
               float* y_values,
               const int locationTotal,
               const int locationWidth,
               const int domainMode,
               const int shift) {
    unsigned int biasDomain =
        (domainMode == 1) ? 1 : 0; // When TP_DOMAIN_MODE = 1, x_points are biased to be in domain 1 to 2
    unsigned int scaleDomain =
        (domainMode == 2) ? 4 : 1; // When TP_DOMAIN_MODE = 2, x_points are scaled to be in domain 0 to 4.

    float slopes[locationTotal];
    float offsets[locationTotal];
    // slope offset values to be temporarily stored as int64_t to allow downwards shift
    int64_t offsetAccum;
    // Integer, use the point slope form
    for (int i = 0; i < (locationTotal); i++) {
        slopes[i] = y_values[i + 1] - y_values[i];
        offsets[i] = y_values[i];
    }
    for (int i = 0; i < (locationTotal); i++) {
        lut_values[2 * i] = (T_L)(((float)locationTotal * locationWidth * slopes[i]) / scaleDomain);
        offsetAccum = (int64_t)(((float)locationTotal * locationWidth * offsets[i]) / scaleDomain);
        lut_values[2 * i + 1] = (T_L)(offsetAccum >> shift);
    }
}
template <>
void createLut<float, float>(float* lut_values,
                             float* x_values,
                             float* y_values,
                             const int locationTotal,
                             const int locationWidth,
                             const int domainMode,
                             const int shift) {
    unsigned int biasDomain =
        (domainMode == 1) ? 1 : 0; // When TP_DOMAIN_MODE = 1, x_points are biased to be in domain 1 to 2
    unsigned int scaleDomain =
        (domainMode == 2) ? 4 : 1; // When TP_DOMAIN_MODE = 2, x_points are scaled to be in domain 0 to 4.
    float slopes[locationTotal];
    float offsets[locationTotal];

    // Integer, use the point slope form
    for (int i = 0; i < (locationTotal); i++) {
        slopes[i] = (y_values[i + 1] - y_values[i]) / (((x_values[i + 1]) - x_values[i]));
        offsets[i] = (y_values[i] - (slopes[i] * (x_values[i] - biasDomain)));
    }
    for (int i = 0; i < (locationTotal); i++) {
        lut_values[2 * i] = slopes[i];
        lut_values[2 * i + 1] = offsets[i];
    }
}
// // create slope offset lookup table for bfloat16 data (float lut values) using slope intercept form.
// // float slope values must have lower 1 mantissa bits zeroed out
template <>
void createLut<bfloat16, float>(float* lut_values,
                                float* x_values,
                                float* y_values,
                                const int locationTotal,
                                const int locationWidth,
                                const int domainMode,
                                const int shift) {
    unsigned int biasDomain =
        (domainMode == 1) ? 1 : 0; // When TP_DOMAIN_MODE = 1, x_points are biased to be in domain 1 to 2
    unsigned int scaleDomain =
        (domainMode == 2) ? 4 : 1; // When TP_DOMAIN_MODE = 2, x_points are scaled to be in domain 0 to 4.
    float slopes[locationTotal];
    float offsets[locationTotal];
    // Integer, use the point slope form
    for (int i = 0; i < (locationTotal); i++) {
        // From linear_approx API documentation:
        // Note that while the floating point linear approx requires the offset data to be 32b floats, the slope data is
        // required to be bfloat16.
        // However, it is required that all values in the LUT be 32b to ensure the LUT is correctly aligned.
        // While it is safe to use floats as the storage type for the lookup table,
        // it is required that the low 16 mantissa bits of the floating point slope value be zero.
        float tmpSlope = (y_values[i + 1] - y_values[i]) / ((x_values[i + 1] - x_values[i]));
        int32& intSlope = *reinterpret_cast<int32*>(&tmpSlope);
        intSlope &= 0xFFFF0000;
        slopes[i] = *reinterpret_cast<float*>(&intSlope);
        offsets[i] = (y_values[i] - (slopes[i] * (x_values[i] - biasDomain)));
    }
    for (int i = 0; i < (locationTotal); i++) {
        lut_values[2 * i] = slopes[i];
        lut_values[2 * i + 1] = offsets[i];
    }
}
// If TP_DOMAIN_MODE = 0, the input domain is 0 to 1. The number of locations in the lookup table will be 1 <<
// (TP_COARSE_BITS).
// If TP_DOMAIN_MODE = 1, the input domain is 1 to 2. This means that the MSB of TP_COARSE_BITS (used for LUT
// addressing) is always 1.
// To improve performance within the func_approx kernel, the MSB of TP_COARSE_BITS for each input data sample should be
// set to zero before
// it is input to the func_approx graph. This allows the corrsponding LUT address to be found with a simple downshift by
// TP_FINE_BITS.
// It will half the size of the required lookup table to be provided.
// Number of locations in LUT will be 1 << (TP_COARSE_BITS - 1), as the MSB of TP_COARSE_BITS is no longer used.
// If TP_DOMAIN_MODE = 2, the input domain is 1 to 4. The number of locations in the lookup table will be 1 <<
// (TP_COARSE_BITS).
// However, the first quadrant in the lookup (LUT locations 0 to (1 << TP_COARSE_BITS) / 4) - 1 will not be used as this
// would represent lookup values for the f(x) where x is 0 to 1.
// Provided input data in the domain of 0 to 1 when using TP_DOMAIN_MODE = 2 may produced inaccurate results and
// undefined behaviour.

/**
 * @ingroup func_approx_utils
 * @brief getSqrt is a utility function that creates a lookup table for approximating the square-root function of x. The
 * recommended domainMode (TP_DOMAIN_MODE) for getSqrt is 2.
 * @tparam T_D Describes the type of data that will be input to the func_approx.
 * @tparam T_L Describes the type of the values stored in the lookup table. This should be equal to T_D unless T_D is
 * bfloat16, in which case T_L should be float.
 * @param[out] lut_values A pointer to the area where the lookup table of slope and offset values will be created.
 * @param[in] coarseBits Describes the number of bits in a sample of input data that will be used to address the
 * provided lookup table. It determines the total number of locations in the lookup table.
 * @param[in] fineBits Describes the number of bits in an input data sample used for fine interpolation. It determines
 * the width of each location.
 * @param[in] domainMode The choice of TP_DOMAIN_MODE template parameter. This creates an approximation of f(x) where x
 * is normalized over a specified domain. \n
 *            This can be: \n
 *               - 0 (input domain is 0 <= x < 1) \n
 *               - 1 (input domain is 1 <= x < 2) \n
 *               - 2 (input domain is 1 <= x < 4). \n
 * @note If domainMode = 1, where the input domain is 1 <= x < 2, it is expected that the most significant bit of
 * coarseBits has been set to zero and will be ignored by the function approximation kernel when addressing the lookup
 * table. \n
 * This means that the lookup table will be half the size of other domainMode (TP_DOMAIN_MODE) with a similar coarseBits
 * value. \n
 * For domainMode values equal to 0 or 2, there are 2 ^ (coarseBits) locations, and when domainMode is equal to 1, there
 * are 2 ^ (coarseBits - 1) locations.
 * @param[in] shift The downward shift value that will be applied to each offset of a locations. This is only applicable
 * to lookup tables with an integer type offset value.
 */
template <typename T_D, typename T_L>
void getSqrt(T_L* lut_values, const int coarseBits, const int fineBits, const int domainMode, const int shift) {
    // When TP_DOMAIN_MODE = 1, x_points are biased to be in domain 1 to 2
    unsigned int biasDomain = (domainMode == 1) ? 1 : 0;
    // When TP_DOMAIN_MODE = 2, x_points are scaled to be in domain 0 to 4.
    unsigned int scaleDomain = (domainMode == 2) ? 4 : 1;

    // Total number of locations in LUT. A location contains a slope value and an office value.
    unsigned int locationTotal = (1 << (coarseBits - biasDomain));
    // Width of a location (distance in x domain between each location) is determined by the number of fine
    // interpolation bits.
    unsigned int locationWidth = 1 << fineBits;
    // float arrays for f(x) and x.
    float x_points[locationTotal + 1];
    float y_points[locationTotal + 1];
    // y_points and x_points require an additional point in order to calculate the slope of the final location.
    for (int i = 0; i < (locationTotal + 1); i++) {
        // create x between 0 and scaleDomain
        x_points[i] = (scaleDomain) * ((float)i / locationTotal) + biasDomain;
        // y_points = func(x_points)
        y_points[i] = sqrt(x_points[i]);
    }
    // create slope/offset lookup using x_points and y_points
    createLut<T_D, T_L>((T_L*)&lut_values[0], &x_points[0], &y_points[0], locationTotal, locationWidth, domainMode,
                        shift);
}
/**
 * @ingroup func_approx_utils
 * @brief getInvSqrt is a utility function that will create a lookup table to be used for the approximation of the
 * inverse square-root function of x. The recommended domainMode (TP_DOMAIN_MODE) for getInvSqrt is 2.
 * @tparam T_D Describes the type of data that will be input to the func_approx.
 * @tparam T_L Describes the type of the values stored in the lookup table. This should be equal to T_D unless T_D is
 * bfloat16, in which case T_L should be float.
 * @param[out] lut_values A pointer to the area where the lookup table of slope and offset values will be created.
 * @param[in] coarseBits Describes the number of bits in a sample of input data that will be used to address the
 * provided lookup table. It determines the total number of locations in the lookup table.
 * @param[in] fineBits Describes the number of bits in an input data sample used for fine interpolation. It determines
 * the width of each location.
 * @param[in] domainMode The choice of TP_DOMAIN_MODE template parameter. This creates an approximation of f(x) where x
 * is normalized over a specified domain. \n
 *            This can be: \n
 *               - 0 (input domain is 0 <= x < 1) \n
 *               - 1 (input domain is 1 <= x < 2) \n
 *               - 2 (input domain is 1 <= x < 4). \n
 * @note If domainMode = 1, where the input domain is 1 <= x < 2, it is expected that the most significant bit of
 * coarseBits has been set to zero and will be ignored by the function approximation kernel when addressing the lookup
 * table. \n
 * This means that the lookup table will be half the size of other domainMode (TP_DOMAIN_MODE) with a similar coarseBits
 * value. \n
 * For domainMode values equal to 0 or 2, there are 2 ^ (coarseBits) locations, and when domainMode is equal to 1, there
 * are 2 ^ (coarseBits - 1) locations.
 * @param[in] shift The downward shift value that will be applied to each offset of a location. This is only applicable
 * to lookup tables with an integer type offset value.
 */
template <typename T_D, typename T_L>
void getInvSqrt(T_L* lut_values, const int coarseBits, const int fineBits, const int domainMode, const int shift) {
    // When TP_DOMAIN_MODE = 1, x_points are biased to be in domain 1 to 2
    unsigned int biasDomain = (domainMode == 1) ? 1 : 0;
    // When TP_DOMAIN_MODE = 2, x_points are scaled to be in domain 0 to 4.
    unsigned int scaleDomain = (domainMode == 2) ? 4 : 1;

    // Total number of locations in LUT. A location contains a slope value and an office value.
    unsigned int locationTotal = (1 << (coarseBits - biasDomain));
    // Width of a location (distance in x domain between each location) is determined by the number of fine
    // interpolation bits.
    unsigned int locationWidth = 1 << fineBits;
    // float arrays for f(x) and x.
    float x_points[locationTotal + 1];
    float y_points[locationTotal + 1];
    for (int i = 0; i < (locationTotal + 1); i++) {
        x_points[i] = (scaleDomain) * ((float)i / locationTotal) + biasDomain;
        // create x between 0 and scaleDomain
        if (x_points[i] == 0) {
            // protection against division by 0
            y_points[i] = scaleDomain;
        } else {
            y_points[i] = 1 / sqrt(x_points[i]);
        }
    }
    // create slope/offset lookup using x_points and y_points
    createLut<T_D, T_L>((T_L*)&lut_values[0], &x_points[0], &y_points[0], locationTotal, locationWidth, domainMode,
                        shift);
}
/**
 * @ingroup func_approx_utils
 * @brief getLog is a utility function that will create a lookup table to be used for the approximation of the natural
 * log function of x. The recommended domainMode (TP_DOMAIN_MODE) for getLog is 1.
 * @tparam T_D Describes the type of data that will be input to the func_approx.
 * @tparam T_L Describes the type of the values stored in the lookup table. This should be equal to T_D unless T_D is
 * bfloat16, in which case T_L should be float.
 * @param[out] lut_values A pointer to the area where the lookup table of slope and offset values will be created.
 * @param[in] coarseBits Describes the number of bits in a sample of input data that will be used to address the
 * provided lookup table. It determines the total number of locations in the lookup table.
 * @param[in] fineBits Describes the number of bits in an input data sample used for fine interpolation. It determines
 * the width of each location.
 * @param[in] domainMode The choice of TP_DOMAIN_MODE template parameter. This creates an approximation of f(x) where x
 * is normalized over a specified domain. \n
 *            This can be: \n
 *               - 0 (input domain is 0 <= x < 1) \n
 *               - 1 (input domain is 1 <= x < 2) \n
 *               - 2 (input domain is 1 <= x < 4). \n
 * @note If domainMode = 1, where the input domain is 1 <= x < 2, it is expected that the most significant bit of
 * coarseBits has been set to zero and will be ignored by the function approximation kernel when addressing the lookup
 * table. \n
 * This means that the lookup table will be half the size of other domainMode (TP_DOMAIN_MODE) with a similar coarseBits
 * value. \n
 * For domainMode values equal to 0 or 2, there are 2 ^ (coarseBits) locations, and when domainMode is equal to 1, there
 * are 2 ^ (coarseBits - 1) locations.
 * @param[in] shift The downward shift value that will be applied to each offset of a location. This is only applicable
 * to lookup tables with an integer type offset value.
 */
template <typename T_D, typename T_L>
void getLog(T_L* lut_values, const int coarseBits, const int fineBits, const int domainMode, const int shift) {
    // When TP_DOMAIN_MODE = 1, x_points are biased to be in domain 1 to 2
    unsigned int biasDomain = (domainMode == 1) ? 1 : 0;
    // When TP_DOMAIN_MODE = 2, x_points are scaled to be in domain 0 to 4.
    unsigned int scaleDomain = (domainMode == 2) ? 4 : 1;

    // Total number of locations in LUT. A location contains a slope value and an office value.
    unsigned int locationTotal = (1 << (coarseBits - biasDomain));
    // Width of a location (distance in x domain between each location) is determined by the number of fine
    // interpolation bits.
    unsigned int locationWidth = 1 << fineBits;
    // float arrays for f(x) and x.
    float x_points[locationTotal + 1];
    float y_points[locationTotal + 1];
    for (int i = 0; i < (locationTotal + 1); i++) {
        // create x between 0 and scaleDomain
        x_points[i] = (scaleDomain) * ((float)i / locationTotal) + biasDomain;
        // y_points = func(x_points)
        y_points[i] = log(x_points[i]);
    }
    // create slope/offset lookup using x_points and y_points
    createLut<T_D, T_L>((T_L*)&lut_values[0], &x_points[0], &y_points[0], locationTotal, locationWidth, domainMode,
                        shift);
}
/**
 * @ingroup func_approx_utils
 * @brief getExp is a utility function that will create a lookup table to be used for the approximation of the
 * exponential function of x. The recommended domainMode (TP_DOMAIN_MODE) for getExp is 0.
 * @tparam T_D Describes the type of data that will be input to the func_approx.
 * @tparam T_L Describes the type of the values stored in the lookup table. This should be equal to T_D unless T_D is
 * bfloat16, in which case T_L should be float.
 * @param[out] lut_values A pointer to the area where the lookup table of slope and offset values will be created.
 * @param[in] coarseBits Describes the number of bits in a sample of input data that will be used to address the
 * provided lookup table. It determines the total number of locations in the lookup table.
 * @param[in] fineBits Describes the number of bits in an input data sample used for fine interpolation. It determines
 * the width of each location.
 * @param[in] domainMode The choice of TP_DOMAIN_MODE template parameter. This creates an approximation of f(x) where x
 * is normalized over a specified domain. \n
 *            This can be: \n
 *               - 0 (input domain is 0 <= x < 1) \n
 *               - 1 (input domain is 1 <= x < 2) \n
 *               - 2 (input domain is 1 <= x < 4). \n
 * @note If domainMode = 1, where the input domain is 1 <= x < 2, it is expected that the most significant bit of
 * coarseBits has been set to zero and will be ignored by the function approximation kernel when addressing the lookup
 * table. \n
 * This means that the lookup table will be half the size of other domainMode (TP_DOMAIN_MODE) with a similar coarseBits
 * value. \n
 * For domainMode values equal to 0 or 2, there are 2 ^ (coarseBits) locations, and when domainMode is equal to 1, there
 * are 2 ^ (coarseBits - 1) locations.
 * @param[in] shift The downward shift value that will be applied to each offset of a location. This is only applicable
 * to lookup tables with an integer type offset value.
 */
template <typename T_D, typename T_L>
void getExp(T_L* lut_values, const int coarseBits, const int fineBits, const int domainMode, const int shift) {
    // When TP_DOMAIN_MODE = 1, x_points are biased to be in domain 1 to 2
    unsigned int biasDomain = (domainMode == 1) ? 1 : 0;
    // When TP_DOMAIN_MODE = 2, x_points are scaled to be in domain 0 to 4.
    unsigned int scaleDomain = (domainMode == 2) ? 4 : 1;

    // Total number of locations in LUT. A location contains a slope value and an office value.
    unsigned int locationTotal = (1 << (coarseBits - biasDomain));
    // Width of a location (distance in x domain between each location) is determined by the number of fine
    // interpolation bits.
    unsigned int locationWidth = 1 << fineBits;
    // float arrays for f(x) and x.
    float x_points[locationTotal + 1];
    float y_points[locationTotal + 1];
    for (int i = 0; i < (locationTotal + 1); i++) {
        // create x between 0 and scaleDomain
        x_points[i] = (scaleDomain) * ((float)i / locationTotal) + biasDomain;
        // y_points = func(x_points)
        y_points[i] = exp(x_points[i]);
    }
    // create slope/offset lookup using x_points and y_points
    createLut<T_D, T_L>((T_L*)&lut_values[0], &x_points[0], &y_points[0], locationTotal, locationWidth, domainMode,
                        shift);
}
/**
 * @ingroup func_approx_utils
 * @brief getInv is a utility function that will create a lookup table to be used for the approximation of the inverse
 * of x. The recommended domainMode (TP_DOMAIN_MODE) for getInv is 2.
 * @tparam T_D Describes the type of data that will be input to the func_approx.
 * @tparam T_L Describes the type of the values stored in the lookup table. This should be equal to T_D unless T_D is
 * bfloat16, in which case T_L should be float.
 * @param[out] lut_values A pointer to the area where the lookup table of slope and offset values will be created.
 * @param[in] coarseBits Describes the number of bits in a sample of input data that will be used to address the
 * provided lookup table. It determines the total number of locations in the lookup table.
 * @param[in] fineBits Describes the number of bits in an input data sample used for fine interpolation. It determines
 * the width of each location.
 * @param[in] domainMode The choice of TP_DOMAIN_MODE template parameter. This creates an approximation of f(x) where x
 * is normalized over a specified domain. \n
 *            This can be: \n
 *               - 0 (input domain is 0 <= x < 1) \n
 *               - 1 (input domain is 1 <= x < 2) \n
 *               - 2 (input domain is 1 <= x < 4). \n
 * @note If domainMode = 1, where the input domain is 1 <= x < 2, it is expected that the most significant bit of
 * coarseBits has been set to zero and will be ignored by the function approximation kernel when addressing the lookup
 * table. \n
 * This means that the lookup table will be half the size of other domainMode (TP_DOMAIN_MODE) with a similar coarseBits
 * value. \n
 * For domainMode values equal to 0 or 2, there are 2 ^ (coarseBits) locations, and when domainMode is equal to 1, there
 * are 2 ^ (coarseBits - 1) locations.
 * @param[in] shift The downward shift value that will be applied to each offset of a location. This is only applicable
 * to lookup tables with an integer type offset value.
 */
template <typename T_D, typename T_L>
void getInv(T_L* lut_values, const int coarseBits, const int fineBits, const int domainMode, const int shift) {
    // When TP_DOMAIN_MODE = 1, x_points are biased to be in domain 1 to 2
    unsigned int biasDomain = (domainMode == 1) ? 1 : 0;
    // When TP_DOMAIN_MODE = 2, x_points are scaled to be in domain 0 to 4.
    unsigned int scaleDomain = (domainMode == 2) ? 4 : 1;

    // Total number of locations in LUT. A location contains a slope value and an office value.
    unsigned int locationTotal = (1 << (coarseBits - biasDomain));
    // Width of a location (distance in x domain between each location) is determined by the number of fine
    // interpolation bits.
    unsigned int locationWidth = 1 << fineBits;
    // float arrays for f(x) and x.
    float x_points[locationTotal + 1];
    float y_points[locationTotal + 1];
    for (int i = 0; i < (locationTotal + 1); i++) {
        // create x between 0 and scaleDomain
        x_points[i] = (scaleDomain) * ((float)i / locationTotal) + biasDomain;
        // y_points = func(x_points)
        // Avoid division by zero
        if (x_points[i] >= 0.125) {
            y_points[i] = 1 / x_points[i];
        } else {
            y_points[i] = 8;
        }
    }
    // create slope/offset lookup using x_points and y_points
    createLut<T_D, T_L>((T_L*)&lut_values[0], &x_points[0], &y_points[0], locationTotal, locationWidth, domainMode,
                        shift);
}
}
}
}
}
#endif
