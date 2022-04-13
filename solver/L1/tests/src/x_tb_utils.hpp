/*
 * Copyright 2021 Xilinx, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the Licens/.
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

#ifndef _XF_SOLVER_X_TB_UTILS_HPP_
#define _XF_SOLVER_X_TB_UTILS_HPP_

#include <iostream>
#include <sstream>
#include <cstdio>
#include <math.h>
#include <time.h>
#include "utils/x_hls_utils.h"
#include "ap_fixed.h"
#include <string.h>
#include <limits>
#include "utils/x_hls_defines.h"
#include "utils/x_hls_traits.h"
#include <cmath>
#include <stdlib.h>
#include "hls_half.h"
#include "hls_x_complex.h"
#include <complex>

namespace solver_tb {
template <bool B, class T = void>
struct enable_if {};

template <class T>
struct enable_if<true, T> {
    typedef T type;
};
template <typename T, T _v>
struct integral_constant {
    static const T value = _v;
    typedef T value_type;
    typedef integral_constant<T, _v> type;
    operator value_type() { return value; }
};

typedef integral_constant<bool, true> true_type;
typedef integral_constant<bool, false> false_type;

template <typename T1, typename T2>
struct is_same;

template <typename T1, typename T2>
struct is_same : public false_type {};

template <typename T1>
struct is_same<T1, T1> : public true_type {};

template <typename T>
struct is_arithmetic : public integral_constant<bool, (is_integraltype<T>::value || is_fptype<T>::value)> {};
}

#include <typeinfo>
#include <cxxabi.h>

// Dump a C++ type... useful for debugging templates.
template <typename T>
static void dump_type(std::ostream& os) {
    size_t len;
    int s;
    char* p = abi::__cxa_demangle(typeid(T).name(), 0, &len, &s);
    os << p;
}

// return the max/min value of a specific integer type
template <typename T>
class max_min_ {
   public:
    max_min_() {
        _max = ((T)(-1) > 0) ? (((1ULL << Type_BitWidth<T>::Value) - 1) * 2 + 1)
                             : ((1ULL << (Type_BitWidth<T>::Value - 1)) - 1);
        _min = ((T)(-1) > 0) ? 0 : -(1ULL << (Type_BitWidth<T>::Value - 1));
    }
    T _max;
    T _min;
};

template <bool b>
struct binary_spec {
    template <class T>
    static std::string f(T x);
};

template <>
struct binary_spec<true> {
    template <class T>
    static std::string f(T x) {
        std::stringstream s;
        s << std::hex << x;
        return s.str();
    }
};

template <>
struct binary_spec<false> {
    template <class T>
    static std::string f(T x) {
        char s2[1024];
        snprintf(s2, 1024, "%a", x);
        std::stringstream s;
        s << std::hex << fp_struct<T>(x).data() << " " << s2;
        return s.str();
    }
};

template <class T>
std::string binary(T x) {
    return binary_spec<std::numeric_limits<T>::is_integer>::f(x);
}

#ifdef X_AP_FLOAT_H
template <int W, int I, int E_W, bool N>
std::string binary(ap_float<W, I, E_W, N> x) {
    return x.to_string(16);
}
#endif

template <int W, int I, ap_q_mode Q, ap_o_mode O>
std::string binary(ap_fixed<W, I, Q, O> x) {
    return x.to_string(16);
}

template <int W, int I, ap_q_mode Q, ap_o_mode O>
std::string binary(ap_ufixed<W, I, Q, O> x) {
    return x.to_string(16);
}

template <int W>
std::string binary(ap_int<W> x) {
    return x.to_string(16);
}

template <int W>
std::string binary(ap_uint<W> x) {
    return x.to_string(16);
}

static std::string binary(std::string x) {
    return x;
}

static std::string binary(half x) {
    float f = x;
    char s2[1024];
    snprintf(s2, 1024, "%a", f);
    std::stringstream s;
    s << std::hex << x.get_bits() << " " << s2;
    return s.str();
}
namespace solver_tb {
#ifdef X_AP_FLOAT_H
template <int W, int I, int E_W, bool N>
ap_float<W + 1, I + 1, E_W, N> fabs(ap_float<W, I, E_W, N> x) {
    return x > 0 ? ap_float<W + 1, I + 1, E_W, N>(x) : ap_float<W + 1, I + 1, E_W, N>(-x);
}
#endif

template <int W, int I, ap_q_mode Q, ap_o_mode O>
ap_fixed<W + 1, I + 1, Q, O> fabs(ap_fixed<W, I, Q, O> x) {
    return x > 0 ? ap_fixed<W + 1, I + 1, Q, O>(x) : ap_fixed<W + 1, I + 1, Q, O>(-x);
}

template <int W, int I, ap_q_mode Q, ap_o_mode O>
ap_ufixed<W + 1, I + 1, Q, O> fabs(ap_ufixed<W, I, Q, O> x) {
    return x;
}

template <int W>
ap_int<W + 1> fabs(ap_int<W> x) {
    return x > 0 ? ap_int<W + 1>(x) : ap_int<W + 1>(-x);
}

template <int W>
ap_uint<W + 1> fabs(ap_uint<W> x) {
    return x;
}

template <typename T>
T fabs(T x) {
    return std::fabs(x);
}
}

#ifdef X_AP_FLOAT_H
template <int W, int I, int E_W, bool N>
bool generic_isnan(ap_float<W, I, E_W, N> x) {
    return false;
}
#endif

template <int W, int I, ap_q_mode Q, ap_o_mode O>
bool generic_isnan(ap_fixed<W, I, Q, O> x) {
    return false;
}

template <int W, int I, ap_q_mode Q, ap_o_mode O>
bool generic_isnan(ap_ufixed<W, I, Q, O> x) {
    return false;
}

template <int W>
bool generic_isnan(ap_int<W> x) {
    return false;
}

template <int W>
bool generic_isnan(ap_uint<W> x) {
    return false;
}

#ifndef AESL_SYN
static bool generic_isnan(half x) {
    return detail::isnan(x);
}
#endif

template <typename T>
bool generic_isnan(T x) {
    return std::isnan(x);
}

#ifdef X_AP_FLOAT_H
template <int W, int I, int E_W, bool N>
bool generic_issubnormal(ap_float<W, I, E_W, N> x) {
    return false;
}
#endif
template <int W, int I, ap_q_mode Q, ap_o_mode O>
bool generic_issubnormal(ap_fixed<W, I, Q, O> x) {
    return false;
}

template <int W, int I, ap_q_mode Q, ap_o_mode O>
bool generic_issubnormal(ap_ufixed<W, I, Q, O> x) {
    return false;
}

template <int W>
bool generic_issubnormal(ap_int<W> x) {
    return false;
}

template <int W>
bool generic_issubnormal(ap_uint<W> x) {
    return false;
}

static bool generic_issubnormal(half x) {
    return (detail::fpclassify(x) == FP_SUBNORMAL);
}

template <typename T>
bool generic_issubnormal(T x) {
    return (std::fpclassify(x) == FP_SUBNORMAL);
}

static void load_from_file(char* file_name, int* destination_array, int array_size) {
    /*======================================================*/
    /* Loads the data from .txt file created in MATLAB.     */
    /* Used in testbench scenarios to perform data testing	*/
    /*======================================================*/

    FILE* file_id;
    int counter;

    printf("Loading from file %s\n", file_name);
    file_id = fopen(file_name, "r");
    if (file_id != NULL) {
        printf("File name valid!\n");
        for (counter = 0; counter < array_size; counter++) {
            fscanf(file_id, "%i", &destination_array[counter]);
        }
        fclose(file_id);
    } else {
        printf("File %s not found.\n", file_name);
    }
}

template <typename T>
void load_from_file(char* file_name, T* destination_array, int array_size) {
    /*======================================================*/
    /* Loads the data from .txt file created in MATLAB.     */
    /* Used in testbench scenarios to perform data testing	*/
    /*======================================================*/

    FILE* file_id;
    int counter;

    printf("Loading from file %s\n", file_name);
    file_id = fopen(file_name, "r");
    if (file_id != NULL) {
        printf("File name valid!\n");
        for (counter = 0; counter < array_size; counter++) {
            fscanf(file_id, "%e", &destination_array[counter]);
        }
        fclose(file_id);
    } else {
        printf("File %s not found.\n", file_name);
    }
}

static void save_to_file(char* file_name, int* source_array, int array_size) {
    /*======================================================*/
    /* Saves the data to a txt. The files are used			*/
    /* for comparison to expected, MATLAB generated values	*/
    /*======================================================*/

    FILE* file_id;
    int counter;

    file_id = fopen(file_name, "w+");
    if (file_id != NULL) {
        for (counter = 0; counter < array_size; counter++) {
            fprintf(file_id, "%d\n", source_array[counter]);
        }
        fclose(file_id);
    }
}

template <typename T>
void save_to_file(char* file_name, T* source_array, int array_size) {
    /*======================================================*/
    /* Saves the data to a txt. The files are used			*/
    /* for comparison to expected, MATLAB generated values	*/
    /*======================================================*/

    FILE* file_id;
    int counter;

    file_id = fopen(file_name, "w+");
    if (file_id != NULL) {
        for (counter = 0; counter < array_size; counter++) {
            fprintf(file_id, "%e\n", source_array[counter]);
        }
        fclose(file_id);
    }
}

/*
* return value
* 0: failed, 1: passed
*/

/*
* Compare values approximately.
* Returns true if error is within a certain threshold (0.1%)
*/
static int comp_approx(float d1, float d2, float base, float err) {
    float diff = solver_tb::fabs(d1 - d2);
    float perc = 100 * diff / base;
    printf("comp_approx (%e, %e), delta: %e, base: %e\n", d1, d2, perc, base);
    if (perc >= err) printf("Exceed error threshold!\n");
    return (perc < err);
}

template <int DA, int DMA, ap_q_mode Q, ap_o_mode O>
int compare_files(
    ap_fixed<DA, DMA, Q, O> dummy, char* expected_file, char* generated_file, int start_index, int stop_index) {
    /*======================================================*/
    /* Compares the values from two txt files. If the		*/
    /* the values differ, the values are printed in the		*/
    /* in the command line. It is possible to specify the   */
    /* range of the compared values							*/
    /*======================================================*/

    FILE *exp_file_id, *gen_file_id;
    int counter;
    int error_count = 0;
    int expected_value, generated_value;

    exp_file_id = fopen(expected_file, "r");
    gen_file_id = fopen(generated_file, "r");
    if (exp_file_id == NULL) {
        printf("Failed to open expected file!\n");
        return 0;
    } else {
        if (gen_file_id == NULL) {
            printf("Failed to open generated file!\n");
            return 0;
        } else {
            for (counter = 0; counter < stop_index; counter++) {
                fscanf(exp_file_id, "%i", &expected_value);
                fscanf(gen_file_id, "%i", &generated_value);
                if (counter >= start_index - 1) {
                    if (expected_value != generated_value) {
                        // Debug code
                        error_count = error_count + 1;
                        if (error_count <= 10) {
                            printf("Error! Line: %i, Expected: %i; Generated: %i\n", counter, expected_value,
                                   generated_value);
                        }
                        if (error_count == 11) {
                            printf("To many errors, abort displaying...\n");
                        }

                        return 0;
                    }
                }
            }
            fclose(gen_file_id);
        }
        fclose(exp_file_id);
    }

    return 1;
}

static int scan_file_for_avg(float dummy, char* target_file, int start_index, int stop_index, float& avg) {
    FILE* file_id;
    int counter;
    float value;

    avg = 0.0;

    file_id = fopen(target_file, "r");
    if (file_id == NULL) {
        printf("Failed to open file!\n");
        return 0;
    } else {
        counter = 0;
        while (counter < stop_index) {
            fscanf(file_id, "%e", &value);
            if (counter >= start_index - 1) {
                avg += solver_tb::fabs(value);
                counter++;
                //		  printf("current avg = %e (%e/%d), val: %e\n",avg/counter,avg,counter,value);
            }
        }
        fclose(file_id);
    }
    avg = avg / counter;
    printf("Scan for file avg: %e (%d)\n", avg, counter);

    return 1;
}

static int compare_files(
    float dummy, char* expected_file, char* generated_file, int start_index, int stop_index, float base, float err) {
    /*======================================================*/
    /* Compares the values from two txt files. If the		*/
    /* the values differ, the values are printed in the		*/
    /* in the command line. It is possible to specify the   */
    /* range of the compared values							*/
    /*======================================================*/

    //	float base = 0.0;
    //	scan_file_for_avg(dummy, expected_file, start_index, stop_index, base);

    FILE *exp_file_id, *gen_file_id;
    int counter;
    int error_count = 0;
    float expected_value, generated_value;

    exp_file_id = fopen(expected_file, "r");
    gen_file_id = fopen(generated_file, "r");
    if (exp_file_id == NULL) {
        printf("Failed to open expected file!\n");
        return 0;
    } else {
        if (gen_file_id == NULL) {
            printf("Failed to open generated file!\n");
            return 0;
        } else {
            for (counter = 0; counter < stop_index; counter++) {
                fscanf(exp_file_id, "%e", &expected_value);
                fscanf(gen_file_id, "%e", &generated_value);
                if (counter >= start_index - 1) {
                    // if(expected_value != generated_value)
                    if (!comp_approx(expected_value, generated_value, base, err)) {
                        // Debug code
                        error_count = error_count + 1;
                        if (error_count <= 10) {
                            printf("Error! Line: %i, Expected: %e; Generated: %e\n", counter, expected_value,
                                   generated_value);
                        }
                        if (error_count == 11) {
                            printf("To many errors, abort displaying...\n");
                        }

                        return 0;
                    }
                }
            }
            fclose(gen_file_id);
        }
        fclose(exp_file_id);
    }

    return 1;
}

static void dump(float a) {
    printf("%e\n", a);
}

template <int W, int I, ap_q_mode Q, ap_o_mode O>
void dump(ap_fixed<W, I, Q, O> a) {
    printf("%e\n", a.to_float());
}

// static
// void
// dump(x_complex<float> a) {
// 	printf("(%e,%e)\n",a.real(),a.imag());
// }

template <int W, int E>
void dump(float_struct<W, E> a) {
    printf("(mant.x,exp.d,sign.d)(%x,%d,%d)\n", a.mant.to_uint(), a.exp.to_uint(), a.sign.to_uint());
}

// template<int W, int E>
// void
// dump(x_complex<float_struct<W,E> > a) {
// 	printf("(mant.x,exp.d,sign.d)(%x,%d,%d)(%x,%d,%d)\n",a.real().mant.to_uint(),a.real().exp.to_uint(),a.real().sign.to_uint(),a.imag().mant.to_uint(),a.imag().exp.to_uint(),a.imag().sign.to_uint());
// }

// template<int W>
// void
// dump(x_complex<ap_int<W> > a) {
// 	printf("(%x,%x)\n",a.real().to_int(),a.imag().to_int());
// }

// template<int W, int I, ap_q_mode Q, ap_o_mode O>
// void
// dump(x_complex<ap_fixed<W,I,Q,O> > a) {
// 	printf("(%e,%e)\n",a.real().to_int(),a.imag().to_int());
// }

template <int DIM>
void dumpv(float a[DIM]) {
    for (int i = 0; i < DIM; i++) {
        printf("%e ", a[i]);
    }
    printf("\n");
}

// template<int DIM>
// void
// dumpv(
// 	x_complex<float> a[DIM])
// {
// 	for(int i=0; i<DIM; i++) {
// 		printf("(%e,%e) ",a[i].real(),a[i].imag());
// 	}
// 	printf("\n");
// }

// template<int DIM, int W, int I, ap_q_mode Q, ap_o_mode O>
// void
// dumpv(
//       x_complex<ap_fixed<W,I,Q,O> > a[DIM])
// {
// 	for(int i=0; i<DIM; i++) {
// 		printf("(%e,%e) ",a[i].real().to_float(),a[i].imag().to_float());
// 	}
// 	printf("\n");
// }

template <int ROWS, int COLS, int _TDM_CHUNKS, int TRANS>
void dumpm(float a[ROWS][COLS][_TDM_CHUNKS]) {
    for (int i = 0; i < ROWS; i++) {
        for (int j = 0; j < COLS; j++) {
            float tmp = (TRANS) ? a[j][i][0] : a[i][j][0];
            printf("%e\t", tmp);
        }
        printf("\n");
    }
    printf("\n");
}

template <int ROWS, int COLS, int _TDM_CHUNKS, int TRANS, int W, int I, ap_q_mode Q, ap_o_mode O>
void dumpm(ap_fixed<W, I, Q, O> a[ROWS][COLS][_TDM_CHUNKS]) {
    for (int i = 0; i < ROWS; i++) {
        for (int j = 0; j < COLS; j++) {
            float tmp = (TRANS) ? a[j][i][0] : a[i][j][0];
            printf("%e\t", tmp);
        }
        printf("\n");
    }
    printf("\n");
}

// template<int ROWS, int COLS, int _TDM_CHUNKS, int TRANS, class T>
// void
// dumpm(
// 	x_complex<T> in_a[ROWS][COLS][_TDM_CHUNKS])
// {
// 	printf("Real matrix\n");
// 	for(int i=0; i<ROWS; i++) {
// 		for(int j=0; j<COLS; j++) {
// 			float tmp = (TRANS) ? (float)(in_a[j][i][0].real()) : (float)(in_a[i][j][0].real());
// 			printf("%e\t",tmp);
// 		}
// 		printf("\n");
// 	}
// 	printf("\n");
// 	printf("Imaginary matrix\n");
// 	for(int i=0; i<ROWS; i++) {
// 		for(int j=0; j<COLS; j++) {
// 			float tmp = (TRANS) ? (float)(-in_a[j][i][0].imag()) : (float)(in_a[i][j][0].imag());
// 			printf("%e\t",tmp);
// 		}
// 		printf("\n");
// 	}
// 	printf("\n");
// }

template <int DIM>
void dumpm_div_2(float a[DIM / 2][DIM / 2]) {
    for (int i = 0; i < DIM / 2; i++) {
        for (int j = 0; j < DIM / 2; j++) {
            printf("%e\t", a[i][j]);
        }
        printf("\n");
    }
    printf("\n");
}

/*
* chunks: <0 : all chunks, else chunk num
*
*/
template <int ROWS, int COLS, int CHUNKS>
void dumpmf(float a[ROWS][COLS][CHUNKS], int chunks) {
    int maxchunk, ch;

    if (chunks < 0) {
        maxchunk = CHUNKS;
        ch = 0;
    } else {
        maxchunk = chunks + 1;
        ch = chunks;
    }
    for (; ch < maxchunk; ch++) {
        for (int i = 0; i < ROWS; i++) {
            for (int j = 0; j < COLS; j++) {
                printf("%e\t", a[i][j][ch]);
            }
            printf("\n");
        }
        printf("\n");
    }
}

/*
* Dump out the complex transformation matrices
*/
template <int DIM>
void dumpct(float tl_w_re[DIM / 2][DIM / 2],
            float tl_x_re[DIM / 2][DIM / 2],
            float tl_y_re[DIM / 2][DIM / 2],
            float tl_z_re[DIM / 2][DIM / 2],
            float tl_w_im[DIM / 2][DIM / 2],
            float tl_x_im[DIM / 2][DIM / 2],
            float tl_y_im[DIM / 2][DIM / 2],
            float tl_z_im[DIM / 2][DIM / 2],
            float tr_w_re[DIM / 2][DIM / 2],
            float tr_x_re[DIM / 2][DIM / 2],
            float tr_y_re[DIM / 2][DIM / 2],
            float tr_z_re[DIM / 2][DIM / 2],
            float tr_w_im[DIM / 2][DIM / 2],
            float tr_x_im[DIM / 2][DIM / 2],
            float tr_y_im[DIM / 2][DIM / 2],
            float tr_z_im[DIM / 2][DIM / 2]) {
    printf("DEBUG: tl_w_re state\n");
    dumpm_div_2(tl_w_re);
    printf("DEBUG: tl_w_im state\n");
    dumpm_div_2(tl_w_im);

    printf("DEBUG: tl_x_re state\n");
    dumpm_div_2(tl_x_re);
    printf("DEBUG: tl_x_im state\n");
    dumpm_div_2(tl_x_im);

    printf("DEBUG: tl_y_re state\n");
    dumpm_div_2(tl_y_re);
    printf("DEBUG: tl_y_im state\n");
    dumpm_div_2(tl_y_im);

    printf("DEBUG: tl_z_re state\n");
    dumpm_div_2(tl_z_re);
    printf("DEBUG: tl_z_im state\n");
    dumpm_div_2(tl_z_im);

    printf("DEBUG: tr_w_re state\n");
    dumpm_div_2(tr_w_re);
    printf("DEBUG: tr_w_im state\n");
    dumpm_div_2(tr_w_im);

    printf("DEBUG: tr_x_re state\n");
    dumpm_div_2(tr_x_re);
    printf("DEBUG: tr_x_im state\n");
    dumpm_div_2(tr_x_im);

    printf("DEBUG: tr_y_re state\n");
    dumpm_div_2(tr_y_re);
    printf("DEBUG: tr_y_im state\n");
    dumpm_div_2(tr_y_im);

    printf("DEBUG: tr_z_re state\n");
    dumpm_div_2(tr_z_re);
    printf("DEBUG: tr_z_im state\n");
    dumpm_div_2(tr_z_im);
}

// template<class T1, class T2, class T3>
// void
// dump_bs(
// 	x_complex<T1> data_1, int i, int j, int k, x_complex<T2> R, x_complex<T3> tmp_inv)
// {
// 	float data_1_re_f = data_1.real();
// 	float data_1_im_f = data_1.imag();
// 	float R_re_f = R.real();
// 	float R_im_f = R.imag();
// 	float tmp_inv_re_f = tmp_inv.real();
// 	float tmp_inv_im_f = tmp_inv.imag();
// 	printf("\ndata_1 (re:%e, im: %e) / R[%i][%i][%i] (re: %e, im: %e) = tmp_inv (re: %e, im:
// %e)\n",data_1_re_f,data_1_im_f,i,j,k,R_re_f,R_im_f,tmp_inv_re_f,tmp_inv_im_f);
// }

template <class T1, class T2, class T3>
void dump_bs(T1 data_1, int i, int j, int k, T2 R, T3 tmp_inv) {
    float data_1_f = data_1;
    float R_f = R;
    float tmp_inv_f = tmp_inv;
    printf("\ndata_1 (%e) / R[%i][%i][%i] (%e) = tmp_inv (%e)\n", data_1_f, i, j, k, R_f, tmp_inv_f);
}

/*
* set matrix a values to matrix b
*/
template <int DIM>
void setm(float a[DIM][DIM], float b[DIM][DIM]) {
    for (int i = 0; i < DIM; i++) {
        for (int j = 0; j < DIM; j++) {
            b[i][j] = a[i][j];
        }
    }
}

// template<int DIM, int _TDM_CHUNKS, int tranA, int tranB, class T>
// void
// mm(
// //	bool tranA, bool tranB,
// 	x_complex<T> in_a[DIM][DIM][_TDM_CHUNKS],
// 	x_complex<T> in_b[DIM][DIM][_TDM_CHUNKS],
// 	x_complex<T> out_c[DIM][DIM][_TDM_CHUNKS])
// {
// 	int i,j,k;
// 	x_complex<T> tmp;
// 	for(i=0; i<DIM; i++) { // rows of A
// 		for(j=0; j<DIM; j++) 	{ // col of B
// 			tmp = 0;
// 			for(k=0; k<DIM; k++) 	{ // cols of A, rows of B
// 				x_complex<T> tmp_a = (tranA) ? x_conj(in_a[k][i][0]) : in_a[i][k][0];
// 				x_complex<T> tmp_b = (tranB) ? x_conj(in_b[j][k][0]) : in_b[k][j][0];

// //				printf("mm: (%d,%d,%d) tmp_a_re: %e, tmp_a_im: %e, tmp_b_re: %e, tmp_b_im:
// %e\n",i,j,k,tmp_a_re,tmp_a_im,tmp_b_re,tmp_b_im);
// 				tmp += (tmp_a * tmp_b);
// 			}
// 			out_c[i][j][0] = tmp;
// 		}
// 	}
// }

// /*
// * Like mm but does not conjugate when it transposes
// */
// template<int DIM, int _TDM_CHUNKS, int tranA, int tranB, class T>
// void
// mm2(
// 	x_complex<T> in_a[DIM][DIM][_TDM_CHUNKS],
// 	x_complex<T> in_b[DIM][DIM][_TDM_CHUNKS],
// 	x_complex<T> out_c[DIM][DIM][_TDM_CHUNKS])
// {
// 	int i,j,k;
// 	x_complex<T> tmp;
// 	for(i=0; i<DIM; i++) { // rows of A
// 		for(j=0; j<DIM; j++) 	{ // col of B
// 			tmp = 0;
// 			for(k=0; k<DIM; k++) 	{ // cols of A, rows of B
// 				x_complex<T> tmp_a = (tranA) ? in_a[k][i][0] : in_a[i][k][0];
// 				x_complex<T> tmp_b = (tranB) ? in_b[j][k][0] : in_b[k][j][0];

// //				printf("mm: (%d,%d,%d) tmp_a_re: %e, tmp_a_im: %e, tmp_b_re: %e, tmp_b_im:
// %e\n",i,j,k,tmp_a_re,tmp_a_im,tmp_b_re,tmp_b_im);
// 				tmp += (tmp_a * tmp_b);
// 			}
// 			out_c[i][j][0] = tmp;
// 		}
// 	}
// }

template <int DIM, int _TDM_CHUNKS, int tranA, int tranB>
void mm(float a[DIM][DIM][_TDM_CHUNKS], float b[DIM][DIM][_TDM_CHUNKS], float c[DIM][DIM][_TDM_CHUNKS]) {
    for (int i = 0; i < DIM; i++) {
        for (int j = 0; j < DIM; j++) {
            c[i][j][0] = vmult(a, b, i, j, DIM, tranA, tranB);
        }
    }
}

template <int DIM, int _TDM_CHUNKS, int tranA, int tranB, int W, int I>
void mm(ap_fixed<W, I> a[DIM][DIM][_TDM_CHUNKS],
        ap_fixed<W, I> b[DIM][DIM][_TDM_CHUNKS],
        ap_fixed<W, I> c[DIM][DIM][_TDM_CHUNKS]) {
    for (int i = 0; i < DIM; i++) {
        for (int j = 0; j < DIM; j++) {
            c[i][j][0] = vmult(a, b, i, j, DIM, tranA, tranB);
        }
    }
}

template <int DIM, int _TDM_CHUNKS>
void test_orthogonal(float a[DIM][DIM][_TDM_CHUNKS]) {
    float c[DIM][DIM];
    for (int i = 0; i < DIM; i++) {
        for (int j = 0; j < DIM; j++) {
            c[i][j] = vmult(a, a, i, j, DIM, false, true);
            printf("%e ", c[i][j]);
        }
        printf("\n");
    }
    printf("\n\n");
}

template <int DIM>
void swap_diag_elements(float r_out[DIM][DIM], int c1, int c2) {
    float tmp = r_out[c1][c1];
    r_out[c1][c1] = r_out[c2][c2];
    r_out[c2][c2] = tmp;
}

template <int DIM>
void swap_columns(float m[DIM][DIM], int c1, int c2) {
    float tmp[DIM];
    for (int i = 0; i < DIM; i++) {
        tmp[i] = m[i][c1];
    }
    for (int i = 0; i < DIM; i++) {
        m[i][c1] = m[i][c2];
        m[i][c2] = tmp[i];
    }
}

