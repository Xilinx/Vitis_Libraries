source settings.tcl
open_project -reset prj_impulse_test
set_top fft_top
add_files src/data_path.hpp
add_files src/top_module.cpp -cflags "-I../1dfftFix"
add_files src/top_module.hpp
add_files -tb src/main.cpp -cflags "-I../1dfftFix"
open_solution -reset "solution1"
#set_part {xcu200-fsgd2104-2-e}
 set_part $XPART
create_clock -period 10 -name default
if {$CSIM == 1} {
 csim_design -clean
}
exit
