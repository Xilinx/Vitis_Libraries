/*
 * Copyright 2021 Xilinx, Inc.
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

/*
This file is the test harness for the fft_ifft_dit_1ch graph class.
*/

#include <stdio.h>
#include "test.hpp"

#if (PARALLEL_POWER == 0)
#if (API_IO == 0)
simulation::platform<1, 1> platform(QUOTE(INPUT_FILE), QUOTE(OUTPUT_FILE));
#else
simulation::platform<2, 2> platform(QUOTE(INPUT_FILE0), QUOTE(INPUT_FILE1), QUOTE(OUTPUT_FILE0), QUOTE(OUTPUT_FILE1));
#endif
#elif (PARALLEL_POWER == 1)
simulation::platform<4, 4> platform(QUOTE(INPUT_FILE0),
                                    QUOTE(INPUT_FILE1),
                                    QUOTE(INPUT_FILE2),
                                    QUOTE(INPUT_FILE3),
                                    QUOTE(OUTPUT_FILE0),
                                    QUOTE(OUTPUT_FILE1),
                                    QUOTE(OUTPUT_FILE2),
                                    QUOTE(OUTPUT_FILE3));
#elif (PARALLEL_POWER == 2)
simulation::platform<8, 8> platform(QUOTE(INPUT_FILE0),
                                    QUOTE(INPUT_FILE1),
                                    QUOTE(INPUT_FILE2),
                                    QUOTE(INPUT_FILE3),
                                    QUOTE(INPUT_FILE4),
                                    QUOTE(INPUT_FILE5),
                                    QUOTE(INPUT_FILE6),
                                    QUOTE(INPUT_FILE7),
                                    QUOTE(OUTPUT_FILE0),
                                    QUOTE(OUTPUT_FILE1),
                                    QUOTE(OUTPUT_FILE2),
                                    QUOTE(OUTPUT_FILE3),
                                    QUOTE(OUTPUT_FILE4),
                                    QUOTE(OUTPUT_FILE5),
                                    QUOTE(OUTPUT_FILE6),
                                    QUOTE(OUTPUT_FILE7));
#elif (PARALLEL_POWER == 3)
simulation::platform<16, 16> platform(QUOTE(INPUT_FILE0),
                                      QUOTE(INPUT_FILE1),
                                      QUOTE(INPUT_FILE2),
                                      QUOTE(INPUT_FILE3),
                                      QUOTE(INPUT_FILE4),
                                      QUOTE(INPUT_FILE5),
                                      QUOTE(INPUT_FILE6),
                                      QUOTE(INPUT_FILE7),
                                      QUOTE(INPUT_FILE8),
                                      QUOTE(INPUT_FILE9),
                                      QUOTE(INPUT_FILE10),
                                      QUOTE(INPUT_FILE11),
                                      QUOTE(INPUT_FILE12),
                                      QUOTE(INPUT_FILE13),
                                      QUOTE(INPUT_FILE14),
                                      QUOTE(INPUT_FILE15),
                                      QUOTE(OUTPUT_FILE0),
                                      QUOTE(OUTPUT_FILE1),
                                      QUOTE(OUTPUT_FILE2),
                                      QUOTE(OUTPUT_FILE3),
                                      QUOTE(OUTPUT_FILE4),
                                      QUOTE(OUTPUT_FILE5),
                                      QUOTE(OUTPUT_FILE6),
                                      QUOTE(OUTPUT_FILE7),
                                      QUOTE(OUTPUT_FILE8),
                                      QUOTE(OUTPUT_FILE9),
                                      QUOTE(OUTPUT_FILE10),
                                      QUOTE(OUTPUT_FILE11),
                                      QUOTE(OUTPUT_FILE12),
                                      QUOTE(OUTPUT_FILE13),
                                      QUOTE(OUTPUT_FILE14),
                                      QUOTE(OUTPUT_FILE15));