template <int DIM>
void abssort(float r_out[DIM][DIM], float u_out[DIM][DIM], float v_out[DIM][DIM]) {
    int i, j, k;
    float w, x, y, z;
    float i_perm[DIM][DIM];

    for (i = 0; i < DIM; i++) {
        for (j = 0; j < DIM; j++) {
            if (i == j) {
                i_perm[i][j] = (r_out[i][j] < 0.0f) ? -1.0f : 1.0f;
            } else {
                i_perm[i][j] = 0.0f;
            }
        }
    }
    float tmp[DIM][DIM];

    // r_out matrix
    printf("DEBUG: abssort, r_out before\n");
    dumpm<DIM, DIM>(r_out);

    mm<DIM>(i_perm, r_out, tmp);
    setm(tmp, r_out);

    printf("DEBUG: abssort, r_out after\n");
    dumpm<DIM, DIM>(r_out);

    // u_out matrix
    printf("DEBUG: abssort, u_out after\n");
    dumpm<DIM, DIM>(u_out);

    mm<DIM>(i_perm, u_out, tmp);
    setm(tmp, u_out);

    printf("DEBUG: abssort, u_out after\n");
    dumpm<DIM, DIM>(u_out);
}

template <int DIM>
void abssort2(float r_out[DIM][DIM], float u_out[DIM][DIM], float v_out[DIM][DIM]) {
    int i, j;

    // r_out matrix
    printf("DEBUG: abssort2, r_out before\n");
    dumpm<DIM, DIM>(r_out);

    // v_out matrix
    printf("DEBUG: abssort2, v_out before\n");
    dumpm<DIM, DIM>(v_out);

    for (j = 0; j < DIM; j++) {
        float tmps = r_out[j][j];
        if (tmps < 0.0f) {
            for (i = 0; i < DIM; i++) {
                float tmpv = v_out[i][j];
                v_out[i][j] = -tmpv;
            }
            r_out[j][j] = -tmps;
        }
    }

    printf("DEBUG: abssort2, r_out after\n");
    dumpm<DIM, DIM>(r_out);

    printf("DEBUG: abssort2, v_out after\n");
    dumpm<DIM, DIM>(v_out);
}

template <int DIM>
void dsort2(float s_out[DIM][DIM], float u_out[DIM][DIM], float v_out[DIM][DIM]) {
    int i, j, i_max;
    float S_max, Sj;

    printf("DEBUG: dsort2, s_out before\n");
    dumpm<DIM, DIM>(s_out);

    printf("DEBUG: dsort2, u_out before\n");
    dumpm<DIM, DIM>(u_out);

    printf("DEBUG: dsort2, v_out before\n");
    dumpm<DIM, DIM>(v_out);

    for (i = 0; i < DIM; i++) {
        S_max = s_out[i][i];
        i_max = i;
        for (j = i + 1; j < DIM; j++) {
            Sj = s_out[j][j];
            if (Sj > S_max) {
                S_max = Sj;
                i_max = j;
            }
        }
        if (i_max != i) {
            swap_diag_elements<DIM>(s_out, i, i_max);
            swap_columns<DIM>(u_out, i, i_max);
            swap_columns<DIM>(v_out, i, i_max);
        }
    }
    printf("DEBUG: dsort2, s_out after\n");
    dumpm<DIM, DIM>(s_out);

    printf("DEBUG: dsort2, u_out after\n");
    dumpm<DIM, DIM>(u_out);

    printf("DEBUG: dsort2, v_out after\n");
    dumpm<DIM, DIM>(v_out);
}

template <int DIM>
void swap_rows(float m[DIM][DIM], int c1, int c2) {
    float tmp[DIM];
    for (int i = 0; i < DIM; i++) {
        tmp[i] = m[c1][i];
    }
    for (int i = 0; i < DIM; i++) {
        m[c1][i] = m[c2][i];
        m[c2][i] = tmp[i];
    }
}

template <int DIM>
void swap_row_columns(float m[DIM][DIM], int c1, int c2) {
    float tmp[DIM];
    for (int i = 0; i < DIM; i++) {
        tmp[i] = m[c1][i];
    }
    for (int i = 0; i < DIM; i++) {
        m[c1][i] = m[c2][i];
        m[c2][i] = tmp[i];
    }
    swap_columns(m, c1, c2);
}

template <int DIM>
void clear_diagm(float s_re[DIM][DIM]) {
    for (int i = 0; i < DIM; i++) {
        for (int j = 0; j < DIM; j++) {
            if (i != j) s_re[i][j] = 0.0f;
        }
    }
}

template <int DIM>
void dumpcs(float c1i[DIM / 2][DIM / 2],
            float s1i[DIM / 2][DIM / 2],
            float c2i[DIM / 2][DIM / 2],
            float s2i[DIM / 2][DIM / 2],
            float k[DIM / 2][DIM / 2]) {
    printf("DEBUG: c1 state\n");
    for (int ti = 0; ti < DIM / 2; ti++) {
        for (int tj = 0; tj < DIM / 2; tj++) {
            printf("%e\t", c1i[ti][tj]);
        }
        printf("\n");
    }
    printf("\n");

    printf("DEBUG: s1 state\n");
    for (int ti = 0; ti < DIM / 2; ti++) {
        for (int tj = 0; tj < DIM / 2; tj++) {
            printf("%e\t", s1i[ti][tj]);
        }
        printf("\n");
    }
    printf("\n");

    printf("DEBUG: k state\n");
    for (int ti = 0; ti < DIM / 2; ti++) {
        for (int tj = 0; tj < DIM / 2; tj++) {
            printf("%e\t", k[ti][tj]);
        }
        printf("\n");
    }
    printf("\n");

    printf("DEBUG: c2 state\n");
    for (int ti = 0; ti < DIM / 2; ti++) {
        for (int tj = 0; tj < DIM / 2; tj++) {
            printf("%e\t", c2i[ti][tj]);
        }
        printf("\n");
    }
    printf("\n");

    printf("DEBUG: s2 state\n");
    for (int ti = 0; ti < DIM / 2; ti++) {
        for (int tj = 0; tj < DIM / 2; tj++) {
            printf("%e\t", s2i[ti][tj]);
        }
        printf("\n");
    }
    printf("\n");
}

template <int X_DIM, int Y_DIM, int _TDM_CHUNKS>
void vmultc(float* out_re,
            float* out_im,
            float a_re[X_DIM][X_DIM][_TDM_CHUNKS],
            float a_im[X_DIM][X_DIM][_TDM_CHUNKS],
            float b_re[X_DIM][X_DIM][_TDM_CHUNKS],
            float b_im[X_DIM][X_DIM][_TDM_CHUNKS],
            int i,
            int j,
            int l,
            bool a_t,
            bool b_t) {
    float tmp_re, tmp_im, aval_re, aval_im, bval_re, bval_im;
    tmp_re = 0.0;
    tmp_im = 0.0;
    for (int k = 0; k < l; k++) {
        aval_re = (a_t) ? a_re[k][i][0] : a_re[i][k][0];
        aval_im = (a_t) ? -a_im[k][i][0] : a_im[i][k][0];
        bval_re = (b_t) ? b_re[j][k][0] : b_re[k][j][0];
        bval_im = (b_t) ? -b_im[j][k][0] : b_im[k][j][0];
        tmp_re += aval_re * bval_re - aval_im * bval_im;
        tmp_im += aval_re * bval_im + aval_im * bval_re;
    }
    *out_re = tmp_re;
    *out_im = tmp_im;
}

// template<int X_DIM, int Y_DIM, int _TDM_CHUNKS, class T>
// void
// vmultc(
// 	x_complex<T> &out_a,
// 	x_complex<T> in_a[X_DIM][X_DIM][_TDM_CHUNKS],
// 	x_complex<T> in_b[X_DIM][X_DIM][_TDM_CHUNKS],
// 	int i, int j, int l, bool a_t, bool b_t)
// {
// 	x_complex<T> tmp, aval, bval;
// 	tmp = 0.0;
// 	for(int k=0; k<l; k++) {
// 		aval = (a_t) ? x_conj(in_a[k][i][0]) : in_a[i][k][0];
// 		bval = (b_t) ? x_conj(in_b[j][k][0]) : in_b[k][j][0];
// 		tmp += aval*bval;
// 	}
// 	out_a = tmp;
// }

template <int X_DIM, int Y_DIM, int _TDM_CHUNKS>
void jacobi_mult_c(float u_re[X_DIM][X_DIM][_TDM_CHUNKS],
                   float u_im[X_DIM][X_DIM][_TDM_CHUNKS],
                   float s_re[X_DIM][X_DIM][_TDM_CHUNKS],
                   float s_im[X_DIM][X_DIM][_TDM_CHUNKS],
                   float v_re[X_DIM][X_DIM][_TDM_CHUNKS],
                   float v_im[X_DIM][X_DIM][_TDM_CHUNKS],
                   float A_re[X_DIM][X_DIM][_TDM_CHUNKS],
                   float A_im[X_DIM][X_DIM][_TDM_CHUNKS],
                   bool v_trans) // TODO printf out A_re
{
    int maxd = (X_DIM > Y_DIM) ? X_DIM : Y_DIM;
    float tmp_re[X_DIM][Y_DIM][_TDM_CHUNKS];
    float tmp_im[X_DIM][Y_DIM][_TDM_CHUNKS];
    for (int i = 0; i < X_DIM; i++) {
        for (int j = 0; j < Y_DIM; j++) {
            vmultc<X_DIM, Y_DIM, _TDM_CHUNKS>(&tmp_re[i][j][0], &tmp_im[i][j][0], u_re, u_im, s_re, s_im, i, j, X_DIM,
                                              false, false);
        }
    }
    for (int i = 0; i < X_DIM; i++) {
        for (int j = 0; j < Y_DIM; j++) {
            vmultc<X_DIM, Y_DIM, _TDM_CHUNKS>(&A_re[i][j][0], &A_im[i][j][0], tmp_re, tmp_im, v_re, v_im, i, j, X_DIM,
                                              false, v_trans);
        }
    }
}

// template<int X_DIM, int Y_DIM, int _TDM_CHUNKS, class T>
// void
// jacobi_mult_c(
// 	x_complex<T> in_u[X_DIM][X_DIM][_TDM_CHUNKS],
// 	x_complex<T> in_s[X_DIM][X_DIM][_TDM_CHUNKS],
// 	x_complex<T> in_v[X_DIM][X_DIM][_TDM_CHUNKS],
// 	x_complex<T> out_A[X_DIM][X_DIM][_TDM_CHUNKS],
// 	bool v_trans) // TODO printf out A_re
// {
// 	int maxd = (X_DIM > Y_DIM) ? X_DIM : Y_DIM;
// 	x_complex<T> tmp[X_DIM][Y_DIM][_TDM_CHUNKS];
// 	for(int i=0; i<X_DIM; i++) {
// 		for(int j=0; j<Y_DIM; j++) {
// 			vmultc<X_DIM,Y_DIM,_TDM_CHUNKS>(tmp[i][j][0], in_u, in_s, i, j, X_DIM, false, false);
// 		}
// 	}
// 	for(int i=0; i<X_DIM; i++) {
// 		for(int j=0; j<Y_DIM; j++) {
// 			vmultc<X_DIM,Y_DIM,_TDM_CHUNKS>(out_A[i][j][0], tmp, in_v, i, j, X_DIM, false, v_trans);
// 		}
// 	}
// }

// float vmult(float a[][N], float b[][N], int i, int j, int l, bool a_t, bool b_t);

template <int DIM>
void mm_comp(float a_re[DIM][DIM],
             float a_im[DIM][DIM],
             float b_re[DIM][DIM],
             float b_im[DIM][DIM],
             float c_re[DIM][DIM],
             float c_im[DIM][DIM]) {
    for (int i = 0; i < DIM; i++) {
        for (int j = 0; j < DIM; j++) {
            c_re[i][j] = 0;
            c_im[i][j] = 0;
            for (int k = 0; k < DIM; k++) {
                c_re[i][j] += (a_re[i][k] * b_re[k][j] - a_im[i][k] * b_im[k][j]);
                c_im[i][j] += (a_re[i][k] * b_im[k][j] + a_im[i][k] * b_re[k][j]);
            }
            printf("[%e %e] ", c_re[i][j], c_im[i][j]);
        }
    }
}

template <int DIM>
void mm_transp_comp(float a_re[DIM][DIM], float a_im[DIM][DIM], float c_re[DIM][DIM], float c_im[DIM][DIM]) {
    float b_re[DIM][DIM];
    float b_im[DIM][DIM];
    for (int i = 0; i < DIM; i++) {
        for (int j = 0; j < DIM; j++) {
            b_re[i][j] = a_re[j][i];
            b_im[i][j] = -a_im[j][i];
        }
    }
    mm_comp<DIM>(a_re, a_im, b_re, b_im, c_re, c_im);
}

template <int DIM>
void transp(float a[DIM][DIM]) {
    float tmp[DIM][DIM];

    for (int i = 0; i < DIM; i++) {
        for (int j = 0; j < DIM; j++) {
            tmp[j][i] = a[i][j];
        }
    }
    for (int i = 0; i < DIM; i++) {
        for (int j = 0; j < DIM; j++) {
            a[i][j] = tmp[i][j];
        }
    }
}

template <int DIM, int _TDM_CHUNKS, class T>
float vmult(T a[DIM][DIM][_TDM_CHUNKS], T b[DIM][DIM][_TDM_CHUNKS], int i, int j, int l, bool a_t, bool b_t) {
    T tmp, aval, bval;
    tmp = 0;
    for (int k = 0; k < l; k++) {
        aval = (a_t) ? a[k][i][0] : a[i][k][0];
        bval = (b_t) ? b[j][k][0] : b[k][j][0];
        tmp += aval * bval;
    }
    return tmp;
}

// TODO Placeholder - shouldn't need this after SVD is cleaned up
template <int DIM, int _TDM_CHUNKS>
void jacobi_mult(float u_re[DIM][DIM][_TDM_CHUNKS],
                 float s_re[DIM][DIM][_TDM_CHUNKS],
                 float v_re[DIM][DIM][_TDM_CHUNKS],
                 float A_re[DIM][DIM][_TDM_CHUNKS],
                 bool v_trans) // TODO printf out A_re
{
    int maxd = DIM; //(DIM > DIM) ? DIM : DIM;
    float tmp[DIM][DIM][_TDM_CHUNKS];
    for (int i = 0; i < DIM; i++) {
        for (int j = 0; j < DIM; j++) {
            tmp[i][j][0] = vmult<DIM, _TDM_CHUNKS>(u_re, s_re, i, j, DIM, false, false);
        }
    }
    for (int i = 0; i < DIM; i++) {
        for (int j = 0; j < DIM; j++) {
            A_re[i][j][0] = vmult<DIM, _TDM_CHUNKS>(tmp, v_re, i, j, DIM, false, v_trans);
        }
    }
}

// void clear_diagm(float s_re[N][N]);

/*
* v_re      input matrix
* v_out_re  mult operation result
*/
template <int DIM>
void multbytrans(float m_re[DIM][DIM],
                 float m_out_re[DIM][DIM],
                 bool trans2) // transpose mult second (m*m^t)
{
    for (int i = 0; i < DIM; i++) {
        for (int j = 0; j < DIM; j++) {
            m_out_re[i][j] = vmult<DIM, DIM>(m_re, m_re, i, j, DIM, !trans2, trans2);
        }
    }
}

/*
* v_re      input matrix (real)
* v_im      input matrix (imaginary)
* v_out_re  mult operation result (real)
* v_out_im  mult operation result (imaginary)
*/
template <int DIM, int _TDM_CHUNKS>
void multbytransc(float m_re[DIM][DIM][_TDM_CHUNKS],
                  float m_im[DIM][DIM][_TDM_CHUNKS],
                  float m_out_re[DIM][DIM][_TDM_CHUNKS],
                  float m_out_im[DIM][DIM][_TDM_CHUNKS],
                  bool trans2) // transpose mult second (m*m^t)
{
    for (int i = 0; i < DIM; i++) {
        for (int j = 0; j < DIM; j++) {
            vmultc<DIM, DIM, _TDM_CHUNKS>(&m_out_re[i][j][0], &m_out_im[i][j][0], m_re, m_im, m_re, m_im, i, j, DIM,
                                          !trans2, trans2);
        }
    }
}

// template<int DIM, int _TDM_CHUNKS, class T>
// void
// multbytransc(
// 	x_complex<T> in_a[DIM][DIM][_TDM_CHUNKS],
// 	x_complex<T> out_a[DIM][DIM][_TDM_CHUNKS],
// 	bool trans2) // transpose mult second (m*m^t)
// {
// 	for(int i=0; i<DIM; i++) {
// 		for(int j=0; j<DIM; j++) {
// 			vmultc<DIM,DIM,_TDM_CHUNKS>(out_a[i][j][0], in_a, in_a, i, j, DIM, !trans2, trans2);
// 		}
// 	}
// }

static float conv_int_to_float(int a, int fixed_width, int int_width) {
    float tmp = a / (float)pow((double)2.0, (double)(fixed_width - int_width));
    printf("int(dec): %d, int(hex): %x, float: %f\n", a, a, tmp);
    return tmp;
}

/*
* mask only works with 12 fractional bits
*/
static int conv_float_to_int(float a, int fixed_width, int int_width) {
    int tmp = (int)(a * (float)pow((double)2, (double)(fixed_width - int_width)));
    ;
    printf("int(dec): %d, int(hex): %x, float: %f\n", tmp, tmp, a);
    return tmp;
}

template <int X_DIM, int Y_DIM, int _TDM_CHUNKS, class T>
void check_QRD_results(T x[X_DIM][2 * Y_DIM][_TDM_CHUNKS]) {
    printf("QRD done.\n");

    printf("\nMatrix after QRD\n");
    printf("------------------------\n");
    for (int ti = 0; ti < X_DIM; ti++) {
        for (int tj = 0; tj < 2 * Y_DIM; tj++) {
            float ptmp = x[ti][tj][0];
            printf("%f ", ptmp);
        }
        printf("\n");
    }
    printf("\n\n");

    for (int ti = 0; ti < X_DIM; ti++) {
        for (int tj = 0; tj < ti; tj++) {
            printf("0.000000e+00 ");
        }
        for (int tj = 0; tj < Y_DIM - ti; tj++) {
            float ptmp = x[ti][tj][0];
            printf("%e ", ptmp);
        }
        printf("\n");
    }
    printf("\n\n");

    for (int ti = 0; ti < X_DIM; ti++) {
        for (int tj = 0; tj < Y_DIM; tj++) {
            float ptmp = x[ti][Y_DIM - ti + tj][0];
            printf("%e ", ptmp);
        }
        printf("\n");
    }
}

template <int X_DIM, int Y_DIM, int _TDM_CHUNKS, class T>
void check_QRD_results(T x_re[X_DIM][2 * Y_DIM][_TDM_CHUNKS], T x_im[X_DIM][2 * Y_DIM][_TDM_CHUNKS]) {
    printf("QRD done.\n");

    printf("\nMatrix (X) after QRD\n");
    printf("-------------------------n");
    printf("\nMatrix (real)\n");
    printf("----------------\n");
    for (int ti = 0; ti < X_DIM; ti++) {
        for (int tj = 0; tj < ti; tj++) {
            printf("0.000000 ");
        }
        for (int tj = 0; tj < Y_DIM - ti; tj++) {
            float ptmp = x_re[ti][tj][0];
            printf("%f ", ptmp);
        }
        printf("     ");

        for (int tj = 0; tj < Y_DIM; tj++) {
            float ptmp = x_re[ti][Y_DIM - ti + tj][0];
            printf("%f ", ptmp);
        }
        printf("\n");
    }

    printf("\nMatrix (imag)\n");
    printf("----------------\n");
    for (int ti = 0; ti < X_DIM; ti++) {
        for (int tj = 0; tj < ti; tj++) {
            printf("0.000000 ");
        }
        for (int tj = 0; tj < Y_DIM - ti; tj++) {
            float ptmp = x_im[ti][tj][0];
            printf("%f ", ptmp);
        }
        printf("     ");

        for (int tj = 0; tj < Y_DIM; tj++) {
            float ptmp = x_im[ti][Y_DIM - ti + tj][0];
            printf("%f ", ptmp);
        }
        printf("\n");
    }

    /*
    * TODO
    * Multiply Q*Q^t to see if it produces the Identity matrix
    * Multiply Q*R to see if produces A again
    */
    printf("Check Q*Q^t\n");
    printf("Store Q, R\n");
    float tmp_mat_re[X_DIM][Y_DIM];
    float tmp_mat_im[X_DIM][Y_DIM];
    float tmp_q_re[X_DIM][Y_DIM];
    float tmp_q_im[X_DIM][Y_DIM];
    float tmp_r_re[X_DIM][Y_DIM];
    float tmp_r_im[X_DIM][Y_DIM];
    for (int ti = 0; ti < X_DIM; ti++) {
        for (int tj = 0; tj < Y_DIM; tj++) {
            printf("ti: %d, tj: %d\n", ti, tj);
            tmp_mat_re[ti][tj] = x_re[ti][tj + Y_DIM - ti][0];
            printf("X_re[%d][%d][0] = %e\n", ti, tj + Y_DIM - ti, tmp_mat_re[ti][tj]);
            tmp_mat_im[ti][tj] = x_im[ti][tj + Y_DIM - ti][0];
            printf("X_im[%d][%d][0] = %e\n", ti, tj + Y_DIM - ti, tmp_mat_im[ti][tj]);

            tmp_q_re[tj][ti] = x_re[ti][tj + Y_DIM - ti][0];
            printf("X_re[%d][%d][0] = %e\n", ti, tj + Y_DIM - ti, tmp_q_re[tj][ti]);
            tmp_q_im[tj][ti] = -x_im[ti][tj + Y_DIM - ti][0];
            printf("X_im[%d][%d][0] = %e\n", ti, tj + Y_DIM - ti, tmp_q_im[tj][ti]);

            if (tj < ti) {
                tmp_r_re[ti][tj] = 0;
                tmp_r_im[ti][tj] = 0;
            } else {
                tmp_r_re[ti][tj] = x_re[ti][tj - ti][0];
                tmp_r_im[ti][tj] = x_im[ti][tj - ti][0];
            }
        }
    }

    /*
            tmp_mat_re[0][0] = -0.090691;
            tmp_mat_re[0][1] =  0.417296;
            tmp_mat_re[0][2] = -0.306837;
            tmp_mat_re[0][3] = -0.520790;
            tmp_mat_re[1][0] = -0.475003;
            tmp_mat_re[1][1] = -0.237293;
            tmp_mat_re[1][2] = -0.169015;
            tmp_mat_re[1][3] =  0.008679;
            tmp_mat_re[2][0] =  0.479512;
            tmp_mat_re[2][1] = -0.332793;
            tmp_mat_re[2][2] =  0.030529;
            tmp_mat_re[2][3] =  0.079810;
            tmp_mat_re[3][0] = -0.297628;
            tmp_mat_re[3][1] = -0.468605;
            tmp_mat_re[3][2] = -0.174566;
            tmp_mat_re[3][3] = -0.608285;

            tmp_mat_im[0][0] = -0.161341;
            tmp_mat_im[0][1] =  0.456402;
            tmp_mat_im[0][2] = -0.373242;
            tmp_mat_im[0][3] =  0.280402;
            tmp_mat_im[1][0] =  0.496548;
            tmp_mat_im[1][1] = -0.283132;
            tmp_mat_im[1][2] = -0.601775;
            tmp_mat_im[1][3] = -0.023788;
            tmp_mat_im[2][0] = -0.352745;
            tmp_mat_im[2][1] = -0.281551;
            tmp_mat_im[2][2] = -0.433460;
            tmp_mat_im[2][3] =  0.510322;
            tmp_mat_im[3][0] = -0.224975;
            tmp_mat_im[3][1] = -0.267352;
            tmp_mat_im[3][2] =  0.395663;
            tmp_mat_im[3][3] =  0.112706;
    */

    printf("\nQ Matrix (real)\n");
    printf("------------------------\n");
    for (int ti = 0; ti < X_DIM; ti++) {
        for (int tj = 0; tj < Y_DIM; tj++) {
            float ptmp = tmp_mat_re[ti][tj];
            printf("%f ", ptmp);
        }
        printf("\n");
    }

    printf("\nQ Matrix (imag)\n");
    printf("------------------------\n");
    for (int ti = 0; ti < X_DIM; ti++) {
        for (int tj = 0; tj < Y_DIM; tj++) {
            float ptmp = tmp_mat_im[ti][tj];
            printf("%f ", ptmp);
        }
        printf("\n");
    }
    printf("\n\n");

    printf("\nR Matrix (real)\n");
    printf("------------------------\n");
    for (int ti = 0; ti < X_DIM; ti++) {
        for (int tj = 0; tj < Y_DIM; tj++) {
            float ptmp = tmp_r_re[ti][tj];
            printf("%f ", ptmp);
        }
        printf("\n");
    }

    printf("\nR Matrix (imag)\n");
    printf("------------------------\n");
    for (int ti = 0; ti < X_DIM; ti++) {
        for (int tj = 0; tj < Y_DIM; tj++) {
            float ptmp = tmp_r_im[ti][tj];
            printf("%f ", ptmp);
        }
        printf("\n");
    }
    printf("\n\n");

    float tmp_mat_res_re[X_DIM][Y_DIM];
    float tmp_mat_res_im[X_DIM][Y_DIM];

    mm_comp<X_DIM>(tmp_mat_re, tmp_mat_im, tmp_r_re, tmp_r_im, tmp_mat_res_re, tmp_mat_res_im);

    printf("\nQ*R Matrix (real)\n");
    printf("------------------------\n");
    for (int ti = 0; ti < X_DIM; ti++) {
        for (int tj = 0; tj < Y_DIM; tj++) {
            float ptmp = tmp_mat_res_re[ti][tj];
            printf("%f ", ptmp);
        }
        printf("\n");
    }

    printf("\nQ*R Matrix (imag)\n");
    printf("------------------------\n");
    for (int ti = 0; ti < X_DIM; ti++) {
        for (int tj = 0; tj < Y_DIM; tj++) {
            float ptmp = tmp_mat_res_im[ti][tj];
            printf("%f ", ptmp);
        }
        printf("\n");
    }
    printf("\n\n");

    mm_transp_comp<X_DIM>(tmp_mat_re, tmp_mat_im, tmp_mat_res_re, tmp_mat_res_im);

    printf("\nQ*Q^t Matrix (real)\n");
    printf("------------------------\n");
    for (int ti = 0; ti < X_DIM; ti++) {
        for (int tj = 0; tj < Y_DIM; tj++) {
            float ptmp = tmp_mat_res_re[ti][tj];
            printf("%f ", ptmp);
        }
        printf("\n");
    }

    printf("\nQ*Q^t Matrix (imag)\n");
    printf("------------------------\n");
    for (int ti = 0; ti < X_DIM; ti++) {
        for (int tj = 0; tj < Y_DIM; tj++) {
            float ptmp = tmp_mat_res_im[ti][tj];
            printf("%f ", ptmp);
        }
        printf("\n");
    }
    printf("\n\n");
}

template <int X_DIM, int Y_DIM, int _TDM_CHUNKS, class T>
void check_inverse_results(T x_inv[X_DIM][Y_DIM][_TDM_CHUNKS]) {
    printf("Backsub done.\n");

    printf("\nMatrix (real) after Backsub\n");
    printf("------------------------\n");
    dumpm<X_DIM, Y_DIM, _TDM_CHUNKS, 0>(x_inv);
}

