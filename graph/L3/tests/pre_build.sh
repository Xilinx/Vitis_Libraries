source $VITIS_ENV
source $VITIS_XRT_ENV
source $VITIS_XRM_ENV

curDir=$(dirname $0)
File1=$curDir/../lib/libgraphL3.so
File2=$curDir/../lib/libgraphL3.json
if [ ! -f "$File1" ] && [ ! -f "$File2" ]; then
    make -C $curDir/../lib libgraphL3 TARGET=hw_emu
fi
