/*
 * MIT License
 *
 * Copyright (C) 2023 Advanced Micro Devices, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated
 * documentation files (the “Software”), to deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 * WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Except as contained in this notice, the name of Advanced Micro Devices, Inc. shall not be used in advertising or
 * otherwise to promote the sale, use or other dealings in this Software without prior written authorization from
 * Advanced Micro Devices, Inc.
 */

#ifndef _VCK190_TEST_HARNESS_HPP_
#define _VCK190_TEST_HARNESS_HPP_

#include <ap_int.h>
#include <ap_axi_sdata.h>
#include <hls_stream.h>
#include <hls_burst_maxi.h>
#ifndef __SYNTHESIS__
#include <iostream>
#endif

/**
 * @brief buffer for test harness
 *
 * @tparam W Bit width of AXI-M, AXI-Stream and URAM buffer width. 128 default.
 * @tparam D Depth of URAM buffer for input/output.
 */
template <int W, int D>
class buff_channel {
   public:
#ifndef __SYNTHESIS__
    ap_uint<W>* buff;
#else
    ap_uint<W> buff[D];
#endif

    buff_channel() {
#ifndef __SYNTHESIS__
        buff = (ap_uint<W>*)malloc(sizeof(ap_uint<W>) * D);
#else
#pragma HLS inline
#pragma HLS bind_storage variable = buff type = RAM_1P impl = URAM
#endif
    }

    ~buff_channel() {
#ifndef __SYNTHESIS__
        free(buff);
#endif
    }

    void load_buff(ap_uint<W>* source, int size_in_frame) {
        for (int i = 0; i < size_in_frame; i++) {
#pragma HLS pipeline II = 1
            buff[i] = source[i];
        }
    }

    void load_stream(
        hls::stream<ap_axiu<W, 0, 0, 0> >& to_strm, ap_uint<64> delay, int frames, int rep, ap_uint<64>& end) {
#pragma HLS dataflow
        hls::stream<bool> trigger_strm;
#pragma HLS stream variable = trigger_strm depth = 4
        feed(to_strm, delay, frames, rep, trigger_strm);
        counter(trigger_strm, end);
    }

    void store_buff(ap_uint<W>* dst, int size_in_frame) {
        for (int i = 0; i < size_in_frame; i++) {
#pragma HLS pipeline II = 1
            dst[i] = buff[i];
        }
    }

    void store_stream(
        hls::stream<ap_axiu<W, 0, 0, 0> >& from_strm, ap_uint<64> delay, int frames, int rep, ap_uint<64>& end) {
#pragma HLS dataflow
        hls::stream<bool> trigger_strm;
#pragma HLS stream variable = trigger_strm depth = 4
        get(from_strm, delay, frames, rep, trigger_strm);
        counter(trigger_strm, end);
    }

   private:
    void counter(hls::stream<bool>& trigger_strm, ap_uint<64>& end) {
        bool succ_start = false;
        bool succ_end = false;
        ap_uint<64> counter = 0;
        while (!succ_start) {
#pragma HLS pipeline II = 1
            counter++;
            bool tmp;
            succ_start = trigger_strm.read_nb(tmp);
        }
        while (!succ_end) {
#pragma HLS pipeline II = 1
            counter++;
            bool tmp;
            succ_end = trigger_strm.read_nb(tmp);
            if (succ_end) {
                end = counter;
            }
        }
    }

    void feed(hls::stream<ap_axiu<W, 0, 0, 0> >& to_strm,
              ap_uint<64> delay,
              int frames,
              int rep,
              hls::stream<bool>& trigger_strm) {
        // delay and send one trigger
        if (delay != 0) {
            while (delay != 0) {
#pragma HLS pipeline II = 1
                delay--;
                if (delay == 0) {
                    trigger_strm.write(true);
                }
            }
        } else {
            trigger_strm.write(true);
        }
        //
        for (int j = 0; j < rep; j++) {
            for (int i = 0; i < frames; i++) {
#pragma HLS pipeline II = 1
                ap_axiu<W, 0, 0, 0> tmp;
                tmp.data = buff[i];
                tmp.keep = -1;
                tmp.last = 0;
                to_strm.write(tmp);
            }
        }
        trigger_strm.write(true);
    }

