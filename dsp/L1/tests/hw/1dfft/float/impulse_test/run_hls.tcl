source settings.tcl
open_project -reset prj_impulse_test_float
set_top fft_top
set_top fft_top
add_files src/data_path.h
add_files src/top_module.cpp -cflags "-I../1dfftFloat/"
add_files src/top_module.hpp
add_files -tb src/main.cpp -cflags "-I../1dfftFloat/"
open_solution -reset "solution1-float"
 set_part $XPART
create_clock -period 4 -name default

if {$CSIM == 1} {
 csim_design -clean
}
if {$CSYNTH == 1} {
 csynth_design
}
if {$COSIM == 1} {
 cosim_design 
} 
if {$VIVADO_SYN == 1} {
 export_design -flow syn -rtl verilog
}
if {$VIVADO_IMPL == 1} {
 export_design -flow impl -rtl verilog
}
if {$QOR_CHECK == 1} {
   puts "QoR check not implemented yet"
}
 
exit

