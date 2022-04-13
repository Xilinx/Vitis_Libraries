#!/bin/bash
EXE_FILE=$1
LIB_PROJ_ROOT=$2
XCLBIN_FILE=$3
echo "XCL_MODE=${XCL_EMULATION_MODE}"
if [ "${XCL_EMULATION_MODE}" != "hw_emu" ] 
then
    cp $LIB_PROJ_ROOT/common/data/sample.txt ./sample_run.txt
    cp $LIB_PROJ_ROOT/common/data/test.list ./test.list
    find ./reports/ -type f | xargs cat >> ./sample_run.txt
    split -b 100M ./sample_run.txt ./segment
    mv ./segmentaa ./sample_run.txt 
    rm -f segment*
    
#    echo -e "\n\n-----------Running only Compression-----------\n"
#    cmd1="$EXE_FILE -c ./sample_run.txt -xbin $XCLBIN_FILE"
#    echo $cmd1
#    $cmd1
#    echo -e "\n\n-----------Running only Decompression-----------\n"
#    cmd2="$EXE_FILE -d ./sample_run.txt.lz4 -xbin $XCLBIN_FILE"
#    echo $cmd2
#    $cmd2
    echo -e "\n\n-----------Running both Compression and Decompression-----------\n"
    cmd2="$EXE_FILE -l ./test.list -mcr 20 -xbin $XCLBIN_FILE"
    echo $cmd2
    $cmd2
#    echo -e "\n\n-----------Block Size: 256Kb-----------\n"
#    cmd1="$EXE_FILE -l ./test.list -xbin $XCLBIN_FILE -B 1"
#    echo $cmd1
#    $cmd1
#    echo -e "\n\n-----------Block Size: 1024Kb-----------\n"
#    cmd1="$EXE_FILE -l ./test.list -xbin $XCLBIN_FILE -B 2"
#    echo $cmd1
#    $cmd1
    
fi
