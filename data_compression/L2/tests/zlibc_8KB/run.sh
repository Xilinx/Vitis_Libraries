#!/bin/bash
EXE_FILE=$1
LIB_PROJ_ROOT=$2
XCLBIN_FILE=$3
echo "XCL_MODE=${XCL_EMULATION_MODE}"
if [ "${XCL_EMULATION_MODE}" != "hw_emu" ] 
then
    cp $LIB_PROJ_ROOT/common/data/sample.txt ./sample_run.txt
    find ./reports/ -type f | xargs cat >> ./sample_run.txt
    split -b 100M ./sample_run.txt ./segment
    mv ./segmentaa ./sample_run.txt 
    rm -f segment*
       echo -e "\n\n-----------Running Compression for large file-----------\n"
    cmd1="$EXE_FILE -c ./sample_run.txt -xbin $XCLBIN_FILE -zlib 1"
    echo $cmd1
    $cmd1
fi