template <int X_DIM, int Y_DIM, int _TDM_CHUNKS, class T>
void check_inverse_results(T in_mat_re[X_DIM][Y_DIM][_TDM_CHUNKS],
                           T in_mat_im[X_DIM][Y_DIM][_TDM_CHUNKS],
                           T x_inv_re[X_DIM][Y_DIM][_TDM_CHUNKS],
                           T x_inv_im[X_DIM][Y_DIM][_TDM_CHUNKS]) {
    float tmp_mat_res_re[X_DIM][Y_DIM];
    float tmp_mat_res_im[X_DIM][Y_DIM];

    printf("Backsub done.\n");

    printf("\nMatrix (real) after Backsub\n");
    printf("------------------------\n");
    for (int ti = 0; ti < X_DIM; ti++) {
        for (int tj = 0; tj < Y_DIM; tj++) {
            float ptmp = x_inv_re[ti][tj][0];
            printf("%f ", ptmp);
        }
        printf("\n");
    }

    printf("\nMatrix (imaginary) after Backsub\n");
    printf("------------------------\n");
    for (int ti = 0; ti < X_DIM; ti++) {
        for (int tj = 0; tj < Y_DIM; tj++) {
            float ptmp = x_inv_im[ti][tj][0];
            printf("%f ", ptmp);
        }
        printf("\n");
    }
    printf("\n");

    printf("Test accuracy of inverse\n");
    float in_mat_re_float[X_DIM][Y_DIM];
    float in_mat_im_float[X_DIM][Y_DIM];
    float x_inv_re_float[X_DIM][Y_DIM];
    float x_inv_im_float[X_DIM][Y_DIM];
    for (int ti = 0; ti < X_DIM; ti++) {
        for (int tj = 0; tj < Y_DIM; tj++) {
            in_mat_re_float[ti][tj] = in_mat_re[ti][tj][0];
            in_mat_im_float[ti][tj] = in_mat_im[ti][tj][0];
            x_inv_re_float[ti][tj] = x_inv_re[ti][tj][0];
            x_inv_im_float[ti][tj] = x_inv_im[ti][tj][0];
        }
    }

    mm_comp<X_DIM>(in_mat_re_float, in_mat_im_float, x_inv_re_float, x_inv_im_float, tmp_mat_res_re, tmp_mat_res_im);

    printf("\nA*A^-1 Matrix (real)\n");
    printf("------------------------\n");
    for (int ti = 0; ti < X_DIM; ti++) {
        for (int tj = 0; tj < Y_DIM; tj++) {
            float ptmp = tmp_mat_res_re[ti][tj];
            printf("%f ", ptmp);
        }
        printf("\n");
    }

    printf("\nA*A^-1 Matrix (imag)\n");
    printf("------------------------\n");
    for (int ti = 0; ti < X_DIM; ti++) {
        for (int tj = 0; tj < Y_DIM; tj++) {
            float ptmp = tmp_mat_res_im[ti][tj];
            printf("%f ", ptmp);
        }
        printf("\n");
    }
    printf("\n\n");
}

template <int WA, int WMA>
static void conv_data_formats(int data_in, ap_fixed<WA, WMA, AP_TRN, AP_WRAP>& data_out) {
    data_out = conv_int_to_float(data_in, WA, WMA);
}

template <int WA, int WMA>
static void conv_data_formats(ap_fixed<WA, WMA, AP_TRN, AP_WRAP> data_in, int& data_out) {
    data_out = conv_float_to_int(data_in, WA, WMA);
}

template <typename T1, typename T2>
static void conv_data_formats(T1 data_in, T2& data_out) {
    data_out = data_in;
}

template <int X_DIM, int Y_DIM, int _TDM_CHUNKS, class T1, class T2>
void load_input_matrix(int chunk_num, T1* data_in, T2 in_re[X_DIM][Y_DIM][_TDM_CHUNKS]) {
    for (int i = 0; i < X_DIM; i++) {
        for (int j = 0; j < Y_DIM; j++) {
            for (int k = 0; k < _TDM_CHUNKS; k++) {
                conv_data_formats(
                    data_in[chunk_num * _TDM_CHUNKS * X_DIM * Y_DIM + j * _TDM_CHUNKS * X_DIM + i * _TDM_CHUNKS + k],
                    in_re[i][j][k]);
                float tmp = in_re[i][j][k];
                printf("in[%d][%d][%d] = %e\n", i, j, k, tmp);
            }
        }
    }
}

union ufloat {
    float f;
    unsigned u;
};

/*
* Automatically stores data to a float array as well (even if the first array
* is already a float). This is to support ap_fixed (or other) data types to ensure
* a float version exists for comparison.
*/
template <int X_DIM, int Y_DIM, int _TDM_CHUNKS, class T1, class T2>
void store_output_matrix(int chunk_num, T1 out_re[X_DIM][Y_DIM][_TDM_CHUNKS], T2* data_out, float* data_out_float) {
    for (int i = 0; i < X_DIM; i++) {
        for (int j = 0; j < Y_DIM; j++) {
            for (int k = 0; k < _TDM_CHUNKS; k++) {
                conv_data_formats(out_re[i][j][k], *(data_out + chunk_num * _TDM_CHUNKS * X_DIM * Y_DIM +
                                                     j * _TDM_CHUNKS * X_DIM + i * _TDM_CHUNKS + k));
                *(data_out_float + chunk_num * _TDM_CHUNKS * X_DIM * Y_DIM + j * _TDM_CHUNKS * X_DIM + i * _TDM_CHUNKS +
                  k) = out_re[i][j][k]; // Relies on overloaded = operation to convert format
                // TODO temp

                ufloat tmp;
                tmp.f = (float)(out_re[i][j][k]);
                int sign = (tmp.u >> 31) & 0x1;
                int exponent = ((tmp.u >> 23) & 0xff) - 127;
                unsigned int mantissa = tmp.u & 0x7fffff;
                float mantissaf = 1 + ((float)mantissa / 8388608);
                float num = (exponent < 0) ? mantissaf / pow((double)2, (double)(0 - exponent))
                                           : mantissaf * pow((double)2, (double)exponent);
                num = (sign > 0) ? -num : num;
                printf("out_re[%d][%d][%d] = %e (%x:%d,%d,%x/%f; %f) \n", i, j, k, tmp.f, tmp.u, sign, exponent,
                       mantissa, mantissaf, num);
            }
        }
    }
}

/*
* real B = f(real A)
*
* T2: default data type (float, ap_fixed)
* T1: storage data type of input file (float -> float, ap_fixed -> int)
*/
template <int X_DIM, int Y_DIM, int _TDM_CHUNKS, int NUM_CHUNKS, int SEL, class T1, class T2in, class T2out>
int test_func_1_1(int argc,
                  char* argv[],
                  float base,
                  float err,
                  void (*func)(T2in[X_DIM][Y_DIM][_TDM_CHUNKS], T2out[X_DIM][Y_DIM][_TDM_CHUNKS])) {
    // File names
    char* input_data_file = argv[1];    //"ch_matrices_re.txt";
    char* output_data_file = argv[2];   //"results_re.txt";
    char* expected_data_file = argv[3]; //"simulation_data_re.txt";

    // Matrix dimension
    short matrix_dimension[2] = {X_DIM, Y_DIM};
    int m = 2 * matrix_dimension[0];
    int n = 2 * matrix_dimension[1];

    // Data size
    int input_data_size = X_DIM * Y_DIM * NUM_CHUNKS * _TDM_CHUNKS;
    int output_data_size = X_DIM * Y_DIM * NUM_CHUNKS * _TDM_CHUNKS;

    // File data pointers
    T1* data_in;
    T1* data_out;
    float* data_out_float;
    data_in = (T1*)malloc(input_data_size * sizeof(T1));
    data_out = (T1*)malloc(output_data_size * sizeof(T1));
    data_out_float = (float*)malloc(output_data_size * sizeof(float));

    // Function arguments - data matrices
    T2in chunk_in_re[X_DIM][Y_DIM][_TDM_CHUNKS];
    T2out chunk_out_re[X_DIM][Y_DIM][_TDM_CHUNKS];

    printf("Test beginning...\n");
    /*
    ******************************************************************************
    * Load data, perfrom function, and store result
    ******************************************************************************
    */
    printf("Loading values from txt files ...\n");
    load_from_file(input_data_file, data_in, input_data_size);

    for (int i = 0; i < NUM_CHUNKS; i++) {
        printf("Loading input data array ...\n");
        load_input_matrix<X_DIM, Y_DIM, _TDM_CHUNKS>(i, data_in, chunk_in_re);

        printf("Perform main function!!\n");
        (*func)(chunk_in_re, chunk_out_re);

        printf("Store output result ... \n");
        // store_output_matrix<X_DIM,Y_DIM,_TDM_CHUNKS,WA,WMA>(i, chunk_out_re, data_out, data_out_float);
        store_output_matrix<X_DIM, Y_DIM, _TDM_CHUNKS>(i, chunk_out_re, data_out, data_out_float);
    }

    printf("Saving results to output file (%s) ...\n", output_data_file);
    save_to_file(output_data_file, data_out, X_DIM * Y_DIM * NUM_CHUNKS * _TDM_CHUNKS);

    int out_name_len = strlen(output_data_file);
    char output_data_file_float[out_name_len + 7];
    strcpy(output_data_file_float, output_data_file);
    output_data_file_float[out_name_len - 4] = '\0';
    const char* float_end = "_float.txt";
    strcat(output_data_file_float, float_end);
    printf("Saving results to output file (%s) ...\n", output_data_file_float);
    save_to_file(output_data_file_float, data_out_float, X_DIM * Y_DIM * NUM_CHUNKS * _TDM_CHUNKS);

    /*
    ******************************************************************************
    * Compare C code results to expected Matlab reference script results
    ******************************************************************************
    */
    printf("Comparing results (%s,%s)...\n", output_data_file, expected_data_file);
    int cmp_res_i;
    T2out dummy;
    cmp_res_i = compare_files(dummy, expected_data_file, output_data_file, 1, output_data_size, base, err);

    if (cmp_res_i != 0) {
        printf("Data verification completed successfully.\n");
    } else {
        printf("Data verification completed with errors.\n");
    }

    free(data_in);
    free(data_out);
    free(data_out_float);

    return 0;
}

/*
* real C = f(real A, real B)
*
* T2: default data type (float, ap_fixed)
* T1: storage data type of input file (float -> float, ap_fixed -> int)
*/
template <int X_DIM, int Y_DIM, int _TDM_CHUNKS, int NUM_CHUNKS, int SEL, class T1, class T2in, class T2out>
int test_func_2_1(int argc,
                  char* argv[],
                  float base,
                  float err,
                  void (*func)(T2in[X_DIM][Y_DIM][_TDM_CHUNKS],
                               T2in[X_DIM][Y_DIM][_TDM_CHUNKS],
                               T2out[X_DIM][Y_DIM][_TDM_CHUNKS])) {
    // File names
    char* input_a_data_file = argv[1];  //"ch_matrices_re.txt";
    char* input_b_data_file = argv[2];  //"ch_matrices_re.txt";
    char* output_data_file = argv[3];   //"results_re.txt";
    char* expected_data_file = argv[4]; //"simulation_data_re.txt";

    // Matrix dimension
    short matrix_dimension[2] = {X_DIM, Y_DIM};
    int m = 2 * matrix_dimension[0];
    int n = 2 * matrix_dimension[1];

    // Data size
    int input_data_size = X_DIM * Y_DIM * NUM_CHUNKS * _TDM_CHUNKS;
    int output_data_size = X_DIM * Y_DIM * NUM_CHUNKS * _TDM_CHUNKS;

    // File data pointers
    T1 *data_a_in, *data_b_in;
    T1* data_out;
    float* data_out_float;
    data_a_in = (T1*)malloc(input_data_size * sizeof(T1));
    data_b_in = (T1*)malloc(input_data_size * sizeof(T1));
    data_out = (T1*)malloc(output_data_size * sizeof(T1));
    data_out_float = (float*)malloc(output_data_size * sizeof(float));

    // Function arguments - data matrices
    T2in chunk_a_in_re[X_DIM][Y_DIM][_TDM_CHUNKS];
    T2in chunk_b_in_re[X_DIM][Y_DIM][_TDM_CHUNKS];
    T2out chunk_out_re[X_DIM][Y_DIM][_TDM_CHUNKS];

    printf("Test beginning...\n");
    /*
    ******************************************************************************
    * Load data, perfrom function, and store result
    ******************************************************************************
    */
    printf("Loading values from txt files (%s, %s)...\n", input_a_data_file, input_b_data_file);
    load_from_file(input_a_data_file, data_a_in, input_data_size);
    load_from_file(input_b_data_file, data_b_in, input_data_size);

    for (int i = 0; i < NUM_CHUNKS; i++) {
        printf("Loading input data array ...\n");
        load_input_matrix<X_DIM, Y_DIM, _TDM_CHUNKS>(i, data_a_in, chunk_a_in_re);
        load_input_matrix<X_DIM, Y_DIM, _TDM_CHUNKS>(i, data_b_in, chunk_b_in_re);

        printf("Perform main function!!\n");
        (*func)(chunk_a_in_re, chunk_b_in_re, chunk_out_re);

        printf("Store output result ... \n");
        store_output_matrix<X_DIM, Y_DIM, _TDM_CHUNKS>(i, chunk_out_re, data_out, data_out_float);
    }

    printf("Saving results to output file (%s)...\n", output_data_file);
    save_to_file(output_data_file, data_out, X_DIM * Y_DIM * NUM_CHUNKS * _TDM_CHUNKS);

    int out_name_len = strlen(output_data_file);
    char output_data_file_float[out_name_len + 7];
    strcpy(output_data_file_float, output_data_file);
    output_data_file_float[out_name_len - 4] = '\0';
    const char* float_end = "_float.txt";
    strcat(output_data_file_float, float_end);
    printf("Saving results to output file (%s) ...\n", output_data_file_float);
    save_to_file(output_data_file_float, data_out_float, X_DIM * Y_DIM * NUM_CHUNKS * _TDM_CHUNKS);

    /*
    ******************************************************************************
    * Compare C code results to expected Matlab reference script results
    ******************************************************************************
    */
    printf("Comparing results (%s, %s)...\n", output_data_file, expected_data_file);
    int cmp_res_i;
    T2out dummy;
    cmp_res_i = compare_files(dummy, expected_data_file, output_data_file, 1, output_data_size, base, err);

    if (cmp_res_i != 0) {
        printf("Data verification completed successfully.\n");
    } else {
        printf("Data verification completed with errors.\n");
    }

    free(data_out);
    free(data_out_float);
    free(data_a_in);
    free(data_b_in);

    return 0;
}

/*
* (real B, real C) = f(real A)
*
* T2: default data type (float, ap_fixed)
* T1: storage data type of input file (float -> float, ap_fixed -> int)
*/
template <int X_DIM, int Y_DIM, int _TDM_CHUNKS, int NUM_CHUNKS, int SEL, class T1, class T2in, class T2out>
int test_func_1_2(int argc,
                  char* argv[],
                  float base,
                  float err,
                  void (*func)(T2in[X_DIM][Y_DIM][_TDM_CHUNKS],
                               T2out[X_DIM][Y_DIM][_TDM_CHUNKS],
                               T2out[X_DIM][Y_DIM][_TDM_CHUNKS])) {
    // File names
    char* input_data_file = argv[1];      //"ch_matrices_re.txt";
    char* output_a_data_file = argv[2];   //"results_a_re.txt";
    char* output_b_data_file = argv[3];   //"results_b_re.txt";
    char* expected_a_data_file = argv[4]; //"simulation_a_data_re.txt";
    char* expected_b_data_file = argv[5]; //"simulation_b_data_re.txt";

    // Matrix dimension
    short matrix_dimension[2] = {X_DIM, Y_DIM};
    int m = 2 * matrix_dimension[0];
    int n = 2 * matrix_dimension[1];

    // Data size
    int input_data_size = X_DIM * Y_DIM * NUM_CHUNKS * _TDM_CHUNKS;
    int output_data_size = X_DIM * Y_DIM * NUM_CHUNKS * _TDM_CHUNKS;

    // File data pointers
    T1* data_in;
    T1 *data_a_out, *data_b_out;
    float *data_a_out_float, *data_b_out_float;
    data_in = (T1*)malloc(input_data_size * sizeof(T1));
    data_a_out = (T1*)malloc(output_data_size * sizeof(T1));
    data_b_out = (T1*)malloc(output_data_size * sizeof(T1));
    data_a_out_float = (float*)malloc(output_data_size * sizeof(float));
    data_b_out_float = (float*)malloc(output_data_size * sizeof(float));

    // Function arguments - data matrices
    T2in chunk_in_re[X_DIM][Y_DIM][_TDM_CHUNKS];
    T2out chunk_a_out_re[X_DIM][Y_DIM][_TDM_CHUNKS];
    T2out chunk_b_out_re[X_DIM][Y_DIM][_TDM_CHUNKS];

    printf("Test beginning...\n");
    /*
    ******************************************************************************
    * Load data, perfrom function, and store result
    ******************************************************************************
    */
    printf("Loading values from txt files (%s)...\n", input_data_file);
    load_from_file(input_data_file, data_in, input_data_size);

    for (int i = 0; i < NUM_CHUNKS; i++) {
        printf("Loading input data array ...\n");
        load_input_matrix<X_DIM, Y_DIM, _TDM_CHUNKS>(i, data_in, chunk_in_re);

        printf("Perform main function!!\n");
        (*func)(chunk_in_re, chunk_a_out_re, chunk_b_out_re);

        printf("Store output result ... \n");
        store_output_matrix<X_DIM, Y_DIM, _TDM_CHUNKS>(i, chunk_a_out_re, data_a_out, data_a_out_float);
        store_output_matrix<X_DIM, Y_DIM, _TDM_CHUNKS>(i, chunk_b_out_re, data_b_out, data_b_out_float);
    }

    printf("Saving results to output file (%s, %s)...\n", output_a_data_file, output_b_data_file);
    save_to_file(output_a_data_file, data_a_out, X_DIM * Y_DIM * NUM_CHUNKS * _TDM_CHUNKS);
    save_to_file(output_b_data_file, data_b_out, X_DIM * Y_DIM * NUM_CHUNKS * _TDM_CHUNKS);

    int filename_len;
    const char* float_end = "_float.txt";

    filename_len = strlen(output_a_data_file);
    char* output_a_data_file_float = (char*)malloc((filename_len + 7) * sizeof(char));
    strcpy(output_a_data_file_float, output_a_data_file);
    output_a_data_file_float[filename_len - 4] = '\0';
    strcat(output_a_data_file_float, float_end);

    filename_len = strlen(output_b_data_file);
    char* output_b_data_file_float = (char*)malloc((filename_len + 7) * sizeof(char));
    strcpy(output_b_data_file_float, output_b_data_file);
    output_b_data_file_float[filename_len - 4] = '\0';
    strcat(output_b_data_file_float, float_end);

    printf("Saving results to output file (%s, %s) ...\n", output_a_data_file_float, output_b_data_file_float);
    save_to_file(output_a_data_file_float, data_a_out_float, X_DIM * Y_DIM * NUM_CHUNKS * _TDM_CHUNKS);
    save_to_file(output_b_data_file_float, data_b_out_float, X_DIM * Y_DIM * NUM_CHUNKS * _TDM_CHUNKS);

    /*
    ******************************************************************************
    * Compare C code results to expected Matlab reference script results
    ******************************************************************************
    */
    printf("Comparing results (%s, %s, %s, %s)...\n", output_a_data_file, expected_a_data_file, output_b_data_file,
           expected_b_data_file);
    int cmp_res_i;
    T2out dummy;
    cmp_res_i = compare_files(dummy, expected_a_data_file, output_a_data_file, 1, output_data_size, base, err);
    cmp_res_i =
        cmp_res_i & compare_files(dummy, expected_b_data_file, output_b_data_file, 1, output_data_size, base, err);

    if (cmp_res_i != 0) {
        printf("Data verification completed successfully.\n");
    } else {
        printf("Data verification completed with errors.\n");
    }

    free(data_a_out);
    free(data_a_out_float);
    free(data_b_out);
    free(data_b_out_float);
    free(data_in);

    return 0;
}

// template<int X_DIM, int Y_DIM, int _TDM_CHUNKS, int NUM_CHUNKS, int SEL,
// 	 class T1, class T2in, class T2out>
// int test_func_1_2_c(
// 	int argc, char* argv[], float base, float err,
// 	void (*func)(x_complex<T2in>[X_DIM][Y_DIM][_TDM_CHUNKS],
// 		x_complex<T2out>[X_DIM][Y_DIM][_TDM_CHUNKS],
// 		x_complex<T2out>[X_DIM][Y_DIM][_TDM_CHUNKS]))
// {
// 	// File names
// 	char *input_data_file_re       = argv[1];//"ch_matrices_re.txt";
// 	char *input_data_file_im       = argv[2];//"ch_matrices_im.txt";
// 	char *output_a_data_file_re    = argv[3];//"results_a_re.txt";
// 	char *output_a_data_file_im    = argv[4];//"results_a_im.txt";
// 	char *output_b_data_file_re    = argv[5];//"results_b_re.txt";
// 	char *output_b_data_file_im    = argv[6];//"results_b_im.txt";
// 	char *expected_a_data_file_re  = argv[7];//"simulation_a_data_re.txt";
// 	char *expected_a_data_file_im  = argv[8];//"simulation_a_data_im.txt";
// 	char *expected_b_data_file_re  = argv[9]; //"simulation_b_data_re.txt";
// 	char *expected_b_data_file_im  = argv[10];//"simulation_b_data_im.txt";

// 	// Matrix dimension
// 	short matrix_dimension[2]  = {X_DIM,Y_DIM};
// 	int m = 2*matrix_dimension[0];
// 	int n = 2*matrix_dimension[1];

// 	// Data size
// 	int input_data_size        = X_DIM*Y_DIM*NUM_CHUNKS*_TDM_CHUNKS;
// 	int output_data_size       = X_DIM*Y_DIM*NUM_CHUNKS*_TDM_CHUNKS;

// 	// File data pointers
// 	T1 *data_in_re, *data_in_im;
// 	T1 *data_a_out_re, *data_a_out_im, *data_b_out_re, *data_b_out_im;
// 	float *data_a_out_float_re, *data_a_out_float_im, *data_b_out_float_re, *data_b_out_float_im;
// 	data_in_re                = (T1*)malloc(input_data_size * sizeof(T1));
// 	data_in_im                = (T1*)malloc(input_data_size * sizeof(T1));
// 	data_a_out_re             = (T1*)malloc(output_data_size * sizeof(T1));
// 	data_a_out_im             = (T1*)malloc(output_data_size * sizeof(T1));
// 	data_b_out_re             = (T1*)malloc(output_data_size * sizeof(T1));
// 	data_b_out_im             = (T1*)malloc(output_data_size * sizeof(T1));
// 	data_a_out_float_re       = (float*)malloc(output_data_size * sizeof(float));
// 	data_a_out_float_im       = (float*)malloc(output_data_size * sizeof(float));
// 	data_b_out_float_re       = (float*)malloc(output_data_size * sizeof(float));
// 	data_b_out_float_im       = (float*)malloc(output_data_size * sizeof(float));

// 	// Function arguments - data matrices
// 	T2in chunk_in_re[X_DIM][Y_DIM][_TDM_CHUNKS];
// 	T2in chunk_in_im[X_DIM][Y_DIM][_TDM_CHUNKS];
// 	T2out chunk_a_out_re[X_DIM][Y_DIM][_TDM_CHUNKS];
// 	T2out chunk_a_out_im[X_DIM][Y_DIM][_TDM_CHUNKS];
// 	T2out chunk_b_out_re[X_DIM][Y_DIM][_TDM_CHUNKS];
// 	T2out chunk_b_out_im[X_DIM][Y_DIM][_TDM_CHUNKS];

// 	printf("Test beginning...\n");
// 	/*
// 	******************************************************************************
// 	* Load data, perfrom function, and store result
// 	******************************************************************************
// 	*/
// 	printf("Loading values from txt files (%s, %s)...\n",input_data_file_re,input_data_file_im);
// 	load_from_file(input_data_file_re, data_in_re, input_data_size);
// 	load_from_file(input_data_file_im, data_in_im, input_data_size);

// 	for(int i = 0; i < NUM_CHUNKS; i++)
// 	{
// 		printf("Loading input data array ...\n");
// 		load_input_matrix<X_DIM,Y_DIM,_TDM_CHUNKS>(i, data_in_re,chunk_in_re);
// 		load_input_matrix<X_DIM,Y_DIM,_TDM_CHUNKS>(i, data_in_im,chunk_in_im);

// 		x_complex<T2in> in_a[X_DIM][Y_DIM][_TDM_CHUNKS];
// 		x_complex<T2out> out_a[X_DIM][Y_DIM][_TDM_CHUNKS];
// 		x_complex<T2out> out_b[X_DIM][Y_DIM][_TDM_CHUNKS];
// 		for(int ii=0; ii<X_DIM; ii++) {
// 			for(int jj=0; jj<Y_DIM; jj++) {
// 				in_a[ii][jj][0].real(chunk_in_re[ii][jj][0]);
// 				in_a[ii][jj][0].imag(chunk_in_im[ii][jj][0]);
// 			}
// 		}

// 		printf("Perform main function!!\n");
// 		(*func)(in_a,out_a,out_b);

// 		for(int ii=0; ii<X_DIM; ii++) {
// 			for(int jj=0; jj<Y_DIM; jj++) {
// 				chunk_a_out_re[ii][jj][0] = out_a[ii][jj][0].real();
// 				chunk_a_out_im[ii][jj][0] = out_a[ii][jj][0].imag();
// 				chunk_b_out_re[ii][jj][0] = out_b[ii][jj][0].real();
// 				chunk_b_out_im[ii][jj][0] = out_b[ii][jj][0].imag();
// 			}
// 		}

// 		printf("Store output result ... \n");
// 		store_output_matrix<X_DIM,Y_DIM,_TDM_CHUNKS>(i, chunk_a_out_re, data_a_out_re, data_a_out_float_re);
// 		store_output_matrix<X_DIM,Y_DIM,_TDM_CHUNKS>(i, chunk_a_out_im, data_a_out_im, data_a_out_float_im);
// 		store_output_matrix<X_DIM,Y_DIM,_TDM_CHUNKS>(i, chunk_b_out_re, data_b_out_re, data_b_out_float_re);
// 		store_output_matrix<X_DIM,Y_DIM,_TDM_CHUNKS>(i, chunk_b_out_im, data_b_out_im, data_b_out_float_im);
// 	}

// 	printf("Saving results to output file (%s, %s, %s, %s)...\n",output_a_data_file_re,output_a_data_file_im,
// output_b_data_file_re,output_b_data_file_im );
// 	save_to_file(output_a_data_file_re, data_a_out_re, X_DIM*Y_DIM*NUM_CHUNKS*_TDM_CHUNKS);
// 	save_to_file(output_a_data_file_im, data_a_out_im, X_DIM*Y_DIM*NUM_CHUNKS*_TDM_CHUNKS);
// 	save_to_file(output_b_data_file_re, data_b_out_re, X_DIM*Y_DIM*NUM_CHUNKS*_TDM_CHUNKS);
// 	save_to_file(output_b_data_file_im, data_b_out_im, X_DIM*Y_DIM*NUM_CHUNKS*_TDM_CHUNKS);

// 	int filename_len;
// 	const char *float_end = "_float.txt";

// 	filename_len  = strlen(output_a_data_file_re);
// 	char *output_a_data_file_float_re = (char*)malloc((filename_len+7)*sizeof(char));
// 	strcpy(output_a_data_file_float_re, output_a_data_file_re);
// 	output_a_data_file_float_re[filename_len-4] = '\0';
// 	strcat(output_a_data_file_float_re, float_end);

// 	filename_len  = strlen(output_a_data_file_im);
// 	char *output_a_data_file_float_im = (char*)malloc((filename_len+7)*sizeof(char));
// 	strcpy(output_a_data_file_float_im, output_a_data_file_im);
// 	output_a_data_file_float_im[filename_len-4] = '\0';
// 	strcat(output_a_data_file_float_im, float_end);