    void get(hls::stream<ap_axiu<W, 0, 0, 0> >& from_strm,
             ap_uint<64> delay,
             int frames,
             int rep,
             hls::stream<bool>& trigger_strm) {
        if (delay != 0) {
            while (delay != 0) {
#pragma HLS pipeline II = 1
                delay--;
                if (delay == 0) {
                    trigger_strm.write(true);
                }
            }
        } else {
            trigger_strm.write(true);
        }
        for (int j = 0; j < rep; j++) {
            for (int i = 0; i < frames; i++) {
#pragma HLS pipeline II = 1
                ap_axiu<W, 0, 0, 0> tmp = from_strm.read();
                buff[i] = tmp.data;
            }
        }
        trigger_strm.write(true);
    }
};

/**
 * @brief test harness
 *
 * @tparam W Bit width of AXI-M, AXI-Stream and URAM buffer width. 128 default.
 * @tparam D Depth of URAM buffer for input/output.
 * @tparam N Number of channel to feed AIE and get from AIE. Should be 1, 2, 4, 8, 16
 */
template <int W, int D, int N>
class test_harness {
   public:
    buff_channel<W, D> to_aie_ch[N];
    buff_channel<W, D> from_aie_ch[N];
    ap_uint<64> to_aie_delay[N];
    ap_uint<64> to_aie_last[N];
    ap_uint<64> to_aie_frame[N];
    ap_uint<64> to_aie_rep[N];
    ap_uint<64> from_aie_delay[N];
    ap_uint<64> from_aie_last[N];
    ap_uint<64> from_aie_frame[N];
    ap_uint<64> from_aie_rep[N];

    test_harness() {
#pragma HLS inline
#pragma HLS array_partition variable = to_aie_ch type = complete dim = 1
#pragma HLS array_partition variable = from_aie_ch type = complete dim = 1
#pragma HLS array_partition variable = to_aie_delay type = complete dim = 1
#pragma HLS array_partition variable = to_aie_last type = complete dim = 1
#pragma HLS array_partition variable = to_aie_frame type = complete dim = 1
#pragma HLS array_partition variable = to_aie_rep type = complete dim = 1
#pragma HLS array_partition variable = from_aie_delay type = complete dim = 1
#pragma HLS array_partition variable = from_aie_last type = complete dim = 1
#pragma HLS array_partition variable = from_aie_frame type = complete dim = 1
#pragma HLS array_partition variable = from_aie_rep type = complete dim = 1
    }

    void load_cfg(ap_uint<64>* cfg) {
        for (int i = 0; i < N; i++) {
            to_aie_delay[i] = cfg[i];
        }

        for (int i = 0; i < N; i++) {
            to_aie_frame[i] = cfg[i + N];
        }

        for (int i = 0; i < N; i++) {
            to_aie_rep[i] = cfg[i + N * 2];
        }

        for (int i = 0; i < N; i++) {
            from_aie_delay[i] = cfg[i + N * 3];
        }

        for (int i = 0; i < N; i++) {
            from_aie_frame[i] = cfg[i + N * 4];
        }

        for (int i = 0; i < N; i++) {
            from_aie_rep[i] = cfg[i + N * 5];
        }
    }

    void store_perf(ap_uint<64>* perf) {
        for (int i = 0; i < N; i++) {
            perf[i] = to_aie_last[i];
        }

        for (int i = 0; i < N; i++) {
            perf[i + N] = from_aie_last[i];
        }
    }

    void load_buff(ap_uint<W>* data) {
        for (int i = 0; i < N; i++) {
            to_aie_ch[i].load_buff(data + i * D, to_aie_frame[i]);
        }
    }

    void store_buff(ap_uint<W>* data) {
        for (int i = 0; i < N; i++) {
            from_aie_ch[i].store_buff(data + i * D, from_aie_frame[i]);
        }
    }

