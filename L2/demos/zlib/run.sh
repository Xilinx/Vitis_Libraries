#!/bin/bash
EXE_FILE=$1
LIB_PROJ_ROOT=$2
XCLBIN_FILE=$3
echo "XCL_MODE=${XCL_EMULATION_MODE}"
if [ "${XCL_EMULATION_MODE}" != "hw_emu" ] 
then
    cp $LIB_PROJ_ROOT/common/data/sample.txt ./sample_run.txt
    cp $LIB_PROJ_ROOT/common/data/test.list ./test.list
     cp $LIB_PROJ_ROOT/common/data/cr_1072987 ./cr_1072987
    for ((i = 0 ; i < 10 ; i++))
    do
        find ./reports/ -type f | xargs cat >> ./sample_run.txt
    done
    
    echo -e "\n\n-----------Running Compression and Decompression for  a Tiny File---------\n" 
    cmd1="$EXE_FILE -v ./cr_1072987 -sx $XCLBIN_FILE"
    echo $cmd1
    $cmd1

    echo -e "\n\n-----------Running Compression and Decompression for large file-----------\n"
    cmd1="$EXE_FILE -l ./test.list -sx $XCLBIN_FILE"
    echo $cmd1
    $cmd1
fi
