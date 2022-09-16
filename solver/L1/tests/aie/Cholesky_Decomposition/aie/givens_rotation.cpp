#include "aie_kernels.hpp"
#include "aie_api/aie.hpp"

void givens_rotation(input_stream<float>* lower_row,
                     input_stream<float>* higher_row,
                     output_stream<float>* update_lower_row,
                     output_stream<float>* update_higher_row,
                     unsigned int row_num,
                     unsigned int column_num) {
    for (int j = 0; j < column_num - 1; j++) {
        for (int i = row_num - 1; i > j; i--) {
            // givens param
            float c, s;
            float a11 = readincr(lower_row);
            float a21 = readincr(higher_row);
            if (a21 == 0.0) {
                c = 1;
                s = 0;
            } else {
                float cot = a11 / a21;
                float tmp = 1.0 + cot * cot;
                s = 1.0 / aie::sqrt(tmp);
                c = s * cot;
            }
            // givens rotation
            for (int b = 0; b < column_num; b++) {
                float Ajb = readincr(lower_row);
                float Aib = readincr(higher_row);
                float Rjb = c * Ajb + s * Aib;
                float Rib = -s * Ajb + c * Aib;
                writeincr(update_lower_row, Rjb);
                writeincr(update_higher_row, Rib);
            }
        }
    }
}
