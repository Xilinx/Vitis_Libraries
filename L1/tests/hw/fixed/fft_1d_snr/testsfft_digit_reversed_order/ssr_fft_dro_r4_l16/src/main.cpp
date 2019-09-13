

#define SSR_FFT_REGRESSION_TEST
#include "DEBUG_CONSTANTS.hpp"
#include <iostream>
#include <assert.h>
#include <bitset>
#include "hls_ssr_fft_data_path.hpp"
#include "../../../common/utils/spu.hpp"
#include "../../../common/utils/mVerificationUtlityFunctions.hpp"
#include "../../../common/utils/suf.hpp"
#include "../../../common/utils/dsp_utilities.hpp"
#include "../../../common/utils/sorting.hpp"

//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
#ifdef TEST_SSR_FFT

#define DYNAMIC_FUNS

#define NBF SSR_FFT_L / SSR_FFT_R
typedef std::complex<ap_fixed<16, 8> > inT;
typedef std::complex<ap_fixed<22, 14> > outT;
typedef std::complex<ap_fixed<10, 2> > twType;

//////////// FLOATING POINT Model Input Typedefs /////////////////////////////
typedef std::complex<double> Tin_fp;
typedef std::complex<double> Tout_fp;
typedef std::complex<double> Ttw_fp;
typedef std::complex<double> Texp_fp;
//////////////////////////////////////////////////////////////////////////////

void fft_top(T_SSR_FFT_IN inD[SSR_FFT_R][SSR_FFT_L / SSR_FFT_R], T_SSR_FFT_OUT outD[SSR_FFT_R][SSR_FFT_L / SSR_FFT_R]) {
#pragma HLS TOP

    xf::dsp::fft::fft<ssr_fft_fix_params>(inD, outD);
}

void fft_top_c(T_SSR_FFT_IN inD[SSR_FFT_R][SSR_FFT_L / SSR_FFT_R],
               T_SSR_FFT_OUT outD[SSR_FFT_R][SSR_FFT_L / SSR_FFT_R]) {
    xf::dsp::fft::fft<ssr_fft_fix_params>(inD, outD);
}

