// Printing Utility Functions !!
#ifndef SPU_H_
#define SPU_H_
#include <iostream>
#include <iomanip>
//#include "hls_ssr_fft_types.h"
//#include <DEBUG_CONSTANTS.hpp>
#include <complex>
//###################################### Utility Macros##################################START
#ifdef SHOW_DEC_MACRO_ENABLED
#define SHOW_DEC(a) std::cout << std::setw(10) << #a << std::dec << ":" << (a) << std::endl
#else
#define SHOW_DEC(a) ;
#endif
//###################################### Utility Macros####################################END

// Standard std::complex support

template <int p_rows, int p_cols, typename tt_dtype>
void SidebySidePrintComplexNumArrays(std::complex<tt_dtype> in[p_rows][p_cols],
                                     std::complex<tt_dtype> in2[p_rows][p_cols]) {
    std::cout << "\n\n################### Printing Arrays Side by Side for Comparison ##############" << std::endl;
    for (int i = 0; i < p_rows; i++)
        for (int j = 0; j < p_cols; j++) {
            // in[i][j] = i*(R)+j;
            std::cout << "[" << i << "]"
                      << "[" << j << "].real"
                      << "=" << std::setw(10) << in[i][j].real() << "  :  " << std::setw(10) << in2[i][j].real()
                      << " , ";
            std::cout << "[" << i << "]"
                      << "[" << j << "].imag"
                      << "=" << std::setw(10) << in[i][j].imag() << "  :  " << std::setw(10) << in2[i][j].imag()
                      << " , " << std::endl;
        }
    std::cout
        << "################### Element Wise Difference :: Printing Arrays Side by Side for Comparison ##############"
        << std::endl;

    for (int i = 0; i < p_rows; i++)
        for (int j = 0; j < p_cols; j++) {
            // in[i][j] = i*(R)+j;
            std::cout << "[" << i << "]"
                      << "[" << j << "].real.diff"
                      << "=" << std::setw(10) << in[i][j].real() - in2[i][j].real() << " , ";
            std::cout << "[" << i << "]"
                      << "[" << j << "].imag.diff"
                      << "=" << std::setw(10) << in[i][j].imag() - in2[i][j].imag() << " , " << std::endl;
        }
}

template <int p_rows, int p_cols, typename tt_dtype1, typename tt_dtype2>
void SidebySidePrintComplexNumArrays(std::complex<tt_dtype1> in[p_rows][p_cols],
                                     std::complex<tt_dtype2> in2[p_rows][p_cols]) {
    std::cout << "\n\n################### Printing Arrays Side by Side for Comparison ##############" << std::endl;
    for (int i = 0; i < p_rows; i++)
        for (int j = 0; j < p_cols; j++) {
            // in[i][j] = i*(R)+j;
            std::cout << "[" << i << "]"
                      << "[" << j << "].real"
                      << "=" << std::setw(10) << in[i][j].real() << "  :  " << std::setw(10) << in2[i][j].real()
                      << " , ";
            std::cout << "[" << i << "]"
                      << "[" << j << "].imag"
                      << "=" << std::setw(10) << in[i][j].imag() << "  :  " << std::setw(10) << in2[i][j].imag()
                      << " , " << std::endl;
        }
    std::cout
        << "################### Element Wise Difference :: Printing Arrays Side by Side for Comparison ##############"
        << std::endl;

    for (int i = 0; i < p_rows; i++)
        for (int j = 0; j < p_cols; j++) {
            // in[i][j] = i*(R)+j;
            std::cout << "[" << i << "]"
                      << "[" << j << "].real.diff"
                      << "=" << std::setw(10) << (double)in[i][j].real() - (double)in2[i][j].real() << " , ";
            std::cout << "[" << i << "]"
                      << "[" << j << "].imag.diff"
                      << "=" << std::setw(10) << (double)in[i][j].imag() - (double)in2[i][j].imag() << " , "
                      << std::endl;
        }
}

