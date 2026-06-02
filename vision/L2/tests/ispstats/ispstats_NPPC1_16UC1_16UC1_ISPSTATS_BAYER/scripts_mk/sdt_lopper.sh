#!/bin/bash

XSA="$1"
SDT_OUTPUT="$2"
BOARD_DTS="$3"
PROCESSOR="$4"
INPUT_DTSI="$5"
SDTGEN_TCL="../sdtgen.tcl"
export LOPPER_DTC_FLAGS="-b 0 -@"

touch ${SDTGEN_TCL}
cat > ${SDTGEN_TCL} << EOL

 for { set i 0 } { \$i < \$argc } { incr i } {
  # xsa path
  if { [lindex \$argv \$i] == "-xsa_path" } {
    incr i
    set xsa_path [lindex \$argv \$i]
  }
  # SDT path
  if { [lindex \$argv \$i] == "-sdt_path" } {
    incr i
    set sdt_path [lindex \$argv \$i]
  }
  #board dts name
  if { [lindex \$argv \$i] == "-board_dts" } {
    incr i
    set board_dts [lindex \$argv \$i]
  }
}

set_dt_param -debug enable
set_dt_param -dir \$sdt_path -zocl "enable"
set_dt_param -xsa \$xsa_path
set_dt_param -board_dts \$board_dts
generate_sdt
EOL

#create sdt_gen output
sdtgen ${SDTGEN_TCL} -xsa_path ${XSA} -sdt_path ${SDT_OUTPUT} -board_dts ${BOARD_DTS}

#create pl.dtsi
lopper -f --enhanced ${INPUT_DTSI} -- xlnx_overlay_dt ${PROCESSOR} full

#convert dtsi to dtb
dtc -O dtb -o pl.dtbo -b 0 -@ pl.dtsi