    void load_store_stream(hls::stream<ap_axiu<W, 0, 0, 0> >& to_aie_strm0,
                           hls::stream<ap_axiu<W, 0, 0, 0> >& from_aie_strm0) {
#pragma HLS dataflow

        to_aie_ch[0].load_stream(to_aie_strm0, to_aie_delay[0], to_aie_frame[0], to_aie_rep[0], to_aie_last[0]);

        from_aie_ch[0].store_stream(from_aie_strm0, from_aie_delay[0], from_aie_frame[0], from_aie_rep[0],
                                    from_aie_last[0]);
    }

    void load_store_stream(hls::stream<ap_axiu<W, 0, 0, 0> >& to_aie_strm0,
                           hls::stream<ap_axiu<W, 0, 0, 0> >& to_aie_strm1,
                           hls::stream<ap_axiu<W, 0, 0, 0> >& from_aie_strm0,
                           hls::stream<ap_axiu<W, 0, 0, 0> >& from_aie_strm1) {
#pragma HLS dataflow
        to_aie_ch[0].load_stream(to_aie_strm0, to_aie_delay[0], to_aie_frame[0], to_aie_rep[0], to_aie_last[0]);
        to_aie_ch[1].load_stream(to_aie_strm1, to_aie_delay[1], to_aie_frame[1], to_aie_rep[1], to_aie_last[1]);

        from_aie_ch[0].store_stream(from_aie_strm0, from_aie_delay[0], from_aie_frame[0], from_aie_rep[0],
                                    from_aie_last[0]);
        from_aie_ch[1].store_stream(from_aie_strm1, from_aie_delay[1], from_aie_frame[1], from_aie_rep[1],
                                    from_aie_last[1]);
    }

    void load_store_stream(hls::stream<ap_axiu<W, 0, 0, 0> >& to_aie_strm0,
                           hls::stream<ap_axiu<W, 0, 0, 0> >& to_aie_strm1,
                           hls::stream<ap_axiu<W, 0, 0, 0> >& to_aie_strm2,
                           hls::stream<ap_axiu<W, 0, 0, 0> >& to_aie_strm3,
                           hls::stream<ap_axiu<W, 0, 0, 0> >& from_aie_strm0,
                           hls::stream<ap_axiu<W, 0, 0, 0> >& from_aie_strm1,
                           hls::stream<ap_axiu<W, 0, 0, 0> >& from_aie_strm2,
                           hls::stream<ap_axiu<W, 0, 0, 0> >& from_aie_strm3) {
#pragma HLS dataflow

        to_aie_ch[0].load_stream(to_aie_strm0, to_aie_delay[0], to_aie_frame[0], to_aie_rep[0], to_aie_last[0]);
        to_aie_ch[1].load_stream(to_aie_strm1, to_aie_delay[1], to_aie_frame[1], to_aie_rep[1], to_aie_last[1]);
        to_aie_ch[2].load_stream(to_aie_strm2, to_aie_delay[2], to_aie_frame[2], to_aie_rep[2], to_aie_last[2]);
        to_aie_ch[3].load_stream(to_aie_strm3, to_aie_delay[3], to_aie_frame[3], to_aie_rep[3], to_aie_last[3]);

        from_aie_ch[0].store_stream(from_aie_strm0, from_aie_delay[0], from_aie_frame[0], from_aie_rep[0],
                                    from_aie_last[0]);
        from_aie_ch[1].store_stream(from_aie_strm1, from_aie_delay[1], from_aie_frame[1], from_aie_rep[1],
                                    from_aie_last[1]);
        from_aie_ch[2].store_stream(from_aie_strm2, from_aie_delay[2], from_aie_frame[2], from_aie_rep[2],
                                    from_aie_last[2]);
        from_aie_ch[3].store_stream(from_aie_strm3, from_aie_delay[3], from_aie_frame[3], from_aie_rep[3],
                                    from_aie_last[3]);
    }

