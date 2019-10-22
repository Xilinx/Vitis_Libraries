source settings.tcl
open_project -reset prj_impulse_test
set_top fft_top
add_files src/data_path.hpp
add_files src/top_module.cpp -cflags "-I../../include/hw/vitis_fft/fixed/"
add_files src/top_module.hpp
add_files -tb src/main.cpp -cflags "-I../../include/hw/vitis_fft/fixed/"
open_solution -reset "solution1"
#set_part {xcu200-fsgd2104-2-e}
 set_part $XPART
create_clock -period 10 -name default
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
 
#export_design -format ip_catalog
exit
