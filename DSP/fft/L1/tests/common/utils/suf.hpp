// File Name :  suf.hpp
#ifndef __SUF__H__
#define __SUF__H__
#include <complex>
template <int dim1, typename tt_T>
int compare1DArrays(tt_T arr1[dim1], tt_T arr2[dim1]) {
    int mis_matches = 0;
    for (int t = 0; t < dim1; t++) {
        if (arr1[t] != arr2[t]) mis_matches++;
    }

    return mis_matches;
}

template <int dim1, int dim2, typename tt_T>
int compare2DArrays(tt_T arr1[dim1][dim2], tt_T arr2[dim1][dim2]) {
    int mis_matches = 0;
    for (int t = 0; t < dim1; t++) {
        for (int r = 0; r < dim2; r++) {
            if (arr1[t][r] == arr2[t][r]) {
            } else
                mis_matches++;
        }
    }

    return mis_matches;
}

template <int dim1, int dim2, typename tt_T>
int compare2DArrays(std::complex<tt_T> arr1[dim1][dim2], std::complex<tt_T> arr2[dim1][dim2]) {
    int mis_matches = 0;
    for (int t = 0; t < dim1; t++) {
        for (int r = 0; r < dim2; r++) {
            if (arr1[t][r].real() == arr2[t][r].real() && arr1[t][r].imag() == arr2[t][r].imag()) {
            } else
                mis_matches++;
        }
    }

    return mis_matches;
}

template <int dim1, int dim2>
int compare2DArraysXcomplex(std::complex<int> arr1[dim1][dim2], std::complex<int> arr2[dim1][dim2]) {
    int mis_matches = 0;
    for (int t = 0; t < dim1; t++) {
        for (int r = 0; r < dim2; r++) {
            if (arr1[t][r].real() == arr2[t][r].real() && arr1[t][r].imag() == arr2[t][r].imag()) {
            } else
                mis_matches++;
        }
    }

    return mis_matches;
}
#endif