#elif (PARALLEL_POWER == 4)
simulation::platform<32, 32> platform(QUOTE(INPUT_FILE0),
                                      QUOTE(INPUT_FILE1),
                                      QUOTE(INPUT_FILE2),
                                      QUOTE(INPUT_FILE3),
                                      QUOTE(INPUT_FILE4),
                                      QUOTE(INPUT_FILE5),
                                      QUOTE(INPUT_FILE6),
                                      QUOTE(INPUT_FILE7),
                                      QUOTE(INPUT_FILE8),
                                      QUOTE(INPUT_FILE9),
                                      QUOTE(INPUT_FILE10),
                                      QUOTE(INPUT_FILE11),
                                      QUOTE(INPUT_FILE12),
                                      QUOTE(INPUT_FILE13),
                                      QUOTE(INPUT_FILE14),
                                      QUOTE(INPUT_FILE15),
                                      QUOTE(INPUT_FILE16),
                                      QUOTE(INPUT_FILE17),
                                      QUOTE(INPUT_FILE18),
                                      QUOTE(INPUT_FILE19),
                                      QUOTE(INPUT_FILE20),
                                      QUOTE(INPUT_FILE21),
                                      QUOTE(INPUT_FILE22),
                                      QUOTE(INPUT_FILE23),
                                      QUOTE(INPUT_FILE24),
                                      QUOTE(INPUT_FILE25),
                                      QUOTE(INPUT_FILE26),
                                      QUOTE(INPUT_FILE27),
                                      QUOTE(INPUT_FILE28),
                                      QUOTE(INPUT_FILE29),
                                      QUOTE(INPUT_FILE30),
                                      QUOTE(INPUT_FILE31),
                                      QUOTE(OUTPUT_FILE0),
                                      QUOTE(OUTPUT_FILE1),
                                      QUOTE(OUTPUT_FILE2),
                                      QUOTE(OUTPUT_FILE3),
                                      QUOTE(OUTPUT_FILE4),
                                      QUOTE(OUTPUT_FILE5),
                                      QUOTE(OUTPUT_FILE6),
                                      QUOTE(OUTPUT_FILE7),
                                      QUOTE(OUTPUT_FILE8),
                                      QUOTE(OUTPUT_FILE9),
                                      QUOTE(OUTPUT_FILE10),
                                      QUOTE(OUTPUT_FILE11),
                                      QUOTE(OUTPUT_FILE12),
                                      QUOTE(OUTPUT_FILE13),
                                      QUOTE(OUTPUT_FILE14),
                                      QUOTE(OUTPUT_FILE15),
                                      QUOTE(OUTPUT_FILE16),
                                      QUOTE(OUTPUT_FILE17),
                                      QUOTE(OUTPUT_FILE18),
                                      QUOTE(OUTPUT_FILE19),
                                      QUOTE(OUTPUT_FILE20),
                                      QUOTE(OUTPUT_FILE21),
                                      QUOTE(OUTPUT_FILE22),
                                      QUOTE(OUTPUT_FILE23),
                                      QUOTE(OUTPUT_FILE24),
                                      QUOTE(OUTPUT_FILE25),
                                      QUOTE(OUTPUT_FILE26),
                                      QUOTE(OUTPUT_FILE27),
                                      QUOTE(OUTPUT_FILE28),
                                      QUOTE(OUTPUT_FILE29),
                                      QUOTE(OUTPUT_FILE30),
                                      QUOTE(OUTPUT_FILE31));
#endif

xf::dsp::aie::testcase::test_graph fft_tb;
/* An failed attempt to make the connection code more succinct using a recursive function macro
#define DEFCONN(XX) (((XX)==0) ? connect<>(platform.src[0], fft.in[0]); connect<>(fft.out[0], platform.sink[0]); :
connect<>(platform.src[(XX)], fft.in[(XX)]); connect<>(fft.out[(XX)], platform.sink[(XX)]); DEFCONN((XX)-1))

#define NUMCONNS ((1+API_IO)<<PARALLEL_POWER)
DEFCONN(NUMCONNS)
*/