    void load_store_stream(hls::stream<ap_axiu<W, 0, 0, 0> >& to_aie_strm0,
                           hls::stream<ap_axiu<W, 0, 0, 0> >& to_aie_strm1,
                           hls::stream<ap_axiu<W, 0, 0, 0> >& to_aie_strm2,
                           hls::stream<ap_axiu<W, 0, 0, 0> >& to_aie_strm3,
                           hls::stream<ap_axiu<W, 0, 0, 0> >& to_aie_strm4,
                           hls::stream<ap_axiu<W, 0, 0, 0> >& to_aie_strm5,
                           hls::stream<ap_axiu<W, 0, 0, 0> >& to_aie_strm6,
                           hls::stream<ap_axiu<W, 0, 0, 0> >& to_aie_strm7,
                           hls::stream<ap_axiu<W, 0, 0, 0> >& from_aie_strm0,
                           hls::stream<ap_axiu<W, 0, 0, 0> >& from_aie_strm1,
                           hls::stream<ap_axiu<W, 0, 0, 0> >& from_aie_strm2,
                           hls::stream<ap_axiu<W, 0, 0, 0> >& from_aie_strm3,
                           hls::stream<ap_axiu<W, 0, 0, 0> >& from_aie_strm4,
                           hls::stream<ap_axiu<W, 0, 0, 0> >& from_aie_strm5,
                           hls::stream<ap_axiu<W, 0, 0, 0> >& from_aie_strm6,
                           hls::stream<ap_axiu<W, 0, 0, 0> >& from_aie_strm7) {
#pragma HLS dataflow
        to_aie_ch[0].load_stream(to_aie_strm0, to_aie_delay[0], to_aie_frame[0], to_aie_rep[0], to_aie_last[0]);
        to_aie_ch[1].load_stream(to_aie_strm1, to_aie_delay[1], to_aie_frame[1], to_aie_rep[1], to_aie_last[1]);
        to_aie_ch[2].load_stream(to_aie_strm2, to_aie_delay[2], to_aie_frame[2], to_aie_rep[2], to_aie_last[2]);
        to_aie_ch[3].load_stream(to_aie_strm3, to_aie_delay[3], to_aie_frame[3], to_aie_rep[3], to_aie_last[3]);
        to_aie_ch[4].load_stream(to_aie_strm4, to_aie_delay[4], to_aie_frame[4], to_aie_rep[4], to_aie_last[4]);
        to_aie_ch[5].load_stream(to_aie_strm5, to_aie_delay[5], to_aie_frame[5], to_aie_rep[5], to_aie_last[5]);
        to_aie_ch[6].load_stream(to_aie_strm6, to_aie_delay[6], to_aie_frame[6], to_aie_rep[6], to_aie_last[6]);
        to_aie_ch[7].load_stream(to_aie_strm7, to_aie_delay[7], to_aie_frame[7], to_aie_rep[7], to_aie_last[7]);

        from_aie_ch[0].store_stream(from_aie_strm0, from_aie_delay[0], from_aie_frame[0], from_aie_rep[0],
                                    from_aie_last[0]);
        from_aie_ch[1].store_stream(from_aie_strm1, from_aie_delay[1], from_aie_frame[1], from_aie_rep[1],
                                    from_aie_last[1]);
        from_aie_ch[2].store_stream(from_aie_strm2, from_aie_delay[2], from_aie_frame[2], from_aie_rep[2],
                                    from_aie_last[2]);
        from_aie_ch[3].store_stream(from_aie_strm3, from_aie_delay[3], from_aie_frame[3], from_aie_rep[3],
                                    from_aie_last[3]);
        from_aie_ch[4].store_stream(from_aie_strm4, from_aie_delay[4], from_aie_frame[4], from_aie_rep[4],
                                    from_aie_last[4]);
        from_aie_ch[5].store_stream(from_aie_strm5, from_aie_delay[5], from_aie_frame[5], from_aie_rep[5],
                                    from_aie_last[5]);
        from_aie_ch[6].store_stream(from_aie_strm6, from_aie_delay[6], from_aie_frame[6], from_aie_rep[6],
                                    from_aie_last[6]);
        from_aie_ch[7].store_stream(from_aie_strm7, from_aie_delay[7], from_aie_frame[7], from_aie_rep[7],
                                    from_aie_last[7]);
    }

