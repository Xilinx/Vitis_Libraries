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
#ifndef _DSPLIB_FUNC_APPROX_WINDOW_FNS_HPP_
#define _DSPLIB_FUNC_APPROX_WINDOW_FNS_HPP_

#include <vector>

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

    std::vector<float> slopes(locationTotal);
    std::vector<float> offsets(locationTotal);
    int64_t offsetAccum;
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
    std::vector<float> slopes(locationTotal);
    std::vector<float> offsets(locationTotal);
    for (int i = 0; i < (locationTotal); i++) {
        slopes[i] = (y_values[i + 1] - y_values[i]) / (((x_values[i + 1]) - x_values[i]));
        offsets[i] = (y_values[i] - (slopes[i] * (x_values[i] - biasDomain)));
    }
    for (int i = 0; i < (locationTotal); i++) {
        lut_values[2 * i] = slopes[i];
        lut_values[2 * i + 1] = offsets[i];
    }
}
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
    std::vector<float> slopes(locationTotal);
    std::vector<float> offsets(locationTotal);
    for (int i = 0; i < (locationTotal); i++) {
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
/**
 * @brief Domain Mode Explanation
 *
 * The lookup table generation supports three domain modes for input normalization:
 *
 * **domainMode = 0**: Input domain [0, 1)
 * - LUT size: 2^coarseBits locations
 * - Full range utilization
 * - Best for functions naturally bounded in [0,1)
 *
 * **domainMode = 1**: Input domain [1, 2)
 * - LUT size: 2^(coarseBits-1) locations (half size of domainMode 0 and 1)
 * - Assumes MSB of input is always 1, should be cleared before kernel processing
 * - Optimal for functions with natural [1,2) domain (e.g., log)
 *
 * **domainMode = 2**: Input domain [1, 4)
 * - LUT size: 2^coarseBits locations
 * - First quadrant of LUT (0 to 1) unused but allocated
 * - May produce inaccurate results for inputs in [0,1)
 */

/**
 * @ingroup func_approx_utils
 * @brief getSqrt creates a lookup table for approximating the square-root function sqrt(x).
 *
 * This function generates a piecewise linear approximation using slope-offset pairs.
 * The recommended domainMode for getSqrt is 2 (domain 1 ≤ x < 4).
 *
 * @tparam T_D Data type that will be input to the func_approx kernel (e.g., int16, int32, float, bfloat16).
 * @tparam T_L Lookup table storage type. Should equal T_D except when T_D is bfloat16, then T_L should be float.
 *
 * @param[out] lut_values A pointer to the memory where the lookup table of slope and offset values will be created.
 * @param[in] coarseBits Number of most significant bits used for table indexing. Determines table size:
 *                       - domainMode 0,2: 2^coarseBits locations
 *                       - domainMode 1: 2^(coarseBits-1) locations
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
 * @par Example:
 * @code
 * float lut[512]; // 256 slope-offset pairs
 * getSqrt<float, float>(lut, 8, 8, 2, 0); // 8-bit addressing, 8-bit interpolation, domain [1,4)
 * @endcode
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
    std::vector<float> x_points(locationTotal + 1);
    std::vector<float> y_points(locationTotal + 1);
    // y_points and x_points require an additional point in order to calculate the slope of the final location.
    for (int i = 0; i < (locationTotal + 1); i++) {
        // create x between 0 and scaleDomain
        x_points[i] = (scaleDomain) * ((float)i / locationTotal) + biasDomain;
        // y_points = func(x_points)
        y_points[i] = sqrt(x_points[i]);
    }
    // create slope/offset lookup using x_points and y_points
    createLut<T_D, T_L>((T_L*)&lut_values[0], x_points.data(), y_points.data(), locationTotal, locationWidth,
                        domainMode, shift);
}
/**
 * @ingroup func_approx_utils
 * @brief getInvSqrt creates a lookup table for approximating the inverse square-root function 1/sqrt(x).
 *
 * This function generates a piecewise linear approximation for the inverse square root using slope-offset pairs.
 * The recommended domainMode for getInvSqrt is 2 (domain 1 ≤ x < 4).
 *
 * @tparam T_D Data type that will be input to the func_approx kernel (e.g., int16, int32, float, bfloat16).
 * @tparam T_L Lookup table storage type. Should equal T_D except when T_D is bfloat16, then T_L should be float.
 *
 * @param[out] lut_values A pointer to the memory where the lookup table of slope and offset values will be created.
 * @param[in] coarseBits Number of most significant bits used for table indexing. Determines table size:
 *                       - domainMode 0,2: 2^coarseBits locations
 *                       - domainMode 1: 2^(coarseBits-1) locations
 * @param[in] fineBits Number of least significant bits used for linear interpolation within each table entry.
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
 * @par Example:
 * @code
 * float lut[512]; // 256 slope-offset pairs
 * getInvSqrt<float, float>(lut, 8, 8, 2, 0); // For domain [1,4)
 * @endcode
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
    std::vector<float> x_points(locationTotal + 1);
    std::vector<float> y_points(locationTotal + 1);
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
    createLut<T_D, T_L>((T_L*)&lut_values[0], x_points.data(), y_points.data(), locationTotal, locationWidth,
                        domainMode, shift);
}
/**
 * @ingroup func_approx_utils
 * @brief getLog creates a lookup table for approximating the natural logarithm function ln(x).
 *
 * This function generates a piecewise linear approximation for the natural logarithm using slope-offset pairs.
 * The recommended domainMode for getLog is 1 (domain 1 ≤ x < 2).
 *
 * @tparam T_D Data type that will be input to the func_approx kernel (e.g., int16, int32, float, bfloat16).
 * @tparam T_L Lookup table storage type. Should equal T_D except when T_D is bfloat16, then T_L should be float.
 *
 * @param[out] lut_values A pointer to the memory where the lookup table of slope and offset values will be created.
 * @param[in] coarseBits Number of most significant bits used for table indexing. Determines table size:
 *                       - domainMode 0,2: 2^coarseBits locations
 *                       - domainMode 1: 2^(coarseBits-1) locations
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
 * @note For domainMode=1, input values must have their MSB cleared before table lookup since it's
 *       always 1 in the domain [1,2) and provides no indexing information.
 *
 * @par Example:
 * @code
 * float lut[256]; // 128 slope-offset pairs
 * getLog<float, float>(lut, 7, 8, 1, 0); // 7-bit addressing, domain [1,2)
 * @endcode
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
    std::vector<float> x_points(locationTotal + 1);
    std::vector<float> y_points(locationTotal + 1);
    for (int i = 0; i < (locationTotal + 1); i++) {
        // create x between 0 and scaleDomain
        x_points[i] = (scaleDomain) * ((float)i / locationTotal) + biasDomain;
        // y_points = func(x_points)
        y_points[i] = log(x_points[i]);
    }
    // create slope/offset lookup using x_points and y_points
    createLut<T_D, T_L>((T_L*)&lut_values[0], x_points.data(), y_points.data(), locationTotal, locationWidth,
                        domainMode, shift);
}
/**
 * @ingroup func_approx_utils
 * @brief getExp creates a lookup table for approximating the exponential function exp(x) = e^x.
 *
 * This function generates a piecewise linear approximation for the exponential function using slope-offset pairs.
 * The recommended domainMode for getExp is 0 (domain 0 ≤ x < 1).
 *
 * @tparam T_D Data type that will be input to the func_approx kernel (e.g., int16, int32, float, bfloat16).
 * @tparam T_L Lookup table storage type. Should equal T_D except when T_D is bfloat16, then T_L should be float.
 *
 * @param[out] lut_values A pointer to the memory where the lookup table of slope and offset values will be created.
 * @param[in] coarseBits Number of most significant bits used for table indexing. Determines table size:
 *                       - domainMode 0,2: 2^coarseBits locations
 *                       - domainMode 1: 2^(coarseBits-1) locations
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
 * @par Example:
 * @code
 * float lut[512]; // 256 slope-offset pairs
 * getExp<float, float>(lut, 8, 8, 0, 0); // 8-bit addressing, domain [0,1)
 * @endcode
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
    std::vector<float> x_points(locationTotal + 1);
    std::vector<float> y_points(locationTotal + 1);
    for (int i = 0; i < (locationTotal + 1); i++) {
        // create x between 0 and scaleDomain
        x_points[i] = (scaleDomain) * ((float)i / locationTotal) + biasDomain;
        // y_points = func(x_points)
        y_points[i] = exp(x_points[i]);
    }
    // create slope/offset lookup using x_points and y_points
    createLut<T_D, T_L>((T_L*)&lut_values[0], x_points.data(), y_points.data(), locationTotal, locationWidth,
                        domainMode, shift);
}
/**
 * @ingroup func_approx_utils
 * @brief getInv creates a lookup table for approximating the inverse function 1/x.
 *
 * This function generates a piecewise linear approximation for the reciprocal function using slope-offset pairs.
 * The recommended domainMode for getInv is 2 (domain 1 ≤ x < 4).
 *
 * @tparam T_D Data type that will be input to the func_approx kernel (e.g., int16, int32, float, bfloat16).
 * @tparam T_L Lookup table storage type. Should equal T_D except when T_D is bfloat16, then T_L should be float.
 *
 * @param[out] lut_values A pointer to the memory where the lookup table of slope and offset values will be created.
 * @param[in] coarseBits Number of most significant bits used for table indexing. Determines table size:
 *                       - domainMode 0,2: 2^coarseBits locations
 *                       - domainMode 1: 2^(coarseBits-1) locations
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
 * @par Example:
 * @code
 * float lut[512]; // 256 slope-offset pairs
 * getInv<float, float>(lut, 8, 8, 2, 0); // 8-bit addressing, domain [1,4)
 * @endcode
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
    std::vector<float> x_points(locationTotal + 1);
    std::vector<float> y_points(locationTotal + 1);
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
    createLut<T_D, T_L>((T_L*)&lut_values[0], x_points.data(), y_points.data(), locationTotal, locationWidth,
                        domainMode, shift);
}
}
}
}
}
#endif