// 	filename_len  = strlen(output_b_data_file_re);
// 	char *output_b_data_file_float_re = (char*)malloc((filename_len+7)*sizeof(char));
// 	strcpy(output_b_data_file_float_re, output_b_data_file_re);
// 	output_b_data_file_float_re[filename_len-4] = '\0';
// 	strcat(output_b_data_file_float_re, float_end);

// 	filename_len  = strlen(output_b_data_file_im);
// 	char *output_b_data_file_float_im = (char*)malloc((filename_len+7)*sizeof(char));
// 	strcpy(output_b_data_file_float_im, output_b_data_file_im);
// 	output_b_data_file_float_im[filename_len-4] = '\0';
// 	strcat(output_b_data_file_float_im, float_end);

// 	printf("Saving results to output file (%s, %s, %s, %s) ...\n", output_a_data_file_float_re,
// output_a_data_file_float_im, output_b_data_file_float_re, output_b_data_file_im );
// 	save_to_file(output_a_data_file_float_re, data_a_out_float_re, X_DIM*Y_DIM*NUM_CHUNKS*_TDM_CHUNKS);
// 	save_to_file(output_a_data_file_float_im, data_a_out_float_im, X_DIM*Y_DIM*NUM_CHUNKS*_TDM_CHUNKS);
// 	save_to_file(output_b_data_file_float_re, data_b_out_float_re, X_DIM*Y_DIM*NUM_CHUNKS*_TDM_CHUNKS);
// 	save_to_file(output_b_data_file_float_im, data_b_out_float_im, X_DIM*Y_DIM*NUM_CHUNKS*_TDM_CHUNKS);

// 	/*
// 	******************************************************************************
// 	* Compare C code results to expected Matlab reference script results
// 	******************************************************************************
// 	*/
// 	printf("Comparing results (%s, %s, %s, %s, %s, %s, %s, %s)...\n",
// 		output_a_data_file_re,expected_a_data_file_re,
// 		output_a_data_file_im,expected_a_data_file_im,
// 		output_b_data_file_re,expected_b_data_file_re,
// 		output_b_data_file_im,expected_b_data_file_im);
// 	int cmp_res_i;
// 	T2out dummy;
// 	cmp_res_i = compare_files(dummy, expected_a_data_file_re, output_a_data_file_re, 1, output_data_size, base,
// err);
// 	cmp_res_i = cmp_res_i & compare_files(dummy, expected_a_data_file_im, output_a_data_file_im, 1,
// output_data_size, base, err);
// 	cmp_res_i = cmp_res_i & compare_files(dummy, expected_b_data_file_re, output_b_data_file_re, 1,
// output_data_size, base, err);
// 	cmp_res_i = cmp_res_i & compare_files(dummy, expected_b_data_file_im, output_b_data_file_im, 1,
// output_data_size, base, err);

// 	if (cmp_res_i != 0) {
// 		printf("Data verification completed successfully.\n");
// 	} else {
// 		printf("Data verification completed with errors.\n");
// 	}

// 	free(data_a_out_re);
// 	free(data_a_out_im);
// 	free(data_a_out_float_re);
// 	free(data_a_out_float_im);
// 	free(data_b_out_re);
// 	free(data_b_out_im);
// 	free(data_b_out_float_re);
// 	free(data_b_out_float_im);
// 	free(data_in_re);
// 	free(data_in_im);

// 	return 0;
// }

// /*
// * real C = f(real A, real B)
// *
// * T2: default data type (float, ap_fixed)
// * T1: storage data type of input file (float -> float, ap_fixed -> int)
// */

// template<int X_DIM, int Y_DIM, int _TDM_CHUNKS, int NUM_CHUNKS, int SEL,
// 	class T1, class T2in, class T2out>
// int test_func_2_1_c(
// 	int argc, char* argv[], float base, float err,
// 	void (*func)(x_complex<T2in> [X_DIM][Y_DIM][_TDM_CHUNKS],
// 		     x_complex<T2in> [X_DIM][Y_DIM][_TDM_CHUNKS],
// 		     x_complex<T2out> [X_DIM][Y_DIM][_TDM_CHUNKS]))
// {
// 	// File names
// 	char *input_a_data_file_re    = argv[1];//"ch_matrices_re.txt";
// 	char *input_a_data_file_im    = argv[2];//"ch_matrices_im.txt";
// 	char *input_b_data_file_re    = argv[3];//"ch_matrices_re.txt";
// 	char *input_b_data_file_im    = argv[4];//"ch_matrices_im.txt";
// 	char *output_data_file_re     = argv[5];//"results_re.txt";
// 	char *output_data_file_im     = argv[6];//"results_im.txt";
// 	char *expected_data_file_re   = argv[7];//"simulation_data_re.txt";
// 	char *expected_data_file_im   = argv[8];//"simulation_data_im.txt";

// 	// Matrix dimension
// 	short matrix_dimension[2]  = {X_DIM,Y_DIM};
// 	int m = 2*matrix_dimension[0];
// 	int n = 2*matrix_dimension[1];

// 	// Data size
// 	int input_data_size        = X_DIM*Y_DIM*NUM_CHUNKS*_TDM_CHUNKS;
// 	int output_data_size       = X_DIM*Y_DIM*NUM_CHUNKS*_TDM_CHUNKS;

// 	// File data pointers
// 	T1 *data_a_in_re, *data_a_in_im, *data_b_in_re, *data_b_in_im;
// 	T1 *data_out_re, *data_out_im;
// 	float *data_out_float_re, *data_out_float_im;
// 	data_a_in_re            = (T1*)malloc(input_data_size * sizeof(T1));
// 	data_a_in_im            = (T1*)malloc(input_data_size * sizeof(T1));
// 	data_b_in_re            = (T1*)malloc(input_data_size * sizeof(T1));
// 	data_b_in_im            = (T1*)malloc(input_data_size * sizeof(T1));
// 	data_out_re             = (T1*)malloc(output_data_size * sizeof(T1));
// 	data_out_im             = (T1*)malloc(output_data_size * sizeof(T1));
// 	data_out_float_re       = (float*)malloc(output_data_size * sizeof(float));
// 	data_out_float_im       = (float*)malloc(output_data_size * sizeof(float));

// 	// Function arguments - data matrices
// 	T2in chunk_a_in_re[X_DIM][Y_DIM][_TDM_CHUNKS];
// 	T2in chunk_a_in_im[X_DIM][Y_DIM][_TDM_CHUNKS];
// 	T2in chunk_b_in_re[X_DIM][Y_DIM][_TDM_CHUNKS];
// 	T2in chunk_b_in_im[X_DIM][Y_DIM][_TDM_CHUNKS];
// 	T2out chunk_out_re[X_DIM][Y_DIM][_TDM_CHUNKS];
// 	T2out chunk_out_im[X_DIM][Y_DIM][_TDM_CHUNKS];

//   printf("Test beginning...\n");
// 	/*
// 	******************************************************************************
// 	* Load data, perfrom function, and store result
// 	******************************************************************************
// 	*/
// 	printf("Loading values from txt files (%s, %s, %s,
// %s)...\n",input_a_data_file_re,input_a_data_file_im,input_b_data_file_re,input_b_data_file_im);
// 	load_from_file(input_a_data_file_re, data_a_in_re, input_data_size);
// 	load_from_file(input_a_data_file_im, data_a_in_im, input_data_size);
// 	load_from_file(input_b_data_file_re, data_b_in_re, input_data_size);
// 	load_from_file(input_b_data_file_im, data_b_in_im, input_data_size);

// 	for(int i = 0; i < NUM_CHUNKS; i++)
// 	{
// 		printf("Loading input data array ...\n");
// 		load_input_matrix<X_DIM,Y_DIM,_TDM_CHUNKS>(i, data_a_in_re,chunk_a_in_re);
// 		load_input_matrix<X_DIM,Y_DIM,_TDM_CHUNKS>(i, data_a_in_im,chunk_a_in_im);
// 		load_input_matrix<X_DIM,Y_DIM,_TDM_CHUNKS>(i, data_b_in_re,chunk_b_in_re);
// 		load_input_matrix<X_DIM,Y_DIM,_TDM_CHUNKS>(i, data_b_in_im,chunk_b_in_im);

// 		x_complex<T2in> in_a[X_DIM][Y_DIM][_TDM_CHUNKS];
// 		x_complex<T2in> in_b[X_DIM][Y_DIM][_TDM_CHUNKS];
// 		x_complex<T2out> out_c[X_DIM][Y_DIM][_TDM_CHUNKS];
// 		for(int ii=0; ii<X_DIM; ii++) {
// 			for(int jj=0; jj<Y_DIM; jj++) {
// 				in_a[ii][jj][0].real(chunk_a_in_re[ii][jj][0]);
// 				in_a[ii][jj][0].imag(chunk_a_in_im[ii][jj][0]);
// 				in_b[ii][jj][0].real(chunk_b_in_re[ii][jj][0]);
// 				in_b[ii][jj][0].imag(chunk_b_in_im[ii][jj][0]);
// 			}
// 		}

// 		printf("Perform main function!!\n");
// 		(*func)(in_a,in_b,out_c);

// 		for(int ii=0; ii<X_DIM; ii++) {
// 			for(int jj=0; jj<Y_DIM; jj++) {
// 				chunk_out_re[ii][jj][0] = out_c[ii][jj][0].real();
// 				chunk_out_im[ii][jj][0] = out_c[ii][jj][0].imag();
// 			}
// 		}

// 		printf("Store output result ... \n");
// 		store_output_matrix<X_DIM,Y_DIM,_TDM_CHUNKS>(i, chunk_out_re, data_out_re, data_out_float_re);
// 		store_output_matrix<X_DIM,Y_DIM,_TDM_CHUNKS>(i, chunk_out_im, data_out_im, data_out_float_im);
// 	}

// 	printf("Saving results to output file (%s, %s)...\n",output_data_file_re,output_data_file_im);
// 	save_to_file(output_data_file_re, data_out_re, X_DIM*Y_DIM*NUM_CHUNKS*_TDM_CHUNKS);
// 	save_to_file(output_data_file_im, data_out_im, X_DIM*Y_DIM*NUM_CHUNKS*_TDM_CHUNKS);

// 	int out_name_len_re = strlen(output_data_file_re);
// 	char output_data_file_float_re[out_name_len_re+7];
// 	strcpy(output_data_file_float_re, output_data_file_re);
// 	output_data_file_float_re[out_name_len_re-4] = '\0';
// 	const char *float_end_re = "_float.txt";
// 	strcat(output_data_file_float_re, float_end_re);

// 	int out_name_len_im = strlen(output_data_file_im);
// 	char output_data_file_float_im[out_name_len_im+7];
// 	strcpy(output_data_file_float_im, output_data_file_im);
// 	output_data_file_float_im[out_name_len_im-4] = '\0';
// 	const char *float_end_im = "_float.txt";
// 	strcat(output_data_file_float_im, float_end_im);

// 	printf("Saving results to output file (%s, %s) ...\n", output_data_file_float_re, output_data_file_float_im);
// 	save_to_file(output_data_file_float_re, data_out_float_re, X_DIM*Y_DIM*NUM_CHUNKS*_TDM_CHUNKS);
// 	save_to_file(output_data_file_float_im, data_out_float_im, X_DIM*Y_DIM*NUM_CHUNKS*_TDM_CHUNKS);

// 	/*
// 	******************************************************************************
// 	* Compare C code results to expected Matlab reference script results
// 	******************************************************************************
// 	*/
// 	printf("Comparing results (%s, %s)...\n",output_data_file_re,expected_data_file_re);
// 	int cmp_res_i;
// 	T2out dummy;
// 	cmp_res_i = compare_files(dummy, expected_data_file_re, output_data_file_re, 1, output_data_size, base, err);
// 	printf("Comparing results (%s, %s)...\n",output_data_file_im,expected_data_file_im);
// 	cmp_res_i = cmp_res_i & compare_files(dummy, expected_data_file_im, output_data_file_im, 1, output_data_size,
// base, err);

// 	if (cmp_res_i != 0) {
// 		printf("Data verification completed successfully.\n");
// 	} else {
// 		printf("Data verification completed with errors.\n");
// 	}

// 	free(data_out_re);
// 	free(data_out_im);
// 	free(data_out_float_re);
// 	free(data_out_float_im);
// 	free(data_a_in_re);
// 	free(data_a_in_im);
// 	free(data_b_in_re);
// 	free(data_b_in_im);

// 	return 0;
// }

// /*
// * real C = f(real A, real B)
// *
// * T2: default data type (float, ap_fixed)
// * T1: storage data type of input file (float -> float, ap_fixed -> int)
// */
// template<int X_DIM1, int Y_DIM1,  int X_DIM2, int Y_DIM2, int X_DIM3, int Y_DIM3,
// 	int _TDM_CHUNKS, int NUM_CHUNKS, int SEL,
// 	class T1, class T2in, class T2out>
// int test_func_2_1_c(
// 	int argc, char* argv[], float base, float err,
// 	void (*func)(x_complex<T2in> [X_DIM1][Y_DIM1][_TDM_CHUNKS],
// 		     x_complex<T2in> [X_DIM2][Y_DIM2][_TDM_CHUNKS],
// 		     x_complex<T2out> [X_DIM3][Y_DIM3][_TDM_CHUNKS]))
// {
// 	// File names
// 	char *input_a_data_file_re    = argv[1];//"ch_matrices_re.txt";
// 	char *input_a_data_file_im    = argv[2];//"ch_matrices_im.txt";
// 	char *input_b_data_file_re    = argv[3];//"ch_matrices_re.txt";
// 	char *input_b_data_file_im    = argv[4];//"ch_matrices_im.txt";
// 	char *output_data_file_re     = argv[5];//"results_re.txt";
// 	char *output_data_file_im     = argv[6];//"results_im.txt";
// 	char *expected_data_file_re   = argv[7];//"simulation_data_re.txt";
// 	char *expected_data_file_im   = argv[8];//"simulation_data_im.txt";

// 	// Matrix dimension
// //	short matrix_dimension[2]  = {X_DIM,Y_DIM};
// //	int m = 2*matrix_dimension[0];
// //	int n = 2*matrix_dimension[1];

// 	// Data size
// 	int input1_data_size       = X_DIM1*Y_DIM1*NUM_CHUNKS*_TDM_CHUNKS;
// 	int input2_data_size       = X_DIM2*Y_DIM2*NUM_CHUNKS*_TDM_CHUNKS;
// 	int output_data_size       = X_DIM3*Y_DIM3*NUM_CHUNKS*_TDM_CHUNKS;

// 	// File data pointers
// 	T1 *data_a_in_re, *data_a_in_im, *data_b_in_re, *data_b_in_im;
// 	T1 *data_out_re, *data_out_im;
// 	float *data_out_float_re, *data_out_float_im;
// 	data_a_in_re            = (T1*)malloc(input1_data_size * sizeof(T1));
// 	data_a_in_im            = (T1*)malloc(input1_data_size * sizeof(T1));
// 	data_b_in_re            = (T1*)malloc(input2_data_size * sizeof(T1));
// 	data_b_in_im            = (T1*)malloc(input2_data_size * sizeof(T1));
// 	data_out_re             = (T1*)malloc(output_data_size * sizeof(T1));
// 	data_out_im             = (T1*)malloc(output_data_size * sizeof(T1));
// 	data_out_float_re       = (float*)malloc(output_data_size * sizeof(float));
// 	data_out_float_im       = (float*)malloc(output_data_size * sizeof(float));

// 	// Function arguments - data matrices
// 	T2in chunk_a_in_re[X_DIM1][Y_DIM1][_TDM_CHUNKS];
// 	T2in chunk_a_in_im[X_DIM1][Y_DIM1][_TDM_CHUNKS];
// 	T2in chunk_b_in_re[X_DIM2][Y_DIM2][_TDM_CHUNKS];
// 	T2in chunk_b_in_im[X_DIM2][Y_DIM2][_TDM_CHUNKS];
// 	T2out chunk_out_re[X_DIM3][Y_DIM3][_TDM_CHUNKS];
// 	T2out chunk_out_im[X_DIM3][Y_DIM3][_TDM_CHUNKS];

//   printf("Test beginning...\n");
// 	/*
// 	******************************************************************************
// 	* Load data, perfrom function, and store result
// 	******************************************************************************
// 	*/
// 	printf("Loading values from txt files (%s, %s, %s,
// %s)...\n",input_a_data_file_re,input_a_data_file_im,input_b_data_file_re,input_b_data_file_im);
// 	load_from_file(input_a_data_file_re, data_a_in_re, input1_data_size);
// 	load_from_file(input_a_data_file_im, data_a_in_im, input1_data_size);
// 	load_from_file(input_b_data_file_re, data_b_in_re, input2_data_size);
// 	load_from_file(input_b_data_file_im, data_b_in_im, input2_data_size);

// 	for(int i = 0; i < NUM_CHUNKS; i++)
// 	{
// 		printf("Loading input data array ...\n");
// 		load_input_matrix<X_DIM1,Y_DIM1,_TDM_CHUNKS>(i, data_a_in_re,chunk_a_in_re);
// 		load_input_matrix<X_DIM1,Y_DIM1,_TDM_CHUNKS>(i, data_a_in_im,chunk_a_in_im);
// 		load_input_matrix<X_DIM2,Y_DIM2,_TDM_CHUNKS>(i, data_b_in_re,chunk_b_in_re);
// 		load_input_matrix<X_DIM2,Y_DIM2,_TDM_CHUNKS>(i, data_b_in_im,chunk_b_in_im);

// 		x_complex<T2in> in_a[X_DIM1][Y_DIM1][_TDM_CHUNKS];
// 		x_complex<T2in> in_b[X_DIM2][Y_DIM2][_TDM_CHUNKS];
// 		x_complex<T2out> out_c[X_DIM3][Y_DIM3][_TDM_CHUNKS];
// 		for(int ii=0; ii<X_DIM1; ii++) {
// 			for(int jj=0; jj<Y_DIM1; jj++) {
// 				in_a[ii][jj][0].real(chunk_a_in_re[ii][jj][0]);
// 				in_a[ii][jj][0].imag(chunk_a_in_im[ii][jj][0]);
// 			}
// 		}

// 		for(int ii=0; ii<X_DIM2; ii++) {
// 			for(int jj=0; jj<Y_DIM2; jj++) {
// 				in_b[ii][jj][0].real(chunk_b_in_re[ii][jj][0]);
// 				in_b[ii][jj][0].imag(chunk_b_in_im[ii][jj][0]);
// 			}
// 		}

// 		//printf("Perform main function!!\n");
// 		printf("Perform main function now!!\n");
// 		(*func)(in_a,in_b,out_c);

// 		printf("Main function done!!\n");

// 		for(int ii=0; ii<X_DIM3; ii++) {
// 			for(int jj=0; jj<Y_DIM3; jj++) {
// 				chunk_out_re[ii][jj][0] = out_c[ii][jj][0].real();
// 				chunk_out_im[ii][jj][0] = out_c[ii][jj][0].imag();
// 			}
// 		}

// 		printf("Store output result ... \n");
// 		store_output_matrix<X_DIM3,Y_DIM3,_TDM_CHUNKS>(i, chunk_out_re, data_out_re, data_out_float_re);
// 		store_output_matrix<X_DIM3,Y_DIM3,_TDM_CHUNKS>(i, chunk_out_im, data_out_im, data_out_float_im);

// 	}

// 	printf("Saving results to output file (%s, %s)...\n",output_data_file_re,output_data_file_im);
// 	save_to_file(output_data_file_re, data_out_re, X_DIM3*Y_DIM3*NUM_CHUNKS*_TDM_CHUNKS);
// 	save_to_file(output_data_file_im, data_out_im, X_DIM3*Y_DIM3*NUM_CHUNKS*_TDM_CHUNKS);

// 	int out_name_len_re = strlen(output_data_file_re);
// 	char output_data_file_float_re[out_name_len_re+7];
// 	strcpy(output_data_file_float_re, output_data_file_re);
// 	output_data_file_float_re[out_name_len_re-4] = '\0';
// 	const char *float_end_re = "_float.txt";
// 	strcat(output_data_file_float_re, float_end_re);

// 	int out_name_len_im = strlen(output_data_file_im);
// 	char output_data_file_float_im[out_name_len_im+7];
// 	strcpy(output_data_file_float_im, output_data_file_im);
// 	output_data_file_float_im[out_name_len_im-4] = '\0';
// 	const char *float_end_im = "_float.txt";
// 	strcat(output_data_file_float_im, float_end_im);

// 	printf("Saving results to output file (%s, %s) ...\n", output_data_file_float_re, output_data_file_float_im);
// 	save_to_file(output_data_file_float_re, data_out_float_re, X_DIM3*Y_DIM3*NUM_CHUNKS*_TDM_CHUNKS);
// 	save_to_file(output_data_file_float_im, data_out_float_im, X_DIM3*Y_DIM3*NUM_CHUNKS*_TDM_CHUNKS);

// 	//------------------------------------------------------------------------------
// 	//- Compare C code results to expected Matlab reference script results
// 	//------------------------------------------------------------------------------

// 	printf("Comparing results (%s, %s)...\n",output_data_file_re,expected_data_file_re);
// 	int cmp_res_i;
// 	T2out dummy;
// 	cmp_res_i = compare_files(dummy, expected_data_file_re, output_data_file_re, 1, output_data_size, base, err);
// 	printf("Comparing results (%s, %s)...\n",output_data_file_im,expected_data_file_im);
// 	cmp_res_i = cmp_res_i & compare_files(dummy, expected_data_file_im, output_data_file_im, 1, output_data_size,
// base, err);

// 	if (cmp_res_i != 0) {
// 		printf("Data verification completed successfully.\n");
// 	} else {
// 		printf("Data verification completed with errors.\n");
// 	}

// 	free(data_out_re);
// 	free(data_out_im);
// 	free(data_out_float_re);
// 	free(data_out_float_im);
// 	free(data_a_in_re);
// 	free(data_a_in_im);
// 	free(data_b_in_re);
// 	free(data_b_in_im);

// 	return 0;
// }

// /*
// * real C, real D = f(real A, real B)
// *
// * T2: default data type (float, ap_fixed)
// * T1: storage data type of input file (float -> float, ap_fixed -> int)
// */
// template<int X_DIM1, int Y_DIM1,  int X_DIM2, int Y_DIM2,
// 	int X_DIM3, int Y_DIM3, int X_DIM4, int Y_DIM4,
// 	int _TDM_CHUNKS, int NUM_CHUNKS, int SEL,
// 	class T1, class T2in, class T2out>
// int test_func_2_2_c(
// 	int argc, char* argv[], float base, float err,
// 	void (*func)(x_complex<T2in> [X_DIM1][Y_DIM1][_TDM_CHUNKS],
// 		     x_complex<T2in> [X_DIM2][Y_DIM2][_TDM_CHUNKS],
// 		     x_complex<T2out> [X_DIM3][Y_DIM3][_TDM_CHUNKS],
// 		     x_complex<T2out> [X_DIM4][Y_DIM4][_TDM_CHUNKS]))
// {
// 	// File names
// 	char *input_a_data_file_re    = argv[1];//"ch_matrices_re.txt";
// 	char *input_a_data_file_im    = argv[2];//"ch_matrices_im.txt";
// 	char *input_b_data_file_re    = argv[3];//"ch_matrices_re.txt";
// 	char *input_b_data_file_im    = argv[4];//"ch_matrices_im.txt";
// 	char *output_c_data_file_re   = argv[5];//"results_re.txt";
// 	char *output_c_data_file_im   = argv[6];//"results_im.txt";
// 	char *output_d_data_file_re   = argv[7];//"results_re.txt";
// 	char *output_d_data_file_im   = argv[8];//"results_im.txt";
// 	char *expected_c_data_file_re = argv[9];//"simulation_data_re.txt";
// 	char *expected_c_data_file_im = argv[10];//"simulation_data_im.txt";
// 	char *expected_d_data_file_re = argv[11];//"simulation_data_re.txt";
// 	char *expected_d_data_file_im = argv[12];//"simulation_data_im.txt";

// 	// Matrix dimension
// //	short matrix_dimension[2]  = {X_DIM,Y_DIM};
// //	int m = 2*matrix_dimension[0];
// //	int n = 2*matrix_dimension[1];

// 	// Data size
// 	int input1_data_size       = X_DIM1*Y_DIM1*NUM_CHUNKS*_TDM_CHUNKS;
// 	int input2_data_size       = X_DIM2*Y_DIM2*NUM_CHUNKS*_TDM_CHUNKS;
// 	int output1_data_size      = X_DIM3*Y_DIM3*NUM_CHUNKS*_TDM_CHUNKS;
// 	int output2_data_size      = X_DIM4*Y_DIM4*NUM_CHUNKS*_TDM_CHUNKS;

// 	// File data pointers
// 	T1 *data_a_in_re, *data_a_in_im, *data_b_in_re, *data_b_in_im;
// 	T1 *data_c_out_re, *data_c_out_im, *data_d_out_re, *data_d_out_im;
// 	float *data_c_out_float_re, *data_c_out_float_im;
// 	float *data_d_out_float_re, *data_d_out_float_im;

// 	data_a_in_re            = (T1*)malloc(input1_data_size * sizeof(T1));
// 	data_a_in_im            = (T1*)malloc(input1_data_size * sizeof(T1));
// 	data_b_in_re            = (T1*)malloc(input2_data_size * sizeof(T1));
// 	data_b_in_im            = (T1*)malloc(input2_data_size * sizeof(T1));
// 	data_c_out_re           = (T1*)malloc(output1_data_size * sizeof(T1));
// 	data_c_out_im           = (T1*)malloc(output1_data_size * sizeof(T1));
// 	data_d_out_re           = (T1*)malloc(output2_data_size * sizeof(T1));
// 	data_d_out_im           = (T1*)malloc(output2_data_size * sizeof(T1));
// 	data_c_out_float_re     = (float*)malloc(output1_data_size * sizeof(float));
// 	data_c_out_float_im     = (float*)malloc(output1_data_size * sizeof(float));
// 	data_d_out_float_re     = (float*)malloc(output2_data_size * sizeof(float));
// 	data_d_out_float_im     = (float*)malloc(output2_data_size * sizeof(float));

// 	// Function arguments - data matrices
// 	T2in chunk_a_in_re[X_DIM1][Y_DIM1][_TDM_CHUNKS];
// 	T2in chunk_a_in_im[X_DIM1][Y_DIM1][_TDM_CHUNKS];
// 	T2in chunk_b_in_re[X_DIM2][Y_DIM2][_TDM_CHUNKS];
// 	T2in chunk_b_in_im[X_DIM2][Y_DIM2][_TDM_CHUNKS];
// 	T2out chunk_c_out_re[X_DIM3][Y_DIM3][_TDM_CHUNKS];
// 	T2out chunk_c_out_im[X_DIM3][Y_DIM3][_TDM_CHUNKS];
// 	T2out chunk_d_out_re[X_DIM4][Y_DIM4][_TDM_CHUNKS];
// 	T2out chunk_d_out_im[X_DIM4][Y_DIM4][_TDM_CHUNKS];

//   printf("Test beginning...\n");
// 	/*
// 	******************************************************************************
// 	* Load data, perfrom function, and store result
// 	******************************************************************************
// 	*/
// 	printf("Loading values from txt files (%s, %s, %s,
// %s)...\n",input_a_data_file_re,input_a_data_file_im,input_b_data_file_re,input_b_data_file_im);
// 	load_from_file(input_a_data_file_re, data_a_in_re, input1_data_size);
// 	load_from_file(input_a_data_file_im, data_a_in_im, input1_data_size);
// 	load_from_file(input_b_data_file_re, data_b_in_re, input2_data_size);
// 	load_from_file(input_b_data_file_im, data_b_in_im, input2_data_size);

