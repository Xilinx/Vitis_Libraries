#!/bin/bash
EXE_FILE=$2
LIB_PROJ_ROOT=$3
XCLBIN_FILE=$4
echo "XCL_MODE=${XCL_EMULATION_MODE}"
if [ "${XCL_EMULATION_MODE}" != "hw_emu" ] 
then
    cp $LIB_PROJ_ROOT/common/data/sample.txt ./sample_run.txt
    cp $LIB_PROJ_ROOT/common/data/test.list ./test.list
    for ((i = 0 ; i < 10 ; i++))
    do
        find ./reports/ -type f | xargs cat >> ./sample_run.txt
    done

    echo -e "\n\n-----------Running both Compression and Decompression for large file-----------\n"
    cmd1="$EXE_FILE -l ./test.list -sx $XCLBIN_FILE"
    echo $cmd1
    $cmd1
fi