#if (PARALLEL_POWER == 0)
#if (API_IO == 0)
connect<> neti0(platform.src[0], fft_tb.in[0]);
connect<> neto0(fft_tb.out[0], platform.sink[0]);
#else
connect<> neti0(platform.src[0], fft_tb.in[0]);
connect<> neti1(platform.src[1], fft_tb.in[1]);
connect<> neto0(fft_tb.out[0], platform.sink[0]);
connect<> neto1(fft_tb.out[1], platform.sink[1]);
#endif
#elif (PARALLEL_POWER == 1)
connect<> neti0(platform.src[0], fft_tb.in[0]);
connect<> neti1(platform.src[1], fft_tb.in[1]);
connect<> neti2(platform.src[2], fft_tb.in[2]);
connect<> neti3(platform.src[3], fft_tb.in[3]);
connect<> neto0(fft_tb.out[0], platform.sink[0]);
connect<> neto1(fft_tb.out[1], platform.sink[1]);
connect<> neto2(fft_tb.out[2], platform.sink[2]);
connect<> neto3(fft_tb.out[3], platform.sink[3]);
#elif (PARALLEL_POWER == 2)
connect<> neti0(platform.src[0], fft_tb.in[0]);
connect<> neti1(platform.src[1], fft_tb.in[1]);
connect<> neti2(platform.src[2], fft_tb.in[2]);
connect<> neti3(platform.src[3], fft_tb.in[3]);
connect<> neti4(platform.src[4], fft_tb.in[4]);
connect<> neti5(platform.src[5], fft_tb.in[5]);
connect<> neti6(platform.src[6], fft_tb.in[6]);
connect<> neti7(platform.src[7], fft_tb.in[7]);
connect<> neto0(fft_tb.out[0], platform.sink[0]);
connect<> neto1(fft_tb.out[1], platform.sink[1]);
connect<> neto2(fft_tb.out[2], platform.sink[2]);
connect<> neto3(fft_tb.out[3], platform.sink[3]);
connect<> neto4(fft_tb.out[4], platform.sink[4]);
connect<> neto5(fft_tb.out[5], platform.sink[5]);
connect<> neto6(fft_tb.out[6], platform.sink[6]);
connect<> neto7(fft_tb.out[7], platform.sink[7]);
#elif (PARALLEL_POWER == 3)
connect<> neti0(platform.src[0], fft_tb.in[0]);
connect<> neti1(platform.src[1], fft_tb.in[1]);
connect<> neti2(platform.src[2], fft_tb.in[2]);
connect<> neti3(platform.src[3], fft_tb.in[3]);
connect<> neti4(platform.src[4], fft_tb.in[4]);
connect<> neti5(platform.src[5], fft_tb.in[5]);
connect<> neti6(platform.src[6], fft_tb.in[6]);
connect<> neti7(platform.src[7], fft_tb.in[7]);
connect<> neti8(platform.src[8], fft_tb.in[8]);
connect<> neti9(platform.src[9], fft_tb.in[9]);
connect<> neti10(platform.src[10], fft_tb.in[10]);
connect<> neti11(platform.src[11], fft_tb.in[11]);
connect<> neti12(platform.src[12], fft_tb.in[12]);
connect<> neti13(platform.src[13], fft_tb.in[13]);
connect<> neti14(platform.src[14], fft_tb.in[14]);
connect<> neti15(platform.src[15], fft_tb.in[15]);
connect<> neto0(fft_tb.out[0], platform.sink[0]);
connect<> neto1(fft_tb.out[1], platform.sink[1]);
connect<> neto2(fft_tb.out[2], platform.sink[2]);
connect<> neto3(fft_tb.out[3], platform.sink[3]);
connect<> neto4(fft_tb.out[4], platform.sink[4]);
connect<> neto5(fft_tb.out[5], platform.sink[5]);
connect<> neto6(fft_tb.out[6], platform.sink[6]);
connect<> neto7(fft_tb.out[7], platform.sink[7]);
connect<> neto8(fft_tb.out[8], platform.sink[8]);
connect<> neto9(fft_tb.out[9], platform.sink[9]);
connect<> neto10(fft_tb.out[10], platform.sink[10]);
connect<> neto11(fft_tb.out[11], platform.sink[11]);
connect<> neto12(fft_tb.out[12], platform.sink[12]);
connect<> neto13(fft_tb.out[13], platform.sink[13]);
connect<> neto14(fft_tb.out[14], platform.sink[14]);
connect<> neto15(fft_tb.out[15], platform.sink[15]);
#elif (PARALLEL_POWER == 4)
connect<> neti0(platform.src[0], fft_tb.in[0]);
connect<> neti1(platform.src[1], fft_tb.in[1]);
connect<> neti2(platform.src[2], fft_tb.in[2]);
connect<> neti3(platform.src[3], fft_tb.in[3]);
connect<> neti4(platform.src[4], fft_tb.in[4]);
connect<> neti5(platform.src[5], fft_tb.in[5]);
connect<> neti6(platform.src[6], fft_tb.in[6]);
connect<> neti7(platform.src[7], fft_tb.in[7]);
connect<> neti8(platform.src[8], fft_tb.in[8]);
connect<> neti9(platform.src[9], fft_tb.in[9]);
connect<> neti10(platform.src[10], fft_tb.in[10]);
connect<> neti11(platform.src[11], fft_tb.in[11]);
connect<> neti12(platform.src[12], fft_tb.in[12]);
connect<> neti13(platform.src[13], fft_tb.in[13]);
connect<> neti14(platform.src[14], fft_tb.in[14]);
connect<> neti15(platform.src[15], fft_tb.in[15]);
connect<> neti16(platform.src[16], fft_tb.in[16]);
connect<> neti17(platform.src[17], fft_tb.in[17]);
connect<> neti18(platform.src[18], fft_tb.in[18]);
connect<> neti19(platform.src[19], fft_tb.in[19]);
connect<> neti20(platform.src[20], fft_tb.in[20]);
connect<> neti21(platform.src[21], fft_tb.in[21]);
connect<> neti22(platform.src[22], fft_tb.in[22]);
connect<> neti23(platform.src[23], fft_tb.in[23]);
connect<> neti24(platform.src[24], fft_tb.in[24]);
connect<> neti25(platform.src[25], fft_tb.in[25]);
connect<> neti26(platform.src[26], fft_tb.in[26]);
connect<> neti27(platform.src[27], fft_tb.in[27]);
connect<> neti28(platform.src[28], fft_tb.in[28]);
connect<> neti29(platform.src[29], fft_tb.in[29]);
connect<> neti30(platform.src[30], fft_tb.in[30]);
connect<> neti31(platform.src[31], fft_tb.in[31]);
connect<> neto0(fft_tb.out[0], platform.sink[0]);
connect<> neto1(fft_tb.out[1], platform.sink[1]);
connect<> neto2(fft_tb.out[2], platform.sink[2]);
connect<> neto3(fft_tb.out[3], platform.sink[3]);
connect<> neto4(fft_tb.out[4], platform.sink[4]);
connect<> neto5(fft_tb.out[5], platform.sink[5]);
connect<> neto6(fft_tb.out[6], platform.sink[6]);
connect<> neto7(fft_tb.out[7], platform.sink[7]);
connect<> neto8(fft_tb.out[8], platform.sink[8]);
connect<> neto9(fft_tb.out[9], platform.sink[9]);
connect<> neto10(fft_tb.out[10], platform.sink[10]);
connect<> neto11(fft_tb.out[11], platform.sink[11]);
connect<> neto12(fft_tb.out[12], platform.sink[12]);
connect<> neto13(fft_tb.out[13], platform.sink[13]);
connect<> neto14(fft_tb.out[14], platform.sink[14]);
connect<> neto15(fft_tb.out[15], platform.sink[15]);
connect<> neto16(fft_tb.out[16], platform.sink[16]);
connect<> neto17(fft_tb.out[17], platform.sink[17]);
connect<> neto18(fft_tb.out[18], platform.sink[18]);
connect<> neto19(fft_tb.out[19], platform.sink[19]);
connect<> neto20(fft_tb.out[20], platform.sink[20]);
connect<> neto21(fft_tb.out[21], platform.sink[21]);
connect<> neto22(fft_tb.out[22], platform.sink[22]);
connect<> neto23(fft_tb.out[23], platform.sink[23]);
connect<> neto24(fft_tb.out[24], platform.sink[24]);
connect<> neto25(fft_tb.out[25], platform.sink[25]);
connect<> neto26(fft_tb.out[26], platform.sink[26]);
connect<> neto27(fft_tb.out[27], platform.sink[27]);
connect<> neto28(fft_tb.out[28], platform.sink[28]);
connect<> neto29(fft_tb.out[29], platform.sink[29]);
connect<> neto30(fft_tb.out[30], platform.sink[30]);
connect<> neto31(fft_tb.out[31], platform.sink[31]);
#endif

int main(void) {
    printf("\n");
    printf("========================\n");
    printf("UUT: ");
    printf(QUOTE(UUT_GRAPH));
    printf("\n");
    printf("========================\n");
    printf("Input samples      = %d \n", INPUT_SAMPLES);
    printf("Output samples     = %d \n", OUTPUT_SAMPLES);
    printf("Point Size         = %d \n", POINT_SIZE);
    printf("FFT/nIFFT          = %d \n", FFT_NIFFT);
    printf("Shift              = %d \n", SHIFT);
    printf("Kernels            = %d \n", CASC_LEN);
    printf("Dynamic point size = %d \n", DYN_PT_SIZE);
    printf("Window Size        = %d \n", WINDOW_VSIZE);
    printf("API_IO             = %d \n", API_IO);
    printf("Parallel Power     = %d \n", PARALLEL_POWER);
    printf("Data type          = ");
    printf(QUOTE(DATA_TYPE));
    printf("\n");
    printf("Twiddle type       = ");
    printf(QUOTE(TWIDDLE_TYPE));
    printf("\n");

    fft_tb.init();
    fft_tb.run(NITER);
    fft_tb.end();

    return 0;
}