// 	for(int i = 0; i < NUM_CHUNKS; i++)
// 	{
// 		printf("Loading input a data array ...\n");
// 		load_input_matrix<X_DIM1,Y_DIM1,_TDM_CHUNKS>(i, data_a_in_re,chunk_a_in_re);
// 		load_input_matrix<X_DIM1,Y_DIM1,_TDM_CHUNKS>(i, data_a_in_im,chunk_a_in_im);
// 		printf("Loading input a data array ...\n");
// 		load_input_matrix<X_DIM2,Y_DIM2,_TDM_CHUNKS>(i, data_b_in_re,chunk_b_in_re);
// 		load_input_matrix<X_DIM2,Y_DIM2,_TDM_CHUNKS>(i, data_b_in_im,chunk_b_in_im);

// 		x_complex<T2in>  in_a[X_DIM1][Y_DIM1][_TDM_CHUNKS];
// 		x_complex<T2in>  in_b[X_DIM2][Y_DIM2][_TDM_CHUNKS];
// 		x_complex<T2out> out_c[X_DIM3][Y_DIM3][_TDM_CHUNKS];
// 		x_complex<T2out> out_d[X_DIM4][Y_DIM4][_TDM_CHUNKS];

// 		for(int ii=0; ii<X_DIM1; ii++) {
// 			for(int jj=0; jj<Y_DIM1; jj++) {
// 				in_a[ii][jj][0].real(chunk_a_in_re[ii][jj][0]);
// 				in_a[ii][jj][0].imag(chunk_a_in_im[ii][jj][0]);
// 			}
// 		}

// 		for(int ii=0; ii<X_DIM2; ii++) {
// 			for(int jj=0; jj<Y_DIM2; jj++) {
// 				in_b[ii][jj][0].real(chunk_b_in_re[ii][jj][0]);
// 				in_b[ii][jj][0].imag(chunk_b_in_im[ii][jj][0]);
// 			}
// 		}

// 		printf("in_a\n");
// 		printf("-------\n");
// 		dumpm<X_DIM1,Y_DIM1,_TDM_CHUNKS,0>(in_a);

// 		printf("in_b\n");
// 		printf("-------\n");
// 		dumpm<X_DIM2,Y_DIM2,_TDM_CHUNKS,0>(in_b);

// 		//printf("Perform main function!!\n");
// 		printf("Perform main function now!!\n");
// 		(*func)(in_a,in_b,out_c,out_d);

// 		printf("Main function done!!\n");

// 		printf("out_c\n");
// 		printf("-------\n");
// 		dumpm<X_DIM3,Y_DIM3,_TDM_CHUNKS,0>(out_c);

// 		printf("out_d\n");
// 		printf("-------\n");
// 		dumpm<X_DIM4,Y_DIM4,_TDM_CHUNKS,0>(out_d);

// 		for(int ii=0; ii<X_DIM3; ii++) {
// 			for(int jj=0; jj<Y_DIM3; jj++) {
// 				chunk_c_out_re[ii][jj][0] = out_c[ii][jj][0].real();
// 				chunk_c_out_im[ii][jj][0] = out_c[ii][jj][0].imag();
// 			}
// 		}

// 		for(int ii=0; ii<X_DIM4; ii++) {
// 			for(int jj=0; jj<Y_DIM4; jj++) {
// 				chunk_d_out_re[ii][jj][0] = out_d[ii][jj][0].real();
// 				chunk_d_out_im[ii][jj][0] = out_d[ii][jj][0].imag();
// 			}
// 		}

// 		printf("Store output result ... \n");
// 		store_output_matrix<X_DIM3,Y_DIM3,_TDM_CHUNKS>(i, chunk_c_out_re, data_c_out_re, data_c_out_float_re);
// 		store_output_matrix<X_DIM3,Y_DIM3,_TDM_CHUNKS>(i, chunk_c_out_im, data_c_out_im, data_c_out_float_im);
// 		store_output_matrix<X_DIM4,Y_DIM4,_TDM_CHUNKS>(i, chunk_d_out_re, data_d_out_re, data_d_out_float_re);
// 		store_output_matrix<X_DIM4,Y_DIM4,_TDM_CHUNKS>(i, chunk_d_out_im, data_d_out_im, data_d_out_float_im);

// 	}

// 	printf("Saving results to output file (%s, %s)...\n",output_c_data_file_re,output_c_data_file_im);
// 	save_to_file(output_c_data_file_re, data_c_out_re, X_DIM3*Y_DIM3*NUM_CHUNKS*_TDM_CHUNKS);
// 	save_to_file(output_c_data_file_im, data_c_out_im, X_DIM3*Y_DIM3*NUM_CHUNKS*_TDM_CHUNKS);

// 	int out_c_name_len_re = strlen(output_c_data_file_re);
// 	char output_c_data_file_float_re[out_c_name_len_re+7];
// 	strcpy(output_c_data_file_float_re, output_c_data_file_re);
// 	output_c_data_file_float_re[out_c_name_len_re-4] = '\0';
// 	const char *float_c_end_re = "_float.txt";
// 	strcat(output_c_data_file_float_re, float_c_end_re);

// 	int out_c_name_len_im = strlen(output_c_data_file_im);
// 	char output_c_data_file_float_im[out_c_name_len_im+7];
// 	strcpy(output_c_data_file_float_im, output_c_data_file_im);
// 	output_c_data_file_float_im[out_c_name_len_im-4] = '\0';
// 	const char *float_c_end_im = "_float.txt";
// 	strcat(output_c_data_file_float_im, float_c_end_im);

// 	printf("Saving results to output file (%s, %s) ...\n", output_c_data_file_float_re,
// output_c_data_file_float_im);
// 	save_to_file(output_c_data_file_float_re, data_c_out_float_re, X_DIM3*Y_DIM3*NUM_CHUNKS*_TDM_CHUNKS);
// 	save_to_file(output_c_data_file_float_im, data_c_out_float_im, X_DIM3*Y_DIM3*NUM_CHUNKS*_TDM_CHUNKS);

// 	printf("Saving results to output file (%s, %s)...\n",output_d_data_file_re,output_d_data_file_im);
// 	save_to_file(output_d_data_file_re, data_d_out_re, X_DIM4*Y_DIM4*NUM_CHUNKS*_TDM_CHUNKS);
// 	save_to_file(output_d_data_file_im, data_d_out_im, X_DIM4*Y_DIM4*NUM_CHUNKS*_TDM_CHUNKS);

// 	int out_d_name_len_re = strlen(output_d_data_file_re);
// 	char output_d_data_file_float_re[out_d_name_len_re+7];
// 	strcpy(output_d_data_file_float_re, output_d_data_file_re);
// 	output_d_data_file_float_re[out_d_name_len_re-4] = '\0';
// 	const char *float_d_end_re = "_float.txt";
// 	strcat(output_d_data_file_float_re, float_d_end_re);

// 	int out_d_name_len_im = strlen(output_d_data_file_im);
// 	char output_d_data_file_float_im[out_d_name_len_im+7];
// 	strcpy(output_d_data_file_float_im, output_d_data_file_im);
// 	output_d_data_file_float_im[out_d_name_len_im-4] = '\0';
// 	const char *float_d_end_im = "_float.txt";
// 	strcat(output_d_data_file_float_im, float_d_end_im);

// 	printf("Saving results to output file (%s, %s) ...\n", output_d_data_file_float_re,
// output_d_data_file_float_im);
// 	save_to_file(output_d_data_file_float_re, data_d_out_float_re, X_DIM4*Y_DIM4*NUM_CHUNKS*_TDM_CHUNKS);
// 	save_to_file(output_d_data_file_float_im, data_d_out_float_im, X_DIM4*Y_DIM4*NUM_CHUNKS*_TDM_CHUNKS);

// 	//------------------------------------------------------------------------------
// 	//- Compare C code results to expected Matlab reference script results
// 	//------------------------------------------------------------------------------

// 	printf("Comparing results (%s, %s)...\n",output_c_data_file_re,expected_c_data_file_re);
// 	int cmp_res_i;
// 	T2out dummy;
// 	cmp_res_i = compare_files(dummy, expected_c_data_file_re, output_c_data_file_re, 1, output1_data_size, base,
// err);
// 	printf("Comparing results (%s, %s)...\n",output_c_data_file_im,expected_c_data_file_im);
// 	cmp_res_i = cmp_res_i & compare_files(dummy, expected_c_data_file_im, output_c_data_file_im, 1,
// output1_data_size, base, err);

// 	printf("Comparing results (%s, %s)...\n",output_d_data_file_re,expected_d_data_file_re);
// 	cmp_res_i = compare_files(dummy, expected_d_data_file_re, output_d_data_file_re, 1, output2_data_size, base,
// err);
// 	printf("Comparing results (%s, %s)...\n",output_d_data_file_im,expected_d_data_file_im);
// 	cmp_res_i = cmp_res_i & compare_files(dummy, expected_d_data_file_im, output_d_data_file_im, 1,
// output2_data_size, base, err);

// 	if (cmp_res_i != 0) {
// 		printf("Data verification completed successfully.\n");
// 	} else {
// 		printf("Data verification completed with errors.\n");
// 	}

// 	free(data_c_out_re);
// 	free(data_c_out_im);
// 	free(data_c_out_float_re);
// 	free(data_c_out_float_im);
// 	free(data_d_out_re);
// 	free(data_d_out_im);
// 	free(data_d_out_float_re);
// 	free(data_d_out_float_im);
// 	free(data_a_in_re);
// 	free(data_a_in_im);
// 	free(data_b_in_re);
// 	free(data_b_in_im);

// 	return 0;
// }

/*
* (real B, real C, real D) = f(real A)
*
* T2: default data type (float, ap_fixed)
* T1: storage data type of input file (float -> float, ap_fixed -> int)
*/
template <int X_DIM, int Y_DIM, int _TDM_CHUNKS, int NUM_CHUNKS, int SEL, class T1, class T2in, class T2out>
int test_func_1_3(int argc,
                  char* argv[],
                  float base,
                  float err,
                  void (*func)(T2in[X_DIM][Y_DIM][_TDM_CHUNKS],
                               T2out[X_DIM][Y_DIM][_TDM_CHUNKS],
                               T2out[X_DIM][Y_DIM][_TDM_CHUNKS],
                               T2out[X_DIM][Y_DIM][_TDM_CHUNKS])) {
    // File names
    char* input_data_file = argv[1];      //"ch_matrices_re.txt";
    char* output_a_data_file = argv[2];   //"results_a_re.txt";
    char* output_b_data_file = argv[3];   //"results_b_re.txt";
    char* output_c_data_file = argv[4];   //"results_c_re.txt";
    char* expected_a_data_file = argv[5]; //"simulation_a_data_re.txt";
    char* expected_b_data_file = argv[6]; //"simulation_b_data_re.txt";
    char* expected_c_data_file = argv[7]; //"simulation_c_data_re.txt";

    // Matrix dimension
    short matrix_dimension[2] = {X_DIM, Y_DIM};
    int m = 2 * matrix_dimension[0];
    int n = 2 * matrix_dimension[1];

    // Data size
    int input_data_size = X_DIM * Y_DIM * NUM_CHUNKS * _TDM_CHUNKS;
    int output_data_size = X_DIM * Y_DIM * NUM_CHUNKS * _TDM_CHUNKS;

    // File data pointers
    T1* data_in;
    T1 *data_a_out, *data_b_out, *data_c_out;
    float *data_a_out_float, *data_b_out_float, *data_c_out_float;
    data_in = (T1*)malloc(input_data_size * sizeof(T1));
    data_a_out = (T1*)malloc(output_data_size * sizeof(T1));
    data_b_out = (T1*)malloc(output_data_size * sizeof(T1));
    data_c_out = (T1*)malloc(output_data_size * sizeof(T1));
    data_a_out_float = (float*)malloc(output_data_size * sizeof(float));
    data_b_out_float = (float*)malloc(output_data_size * sizeof(float));
    data_c_out_float = (float*)malloc(output_data_size * sizeof(float));

    // Function arguments - data matrices
    T2in chunk_in_re[X_DIM][Y_DIM][_TDM_CHUNKS];
    T2out chunk_a_out_re[X_DIM][Y_DIM][_TDM_CHUNKS];
    T2out chunk_b_out_re[X_DIM][Y_DIM][_TDM_CHUNKS];
    T2out chunk_c_out_re[X_DIM][Y_DIM][_TDM_CHUNKS];

    printf("Test beginning...\n");
    /*
    ******************************************************************************
    * Load data, perfrom function, and store result
    ******************************************************************************
    */
    printf("Loading values from txt files (%s)...\n", input_data_file);
    load_from_file(input_data_file, data_in, input_data_size);

    for (int i = 0; i < NUM_CHUNKS; i++) {
        printf("Loading input data array ...\n");
        load_input_matrix<X_DIM, Y_DIM, _TDM_CHUNKS>(i, data_in, chunk_in_re);

        printf("Perform main function!!\n");
        //(*func)<X_DIM,_TDM_CHUNKS,SEL>(chunk_in_re, chunk_out_re);
        (*func)(chunk_in_re, chunk_a_out_re, chunk_b_out_re, chunk_c_out_re);

        printf("Store output result ... \n");
        store_output_matrix<X_DIM, Y_DIM, _TDM_CHUNKS>(i, chunk_a_out_re, data_a_out, data_a_out_float);
        store_output_matrix<X_DIM, Y_DIM, _TDM_CHUNKS>(i, chunk_b_out_re, data_b_out, data_b_out_float);
        store_output_matrix<X_DIM, Y_DIM, _TDM_CHUNKS>(i, chunk_c_out_re, data_c_out, data_c_out_float);
    }

    printf("Saving results to output file (%s, %s, %s)...\n", output_a_data_file, output_b_data_file,
           output_c_data_file);
    save_to_file(output_a_data_file, data_a_out, X_DIM * Y_DIM * NUM_CHUNKS * _TDM_CHUNKS);
    save_to_file(output_b_data_file, data_b_out, X_DIM * Y_DIM * NUM_CHUNKS * _TDM_CHUNKS);
    save_to_file(output_c_data_file, data_c_out, X_DIM * Y_DIM * NUM_CHUNKS * _TDM_CHUNKS);

    int filename_len;
    const char* float_end = "_float.txt";

    filename_len = strlen(output_a_data_file);
    char* output_a_data_file_float = (char*)malloc((filename_len + 7) * sizeof(char));
    strcpy(output_a_data_file_float, output_a_data_file);
    output_a_data_file_float[filename_len - 4] = '\0';
    strcat(output_a_data_file_float, float_end);

    filename_len = strlen(output_b_data_file);
    char* output_b_data_file_float = (char*)malloc((filename_len + 7) * sizeof(char));
    strcpy(output_b_data_file_float, output_b_data_file);
    output_b_data_file_float[filename_len - 4] = '\0';
    strcat(output_b_data_file_float, float_end);

    filename_len = strlen(output_c_data_file);
    char* output_c_data_file_float = (char*)malloc((filename_len + 7) * sizeof(char));
    strcpy(output_c_data_file_float, output_c_data_file);
    output_c_data_file_float[filename_len - 4] = '\0';
    strcat(output_c_data_file_float, float_end);

    printf("Saving results to output file (%s, %s, %s) ...\n", output_a_data_file_float, output_b_data_file_float,
           output_c_data_file_float);
    save_to_file(output_a_data_file_float, data_a_out_float, X_DIM * Y_DIM * NUM_CHUNKS * _TDM_CHUNKS);
    save_to_file(output_b_data_file_float, data_b_out_float, X_DIM * Y_DIM * NUM_CHUNKS * _TDM_CHUNKS);
    save_to_file(output_c_data_file_float, data_c_out_float, X_DIM * Y_DIM * NUM_CHUNKS * _TDM_CHUNKS);

    /*
    ******************************************************************************
    * Compare C code results to expected Matlab reference script results
    ******************************************************************************
    */
    printf("Comparing results (%s, %s)...\n", output_a_data_file, expected_a_data_file);
    int cmp_res_i;
    T2out dummy;
    cmp_res_i = compare_files(dummy, expected_a_data_file, output_a_data_file, 1, output_data_size, base, err);
    printf("Comparing results (%s, %s)...\n", output_b_data_file, expected_b_data_file);
    cmp_res_i =
        cmp_res_i & compare_files(dummy, expected_b_data_file, output_b_data_file, 1, output_data_size, base, err);
    printf("Comparing results (%s, %s)...\n", output_c_data_file, expected_c_data_file);
    cmp_res_i =
        cmp_res_i & compare_files(dummy, expected_c_data_file, output_c_data_file, 1, output_data_size, base, err);

    if (cmp_res_i != 0) {
        printf("Data verification completed successfully.\n");
    } else {
        printf("Data verification completed with errors.\n");
    }

    free(data_a_out);
    free(data_a_out_float);
    free(data_b_out);
    free(data_b_out_float);
    free(data_c_out);
    free(data_c_out_float);
    free(data_in);

    return 0;
}

/*
* complex C = f(complex A)
*
* T2: default data type (float, ap_fixed)
* T1: storage data type of input file (float -> float, ap_fixed -> int)
*/
template <int X_DIM, int Y_DIM, int _TDM_CHUNKS, int NUM_CHUNKS, int SEL, class T1, class T2in, class T2out>
int test_func_1_1(int argc,
                  char* argv[],
                  float base,
                  float err,
                  void (*func)(T2in[X_DIM][Y_DIM][_TDM_CHUNKS],
                               T2in[X_DIM][Y_DIM][_TDM_CHUNKS],
                               T2out[X_DIM][Y_DIM][_TDM_CHUNKS],
                               T2out[X_DIM][Y_DIM][_TDM_CHUNKS])) {
    // File names
    char* input_data_file_re = argv[1];    //"ch_matrices_re.txt";
    char* input_data_file_im = argv[2];    //"ch_matrices_im.txt";
    char* output_data_file_re = argv[3];   //"results_re.txt";
    char* output_data_file_im = argv[4];   //"results_im.txt";
    char* expected_data_file_re = argv[5]; //"simulation_data_re.txt";
    char* expected_data_file_im = argv[6]; //"simulation_data_im.txt";

    // Matrix dimension
    short matrix_dimension[2] = {X_DIM, Y_DIM};
    int m = 2 * matrix_dimension[0];
    int n = 2 * matrix_dimension[1];

    // Data size
    int input_data_size = X_DIM * Y_DIM * NUM_CHUNKS * _TDM_CHUNKS;
    int output_data_size = X_DIM * Y_DIM * NUM_CHUNKS * _TDM_CHUNKS;

    // File data pointers
    T1 *data_in_re, *data_in_im;
    T1 *data_out_re, *data_out_im;
    float *data_out_float_re, *data_out_float_im;
    data_in_re = (T1*)malloc(input_data_size * sizeof(T1));
    data_in_im = (T1*)malloc(input_data_size * sizeof(T1));
    data_out_re = (T1*)malloc(output_data_size * sizeof(T1));
    data_out_im = (T1*)malloc(output_data_size * sizeof(T1));
    data_out_float_re = (float*)malloc(output_data_size * sizeof(float));
    data_out_float_im = (float*)malloc(output_data_size * sizeof(float));

    // Function arguments - data matrices
    T2in chunk_in_re[X_DIM][Y_DIM][_TDM_CHUNKS];
    T2in chunk_in_im[X_DIM][Y_DIM][_TDM_CHUNKS];
    T2out chunk_out_re[X_DIM][Y_DIM][_TDM_CHUNKS];
    T2out chunk_out_im[X_DIM][Y_DIM][_TDM_CHUNKS];

    printf("Test beginning...\n");
    /*
    ******************************************************************************
    * Load data, perfrom function, and store result
    ******************************************************************************
    */
    printf("Loading values from txt files (%s, %s)...\n", input_data_file_re, input_data_file_im);
    load_from_file(input_data_file_re, data_in_re, input_data_size);
    load_from_file(input_data_file_im, data_in_im, input_data_size);

    for (int i = 0; i < NUM_CHUNKS; i++) {
        printf("Loading input data array ...\n");
        load_input_matrix<X_DIM, Y_DIM, _TDM_CHUNKS>(i, data_in_re, chunk_in_re);
        load_input_matrix<X_DIM, Y_DIM, _TDM_CHUNKS>(i, data_in_im, chunk_in_im);

        printf("Perform main function!!\n");
        (*func)(chunk_in_re, chunk_in_im, chunk_out_re, chunk_out_im);

        printf("Store output result ... \n");
        store_output_matrix<X_DIM, Y_DIM, _TDM_CHUNKS>(i, chunk_out_re, data_out_re, data_out_float_re);
        store_output_matrix<X_DIM, Y_DIM, _TDM_CHUNKS>(i, chunk_out_im, data_out_im, data_out_float_im);
    }

    printf("Saving results to output file (%s, %s)...\n", output_data_file_re, output_data_file_im);
    save_to_file(output_data_file_re, data_out_re, X_DIM * Y_DIM * NUM_CHUNKS * _TDM_CHUNKS);
    save_to_file(output_data_file_im, data_out_im, X_DIM * Y_DIM * NUM_CHUNKS * _TDM_CHUNKS);

    int out_name_len_re = strlen(output_data_file_re);
    char output_data_file_float_re[out_name_len_re + 7];
    strcpy(output_data_file_float_re, output_data_file_re);
    output_data_file_float_re[out_name_len_re - 4] = '\0';
    const char* float_end_re = "_float.txt";
    strcat(output_data_file_float_re, float_end_re);

    int out_name_len_im = strlen(output_data_file_im);
    char output_data_file_float_im[out_name_len_im + 7];
    strcpy(output_data_file_float_im, output_data_file_im);
    output_data_file_float_im[out_name_len_im - 4] = '\0';
    const char* float_end_im = "_float.txt";
    strcat(output_data_file_float_im, float_end_im);

    printf("Saving results to output file (%s, %s) ...\n", output_data_file_float_re, output_data_file_float_im);
    save_to_file(output_data_file_float_re, data_out_float_re, X_DIM * Y_DIM * NUM_CHUNKS * _TDM_CHUNKS);
    save_to_file(output_data_file_float_im, data_out_float_im, X_DIM * Y_DIM * NUM_CHUNKS * _TDM_CHUNKS);

    /*
    ******************************************************************************
    * Compare C code results to expected Matlab reference script results
    ******************************************************************************
    */
    printf("Comparing results (%s, %s)...\n", output_data_file_re, expected_data_file_re);
    int cmp_res_i;
    T2out dummy;
    cmp_res_i = compare_files(dummy, expected_data_file_re, output_data_file_re, 1, output_data_size, base, err);
    printf("Comparing results (%s, %s)...\n", output_data_file_im, expected_data_file_im);
    cmp_res_i =
        cmp_res_i & compare_files(dummy, expected_data_file_im, output_data_file_im, 1, output_data_size, base, err);

    if (cmp_res_i != 0) {
        printf("Data verification completed successfully.\n");
    } else {
        printf("Data verification completed with errors.\n");
    }

    free(data_out_re);
    free(data_out_im);
    free(data_out_float_re);
    free(data_out_float_im);
    free(data_in_re);
    free(data_in_im);

    return 0;
}

// /*
// * complex C = f(complex A)
// *
// * T2: default data type (float, ap_fixed)
// * T1: storage data type of input file (float -> float, ap_fixed -> int)
// */
// template<int X_DIM, int Y_DIM, int _TDM_CHUNKS, int NUM_CHUNKS, int SEL,
// 	 class T1, class T2in, class T2out>
// int test_func_1_1_c(
// 	int argc, char* argv[], float base, float err,
// 	void (*func)(x_complex<T2in>[X_DIM][Y_DIM][_TDM_CHUNKS],
// 		     x_complex<T2out>[X_DIM][Y_DIM][_TDM_CHUNKS]))
// {
// 	// File names
// 	char *input_data_file_re    = argv[1];//"ch_matrices_re.txt";
// 	char *input_data_file_im    = argv[2];//"ch_matrices_im.txt";
// 	char *output_data_file_re     = argv[3];//"results_re.txt";
// 	char *output_data_file_im     = argv[4];//"results_im.txt";
// 	char *expected_data_file_re   = argv[5];//"simulation_data_re.txt";
// 	char *expected_data_file_im   = argv[6];//"simulation_data_im.txt";

// 	// Matrix dimension
// 	short matrix_dimension[2]  = {X_DIM,Y_DIM};
// 	int m = 2*matrix_dimension[0];
// 	int n = 2*matrix_dimension[1];

// 	// Data size
// 	int input_data_size        = X_DIM*Y_DIM*NUM_CHUNKS*_TDM_CHUNKS;
// 	int output_data_size       = X_DIM*Y_DIM*NUM_CHUNKS*_TDM_CHUNKS;

// 	// File data pointers
// 	T1 *data_in_re, *data_in_im;
// 	T1 *data_out_re, *data_out_im;
// 	float *data_out_float_re, *data_out_float_im;
// 	data_in_re              = (T1*)malloc(input_data_size * sizeof(T1));
// 	data_in_im              = (T1*)malloc(input_data_size * sizeof(T1));
// 	data_out_re             = (T1*)malloc(output_data_size * sizeof(T1));
// 	data_out_im             = (T1*)malloc(output_data_size * sizeof(T1));
// 	data_out_float_re       = (float*)malloc(output_data_size * sizeof(float));
// 	data_out_float_im       = (float*)malloc(output_data_size * sizeof(float));

// 	// Function arguments - data matrices
// 	T2in  chunk_in_re[X_DIM][Y_DIM][_TDM_CHUNKS];
// 	T2in  chunk_in_im[X_DIM][Y_DIM][_TDM_CHUNKS];
// 	T2out chunk_out_re[X_DIM][Y_DIM][_TDM_CHUNKS];
// 	T2out chunk_out_im[X_DIM][Y_DIM][_TDM_CHUNKS];

//   printf("Test beginning...\n");
// 	/*
// 	******************************************************************************
// 	* Load data, perfrom function, and store result
// 	******************************************************************************
// 	*/
// 	printf("Loading values from txt files (%s, %s)...\n",input_data_file_re,input_data_file_im);
// 	load_from_file(input_data_file_re, data_in_re, input_data_size);
// 	load_from_file(input_data_file_im, data_in_im, input_data_size);

// 	for(int i = 0; i < NUM_CHUNKS; i++)
// 	{
// 		printf("Loading input data array ...\n");
// 		load_input_matrix<X_DIM,Y_DIM,_TDM_CHUNKS>(i, data_in_re,chunk_in_re);
// 		load_input_matrix<X_DIM,Y_DIM,_TDM_CHUNKS>(i, data_in_im,chunk_in_im);

// 		x_complex<T2in> in_a[X_DIM][Y_DIM][_TDM_CHUNKS];
// 		x_complex<T2out> out_c[X_DIM][Y_DIM][_TDM_CHUNKS];
// 		for(int ii=0; ii<X_DIM; ii++) {
// 			for(int jj=0; jj<Y_DIM; jj++) {
// 				in_a[ii][jj][0].real(chunk_in_re[ii][jj][0]);
// 				in_a[ii][jj][0].imag(chunk_in_im[ii][jj][0]);
// 			}
// 		}

// 		printf("Perform main function!!\n");
// 		(*func)(in_a,out_c);

// 		for(int ii=0; ii<X_DIM; ii++) {
// 			for(int jj=0; jj<Y_DIM; jj++) {
// 				chunk_out_re[ii][jj][0] = out_c[ii][jj][0].real();
// 				chunk_out_im[ii][jj][0] = out_c[ii][jj][0].imag();
// 			}
// 		}

// 		printf("Store output result ... \n");
// 		store_output_matrix<X_DIM,Y_DIM,_TDM_CHUNKS>(i, chunk_out_re, data_out_re, data_out_float_re);
// 		store_output_matrix<X_DIM,Y_DIM,_TDM_CHUNKS>(i, chunk_out_im, data_out_im, data_out_float_im);
// 	}

