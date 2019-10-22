open_project -reset prj_impulse_test
set_top fft_top
add_files src/data_path.hpp
add_files src/top_module.cpp -cflags "-I../1dfftFix"
add_files src/top_module.hpp
add_files -tb src/main.cpp -cflags "-I../1dfftFix"
open_solution -reset "solution1"
set_part {xcu200-fsgd2104-2-e}
create_clock -period 10 -name default
csim_design -clean
csynth_design
cosim_design -trace_level port
#export_design -format ip_catalog
exit
