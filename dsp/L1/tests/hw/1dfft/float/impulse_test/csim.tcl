open_project -reset prj_impulse_test_float
set_top fft_top
add_files src/data_path.hpp
add_files src/top_module.cpp -cflags "-I../1dfftFloat/"
add_files src/top_module.hpp
add_files -tb src/main.cpp -cflags "-I../1dfftFloat/"
open_solution -reset "solution1-float"
set_part {xcu200-fsgd2104-2-e}
create_clock -period 3.3 -name default
csim_design -clean
exit