// 	printf("Saving results to output file (%s, %s)...\n",output_data_file_re,output_data_file_im);
// 	save_to_file(output_data_file_re, data_out_re, X_DIM*Y_DIM*NUM_CHUNKS*_TDM_CHUNKS);
// 	save_to_file(output_data_file_im, data_out_im, X_DIM*Y_DIM*NUM_CHUNKS*_TDM_CHUNKS);

// 	int out_name_len_re = strlen(output_data_file_re);
// 	char output_data_file_float_re[out_name_len_re+7];
// 	strcpy(output_data_file_float_re, output_data_file_re);
// 	output_data_file_float_re[out_name_len_re-4] = '\0';
// 	const char *float_end_re = "_float.txt";
// 	strcat(output_data_file_float_re, float_end_re);

// 	int out_name_len_im = strlen(output_data_file_im);
// 	char output_data_file_float_im[out_name_len_im+7];
// 	strcpy(output_data_file_float_im, output_data_file_im);
// 	output_data_file_float_im[out_name_len_im-4] = '\0';
// 	const char *float_end_im = "_float.txt";
// 	strcat(output_data_file_float_im, float_end_im);

// 	printf("Saving results to output file (%s, %s) ...\n", output_data_file_float_re, output_data_file_float_im);
// 	save_to_file(output_data_file_float_re, data_out_float_re, X_DIM*Y_DIM*NUM_CHUNKS*_TDM_CHUNKS);
// 	save_to_file(output_data_file_float_im, data_out_float_im, X_DIM*Y_DIM*NUM_CHUNKS*_TDM_CHUNKS);

// 	/*
// 	******************************************************************************
// 	* Compare C code results to expected Matlab reference script results
// 	******************************************************************************
// 	*/
// 	printf("Comparing results (%s, %s)...\n",output_data_file_re,expected_data_file_re);
// 	int cmp_res_i;
// 	T2out dummy;
// 	cmp_res_i = compare_files(dummy, expected_data_file_re, output_data_file_re, 1, output_data_size, base, err);
// 	printf("Comparing results (%s, %s)...\n",output_data_file_im,expected_data_file_im);
// 	cmp_res_i = cmp_res_i & compare_files(dummy, expected_data_file_im, output_data_file_im, 1, output_data_size,
// base, err);

// 	if (cmp_res_i != 0) {
// 		printf("Data verification completed successfully.\n");
// 	} else {
// 		printf("Data verification completed with errors.\n");
// 	}

// 	free(data_out_re);
// 	free(data_out_im);
// 	free(data_out_float_re);
// 	free(data_out_float_im);
// 	free(data_in_re);
// 	free(data_in_im);

// 	return 0;
// }

/*
* complex C = f(complex A, complex B)
*
* T2: default data type (float, ap_fixed)
* T1: storage data type of input file (float -> float, ap_fixed -> int)
*/
template <int X_DIM, int Y_DIM, int _TDM_CHUNKS, int NUM_CHUNKS, int SEL, class T1, class T2in, class T2out>
int test_func_2_1(int argc,
                  char* argv[],
                  float base,
                  float err,
                  void (*func)(T2in[X_DIM][Y_DIM][_TDM_CHUNKS],
                               T2in[X_DIM][Y_DIM][_TDM_CHUNKS],
                               T2in[X_DIM][Y_DIM][_TDM_CHUNKS],
                               T2in[X_DIM][Y_DIM][_TDM_CHUNKS],
                               T2out[X_DIM][Y_DIM][_TDM_CHUNKS],
                               T2out[X_DIM][Y_DIM][_TDM_CHUNKS])) {
    // File names
    char* input_a_data_file_re = argv[1];  //"ch_matrices_re.txt";
    char* input_a_data_file_im = argv[2];  //"ch_matrices_im.txt";
    char* input_b_data_file_re = argv[3];  //"ch_matrices_re.txt";
    char* input_b_data_file_im = argv[4];  //"ch_matrices_im.txt";
    char* output_data_file_re = argv[5];   //"results_re.txt";
    char* output_data_file_im = argv[6];   //"results_im.txt";
    char* expected_data_file_re = argv[7]; //"simulation_data_re.txt";
    char* expected_data_file_im = argv[8]; //"simulation_data_im.txt";

    // Matrix dimension
    short matrix_dimension[2] = {X_DIM, Y_DIM};
    int m = 2 * matrix_dimension[0];
    int n = 2 * matrix_dimension[1];

    // Data size
    int input_data_size = X_DIM * Y_DIM * NUM_CHUNKS * _TDM_CHUNKS;
    int output_data_size = X_DIM * Y_DIM * NUM_CHUNKS * _TDM_CHUNKS;

    // File data pointers
    T1 *data_a_in_re, *data_a_in_im, *data_b_in_re, *data_b_in_im;
    T1 *data_out_re, *data_out_im;
    float *data_out_float_re, *data_out_float_im;
    data_a_in_re = (T1*)malloc(input_data_size * sizeof(T1));
    data_a_in_im = (T1*)malloc(input_data_size * sizeof(T1));
    data_b_in_re = (T1*)malloc(input_data_size * sizeof(T1));
    data_b_in_im = (T1*)malloc(input_data_size * sizeof(T1));
    data_out_re = (T1*)malloc(output_data_size * sizeof(T1));
    data_out_im = (T1*)malloc(output_data_size * sizeof(T1));
    data_out_float_re = (float*)malloc(output_data_size * sizeof(float));
    data_out_float_im = (float*)malloc(output_data_size * sizeof(float));

    // Function arguments - data matrices
    T2in chunk_a_in_re[X_DIM][Y_DIM][_TDM_CHUNKS];
    T2in chunk_a_in_im[X_DIM][Y_DIM][_TDM_CHUNKS];
    T2in chunk_b_in_re[X_DIM][Y_DIM][_TDM_CHUNKS];
    T2in chunk_b_in_im[X_DIM][Y_DIM][_TDM_CHUNKS];
    T2out chunk_out_re[X_DIM][Y_DIM][_TDM_CHUNKS];
    T2out chunk_out_im[X_DIM][Y_DIM][_TDM_CHUNKS];

    printf("Test beginning...\n");
    /*
    ******************************************************************************
    * Load data, perfrom function, and store result
    ******************************************************************************
    */
    printf("Loading values from txt files (%s, %s, %s, %s)...\n", input_a_data_file_re, input_a_data_file_im,
           input_b_data_file_re, input_b_data_file_im);
    load_from_file(input_a_data_file_re, data_a_in_re, input_data_size);
    load_from_file(input_a_data_file_im, data_a_in_im, input_data_size);
    load_from_file(input_b_data_file_re, data_b_in_re, input_data_size);
    load_from_file(input_b_data_file_im, data_b_in_im, input_data_size);

    for (int i = 0; i < NUM_CHUNKS; i++) {
        printf("Loading input data array ...\n");
        load_input_matrix<X_DIM, Y_DIM, _TDM_CHUNKS>(i, data_a_in_re, chunk_a_in_re);
        load_input_matrix<X_DIM, Y_DIM, _TDM_CHUNKS>(i, data_a_in_im, chunk_a_in_im);
        load_input_matrix<X_DIM, Y_DIM, _TDM_CHUNKS>(i, data_b_in_re, chunk_b_in_re);
        load_input_matrix<X_DIM, Y_DIM, _TDM_CHUNKS>(i, data_b_in_im, chunk_b_in_im);

        printf("Perform main function!!\n");
        (*func)(chunk_a_in_re, chunk_a_in_im, chunk_b_in_re, chunk_b_in_im, chunk_out_re, chunk_out_im);

        printf("Store output result ... \n");
        store_output_matrix<X_DIM, Y_DIM, _TDM_CHUNKS>(i, chunk_out_re, data_out_re, data_out_float_re);
        store_output_matrix<X_DIM, Y_DIM, _TDM_CHUNKS>(i, chunk_out_im, data_out_im, data_out_float_im);
    }

    printf("Saving results to output file (%s, %s)...\n", output_data_file_re, output_data_file_im);
    save_to_file(output_data_file_re, data_out_re, X_DIM * Y_DIM * NUM_CHUNKS * _TDM_CHUNKS);
    save_to_file(output_data_file_im, data_out_im, X_DIM * Y_DIM * NUM_CHUNKS * _TDM_CHUNKS);

    int out_name_len_re = strlen(output_data_file_re);
    char output_data_file_float_re[out_name_len_re + 7];
    strcpy(output_data_file_float_re, output_data_file_re);
    output_data_file_float_re[out_name_len_re - 4] = '\0';
    const char* float_end_re = "_float.txt";
    strcat(output_data_file_float_re, float_end_re);

    int out_name_len_im = strlen(output_data_file_im);
    char output_data_file_float_im[out_name_len_im + 7];
    strcpy(output_data_file_float_im, output_data_file_im);
    output_data_file_float_im[out_name_len_im - 4] = '\0';
    const char* float_end_im = "_float.txt";
    strcat(output_data_file_float_im, float_end_im);

    printf("Saving results to output file (%s, %s) ...\n", output_data_file_float_re, output_data_file_float_im);
    save_to_file(output_data_file_float_re, data_out_float_re, X_DIM * Y_DIM * NUM_CHUNKS * _TDM_CHUNKS);
    save_to_file(output_data_file_float_im, data_out_float_im, X_DIM * Y_DIM * NUM_CHUNKS * _TDM_CHUNKS);

    /*
    ******************************************************************************
    * Compare C code results to expected Matlab reference script results
    ******************************************************************************
    */
    printf("Comparing results (%s, %s)...\n", output_data_file_re, expected_data_file_re);
    int cmp_res_i;
    T2out dummy;
    cmp_res_i = compare_files(dummy, expected_data_file_re, output_data_file_re, 1, output_data_size, base, err);
    printf("Comparing results (%s, %s)...\n", output_data_file_im, expected_data_file_im);
    cmp_res_i =
        cmp_res_i & compare_files(dummy, expected_data_file_im, output_data_file_im, 1, output_data_size, base, err);

    if (cmp_res_i != 0) {
        printf("Data verification completed successfully.\n");
    } else {
        printf("Data verification completed with errors.\n");
    }

    free(data_out_re);
    free(data_out_im);
    free(data_out_float_re);
    free(data_out_float_im);
    free(data_a_in_re);
    free(data_a_in_im);
    free(data_b_in_re);
    free(data_b_in_im);

    return 0;
}

/*
* (complex B, complex C, complex D) = f(complex A)
*
* T2: default data type (float, ap_fixed)
* T1: storage data type of input file (float -> float, ap_fixed -> int)
*/
template <int X_DIM, int Y_DIM, int _TDM_CHUNKS, int NUM_CHUNKS, int SEL, class T1, class T2in, class T2out>
int test_func_1_3(int argc,
                  char* argv[],
                  float base,
                  float err,
                  void (*func)(T2in[X_DIM][Y_DIM][_TDM_CHUNKS],
                               T2in[X_DIM][Y_DIM][_TDM_CHUNKS],
                               T2out[X_DIM][Y_DIM][_TDM_CHUNKS],
                               T2out[X_DIM][Y_DIM][_TDM_CHUNKS],
                               T2out[X_DIM][Y_DIM][_TDM_CHUNKS],
                               T2out[X_DIM][Y_DIM][_TDM_CHUNKS],
                               T2out[X_DIM][Y_DIM][_TDM_CHUNKS],
                               T2out[X_DIM][Y_DIM][_TDM_CHUNKS])) {
    // File names
    char* input_data_file_re = argv[1];       //"ch_matrices_re.txt";
    char* input_data_file_im = argv[2];       //"ch_matrices_im.txt";
    char* output_a_data_file_re = argv[3];    //"results_a_re.txt";
    char* output_a_data_file_im = argv[4];    //"results_a_im.txt";
    char* output_b_data_file_re = argv[5];    //"results_b_re.txt";
    char* output_b_data_file_im = argv[6];    //"results_b_im.txt";
    char* output_c_data_file_re = argv[7];    //"results_c_re.txt";
    char* output_c_data_file_im = argv[8];    //"results_c_im.txt";
    char* expected_a_data_file_re = argv[9];  //"simulation_a_data_re.txt";
    char* expected_a_data_file_im = argv[10]; //"simulation_a_data_im.txt";
    char* expected_b_data_file_re = argv[11]; //"simulation_b_data_re.txt";
    char* expected_b_data_file_im = argv[12]; //"simulation_b_data_im.txt";
    char* expected_c_data_file_re = argv[13]; //"simulation_c_data_re.txt";
    char* expected_c_data_file_im = argv[14]; //"simulation_c_data_im.txt";

    // Matrix dimension
    short matrix_dimension[2] = {X_DIM, Y_DIM};
    int m = 2 * matrix_dimension[0];
    int n = 2 * matrix_dimension[1];

    // Data size
    int input_data_size = X_DIM * Y_DIM * NUM_CHUNKS * _TDM_CHUNKS;
    int output_data_size = X_DIM * Y_DIM * NUM_CHUNKS * _TDM_CHUNKS;

    // File data pointers
    T1 *data_in_re, *data_in_im;
    T1 *data_a_out_re, *data_a_out_im;
    T1 *data_b_out_re, *data_b_out_im;
    T1 *data_c_out_re, *data_c_out_im;

    float *data_a_out_float_re, *data_a_out_float_im;
    float *data_b_out_float_re, *data_b_out_float_im;
    float *data_c_out_float_re, *data_c_out_float_im;

    data_in_re = (T1*)malloc(input_data_size * sizeof(T1));
    data_in_im = (T1*)malloc(input_data_size * sizeof(T1));
    data_a_out_re = (T1*)malloc(output_data_size * sizeof(T1));
    data_a_out_im = (T1*)malloc(output_data_size * sizeof(T1));
    data_b_out_re = (T1*)malloc(output_data_size * sizeof(T1));
    data_b_out_im = (T1*)malloc(output_data_size * sizeof(T1));
    data_c_out_re = (T1*)malloc(output_data_size * sizeof(T1));
    data_c_out_im = (T1*)malloc(output_data_size * sizeof(T1));
    data_a_out_float_re = (float*)malloc(output_data_size * sizeof(float));
    data_a_out_float_im = (float*)malloc(output_data_size * sizeof(float));
    data_b_out_float_re = (float*)malloc(output_data_size * sizeof(float));
    data_b_out_float_im = (float*)malloc(output_data_size * sizeof(float));
    data_c_out_float_re = (float*)malloc(output_data_size * sizeof(float));
    data_c_out_float_im = (float*)malloc(output_data_size * sizeof(float));

    // Function arguments - data matrices
    T2in chunk_in_re[X_DIM][Y_DIM][_TDM_CHUNKS];
    T2in chunk_in_im[X_DIM][Y_DIM][_TDM_CHUNKS];
    T2out chunk_a_out_re[X_DIM][Y_DIM][_TDM_CHUNKS];
    T2out chunk_a_out_im[X_DIM][Y_DIM][_TDM_CHUNKS];
    T2out chunk_b_out_re[X_DIM][Y_DIM][_TDM_CHUNKS];
    T2out chunk_b_out_im[X_DIM][Y_DIM][_TDM_CHUNKS];
    T2out chunk_c_out_re[X_DIM][Y_DIM][_TDM_CHUNKS];
    T2out chunk_c_out_im[X_DIM][Y_DIM][_TDM_CHUNKS];

    printf("Test beginning...\n");
    /*
    ******************************************************************************
    * Load data, perfrom function, and store result
    ******************************************************************************
    */
    printf("Loading values from txt files (%s, %s)...\n", input_data_file_re, input_data_file_im);
    load_from_file(input_data_file_re, data_in_re, input_data_size);
    load_from_file(input_data_file_im, data_in_im, input_data_size);

    for (int i = 0; i < NUM_CHUNKS; i++) {
        printf("Loading input data array ...\n");
        load_input_matrix<X_DIM, Y_DIM, _TDM_CHUNKS>(i, data_in_re, chunk_in_re);
        load_input_matrix<X_DIM, Y_DIM, _TDM_CHUNKS>(i, data_in_im, chunk_in_im);

        printf("Perform main function!!\n");
        (*func)(chunk_in_re, chunk_in_im, chunk_a_out_re, chunk_a_out_im, chunk_b_out_re, chunk_b_out_im,
                chunk_c_out_re, chunk_c_out_im);

        printf("Store output result ... \n");
        store_output_matrix<X_DIM, Y_DIM, _TDM_CHUNKS>(i, chunk_a_out_re, data_a_out_re, data_a_out_float_re);
        store_output_matrix<X_DIM, Y_DIM, _TDM_CHUNKS>(i, chunk_a_out_im, data_a_out_im, data_a_out_float_im);
        store_output_matrix<X_DIM, Y_DIM, _TDM_CHUNKS>(i, chunk_b_out_re, data_b_out_re, data_b_out_float_re);
        store_output_matrix<X_DIM, Y_DIM, _TDM_CHUNKS>(i, chunk_b_out_im, data_b_out_im, data_b_out_float_im);
        store_output_matrix<X_DIM, Y_DIM, _TDM_CHUNKS>(i, chunk_c_out_re, data_c_out_re, data_c_out_float_re);
        store_output_matrix<X_DIM, Y_DIM, _TDM_CHUNKS>(i, chunk_c_out_im, data_c_out_im, data_c_out_float_im);
    }

    printf("Saving results to output file (%s, %s, %s, %s, %s, %s)...\n", output_a_data_file_re, output_a_data_file_im,
           output_b_data_file_re, output_b_data_file_im, output_c_data_file_re, output_c_data_file_im);
    save_to_file(output_a_data_file_re, data_a_out_re, X_DIM * Y_DIM * NUM_CHUNKS * _TDM_CHUNKS);
    save_to_file(output_a_data_file_im, data_a_out_im, X_DIM * Y_DIM * NUM_CHUNKS * _TDM_CHUNKS);
    save_to_file(output_b_data_file_re, data_b_out_re, X_DIM * Y_DIM * NUM_CHUNKS * _TDM_CHUNKS);
    save_to_file(output_b_data_file_im, data_b_out_im, X_DIM * Y_DIM * NUM_CHUNKS * _TDM_CHUNKS);
    save_to_file(output_c_data_file_re, data_c_out_re, X_DIM * Y_DIM * NUM_CHUNKS * _TDM_CHUNKS);
    save_to_file(output_c_data_file_im, data_c_out_im, X_DIM * Y_DIM * NUM_CHUNKS * _TDM_CHUNKS);

    int filename_len;
    const char* float_end = "_float.txt";

    filename_len = strlen(output_a_data_file_re);
    char* output_a_data_file_float_re = (char*)malloc((filename_len + 7) * sizeof(char));
    strcpy(output_a_data_file_float_re, output_a_data_file_re);
    output_a_data_file_float_re[filename_len - 4] = '\0';
    strcat(output_a_data_file_float_re, float_end);

    filename_len = strlen(output_a_data_file_im);
    char* output_a_data_file_float_im = (char*)malloc((filename_len + 7) * sizeof(char));
    strcpy(output_a_data_file_float_im, output_a_data_file_im);
    output_a_data_file_float_im[filename_len - 4] = '\0';
    strcat(output_a_data_file_float_im, float_end);

    filename_len = strlen(output_b_data_file_re);
    char* output_b_data_file_float_re = (char*)malloc((filename_len + 7) * sizeof(char));
    strcpy(output_b_data_file_float_re, output_b_data_file_re);
    output_b_data_file_float_re[filename_len - 4] = '\0';
    strcat(output_b_data_file_float_re, float_end);

    filename_len = strlen(output_b_data_file_im);
    char* output_b_data_file_float_im = (char*)malloc((filename_len + 7) * sizeof(char));
    strcpy(output_b_data_file_float_im, output_b_data_file_im);
    output_b_data_file_float_im[filename_len - 4] = '\0';
    strcat(output_b_data_file_float_im, float_end);

    filename_len = strlen(output_c_data_file_re);
    char* output_c_data_file_float_re = (char*)malloc((filename_len + 7) * sizeof(char));
    strcpy(output_c_data_file_float_re, output_c_data_file_re);
    output_c_data_file_float_re[filename_len - 4] = '\0';
    strcat(output_c_data_file_float_re, float_end);

    filename_len = strlen(output_c_data_file_im);
    char* output_c_data_file_float_im = (char*)malloc((filename_len + 7) * sizeof(char));
    strcpy(output_c_data_file_float_im, output_c_data_file_im);
    output_c_data_file_float_im[filename_len - 4] = '\0';
    strcat(output_c_data_file_float_im, float_end);

    printf("Saving results to output file (%s, %s, %s, %s, %s, %s) ...\n", output_a_data_file_float_re,
           output_a_data_file_float_im, output_b_data_file_float_re, output_b_data_file_float_im,
           output_c_data_file_float_re, output_c_data_file_float_im);
    save_to_file(output_a_data_file_float_re, data_a_out_float_re, X_DIM * Y_DIM * NUM_CHUNKS * _TDM_CHUNKS);
    save_to_file(output_a_data_file_float_im, data_a_out_float_im, X_DIM * Y_DIM * NUM_CHUNKS * _TDM_CHUNKS);
    save_to_file(output_b_data_file_float_re, data_b_out_float_re, X_DIM * Y_DIM * NUM_CHUNKS * _TDM_CHUNKS);
    save_to_file(output_b_data_file_float_im, data_b_out_float_im, X_DIM * Y_DIM * NUM_CHUNKS * _TDM_CHUNKS);
    save_to_file(output_c_data_file_float_re, data_c_out_float_re, X_DIM * Y_DIM * NUM_CHUNKS * _TDM_CHUNKS);
    save_to_file(output_c_data_file_float_im, data_c_out_float_im, X_DIM * Y_DIM * NUM_CHUNKS * _TDM_CHUNKS);

    /*
    ******************************************************************************
    * Compare C code results to expected Matlab reference script results
    ******************************************************************************
    */
    printf("Comparing results (%s - %s, %s - %s, %s - %s, %s - %s, %s - %s, %s - %s)...\n", output_a_data_file_re,
           expected_a_data_file_re, output_a_data_file_im, expected_a_data_file_im, output_b_data_file_re,
           expected_b_data_file_re, output_a_data_file_im, expected_b_data_file_im, output_c_data_file_re,
           expected_c_data_file_re, output_c_data_file_im, expected_c_data_file_im);
    int cmp_res_i;
    T2out dummy;
    cmp_res_i = compare_files(dummy, expected_a_data_file_re, output_a_data_file_re, 1, output_data_size, base, err);
    cmp_res_i = cmp_res_i &
                compare_files(dummy, expected_a_data_file_im, output_a_data_file_im, 1, output_data_size, base, err);
    cmp_res_i = cmp_res_i &
                compare_files(dummy, expected_b_data_file_re, output_b_data_file_re, 1, output_data_size, base, err);
    cmp_res_i = cmp_res_i &
                compare_files(dummy, expected_b_data_file_im, output_b_data_file_im, 1, output_data_size, base, err);
    cmp_res_i = cmp_res_i &
                compare_files(dummy, expected_c_data_file_re, output_c_data_file_re, 1, output_data_size, base, err);
    cmp_res_i = cmp_res_i &
                compare_files(dummy, expected_c_data_file_im, output_c_data_file_im, 1, output_data_size, base, err);

    if (cmp_res_i != 0) {
        printf("Data verification completed successfully.\n");
    } else {
        printf("Data verification completed with errors.\n");
    }

    free(data_a_out_re);
    free(data_a_out_im);
    free(data_a_out_float_re);
    free(data_a_out_float_im);
    free(data_b_out_re);
    free(data_b_out_im);
    free(data_b_out_float_re);
    free(data_b_out_float_im);
    free(data_c_out_re);
    free(data_c_out_im);
    free(data_c_out_float_re);
    free(data_c_out_float_im);
    free(data_in_re);
    free(data_in_im);

    return 0;
}

// /*
// * (complex B, complex C, complex D) = f(complex A)
// *
// * T2: default data type (float, ap_fixed)
// * T1: storage data type of input file (float -> float, ap_fixed -> int)
// */
// template<int X_DIM, int Y_DIM, int _TDM_CHUNKS, int NUM_CHUNKS, int SEL,
// 	class T1, class T2in, class T2out>
// int test_func_1_3_c(
// 	int argc, char* argv[], float base, float err,
// 	void (*func)(x_complex<T2in>[X_DIM][Y_DIM][_TDM_CHUNKS],
// 		     x_complex<T2out>[X_DIM][Y_DIM][_TDM_CHUNKS],
// 		     x_complex<T2out>[X_DIM][Y_DIM][_TDM_CHUNKS],
// 		     x_complex<T2out>[X_DIM][Y_DIM][_TDM_CHUNKS]))
// {
// 	// File names
// 	char *input_data_file_re      = argv[1];//"ch_matrices_re.txt";
// 	char *input_data_file_im      = argv[2];//"ch_matrices_im.txt";
// 	char *output_a_data_file_re   = argv[3];//"results_a_re.txt";
// 	char *output_a_data_file_im   = argv[4];//"results_a_im.txt";
// 	char *output_b_data_file_re   = argv[5];//"results_b_re.txt";
// 	char *output_b_data_file_im   = argv[6];//"results_b_im.txt";
// 	char *output_c_data_file_re   = argv[7];//"results_c_re.txt";
// 	char *output_c_data_file_im   = argv[8];//"results_c_im.txt";
// 	char *expected_a_data_file_re = argv[9];//"simulation_a_data_re.txt";
// 	char *expected_a_data_file_im = argv[10];//"simulation_a_data_im.txt";
// 	char *expected_b_data_file_re = argv[11];//"simulation_b_data_re.txt";
// 	char *expected_b_data_file_im = argv[12];//"simulation_b_data_im.txt";
// 	char *expected_c_data_file_re = argv[13];//"simulation_c_data_re.txt";
// 	char *expected_c_data_file_im = argv[14];//"simulation_c_data_im.txt";

// 	// Matrix dimension
// 	short matrix_dimension[2]  = {X_DIM,Y_DIM};
// 	int m = 2*matrix_dimension[0];
// 	int n = 2*matrix_dimension[1];

// 	// Data size
// 	int input_data_size        = X_DIM*Y_DIM*NUM_CHUNKS*_TDM_CHUNKS;
// 	int output_data_size       = X_DIM*Y_DIM*NUM_CHUNKS*_TDM_CHUNKS;

// 	// File data pointers
// 	T1 *data_in_re, *data_in_im;
// 	T1 *data_a_out_re, *data_a_out_im;
// 	T1 *data_b_out_re, *data_b_out_im;
// 	T1 *data_c_out_re, *data_c_out_im;

// 	float *data_a_out_float_re, *data_a_out_float_im;
// 	float *data_b_out_float_re, *data_b_out_float_im;
// 	float *data_c_out_float_re, *data_c_out_float_im;

// 	data_in_re             = (T1*)malloc(input_data_size * sizeof(T1));
// 	data_in_im             = (T1*)malloc(input_data_size * sizeof(T1));
// 	data_a_out_re          = (T1*)malloc(output_data_size * sizeof(T1));
// 	data_a_out_im          = (T1*)malloc(output_data_size * sizeof(T1));
// 	data_b_out_re          = (T1*)malloc(output_data_size * sizeof(T1));
// 	data_b_out_im          = (T1*)malloc(output_data_size * sizeof(T1));
// 	data_c_out_re          = (T1*)malloc(output_data_size * sizeof(T1));
// 	data_c_out_im          = (T1*)malloc(output_data_size * sizeof(T1));
// 	data_a_out_float_re    = (float*)malloc(output_data_size * sizeof(float));
// 	data_a_out_float_im    = (float*)malloc(output_data_size * sizeof(float));
// 	data_b_out_float_re    = (float*)malloc(output_data_size * sizeof(float));
// 	data_b_out_float_im    = (float*)malloc(output_data_size * sizeof(float));
// 	data_c_out_float_re    = (float*)malloc(output_data_size * sizeof(float));
// 	data_c_out_float_im    = (float*)malloc(output_data_size * sizeof(float));

