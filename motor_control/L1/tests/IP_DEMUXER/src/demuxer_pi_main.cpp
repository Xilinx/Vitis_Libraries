#include <iostream>
#include "common_definitions.hpp"

int main(void) {
    hls::stream<ap_uint<BIT_WIDTH_STREAM_FOC> > input_data_stream, output_data_stream;
    hls::stream<ap_int<BIT_WIDTH_DATA> > input_data_w_1, input_data_w_2, input_data_w_3, input_data_w_4, input_data_w_5;
    hls::stream<int32_t> input_data_w_i, output_data_w_i;
    hls::stream<ap_uint<BIT_WIDTH_LOG_STREAM_FOC> > log_in, log_out;

    ap_uint<BIT_WIDTH_STREAM_FOC> input_values_stream, output_values_stream;
    ap_uint<BIT_WIDTH_LOG_STREAM_FOC> input_log_values, output_log_values;
    ap_int<BIT_WIDTH_DATA> input_values_w_u, output_values_w_u;
    int32_t _data_ag = 0;

    volatile int Id_ = 0, Iq_ = 2, mode_ = 2, theta_ = 0;
    input_data_stream.write(64); // last 8 bit enabled
    log_in.write(64);            // last 8 bit enabled
    input_data_w_1.write(64);    // last 8 bit enabled
    demuxer_pi_inst(input_data_stream, input_data_w_1, input_data_w_2, input_data_w_3, input_data_w_4, input_data_w_5,
                    log_in, log_out, mode_, Id_, Iq_, theta_);

    int cnt_error = 1;
    while (!input_data_w_5.empty()) {
        std::cout << "VAL: " << input_data_w_5.read() << std::endl;
        cnt_error = 0;
    }

    log_out.read();
    input_data_w_2.read();
    input_data_w_3.read();
    input_data_w_4.read();

    return cnt_error;
}
