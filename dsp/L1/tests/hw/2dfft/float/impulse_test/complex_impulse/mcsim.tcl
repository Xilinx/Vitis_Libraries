open_project -reset prj_2dfft_impulse_test
set_top top_fft2d
add_files src/top_2d_fft_test.cpp -cflags "-I../../2dfftLib/"
add_files -tb ../../2dFFTVerificationData/d_2dFFTCrandomData/d_2dFFTCrandomData_2/fft2DGoldenOut_L1024.verif -cflags "-Wno-unknown-pragmas" -csimflags "-Wno-unknown-pragmas"
add_files -tb ../../2dFFTVerificationData/d_2dFFTCrandomData/d_2dFFTCrandomData_2/fft2DGoldenOut_L16.verif -cflags "-Wno-unknown-pragmas" -csimflags "-Wno-unknown-pragmas"
add_files -tb ../../2dFFTVerificationData/d_2dFFTCrandomData/d_2dFFTCrandomData_2/fft2DGoldenOut_L16384.verif -cflags "-Wno-unknown-pragmas" -csimflags "-Wno-unknown-pragmas"
add_files -tb ../../2dFFTVerificationData/d_2dFFTCrandomData/d_2dFFTCrandomData_2/fft2DGoldenOut_L256.verif -cflags "-Wno-unknown-pragmas" -csimflags "-Wno-unknown-pragmas"
add_files -tb ../../2dFFTVerificationData/d_2dFFTCrandomData/d_2dFFTCrandomData_2/fft2DGoldenOut_L4096.verif -cflags "-Wno-unknown-pragmas" -csimflags "-Wno-unknown-pragmas"
add_files -tb ../../2dFFTVerificationData/d_2dFFTCrandomData/d_2dFFTCrandomData_2/fft2DGoldenOut_L64.verif -cflags "-Wno-unknown-pragmas" -csimflags "-Wno-unknown-pragmas"
add_files -tb ../../2dFFTVerificationData/d_2dFFTCrandomData/d_2dFFTCrandomData_2/fft2DGoldenOut_L65536.verif -cflags "-Wno-unknown-pragmas" -csimflags "-Wno-unknown-pragmas"
add_files -tb ../../2dFFTVerificationData/d_2dFFTCrandomData/d_2dFFTCrandomData_2/fft2DStimulusIn_L1024.verif -cflags "-Wno-unknown-pragmas" -csimflags "-Wno-unknown-pragmas"
add_files -tb ../../2dFFTVerificationData/d_2dFFTCrandomData/d_2dFFTCrandomData_2/fft2DStimulusIn_L16.verif -cflags "-Wno-unknown-pragmas" -csimflags "-Wno-unknown-pragmas"
add_files -tb ../../2dFFTVerificationData/d_2dFFTCrandomData/d_2dFFTCrandomData_2/fft2DStimulusIn_L16384.verif -cflags "-Wno-unknown-pragmas" -csimflags "-Wno-unknown-pragmas"
add_files -tb ../../2dFFTVerificationData/d_2dFFTCrandomData/d_2dFFTCrandomData_2/fft2DStimulusIn_L256.verif -cflags "-Wno-unknown-pragmas" -csimflags "-Wno-unknown-pragmas"
add_files -tb ../../2dFFTVerificationData/d_2dFFTCrandomData/d_2dFFTCrandomData_2/fft2DStimulusIn_L4096.verif -cflags "-Wno-unknown-pragmas" -csimflags "-Wno-unknown-pragmas"
add_files -tb ../../2dFFTVerificationData/d_2dFFTCrandomData/d_2dFFTCrandomData_2/fft2DStimulusIn_L64.verif -cflags "-Wno-unknown-pragmas" -csimflags "-Wno-unknown-pragmas"
add_files -tb ../../2dFFTVerificationData/d_2dFFTCrandomData/d_2dFFTCrandomData_2/fft2DStimulusIn_L65536.verif -cflags "-Wno-unknown-pragmas" -csimflags "-Wno-unknown-pragmas"
add_files -tb src/main_2d_fft_test.cpp -cflags "-I../../2dfftLib/ -Wno-unknown-pragmas" -csimflags "-Wno-unknown-pragmas"
add_files -tb src/top_2d_fft_test.cpp -cflags "-I../../2dfftLib/ -Wno-unknown-pragmas" -csimflags "-Wno-unknown-pragmas"
open_solution -reset "solution1"
set_part {xcu200-fsgd2104-2-e}
create_clock -period 3.33 -name default
config_compile -no_signed_zeros=0 -unsafe_math_optimizations=0
config_sdx -target none
config_export -format ip_catalog -rtl verilog -vivado_optimization_level 2 -vivado_phys_opt place -vivado_report_level 0
config_rtl -encoding onehot -kernel_profile=0 -module_auto_prefix=0 -mult_keep_attribute=0 -reset control -reset_async=0 -reset_level high -verbose=0
set_clock_uncertainty 12.5%
csim_design -clean
exit