// 	// Function arguments - data matrices
// 	T2in chunk_in_re[X_DIM][Y_DIM][_TDM_CHUNKS];
// 	T2in chunk_in_im[X_DIM][Y_DIM][_TDM_CHUNKS];
// 	T2out chunk_a_out_re[X_DIM][Y_DIM][_TDM_CHUNKS];
// 	T2out chunk_a_out_im[X_DIM][Y_DIM][_TDM_CHUNKS];
// 	T2out chunk_b_out_re[X_DIM][Y_DIM][_TDM_CHUNKS];
// 	T2out chunk_b_out_im[X_DIM][Y_DIM][_TDM_CHUNKS];
// 	T2out chunk_c_out_re[X_DIM][Y_DIM][_TDM_CHUNKS];
// 	T2out chunk_c_out_im[X_DIM][Y_DIM][_TDM_CHUNKS];

//   printf("Test beginning...\n");
// 	/*
// 	******************************************************************************
// 	* Load data, perfrom function, and store result
// 	******************************************************************************
// 	*/
// 	printf("Loading values from txt files (%s, %s)...\n",input_data_file_re, input_data_file_im);
// 	load_from_file(input_data_file_re, data_in_re, input_data_size);
// 	load_from_file(input_data_file_im, data_in_im, input_data_size);

// 	for(int i = 0; i < NUM_CHUNKS; i++)
// 	{
// 		printf("Loading input data array ...\n");
// 		load_input_matrix<X_DIM,Y_DIM,_TDM_CHUNKS>(i, data_in_re,chunk_in_re);
// 		load_input_matrix<X_DIM,Y_DIM,_TDM_CHUNKS>(i, data_in_im,chunk_in_im);

// 		x_complex<T2in> in_a[X_DIM][Y_DIM][_TDM_CHUNKS];
// 		x_complex<T2out> out_s[X_DIM][Y_DIM][_TDM_CHUNKS];
// 		x_complex<T2out> out_u[X_DIM][Y_DIM][_TDM_CHUNKS];
// 		x_complex<T2out> out_v[X_DIM][Y_DIM][_TDM_CHUNKS];
// 		for(int ii=0; ii<X_DIM; ii++) {
// 			for(int jj=0; jj<Y_DIM; jj++) {
// 				in_a[ii][jj][0].real(chunk_in_re[ii][jj][0]);
// 				in_a[ii][jj][0].imag(chunk_in_im[ii][jj][0]);
// 			}
// 		}

// 		printf("Perform main function!!\n");
// 		(*func)(in_a, out_s, out_u, out_v);

// 		for(int ii=0; ii<X_DIM; ii++) {
// 			for(int jj=0; jj<Y_DIM; jj++) {
// 				chunk_a_out_re[ii][jj][0] = out_s[ii][jj][0].real();
// 				chunk_a_out_im[ii][jj][0] = out_s[ii][jj][0].imag();
// 				chunk_b_out_re[ii][jj][0] = out_u[ii][jj][0].real();
// 				chunk_b_out_im[ii][jj][0] = out_u[ii][jj][0].imag();
// 				chunk_c_out_re[ii][jj][0] = out_v[ii][jj][0].real();
// 				chunk_c_out_im[ii][jj][0] = out_v[ii][jj][0].imag();
// 			}
// 		}

// 		printf("Store output result ... \n");
// 		store_output_matrix<X_DIM,Y_DIM,_TDM_CHUNKS>(i, chunk_a_out_re, data_a_out_re, data_a_out_float_re);
// 		store_output_matrix<X_DIM,Y_DIM,_TDM_CHUNKS>(i, chunk_a_out_im, data_a_out_im, data_a_out_float_im);
// 		store_output_matrix<X_DIM,Y_DIM,_TDM_CHUNKS>(i, chunk_b_out_re, data_b_out_re, data_b_out_float_re);
// 		store_output_matrix<X_DIM,Y_DIM,_TDM_CHUNKS>(i, chunk_b_out_im, data_b_out_im, data_b_out_float_im);
// 		store_output_matrix<X_DIM,Y_DIM,_TDM_CHUNKS>(i, chunk_c_out_re, data_c_out_re, data_c_out_float_re);
// 		store_output_matrix<X_DIM,Y_DIM,_TDM_CHUNKS>(i, chunk_c_out_im, data_c_out_im, data_c_out_float_im);
// 	}

// 	printf("Saving results to output file (%s, %s, %s, %s, %s, %s)...\n",
// 				output_a_data_file_re, output_a_data_file_im,
// 				output_b_data_file_re, output_b_data_file_im,
// 				output_c_data_file_re, output_c_data_file_im);
// 	save_to_file(output_a_data_file_re, data_a_out_re, X_DIM*Y_DIM*NUM_CHUNKS*_TDM_CHUNKS);
// 	save_to_file(output_a_data_file_im, data_a_out_im, X_DIM*Y_DIM*NUM_CHUNKS*_TDM_CHUNKS);
// 	save_to_file(output_b_data_file_re, data_b_out_re, X_DIM*Y_DIM*NUM_CHUNKS*_TDM_CHUNKS);
// 	save_to_file(output_b_data_file_im, data_b_out_im, X_DIM*Y_DIM*NUM_CHUNKS*_TDM_CHUNKS);
// 	save_to_file(output_c_data_file_re, data_c_out_re, X_DIM*Y_DIM*NUM_CHUNKS*_TDM_CHUNKS);
// 	save_to_file(output_c_data_file_im, data_c_out_im, X_DIM*Y_DIM*NUM_CHUNKS*_TDM_CHUNKS);

// 	int filename_len;
// 	const char *float_end = "_float.txt";

// 	filename_len  = strlen(output_a_data_file_re);
// 	char *output_a_data_file_float_re = (char*)malloc((filename_len+7)*sizeof(char));
// 	strcpy(output_a_data_file_float_re, output_a_data_file_re);
// 	output_a_data_file_float_re[filename_len-4] = '\0';
// 	strcat(output_a_data_file_float_re, float_end);

// 	filename_len  = strlen(output_a_data_file_im);
// 	char *output_a_data_file_float_im = (char*)malloc((filename_len+7)*sizeof(char));
// 	strcpy(output_a_data_file_float_im, output_a_data_file_im);
// 	output_a_data_file_float_im[filename_len-4] = '\0';
// 	strcat(output_a_data_file_float_im, float_end);

// 	filename_len  = strlen(output_b_data_file_re);
// 	char *output_b_data_file_float_re = (char*)malloc((filename_len+7)*sizeof(char));
// 	strcpy(output_b_data_file_float_re, output_b_data_file_re);
// 	output_b_data_file_float_re[filename_len-4] = '\0';
// 	strcat(output_b_data_file_float_re, float_end);

// 	filename_len  = strlen(output_b_data_file_im);
// 	char *output_b_data_file_float_im = (char*)malloc((filename_len+7)*sizeof(char));
// 	strcpy(output_b_data_file_float_im, output_b_data_file_im);
// 	output_b_data_file_float_im[filename_len-4] = '\0';
// 	strcat(output_b_data_file_float_im, float_end);

// 	filename_len  = strlen(output_c_data_file_re);
// 	char *output_c_data_file_float_re = (char*)malloc((filename_len+7)*sizeof(char));
// 	strcpy(output_c_data_file_float_re, output_c_data_file_re);
// 	output_c_data_file_float_re[filename_len-4] = '\0';
// 	strcat(output_c_data_file_float_re, float_end);

// 	filename_len  = strlen(output_c_data_file_im);
// 	char *output_c_data_file_float_im = (char*)malloc((filename_len+7)*sizeof(char));
// 	strcpy(output_c_data_file_float_im, output_c_data_file_im);
// 	output_c_data_file_float_im[filename_len-4] = '\0';
// 	strcat(output_c_data_file_float_im, float_end);

// 	printf("Saving results to output file (%s, %s, %s, %s, %s, %s) ...\n",
// 			output_a_data_file_float_re, output_a_data_file_float_im,
// 			output_b_data_file_float_re, output_b_data_file_float_im,
// 			output_c_data_file_float_re, output_c_data_file_float_im);
// 	save_to_file(output_a_data_file_float_re, data_a_out_float_re, X_DIM*Y_DIM*NUM_CHUNKS*_TDM_CHUNKS);
// 	save_to_file(output_a_data_file_float_im, data_a_out_float_im, X_DIM*Y_DIM*NUM_CHUNKS*_TDM_CHUNKS);
// 	save_to_file(output_b_data_file_float_re, data_b_out_float_re, X_DIM*Y_DIM*NUM_CHUNKS*_TDM_CHUNKS);
// 	save_to_file(output_b_data_file_float_im, data_b_out_float_im, X_DIM*Y_DIM*NUM_CHUNKS*_TDM_CHUNKS);
// 	save_to_file(output_c_data_file_float_re, data_c_out_float_re, X_DIM*Y_DIM*NUM_CHUNKS*_TDM_CHUNKS);
// 	save_to_file(output_c_data_file_float_im, data_c_out_float_im, X_DIM*Y_DIM*NUM_CHUNKS*_TDM_CHUNKS);

// 	/*
// 	******************************************************************************
// 	* Compare C code results to expected Matlab reference script results
// 	******************************************************************************
// 	*/
// 	printf("Comparing results (%s - %s, %s - %s, %s - %s, %s - %s, %s - %s, %s - %s)...\n",
// 		output_a_data_file_re,expected_a_data_file_re,
// 		output_a_data_file_im,expected_a_data_file_im,
// 		output_b_data_file_re,expected_b_data_file_re,
// 		output_a_data_file_im,expected_b_data_file_im,
// 		output_c_data_file_re,expected_c_data_file_re,
// 		output_c_data_file_im,expected_c_data_file_im);
// 	int cmp_res_i;
// 	T2out dummy;
// 	cmp_res_i = compare_files(dummy, expected_a_data_file_re, output_a_data_file_re, 1, output_data_size, base,
// err);
// 	cmp_res_i = cmp_res_i & compare_files(dummy, expected_a_data_file_im, output_a_data_file_im, 1,
// output_data_size, base, err);
// 	cmp_res_i = cmp_res_i & compare_files(dummy, expected_b_data_file_re, output_b_data_file_re, 1,
// output_data_size, base, err);
// 	cmp_res_i = cmp_res_i & compare_files(dummy, expected_b_data_file_im, output_b_data_file_im, 1,
// output_data_size, base, err);
// 	cmp_res_i = cmp_res_i & compare_files(dummy, expected_c_data_file_re, output_c_data_file_re, 1,
// output_data_size, base, err);
// 	cmp_res_i = cmp_res_i & compare_files(dummy, expected_c_data_file_im, output_c_data_file_im, 1,
// output_data_size, base, err);

// 	if (cmp_res_i != 0) {
// 		printf("Data verification completed successfully.\n");
// 	} else {
// 		printf("Data verification completed with errors.\n");
// 	}

// 	free(data_a_out_re);
// 	free(data_a_out_im);
// 	free(data_a_out_float_re);
// 	free(data_a_out_float_im);
// 	free(data_b_out_re);
// 	free(data_b_out_im);
// 	free(data_b_out_float_re);
// 	free(data_b_out_float_im);
// 	free(data_c_out_re);
// 	free(data_c_out_im);
// 	free(data_c_out_float_re);
// 	free(data_c_out_float_im);
// 	free(data_in_re);
// 	free(data_in_im);

// 	return 0;
// }

// d1(base)   = d1Int x 2^d1Exp
// d2(target) = d2Int x 2^d2Exp
// h          =         2^hExp ( = d1Exp-1)
// int
// calcULP(
// 	float d1, // base
// 	float d2) // target
// {
//     int d1Exp, d2Exp, hExp; //normalized exponents
//     //	int
//     hExp = d1Exp - 1;

// 	int d1_exp = 0;
// 	int d2_exp = 0;
// 	int hS_exp = 0;

// 	if(d1Exp >= 0) {
// 		d1_exp += d1Exp;
// 	} else {
// 		d2_exp -= d1Exp;
// 		hS_exp -= d1Exp;
// 	}

// 	if(d2Exp >= 0) {
// 		d2_exp += d2Exp;
// 	} else {
// 		d1_exp -= d2Exp;
// 		hS_exp -= d2Exp;
// 	}

// 	// Adjust for half ulp exponent
// 	if(hExp >= 0) {
// 		hS_exp += hExp;
// 	} else {
// 		d1_exp -= hExp;
// 		d2_exp -= hExp;
// 	}

// 	// Remove common power of two factor from all three scaled values
// 	int common_exp2 = min(d1_exp, min(d2_exp, hS_exp));
// 	d1_exp -= common_exp2;
// 	d2_exp -= common_exp2;
// 	hS_exp -= common_exp2;

// Recreate d1,d2 and subtract d1, d2 and compare?
//}

template <int W>
uint64_t calcULP(ap_int<W> A, ap_int<W> B) {
    ap_int<W + 1> diff = A - B;
    if (diff < 0) diff = -diff;
    if (diff > 0x7FFFFFFFFFFFFFFFULL) diff = 0x7FFFFFFFFFFFFFFFULL;
    ap_uint<64> intDiff = diff;
    return intDiff;
}

template <int W>
uint64_t calcULP(ap_uint<W> A, ap_uint<W> B) {
    ap_uint<W + 1> diff = A - B;
    if (diff < 0) diff = -diff;
    if (diff > 0x7FFFFFFFFFFFFFFFULL) diff = 0x7FFFFFFFFFFFFFFFULL;
    ap_uint<64> intDiff = diff;
    return intDiff;
}

template <int W, int I, ap_q_mode Q, ap_o_mode O>
uint64_t calcULP(ap_fixed<W, I, Q, O> A, ap_fixed<W, I, Q, O> B) {
    ap_int<W> A_bits, B_bits;
    A_bits(W - 1, 0) = A(W - 1, 0);
    B_bits(W - 1, 0) = B(W - 1, 0);

    ap_int<W + 1> diff = A_bits - B_bits;
    if (diff < 0) diff = -diff;
    if (diff > 0x7FFFFFFFFFFFFFFFULL) diff = 0x7FFFFFFFFFFFFFFFULL;
    ap_uint<64> intDiff = diff;
    return intDiff;
}

template <int W, int I, ap_q_mode Q, ap_o_mode O>
uint64_t calcULP(ap_ufixed<W, I, Q, O> A, ap_ufixed<W, I, Q, O> B) {
    ap_int<W + 1> A_bits = 0, B_bits = 0;
    A_bits(W - 1, 0) = A(W - 1, 0);
    B_bits(W - 1, 0) = B(W - 1, 0);

    ap_int<W + 1> diff = A_bits - B_bits;
    if (diff < 0) diff = -diff;
    if (diff > 0x7FFFFFFFFFFFFFFFULL) diff = 0x7FFFFFFFFFFFFFFFULL;
    ap_uint<64> intDiff = diff;
    return intDiff;
}

#ifdef X_AP_FLOAT_H
template <int W, int I, int E_W, bool N>
uint64_t calcULP(ap_float<W, I, E_W, N> a, ap_float<W, I, E_W, N> b) {
    if (a.is_nan() && b.is_nan()) return 0;
    if (a.is_infinity() && b.is_infinity()) return 0;

    bool a_big = a.exp > b.exp;
    ap_uint<E_W + 1> shift = a_big ? (a.exp - b.exp) : (b.exp - a.exp);
    ap_fixed<W, I> temp1 = a.mant, temp2 = b.mant;
    if (!a_big) {
        temp1 = b.mant;
        temp2 = a.mant;
    }
    temp2 = temp2 >> shift;
    return calcULP(temp1, temp2);
}
#endif

template <typename T1, typename T2>
uint64_t calcULP(T1 A, T2 B) {
    return std::fabs(double(A - B));
}

// static
// uint64_t calcULP(char A, char B) {
//    return abs(A-B);
//}

#ifndef AESL_SYN
static uint64_t calcULP(half A, half B) {
    if (detail::isnan(A) && detail::isnan(B)) return 0;
    unsigned short A_bits = A.get_bits();
    unsigned short B_bits = B.get_bits();
    int aInt = (detail::signbit(A) ? (0x8000 - A_bits) : A_bits);
    int bInt = (detail::signbit(B) ? (0x8000 - B_bits) : B_bits);
    return abs(aInt - bInt);
}
#endif

static uint64_t calcULP(float A, float B) {
    class fp_struct<float> d1s(A), d2s(B);

    dumpSingle(A, d1s);
    dumpSingle(B, d2s);

    if (std::isnan(A) && std::isnan(B)) return 0;
    int32_t aInt = d1s.data();
    // Make aInt lexicographically ordered as a twos-complement int
    if (std::signbit(A)) aInt = 0x80000000 - aInt;
    // Make bInt lexicographically ordered as a twos-complement int
    int32_t bInt = d2s.data();
    if (std::signbit(B)) bInt = 0x80000000 - bInt;
    int32_t intDiff = abs(aInt - bInt);
    return intDiff;
}

static uint64_t calcULP(double A, // base
                        double B) // target
{
    class fp_struct<double> d1s(A);
    class fp_struct<double> d2s(B);

    dumpDouble(A, d1s);
    dumpDouble(B, d2s);

    if (std::isnan(A) && std::isnan(B)) return 0;
    // Make sure maxUlps is non-negative and small enough that the
    // default NAN won't compare as equal to anything.
    // assert(maxUlps > 0 && maxUlps < 4 * 1024 * 1024);
    int64_t aInt = d1s.data();
    // Make aInt lexicographically ordered as a twos-complement int
    if (A < 0.0) aInt = 0x8000000000000000LL - aInt;
    // Make bInt lexicographically ordered as a twos-complement int
    int64_t bInt = d2s.data();
    if (B < 0.0) bInt = 0x8000000000000000LL - bInt;
    int64_t intDiff = aInt - bInt;
    if (intDiff < 0) intDiff = -intDiff;
    // if (intDiff <= maxUlps)
    //  return true;
    // return false;
    return intDiff;
}

hls::x_complex<int> calcULP(hls::x_complex<float> A,   // base
                            hls::x_complex<float> B) { // target

    hls::x_complex<int> ret;
    ret.real(calcULP(A.real(), B.real()));
    ret.imag(calcULP(A.imag(), B.imag()));
    return ret;
}

hls::x_complex<int64_t> calcULP(hls::x_complex<double> A,   // base
                                hls::x_complex<double> B) { // target

    hls::x_complex<int64_t> ret;
    ret.real(calcULP(A.real(), B.real()));
    ret.imag(calcULP(A.imag(), B.imag()));
    return ret;
}

template <int W, int I, ap_q_mode Q, ap_o_mode O, int N>
hls::x_complex<int64_t> calcULP(hls::x_complex<ap_fixed<W, I, Q, O, N> > A,   // base
                                hls::x_complex<ap_fixed<W, I, Q, O, N> > B) { // target

    hls::x_complex<int64_t> ret;
    ret.real(calcULP(A.real(), B.real()));
    ret.imag(calcULP(A.imag(), B.imag()));
    return ret;
};
////////////////////////////////////////////////////////////////////////////
std::complex<int> calcULP(std::complex<float> A,   // base
                          std::complex<float> B) { // target

    std::complex<int> ret;
    ret.real(calcULP(A.real(), B.real()));
    ret.imag(calcULP(A.imag(), B.imag()));
    return ret;
}

std::complex<int64_t> calcULP(std::complex<double> A,   // base
                              std::complex<double> B) { // target

    std::complex<int64_t> ret;
    ret.real(calcULP(A.real(), B.real()));
    ret.imag(calcULP(A.imag(), B.imag()));
    return ret;
}

template <int W, int I, ap_q_mode Q, ap_o_mode O, int N>
std::complex<int64_t> calcULP(std::complex<ap_fixed<W, I, Q, O, N> > A,   // base
                              std::complex<ap_fixed<W, I, Q, O, N> > B) { // target

    std::complex<int64_t> ret;
    ret.real(calcULP(A.real(), B.real()));
    ret.imag(calcULP(A.imag(), B.imag()));
    return ret;
};
////////////////////////////////////////////////////////////////////////////
// ---------------------------------------------------------------------------------------------
// Compare this ULP error to the max so far, and update the max if this is greater.
// ---------------------------------------------------------------------------------------------
// These funcitons are needed to handle the fact that complex numbers need each field tested
// individually, so "value > *max_ulp_error" won't work
//
void update_max_ulp_error(unsigned value, double* max_ulp_error) {
    if (value > *max_ulp_error) {
        *max_ulp_error = (double)value;
    }
}

void update_max_ulp_error(hls::x_complex<unsigned> value, double* max_ulp_error) {
    update_max_ulp_error(value.real(), max_ulp_error);
    update_max_ulp_error(value.imag(), max_ulp_error);
}
void update_max_ulp_error(std::complex<unsigned> value, double* max_ulp_error) {
    update_max_ulp_error(value.real(), max_ulp_error);
    update_max_ulp_error(value.imag(), max_ulp_error);
}

// Usable AlmostEqual function
static bool AlmostEqual2sComplement(float A, float B, int maxUlps) {
// Make sure maxUlps is non-negative and small enough that the
// default NAN won't compare as equal to anything.
#ifndef __SYNTHESIS__
    assert(maxUlps > 0 && maxUlps < 4 * 1024 * 1024);
#endif
    class fp_struct<double> d1s(A);
    class fp_struct<double> d2s(B);

    int aInt = d1s.data();
    // Make aInt lexicographically ordered as a twos-complement int
    if (aInt < 0) aInt = 0x80000000 - aInt;
    // Make bInt lexicographically ordered as a twos-complement int
    int bInt = d2s.data();
    if (bInt < 0) bInt = 0x80000000 - bInt;
    int intDiff = abs(aInt - bInt);
    if (intDiff <= maxUlps) return true;
    return false;
}

static int rand8() {
    assert(RAND_MAX > 0xFF);
    return rand() & 0xFF;
}

static int rand32() {
    return (rand8() << 24) | (rand8() << 16) | (rand8() << 8) | (rand8());
}

#define RS_SCALE (1.0 / (1.0 + RAND_MAX))
static double drand() {
    double d;
    do {
        d = (((rand() * RS_SCALE) + rand()) * RS_SCALE + rand()) * RS_SCALE;
    } while (d >= 1); /* Round off */
    return d;
}

static const uint64_t double_corner_cases[] = {
    0x0000000000000000LL, // +zero
    0x0000000000000001LL, // positive denormalized real
    0x7FF0000000000000LL, // +infinity
    0x7FF0000000000001LL, // SNaN
    0x7FFFFFFFFFFFFFFFLL, // QNaN
    0x8000000000000000LL, // -zero
    0x8000000000000001LL, // Negative denormalized real
    0xFFF0000000000000LL, // -infinity
    0xFFF0000000000001LL, // -SNaN
    0xFFFFFFFFFFFFFFFFLL, // -QNaN
    0x7FEFFFFFFFFFFFFFLL, // positive max
    0xFFEFFFFFFFFFFFFFLL, // negative max
    0x3FE0000000000000LL, // normal positive, 1.0x2^-1
    0xBFE0000000000000LL, // normal negative, -1.0x2^-1
};

static const uint32_t float_corner_cases[] = {
    0x00000000, // +zero
    0x00000001, // positive denormalized real
    0x007FFFFF, // largest positive denormalized real
    0x00800000, // smallest positive normalized real
    0x7F800000, // +infinity
    0x7F800001, // SNaN
    0x7FFFFFFF, // QNaN
    0x80000000, // -zero
    0x80000001, // Negative denormalized real
    0x807FFFFF, // largest positive denormalized real
    0x80800000, // smallest positive normalized real
    0xFF800000, // -infinity
    0xFF800001, // -SNaN
    0xFFFFFFFF, // -QNaN
    0x7F7FFFFF, // positive max
    0xFF7FFFFF, // negative max
    0x3F000000, // normal positive, 1.0x2^-1
    0xBF000000, // normal negative, -1.0x2^-1
};

static const uint16_t half_corner_cases[] = {
    0x0000, // +zero
    0x0001, // positive denormalized real
    0x7C00, // +infinity
    0x7C01, // SNaN
    0x7FFF, // QNaN
    0x8000, // -zero
    0x8001, // Negative denormalized real
    0xFC00, // -infinity
    0xFC01, // -SNaN
    0xFFFF, // -QNaN
    0x7BFF, // positive max
    0xFBFF, // negative max
    0x3800, // normal positive, 1.0x2^-1
    0xB800, // normal negative, -1.0x2^-1
};

static int NUM_CORNER_CASES = sizeof(float_corner_cases) / sizeof(float_corner_cases[0]);

static const uint32_t float_corner_cases_fpclassify[] = {
    FP_ZERO,      // +zero
    FP_SUBNORMAL, // positive denormalized real
    FP_INFINITE,  // +infinity
    FP_NAN,       // SNaN
    FP_NAN,       // QNaN
    FP_ZERO,      // -zero
    FP_SUBNORMAL, // Negative denormalized real
    FP_INFINITE,  // -infinity
    FP_NAN,       // -SNaN
    FP_NAN,       // -QNaN
    FP_NORMAL,    // positive max
    FP_NORMAL,    // negative max
    FP_NORMAL,    // normal positive, 1.0x2^-1
    FP_NORMAL,    // normal negative, -1.0x2^-1
};

static const uint16_t half_corner_cases_2[] = {
    0x0000, 0x8000, // +zero, -zero
    0x8000, 0x0000, // -zero, +zero
    0x7C00, 0xFC00, // +inf, -inf
    0xFC00, 0x7C00, // +inf, -inf
    0x0400, 0x03FF, // norm1, subn1
    0x03FF, 0x0400, // subn1, norm1
    0x03FF, 0x03FE, // subn1, subn2
    0x03FE, 0x03FF, // subn2, subn1
    0xFC01, 0x7E00, // snan1, qnan0
    0x7E00, 0xFC01, // qnan0, snan1
    0xFC01, 0xFE01, // snan1, neg_snan1
    0xFE01, 0xFC01, // neg_snan1, snan1
    0xFC01, 0xFC02, // snan1, snan2
    0xFC02, 0xFC01, // snan2, snan1
    0xFC01, 0x7E03, // snan1, qnan3
    0x7E03, 0xFC01, // qnan3, snan1
    0xFE01, 0x7E00, // neg_snan1, neg_qnan0
    0x7E00, 0xFE01, // neg_qnan0, neg_snan1
    0xFC01, 0x3C00, // snan1, one
    0x3C00, 0xFC01, // one, snan1
    0x7C00, 0x3C00, // inf, one
    0x3C00, 0x7C00, // one, inf
    0xFC01, 0x7C00, // snan1, inf
    0xFC00, 0xFC01, // inf, snan1
    0xFC01, 0x0000, // snan1, +zero
    0x0000, 0xFC01, // +zero, snan1
    0xFC01, 0xBC00, // snan1, neg_one
    0xBC00, 0xFC01, // neg_one, snan1
    0xFC01, 0xFC00, // snan1, -inf
    0xFC00, 0xFC01, // -inf, snan1
    0xFC01, 0x8000, // snan1, -zero
    0x8000, 0xFC01, // -zero, snan1
};