    void load_store_stream(hls::stream<ap_axiu<W, 0, 0, 0> >& to_aie_strm0,
                           hls::stream<ap_axiu<W, 0, 0, 0> >& to_aie_strm1,
                           hls::stream<ap_axiu<W, 0, 0, 0> >& to_aie_strm2,
                           hls::stream<ap_axiu<W, 0, 0, 0> >& to_aie_strm3,
                           hls::stream<ap_axiu<W, 0, 0, 0> >& to_aie_strm4,
                           hls::stream<ap_axiu<W, 0, 0, 0> >& to_aie_strm5,
                           hls::stream<ap_axiu<W, 0, 0, 0> >& to_aie_strm6,
                           hls::stream<ap_axiu<W, 0, 0, 0> >& to_aie_strm7,
                           hls::stream<ap_axiu<W, 0, 0, 0> >& to_aie_strm8,
                           hls::stream<ap_axiu<W, 0, 0, 0> >& to_aie_strm9,
                           hls::stream<ap_axiu<W, 0, 0, 0> >& to_aie_strm10,
                           hls::stream<ap_axiu<W, 0, 0, 0> >& to_aie_strm11,
                           hls::stream<ap_axiu<W, 0, 0, 0> >& to_aie_strm12,
                           hls::stream<ap_axiu<W, 0, 0, 0> >& to_aie_strm13,
                           hls::stream<ap_axiu<W, 0, 0, 0> >& to_aie_strm14,
                           hls::stream<ap_axiu<W, 0, 0, 0> >& to_aie_strm15,
                           hls::stream<ap_axiu<W, 0, 0, 0> >& from_aie_strm0,
                           hls::stream<ap_axiu<W, 0, 0, 0> >& from_aie_strm1,
                           hls::stream<ap_axiu<W, 0, 0, 0> >& from_aie_strm2,
                           hls::stream<ap_axiu<W, 0, 0, 0> >& from_aie_strm3,
                           hls::stream<ap_axiu<W, 0, 0, 0> >& from_aie_strm4,
                           hls::stream<ap_axiu<W, 0, 0, 0> >& from_aie_strm5,
                           hls::stream<ap_axiu<W, 0, 0, 0> >& from_aie_strm6,
                           hls::stream<ap_axiu<W, 0, 0, 0> >& from_aie_strm7,
                           hls::stream<ap_axiu<W, 0, 0, 0> >& from_aie_strm8,
                           hls::stream<ap_axiu<W, 0, 0, 0> >& from_aie_strm9,
                           hls::stream<ap_axiu<W, 0, 0, 0> >& from_aie_strm10,
                           hls::stream<ap_axiu<W, 0, 0, 0> >& from_aie_strm11,
                           hls::stream<ap_axiu<W, 0, 0, 0> >& from_aie_strm12,
                           hls::stream<ap_axiu<W, 0, 0, 0> >& from_aie_strm13,
                           hls::stream<ap_axiu<W, 0, 0, 0> >& from_aie_strm14,
                           hls::stream<ap_axiu<W, 0, 0, 0> >& from_aie_strm15) {
#pragma HLS dataflow
        to_aie_ch[0].load_stream(to_aie_strm0, to_aie_delay[0], to_aie_frame[0], to_aie_rep[0], to_aie_last[0]);
        to_aie_ch[1].load_stream(to_aie_strm1, to_aie_delay[1], to_aie_frame[1], to_aie_rep[1], to_aie_last[1]);
        to_aie_ch[2].load_stream(to_aie_strm2, to_aie_delay[2], to_aie_frame[2], to_aie_rep[2], to_aie_last[2]);
        to_aie_ch[3].load_stream(to_aie_strm3, to_aie_delay[3], to_aie_frame[3], to_aie_rep[3], to_aie_last[3]);
        to_aie_ch[4].load_stream(to_aie_strm4, to_aie_delay[4], to_aie_frame[4], to_aie_rep[4], to_aie_last[4]);
        to_aie_ch[5].load_stream(to_aie_strm5, to_aie_delay[5], to_aie_frame[5], to_aie_rep[5], to_aie_last[5]);
        to_aie_ch[6].load_stream(to_aie_strm6, to_aie_delay[6], to_aie_frame[6], to_aie_rep[6], to_aie_last[6]);
        to_aie_ch[7].load_stream(to_aie_strm7, to_aie_delay[7], to_aie_frame[7], to_aie_rep[7], to_aie_last[7]);
        to_aie_ch[8].load_stream(to_aie_strm8, to_aie_delay[8], to_aie_frame[8], to_aie_rep[8], to_aie_last[8]);
        to_aie_ch[9].load_stream(to_aie_strm9, to_aie_delay[9], to_aie_frame[9], to_aie_rep[9], to_aie_last[9]);
        to_aie_ch[10].load_stream(to_aie_strm10, to_aie_delay[10], to_aie_frame[10], to_aie_rep[10], to_aie_last[10]);
        to_aie_ch[11].load_stream(to_aie_strm11, to_aie_delay[11], to_aie_frame[11], to_aie_rep[11], to_aie_last[11]);
        to_aie_ch[12].load_stream(to_aie_strm12, to_aie_delay[12], to_aie_frame[12], to_aie_rep[12], to_aie_last[12]);
        to_aie_ch[13].load_stream(to_aie_strm13, to_aie_delay[13], to_aie_frame[13], to_aie_rep[13], to_aie_last[13]);
        to_aie_ch[14].load_stream(to_aie_strm14, to_aie_delay[14], to_aie_frame[14], to_aie_rep[14], to_aie_last[14]);
        to_aie_ch[15].load_stream(to_aie_strm15, to_aie_delay[15], to_aie_frame[15], to_aie_rep[15], to_aie_last[15]);

        from_aie_ch[0].store_stream(from_aie_strm0, from_aie_delay[0], from_aie_frame[0], from_aie_rep[0],
                                    from_aie_last[0]);
        from_aie_ch[1].store_stream(from_aie_strm1, from_aie_delay[1], from_aie_frame[1], from_aie_rep[1],
                                    from_aie_last[1]);
        from_aie_ch[2].store_stream(from_aie_strm2, from_aie_delay[2], from_aie_frame[2], from_aie_rep[2],
                                    from_aie_last[2]);
        from_aie_ch[3].store_stream(from_aie_strm3, from_aie_delay[3], from_aie_frame[3], from_aie_rep[3],
                                    from_aie_last[3]);
        from_aie_ch[4].store_stream(from_aie_strm4, from_aie_delay[4], from_aie_frame[4], from_aie_rep[4],
                                    from_aie_last[4]);
        from_aie_ch[5].store_stream(from_aie_strm5, from_aie_delay[5], from_aie_frame[5], from_aie_rep[5],
                                    from_aie_last[5]);
        from_aie_ch[6].store_stream(from_aie_strm6, from_aie_delay[6], from_aie_frame[6], from_aie_rep[6],
                                    from_aie_last[6]);
        from_aie_ch[7].store_stream(from_aie_strm7, from_aie_delay[7], from_aie_frame[7], from_aie_rep[7],
                                    from_aie_last[7]);
        from_aie_ch[8].store_stream(from_aie_strm8, from_aie_delay[8], from_aie_frame[8], from_aie_rep[8],
                                    from_aie_last[8]);
        from_aie_ch[9].store_stream(from_aie_strm9, from_aie_delay[9], from_aie_frame[9], from_aie_rep[9],
                                    from_aie_last[9]);
        from_aie_ch[10].store_stream(from_aie_strm10, from_aie_delay[10], from_aie_frame[10], from_aie_rep[10],
                                     from_aie_last[10]);
        from_aie_ch[11].store_stream(from_aie_strm11, from_aie_delay[11], from_aie_frame[11], from_aie_rep[11],
                                     from_aie_last[11]);
        from_aie_ch[12].store_stream(from_aie_strm12, from_aie_delay[12], from_aie_frame[12], from_aie_rep[12],
                                     from_aie_last[12]);
        from_aie_ch[13].store_stream(from_aie_strm13, from_aie_delay[13], from_aie_frame[13], from_aie_rep[13],
                                     from_aie_last[13]);
        from_aie_ch[14].store_stream(from_aie_strm14, from_aie_delay[14], from_aie_frame[14], from_aie_rep[14],
                                     from_aie_last[14]);
        from_aie_ch[15].store_stream(from_aie_strm15, from_aie_delay[15], from_aie_frame[15], from_aie_rep[15],
                                     from_aie_last[15]);
    }
};

#endif
