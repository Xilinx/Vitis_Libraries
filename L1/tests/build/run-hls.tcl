####################
# A example to build and debug vivado_hls project
# vivado_hls -f ./build/run-hls.tcl "runCsim 1 runRTLsynth 0 runRTLsim 0 part vu9p dataType double dataWdith 64 resDataType int size 8192 3 logParEntries 4 opName amax runArgs '../out_test/data/app.bin'"
# navigate to csim/build and run
# gdb --args ./csime.exe path_to_app_bin/app.bin 8192
####################
set pwd [pwd]
set pid [pid]

set SDX_PATH $::env(XILINX_SDX)
set VIVADO_PATH $::env(XILINX_VIVADO)

set GCC_VERSION 6.2.0
set GCC_PATH "$VIVADO_PATH/tps/lnx64"
set BOOST_INCLUDE "$VIVADO_PATH/tps/boost_1_64_0"
set BOOST_LIB "$VIVADO_PATH/lib/lnx64.o"

array set opt {
  part    vu9p
  dataType int
  resDataType int
  dataWidth 32
  logParEntries 4
  pageSizeBytes 4096
  memWidthBytes 64
  instrSizeBytes 8
  maxNumInstrs 16
  instrPageIdx 0
  paramPageIdx 1
  statsPageIdx 2
  opName "amax"
  runCsim     1
  runRTLsynth   0
  runRTLsim     0
  runArgs "$pwd/../out_test/data/app.bin"
}

foreach arg $::argv {
  puts $arg
  foreach o [lsort [array names opt]] {
    if {[regexp "$o +(\\w+)" $arg unused opt($o)]} {
      puts "  Setting CONFIG  $o  [set opt($o)]"
    } elseif {[regexp "$o +\'(.*)\'" $arg unused opt($o)]} {
      puts "  Setting CONFIG  $o  [set opt($o)]"
    }
  }
}

puts "Final CONFIG"
set OPT_FLAGS "-std=c++11 "
foreach o [lsort [array names opt]] {
  if { [string match "run*" $o] == 0 } {
    puts "  Using CONFIG  $o  [set opt($o)]"
    append OPT_FLAGS [format {-D BLAS_%s=%s } $o $opt($o)]
  }
}

set CFLAGS_K "-I$pwd/../include/hw -I$pwd/../include/hw/xf_blas  -g -O0 $OPT_FLAGS"
set CFLAGS_H "$CFLAGS_K -I$pwd -I$pwd/sw/include -I$pwd/../.. -I$pwd/hw -I$BOOST_INCLUDE"

set proj_dir [format prj_hls_%s  $opt(part) ]
open_project $proj_dir -reset
set_top uut_top 
add_files $pwd/hw/$opt(opName)/uut_top.cpp -cflags "$CFLAGS_K"
add_files -tb $pwd/sw/src/test.cpp -cflags "$CFLAGS_H"
open_solution sol -reset
config_compile -ignore_long_run_time

if {$opt(part) == "vu9p"} {
  set_part {xcvu9p-fsgd2104-2-i} -tool vivado
} else {
  set_part {xcvu9p-flgb2104-2-i} -tool vivado
}

create_clock -period 3.333333 -name default


if {$opt(runCsim)} {
  puts "***** C SIMULATION *****"
  csim_design -ldflags "-L$BOOST_LIB -lboost_iostreams -lz -lrt -L$GCC_PATH/$GCC_VERSION/lib64 -lstdc++ -Wl,--rpath=$BOOST_LIB" -argv "$opt(runArgs)"
}

if {$opt(runRTLsynth)} {
  puts "***** C/RTL SYNTHESIS *****"
  csynth_design
  if {$opt(runRTLsim)} {
    puts "***** C/RTL SIMULATION *****"
    cosim_design -trace_level all -ldflags "-L$BOOST_LIB -lboost_program_options -lrt" -argv "$opt(runArgs)"
  }
}

exit