template <typename tt_dtype, int p_rows, int p_cols>
void printComplexNumArray(std::complex<tt_dtype> in[p_rows][p_cols]) {
    std::cout << "###################" << std::endl;
    for (int j = 0; j < p_cols; j++)
        for (int i = 0; i < p_rows; i++) {
            // in[i][j] = i*(R)+j;
            std::cout << "[" << i << "]"
                      << "[" << j << "].real"
                      << "=" << in[i][j].real() << " , ";
            std::cout << "[" << i << "]"
                      << "[" << j << "].imag"
                      << "=" << in[i][j].imag() << std::endl;
            ;
        }
}

template <typename tt_dtype, int p_rows>
void printComplexNumArray1D(std::complex<tt_dtype> in[p_rows]) {
    std::cout << "###################" << std::endl;
    for (int i = 0; i < p_rows; i++) {
        // in[i][j] = i*(R)+j;
        std::cout << "[" << i << "]"
                  << ".real"
                  << "=" << in[i].real() << " , ";
        std::cout << "[" << i << "]"
                  << ".imag"
                  << "=" << in[i].imag() << std::endl;
        ;
    }
}

///// @@@@@   complexNum Support

/*template <typename tt_dtype, int p_rows, int p_cols>
void printComplexNumArray(complexNum<tt_dtype> in[p_rows][p_cols])
{
        std::cout << "###################" << std::endl;
        for (int i = 0;i<p_rows;i++)
                for (int j = 0;j<p_cols;j++)
                {
                        //in[i][j] = i*(R)+j;
                        std::cout << "[" << i << "]" << "[" << j << "].real" << "=" << in[i][j].real << " , ";
                        std::cout << "[" << i << "]" << "[" << j << "].imag" << "=" << in[i][j].imag << std::endl;;

                }
}

template <typename tt_dtype, int p_rows>
void printComplexNumArray1D(complexNum<tt_dtype> in[p_rows])
{
        std::cout << "################### Printing 1D Array ####################" << std::endl;
        for (int i = 0;i<p_rows;i++)
                {
                        //in[i][j] = i*(R)+j;
                        std::cout << "[" << i << "]" << ".real" << "=" << in[i].real << " , ";
                        std::cout << "[" << i << "]" << ".imag" << "=" << in[i].imag << std::endl;;

                }
}
*/

template <int dim1, typename tt_T>
void print1DArray(tt_T data[dim1]) {
    std::cout << "\n################### Printing 1D Array ####################" << std::endl;
    for (int t = 0; t < dim1; t++) std::cout << std::setw(6) << t << "\t";
    std::cout << std::endl << "\n";

    for (int t = 0; t < dim1; t++) {
        std::cout << std::setw(6) << data[t] << "\t";
    }
    std::cout << std::endl;
}

template <int dim1, int dim2, typename tt_T>
void print2DArray(tt_T data[dim1][dim2]) {
    std::cout << "\n################### Printing 2D Array ####################" << std::endl;
    for (int t = 0; t < dim1; t++) std::cout << std::setw(6) << t << "\t";
    std::cout << std::endl;

    for (int t = 0; t < dim1 * 5; t++) std::cout << "-";
    std::cout << std::endl;
    for (int r = 0; r < dim2; r++) {
        for (int t = 0; t < dim1; t++) {
            std::cout << std::setw(6) << data[t][r] << "\t";
        }
        std::cout << std::endl;
    }
}

template <int dim1, int dim2, typename tt_T>
void print2DArrayReal(tt_T data[dim1][dim2]) {
    std::cout << "\n################### Printing 2D Array ####################" << std::endl;

    for (int t = 0; t < dim1; t++) std::cout << std::setw(6) << t << "\t";
    std::cout << std::endl << "\n";
    for (int r = 0; r < dim2; r++) {
        for (int t = 0; t < dim1; t++) {
            std::cout << std::setw(6) << data[t][r].real() << "\t";
        }
        std::cout << std::endl;
    }
}

#endif // !SPU_H_
