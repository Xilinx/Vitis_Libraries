set CFLAG "-DBLAS_m=32 -DBLAS_n=32 -DBLAS_k=256 -DBLAS_dataType=int16_t -std=c++11"
open_project -reset prj_gemm
set_top uut_top
add_files uut_top.cpp -cflags "-I/proj/rdi/staff/liangm/GhEnterprice/xf_blas/L1/include/hw/  ${CFLAG}"
add_files -tb main.cpp -cflags "${CFLAG}"
open_solution -reset "sol"
set_part {xcu200-fsgd2104-2-e}
create_clock -period 3.33 -name default
csim_design
csynth_design
cosim_design -trace_level all -tool xsim
#export_design -flow impl -rtl verilog -format ip_catalog
exit
