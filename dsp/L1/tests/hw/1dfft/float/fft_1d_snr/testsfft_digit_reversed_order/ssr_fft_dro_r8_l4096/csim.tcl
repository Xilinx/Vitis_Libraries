source settings.tcl
open_project -reset prj_ssr_fft_reg_test_r8_l4096
set_top fft_top
add_files src/main.cpp -cflags "-I../../../1dfftFloat/ -I../../../commonFloat/"
add_files src/hls_ssr_fft_data_path.hpp
add_files src/DEBUG_CONSTANTS.hpp
add_files -tb src/main.cpp -cflags "-I../../../1dfftFloat/ -I../../../commonFloat/"
add_files -tb ../../../commonFloat/verif/fftStimulusIn_L4096.verif
add_files -tb ../../../commonFloat/verif/fftGoldenOut_L4096.verif
open_solution -reset "solution-reg-test-r8-l4096"
#set_part {xcu200-fsgd2104-2-e}
 set_part $XPART
create_clock -period 4 -name default

if {$CSIM == 1} {
 csim_design -clean
}
#csynth_design
#cosim_design -trace_level port
exit

