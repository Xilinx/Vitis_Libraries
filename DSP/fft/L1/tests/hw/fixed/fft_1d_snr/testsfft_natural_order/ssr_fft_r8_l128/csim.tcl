open_project -reset prj_ssr_fft_reg_test_r8_l128
set_top fft_top
add_files src/main.cpp -cflags "-I../../../../../../include/hw/xf_fft/fixed/"
add_files src/hls_ssr_fft_data_path.h
add_files src/DEBUG_CONSTANTS.h
add_files -tb  src/main.cpp -cflags "-I../../../../../../include/hw/xf_fft/fixed/"
add_files -tb ../../common/verif/fftStimulusIn_L128.verif
add_files -tb ../../common/verif/fftGoldenOut_L128.verif
open_solution -reset "solution-reg-test-r8-l128"
set_part {xczu29dr-ffvf1760-1-i}
create_clock -period 4 -name default

csim_design -clean
exit