#ifndef __SYNTHESIS__
int main(int argc, char** argv) {
    std::complex<double> parFFTTable[SSR_FFT_R];
    std::complex<double> twiddleTable[(SSR_FFT_R - 1) * ((SSR_FFT_L / SSR_FFT_R) - 1) + 1];
    const int tw_len = (SSR_FFT_R - 1) * ((SSR_FFT_L / SSR_FFT_R) - 1) + 1;
    // complex_exp_table<SSR_FFT_R,std::complex < double > >::initComplexExp(parFFTTable);
    // twiddle_table<SSR_FFT_L,SSR_FFT_R,std::complex < double > >
    // ::initTwiddleFactors(twiddleTable);//initTwiddleFactors<double, SSR_FFT_L, SSR_FFT_R>(twiddleTable);
    std::complex<tip_fftInType> in[SSR_FFT_R][SSR_FFT_L / SSR_FFT_R];
    std::complex<tip_fftOutType> out[SSR_FFT_R][SSR_FFT_L / SSR_FFT_R];
    std::complex<tip_fftOutType> out1[SSR_FFT_R][SSR_FFT_L / SSR_FFT_R];

    // FLOATING POINT Arrays for Input and Output Data
    Tin_fp in_data[SSR_FFT_R][SSR_FFT_L / SSR_FFT_R];
    Tout_fp out_data[SSR_FFT_R][SSR_FFT_L / SSR_FFT_R];

    int stage = 0;
    int dim1 = SSR_FFT_L / SSR_FFT_R;
    for (int i = 0; i < SSR_FFT_L / SSR_FFT_R; i++) {
        for (int j = 0; j < SSR_FFT_R; j++) {
            /*in[i][j].real = i*(SSR_FFT_R)+j;
            //std::std::endl; << "[" << i << "]" << "[" << j << "]" << "=" << in[i][j] << std::endl;;
            in[i][j].imag = i*(SSR_FFT_R)+j;*/

            in[j][i] =
                std::complex<tip_fftInType>((tip_fftInType)(i * (SSR_FFT_R) + j), (tip_fftInType)(i * (SSR_FFT_R) + j));
        }
    }

    /*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
     * din and dout define 2-dimensional arrays for the storage of input and output complex samples for
     *  complex<double> ssr fft call
     * ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
     */
    std::complex<type_data> din[SSR_FFT_R][SSR_FFT_L / SSR_FFT_R];
    std::complex<type_data> dout_temp[SSR_FFT_R][SSR_FFT_L / SSR_FFT_R];
    std::complex<type_data> dout[SSR_FFT_R][SSR_FFT_L / SSR_FFT_R];
    //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

    /*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
     * din_fix and dout_fix define 2-dimensional arrays for the storage of input and output complex samples for
     *  complex<ap_fixed> ssr fft call that will synthesize to RTL for implementation
     * ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
     */
    T_SSR_FFT_IN din_fix[SSR_FFT_R][SSR_FFT_L / SSR_FFT_R];
    T_SSR_FFT_OUT dout_fix_temp[SSR_FFT_R][SSR_FFT_L / SSR_FFT_R];
    T_SSR_FFT_OUT dout_fix[SSR_FFT_R][SSR_FFT_L / SSR_FFT_R];
    //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

    /*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
     * din_fix_c and dout_fix_c define 2-dimensional arrays for the storage of input and output complex samples for
     *  complex<ap_fixed> ssr fft call that is NOT SYNTHESIZED create bit true output for comparison during COSIM
     * ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
     */
    T_SSR_FFT_IN din_fix_c[SSR_FFT_R][SSR_FFT_L / SSR_FFT_R];
    T_SSR_FFT_OUT dout_fix_c_temp[SSR_FFT_R][SSR_FFT_L / SSR_FFT_R];
    T_SSR_FFT_OUT dout_fix_c[SSR_FFT_R][SSR_FFT_L / SSR_FFT_R];
    //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

    /// dine_file 1-Dimensional array that will be used to store stimulus input data read from the file, stimulus
    // is created using octave scripts.
    std::complex<type_data> din_file[SSR_FFT_L];

    // golden_output_file is 1-D array that is used to read GOLDEN OUTPUT test vectors for functional verification
    std::complex<type_data> golden_output_file[SSR_FFT_L];

    // The output from the golden_output_file is transformed and stored in 2-D array depending on the choice of radix
    // and the length and stored in array golden_output that is finall used for verification function calls
    std::complex<type_data> golden_output[SSR_FFT_R][SSR_FFT_L / SSR_FFT_R];

    std::string root_path = "";        // Define root path where the input stimulus files are stored
    std::string export_root_path = ""; // Define root path where output log files will be stored

    std::ofstream logStreamBB;
    std::string blockBoxFilename = root_path + "hls_fft_blackBox_verification_log.vsum";
    logStreamBB.open(blockBoxFilename.c_str(), std::fstream::out);
    std::string inputStimulusDataVerifFileName;
    std::stringstream strStream;
    strStream << root_path << "fftStimulusIn_L" << SSR_FFT_L << ".verif";
    inputStimulusDataVerifFileName = strStream.str();

    std::cout << "The Stimulus file with Path : " << inputStimulusDataVerifFileName << "\n";

    std::stringstream strStream1;
    std::string goldenOutputDataVerifFileName;
    strStream1 << root_path << "fftGoldenOut_L" << SSR_FFT_L << ".verif";
    goldenOutputDataVerifFileName = strStream1.str();
    strStream1.str("");
    strStream1 << root_path << "RecursiveSSRFFT_int_output_R" << SSR_FFT_R << "_L" << SSR_FFT_L << ".verif";
    std::string RecInOutFileName;
    RecInOutFileName = strStream1.str();
    std::ofstream logStream;
    std::string verifLogFileName = root_path + "hls_fft_verification_log.vsum";
    std::string exportDataFile1 = export_root_path + "file1.data";

    // Read input stimulus from the file
    readComplexArrayFromFile<type_data>(logStreamBB, "din_file", inputStimulusDataVerifFileName, din_file, SSR_FFT_L);
    // Read GOLDEN OUTPUT vectore for comparison
    readComplexArrayFromFile<type_data>(logStreamBB, "golden_output_file", goldenOutputDataVerifFileName,
                                        golden_output_file, SSR_FFT_L);

    // This loop will transform 1-D data read from the file in 2-D for passing to ssr fft function calls and verificaton
    for (int i = 0; i < SSR_FFT_L / SSR_FFT_R; i++) {
        for (int j = 0; j < SSR_FFT_R; j++) {
            din[j][i] = din_file[i * (SSR_FFT_R) + j];
            din_fix[j][i] = din_file[i * (SSR_FFT_R) + j];
            din_fix_c[j][i] = din_file[i * (SSR_FFT_R) + j];
            golden_output[j][i] = golden_output_file[i * (SSR_FFT_R) + j];
        }
    }

    /*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
     *
     *                          F
     *                             F
     *                                 T
     *                                 					*CALLs*
     *
     *++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
     */
    for (int no_ffts = 0; no_ffts < NO_DATA_FRAMES_TO_SIMULATE; no_ffts++) {
        /*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
         * CAll SSR FFT with complex double type : Reference model call to calculate FLOATING POINT fft that
         * will be used for verifying the implementation and then for comparison with FIXED POINT model to calculate
         *  the SNR
         * ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
         */
        fft<ssr_fft_fix_params>(din, dout_temp);
        fftOutputReorderSimulationModelOnly<SSR_FFT_R, SSR_FFT_L>(dout_temp, dout);

        /*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
         * CAll SSR FFT with complex ap_fixed type : This the actual model that will be synthesized, to generate RTL
         * for implementation
         * ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
         */
        fft_top(din_fix, dout_fix_temp);
        fftOutputReorderSimulationModelOnly<SSR_FFT_R, SSR_FFT_L>(dout_fix_temp, dout_fix);
        /*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
         * CAll SSR FFT with complex ap_fixed type : This function wraps the same C++ model that is wrapped in for
         * synthesis, this function is not synthesized it is only used to compare the final RTL ouput and the C++
         * output during cosimulation for verification. The comparison done with this output has no SIGNIFICANE when
         * the csim/ c++ simulation is run. But during RTL/cosim the output of this model will be used to compare
         * RTL model output and the c++ output ( Bit true verification of RTL model vs. C++ model)
         * ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
         */
        fft_top_c(din_fix_c, dout_fix_c_temp);
        fftOutputReorderSimulationModelOnly<SSR_FFT_R, SSR_FFT_L>(dout_fix_c_temp, dout_fix_c);
    }

    //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    // Verify FLOATING POINT Model Against the GOLDEN OUTPUT VECTORS////////////////////////////////////////////START
    double floating_point_model_snrDBs = snr<SSR_FFT_R, SSR_FFT_L / SSR_FFT_R>(golden_output, dout);
    std::cout << "\n\n\n++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n";
    std::cout << "----------------------------------------------------------------------\n";
    std::cout << "Verification Messages:\"Floating Point Model\" VS. GOLDEN OUTPUT Comparison\n";
    std::cout << "----------------------------------------------------------------------\n";
    std::cout << "----------------------------------------------------------------------\n\n";
    std::cout << "The Signal to Noise Ratio Compares Signal generated by \"FLOATING POINT C++ model\" and the \"GOLDEN "
                 "OUTPUT VECTORS \".\n";
    std::cout << "The SNR is           :"
              << "     " << floating_point_model_snrDBs << " db \n\n";

// exit(0);
#if 0
	std::cout<<"SSR Double Type vs: SSR FIXED POINT Type Comparison..........\n";
	SidebySidePrintComplexNumArrays<SSR_FFT_R,SSR_FFT_L/SSR_FFT_R>(dout,fix_fft_out);


	std::cout<<"SSR Double Type vs: Golden Expected output..........\n";
	SidebySidePrintComplexNumArrays<SSR_FFT_R,SSR_FFT_L/SSR_FFT_R>(dout,golden_output);
#endif

    std::cout << "Comparing C++ \"FLOATING POINT model Output\" with the expected \"GOLDEN OUTPUT\" generated from "
                 "Octave FFT model. \n\n";

    std::complex<tip_fftOutType>
        p_fftOutDataTemp[SSR_FFT_L]; // 1-D array used for conversion of fft_top output from 2-D 1-D
    for (int a1 = 0; a1 < (SSR_FFT_L / SSR_FFT_R); a1++) {
        for (int a2 = 0; a2 < SSR_FFT_R; a2++) {
            p_fftOutDataTemp[a1 * SSR_FFT_R + a2] = dout[a2][a1]; /// convert it to one day array for verification
        }
    }

    int vf3 = verifyArrayOutput<tip_fftOutType>(
        logStream, "GOLDEN OUTPUT", goldenOutputDataVerifFileName, p_fftOutDataTemp, MAX_PERCENT_ERROR_IN_SAMPLE,
        MAX_ALLOWED_PERCENTAGE_OF_SAMPLES_IN_ERROR, SSR_FFT_L); // error given in %

    exportArrayToFile<tip_fftOutType, SSR_FFT_L>(p_fftOutDataTemp,
                                                 RecInOutFileName); // export FIXED POINT fft ouput to file

    if (vf3 != 0) {
        std::cout << "\n\nVerification of C++ FLOATING POINT model Failed..... FAIL " << std::endl;

    } else {
        std::cout << "\n\n\nVerification of C++ FLOATING POINT model PASSED..... SUCCESS " << std::endl;
        std::cout << "VERIFIED.....\n" << std::endl;
    }
    std::cout << "----------------------------------------------------------------------\n";
    std::cout << "----------------------------------------------------------------------\n";
    std::cout << "----------------------------------------------------------------------\n";
    std::cout << "+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n\n\n";

    std::cout << "\n\n\n++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n";
    std::cout << "----------------------------------------------------------------------\n";
    std::cout << "Verification Messages:\"FIXED POINT Model\" VS. GOLDEN OUTPUT Comparison\n";
    std::cout << "----------------------------------------------------------------------\n";
    std::cout << "----------------------------------------------------------------------\n";
    std::cout << "Comparing \"C++ FIXED POINT model\" output with the expected \"GOLDEN OUTPUT\" generated from Octave "
                 "FFT model \n";

    /*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
     * Cast complex<ap_fixed> to complex <double> for comparison with reference double output
     *++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
     */
    std::complex<type_data> fix_fft_out[SSR_FFT_R][SSR_FFT_L / SSR_FFT_R];
    cast_to_double<SSR_FFT_R, SSR_FFT_L / SSR_FFT_R>(dout_fix, fix_fft_out);
    double fix_point_model_snrDBs = snr<SSR_FFT_R, SSR_FFT_L / SSR_FFT_R>(golden_output, fix_fft_out);
    std::cout << "The Signal to Noise Ratio Compares Signal generated by \"FLOATING POINT C++ model\" and the \"GOLDEN "
                 "OUTPUT VECTORS \".\n";
    std::cout << "The SNR is           :"
              << "     " << fix_point_model_snrDBs << " db \n\n";

    for (int a1 = 0; a1 < (SSR_FFT_L / SSR_FFT_R); a1++) {
        for (int a2 = 0; a2 < SSR_FFT_R; a2++) {
            p_fftOutDataTemp[a1 * SSR_FFT_R + a2] = dout_fix[a2][a1]; // convert to 1d array and also cast
        }
    }

    std::stringstream strFile;
    strFile.str("");
    std::cout << "Comparing C++ \"FIXED POINT model Output\" with the expected \"GOLDEN OUTPUT\" generated from Octave "
                 "FFT model. \n\n";

    int vf4 = verifyArrayOutput<tip_fftOutType>(logStream, "GOLDEN OUTPUT", goldenOutputDataVerifFileName,
                                                p_fftOutDataTemp, MAX_PERCENT_ERROR_IN_SAMPLE,
                                                MAX_ALLOWED_PERCENTAGE_OF_SAMPLES_IN_ERROR, SSR_FFT_L); // 50,5
    if (vf4 != 0) {
        std::cout << "Verification of \"C++ FIXED POINT model\" Failed..... FAIL " << std::endl;
    } else {
        std::cout << "\n\n\nVerification of \"C++ FIXED POINT model\" PASSED..... SUCCESS " << std::endl;
        std::cout << "VERIFIED.....\n\n" << std::endl;
    }
    std::cout << "----------------------------------------------------------------------\n";
    std::cout << "----------------------------------------------------------------------\n";
    std::cout << "----------------------------------------------------------------------\n";
    std::cout << "+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n\n\n";

    std::cout << "\n\n\n++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n";
    std::cout << "----------------------------------------------------------------------\n";
    std::cout << "Verification Messages:\"FIXED POINT Model\" VS. \"FLOATING POINT MODEL\" Comparison\n";
    std::cout << "----------------------------------------------------------------------\n";
    std::cout << "----------------------------------------------------------------------\n";
    std::cout << "Comparing \"C++ FIXED POINT model\" output with \"FLOATING POINT MODEL\" output \n";
    double fix_point_vs_floating_model_snrDBs =
        snr<SSR_FFT_R, SSR_FFT_L / SSR_FFT_R>(dout, fix_fft_out); // dour= ref signal and fix_fft_out= noisy signal
    std::cout << "The Signal to Noise Ratio Compares Signal generated by \"FLOATING POINT C++ model\" and the "
                 "\"FLOATING POINT MODEL\".\n";
    std::cout << "The SNR is           :"
              << "     " << fix_point_vs_floating_model_snrDBs << " db \n\n";
    std::cout << "----------------------------------------------------------------------\n";
    std::cout << "----------------------------------------------------------------------\n";
    std::cout << "----------------------------------------------------------------------\n";
    std::cout << "+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n\n\n";

    std::cout << "----------------------------------------------------------------------\n";
    std::cout << "----------------------------------------------------------------------\n";
    std::cout << "COSIM Relevant Verification Messages----------------------------------\n";
    std::cout << "----------------------------------------------------------------------\n";
    std::cout << "----------------------------------------------------------------------\n";
    std::cout << "Comparing RTL output with C++ FIXED POINT model, BIT-TRUE Model Verification \n";
    std::cout << "This message is useful for -----COSIM--- only otherwise it compare C++ model with itself......... \n";
    std::cout << "----------------------------------------------------------------------\n";
    std::cout << "----------------------------------------------------------------------\n";
    std::cout << "----------------------------------------------------------------------\n";
    std::cout << "----------------------------------------------------------------------\n";

    int rtl_verif_flag = 0;
    for (int lll = 0; lll < SSR_FFT_L / SSR_FFT_R; lll++) {
        for (int rrr = 0; rrr < SSR_FFT_R; rrr++) {
            if ((dout_fix[rrr][lll].real() != dout_fix_c[rrr][lll].real()) ||
                (dout_fix[rrr][lll].imag() != dout_fix_c[rrr][lll].imag())) {
                rtl_verif_flag++;
                std::cout << "Expected Output : " << dout_fix_c[rrr][lll] << "\n";
                std::cout << "RTL      Output : " << dout_fix[rrr][lll] << "\n";
            }
        }
    }
    if (rtl_verif_flag != 0) {
        std::cout << "\"C++ MODEL\" FIXED POINT output and the synthesized FIXED POINT \"RTL MODEL\" output does not "
                     "match verification failed..... FAIL "
                  << std::endl;
        std::cout << "Number of Mis-Matches=" << rtl_verif_flag << "\n";
    } else {
        std::cout << "FIXED POINT \"C++ MODEL\"  output and the synthesized FIXED POINT \"RTL MODEL\" outputs "
                     "matched..... SUCCESS "
                  << std::endl;
        std::cout << "VERIFIED.....\n" << std::endl;
    }
    std::cout << "----------------------------------------------------------------------\n";
    std::cout << "----------------------------------------------------------------------\n";
    std::cout << "----------------------------------------------------------------------\n";
    std::cout << "----------------------------------------------------------------------\n";

#if 0
	SidebySidePrintComplexNumArrays<SSR_FFT_R,SSR_FFT_L/SSR_FFT_R,type_data>(dout,fix_fft_out);
	std::cout<<"Comparison with FLOATING POINT output ....\n";
	SidebySidePrintComplexNumArrays<SSR_FFT_L/SSR_FFT_R,SSR_FFT_R,T_INNER_SSR_FFT_OUT,type_data>(dout_fix,dout);
#endif
    if ((vf3 | vf4 | rtl_verif_flag) == 0)
        std::cout << "\n$$$$$$$$$$$$$$$$$$$$$$\nOVERL ALL Simulation was a SUCCESS Done with L=" << SSR_FFT_L
                  << "  R=" << SSR_FFT_R << "\n$$$$$$$$$$$$$$$$$$$$$$$$$\n"
                  << std::endl;
    return (vf3 | vf4 | rtl_verif_flag);
}
#endif
#endif
//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
