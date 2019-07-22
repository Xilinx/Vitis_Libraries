#ifndef XF_BLAS_L1_TESTS_UTIL_HPP
#define XF_BLAS_L1_TESTS_UTIL_HPP
#include <iostream>
#include <vector>
#include <fstream>
#include <string>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/filtering_streambuf.hpp>
#include <boost/algorithm/string.hpp>

/*!
  @brief read Diagonal matrix from .csv file
  @preCondition number of matrix lines, t_NumDiag, along the diagonal direction must be odd and > 1
  @param t_DataType data type
  @param t_N maximum number of entries in each diagonal line
  @param t_NumDiag number of diagonal lines indexed low to up
  @param p_fileName .csv file containing diagonal matrix data
  @param p_n number of entries in each diagonal line
  @param p_out output diagonal matrix
*/
template <typename t_DataType, unsigned int t_N, unsigned int t_NumDiag>
int readDiag(std::string p_fileName, unsigned int p_n, t_DataType p_out[t_N][t_NumDiag]) {
    std::ifstream l_file(p_fileName);

    std::vector<std::string> l_v;
    std::string l_line;

    if (l_file.good()) {
        std::cout << "Opened " << p_fileName << " OK" << std::endl;

        int i = 0;
        while (l_file.good() && (l_file.peek() != EOF)) {
            getline(l_file, l_line);
            boost::split(l_v, l_line, [](char c) { return c == ','; });
            if (l_v.size() == t_NumDiag) {
                for (unsigned int d = 0; d < t_NumDiag; ++d) {
                    p_out[i][d] = (t_DataType)std::stod(l_v[d]);
                }
                i++;
            }
            if (i > p_n) {
                std::cout << "Warning! File has more than expected " << p_n << " lines..." << std::endl;
                break;
            }
        }
    } else {
        std::cout << "Couldn't open " << p_fileName << std::endl;
        return -1;
    }
    return 0;
}

/*!
  @brief read vector from .csv file
  @param t_DataType data type
  @param t_N maximum number of entries in the input vector
  @param p_fileName .csv file containing the input vector data
  @param p_n number of entries in the input vector
  @param p_out output vector
 */
template <typename t_DataType, unsigned int t_N>
int readVector(std::string p_fileName, unsigned int p_n, t_DataType p_out[t_N]) {
    std::ifstream l_file(p_fileName);
    std::string l_line;

    if (l_file.good()) {
        std::cout << "Opened " << p_fileName << " OK" << std::endl;
        int i = 0;
        while (l_file.good() && (l_file.peek() != EOF)) {
            getline(l_file, l_line);
            p_out[i] = (t_DataType)std::stod(l_line);
            i++;
            if (i > p_n) {
                std::cout << "Warning! File has more than expected " << p_n << " lines/entries..." << std::endl;
                break;
            }
        }
    } else {
        std::cout << "Couldn't open " << p_fileName << std::endl;
        return -1;
    }
    return 0;
}

/*!
  @brief calculate golden reference for diagonal matrix vector multiplication
  @preCondition number of matrix lines, t_NumDiag, along the diagonal direction must be odd and > 1
  @param t_DataType data type
  @param t_N maximum number of entries in each diagonal line
  @param t_NumDiag number of diagonal lines indexed low to up
  @param p_in input diagonal matrix
  @param p_n number of entries in each diagonal line
  @param p_inV input dense vector pointer
  @param p_outV output dense vector
*/
template <typename t_DataType, unsigned int t_N, unsigned int t_NumDiag>
void diagMult(t_DataType p_in[t_N][t_NumDiag], t_DataType p_inV[t_N], unsigned int p_n, t_DataType p_outV[t_N]) {
    for (unsigned int i = 0; i < p_n; ++i) {
        p_outV[i] = 0;
    }
    for (unsigned int i = 0; i < t_NumDiag / 2; ++i) {
        for (unsigned int d = 0; d < (t_NumDiag / 2 + i + 1); ++d) {
            p_outV[i] += p_in[i][d + (t_NumDiag / 2 - i)] * p_inV[d];
        }
    }
    for (int i = t_NumDiag / 2; i < p_n; ++i) {
        for (unsigned int d = 0; d < t_NumDiag; ++d) {
            t_DataType l_vec = ((i - t_NumDiag / 2 + d) < p_n) ? p_inV[i - t_NumDiag / 2 + d] : 0;
            p_outV[i] += p_in[i][d] * l_vec;
        }
    }
}

/*!
  @brief calculate golden reference for amin
  @param t_DataType data type
  @param t_N maximum number of entries in the input vector
  @param t_IndexType type of indices
  @param p_in input vector
  @param p_n number of entries in the input vector
  @param p_res index for the entry that has minimum magnitude
*/
template <typename t_DataType, unsigned int t_N, typename t_IndexType>
void aminRef(t_DataType p_in[t_N], unsigned int p_n, t_IndexType& p_res) {
    unsigned int l_index = 0;
    t_DataType l_minMagVal = (p_in[0] < 0) ? -p_in[0] : p_in[0];
    ;

    for (unsigned int i = 0; i < p_n; ++i) {
        t_DataType l_val = (p_in[i] < 0) ? -p_in[i] : p_in[i];
        if (l_minMagVal > l_val) {
            l_minMagVal = l_val;
            l_index = i;
        }
    }
    p_res = l_index;
}
/*!
  @brief calculate golden reference for amax
  @param t_DataType data type
  @param t_N maximum number of entries in the input vector
  @param t_IndexType type of indices
  @param p_in input vector
  @param p_n number of entries in the input vector
  @param p_res index for the entry that has maximum magnitude
*/
template <typename t_DataType, unsigned int t_N, typename t_IndexType>
void amaxRef(t_DataType p_in[t_N], unsigned int p_n, t_IndexType& p_res) {
    unsigned int l_index = 0;
    t_DataType l_maxMagVal = 0;

    for (unsigned int i = 0; i < p_n; ++i) {
        t_DataType l_val = (p_in[i] < 0) ? -p_in[i] : p_in[i];
        if (l_maxMagVal < l_val) {
            l_maxMagVal = l_val;
            l_index = i;
        }
    }
    p_res = l_index;
}
#endif
