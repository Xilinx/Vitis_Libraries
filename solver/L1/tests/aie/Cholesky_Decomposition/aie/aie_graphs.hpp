#ifndef __AIE_GRAPHS_HPP_
#define __AIE_GRAPHS_HPP_

#include "aie_kernels.hpp"

using namespace adf;
class GivensRotationQRD : public adf::graph {
   private:
    kernel k1;

   public:
    input_plio in_lower_row;
    input_plio in_higher_row;
    output_plio out_lower_row;
    output_plio out_higher_row;
    input_port row_num;
    input_port column_num;

    GivensRotationQRD(std::string in_lower_row_name,
                      std::string in_lower_file_name,
                      std::string in_higher_row_name,
                      std::string in_higher_file_name,
                      std::string out_lower_row_name,
                      std::string out_lower_file_name,
                      std::string out_higher_row_name,
                      std::string out_higher_file_name) {
        k1 = kernel::create(givens_rotation);
        source(k1) = "aie/givens_rotation.cpp";
        runtime<ratio>(k1) = 1.0;

        in_lower_row = input_plio::create(in_lower_row_name, adf::plio_32_bits, in_lower_file_name);
        in_higher_row = input_plio::create(in_higher_row_name, adf::plio_32_bits, in_higher_file_name);
        out_lower_row = output_plio::create(out_lower_row_name, adf::plio_32_bits, out_lower_file_name);
        out_higher_row = output_plio::create(out_higher_row_name, adf::plio_32_bits, out_higher_file_name);

        connect<stream>(in_lower_row.out[0], k1.in[0]);
        connect<stream>(in_higher_row.out[0], k1.in[1]);
        connect<stream>(k1.out[0], out_lower_row.in[0]);
        connect<stream>(k1.out[1], out_higher_row.in[0]);
        connect<parameter>(row_num, k1.in[2]);
        connect<parameter>(column_num, k1.in[3]);
    }
};

#endif