static const uint32_t float_corner_cases_2[] = {
    0x00000000, 0x80000000, // +zero, -zero
    0x80000000, 0x00000000, // -zero, +zero
    0x7f800000, 0xff800000, // +inf, -inf
    0xff800000, 0x7f800000, // +inf, -inf
    0x00800000, 0x007fffff, // norm1, subn1
    0x007fffff, 0x00800000, // subn1, norm1
    0x007fffff, 0x007ffffe, // subn1, subn2
    0x007ffffe, 0x007fffff, // subn2, subn1
    0x7f800001, 0x7fc00000, // snan1, qnan0
    0x7fc00000, 0x7f800001, // qnan0, snan1
    0x7f800001, 0xff800001, // snan1, neg_snan1
    0xff800001, 0x7f800001, // neg_snan1, snan1
    0x7f800001, 0x7f800002, // snan1, snan2
    0x7f800002, 0x7f800001, // snan2, snan1
    0x7f800001, 0x7fc00003, // snan1, qnan3
    0x7fc00003, 0x7f800001, // qnan3, snan1
    0xff800001, 0xffc00000, // neg_snan1, neg_qnan0
    0xffc00000, 0xff800001, // neg_qnan0, neg_snan1
    0x7f800001, 0x3f800000, // snan1, one
    0x3f800000, 0x7f800001, // one, snan1
    0x7f800000, 0x3f800000, // inf, one
    0x3f800000, 0x7f800000, // one, inf
    0x7f800001, 0x7f800000, // snan1, inf
    0x7f800000, 0x7f800001, // inf, snan1
    0x7f800001, 0x00000000, // snan1, +zero
    0x00000000, 0x7f800001, // +zero, snan1
    0x7f800001, 0xbf800000, // snan1, neg_one
    0xbf800000, 0x7f800001, // neg_one, snan1
    0x7f800001, 0xff800000, // snan1, -inf
    0xff800000, 0x7f800001, // -inf, snan1
    0x7f800001, 0x80000000, // snan1, -zero
    0x80000000, 0x7f800001, // -zero, snan1
};

static const uint64_t double_corner_cases_2[] = {
    0x0000000000000000ULL, 0x8000000000000000ULL, // +zero00000000, -zero
    0x8000000000000000ULL, 0x0000000000000000ULL, // -zero00000000, +zero
    0x7ff0000000000000ULL, 0xfff0000000000000ULL, // +inf00000000, -inf
    0xfff0000000000000ULL, 0x7ff0000000000000ULL, // +inf00000000, -inf
    0x00f0000000000000ULL, 0x007fffffffffffffULL, // norm100000000, subn1
    0x000fffffffffffffULL, 0x0008000000000000ULL, // subn100000000, norm1
    0x000fffffffffffffULL, 0x000ffffffffffffeULL, // subn100000000, subn2
    0x000ffffffffffffeULL, 0x000fffffffffffffULL, // subn200000000, subn1
    0x7ff0000000000001ULL, 0x7ff8000000000000ULL, // snan100000000, qnan0
    0x7ff8000000000000ULL, 0x7ff0000000000001ULL, // qnan000000000, snan1
    0x7ff0000000000001ULL, 0xfff0000000000001ULL, // snan100000000, neg_snan1
    0xfff0000000000001ULL, 0x7ff0000000000001ULL, // neg_snan100000000, snan1
    0x7ff0000000000001ULL, 0x7ff0000000000002ULL, // snan100000000, snan2
    0x7ff0000000000002ULL, 0x7ff0000000000001ULL, // snan200000000, snan1
    0x7ff0000000000001ULL, 0x7ff8000000000003ULL, // snan100000000, qnan3
    0x7ff8000000000003ULL, 0x7ff0000000000001ULL, // qnan300000000, snan1
    0xfff0000000000001ULL, 0xfff8000000000000ULL, // neg_snan100000000, neg_qnan0
    0xfff8000000000000ULL, 0xfff0000000000001ULL, // neg_qnan000000000, neg_snan1
    0x7ff0000000000001ULL, 0x3ff0000000000000ULL, // snan100000000, one
    0x3ff0000000000000ULL, 0x7ff0000000000001ULL, // one00000000, snan1
    0x7ff0000000000000ULL, 0x3ff0000000000000ULL, // inf, one
    0x3ff0000000000000ULL, 0x7ff0000000000000ULL, // one00000000, inf
    0x7ff0000000000001ULL, 0x7ff0000000000000ULL, // snan100000000, inf
    0x7ff0000000000000ULL, 0x7ff0000000000001ULL, // inf00000000, snan1
    0x7ff0000000000001ULL, 0x0000000000000000ULL, // snan100000000, +zero
    0x0000000000000000ULL, 0x7ff0000000000001ULL, // +zero00000000, snan1
    0x7ff0000000000001ULL, 0xbff0000000000000ULL, // snan100000000, neg_one
    0xbff0000000000000ULL, 0x7ff0000000000001ULL, // neg_one00000000, snan1
    0x7ff0000000000001ULL, 0xfff0000000000000ULL, // snan100000000, -inf
    0xfff0000000000000ULL, 0x7ff0000000000001ULL, // -inf00000000, snan1
    0x7ff0000000000001ULL, 0x8000000000000000ULL, // snan100000000, -zero
    0x8000000000000000ULL, 0x7ff0000000000001ULL, // -zero00000000, snan1
};

static int NUM_CORNER_CASES_2 = sizeof(float_corner_cases_2) / sizeof(float_corner_cases_2[0]);

#define irand(x) ((unsigned int)((x)*drand()))

#include <vector>
template <class T>
class generator {
   public:
    std::vector<T> values;
    virtual int size() const { return values.size(); }
    virtual const T operator[](int i) const {
        assert(i >= 0);
        assert((unsigned int)i < values.size());
        return values[i];
    }
    virtual ~generator() {}
};

// A little syntactic sugar over a vector.
template <class T>
class array_generator : public generator<T> {
   public:
    array_generator() {}
    array_generator(const T* array, int N) {
        std::cout << "array Generator\n";
        generator<T>::values.resize(N);
        for (int i = 0; i < N; i++) {
            generator<T>::values[i] = array[i];
            // std::cout << "value = " <<  generator<T>::values[i] << "\n";
        }
    }
    array_generator(const uint32_t* array, int N) {
        generator<T>::values.resize(N);
        for (int i = 0; i < N; i++) {
            generator<T>::values[i] = fp_struct<T>(array[i]).to_ieee();
        }
    }
    array_generator(const uint64_t* array, int N) {
        generator<T>::values.resize(N);
        for (int i = 0; i < N; i++) {
            generator<T>::values[i] = fp_struct<T>(array[i]).to_ieee();
        }
    }
    array_generator(const uint16_t* array, int N) {
        generator<T>::values.resize(N);
        for (int i = 0; i < N; i++) {
            generator<T>::values[i] = fp_struct<T>(array[i]).to_ieee();
        }
    }
};

template <class T, class Enable = void>
class corner_cases_generator : public generator<T> {
   public:
    corner_cases_generator() { std::cerr << "No corner cases for type..\n"; }
};

template <int W>
class corner_cases_generator<ap_uint<W>, typename solver_tb::enable_if<true>::type> : public generator<ap_uint<W> > {
   public:
    typedef ap_uint<W> T;
    corner_cases_generator() {
        //        generator<T>::values.resize(N);
        T zero(0);
        generator<T>::values.push_back(zero);
        T max(-1); // largest positive value
        generator<T>::values.push_back(max);
        T max_less(-1);
        max_less[0] = 0;
        generator<T>::values.push_back(max_less);
        T one(1);
        generator<T>::values.push_back(one);
    }
};

template <int W>
class corner_cases_generator<ap_int<W>, typename solver_tb::enable_if<true>::type> : public generator<ap_int<W> > {
   public:
    typedef ap_int<W> T;
    corner_cases_generator() {
        T zero(0);
        generator<T>::values.push_back(zero);
        T max(-1); // Largest positive value
        max[W - 1] = 0;
        generator<T>::values.push_back(max);
        T min(0); // Largest magnitude negative value
        min[W - 1] = 1;
        generator<T>::values.push_back(min);
        T max_less(max);
        max_less[0] = 0;
        generator<T>::values.push_back(max_less);
        T one(1);
        generator<T>::values.push_back(one);
        T minus_one(-1);
        generator<T>::values.push_back(minus_one);
    }
};

template <int W, int I, ap_q_mode Q, ap_o_mode O>
class corner_cases_generator<ap_ufixed<W, I, Q, O>, typename solver_tb::enable_if<true>::type>
    : public generator<ap_ufixed<W, I, Q, O> > {
   public:
    typedef ap_ufixed<W, I, Q, O> T;
    corner_cases_generator() {
        //        generator<T>::values.resize(N);
        T zero(0);
        generator<T>::values.push_back(zero);
        T max(-1);
        generator<T>::values.push_back(max);
        T max_less(-1);
        max_less[0] = 0;
        generator<T>::values.push_back(max_less);
        T one(1);
        generator<T>::values.push_back(one);
    }
};

template <int W, int I, ap_q_mode Q, ap_o_mode O>
class corner_cases_generator<ap_fixed<W, I, Q, O>, typename solver_tb::enable_if<true>::type>
    : public generator<ap_fixed<W, I, Q, O> > {
   public:
    typedef ap_fixed<W, I, Q, O> T;
    corner_cases_generator() {
        //        generator<T>::values.resize(N);
        T zero(0);
        generator<T>::values.push_back(zero);
        T max(-1);
        max[W - 1] = 0;
        generator<T>::values.push_back(max);
        T min(0);
        min[W - 1] = 1;
        generator<T>::values.push_back(min);
        T max_less(max);
        max_less[0] = 0;
        generator<T>::values.push_back(max_less);
        T one(1);
        generator<T>::values.push_back(one);
        T minus_one(-1);
        generator<T>::values.push_back(minus_one);
    }
};

template <>
class corner_cases_generator<half, typename solver_tb::enable_if<true>::type> : public array_generator<half> {
   public:
    corner_cases_generator()
        : array_generator<half>(half_corner_cases, sizeof(half_corner_cases) / sizeof(half_corner_cases[0])) {}
};

template <>
class corner_cases_generator<float, typename solver_tb::enable_if<true>::type> : public array_generator<float> {
   public:
    corner_cases_generator()
        : array_generator<float>(float_corner_cases, sizeof(float_corner_cases) / sizeof(float_corner_cases[0])) {}
};

template <>
class corner_cases_generator<double, typename solver_tb::enable_if<true>::type> : public array_generator<double> {
   public:
    corner_cases_generator()
        : array_generator<double>(double_corner_cases, sizeof(double_corner_cases) / sizeof(double_corner_cases[0])) {}
};

template <class T, class Enable = void>
class rand_generator;

template <class T>
class rand_generator<T, typename solver_tb::enable_if<is_fptype<T>::value>::type> : public generator<T> {
   public:
    rand_generator(int N) {
        std::cout << "rand Generator\n";
        generator<T>::values.resize(N);
        for (int i = 0; i < N; i++) {
            long long r = rand32() + ((long long)rand32() << 32); // 64 bit random
            generator<T>::values[i] = fp_struct<T>((typename fp_struct<T>::data_type)r).to_ieee();
            // std::cout << "value = " <<  generator<T>::values[i] << " " << std::hex << r << " " <<
            // generator<T>::values[i] << std::dec << "\n";
        }
    }
};

template <>
class rand_generator<half, void> : public generator<half> {
   public:
    rand_generator(int N) {
        std::cout << "rand Generator\n";
        generator<half>::values.resize(N);
        for (int i = 0; i < N; i++) {
            int r = (rand8() | (rand8() << 8));
            half h;
            h.set_bits((unsigned short)r);
            generator<half>::values[i] = h;
            // std::cout << "value = " <<  generator<half>::values[i] << " " << std::hex << r << " " <<
            // generator<half>::values[i] << std::dec << "\n";
        }
    }
};

#ifdef X_AP_FLOAT_H
template <int W, int I, int E_W, bool Normal>
class rand_generator<ap_float<W, I, E_W, Normal>, void> : public generator<ap_float<W, I, E_W, Normal> > {
    rand_generator<ap_fixed<W, I> > mant_generator;
    rand_generator<ap_int<E_W> > exp_generator;
    using generator<ap_float<W, I, E_W> >::values;

   public:
    rand_generator(int N) : mant_generator(N), exp_generator(N) {
        generator<ap_float<W, I, E_W> >::values.resize(N);
        for (int i = 0; i < N; i++) {
            ap_float<W, I, E_W, false> x;
            x.exp = exp_generator.values[i];
            x.mant = mant_generator.values[i];
            generator<ap_float<W, I, E_W, Normal> >::values[i] = x;
            if (Normal) values[i] = values[i].normalize();
        }
    }
};
#endif

template <int W, int I, ap_q_mode Q, ap_o_mode O>
class rand_generator<ap_fixed<W, I, Q, O>, void> : public generator<ap_fixed<W, I, Q, O> > {
   public:
    rand_generator(int N) {
        generator<ap_fixed<W, I, Q, O> >::values.resize(N);
        for (int i = 0; i < N; i++) {
            ap_uint<W> x = 0;
            for (int j = 0; j < (W + 31) / 32; j++) {
                x = (x << 32) + rand32();
            }
            generator<ap_fixed<W, I, Q, O> >::values[i](W - 1, 0) = x(W - 1, 0);
            if (O == AP_SAT_SYM) {
                // It is possible to generate an unsaturated value.
                // Below ensures that the result is always saturated.
                generator<ap_fixed<W, I, Q, O> >::values[i]++;
            }
        }
    }
};

template <int W, int I, ap_q_mode Q, ap_o_mode O>
class rand_generator<ap_ufixed<W, I, Q, O>, void> : public generator<ap_ufixed<W, I, Q, O> > {
   public:
    rand_generator(int N) {
        generator<ap_ufixed<W, I, Q, O> >::values.resize(N);
        for (int i = 0; i < N; i++) {
            ap_uint<W> x = 0;
            for (int j = 0; j < (W + 31) / 32; j++) {
                x = (x << 32) + rand32();
            }
            generator<ap_ufixed<W, I, Q, O> >::values[i](W - 1, 0) = x(W - 1, 0);
        }
    }
};

template <typename T>
class rand_generator<T, typename solver_tb::enable_if<is_integraltype<T>::value>::type> : public generator<T> {
   public:
    rand_generator(int N) {
        generator<T>::values.resize(N);
        for (int i = 0; i < N; i++) {
            long long r = rand32() + ((long long)rand32() << 32); // 64 bit random
            generator<T>::values[i] = r;
        }
    }
};

// template <class T>
// class rand_generator<T, typename enable_if<is_integraltype<T>::value >::type> : public generator<T> {
// public:
//     rand_generator(int N) {
//         generator<T>::values.resize(N);
//         for(int i = 0; i < N; i++) {
//             long long r = rand32() + ((long long)rand32() << 32); // 64 bit random
//             generator<T>::values[i] = fp_struct<T>((typename fp_struct<T>::data_type)r).to_ieee();
//         }
//     }
// };

template <class T, class Enable = void>
class rand_range_generator;

template <class T>
class rand_range_generator<T, typename solver_tb::enable_if<is_fptype<T>::value>::type> : public generator<T> {
   public:
    rand_range_generator(T minVal, T maxVal, int N) {
        generator<T>::values.resize(N);
        for (int i = 0; i < N; i++) {
            double range = maxVal - minVal;
            generator<T>::values[i] = drand() * range + minVal;
        }
    }
};

template <>
class rand_range_generator<half, void> : public generator<half> {
   public:
    rand_range_generator(half minVal, half maxVal, int N) {
        generator<half>::values.resize(N);
        for (int i = 0; i < N; i++) {
            double range = (double)(maxVal - minVal);
            generator<half>::values[i] = drand() * range + (double)minVal;
            // std::cout << "value = " <<  generator<half>::values[i] << " " << std::hex << " " <<
            // generator<half>::values[i] << std::dec << "\n";
        }
    }
};

template <int W, int I, ap_q_mode Q, ap_o_mode O>
class rand_range_generator<ap_fixed<W, I, Q, O>, void> : public generator<ap_fixed<W, I, Q, O> > {
   public:
    rand_range_generator(ap_fixed<W, I, Q, O> minVal, ap_fixed<W, I, Q, O> maxVal, int N) {
        generator<ap_fixed<W, I, Q, O> >::values.resize(N);
        for (int i = 0; i < N; i++) {
            ap_fixed<W + 1, I + 1, Q, O> range = maxVal - minVal;
            generator<ap_fixed<W, I, Q, O> >::values[i] =
                ap_fixed<W, I, AP_RND, AP_SAT>(ap_fixed<64, 32, AP_RND, AP_SAT>(drand()) * range + minVal);
        }
    }
};

template <int W, int I, ap_q_mode Q, ap_o_mode O>
class rand_range_generator<ap_ufixed<W, I, Q, O>, void> : public generator<ap_ufixed<W, I, Q, O> > {
   public:
    rand_range_generator(ap_ufixed<W, I, Q, O> minVal, ap_ufixed<W, I, Q, O> maxVal, int N) {
        generator<ap_ufixed<W, I, Q, O> >::values.resize(N);
        for (int i = 0; i < N; i++) {
            ap_ufixed<W, I, Q, O> range = maxVal - minVal;
            generator<ap_ufixed<W, I, Q, O> >::values[i] =
                ap_ufixed<W, I, AP_RND, AP_SAT>(ap_ufixed<64, 32, AP_RND, AP_SAT>(drand()) * range + minVal);
        }
    }
};

template <class T>
class rand_range_generator<T, typename solver_tb::enable_if<is_integraltype<T>::value>::type> : public generator<T> {
   public:
    rand_range_generator(T minVal, T maxVal, int N) {
        generator<T>::values.resize(N);
        for (int i = 0; i < N; i++) {
            double range = maxVal - minVal;
            generator<T>::values[i] = drand() * range + (double)minVal;
        }
    }
};

template <class T>
class rand_subnormal_generator : public generator<T> {
   public:
    rand_subnormal_generator(int N) {
        generator<T>::values.resize(N);
        for (int i = 0; i < N; i++) {
            long long r = rand32() + ((long long)rand32() << 32);
            fp_struct<T> x((typename fp_struct<T>::data_type)r);
            x.exp = 0;
            generator<T>::values[i] = x.to_ieee();
        }
    }
};

template <class T, class Enable = void>
class range_generator;

template <class T>
class range_generator<T, typename solver_tb::enable_if<is_fptype<T>::value>::type> : public generator<T> {
   public:
    range_generator(T minVal, T maxVal, int N) {
        generator<T>::values.resize(N);
        bool minNeg = minVal < 0.0;
        bool maxNeg = maxVal < 0.0;
        fp_struct<T> minVals = minVal;
        fp_struct<T> maxVals = maxVal;
        fp_struct<T> negZero = (T)-0.0;
        long long start = minNeg ? (negZero.to_int() - minVals.to_int()) : minVals.to_int();
        long long end = maxNeg ? (negZero.to_int() - maxVals.to_int()) : maxVals.to_int();

        std::cout << "range Generator\n";
        std::cout << start << " to " << end << "\n";
        long long incr = (end - start) / N;
        if (incr < 0) incr = -incr;
        std::cout << N << " steps of " << incr << "\n";
        assert(incr > 1);
        //        incr = incr/N;
        // std::cout << "incr = " << incr << "\n";
        long long news = start;
        for (int i = 0; i < N; i++) {
            //  std::cout << "news = " << std::hex << news << std::dec << "\n";
            fp_struct<T> s = typename fp_struct<T>::data_type((news < 0) ? (negZero.to_int() - news) : news);
            //  std::cout << "v = " << s.to_ieee() << " " << s.data().to_string(2) << "\n";
            T t = s.to_ieee();
            generator<T>::values[i] = t;
            news += incr;
        }
    }
};

template <class T>
class range_generator<T, typename solver_tb::enable_if<enable_or<is_integraltype<T>, is_fixedtype<T> >::value>::type>
    : public generator<T> {
   public:
    range_generator(T minVal, T maxVal, int N) {
        generator<T>::values.resize(N);

        std::cout << "range Generator\n";
        std::cout << minVal << " to " << maxVal << "\n";
        T incr = (maxVal - minVal) / N;
        std::cout << N << " steps of " << incr << "\n";
        assert(incr > 0);
        T news = minVal;
        for (int i = 0; i < N; i++) {
            generator<T>::values[i] = news;
            news += incr;
        }
    }
};

template <>
class range_generator<half, void> : public generator<half> {
   public:
    range_generator(half minVal, half maxVal, int N) {
        generator<half>::values.resize(N);
        std::cout << "range Generator\n";
        std::cout << minVal << " to " << maxVal << "\n";
        half incr = (maxVal - minVal) / N;
        std::cout << N << " steps of " << incr << "\n";
        assert(incr > 0);
        half news = minVal;
        for (int i = 0; i < N; i++) {
            generator<half>::values[i] = news;
            news += incr;
        }
    }
};

template <class T>
class sweep_exp_rand_mantissa_generator : public generator<T> {
   public:
    sweep_exp_rand_mantissa_generator() {
        fp_struct<T> tmp;
        int N = tmp.EXP_BIAS + 1;
        generator<T>::values.resize(2 * N);
        int sign = 1;
        int exp = 0;
        for (int i = 0; i < 2 * N; i++) {
            fp_struct<T> din;
            din.exp = exp;
            exp = (din.exp == (N - 1)) ? 0 : exp + 1;
            din.sign = sign;
            sign = (din.exp == (N - 1)) ? ~sign : sign;
            din.sig = (rand32() + ((long long)rand32() << 32));
            generator<T>::values[i] = din.to_ieee();
        }
    }
};

#ifdef X_AP_FLOAT_H
template <int W, int I, int E_W, bool Normal>
class sweep_exp_rand_mantissa_generator<ap_float<W, I, E_W, Normal> > : public generator<ap_float<W, I, E_W, Normal> > {
    rand_generator<ap_fixed<W, I> > mant_generator;
    range_generator<ap_int<E_W> > exp_generator;
    const static int N = 1 << (E_W - 1);

   public:
    sweep_exp_rand_mantissa_generator() : mant_generator(N), exp_generator(0, -1, N) {
        generator<ap_float<W, I, E_W, Normal> >::values.resize(N);
        for (int i = 0; i < N; i++) {
            ap_float<W, I, E_W> x;
            x.exp = exp_generator.values[i];
            x.mant = mant_generator.values[i];
            if (Normal) x.mant[W - 2] = !x.mant[W - 1];
            generator<ap_float<W, I, E_W, Normal> >::values[i] = x;
        }
    }
};
#endif

template <class T, class Enable = void>
class dense_generator;

template <class T>
class dense_generator<T, typename solver_tb::enable_if<is_fptype<T>::value>::type> : public generator<T> {
   public:
    dense_generator(T centerVal, int N) {
        generator<T>::values.resize(N);
        fp_struct<T> s = centerVal;
        fp_struct<T> negZero = (T)-0.0;
        int news = (centerVal < 0.0) ? (negZero.to_int() - s.to_int()) : s.to_int();
        for (int i = 0; i < N; i++) {
            fp_struct<T> s = typename fp_struct<T>::data_type((news < 0) ? (negZero.to_int() - news) : news);
            generator<T>::values[i] = s.to_ieee();
            news = (i % 2 == 0) ? news + i : news - i;
        }
    }
};

template <int W, int I, ap_q_mode Q, ap_o_mode O>
class dense_generator<ap_fixed<W, I, Q, O>, typename solver_tb::enable_if<true>::type>
    : public generator<ap_fixed<W, I, Q, O> > {
   public:
    typedef ap_fixed<W, I, Q, O> T;
    dense_generator(T centerVal, int N) {
        generator<T>::values.resize(N);
        T news = centerVal;
        for (int i = 0; i < N; i++) {
            generator<T>::values[i] = news;
            T incr;
            incr(W - 1, 0) = i;
            news = (i % 2 == 0) ? news + incr : news - incr;
        }
    }
};

template <int W, int I, ap_q_mode Q, ap_o_mode O>
class dense_generator<ap_ufixed<W, I, Q, O>, typename solver_tb::enable_if<true>::type>
    : public generator<ap_ufixed<W, I, Q, O> > {
   public:
    typedef ap_ufixed<W, I, Q, O> T;
    dense_generator(T centerVal, int N) {
        generator<T>::values.resize(N);
        T news = centerVal;
        for (int i = 0; i < N; i++) {
            generator<T>::values[i] = news;
            T incr;
            incr(W - 1, 0) = i;
            news = (i % 2 == 0) ? ap_ufixed<W, I, Q, O>(news + incr) : ap_ufixed<W, I, Q, O>(news - incr);
        }
    }
};

template <>
class dense_generator<half, void> : public generator<half> {
   public:
    dense_generator(half centerVal, int N) {
        generator<half>::values.resize(N);
        fp_struct<half> s = centerVal;
        fp_struct<half> negZero = (half)-0.0;
        int news = (centerVal < 0.0) ? (negZero.to_int() - s.to_int()) : s.to_int();
        for (int i = 0; i < N; i++) {
            fp_struct<half> s = typename fp_struct<half>::data_type((news < 0) ? (negZero.to_int() - news) : news);
            generator<half>::values[i] = s.to_ieee();
            news = (i % 2 == 0) ? news + i : news - i;
        }
    }
};

// template <class T>
// class dense_generator<T, typename enable_if<is_integraltype<T>::value >::type> : public generator<T> {
// public:
//     dense_generator(T centerVal, int N) {
//         generator<T>::values.resize(N);
//         T news = centerVal;
//         for(int i = 0; i < N; i++) {
//             generator<T>::values[i] = news;
//             news = (i%2 == 0) ? news+i : news-i;
//         }
//     }
// };

template <class T, class Enable = void>
class exhaustive_generator;

template <int W, int I, ap_q_mode Q, ap_o_mode O>
class exhaustive_generator<ap_fixed<W, I, Q, O>, typename solver_tb::enable_if<W <= 16>::type>
    : public generator<ap_fixed<W, I, Q, O> > {
   public:
    typedef ap_fixed<W, I, Q, O> T;
    exhaustive_generator() {
        int N = 1 << W;
        generator<T>::values.resize(N);
        for (int i = 0; i < N; i++) {
            T val;
            val(W - 1, 0) = i;
            generator<T>::values[i] = val;
        }
    }
};

template <int W, int I, ap_q_mode Q, ap_o_mode O>
class exhaustive_generator<ap_ufixed<W, I, Q, O>, typename solver_tb::enable_if<W <= 16>::type>
    : public generator<ap_ufixed<W, I, Q, O> > {
   public:
    typedef ap_ufixed<W, I, Q, O> T;
    exhaustive_generator() {
        int N = 1 << W;
        generator<T>::values.resize(N);
        for (int i = 0; i < N; i++) {
            T val;
            val(W - 1, 0) = i;
            generator<T>::values[i] = val;
        }
    }
};

template <>
class exhaustive_generator<half, void> : public generator<half> {
   public:
    exhaustive_generator() {
        const int N = 65536;
        generator<half>::values.resize(N);
        for (int i = 0; i < N; i++) {
            fp_struct<half> s = typename fp_struct<half>::data_type(i);
            generator<half>::values[i] = s.to_ieee();
        }
    }
};

template <>
class exhaustive_generator<float, void> : public generator<float> {
   public:
    typename fp_struct<float>::data_type min, max;
    exhaustive_generator() {
        min = fp_struct<float>(std::numeric_limits<float>::min()).data();
        max = fp_struct<float>(std::numeric_limits<float>::max()).data();
    }
    exhaustive_generator(float _min, float _max) {
        min = fp_struct<float>(_min).data();
        max = fp_struct<float>(_max).data();
        assert(max > min);
    }
    int size() const { return max - min + 1; }
    const float operator[](int i) const {
        assert(i >= 0);
        assert(i < size());
        typename fp_struct<float>::data_type x = min + i;
        if ((i & 0xFFFFF) == 0) {
            std::cout << std::hex << i << " " << x << " " << fp_struct<float>(x).to_ieee() << "\n";
        }
        fp_struct<float> s = x;
        return s.to_ieee();
    }
};

#endif
